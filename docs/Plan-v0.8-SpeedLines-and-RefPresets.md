# TrueGlow v0.8 计划 —— 速度线 SpeedLines + 参考图四预设

> 状态：待实施（本文档为实施蓝本，按里程碑逐步落地）
> 前置：v0.7（GodRays，commit f5bbb71）已交付，12 效果管线全绿。

---

## 0. 背景：四张参考图的可行性结论

用户提供 4 张游戏截图，逐张拆解后的结论：

| 图 | 光源 | 特效本体（**不归辉光管**） | 辉光层（TrueGlow 可做） |
|---|---|---|---|
| ① ∞ 能量环（蓝） | 蓝色丝状光带 | ∞ 形 Ribbon 缠绕（Niagara/模型） | 大范围蓝晕、横向冰蓝光条、交叉点星芒、彩虹边、小幻影 |
| ② 红色弹道（红） | 能量弹拖尾 | 弹道本体 | 白核+红晕、红雾散射、横向红光条、**多条平行斜向速度线 ⚠️ 缺口** |
| ③ 红日穿云（红） | 红色太阳 | 建筑轮廓边缘光（场景 rim light 打光） | 亮核、大面积雾状散射、横向红色拉丝、云隙光束 |
| ④ 蓝色能量弹（蓝） | 法术弹头 | 蜂窝六边形点阵（粒子） | 白炽光核、蓝色体积雾罩、长锥形光束、横向蓝光条 |

**结论**：四图的辉光层，现有 12 效果 + 调参可覆盖 90%；唯一形态缺口是图②的
**任意角度、多根平行、间距可调的斜向细线（"速度线/弹道拖尾感"）**——现有横向/纵向
streak 与过中心对称的星芒镜都做不出。本计划补上该通道，并把四张图固化为一键预设。

---

## 1. 功能 A：速度线 SpeedLines（新通道，第 13 个效果）

### 1.1 视觉定义

围绕高亮源，沿任意角度 θ 呈扇状排布 N 条**平行**细光：中心线最长最亮，向外按
间距 Spacing 排开、逐条变短变淡（图②红色弹道的多线速度感）。

### 1.2 参数（8 个，进 `FTrueGlowParams` / `UTrueGlowSettings`）

| 字段 | 类型/默认 | 说明 |
|---|---|---|
| `bSpeedLines` | bool = false | 开关 |
| `SpeedLinesIntensity` | float = 0.6 | 总强度 |
| `SpeedLinesAngle` | float = -20° | 线的角度（度，0=水平，屏幕空间） |
| `SpeedLinesCount` | int32 = 5 | 线数 1-16（奇数为佳，中心线对称） |
| `SpeedLinesSpacing` | float = 14 | 相邻线间距（像素@1080） |
| `SpeedLinesLength` | float = 520 | 中心线长度（像素@1080，逐条向外衰减） |
| `SpeedLinesThickness` | float = 1.5 | 线粗（像素，垂直高斯 σ） |
| `SpeedLinesTint` | FLinearColor = (1,1,1) | 染色 |

> 阈值：速度线只对"极亮核心"起反应才有速度感，复用 `StreakBase`（光条级亮部图），
> 即天然吃 streak 自己的阈值链，不单设阈值参数（保持参数面克制）。

### 1.3 着色器方案（推荐 A，备选 B）

**方案 A：单 pass 专用着色器 `FTrueGlowSpeedLinesPS`（新 usf）**
- 分辨率：¼ res（与 streak/glare 同级）；输入复用已建好的 `MipTex[0..3]` 级联
  （断层根治同款：per-tap 几何级数距离 + `mip = log2(d/1.5)` 选级）。
- 每输出像素 UV，方向 `Dir = (cosθ, sinθ)`（**像素空间构造**再除以 QuarterSize 转
  UV 步进，保证屏幕上的斜率与角度参数一致、不被宽高比扭斜），`Perp = (-Dir.y, Dir.x)`。
