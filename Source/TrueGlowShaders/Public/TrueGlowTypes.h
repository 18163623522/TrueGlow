// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 渲染线程每帧消费的参数快照（游戏线程从 UTrueGlowSettings + CVar 构建）。
 * 纯 POD，拷贝开销可忽略。
 */
struct FTrueGlowParams
{
	// General
	bool bEnabled = true;
	bool bAllowSceneCapture = false;

	// Bloom（金字塔）
	bool bBloomEnabled = true;
	float BloomIntensity = 1.0f;
	float BloomThreshold = 1.0f;
	float BloomKnee = 0.6f;
	float BloomBrightMultiplier = 1.0f; // 过阈值能量增益（REAL BLOOM 的 bright-pass multiplier）
	int32 BloomLevels = 6;          // 1..6
	float BloomBlurRadius = 3.0f;   // 每级高斯半径（像素，0 = 关）
	int32 GaussianIterations = 2;   // 1..2 每级 H/V 模糊迭代数（高斯感柔化）
	bool bBloomFastMode = false;    // 跳过每级高斯保帧率（REAL BLOOM 的 fast mode）
	// 电影质感 Cinematic
	float ChromaticDispersion = 0.18f; // 镜头色散：径向 RGB 分离的彩虹边（0=关）
	bool bLensDirt = false;            // 脏镜头光晕（程序化污渍调制整体辉光）
	float LensDirtIntensity = 1.0f;    // 脏镜头强度 0-3
	float FilmSoftIntensity = 0.0f;    // 电影柔光层强度（宽半径低频清洗，0=关）
	float FilmSoftRadius = 3.0f;       // 柔光半径 1-8
	float DualTintStrength = 0.0f;     // 双色温渐染：亮芯偏暖、外晕偏冷（0=关）
	FLinearColor WarmCoreColor = FLinearColor(1.00f, 0.95f, 0.85f, 1.0f);
	FLinearColor CoolFringeColor = FLinearColor(0.80f, 0.90f, 1.15f, 1.0f);
	FVector2D BloomScale = FVector2D(1.0f, 1.0f); // 高斯核 X/Y 各向异性缩放（kernel scale）
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
	float StreakThickness = 2.0f;     // 垂直宽度（像素，最细 0.25）
	bool bStreakDualLine = false;    // 双线横光：主横光上下各复制一道平行细光
	float StreakDualLineSeparation = 12.0f; // 双线间距（像素@1080）
	float StreakDualLineIntensity = 0.5f;   // 双线强度（0-1，相对主横光）
	float StreakAttenuation = 0.35f;  // 衰减率，越大光条越收
	int32 StreakPasses = 4;           // 1..8 迭代次数
	bool bStreakOwnThreshold = false; // 光条独立阈值（默认关 = 与 bloom 共用，保持现有观感）
	float StreakThreshold = 2.0f;     // 独立阈值亮度

	// 纵向 streak（灯管上下漏光，REAL BLOOM 的 vertical streak；衰减/tint/次数与横向共用）
	bool bStreakVerticalEnabled = false;
	float StreakVerticalIntensity = 0.25f;
	float StreakVerticalLength = 240.0f; // 参考 1080p 像素

	// Glare（星芒）
	bool bGlareEnabled = true;
	float GlareIntensity = 0.25f;
	FLinearColor GlareTint = FLinearColor(1, 1, 1, 1);
	int32 GlareDirections = 4;        // 4 或 6
	float GlareRadius1 = 24.0f;       // 短半径（像素）
	float GlareRadius2 = 72.0f;       // 长半径（像素）
	int32 GlareTaps = 6;              // 每方向每半径采样数
};
