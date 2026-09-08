// Copyright pengxiwei. All Rights Reserved.

#include "KuroGlowSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "KuroGlowEditor"

/**
 * 编辑器模块：把 UKuroGlowSettings 注册进 Project Settings（Plugins 分类）。
 * 运行时模块刻意不依赖 DeveloperSettings（Default 相位加载时该 DLL 还不在进程内，
 * 硬 import 会让 OS 拒载插件 DLL），因此这里手动注册。
 */
class FKuroGlowEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			SettingsModule->RegisterSettings(
				TEXT("Project"),
				TEXT("Plugins"),
				TEXT("KuroGlow"),
				LOCTEXT("KuroGlowSettings_Name", "KuroGlow 物理辉光"),
				LOCTEXT("KuroGlowSettings_Desc", "HDR 金字塔 Bloom + 变形镜头 Streak + 星芒 Glare"),
				GetMutableDefault<UKuroGlowSettings>());
		}
	}

	virtual void ShutdownModule() override
	{
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			SettingsModule->UnregisterSettings(TEXT("Project"), TEXT("Plugins"), TEXT("KuroGlow"));
		}
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKuroGlowEditorModule, KuroGlowEditor)