- 双层循环：外层 k = -K..K（K = Count/2）条平行线，源采样坐标加垂直偏移
  `SrcUV = UV + Perp·(k·Spacing)`；逐条强度 `exp(-|k|·0.55)`、长度
  `Length·exp(-|k|·0.35)`（外圈更短更淡，图②同款观感）。
- 内层 24 tap 沿 Dir 正负双向几何级数采样（抄 `TrueGlowStreakMip.usf` 的 tap/mip
  选择逻辑）；每 tap 垂直方向再加 3 tap 小高斯（σ=Thickness）控制线粗，近端
  强制 mip0 保持锐利。
- 输出：自己的 ¼ res 累加纹理 `SpeedAccum`（PF_FloatRGB），Composite 新增一项
  `SpeedLinesTexture + SpeedLinesIntensity` 输入加法合成。
- SM5 动态循环（Count/Spacing 均为 uniform），单 pass 完成，无中间纹理往返。

**方案 B（fallback）：链式复用 `AddMipStreakPass`**
- 给 `TrueGlowStreakMip.usf` 加一个 `MipSampleOffset`（仅作用于 mip 采样的 UV 偏移，
  不影响 AccumTexture 与输出位置），每条线一个链式 pass（SelfTint 编码逐条衰减）。
- 优点：几乎零新着色器代码；缺点：N 个 RDG pass、N 次全 ¼ res 读写，Count 大时贵。
- 若方案 A 着色器编译踩坑（4.26 SM5 动态循环限制）则退到 B。

### 1.4 管线插入点

```
BrightPass → Mip 级联(MipTex0-3) → Bloom 金字塔 → Streak H/V
          → 星芒镜 → ★SpeedLines(读 MipTex, 写 SpeedAccum)★ → Flare → GodRays → Composite
```
- 位置在星芒镜之后、Flare 之前（与星芒镜同为"读 MipTex 的长线效果"，归同一
  `RDG_GPU_STAT_SCOPE`；新增 `DECLARE_GPU_STAT_NAMED(TrueGlowSpeedLines, "TrueGlow.SpeedLines")`）。
- 门控：`bSpeedLines && SpeedLinesIntensity > 0.001 && (StreakBase || QuarterBase)`
  （与星芒镜同款守卫；MipTex 仅在 StreakBase 存在时有效，条件一致）。

### 1.5 顺手修复（本功能依赖/相关）

- **BloomLevels 管线 clamp 放宽**：`TrueGlowViewExtension.cpp` 中
  `FMath::Clamp(P.BloomLevels, 1, 6)` → `1, 8`，同时金字塔临时数组
  `TInlineAllocator<6>` ×2 → `TInlineAllocator<8>`。设置面板早已放宽到 8，管线却
  截到 6（实测发现的不一致）；红日预设需要 8 级超宽晕。

---

## 2. 功能 B：四张参考图 → 4 个新预设

`ETrueGlowPreset` 新增 4 值（DisplayName 中英双语，对齐现有风格）：

| 枚举 | DisplayName | 对应图 |
|---|---|---|
| `EnergyBlue` | "能量蓝 EnergyBlue" | ① ∞ 环 |
| `BallisticRed` | "弹道红 BallisticRed" | ② 红弹道 |
| `RedSun` | "红日 RedSun" | ③ 红日穿云 |
| `TechBlue` | "科技蓝 TechBlue" | ④ 蓝能量弹 |

### 2.1 参数表（字段名与 `TrueGlowTypes.h`/`UTrueGlowSettings` 完全一致）

