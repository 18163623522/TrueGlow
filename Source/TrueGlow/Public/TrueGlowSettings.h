// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrueGlowTypes.h"
#include "TrueGlowSettings.generated.h"

UENUM()
enum class ETrueGlowPreset : uint8
{
	Custom UMETA(DisplayName = "Custom 自定义"),
	WuWa   UMETA(DisplayName = "鸣潮 WuWa（默认推荐）"),
	Neon   UMETA(DisplayName = "Neon 赛博夜景"),
	Subtle UMETA(DisplayName = "Subtle 克制柔和"),
};

/**
 * TrueGlow 全局配置。
 * 注意：故意不继承 UDeveloperSettings——插件运行时模块在 Default 相位加载，
 * 而 DeveloperSettings 模块那时尚未进入进程，硬 import 会导致 DLL 无法被 OS 加载。
 * 唯一 UI 入口：编辑器 Window → TrueGlow 面板。
 */
UCLASS(config = Game, defaultconfig)
class TRUEGLOW_API UTrueGlowSettings : public UObject
{
	GENERATED_BODY()

public:
	static UTrueGlowSettings* Get() { return GetMutableDefault<UTrueGlowSettings>(); }

	/** 构建渲染线程快照（游戏线程调用）。 */
	FTrueGlowParams BuildParams() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// ---------------- 总控 ----------------

