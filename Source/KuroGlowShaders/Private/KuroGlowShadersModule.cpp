// Copyright pengxiwei. All Rights Reserved.

#include "KuroGlowShaderBridge.h"
#include "KuroGlowCVars.h"
#include "KuroGlowViewExtension.h"

#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogKuroGlow, Log, All);

// ---------------------------------------------------------------------------
// CVars（实时调参入口；负值 = 不覆盖 Project Settings）
TAutoConsoleVariable<int32> GCVarKGEnable(
	TEXT("kg.Enable"), 1,
	TEXT("KuroGlow master switch. 0 = passthrough (no cost)."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGBloomIntensity(
	TEXT("kg.Bloom.Intensity"), -1.0f,
	TEXT(">= 0 overrides BloomIntensity from settings."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGBloomThreshold(
	TEXT("kg.Bloom.Threshold"), -1.0f,
	TEXT(">= 0 overrides BloomThreshold from settings."),
	ECVF_Default);

TAutoConsoleVariable<int32> GCVarKGBloomLevels(
	TEXT("kg.Bloom.Levels"), -1,
	TEXT(">= 1 overrides BloomLevels from settings (1..6)."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGStreakIntensity(
	TEXT("kg.Streak.Intensity"), -1.0f,
	TEXT(">= 0 overrides StreakIntensity from settings."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGStreakLength(
	TEXT("kg.Streak.Length"), -1.0f,
	TEXT(">= 0 overrides StreakLength from settings."),
	ECVF_Default);

TAutoConsoleVariable<float> GCVarKGGlareIntensity(
	TEXT("kg.Glare.Intensity"), -1.0f,
	TEXT(">= 0 overrides GlareIntensity from settings."),
	ECVF_Default);

namespace KuroGlowCVars
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
	FKuroGlowShaderBridge::FParamsProvider GParamsProvider;
}

void FKuroGlowShaderBridge::SetParamsProvider(FParamsProvider InProvider)
{
	FScopeLock Lock(&GProviderCS);
	GParamsProvider = MoveTemp(InProvider);
}

FKuroGlowParams FKuroGlowShaderBridge::GetParamsSnapshot()
{
	FScopeLock Lock(&GProviderCS);
	return GParamsProvider ? GParamsProvider() : FKuroGlowParams();
}

// ---------------------------------------------------------------------------
class FKuroGlowShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("KuroGlow"));
		if (ThisPlugin.IsValid())
		{
			const FString ShaderDir = FPaths::Combine(ThisPlugin->GetBaseDir(), TEXT("Shaders"));
			AddShaderSourceDirectoryMapping(TEXT("/Plugin/KuroGlow"), ShaderDir);
		}

		// 本模块 PostConfigInit 加载时 GEngine 尚未创建，而注册视图扩展需要它；
		// 引擎模块的标准做法是延迟到 PostEngineInit 再创建。
		if (GEngine)
		{
			CreateExtension();
		}
		else
		{
			FCoreDelegates::OnPostEngineInit.AddRaw(this, &FKuroGlowShadersModule::CreateExtension);
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
			Extension = FKuroGlowViewExtension::Create();
			UE_LOG(LogKuroGlow, Log, TEXT("KuroGlow shaders module up; view extension registered (kg.Enable 1)."));
		}
	}

	TSharedPtr<FKuroGlowViewExtension, ESPMode::ThreadSafe> Extension;
};

IMPLEMENT_MODULE(FKuroGlowShadersModule, KuroGlowShaders)
