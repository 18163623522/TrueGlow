// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"

namespace TrueGlowPresets
{

void Apply(ETrueGlowPreset Preset, UTrueGlowSettings& S)
{
	// 公共默认（鸣潮基线）
	S.bEnabled = true;
	S.bBloomEnabled = true;
	S.BloomThreshold = 1.0f;
	S.BloomKnee = 0.6f;
	S.BloomBrightMultiplier = 1.0f;
	S.BloomLevels = 6;
	S.BloomBlurRadius = 2.0f;
	S.bBloomFastMode = false;
	S.BloomScale = FVector2D(1.0f, 1.0f);

	S.BloomLevelWeights = { 0.60f, 0.70f, 0.80f, 0.95f, 1.10f, 1.25f };
	S.BloomLevelTints = {
		FLinearColor(0.95f, 0.98f, 1.00f, 1.0f),
		FLinearColor(0.90f, 0.95f, 1.05f, 1.0f),
		FLinearColor(0.85f, 0.92f, 1.10f, 1.0f),
		FLinearColor(0.80f, 0.90f, 1.15f, 1.0f),
		FLinearColor(0.75f, 0.87f, 1.20f, 1.0f),
		FLinearColor(0.70f, 0.85f, 1.25f, 1.0f),
	};

	S.bStreakEnabled = true;
	S.StreakTint = FLinearColor(0.55f, 0.75f, 1.00f, 1.0f);
	S.StreakThickness = 2.0f;
	S.StreakAttenuation = 0.35f;
	S.StreakPasses = 4;
	S.bStreakVerticalEnabled = false;
	S.StreakVerticalIntensity = 0.25f;
	S.StreakVerticalLength = 240.0f;

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
		S.GlareIntensity = 0.35f;
		S.GlareTint = FLinearColor(0.85f, 0.95f, 1.00f, 1.0f);
		break;

	case ETrueGlowPreset::Subtle:
		S.BloomIntensity = 0.60f;
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
