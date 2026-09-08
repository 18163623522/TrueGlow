# TrueGlow 技术文档

> UE 4.26.2 物理辉光插件：HDR 金字塔 Bloom + 变形镜头 Streak + 星芒 Glare
> 对标 Fab 的 REAL BLOOM，默认预设按"鸣潮"式观感调校
>
> 本文写给两类读者：想了解**为什么这样设计**的渲染工程师（第 2-4 章、第 8 章），
> 和想**二次开发/移植**的人（第 6-7 章、第 9 章）。全部结论都有引擎源码行号背书。

---

## 目录

1. [效果定位：'光追感辉光'的本质](#1-效果定位)
2. [渲染注入点分析（4.26 源码证据链）](#2-渲染注入点分析)
3. [三模块加载相位架构（被迫的）](#3-三模块加载相位架构)
4. [渲染管线全解（每个 Pass 的数学）](#4-渲染管线全解)
5. [参数系统设计](#5-参数系统设计)
6. [Shader 代码导览](#6-shader-代码导览)
7. [性能模型](#7-性能模型)
8. [已知限制与设计权衡](#8-已知限制与设计权衡)
9. [UE4.26 插件渲染注入坑清单（七坑）](#9-坑清单)
10. [二次开发指南：加一个新通道](#10-二次开发指南)

---

## 1. 效果定位

路径追踪截图里高亮"高级"的辉光，不是光线追踪算出来的，而是**物理正确的镜头眩光（glare）**
三件套叠加，外加一条能量纪律：

| 要素 | 视觉 | TrueGlow 对应 |
|---|---|---|
| 宽动态能量扩散 | 夜景霓虹/天空高亮整片"呼吸感"光晕，能量大但不糊 | 金字塔 Bloom（阈值前 HDR 域、软膝部、能量归一） |
| 变形镜头光条 | 高亮点拉出横向冷白/冰蓝细长光条 | Anamorphic Streak（exp 衰减横向模糊迭代） |
| 光圈衍射星芒 | 点光源短十字/六芒 | Glare（4/6 方向 × 双半径采样） |
| 压得住的 roll-off | 辉光不把画面冲成一片白 | tonemap 前线性加法 + 可关引擎 bloom 防双重过曝 |

鸣潮观感的数值特征（默认预设 WuWa）：低阈值高能量 bloom（宽半径各级权重递增 0.6→1.25、
大半径级偏冷色 tint）、中等强度冰蓝 streak（480px@1080）、克制的白色星芒。

## 2. 渲染注入点分析

### 2.1 可选路径对比

| 路径 | HDR? | 多 pass? | 结论 |
|---|---|---|---|
| PostProcessMaterial @ BeforeTonemapping | ✓（HDR 线性） | ✗（单 pass，无中间 RT） | 质量上限低 |
| `PostRenderViewFamily_RenderThread`（渲染末尾钩子） | ✗（已 tonemap） | ✓ | HDR 阈值失真 |
| 修改引擎 FPostProcessing | ✓ | ✓ | 违背"纯插件"目标 |
| **`SubscribeToPostProcessingPass` after-pass 回调** | **✓** | **✓（FRDGBuilder 全权）** | **采用** |

### 2.2 证据链（UE 4.26.2 源码，行号实测）

**API 存在性**：`Engine/Public/SceneViewExtension.h`
- L93：`FUNC_DECLARE_DELEGATE(FAfterPassCallbackDelegate, FScreenPassTexture, FRDGBuilder&, const FSceneView&, const FPostProcessMaterialInputs&)`
- L101-108：`EPostProcessingPass { MotionBlur, Tonemap, FXAA, VisualizeDepthOfField, MAX }`
- L161：`virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) {}`

**引擎调用我们**：`Renderer/Private/PostProcess/PostProcessing.cpp` `FPostProcessing::Process`
- L434-446：对所有 ViewExtension 逐 pass 调 `SubscribeToPostProcessingPass(...)`，传入
  `PassSequence.GetAfterPassCallbacks(...)` 订阅数组与该 pass 是否启用
- **L596：`SceneColor = AddAfterPass(EPass::MotionBlur, SceneColor);` —— 不在任何 if 内，
  即使工程关闭动态模糊也执行**（这是选 MotionBlur 槽位的核心原因）
- L364-371：`AddAfterPass` 内部把 SceneColor/SeparateTranslucency/Velocity/SceneTextures 填入
  `FPostProcessMaterialInputs`，逐个 `AfterPassCallback.Execute(GraphBuilder, View, Inputs)`，
  返回值替换 SceneColor 继续链条

**注入点的色彩空间**（桌面延迟管线实测顺序，同一函数内）：

```
BeforeTranslucency 链 (L450)
  → DOF (L460) → BeforeTonemapping 材质链 (L478) → TAA (L493)
  → 【我们的回调 (L596)】← 此处 SceneColor = HDR 线性（预曝光已由采样侧乘 OneOverPreExposure 还原）
  → 半分辨率下采样 (L599) → 直方图曝光 (L620) → 引擎 Bloom mip 链 (L646)
  → Tonemap (L703, L701 处 SceneColorBeforeTonemap) → FXAA → AfterTonemapping 材质链
```

两个推论：
- **TAA 在我们之前**：streak/glare 源是已 AA 图像，无高频闪烁；代价是我们的辉光不再被 TAA 处理
  （辉光本身低频，无碍）
- **我们在直方图曝光之前**：辉光会轻微推高自动曝光（见 §8）

**渲染 API**：回调内用 `FPixelShaderUtils::AddFullscreenPass(GraphBuilder, GetGlobalShaderMap(...), ...)`
（RenderCore 公共工具），自带全屏三角 VS；shader 走全局 shader map，**完全不需要 FViewInfo**，
避免了对更多 Renderer 私有类型的依赖。需要的私有类型只有三个：`FScreenPassTexture`/
`FScreenPassTextureViewport`（`Renderer/Private/ScreenPass.h`）与 `FPostProcessMaterialInputs`
（`Renderer/Private/PostProcess/PostProcessMaterial.h`），符号多带 `RENDERER_API` 导出。

## 3. 三模块加载相位架构

单模块方案在 4.26 上会被**两个方向相反的引擎硬约束**夹死，必须拆模块：

| 模块 | 相位 | 内容 | 为什么是这个相位 |
|---|---|---|---|
| `TrueGlowShaders` | **PostConfigInit** | 7 个 FGlobalShader + 视图扩展 + CVar + 参数桥 | 约束① |
| `TrueGlow` | Default | `UTrueGlowSettings`(UCLASS config) + 预设 + `tg.*` 命令 | 约束② |
| `TrueGlowEditor` | PostEngineInit | ISettingsModule 注册设置页 | 编辑器服务就绪 |

### 约束①：shader 类型注册必须早于 InitializeShaderTypes

`RenderCore/Private/Shader.cpp` `FShaderType::FShaderType` 构造函数（L247 附近）：

```cpp
// This will trigger if an IMPLEMENT_SHADER_TYPE was in a module not loaded before InitializeShaderTypes
// Shader types need to be implemented in modules that are loaded before that
checkf(!bInitializedSerializationHistory, TEXT("Shader type was loaded after engine init, use ELoadingPhase::PostConfigInit on your module to cause it to load earlier."));
```

`IMPLEMENT_GLOBAL_SHADER` 生成全局静态对象，其构造在 **DLL 加载时**执行。Default 相位加载的
模块会命中这条 checkf → 静态初始化中途崩 → Windows 加载器报
**GetLastError=1114**（ERROR_DLL_INIT_FAILED，"couldn't be loaded by the OS"）——
这个错误码极易被误诊为"依赖 DLL 缺失"，实际是加载相位问题。引擎自带 shader 插件
（LensDistortion/OpenColorIO）全部用 PostConfigInit，即错误信息里写出的官方答案。

### 约束②：UCLASS 注册必须晚于 UObject 初始化

PostConfigInit 相位 `UObjectInitialized()` 尚为 false，含 `UCLASS` 的模块加载时
UHT 生成的注册代码在 `UObjectBase.cpp:145` 断言崩。所以 UCLASS 必须留在 Default 相位模块。

### 约束③：PostConfigInit 时 GEngine 尚未创建

`FSceneViewExtensions::RegisterExtension`（`SceneViewExtension.cpp:42`）ensure `GEngine` 非空。
解法：模块 StartupModule 里判断，未就绪则挂 `FCoreDelegates::OnPostEngineInit` 延迟创建视图扩展
（引擎模块同款做法），ShutdownModule 记得 RemoveAll 防悬挂。

### 跨模块参数桥

shader 模块（早加载）不能反向 import 主模块（晚加载，见 §9 坑5），所以用回调解耦：

```
TrueGlow(主模块) ──StartupModule 注册──► FKuroGlowShaderBridge::SetParamsProvider([]{ return Settings->BuildParams(); })
TrueGlowShaders(扩展) ──每帧游戏线程──► Bridge::GetParamsSnapshot() ──► 渲染线程快照
```

引擎先例：Niagara 的 `NiagaraShader` 模块就是同样的"shader 声明单独成模块 + 早相位"拆分。

## 4. 渲染管线全解

回调内一次构建，全 RDG、全像素着色器、`PF_FloatRGB`：

```
SceneColor(HDR, ViewRect)                          [全分辨率]
  │ ①BrightPass+½降采样：每 tap 软膝阈值后 4-tap 平均 × Tint₀
  ▼
BrightHalf                                         [½分辨率]
  ├─②金字塔：4-tap box 逐级降采样（Tintᵢ 逐级染色）→ L1..L5
  │    每级可分离高斯 H/V（半径 r=BlurRadius, σ=r/2）
  ├─③Tent 升采样合并：U[k] = L[k] + Tent9(U[k+1])·w[k+1]，从粗到细
  ▼
BloomResult(=U[0])
BrightHalf ──④4-tap 降采样──► QuarterBase          [¼分辨率]
  ├─⑤Streak：Passes 次横向模糊（±12 tap，exp(-|k|·attenuation) 权重，归一化）
  │            + 1 次纵向加粗（±2 tap）
  └─⑥Glare：Dirs(4|6) × Taps × 双半径采样，w=(1-t)²
  ▼
⑦Composite：Scene + Bloom·I_b + Streak·T_s·I_s + Glare·T_g·I_g   [全分辨率]
  │ 视口变换：GetScreenPassTextureViewportTransform 烘进 UVScaleBias
  ▼ 返回替换 SceneColor（ViewRect 重定为全幅，引擎下游用 viewport 参数自适应）
```

### 各 Pass 的数学

**① 软膝阈值**（Jimenez 风格，逐通道，分支消解为三段）：

```
x ≤ T-K : 0
x ≥ T+K : x - T
其余    : (x - T + K)² / (4K)        ← 膝区二次过渡，K→0 退化为硬阈值
```

**关键纪律：阈值在 4-tap 平均之前逐 tap 施加**——先平均再阈值会把亚像素尖峰抹掉，
尖峰正是 streak/glare 的能量源。

**③ Tent9**（COD:AW / Karis 式，9 次双线性覆盖 4×4 区域）：

```
权重矩阵 [1 2 1; 2 4 2; 1 2 1] / 16，texel 步长取低分辨率纹理
```

**⑤ Streak 衰减**：第 k 个 tap 权重 `exp(-|k|·attenuation)`，按总权重归一（能量守恒，
拉长不改总能量，只改分布）；迭代次数越多分布越平缓。步长
`step = Length·(QuarterHeight/1080) / (Passes·Taps)`，长度参数以 1080p 像素标定、随视口高度缩放。

**⑥ Glare**：每方向两圈采样（半径 r₁ 短/r₂ 长），`t = k/Taps ∈ (0,1]`，
权重 `(1-t)²`；4 方向步进 45°，6 方向步进 30°；按 `(Dirs·Taps·2)` 归一。

**⑦ 合成**：线性 HDR 域直接加法（tonemap 前的正确位置），
三个通道各自带 Tint×Intensity 预乘的 float4，禁用通道乘 0 并指向 4×4 黑色占位纹理。

### 分辨率与屏幕百分比

所有中间纹理按 `ViewRect.Size()` 的比例自建（等尺寸、Rect==Extent）；
只有对 SceneColor 的两次采样需要视口变换（引擎 `GetScreenPassTextureViewportTransform`），
天然兼容屏幕百分比/分屏。合成输出为精确尺寸新纹理，ViewRect 重定全幅——
与引擎自带 PPM 链输出行为一致，下游全部经 `FScreenPassTextureViewport` 自适应。

## 5. 参数系统设计

```
UTrueGlowSettings (UCLASS config=Game, defaultconfig)     ← Project Settings 编辑，存 DefaultEngine.ini
        │ BuildParams()：Settings 值 + CVar 覆盖(≥0 者) + clamp
        ▼
FKuroGlowParams（POD 快照，游戏线程每帧 BeginRenderViewFamily 刷新，临界区拷贝）
        ▼
渲染线程回调消费
```

- **双入口**：设置页（美术）与 `tg.*` CVar（技术/PIE 实时）并存，CVar 负值=不覆盖
- **预设机制**：设置页改 Preset 枚举 → `PostEditChangeProperty` 整套覆盖参数并 SaveConfig；
  手动改任何单项自动回落 Custom（保留用户值）
- 为什么 settings 是普通 `UCLASS(config)` 而不是 `UDeveloperSettings` 子类：见 §9 坑5
- SceneCapture/缩略图默认不生效（`View.bIsSceneCapture` 门控），`bAllowSceneCapture` 放开

## 6. Shader 代码导览

`Shaders/Private/`，7 个 usf + 1 个 ush，全部共享引擎 `ScreenPass.ush` 的 viewport 参数宏：

| 文件 | 入口 | 采样数 | 参数要点 |
|---|---|---|---|
| `TrueGlowCommon.ush` | （函数库） | — | `SoftThreshold`、`Tent9` |
| `TrueGlowBrightDownsample.usf` | MainPS | 4 | Threshold/Knee/Tint₀/InputUVScaleBias |
| `TrueGlowDownsample.usf` | MainPS | 4 | Tintᵢ/InputUVScaleBias |
| `TrueGlowBlurGaussian.usf` | MainPS | 2R+1 (R≤6) | Direction(1,0)/(0,1)、Sigma |
| `TrueGlowTentUpsampleAdd.usf` | MainPS | 9+1 | 双输入(Low/High)、Weight |
| `TrueGlowStreak.usf` | MainPS | 2T+1 (T=12) | Direction/StepPixels/Attenuation |
| `TrueGlowGlare.usf` | MainPS | Dirs·Taps·2 (≤192) | Radius1/2/TapsPerDirection/DirectionCount |
| `TrueGlowComposite.usf` | MainPS | 4 | Scene 视口变换 + 三通道 Tint×Intensity |

C++ 侧声明集中在 `Source/TrueGlowShaders/Private/TrueGlowShaders.h`（单 TU include，
`IMPLEMENT_GLOBAL_SHADER` 要求）。usf 的 include 一律用**虚拟绝对路径**
（`/Engine/Private/ScreenPass.ush`、`/Plugin/TrueGlow/Private/TrueGlowCommon.ush`），
相对路径只在同目录解析（§9 坑4）。

## 7. 性能模型

以 1080p（1920×1080，ViewRect）全开（Bloom 6 级+模糊、Streak 4+1 pass、Glare 4 向 6 tap）估算：

| Pass 组 | 分辨率 | 像素量(相对全屏) | 采样数/像素 |
|---|---|---|---|
| BrightPass | ½ | 25% | 4 |
| 降采样 ×5 | ¼→1/64 | ~6.6% | 4 |
| 高斯 H/V ×6 级 | ½→1/64 | ~33% | ≤13 |
| Tent 合并 ×5 | ½→1/32 | ~30% | 10 |
| Quarter 降采样 | ¼ | 25% | 4 |
| Streak ×5 | ¼ | 125% | 25 |
| Glare | ¼ | 25% | 48 |
| Composite | 全 | 100% | 4 |

合计约 3.7 倍全屏等效采样，中端 GPU 约 0.5~1.5ms。降档路径：
`BloomLevels`↓、`BloomBlurRadius=0`、`StreakPasses`↓、`GlareTaps`↓、`tg.Enable 0`（完全旁路，
仅剩 IsActiveThisFrame 布尔判断开销）。

## 8. 已知限制与设计权衡

1. **辉光进入自动曝光**：注入点在直方图之前（源码级事实），大幅调高 BloomIntensity 会压暗全局曝光。
   物理上说得通（真实眩光也进测光），必要时用 PostProcessVolume ExposureBias 补偿。
2. **绑定源码版 4.26.2**：Renderer 私有头 ABI。换引擎/升级需回归（Build.cs 的引擎根自动解析
   支持环境变量 `UE426_ENGINE_ROOT` 兜底）。
3. **双重 bloom**：引擎自带 bloom 在我们之后仍会跑，叠亮。`tg.SetupVolume` 一键生成全局
   PostProcessVolume 关闭之；或手动把 BloomIntensity 设 0。
4. **辉光不被 TAA**：源图已 AA，辉光自身是低频的，实测无闪烁；极端参数下的单像素星芒可能锯齿。
5. **SceneCapture 默认过滤**：反射/监控画面不吃辉光（可用 `bAllowSceneCapture` 放开，
   注意成本×捕获数量）。
6. 视觉参数为"鸣潮风"初版标定，未经逐场景美术验收，预设数值欢迎 PR。

## 9. 坑清单

4.26 上做"带自定义 shader 的渲染插件"的完整雷区，全部实测踩中过：

| # | 症状 | 根因 | 解法 |
|---|---|---|---|
| 1 | `-Module=` 编译后编辑器报 "Unable to load module ... 0 instances found" | 单模块编译不生成 `Binaries/Win64/UE4Editor.modules` 清单，ModuleManager 靠它发现插件模块 | 手写 JSON（BuildId 抄 `Engine/Binaries/Win64/UE4Editor.modules`）；全量编译后 UBT 自动覆盖 |
| 2 | Build.cs 里 C# 语法报奇怪的 CS1026 | UBT 规则编译器 ≈ C#5 | 禁用模式匹配 `is T x`、字符串插值 |
| 3 | `PrivateIncludePaths` 引擎私有头随机失效 | `Target.RelativeEnginePath` 对注册表关联引擎返回错误相对值 | 用 `Process.GetCurrentProcess().MainModule.FileName`（UBT 进程在引擎目录下）反推引擎根；`Assembly.Location` 被 CodeDom 影子加载清空，不可用 |
| 4 | 启动即 `ShaderCore.cpp:818 Fatal: Couldn't find source file of virtual shader path` | usf 的 include 用了相对路径 / IMPLEMENT 宏里的 usf 文件名与物理文件名不同步 | include 全部虚拟绝对路径；改插件名时 usf **文件**也要改名 |
| 5 | 运行时模块 DLL 报 GetLastError=126"找不到依赖" | import 了当时尚未加载的引擎模块 DLL（altered search path 不会去引擎目录找） | UDeveloperSettings 子类不能放 Default 相位运行时模块 → 普通 `UCLASS(config)` + Editor 模块注册 |
| 6 | 含 `IMPLEMENT_GLOBAL_SHADER` 的模块报 GetLastError=1114 | 加载晚于 InitializeShaderTypes，静态注册 checkf 崩（见 §3 约束①） | 模块相位 PostConfigInit |
| 7 | 同一模块 PostConfigInit 后 UObject 断言崩 / 注册视图扩展 ensure GEngine | UCLASS 要晚、shader 要早、GEngine 要再晚一点 | 三模块拆分 + `OnPostEngineInit` 延迟创建扩展 |

附加经验：无头 `-unattended` 冒烟时**工程里任何坏插件都会让引擎整进程退出**，掩盖你的真实
结果——用隔离空工程冒烟；`-ExecCmds` 的分号不切分参数（`"A B;C"` 会整体变成一个参数）。

## 10. 二次开发指南

以"加一个 Lens Dirt（污渍贴图）通道"为例，五步：

1. **shader**：`Shaders/Private/TrueGlowDirt.usf` —— 采样一张用户贴图 × BloomResult 调制；
   include 用虚拟绝对路径
2. **声明**：`TrueGlowShaders.h` 加 `FTrueGlowDirtPS`（BEGIN_SHADER_PARAMETER_STRUCT + 
   RENDER_TARGET_BINDING_SLOTS）+ `IMPLEMENT_GLOBAL_SHADER`，注意 usf 文件名与宏字符串一致
3. **管线**：`TrueGlowViewExtension.cpp` 回调里在 Composite 前插入 pass；贴图参数从
   `FKuroGlowParams` 带进来（UTexture→`GraphBuilder.RegisterExternalTexture`）
4. **参数**：`TrueGlowTypes.h` 快照结构体加字段 → `TrueGlowSettings.h/.cpp` 加 UPROPERTY +
   BuildParams 拷贝 → 需要实时调再加 `tg.Dirt.Intensity` CVar
5. **预设**：`TrueGlowPresets.cpp` 三套预设补默认值

验证流程照抄仓库的冒烟链：`-Module=` 编译 → 手写 modules 清单（若单模块编译）→
隔离工程 `-nullrhi` 验证加载 → 真 RHI 看 `first AfterMotionBlur` 日志 → GPU Profiler 里
`RDG_EVENT_SCOPE("TrueGlow")` 分组核对每 pass 耗时。
