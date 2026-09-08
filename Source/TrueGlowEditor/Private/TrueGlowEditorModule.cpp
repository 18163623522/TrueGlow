// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "TrueGlowEditor"

DEFINE_LOG_CATEGORY_STATIC(LogTrueGlowEditor, Log, All);

/**
 * 编辑器模块：把 UTrueGlowSettings 注册进 Project Settings（Plugins 分类）。
 * 运行时模块刻意不依赖 DeveloperSettings（Default 相位加载时该 DLL 还不在进程内，
 * 硬 import 会让 OS 拒载插件 DLL），因此这里手动注册。
 */
class FTrueGlowEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// 强制确保 Settings 模块在位（PostEngineInit 正常都已加载，这里防呆）
		ISettingsModule* SettingsModule = FModuleManager::LoadModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			TWeakPtr<ISettingsSection> Section = SettingsModule->RegisterSettings(
				TEXT("Project"),
				TEXT("Plugins"),
				TEXT("TrueGlow"),
				LOCTEXT("TrueGlowSettings_Name", "TrueGlow 物理辉光"),
				LOCTEXT("TrueGlowSettings_Desc", "HDR 金字塔 Bloom + 变形镜头 Streak + 星芒 Glare"),
				GetMutableDefault<UTrueGlowSettings>());

			UE_LOG(LogTrueGlowEditor, Log, TEXT("TrueGlow settings registered into Project Settings (Plugins category)."));
		}
		else
		{
			UE_LOG(LogTrueGlowEditor, Warning, TEXT("TrueGlow: Settings module unavailable; Project Settings page NOT registered."));
		}
	}

	virtual void ShutdownModule() override
	{
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			SettingsModule->UnregisterSettings(TEXT("Project"), TEXT("Plugins"), TEXT("TrueGlow"));
		}
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrueGlowEditorModule, TrueGlowEditor)
