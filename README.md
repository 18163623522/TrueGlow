# TrueGlow

**UE 4.26 物理辉光插件** —— HDR 金字塔 Bloom + 变形镜头 Streak + 星芒 Glare，纯插件实现、不改引擎源码。

对标 Fab 的 REAL BLOOM；默认预设按**鸣潮**式辉光观感标定：宽半径高能量光晕、高亮点横向冰蓝光条、克制的点状星芒。效果直接在**编辑器视口**实时可见，无需 PIE。

> 📖 深入了解设计决策、每个 pass 的数学、4.26 插件开发七坑 → **[docs/TechnicalDoc.md](docs/TechnicalDoc.md)**

## 效果构成

| 通道 | 视觉 | 核心算法 |
|---|---|---|
| **Bloom** | 夜景霓虹/天空高亮整片呼吸感光晕，能量大不糊 | 软膝阈值 bright pass → 6 级金字塔（逐级高斯+染色）→ 9-tap tent 升采样合并 |
| **Streak（横向）** | 技能/武器高光拖出的横向冷蓝光条 | ¼ 分辨率 exp 衰减横向模糊迭代 + 纵向加粗，长度/粗细/衰减/次数可调 |
| **Streak（纵向）** | 灯管/霓虹上下漏光 | 独立开关与长度/强度的纵向衰减模糊组（衰减/色调/迭代与横向共用） |
| **Glare** | 点光源的短十字衍射星芒 | 4/6 方向 × 双半径采样，(1-t)² 权重 |

三通道独立开关、独立参数，在 **tonemap 之前的 HDR 线性段**合成（这就是"光追截图式"辉光
能量感的来源——物理正确的镜头眩光，而不是往屏幕上糊一层雾）。

## 快速开始

### 安装

1. 本目录（或其 junction/复制）放到目标工程 `Plugins/TrueGlow`
2. `.uproject` 启用：`"Plugins": [{ "Name": "TrueGlow", "Enabled": true }]`
3. 编译（本仓库绑定源码版 UE 4.26.2；引擎根自动探测，失败可设环境变量 `UE426_ENGINE_ROOT`）：
   ```bat
   <Engine>\Engine\Build\BatchFiles\Build.bat <Project>Editor Win64 Development ^
     -Project="<Project>.uproject" -Module=TrueGlowShaders -WaitMutex
   :: TrueGlow / TrueGlowEditor 同理
   ```
   ⚠️ `-Module=` 单模块编译不生成 `UE4Editor.modules` 清单，模块管理器会找不到模块——
   首次需手写（BuildId 抄 `<Engine>\Engine\Binaries\Win64\UE4Editor.modules`），或直接全量编译。

### 使用

| 操作 | 方式 |
|---|---|
| 调参 | **Window → TrueGlow** 面板窗口（唯一 UI 入口：预设按钮 + 全参数实时预览） |
| 预设 | 设置页 Preset 下拉：**鸣潮 WuWa**（默认）/ Neon 赛博 / Subtle 克制；或控制台 `tg.ApplyPreset Neon` |
| 开关对比 | `tg.Enable 0` / `1`（0 = 完全旁路零开销） |
| 关引擎 bloom | 控制台 `tg.SetupVolume`（防双重辉光，建议进关卡先跑一次） |
| 实时微调 | `tg.Bloom.Intensity`、`tg.Bloom.Threshold`、`tg.Bloom.Levels`、`tg.Streak.Intensity`、`tg.Streak.Length`、`tg.Glare.Intensity`（负值=不覆盖设置页） |

编辑器视口需开 **Realtime** 实时渲染即可见；PIE 与打包游戏同样生效。

**Bloom 细节控制**（对标 REAL BLOOM）：`亮部增益`（过阈值能量增益）、`模糊核缩放 XY`
（各向异性，做椭圆/竖向光雾）、`快速模式`（跳过每级模糊保帧率）。

