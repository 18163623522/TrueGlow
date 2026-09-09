// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"
#include "TrueGlowBlueprintLibrary.h"
#include "TrueGlowShaderBridge.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTrueGlowMain, Log, All);

#define LOCTEXT_NAMESPACE "TrueGlow"

/**
 * TrueGlow 主模块（Default 相位）：UCLASS 设置 + 预设 + 控制台命令。
 * shader 声明与视图扩展在 TrueGlowShaders 模块（PostConfigInit，见其注释）。
 */
class FTrueGlowModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// 参数桥：渲染线程扩展每帧经此读 Project Settings
		FTrueGlowShaderBridge::SetParamsProvider([]()
		{
			return UTrueGlowSettings::Get()->BuildParams();
		});

		UE_LOG(LogTrueGlowMain, Log, TEXT("TrueGlow main module up; settings provider registered."));
	}

	virtual void ShutdownModule() override
	{
		FTrueGlowShaderBridge::SetParamsProvider(FTrueGlowShaderBridge::FParamsProvider());
	}
};

// ---------------------------------------------------------------------------
// 控制台命令

static FAutoConsoleCommand GTrueGlowApplyPreset(
	TEXT("tg.ApplyPreset"),
	TEXT("tg.ApplyPreset <WuWa|Neon|Subtle> — apply preset to TrueGlow settings and persist."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTrueGlowMain, Warning, TEXT("Usage: tg.ApplyPreset <WuWa|Neon|Subtle>"));
			return;
		}
		UTrueGlowSettings* Settings = UTrueGlowSettings::Get();
		if (TrueGlowPresets::ApplyByName(Args[0], *Settings))
		{
			Settings->SaveConfig();
			UE_LOG(LogTrueGlowMain, Log, TEXT("TrueGlow preset '%s' applied."), *Args[0]);
		}
		else
		{
			UE_LOG(LogTrueGlowMain, Warning, TEXT("Unknown preset '%s'."), *Args[0]);
		}
	}));

static FAutoConsoleCommand GTrueGlowSetupVolume(
	TEXT("tg.SetupVolume"),
	TEXT("Spawn/find a global PostProcessVolume that disables the engine bloom (TrueGlow replaces it)."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UTrueGlowBlueprintLibrary::SetupEngineBloomOffVolume();
		UE_LOG(LogTrueGlowMain, Log, TEXT("tg.SetupVolume: engine bloom off (global PostProcessVolume)."));
	}));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrueGlowModule, TrueGlow)
