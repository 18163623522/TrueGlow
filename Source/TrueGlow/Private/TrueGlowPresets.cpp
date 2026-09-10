// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"

namespace TrueGlowPresets
{

static void DualTintDefaults(UTrueGlowSettings& S)
{
	S.DualTintStrength = 0.0f;
	S.WarmCoreColor = FLinearColor(1.00f, 0.95f, 0.85f, 1.0f);
	S.CoolFringeColor = FLinearColor(0.80f, 0.90f, 1.15f, 1.0f);
}

void Apply(ETrueGlowPreset Preset, UTrueGlowSettings& S)
{
	// 公共默认（鸣潮基线）
	S.bEnabled = true;
	S.bBloomEnabled = true;
	S.BloomThreshold = 1.0f;
	S.BloomKnee = 0.6f;
	S.BloomBrightMultiplier = 1.0f;
	S.BloomLevels = 6;
	S.BloomBlurRadius = 3.0f;
	S.GaussianIterations = 2;
	S.bBloomFastMode = false;
	S.ChromaticDispersion = 0.18f;
	S.bLensDirt = false;
	S.LensDirtIntensity = 1.0f;
	S.FilmSoftIntensity = 0.0f;
	S.FilmSoftRadius = 3.0f;
	DualTintDefaults(S);
	S.bStreakDualLine = false;
	S.StreakDualLineSeparation = 12.0f;
	S.StreakDualLineIntensity = 0.5f;
	S.BloomScale = FVector2D(1.0f, 1.0f);

	S.BloomLevelWeights = { 0.60f, 0.70f, 0.80f, 0.95f, 1.10f, 1.25f, 1.40f, 1.55f };
	S.BloomLevelTints = {
		FLinearColor(0.95f, 0.98f, 1.00f, 1.0f),
		FLinearColor(0.90f, 0.95f, 1.05f, 1.0f),
		FLinearColor(0.85f, 0.92f, 1.10f, 1.0f),
		FLinearColor(0.80f, 0.90f, 1.15f, 1.0f),
		FLinearColor(0.75f, 0.87f, 1.20f, 1.0f),
		FLinearColor(0.70f, 0.85f, 1.25f, 1.0f),
		FLinearColor(0.65f, 0.83f, 1.30f, 1.0f),
		FLinearColor(0.60f, 0.80f, 1.35f, 1.0f),
	};

	S.bStreakEnabled = true;
	S.StreakTint = FLinearColor(0.55f, 0.75f, 1.00f, 1.0f);
	S.StreakThickness = 2.0f;
	S.StreakAttenuation = 0.35f;
	S.StreakPasses = 4;
	S.bStreakOwnThreshold = false;
	S.StreakThreshold = 2.0f;
	S.bStreakVerticalEnabled = false;
	S.StreakVerticalIntensity = 0.25f;
	S.StreakVerticalLength = 240.0f;

	S.GhostIntensity = 0.0f;
	S.GhostCount = 4;
	S.GhostSpacing = 0.45f;
	S.GhostDispersal = 1.0f;
	S.HaloIntensity = 0.0f;
	S.HaloRadius = 0.12f;
	S.FlareTint = FLinearColor(1, 1, 1, 1);
	S.bStarFilter = false;
	S.StarFilterIntensity = 0.8f;
	S.StarFilterRays = 4;
	S.StarFilterRotation = 15.0f;
	S.StarFilterLength = 720.0f;
	S.StarFilterTint = FLinearColor(1, 1, 1, 1);
	S.PolyIrisIntensity = 0.0f;
	S.PolyIrisSides = 6;
	S.PolyIrisRotation = 0.0f;
	S.PolyIrisRadius = 0.10f;
	S.FanIntensity = 0.0f;
	S.FanCount = 7;
	S.FanSpread = 40.0f;
	S.FanRadius = 0.18f;

	S.bGlareEnabled = true;
	S.GlareTint = FLinearColor(1, 1, 1, 1);
	S.GlareDirections = 4;
	S.GlareRadius1 = 24.0f;
	S.GlareRadius2 = 72.0f;
	S.GlareTaps = 6;

	switch (Preset)
	{
	case ETrueGlowPreset::WuWa:
		S.BloomIntensity = 1.00f;
		S.ChromaticDispersion = 0.18f;
		S.FilmSoftIntensity = 0.35f;
		S.DualTintStrength = 0.35f;
		S.StreakIntensity = 0.35f;
		S.StreakLength = 480.0f;
		S.GlareIntensity = 0.25f;
		break;

	case ETrueGlowPreset::Neon:
		S.BloomIntensity = 1.40f;
		S.BloomBrightMultiplier = 1.3f;
		S.StreakIntensity = 0.70f;
		S.StreakLength = 720.0f;
		S.StreakTint = FLinearColor(0.35f, 0.60f, 1.00f, 1.0f);
		S.StreakAttenuation = 0.25f;
		S.StreakPasses = 6;
		S.bStreakVerticalEnabled = true;   // 灯管上下漏光
		S.StreakVerticalIntensity = 0.40f;
		S.StreakVerticalLength = 300.0f;
		S.ChromaticDispersion = 0.30f;
		S.GhostIntensity = 0.35f;         // 幻影+光环
		S.HaloIntensity = 0.20f;
		S.bStarFilter = true;            // 星芒镜
		S.StarFilterIntensity = 0.6f;
		S.PolyIrisIntensity = 0.4f;      // 六边形光圈
		S.FanIntensity = 0.3f;           // 光谱扇
		S.bLensDirt = true;               // 脏镜头
		S.LensDirtIntensity = 0.8f;
		S.FilmSoftIntensity = 0.50f;
		S.DualTintStrength = 0.45f;
		S.bStreakDualLine = true;         // 双线横光
		S.StreakDualLineIntensity = 0.45f;
		S.StreakDualLineSeparation = 16.0f;
		S.GlareIntensity = 0.35f;
		S.GlareTint = FLinearColor(0.85f, 0.95f, 1.00f, 1.0f);
		break;

	case ETrueGlowPreset::Subtle:
		S.BloomIntensity = 0.60f;
		S.ChromaticDispersion = 0.08f;
		S.FilmSoftIntensity = 0.20f;
		S.DualTintStrength = 0.15f;
		S.BloomBlurRadius = 1.0f;
		S.StreakIntensity = 0.12f;
		S.StreakLength = 240.0f;
		S.GlareIntensity = 0.10f;
		break;

	case ETrueGlowPreset::Custom:
	default:
		break;
	}
}

bool ApplyByName(const FString& PresetName, UTrueGlowSettings& OutSettings)
{
	const FString Name = PresetName.ToLower();

	ETrueGlowPreset Preset = ETrueGlowPreset::Custom;
	if (Name == TEXT("wuwa") || Name == TEXT("鸣潮"))
	{
		Preset = ETrueGlowPreset::WuWa;
	}
	else if (Name == TEXT("neon"))
	{
		Preset = ETrueGlowPreset::Neon;
	}
	else if (Name == TEXT("subtle"))
	{
		Preset = ETrueGlowPreset::Subtle;
	}
	else
	{
		return false;
	}

	Apply(Preset, OutSettings);
	OutSettings.Preset = Preset;
	return true;
}

} // namespace TrueGlowPresets