| 参数 | EnergyBlue | BallisticRed | RedSun | TechBlue |
|---|---|---|---|---|
| BloomIntensity | 1.5 | 2.0 | 2.5 | 1.8 |
| BloomThreshold | 1.0 | 1.2 | **0.6** | 1.0 |
| BloomBrightMultiplier | 1.1 | **1.5** | 1.2 | 1.2 |
| BloomLevels | 6 | 6 | **8** | 6 |
| FilmSoftIntensity | 0.40 | 0.60 | **0.80** | 0.50 |
| DualTintStrength | 0.30 | 0.25 | 0.45 | 0.30 |
| ChromaticDispersion | 0.25 | 0.10 | 0.15 | 0.20 |
| bStreakEnabled / Intensity | ✓ 0.50 | ✓ 0.60 | ✓ 0.55 | ✓ 0.60 |
| StreakTint | (0.55,0.75,1) | **(1,0.3,0.2)** | **(1,0.35,0.25)** | (0.45,0.70,1) |
| StreakLength | 600 | 560 | **800** | 520 |
| bStarFilter / Intensity / Rays | ✓ 0.4 / 6 | ✗ | ✗ | ✓ 0.25 / 4 |
| GhostIntensity | 0.30 | 0.20 | 0.15 | 0.25 |
| HaloIntensity | 0.10 | ✗ | ✗ | 0.15 |
| bSpeedLines（新） | ✗ | **✓ Int 0.7 / 角 -20° / 5 根 / 间距 14 / 长 520 / (1,0.25,0.15)** | ✗ | ✗ |
| bGodRays（新启用） | ✗ | ✗ | **✓ 0.8 / Tint(1,0.55,0.35)** | **✓ 0.7 / Tint(0.5,0.75,1)** |
| GlareIntensity | 0.35 | 0.20 | 0.15 | 0.30 |
| 其余（Dirt/PolyIris/Fan/DualLine/Vertical） | ✗ | ✗ | ✗ | ✗ |

- `ApplyByName` 别名扩展：`energy/energyblue`、`ballistic/ballisticred`、`redsun`、
  `tech/techblue`。
- 预设公共段（Apply 开头的基线重置块）追加 SpeedLines 8 字段默认值，保证切换预设
  不残留上一预设的速度线参数（与 GodRays 字段同款处理）。

### 2.2 蓝图库

`UTrueGlowBlueprintLibrary` 追加 `SetSpeedLines*`（Enabled/Intensity/Angle/Count/
Spacing/Length/Thickness/Tint，8 个 setter，风格对齐现有 GodRays 系列）。

---

## 3. 文件改动清单（锚点已核对）

| # | 文件 | 改动 |
|---|---|---|
| 1 | `Source/TrueGlowShaders/Public/TrueGlowTypes.h` | `FTrueGlowParams` 末尾（Glare 块后）加 SpeedLines 8 字段 |
| 2 | `Source/TrueGlow/Public/TrueGlowSettings.h` | 预设枚举 +4（行 10-16）；新分类"速度线 Speed Lines"8 参数（双语 DisplayName）；Clamp：Count 1-16 / Spacing 2-64 / Length 16-2000 / Thickness 0.25-8 / Angle -90~90 |
| 3 | `Source/TrueGlowShaders/Private/TrueGlowShaders.h` | +`FTrueGlowSpeedLinesPS`（SHADER_USE_PARAMETER_STRUCT，IMPLEMENT_GLOBAL_SHADER 指向 `/Plugin/TrueGlow/Private/TrueGlowSpeedLines.usf`）；`FTrueGlowCompositePS::FParameters` +`SpeedLinesTexture/Sampler/Intensity` |
| 4 | `Shaders/Private/TrueGlowSpeedLines.usf`（新） | 方案 A 着色器（§1.3） |
| 5 | `Shaders/Private/TrueGlowComposite.usf` | 合成加一项 speedlines 贡献（与 Flare/GodRays 同款加法） |
| 6 | `Source/TrueGlowShaders/Private/TrueGlowViewExtension.cpp` | +GPU stat；星芒镜块后（行 ~515，flare 之前）插 SpeedLines pass；Composite 参数绑定；**BloomLevels clamp 1,6→1,8 + TInlineAllocator 6→8（§1.5）** |
| 7 | `Source/TrueGlow/Private/TrueGlowPresets.cpp` | 公共基线块 + SpeedLines 默认值；switch +4 case（§2.1 表）；ApplyByName +4 别名 |
| 8 | `Source/TrueGlow/Public+Private/TrueGlowBlueprintLibrary.*` | +8 个 SetSpeedLines* |
| 9 | `docs/TechnicalDoc.md` / `README.md` | 效果清单 12→13；预设 3→7；参数表补速度线 |
| 10 | `TrueGlow.uplugin` | VersionName 0.7 → 0.8 |

