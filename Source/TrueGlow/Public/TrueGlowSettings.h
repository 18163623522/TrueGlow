// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrueGlowTypes.h"
#include "TrueGlowSettings.generated.h"

UENUM()
enum class ETrueGlowPreset : uint8
{
	Custom UMETA(DisplayName = "Custom（手动调参）"),
	WuWa   UMETA(DisplayName = "鸣潮（默认推荐）"),
	Neon   UMETA(DisplayName = "Neon（赛博夜景）"),
	Subtle UMETA(DisplayName = "Subtle（克制柔和）"),
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

	UPROPERTY(EditAnywhere, Config, Category = "总控", meta = (DisplayName = "启用 TrueGlow", ToolTip = "总开关（与 tg.Enable CVar 相与）"))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "总控", meta = (DisplayName = "预设", ToolTip = "选择预设后立刻把参数覆盖为该预设值，并自动存盘；之后手动改任何参数会变回 Custom"))
	ETrueGlowPreset Preset = ETrueGlowPreset::WuWa;

	UPROPERTY(EditAnywhere, Config, Category = "总控", meta = (DisplayName = "对场景捕获生效", ToolTip = "是否对 SceneCapture 渲染的视图（反射、监控画面等）生效"))
	bool bAllowSceneCapture = false;

	// ---------------- 泛光 Bloom ----------------

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "启用泛光"))
	bool bBloomEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "泛光强度", ClampMin = "0", ClampMax = "8"))
	float BloomIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "泛光阈值", ToolTip = "亮度高于该值才产生辉光", ClampMin = "0", ClampMax = "10"))
	float BloomThreshold = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "阈值软膝", ToolTip = "0=硬阈值，越大过渡越柔和", ClampMin = "0", ClampMax = "1"))
	float BloomKnee = 0.6f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "亮部增益", ToolTip = "过阈值能量的倍率（bright-pass multiplier）", ClampMin = "0", ClampMax = "8"))
	float BloomBrightMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "金字塔级数", ToolTip = "1-6 级，越多光晕越宽", ClampMin = "1", ClampMax = "6", ClampType = "int"))
	int32 BloomLevels = 6;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "每级模糊半径", ToolTip = "每级高斯模糊半径（像素），0 = 关", ClampMin = "0", ClampMax = "6"))
	float BloomBlurRadius = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "快速模式", ToolTip = "跳过每级模糊保帧率（金字塔 tent 合并仍提供基础扩散）"))
	bool bBloomFastMode = false;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "模糊核缩放 XY", ToolTip = "高斯核 X/Y 各向异性缩放：>1 拉长对应方向光晕（如 4,1 = 横向椭圆光雾）", ClampMin = "0.1", ClampMax = "4"))
	FVector2D BloomScale = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "各级权重", ToolTip = "金字塔每级贡献权重（0 = 最细级）"))
	TArray<float> BloomLevelWeights;

	UPROPERTY(EditAnywhere, Config, Category = "泛光 Bloom", meta = (DisplayName = "各级色调", ToolTip = "金字塔每级色调（线性空间）"))
	TArray<FLinearColor> BloomLevelTints;

	// ---------------- 光条 Streak ----------------

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "启用横向光条"))
	bool bStreakEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "横向强度", ClampMin = "0", ClampMax = "4"))
	float StreakIntensity = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "光条颜色", ToolTip = "白色 = 保留源色；彩色按保亮度方式染色（颜色改变、能量不变）"))
	FLinearColor StreakTint = FLinearColor(0.55f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "横向长度", ToolTip = "像素（按 1080 高度标定，随视口缩放）", ClampMin = "16", ClampMax = "2048"))
	float StreakLength = 480.0f;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "光条粗细", ClampMin = "1", ClampMax = "16"))
	float StreakThickness = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "衰减率", ToolTip = "越大光条收得越紧", ClampMin = "0.05", ClampMax = "1"))
	float StreakAttenuation = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "迭代次数", ClampMin = "1", ClampMax = "8", ClampType = "int"))
	int32 StreakPasses = 4;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "启用独立阈值", ToolTip = "开启后光条使用自己的阈值（只抓最亮的灯），不与泛光阈值共用", EditCondition = "bStreakEnabled"))
	bool bStreakOwnThreshold = false;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "光条阈值", EditCondition = "bStreakOwnThreshold", ClampMin = "0", ClampMax = "20"))
	float StreakThreshold = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "启用纵向光条", ToolTip = "灯管上下漏光；衰减/颜色/迭代与横向共用"))
	bool bStreakVerticalEnabled = false;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "纵向强度", ClampMin = "0", ClampMax = "4"))
	float StreakVerticalIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Config, Category = "光条 Streak", meta = (DisplayName = "纵向长度", ClampMin = "16", ClampMax = "2048"))
	float StreakVerticalLength = 240.0f;

	// ---------------- 星芒 Glare ----------------

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "启用星芒"))
	bool bGlareEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "星芒强度", ClampMin = "0", ClampMax = "4"))
	float GlareIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "星芒颜色", ToolTip = "白色 = 保留源色；彩色按保亮度方式染色"))
	FLinearColor GlareTint = FLinearColor(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "方向数", ToolTip = "4 向（十字）或 6 向（六芒）", ClampMin = "4", ClampMax = "6", ClampType = "int"))
	int32 GlareDirections = 4;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "短半径", ClampMin = "4", ClampMax = "128"))
	float GlareRadius1 = 24.0f;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "长半径", ClampMin = "8", ClampMax = "512"))
	float GlareRadius2 = 72.0f;

	UPROPERTY(EditAnywhere, Config, Category = "星芒 Glare", meta = (DisplayName = "每方向采样数", ClampMin = "2", ClampMax = "16", ClampType = "int"))
	int32 GlareTaps = 6;
};

/** 把某套预设值写入 Settings（不负责存盘）。TrueGlowPresets.cpp 实现。 */
namespace TrueGlowPresets
{
	TRUEGLOW_API void Apply(ETrueGlowPreset Preset, UTrueGlowSettings& OutSettings);
	TRUEGLOW_API bool ApplyByName(const FString& PresetName, UTrueGlowSettings& OutSettings);
}
