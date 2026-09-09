// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"
#include "TrueGlowBlueprintLibrary.h"
#include "Modules/ModuleManager.h"

#include "Framework/Docking/TabManager.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TrueGlowEditor"

DEFINE_LOG_CATEGORY_STATIC(LogTrueGlowEditor, Log, All);

namespace
{
	TSharedRef<SDockTab> SpawnTrueGlowTab(const FSpawnTabArgs& Args)
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs DetailsArgs;
		DetailsArgs.bUpdatesFromSelection = false;
		DetailsArgs.bLockable = false;
		DetailsArgs.bAllowSearch = true;
		DetailsArgs.bHideSelectionTip = true;

		TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsArgs);
		DetailsView->SetObject(UTrueGlowSettings::Get());

		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(8)
				[
					SNew(SVerticalBox)
					// 说明
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("WindowHint",
							"改动即时生效于所有视口（编辑器视口需开启 Realtime）。预设按钮会覆盖下方参数并存盘；手动改任何参数会回落 Custom。"))
						.AutoWrapText(true)
					]
					// 预设按钮行
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("PresetWuWa", "鸣潮预设"))
							.OnClicked_Lambda([]()
							{
								UTrueGlowBlueprintLibrary::ApplyPreset(ETrueGlowPreset::WuWa);
								UTrueGlowBlueprintLibrary::SaveSettings();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("PresetNeon", "Neon 预设"))
							.OnClicked_Lambda([]()
							{
								UTrueGlowBlueprintLibrary::ApplyPreset(ETrueGlowPreset::Neon);
								UTrueGlowBlueprintLibrary::SaveSettings();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("PresetSubtle", "Subtle 预设"))
							.OnClicked_Lambda([]()
							{
								UTrueGlowBlueprintLibrary::ApplyPreset(ETrueGlowPreset::Subtle);
								UTrueGlowBlueprintLibrary::SaveSettings();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("SetupVolume", "关引擎 Bloom"))
							.ToolTipText(LOCTEXT("SetupVolumeTT", "生成全局 PostProcessVolume 并把引擎自带 BloomIntensity 设 0（防双重辉光）"))
							.OnClicked_Lambda([]()
							{
								UTrueGlowBlueprintLibrary::SetupEngineBloomOffVolume();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("SaveSettings", "存盘"))
							.OnClicked_Lambda([]()
							{
								UTrueGlowBlueprintLibrary::SaveSettings();
								return FReply::Handled();
							})
						]
					]
					// 参数面板
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SBorder)
						.Padding(4)
						[
							SNew(SBox)
							.MinDesiredHeight(400)
							[
								DetailsView
							]
						]
					]
				]
			];
	}
}

/**
 * 编辑器模块：Window→TrueGlow 参数面板（唯一 UI 入口）。
 * 注意：故意不再向 ISettingsModule 注册 Project Settings 页——那会在 Window 菜单
 * 产生第二个条目（与面板重复），所有参数已全部在面板里。
 */
class FTrueGlowEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Window → TrueGlow 面板（唯一入口）
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner("TrueGlowWindow",
			FOnSpawnTab::CreateStatic(&SpawnTrueGlowTab))
			.SetDisplayName(LOCTEXT("TabTitle", "TrueGlow"))
			.SetTooltipText(LOCTEXT("TabTooltip", "TrueGlow 物理辉光参数面板"))
			.SetMenuType(ETabSpawnerMenuType::Enabled);

		UE_LOG(LogTrueGlowEditor, Log, TEXT("TrueGlow panel tab registered (Window menu)."));
	}

	virtual void ShutdownModule() override
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("TrueGlowWindow");
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrueGlowEditorModule, TrueGlowEditor)
