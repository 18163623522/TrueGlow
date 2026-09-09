// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"
#include "TrueGlowCVars.h"

FTrueGlowParams UTrueGlowSettings::BuildParams() const
{
	FTrueGlowParams P;

	P.bEnabled = bEnabled && TrueGlowCVars::IsEnabled();
	P.bAllowSceneCapture = bAllowSceneCapture;

	P.bBloomEnabled = bBloomEnabled;
	P.BloomIntensity = FMath::Max(0.0f, BloomIntensity);
	P.BloomThreshold = FMath::Max(0.0f, BloomThreshold);
	P.BloomKnee = FMath::Clamp(BloomKnee, 0.0f, 1.0f);
	P.BloomBrightMultiplier = FMath::Clamp(BloomBrightMultiplier, 0.0f, 8.0f);
	P.BloomLevels = FMath::Clamp(BloomLevels, 1, 6);
	P.BloomBlurRadius = FMath::Clamp(BloomBlurRadius, 0.0f, 8.0f);
	P.GaussianIterations = FMath::Clamp(GaussianIterations, 1, 2);
	P.ChromaticDispersion = FMath::Clamp(ChromaticDispersion, 0.0f, 1.0f);
	P.bLensDirt = bLensDirt;
	P.LensDirtIntensity = FMath::Clamp(LensDirtIntensity, 0.0f, 3.0f);
	P.FilmSoftIntensity = FMath::Clamp(FilmSoftIntensity, 0.0f, 2.0f);
	P.FilmSoftRadius = FMath::Clamp(FilmSoftRadius, 1.0f, 8.0f);
	P.DualTintStrength = FMath::Clamp(DualTintStrength, 0.0f, 1.0f);
	P.WarmCoreColor = WarmCoreColor;
	P.CoolFringeColor = CoolFringeColor;
	P.bBloomFastMode = bBloomFastMode;
	P.BloomScale = FVector2D(
		FMath::Clamp(BloomScale.X, 0.1f, 4.0f),
		FMath::Clamp(BloomScale.Y, 0.1f, 4.0f));

	const float BloomIntensityO = TrueGlowCVars::BloomIntensityOverride();
	const float BloomThresholdO = TrueGlowCVars::BloomThresholdOverride();
	const int32 BloomLevelsO = TrueGlowCVars::BloomLevelsOverride();
	if (BloomIntensityO >= 0.0f) { P.BloomIntensity = BloomIntensityO; }
	if (BloomThresholdO >= 0.0f) { P.BloomThreshold = BloomThresholdO; }
	if (BloomLevelsO >= 1) { P.BloomLevels = FMath::Clamp(BloomLevelsO, 1, 6); }

	for (int32 i = 0; i < 6; ++i)
	{
		if (i < BloomLevelWeights.Num())
		{
			P.BloomLevelWeights[i] = FMath::Max(0.0f, BloomLevelWeights[i]);
		}
		if (i < BloomLevelTints.Num())
		{
			P.BloomLevelTints[i] = BloomLevelTints[i];
		}
	}

	P.bStreakEnabled = bStreakEnabled;
	P.StreakIntensity = FMath::Max(0.0f, StreakIntensity);
	P.StreakTint = StreakTint;
	P.StreakLength = FMath::Clamp(StreakLength, 16.0f, 2048.0f);
	P.StreakThickness = FMath::Clamp(StreakThickness, 0.25f, 16.0f);
	P.bStreakDualLine = bStreakDualLine;
	P.StreakDualLineSeparation = FMath::Clamp(StreakDualLineSeparation, 2.0f, 64.0f);
	P.StreakDualLineIntensity = FMath::Clamp(StreakDualLineIntensity, 0.0f, 1.0f);
	P.StreakAttenuation = FMath::Clamp(StreakAttenuation, 0.05f, 1.0f);
	P.StreakPasses = FMath::Clamp(StreakPasses, 1, 8);
	P.bStreakOwnThreshold = bStreakOwnThreshold;
	P.StreakThreshold = FMath::Max(0.0f, StreakThreshold);
	const float StreakIntensityO = TrueGlowCVars::StreakIntensityOverride();
	const float StreakLengthO = TrueGlowCVars::StreakLengthOverride();
	if (StreakIntensityO >= 0.0f) { P.StreakIntensity = StreakIntensityO; }
	if (StreakLengthO >= 0.0f) { P.StreakLength = FMath::Clamp(StreakLengthO, 16.0f, 2048.0f); }

	P.bStreakVerticalEnabled = bStreakVerticalEnabled;
	P.StreakVerticalIntensity = FMath::Max(0.0f, StreakVerticalIntensity);
	P.StreakVerticalLength = FMath::Clamp(StreakVerticalLength, 16.0f, 2048.0f);

	P.bGlareEnabled = bGlareEnabled;
	P.GlareIntensity = FMath::Max(0.0f, GlareIntensity);
	P.GlareTint = GlareTint;
	P.GlareDirections = (GlareDirections == 6) ? 6 : 4;
	P.GlareRadius1 = FMath::Clamp(GlareRadius1, 4.0f, 128.0f);
	P.GlareRadius2 = FMath::Clamp(GlareRadius2, 8.0f, 512.0f);
	P.GlareTaps = FMath::Clamp(GlareTaps, 2, 16);
	const float GlareIntensityO = TrueGlowCVars::GlareIntensityOverride();
	if (GlareIntensityO >= 0.0f) { P.GlareIntensity = GlareIntensityO; }

	return P;
}

#if WITH_EDITOR
void UTrueGlowSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrueGlowSettings, Preset))
		{
			if (Preset != ETrueGlowPreset::Custom)
			{
				TrueGlowPresets::Apply(Preset, *this);
				SaveConfig();
			}
		}
		else
		{
			// 手动改任何参数 → 脱离预设（保留用户值）
			Preset = ETrueGlowPreset::Custom;
			SaveConfig();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
