# TrueGlow — UE4.26 物理辉光插件（鸣潮观感）

对标 Fab 的 REAL BLOOM，默认预设调出鸣潮式辉光：**超宽半径 HDR 金字塔 Bloom + 变形镜头横向 Streak + 点状星芒 Glare**。
纯插件实现，不改引擎源码；在 tonemap 之前的 HDR 线性段注入，观感与光线追踪截图的镜头眩光一致。

## 架构

```
SceneColor (HDR, TAA 后) ──► BrightPass(软阈值,½res)
                              ├─► 金字塔 L1..L6(降采样+高斯) ─► Tent 升采样合并 ─┐
                              └─► ¼res 基底 ─► Streak 迭代横向模糊(×N)+纵向加粗  ─┤
                                            └─► Glare 多方向双半径星芒           ├─► 全分辨率加法合成
SceneColor ──────────────────────────────────────────────────────────────────────┘
```

注入点：`ISceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass::MotionBlur)` 的
after-pass 回调（`PostProcessing.cpp:596` 无条件执行，位于 TAA 之后、曝光/引擎 bloom/tonemap 之前）。

### 三模块（关键坑，见下）

| 模块 | 类型/相位 | 内容 |
|---|---|---|
| `TrueGlowShaders` | Runtime / **PostConfigInit** | 7 个 FGlobalShader + 视图扩展 + CVar + 参数桥 |
| `TrueGlow` | Runtime / Default | `UTrueGlowSettings`(UCLASS config) + 预设 + 控制台命令 |
| `TrueGlowEditor` | Editor / PostEngineInit | Project Settings 页面注册 |

> 单模块无法同时满足两个引擎硬约束：
> ① `IMPLEMENT_GLOBAL_SHADER` 要求模块在 `InitializeShaderTypes` 之前加载完（否则静态初始化
> checkf 崩，加载器报 GetLastError=1114）；② UCLASS 注册要求 `UObjectInitialized()`（PostConfigInit
> 阶段断言崩）。因此 shader 声明与 UCLASS 必须分家 —— 引擎自己的 NiagaraShader 同款先例。
> 另外 PostConfigInit 时 `GEngine` 还没建，视图扩展必须延迟到 `OnPostEngineInit` 再创建。

## 安装

1. 本目录 junction/复制到目标工程 `Plugins/TrueGlow`（或引擎 `Engine/Plugins/Marketplace/`）
2. `.uproject` 的 Plugins 里启用 `"TrueGlow": true`
3. 编译：
   ```
   <Engine>/Engine/Build/BatchFiles/Build.bat <Project>Editor Win64 Development \
     -Project="<Project>.uproject" -Module=TrueGlowShaders -WaitMutex
   （TrueGlow / TrueGlowEditor 同理；或直接全量编译让 UBT 自动生成 manifest）
   ```
4. `-Module=` 单模块编译**不会**生成 `Binaries/Win64/UE4Editor.modules`，模块管理器会找不到模块；
   首次需手写一份（BuildId 抄引擎 `Engine/Binaries/Win64/UE4Editor.modules`），全量编译后 UBT 会覆盖为正版。

## 使用

- **总开关**：`tg.Enable 0/1`（0 = 完全旁路零开销）；Project Settings → Plugins → TrueGlow
- **预设**：设置页 Preset 下拉（WuWa 鸣潮[默认] / Neon 赛博 / Subtle 克制），或控制台
  `tg.ApplyPreset WuWa|Neon|Subtle`
- **关引擎自带 bloom**（避免双重辉光）：控制台 `tg.SetupVolume` 生成全局 PostProcessVolume 并置
  BloomIntensity=0
- **实时调参 CVar**：`tg.Bloom.Intensity/Threshold/Levels`、`tg.Streak.Intensity/Length`、
  `tg.Glare.Intensity`（负值=不覆盖设置页）

### 参数速查

| 通道 | 关键参数 |
|---|---|
| Bloom | Intensity / Threshold / Knee(软阈值膝) / Levels 1-6 / BlurRadius / 每级 Weight+Tint |
| Streak | Intensity / Tint(默认冰蓝) / Length(px@1080) / Thickness / Attenuation / Passes 1-8 |
| Glare | Intensity / Tint / Directions 4\|6 / Radius1/2 / Taps |

1080p 全开约 0.5~1.5ms（金字塔/streak pass 数均可降档）；SceneCapture 与缩略图默认过滤（`bAllowSceneCapture` 可放开）。

## 约束

- Renderer 私有头（ScreenPass.h / PostProcessMaterial.h）经 Build.cs 的引擎路径解析引入，
  **绑定本机源码版 UE 4.26.2**（`D:\SoftWave\UnrealEngine\UE4.26\UE_4.26`），引擎升级需回归。
- 我们合成后的辉光位于直方图曝光之前，会轻微影响自动曝光（物理上合理）；必要时用
  PostProcessVolume 的 ExposureBias 补偿。

## 目录

```
Source/TrueGlowShaders/  # shader 声明 + 视图扩展 + RDG 管线
Source/TrueGlow/         # 设置/预设/控制台命令
Source/TrueGlowEditor/   # 设置页注册
Shaders/Private/         # 7 个 .usf + TrueGlowCommon.ush
```
