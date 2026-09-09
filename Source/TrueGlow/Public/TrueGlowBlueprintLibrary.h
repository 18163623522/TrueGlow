// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TrueGlowSettings.h"
#include "TrueGlowBlueprintLibrary.generated.h"

/**
 * 蓝图/运行时参数入口：全部即时生效（每帧重新快照），不自动写盘（要持久化调 SaveSettings）。
 * 典型用途：过场按镜头调辉光、游戏状态驱动（低血量红晕可在上层用 tint 实现）、昼夜循环。
 */
UCLASS()
class TRUEGLOW_API UTrueGlowBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ---------------- 总控 ----------------
	UFUNCTION(BlueprintCallable, Category = "TrueGlow")
	static void SetEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "TrueGlow", BlueprintPure)
	static bool IsEnabled();

	// ---------------- Bloom ----------------
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomIntensity(float InIntensity);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomThreshold(float InThreshold);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomLevels(int32 InLevels);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomBrightMultiplier(float InMultiplier);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomFastMode(bool bInFast);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetBloomScale(FVector2D InScale);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetGaussianIterations(int32 InIterations);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Bloom") static void SetChromaticDispersion(float InDispersion);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Cinematic") static void SetFilmSoftIntensity(float InIntensity);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Cinematic") static void SetFilmSoftRadius(float InRadius);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Cinematic") static void SetDualTintStrength(float InStrength);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Cinematic") static void SetLensDirtEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Cinematic") static void SetLensDirtIntensity(float InIntensity);

	// ---------------- Streak（横向） ----------------
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetStreakEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetStreakIntensity(float InIntensity);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetStreakLength(float InLengthPx);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetStreakTint(FLinearColor InTint);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetStreakOwnThreshold(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetStreakThreshold(float InThreshold);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetDualLineEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetDualLineSeparation(float InPx);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetDualLineIntensity(float InIntensity);

	// ---------------- Streak（纵向） ----------------
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetVerticalStreakEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetVerticalStreakIntensity(float InIntensity);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Streak") static void SetVerticalStreakLength(float InLengthPx);

	// ---------------- Glare ----------------
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Glare") static void SetGlareEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Glare") static void SetGlareIntensity(float InIntensity);
	UFUNCTION(BlueprintCallable, Category = "TrueGlow|Glare") static void SetGlareTint(FLinearColor InTint);

	// ---------------- 预设与工具 ----------------
	/** 应用三套预设之一（WuWa/Neon/Subtle）。 */
	UFUNCTION(BlueprintCallable, Category = "TrueGlow")
	static void ApplyPreset(ETrueGlowPreset InPreset);

	/** 在当前世界生成/复用全局 PostProcessVolume 并关闭引擎自带 bloom。 */
	UFUNCTION(BlueprintCallable, Category = "TrueGlow")
	static void SetupEngineBloomOffVolume();

	/** 把当前全部参数持久化到 DefaultEngine.ini。 */
	UFUNCTION(BlueprintCallable, Category = "TrueGlow")
	static void SaveSettings();
};
