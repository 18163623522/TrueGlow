// Copyright pengxiwei. All Rights Reserved.

#include "KuroGlowSettings.h"
#include "KuroGlowShaderBridge.h"

#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKuroGlowMain, Log, All);

#define LOCTEXT_NAMESPACE "KuroGlow"

/**
 * KuroGlow 主模块（Default 相位）：UCLASS 设置 + 预设 + 控制台命令。
 * shader 声明与视图扩展在 KuroGlowShaders 模块（PostConfigInit，见其注释）。
 */
class FKuroGlowModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// 参数桥：渲染线程扩展每帧经此读 Project Settings
		FKuroGlowShaderBridge::SetParamsProvider([]()
		{
			return UKuroGlowSettings::Get()->BuildParams();
		});

		UE_LOG(LogKuroGlowMain, Log, TEXT("KuroGlow main module up; settings provider registered."));
	}

	virtual void ShutdownModule() override
	{
		FKuroGlowShaderBridge::SetParamsProvider(FKuroGlowShaderBridge::FParamsProvider());
	}
};

// ---------------------------------------------------------------------------
// 控制台命令

static FAutoConsoleCommand GKuroGlowApplyPreset(
	TEXT("kg.ApplyPreset"),
	TEXT("kg.ApplyPreset <WuWa|Neon|Subtle> — apply preset to KuroGlow settings and persist."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogKuroGlowMain, Warning, TEXT("Usage: kg.ApplyPreset <WuWa|Neon|Subtle>"));
			return;
		}
		UKuroGlowSettings* Settings = UKuroGlowSettings::Get();
		if (KuroGlowPresets::ApplyByName(Args[0], *Settings))
		{
			Settings->SaveConfig();
			UE_LOG(LogKuroGlowMain, Log, TEXT("KuroGlow preset '%s' applied."), *Args[0]);
		}
		else
		{
			UE_LOG(LogKuroGlowMain, Warning, TEXT("Unknown preset '%s'."), *Args[0]);
		}
	}));

static FAutoConsoleCommand GKuroGlowSetupVolume(
	TEXT("kg.SetupVolume"),
	TEXT("Spawn/find a global PostProcessVolume that disables the engine bloom (KuroGlow replaces it)."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UWorld* World = GWorld;
		if (!World)
		{
			UE_LOG(LogKuroGlowMain, Warning, TEXT("kg.SetupVolume: no world."));
			return;
		}

		APostProcessVolume* Volume = nullptr;
		for (TActorIterator<APostProcessVolume> It(World); It; ++It)
		{
			if (It->GetName().Contains(TEXT("KuroGlow")))
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
				Volume->SetActorLabel(TEXT("KuroGlowPostProcess"));
			}
#endif
		}
		if (!Volume)
		{
			UE_LOG(LogKuroGlowMain, Warning, TEXT("kg.SetupVolume: failed to spawn volume."));
			return;
		}

		Volume->bUnbound = true;
		Volume->Settings.bOverride_BloomIntensity = true;
		Volume->Settings.BloomIntensity = 0.0f;
		UE_LOG(LogKuroGlowMain, Log, TEXT("kg.SetupVolume: global PostProcessVolume '%s' ready, engine bloom off."),
			*Volume->GetName());
	}));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKuroGlowModule, KuroGlow)
