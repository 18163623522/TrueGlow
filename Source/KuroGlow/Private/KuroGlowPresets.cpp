// Copyright pengxiwei. All Rights Reserved.

#include "KuroGlowSettings.h"

namespace KuroGlowPresets
{

void Apply(EKuroGlowPreset Preset, UKuroGlowSettings& S)
{
	// 公共默认（鸣潮基线）
	S.bEnabled = true;
	S.bBloomEnabled = true;
	S.BloomThreshold = 1.0f;
	S.BloomKnee = 0.6f;
	S.BloomLevels = 6;
	S.BloomBlurRadius = 2.0f;

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

	S.bGlareEnabled = true;
	S.GlareTint = FLinearColor(1, 1, 1, 1);
	S.GlareDirections = 4;
	S.GlareRadius1 = 24.0f;
	S.GlareRadius2 = 72.0f;
	S.GlareTaps = 6;

	switch (Preset)
	{
	case EKuroGlowPreset::WuWa:
		S.BloomIntensity = 1.00f;
		S.StreakIntensity = 0.35f;
		S.StreakLength = 480.0f;
		S.GlareIntensity = 0.25f;
		break;

	case EKuroGlowPreset::Neon:
		S.BloomIntensity = 1.40f;
		S.StreakIntensity = 0.70f;
		S.StreakLength = 720.0f;
		S.StreakTint = FLinearColor(0.35f, 0.60f, 1.00f, 1.0f);
		S.StreakAttenuation = 0.25f;
		S.StreakPasses = 6;
		S.GlareIntensity = 0.35f;
		S.GlareTint = FLinearColor(0.85f, 0.95f, 1.00f, 1.0f);
		break;

	case EKuroGlowPreset::Subtle:
		S.BloomIntensity = 0.60f;
		S.BloomBlurRadius = 1.0f;
		S.StreakIntensity = 0.12f;
		S.StreakLength = 240.0f;
		S.GlareIntensity = 0.10f;
		break;

	case EKuroGlowPreset::Custom:
	default:
		break;
	}
}

bool ApplyByName(const FString& PresetName, UKuroGlowSettings& OutSettings)
{
	const FString Name = PresetName.ToLower();

	EKuroGlowPreset Preset = EKuroGlowPreset::Custom;
	if (Name == TEXT("wuwa") || Name == TEXT("鸣潮"))
	{
		Preset = EKuroGlowPreset::WuWa;
	}
	else if (Name == TEXT("neon"))
	{
		Preset = EKuroGlowPreset::Neon;
	}
	else if (Name == TEXT("subtle"))
	{
		Preset = EKuroGlowPreset::Subtle;
	}
	else
	{
		return false;
	}

	Apply(Preset, OutSettings);
	OutSettings.Preset = Preset;
	return true;
}

} // namespace KuroGlowPresets
