// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowShaderBridge.h"
#include "TrueGlowCVars.h"
#include "TrueGlowViewExtension.h"

#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogTrueGlow, Log, All);

// ---------------------------------------------------------------------------
// CVars（实时调参入口；负值 = 不覆盖 Project Settings）
TAutoConsoleVariable<int32> GCVarKGEnable(
	TEXT("tg.Enable"), 1,
	TEXT("TrueGlow master switch. 0 = passthrough (no cost)."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGBloomIntensity(
	TEXT("tg.Bloom.Intensity"), -1.0f,
	TEXT(">= 0 overrides BloomIntensity from settings."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGBloomThreshold(
	TEXT("tg.Bloom.Threshold"), -1.0f,
	TEXT(">= 0 overrides BloomThreshold from settings."),
	ECVF_Default);

TAutoConsoleVariable<int32> GCVarKGBloomLevels(
	TEXT("tg.Bloom.Levels"), -1,
	TEXT(">= 1 overrides BloomLevels from settings (1..6)."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGStreakIntensity(
	TEXT("tg.Streak.Intensity"), -1.0f,
	TEXT(">= 0 overrides StreakIntensity from settings."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGStreakLength(
	TEXT("tg.Streak.Length"), -1.0f,
	TEXT(">= 0 overrides StreakLength from settings."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGGlareIntensity(
	TEXT("tg.Glare.Intensity"), -1.0f,
	TEXT(">= 0 overrides GlareIntensity from settings."),
	ECVF_Default);

namespace TrueGlowCVars
{
	bool IsEnabled() { return GCVarKGEnable.GetValueOnGameThread() != 0; }
	float BloomIntensityOverride() { return GCVarKGBloomIntensity.GetValueOnGameThread(); }
	float BloomThresholdOverride() { return GCVarKGBloomThreshold.GetValueOnGameThread(); }
	int32 BloomLevelsOverride() { return GCVarKGBloomLevels.GetValueOnGameThread(); }
	float StreakIntensityOverride() { return GCVarKGStreakIntensity.GetValueOnGameThread(); }
	float StreakLengthOverride() { return GCVarKGStreakLength.GetValueOnGameThread(); }
	float GlareIntensityOverride() { return GCVarKGGlareIntensity.GetValueOnGameThread(); }
}

// ---------------------------------------------------------------------------
// 参数桥实现
namespace
{
	FCriticalSection GProviderCS;
	FTrueGlowShaderBridge::FParamsProvider GParamsProvider;
}

void FTrueGlowShaderBridge::SetParamsProvider(FParamsProvider InProvider)
{
	FScopeLock Lock(&GProviderCS);
	GParamsProvider = MoveTemp(InProvider);
}

FTrueGlowParams FTrueGlowShaderBridge::GetParamsSnapshot()
{
	FScopeLock Lock(&GProviderCS);
	return GParamsProvider ? GParamsProvider() : FTrueGlowParams();
}

// ---------------------------------------------------------------------------
class FTrueGlowShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("TrueGlow"));
		if (ThisPlugin.IsValid())
		{
			const FString ShaderDir = FPaths::Combine(ThisPlugin->GetBaseDir(), TEXT("Shaders"));
			AddShaderSourceDirectoryMapping(TEXT("/Plugin/TrueGlow"), ShaderDir);
		}

		// 本模块 PostConfigInit 加载时 GEngine 尚未创建，而注册视图扩展需要它；
		// 引擎模块的标准做法是延迟到 PostEngineInit 再创建。
		if (GEngine)
		{
			CreateExtension();
		}
		else
		{
			FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTrueGlowShadersModule::CreateExtension);
		}
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPostEngineInit.RemoveAll(this);
		Extension.Reset();
	}

private:
	void CreateExtension()
	{
		if (!Extension.IsValid())
		{
			Extension = FTrueGlowViewExtension::Create();
			UE_LOG(LogTrueGlow, Log, TEXT("TrueGlow shaders module up; view extension registered (tg.Enable 1)."));
		}
	}

	TSharedPtr<FTrueGlowViewExtension, ESPMode::ThreadSafe> Extension;
};

IMPLEMENT_MODULE(FTrueGlowShadersModule, TrueGlowShaders)
