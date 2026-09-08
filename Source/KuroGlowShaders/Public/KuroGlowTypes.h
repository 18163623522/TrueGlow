// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 渲染线程每帧消费的参数快照（游戏线程从 UKuroGlowSettings + CVar 构建）。
 * 纯 POD，拷贝开销可忽略。
 */
struct FKuroGlowParams
{
	// General
	bool bEnabled = true;
	bool bAllowSceneCapture = false;

	// Bloom（金字塔）
	bool bBloomEnabled = true;
	float BloomIntensity = 1.0f;
	float BloomThreshold = 1.0f;
	float BloomKnee = 0.6f;
	int32 BloomLevels = 6;          // 1..6
	float BloomBlurRadius = 2.0f;   // 每级高斯半径（像素，0 = 关）
	float BloomLevelWeights[6] = { 0.6f, 0.7f, 0.8f, 0.95f, 1.1f, 1.25f };
	FLinearColor BloomLevelTints[6] = {
		FLinearColor(0.95f, 0.98f, 1.00f, 1.0f),
		FLinearColor(0.90f, 0.95f, 1.05f, 1.0f),
		FLinearColor(0.85f, 0.92f, 1.10f, 1.0f),
		FLinearColor(0.80f, 0.90f, 1.15f, 1.0f),
		FLinearColor(0.75f, 0.87f, 1.20f, 1.0f),
		FLinearColor(0.70f, 0.85f, 1.25f, 1.0f),
	};

	// Anamorphic streak（横向光条）
	bool bStreakEnabled = true;
	float StreakIntensity = 0.35f;
	FLinearColor StreakTint = FLinearColor(0.55f, 0.75f, 1.0f, 1.0f);
	float StreakLength = 480.0f;      // 参考 1080p 像素长度
	float StreakThickness = 2.0f;     // 垂直粗细（像素）
	float StreakAttenuation = 0.35f;  // 衰减率，越大光条越收
	int32 StreakPasses = 4;           // 1..8 迭代次数

	// Glare（星芒）
	bool bGlareEnabled = true;
	float GlareIntensity = 0.25f;
	FLinearColor GlareTint = FLinearColor(1, 1, 1, 1);
	int32 GlareDirections = 4;        // 4 或 6
	float GlareRadius1 = 24.0f;       // 短半径（像素）
	float GlareRadius2 = 72.0f;       // 长半径（像素）
	int32 GlareTaps = 6;              // 每方向每半径采样数
};