	UPROPERTY(EditAnywhere, Config, Category = "常规 General", meta = (DisplayName = "总开关 Enable", ToolTip = "总开关（与 tg.Enable CVar 相与）"))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "常规 General", meta = (DisplayName = "预设 Preset", ToolTip = "选择预设后立刻把参数覆盖为该预设值，并自动存盘；之后手动改任何参数会变回 Custom"))
	ETrueGlowPreset Preset = ETrueGlowPreset::WuWa;

	UPROPERTY(EditAnywhere, Config, Category = "常规 General", meta = (DisplayName = "作用于场景捕获 AllowSceneCapture", ToolTip = "是否对 SceneCapture 渲染的视图（反射、监控画面等）生效"))
	bool bAllowSceneCapture = false;

	// ---------------- 泛光 Bloom ----------------

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "启用泛光 BloomEnabled"))
	bool bBloomEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "泛光强度 BloomIntensity", ClampMin = "0", ClampMax = "8"))
	float BloomIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "泛光阈值 BloomThreshold", ToolTip = "亮度高于该值才产生辉光", ClampMin = "0", ClampMax = "10"))
	float BloomThreshold = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "阈值软膝 BloomKnee", ToolTip = "0=硬阈值，越大过渡越柔和", ClampMin = "0", ClampMax = "1"))
	float BloomKnee = 0.6f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "亮部提取增益 BrightMultiplier", ToolTip = "过阈值能量的倍率（bright-pass multiplier）", ClampMin = "0", ClampMax = "8"))
	float BloomBrightMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "金字塔层数 Levels", ToolTip = "1-6 级，越多光晕越宽", ClampMin = "1", ClampMax = "6", ClampType = "int"))
	int32 BloomLevels = 6;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "每级模糊半径 BlurRadius", ToolTip = "每级高斯模糊半径（像素），0 = 关", ClampMin = "0", ClampMax = "8"))
	float BloomBlurRadius = 3.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "高斯柔化迭代 GaussianIterations", ToolTip = "每级 H/V 高斯的迭代数：2 = 更柔更接近真实高斯（帧率敏感时改 1）", ClampMin = "1", ClampMax = "2", ClampType = "int"))
	int32 GaussianIterations = 2;

	// ---------------- 电影质感 Cinematic ----------------

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "镜头色散 ChromaticDispersion", ToolTip = "辉光径向红蓝分离出彩虹边（镜头折射色散）；0 = 关", ClampMin = "0", ClampMax = "1"))
	float ChromaticDispersion = 0.18f;

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "启用脏镜头光晕 LensDirt", ToolTip = "程序化镜头污渍调制整体辉光（横向涂抹感，电影常见的 dirty lens）"))
	bool bLensDirt = false;

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "脏镜头强度 DirtIntensity", EditCondition = "bLensDirt", ClampMin = "0", ClampMax = "3"))
	float LensDirtIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "电影柔光强度 FilmSoftIntensity", ToolTip = "宽半径低频柔光层（胶片/梦境般的能量清洗）；0 = 关", ClampMin = "0", ClampMax = "2"))
	float FilmSoftIntensity = 0.0f;

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "电影柔光半径 FilmSoftRadius", ClampMin = "1", ClampMax = "8"))
	float FilmSoftRadius = 3.0f;

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "双色温渐染强度 DualTintStrength", ToolTip = "胶片感双层色温：亮芯偏暖、外晕偏冷；0 = 关", ClampMin = "0", ClampMax = "1"))
	float DualTintStrength = 0.0f;

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "暖芯色 WarmCoreColor"))
	FLinearColor WarmCoreColor = FLinearColor(1.00f, 0.95f, 0.85f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "电影质感 Cinematic", meta = (DisplayName = "冷晕色 CoolFringeColor"))
	FLinearColor CoolFringeColor = FLinearColor(0.80f, 0.90f, 1.15f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "快速模式 FastMode", ToolTip = "跳过每级模糊保帧率（金字塔 tent 合并仍提供基础扩散）"))
	bool bBloomFastMode = false;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "模糊核缩放 BloomScale XY", ToolTip = "高斯核 X/Y 各向异性缩放：>1 拉长对应方向光晕（如 4,1 = 横向椭圆光雾）", ClampMin = "0.1", ClampMax = "4"))
	FVector2D BloomScale = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "各级权重 LevelWeights", ToolTip = "金字塔每级贡献权重（0 = 最细级）"))
	TArray<float> BloomLevelWeights;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "各级色调 LevelTints", ToolTip = "金字塔每级色调（线性空间）"))
	TArray<FLinearColor> BloomLevelTints;

	// ---------------- 光条 Streak ----------------

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "启用横向光条 Horizontal"))
	bool bStreakEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "横向强度 StreakIntensity", ClampMin = "0", ClampMax = "4"))
	float StreakIntensity = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "光条颜色 StreakTint", ToolTip = "白色 = 保留源色；彩色按保亮度方式染色（颜色改变、能量不变）"))
	FLinearColor StreakTint = FLinearColor(0.55f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "横向长度 StreakLength", ToolTip = "像素（按 1080 高度标定，随视口缩放）", ClampMin = "16", ClampMax = "2048"))
	float StreakLength = 480.0f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "光条宽度 Thickness", ToolTip = "可低至 0.25 的亚像素细线", ClampMin = "0.25", ClampMax = "16"))
	float StreakThickness = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "启用双线横光 DualLine", ToolTip = "在主横光上下各复制一道平行细光（双线变形镜头光斑）"))
	bool bStreakDualLine = false;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "双线间距 DualLineSeparation", EditCondition = "bStreakDualLine", ClampMin = "2", ClampMax = "64"))
	float StreakDualLineSeparation = 12.0f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "双线强度 DualLineIntensity", EditCondition = "bStreakDualLine", ClampMin = "0", ClampMax = "1"))
	float StreakDualLineIntensity = 0.5f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "衰减率 Attenuation", ToolTip = "越大光条收得越紧", ClampMin = "0.05", ClampMax = "1"))
	float StreakAttenuation = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "模糊迭代数 Passes", ClampMin = "1", ClampMax = "8", ClampType = "int"))
	int32 StreakPasses = 4;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "使用独立阈值 OwnThreshold", ToolTip = "开启后光条使用自己的阈值（只抓最亮的灯），不与泛光阈值共用", EditCondition = "bStreakEnabled"))
	bool bStreakOwnThreshold = false;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "光条阈值 StreakThreshold", EditCondition = "bStreakOwnThreshold", ClampMin = "0", ClampMax = "20"))
	float StreakThreshold = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "启用纵向光条 Vertical", ToolTip = "灯管上下漏光；衰减/颜色/迭代与横向共用"))
	bool bStreakVerticalEnabled = false;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "纵向强度 VerticalIntensity", ClampMin = "0", ClampMax = "4"))
	float StreakVerticalIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Config, Category = "变形光条 Streak", meta = (DisplayName = "纵向长度 VerticalLength", ClampMin = "16", ClampMax = "2048"))
	float StreakVerticalLength = 240.0f;

	// ---------------- 星芒 Glare ----------------

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "启用星芒 GlareEnabled"))
	bool bGlareEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "星芒强度 GlareIntensity", ClampMin = "0", ClampMax = "4"))
	float GlareIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "星芒颜色 GlareTint", ToolTip = "白色 = 保留源色；彩色按保亮度方式染色"))
	FLinearColor GlareTint = FLinearColor(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "方向数 Directions", ToolTip = "4 向（十字）或 6 向（六芒）", ClampMin = "4", ClampMax = "6", ClampType = "int"))
	int32 GlareDirections = 4;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "内圈半径 Radius1", ClampMin = "4", ClampMax = "128"))
	float GlareRadius1 = 24.0f;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "外圈半径 Radius2", ClampMin = "8", ClampMax = "512"))
	float GlareRadius2 = 72.0f;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "每方向采样数 Taps", ClampMin = "2", ClampMax = "16", ClampType = "int"))
	int32 GlareTaps = 6;
};

/** 把某套预设值写入 Settings（不负责存盘）。TrueGlowPresets.cpp 实现。 */
namespace TrueGlowPresets
{
	TRUEGLOW_API void Apply(ETrueGlowPreset Preset, UTrueGlowSettings& OutSettings);
	TRUEGLOW_API bool ApplyByName(const FString& PresetName, UTrueGlowSettings& OutSettings);
}
