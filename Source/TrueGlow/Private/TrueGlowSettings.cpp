// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"
#include "TrueGlowCVars.h"

FTrueGlowParams UTrueGlowSettings::BuildParams() const
{
	FTrueGlowParams P;

	P.bEnabled = bEnabled && TrueGlowCVars::IsEnabled();
	P.bAllowSceneCapture = bAllowSceneCapture;

	P.bBloomEnabled = bBloomEnabled;
	P.BloomIntensity = BloomIntensity;
	P.BloomThreshold = FMath::Max(0.0f, BloomThreshold);
	P.BloomKnee = FMath::Clamp(BloomKnee, 0.0f, 1.0f);
	P.BloomBrightMultiplier = FMath::Max(0.0f, BloomBrightMultiplier);
	P.BloomLevels = FMath::Clamp(BloomLevels, 1, 8);
	P.BloomBlurRadius = FMath::Max(0.0f, BloomBlurRadius);
	P.GaussianIterations = FMath::Clamp(GaussianIterations, 1, 3);
	P.ChromaticDispersion = FMath::Max(0.0f, ChromaticDispersion);
	P.bLensDirt = bLensDirt;
	P.LensDirtIntensity = FMath::Max(0.0f, LensDirtIntensity);
	P.FilmSoftIntensity = FMath::Max(0.0f, FilmSoftIntensity);
	P.FilmSoftRadius = FMath::Max(0.25f, FilmSoftRadius);
	P.DualTintStrength = FMath::Max(0.0f, DualTintStrength);
	P.WarmCoreColor = WarmCoreColor;
	P.CoolFringeColor = CoolFringeColor;
	P.bBloomFastMode = bBloomFastMode;
	P.BloomScale = FVector2D(
		FMath::Clamp(BloomScale.X, 0.05f, 10.0f),
		FMath::Clamp(BloomScale.Y, 0.05f, 10.0f));

	const float BloomIntensityO = TrueGlowCVars::BloomIntensityOverride();
	const float BloomThresholdO = TrueGlowCVars::BloomThresholdOverride();
	const int32 BloomLevelsO = TrueGlowCVars::BloomLevelsOverride();
	if (BloomIntensityO >= 0.0f) { P.BloomIntensity = BloomIntensityO; }
	if (BloomThresholdO >= 0.0f) { P.BloomThreshold = BloomThresholdO; }
	if (BloomLevelsO >= 1) { P.BloomLevels = FMath::Clamp(BloomLevelsO, 1, 8); }

	for (int32 i = 0; i < 8; ++i)
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
	P.StreakLength = FMath::Max(2.0f, StreakLength);
	P.StreakThickness = FMath::Max(0.01f, StreakThickness);
	P.bStreakDualLine = bStreakDualLine;
	P.StreakDualLineSeparation = FMath::Max(0.5f, StreakDualLineSeparation);
	P.StreakDualLineIntensity = FMath::Max(0.0f, StreakDualLineIntensity);
	P.StreakAttenuation = FMath::Max(0.001f, StreakAttenuation);
	P.StreakPasses = FMath::Clamp(StreakPasses, 1, 16);
	P.bStreakOwnThreshold = bStreakOwnThreshold;
	P.StreakThreshold = FMath::Max(0.0f, StreakThreshold);
	const float StreakIntensityO = TrueGlowCVars::StreakIntensityOverride();
	const float StreakLengthO = TrueGlowCVars::StreakLengthOverride();
	if (StreakIntensityO >= 0.0f) { P.StreakIntensity = StreakIntensityO; }
	if (StreakLengthO >= 0.0f) { P.StreakLength = FMath::Max(2.0f, StreakLengthO); }

	P.bStreakVerticalEnabled = bStreakVerticalEnabled;
	P.StreakVerticalIntensity = FMath::Max(0.0f, StreakVerticalIntensity);
	P.StreakVerticalLength = FMath::Max(2.0f, StreakVerticalLength);

	P.GhostIntensity = FMath::Max(0.0f, GhostIntensity);
	P.GhostCount = FMath::Clamp(GhostCount, 1, 8);
	P.GhostSpacing = FMath::Clamp(GhostSpacing, 0.1f, 1.5f);
	P.GhostDispersal = FMath::Max(0.0f, GhostDispersal);
	P.HaloIntensity = FMath::Max(0.0f, HaloIntensity);
	P.HaloRadius = FMath::Clamp(HaloRadius, 0.02f, 0.6f);
	P.FlareTint = FlareTint;
	P.bStarFilter = bStarFilter;
	P.StarFilterIntensity = FMath::Max(0.0f, StarFilterIntensity);
	P.StarFilterRays = FMath::Clamp(StarFilterRays, 4, 8);
	P.StarFilterRotation = StarFilterRotation;
	P.StarFilterLength = FMath::Max(4.0f, StarFilterLength);
	P.StarFilterTint = StarFilterTint;
	P.PolyIrisIntensity = FMath::Max(0.0f, PolyIrisIntensity);
	P.PolyIrisSides = FMath::Clamp(PolyIrisSides, 5, 9);
	P.PolyIrisRotation = PolyIrisRotation;
	P.PolyIrisRadius = FMath::Clamp(PolyIrisRadius, 0.02f, 0.6f);
	P.FanIntensity = FMath::Max(0.0f, FanIntensity);
	P.FanCount = FMath::Clamp(FanCount, 3, 12);
	P.FanSpread = FMath::Clamp(FanSpread, 5.0f, 120.0f);
	P.FanRadius = FMath::Clamp(FanRadius, 0.03f, 0.6f);

	P.bGodRays = bGodRays;
	P.GodRaysIntensity = FMath::Max(0.0f, GodRaysIntensity);
	P.GodRaysLightPos = FVector2D(FMath::Clamp(GodRaysLightPos.X, -0.5f, 1.5f), FMath::Clamp(GodRaysLightPos.Y, -0.5f, 1.5f));
	P.GodRaysLength = FMath::Clamp(GodRaysLength, 0.05f, 1.0f);
	P.GodRaysDecay = FMath::Clamp(GodRaysDecay, 0.8f, 0.999f);
	P.GodRaysDensity = FMath::Clamp(GodRaysDensity, 0.3f, 3.0f);
	P.GodRaysTint = GodRaysTint;

	P.bGlareEnabled = bGlareEnabled;
	P.GlareIntensity = FMath::Max(0.0f, GlareIntensity);
	P.GlareTint = GlareTint;
	P.GlareDirections = FMath::Clamp(GlareDirections, 2, 12);
	P.GlareRadius1 = FMath::Max(0.5f, GlareRadius1);
	P.GlareRadius2 = FMath::Max(1.0f, GlareRadius2);
	P.GlareTaps = FMath::Clamp(GlareTaps, 1, 32);
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
