// Copyright pengxiwei. All Rights Reserved.

#include "KuroGlowSettings.h"
#include "KuroGlowCVars.h"

FKuroGlowParams UKuroGlowSettings::BuildParams() const
{
	FKuroGlowParams P;

	P.bEnabled = bEnabled && KuroGlowCVars::IsEnabled();
	P.bAllowSceneCapture = bAllowSceneCapture;

	P.bBloomEnabled = bBloomEnabled;
	P.BloomIntensity = FMath::Max(0.0f, BloomIntensity);
	P.BloomThreshold = FMath::Max(0.0f, BloomThreshold);
	P.BloomKnee = FMath::Clamp(BloomKnee, 0.0f, 1.0f);
	P.BloomLevels = FMath::Clamp(BloomLevels, 1, 6);
	P.BloomBlurRadius = FMath::Clamp(BloomBlurRadius, 0.0f, 6.0f);

	const float BloomIntensityO = KuroGlowCVars::BloomIntensityOverride();
	const float BloomThresholdO = KuroGlowCVars::BloomThresholdOverride();
	const int32 BloomLevelsO = KuroGlowCVars::BloomLevelsOverride();
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
	P.StreakThickness = FMath::Clamp(StreakThickness, 1.0f, 16.0f);
	P.StreakAttenuation = FMath::Clamp(StreakAttenuation, 0.05f, 1.0f);
	P.StreakPasses = FMath::Clamp(StreakPasses, 1, 8);
	const float StreakIntensityO = KuroGlowCVars::StreakIntensityOverride();
	const float StreakLengthO = KuroGlowCVars::StreakLengthOverride();
	if (StreakIntensityO >= 0.0f) { P.StreakIntensity = StreakIntensityO; }
	if (StreakLengthO >= 0.0f) { P.StreakLength = FMath::Clamp(StreakLengthO, 16.0f, 2048.0f); }

	P.bGlareEnabled = bGlareEnabled;
	P.GlareIntensity = FMath::Max(0.0f, GlareIntensity);
	P.GlareTint = GlareTint;
	P.GlareDirections = (GlareDirections == 6) ? 6 : 4;
	P.GlareRadius1 = FMath::Clamp(GlareRadius1, 4.0f, 128.0f);
	P.GlareRadius2 = FMath::Clamp(GlareRadius2, 8.0f, 512.0f);
	P.GlareTaps = FMath::Clamp(GlareTaps, 2, 16);
	const float GlareIntensityO = KuroGlowCVars::GlareIntensityOverride();
	if (GlareIntensityO >= 0.0f) { P.GlareIntensity = GlareIntensityO; }

	return P;
}

#if WITH_EDITOR
void UKuroGlowSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UKuroGlowSettings, Preset))
		{
			if (Preset != EKuroGlowPreset::Custom)
			{
				KuroGlowPresets::Apply(Preset, *this);
				SaveConfig();
			}
		}
		else
		{
			// 手动改任何参数 → 脱离预设（保留用户值）
			Preset = EKuroGlowPreset::Custom;
			SaveConfig();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