**UTF-8 BOM 铁律**：所有新/改 .h/.cpp/.usf 必须 BOM 头（4.26 无 /utf-8 编译旗，
MSVC 靠 BOM 识别中文注释，丢 BOM = 乱码/编译错）。

---

## 4. 里程碑（每步可编译可验证）

| # | 内容 | 验证 |
|---|---|---|
| M1 | 参数面先行：Types/Settings/枚举/预设表/蓝图 setter 全部落位（管线未消费） | 编译过，面板出现"速度线"分类、预设下拉 7 项、无乱码 |
| M2 | BloomLevels clamp 放宽（§1.5 独立小改） | BloomLevels=8 冒烟无越界，金字塔 8 级日志 |
| M3 | SpeedLines 着色器 + 管线 pass + Composite 合成 | 编译过；测试场景自发光球 + tg.SpeedLines.* 实时调参冒烟 |
| M4 | 四预设落位 + ApplyByName | 切换 4 预设无残留参数；截图对比参考图观感 |
| M5 | 文档 + README + uplugin 版本号；git commit + push | GitHub 同步 |

冒烟命令（沿旧约）：
```
Build.bat UE4Editor Win64 Development -Project=D:\000_Workbench\UE\VFX_Project_4_26_Test\VFX_Project_4_26_Test.uproject -Module=TrueGlow -WaitMutex
# 双模块注意：TrueGlowShaders 改动需同编 TrueGlowShaders + TrueGlow + TrueGlowEditor 三次或整编
UE4Editor-Cmd.exe <uproject> -game -windowed -resx=1280 -resy=720 -log → grep "TrueGlow" / "Critical error"
```

**视觉验收标准（用户在编辑器内）**：
1. 弹道红预设 + 一个高亮自发光球 → 球周围出现 5 条 -20° 平行红细线，中心线最长；
2. 切其余 6 预设 → 无速度线残留；
3. 红日预设 BloomLevels=8 超宽红雾，无断层；GodRays 从太阳位置出光柱；
4. stat gpu 中 TrueGlow.SpeedLines ¼ res 单 pass，1080p 预算 ≤0.3ms。

---

## 5. 风险与对策

| 风险 | 对策 |
|---|---|
| SM5 动态双层循环（N×24+ tap）编译慢/寄存器压力 | Count>8 时强制退化为方案 B 链式 pass；或 Count 上限收 12 |
| ¼ res 对 1-2px 细线的采样走样 | Thickness 最小 0.25 但预置 σ 高斯 + 近端 mip0；实测糊则整 pass 升 ½ res（成本仍可控） |
| 斜向角度被宽高比扭斜 | 方向向量像素空间构造（§1.3），实现后用 45° 目测校验 |
| 预设切换参数残留 | 公共基线块全量重置 SpeedLines 字段（GodRays 同款教训） |
| 改 StreakMip.usf（方案 B 时）破坏现有 streak/星芒镜 | 方案 B 的 MipSampleOffset 默认 (0,0)，现有调用零行为变化；回归冒烟 streak+星芒镜 |

---

## 6. 明确不做（边界声明）

- **∞ 形丝带 / 蜂窝点阵 / 弹道本体**：特效本体（Niagara/模型/材质），不是辉光后
  处理能产生的形态——需要场景侧制作，辉光管负责让它们"发光"。
- **建筑轮廓边缘光（图③）**：rim light 是打光/材质层的事；后处理做深度描边发光
  是另一个功能（轮廓辉光），如需要单独立项。
- **真实光源位置自动追踪 GodRays**：仍走 `GodRaysLightPos` 蓝图驱动（SetGodRaysLightPosition
  已有），不自动找最亮像素（闪烁风险，刻意不做）。