**光条进阶**：`使用独立阈值`（光条只抓最亮的灯，不与泛光阈值共用——霓虹场景利器）、
**双线横光 DualLine**（主横光上下各一道平行细光，双线变形镜头光斑）、光条宽度最细 0.25px。
颜色采用**保亮度染色**（白色=保留源色，彩色只改色相不改能量，星芒同）。

**电影质感 Cinematic 套件**（各自独立开关）：
| 效果 | 说明 |
|---|---|
| 镜头色散 ChromaticDispersion | 辉光径向红蓝分离出彩虹边（镜头折射色散） |
| 脏镜头光晕 LensDirt | 程序化镜头污渍调制整体辉光（横向涂抹感） |
| 电影柔光层 FilmSoft | 宽半径低频能量清洗（胶片/梦境感） |
| 双色温渐染 DualTint | 亮芯偏暖、外晕偏冷（胶片色温分离） |

高斯感增强：每级模糊半径上限 8 + `高斯柔化迭代`（1-2 次 H/V 迭代，2≈真实高斯）。
`stat gpu` 里四组命名统计（Bloom/Streak/Glare/Composite）可直接看各通道耗时。

**全中文界面**：面板内全部参数与分类均中文显示。

**蓝图驱动**：`UTrueGlowBlueprintLibrary`（SetEnabled / SetBloomIntensity / SetStreakLength /
ApplyPreset / SetupEngineBloomOffVolume / SaveSettings 等静态函数），过场、游戏状态、
昼夜循环可在运行时动态调辉光，即时生效、不自动写盘。

## 架构一图流

```
SceneColor(HDR, TAA后) ─► BrightPass(软阈值,½res)
                           ├─► 金字塔L1..L6 ─► Tent升采样合并 ─┐
                           └─► ¼res基底 ─► Streak迭代 ─► Glare ─┤
SceneColor ─────────────────────────────────────────────────────┴─► 全分辨率合成 ─► tonemap 继续
```

注入点：`SceneViewExtension::SubscribeToPostProcessingPass(MotionBlur)` 的 after-pass 回调
（无条件执行、TAA 后曝光前 HDR 线性段）——REAL BLOOM 在 UE5 用 RDG 注入点做同样的事，
本插件证明了 4.26 上的等价路径。详见[技术文档 §2](docs/TechnicalDoc.md#2-渲染注入点分析)。

三模块加载相位（shader 声明必须 PostConfigInit、UCLASS 必须 Default，引擎双硬约束）：
`TrueGlowShaders`（PostConfigInit）/ `TrueGlow`（Default）/ `TrueGlowEditor`（PostEngineInit）。

## 性能

1080p 全开约 **0.5~1.5ms**（D3D11 SM5 像素着色器路线，无 compute 依赖）。降档：
金字塔级数、每级模糊、streak 迭代数、glare tap 数全部可调；`tg.Enable 0` 后仅剩一次布尔判断。

## 约束与兼容

- **Win64 + 源码版 UE 4.26.2**（Renderer 私有头 ABI 绑定，换引擎需回归验证）
- 辉光在直方图曝光之前合成，大幅增强会轻微影响自动曝光（物理合理，可用 ExposureBias 补偿）
- SceneCapture/缩略图默认不生效，`bAllowSceneCapture` 可放开

## 仓库结构

```
Source/TrueGlowShaders/   # 7 个全局 shader + 视图扩展 + RDG 管线 + CVar
Source/TrueGlow/          # 设置(UCLASS config) + 三预设 + tg.* 命令 + 蓝图函数库
Source/TrueGlowEditor/    # Window→TrueGlow 参数面板窗口（唯一 UI 入口）
Shaders/Private/           # 7 个 .usf + 公共函数库 .ush
docs/TechnicalDoc.md       # 深度技术文档（注入点证据链/三模块约束/pass 数学/七坑清单）
```

## License

MIT
