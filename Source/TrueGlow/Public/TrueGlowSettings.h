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
 * Project Settings 页面由 TrueGlowEditor 模块通过 ISettingsModule 注册。
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

	// ---------------- General ----------------

	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (ToolTip = "总开关（与 tg.Enable CVar 相与）"))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (ToolTip = "选择预设后立刻把参数覆盖为该预设值，并自动存盘；之后手动改任何参数会变回 Custom"))
	ETrueGlowPreset Preset = ETrueGlowPreset::WuWa;

	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (ToolTip = "是否对 SceneCapture 渲染的视图生效"))
	bool bAllowSceneCapture = false;

	// ---------------- Bloom ----------------

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ClampMin = "0", ClampMax = "8"))
	bool bBloomEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ClampMin = "0", ClampMax = "8"))
	float BloomIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ClampMin = "0", ClampMax = "10"))
	float BloomThreshold = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ClampMin = "0", ClampMax = "1"))
	float BloomKnee = 0.6f;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ClampMin = "1", ClampMax = "6", ClampType = "int"))
	int32 BloomLevels = 6;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ClampMin = "0", ClampMax = "6"))
	float BloomBlurRadius = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ToolTip = "每级贡献权重（0=最细级）"))
	TArray<float> BloomLevelWeights;

	UPROPERTY(EditAnywhere, Config, Category = "Bloom", meta = (ToolTip = "每级色调（线性空间）"))
	TArray<FLinearColor> BloomLevelTints;

	// ---------------- Streak ----------------

	UPROPERTY(EditAnywhere, Config, Category = "Streak")
	bool bStreakEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Streak", meta = (ClampMin = "0", ClampMax = "4"))
	float StreakIntensity = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "Streak")
	FLinearColor StreakTint = FLinearColor(0.55f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Config, Category = "Streak", meta = (ClampMin = "16", ClampMax = "2048"))
	float StreakLength = 480.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Streak", meta = (ClampMin = "1", ClampMax = "16"))
	float StreakThickness = 2.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Streak", meta = (ClampMin = "0.05", ClampMax = "1"))
	float StreakAttenuation = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "Streak", meta = (ClampMin = "1", ClampMax = "8", ClampType = "int"))
	int32 StreakPasses = 4;

	// ---------------- Glare ----------------

	UPROPERTY(EditAnywhere, Config, Category = "Glare")
	bool bGlareEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Glare", meta = (ClampMin = "0", ClampMax = "4"))
	float GlareIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Config, Category = "Glare")
	FLinearColor GlareTint = FLinearColor(1, 1, 1, 1);

	UPROPERTY(EditAnywhere, Config, Category = "Glare", meta = (ClampMin = "4", ClampMax = "6", ClampType = "int"))
	int32 GlareDirections = 4;

	UPROPERTY(EditAnywhere, Config, Category = "Glare", meta = (ClampMin = "4", ClampMax = "128"))
	float GlareRadius1 = 24.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Glare", meta = (ClampMin = "8", ClampMax = "512"))
	float GlareRadius2 = 72.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Glare", meta = (ClampMin = "2", ClampMax = "16", ClampType = "int"))
	int32 GlareTaps = 6;
};

/** 把某套预设值写入 Settings（不负责存盘）。TrueGlowPresets.cpp 实现。 */
namespace TrueGlowPresets
{
	TRUEGLOW_API void Apply(ETrueGlowPreset Preset, UTrueGlowSettings& OutSettings);
	TRUEGLOW_API bool ApplyByName(const FString& PresetName, UTrueGlowSettings& OutSettings);
}
