// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowBlueprintLibrary.h"

#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "TrueGlow"

namespace
{
	UTrueGlowSettings* S() { return UTrueGlowSettings::Get(); }
}

void UTrueGlowBlueprintLibrary::SetEnabled(bool bInEnabled) { S()->bEnabled = bInEnabled; }
bool UTrueGlowBlueprintLibrary::IsEnabled() { return S()->bEnabled; }

void UTrueGlowBlueprintLibrary::SetBloomEnabled(bool bInEnabled) { S()->bBloomEnabled = bInEnabled; }
void UTrueGlowBlueprintLibrary::SetBloomIntensity(float InIntensity) { S()->BloomIntensity = FMath::Max(0.0f, InIntensity); }
void UTrueGlowBlueprintLibrary::SetBloomThreshold(float InThreshold) { S()->BloomThreshold = FMath::Max(0.0f, InThreshold); }
void UTrueGlowBlueprintLibrary::SetBloomLevels(int32 InLevels) { S()->BloomLevels = FMath::Clamp(InLevels, 1, 6); }
void UTrueGlowBlueprintLibrary::SetBloomBrightMultiplier(float InMultiplier) { S()->BloomBrightMultiplier = FMath::Clamp(InMultiplier, 0.0f, 8.0f); }
void UTrueGlowBlueprintLibrary::SetBloomFastMode(bool bInFast) { S()->bBloomFastMode = bInFast; }
void UTrueGlowBlueprintLibrary::SetBloomScale(FVector2D InScale)
{
	S()->BloomScale = FVector2D(FMath::Clamp(InScale.X, 0.1f, 4.0f), FMath::Clamp(InScale.Y, 0.1f, 4.0f));
}

void UTrueGlowBlueprintLibrary::SetStreakEnabled(bool bInEnabled) { S()->bStreakEnabled = bInEnabled; }
void UTrueGlowBlueprintLibrary::SetStreakIntensity(float InIntensity) { S()->StreakIntensity = FMath::Max(0.0f, InIntensity); }
void UTrueGlowBlueprintLibrary::SetStreakLength(float InLengthPx) { S()->StreakLength = FMath::Clamp(InLengthPx, 16.0f, 2048.0f); }
void UTrueGlowBlueprintLibrary::SetStreakTint(FLinearColor InTint) { S()->StreakTint = InTint; }

void UTrueGlowBlueprintLibrary::SetVerticalStreakEnabled(bool bInEnabled) { S()->bStreakVerticalEnabled = bInEnabled; }
void UTrueGlowBlueprintLibrary::SetVerticalStreakIntensity(float InIntensity) { S()->StreakVerticalIntensity = FMath::Max(0.0f, InIntensity); }
void UTrueGlowBlueprintLibrary::SetVerticalStreakLength(float InLengthPx) { S()->StreakVerticalLength = FMath::Clamp(InLengthPx, 16.0f, 2048.0f); }

void UTrueGlowBlueprintLibrary::SetGlareEnabled(bool bInEnabled) { S()->bGlareEnabled = bInEnabled; }
void UTrueGlowBlueprintLibrary::SetGlareIntensity(float InIntensity) { S()->GlareIntensity = FMath::Max(0.0f, InIntensity); }
void UTrueGlowBlueprintLibrary::SetGlareTint(FLinearColor InTint) { S()->GlareTint = InTint; }

void UTrueGlowBlueprintLibrary::ApplyPreset(ETrueGlowPreset InPreset)
{
	if (InPreset == ETrueGlowPreset::Custom)
	{
		return;
	}
	UTrueGlowSettings* Settings = S();
	TrueGlowPresets::Apply(InPreset, *Settings);
	Settings->Preset = InPreset;
}

void UTrueGlowBlueprintLibrary::SetupEngineBloomOffVolume()
{
	UWorld* World = GWorld;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrueGlow SetupEngineBloomOffVolume: no world."));
		return;
	}

	APostProcessVolume* Volume = nullptr;
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		if (It->GetName().Contains(TEXT("TrueGlow")))
		{
			Volume = *It;
			break;
		}
	}
	if (!Volume)
	{
		Volume = World->SpawnActor<APostProcessVolume>();
#if WITH_EDITOR
		if (Volume)
		{
			Volume->SetActorLabel(TEXT("TrueGlowPostProcess"));
		}
#endif
	}
	if (!Volume)
	{
		return;
	}

	Volume->bUnbound = true;
	Volume->Settings.bOverride_BloomIntensity = true;
	Volume->Settings.BloomIntensity = 0.0f;
}

void UTrueGlowBlueprintLibrary::SaveSettings() { S()->SaveConfig(); }

#undef LOCTEXT_NAMESPACE
