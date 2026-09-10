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
	int32 BloomLevels = 6;          // 1..8
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
	float BloomLevelWeights[8] = { 0.6f, 0.7f, 0.8f, 0.95f, 1.1f, 1.25f, 1.4f, 1.55f };
	FLinearColor BloomLevelTints[8] = {
		FLinearColor(0.95f, 0.98f, 1.00f, 1.0f),
		FLinearColor(0.90f, 0.95f, 1.05f, 1.0f),
		FLinearColor(0.85f, 0.92f, 1.10f, 1.0f),
		FLinearColor(0.80f, 0.90f, 1.15f, 1.0f),
		FLinearColor(0.75f, 0.87f, 1.20f, 1.0f),
		FLinearColor(0.70f, 0.85f, 1.25f, 1.0f),
		FLinearColor(0.65f, 0.83f, 1.30f, 1.0f),
		FLinearColor(0.60f, 0.80f, 1.35f, 1.0f),
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

	// 镜头光斑 Flare（幻影 Ghost + 光环 Halo）
	float GhostIntensity = 0.0f;     // 幻影强度（0=关）
	int32 GhostCount = 4;           // 1-8 枚
	float GhostSpacing = 0.45f;     // 沿光轴间距系数
	float GhostDispersal = 1.0f;    // 幻影彩虹色散
	float HaloIntensity = 0.0f;     // 光环强度（0=关）
	float HaloRadius = 0.12f;       // 光环半径（UV 比例）
	FLinearColor FlareTint = FLinearColor(1, 1, 1, 1);

	// 星芒镜 Star Filter（Optical Flares/Knoll 风格：多条任意角度长芒，Mip 级联无断层）
	bool bStarFilter = false;
	float StarFilterIntensity = 0.8f;
	int32 StarFilterRays = 4;        // 芒数 4/6/8
	float StarFilterRotation = 15.0f; // 旋转角（度）
	float StarFilterLength = 720.0f;  // 像素@1080
	FLinearColor StarFilterTint = FLinearColor(1, 1, 1, 1);

	// 多边形光圈 Poly Iris（光圈叶片形状光斑，光谱染色）
	float PolyIrisIntensity = 0.0f;
	int32 PolyIrisSides = 6;
	float PolyIrisRotation = 0.0f;
	float PolyIrisRadius = 0.10f;

	// 光谱扇 Chroma Fan（彩虹扇形光束）
	float FanIntensity = 0.0f;
	int32 FanCount = 7;
	float FanSpread = 40.0f;
	float FanRadius = 0.18f;

	// 光束 GodRays（屏幕空间径向体积光，AE Trapcode Shine 式）
	bool bGodRays = false;
	float GodRaysIntensity = 1.0f;   // 强度
	FVector2D GodRaysLightPos = FVector2D(0.5f, 0.35f); // 光源屏幕位置（0-1，可由蓝图按太阳投影驱动）
	float GodRaysLength = 0.9f;      // 采样长度（0.05-1）
	float GodRaysDecay = 0.965f;     // 逐采样衰减（0.8-0.999）
	float GodRaysDensity = 1.0f;     // 采样密度（0.3-3）
	FLinearColor GodRaysTint = FLinearColor(1, 1, 1, 1);

	// Glare（星芒）
	bool bGlareEnabled = true;
	float GlareIntensity = 0.25f;
	FLinearColor GlareTint = FLinearColor(1, 1, 1, 1);
	int32 GlareDirections = 4;        // 4 或 6
	float GlareRadius1 = 24.0f;       // 短半径（像素）
	float GlareRadius2 = 72.0f;       // 长半径（像素）
	int32 GlareTaps = 6;              // 每方向每半径采样数
};
