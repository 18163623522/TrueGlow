// Copyright pengxiwei. All Rights Reserved.

// TrueGlow 的 7 个全局像素着色器声明。
// 仅被 TrueGlowViewExtension.cpp include 一次（IMPLEMENT_GLOBAL_SHADER 要求单翻译单元）。

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ScreenPass.h" // FScreenPassTextureViewportParameters

// 临时二分开关已定位根因（模块必须 PostConfigInit 加载，早于 InitializeShaderTypes）
#define KG_SHADERS_ENABLED 1

#if KG_SHADERS_ENABLED

// ---------------------------------------------------------------------------
// 1) BrightPass + 降到 1/2 分辨率：4 tap，每 tap 先做软阈值再平均（保尖峰能量）
class FTrueGlowBrightDownsamplePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowBrightDownsamplePS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowBrightDownsamplePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER(FVector4, InputUVScaleBias)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(float, Threshold)
		SHADER_PARAMETER(float, Knee)
		SHADER_PARAMETER(FVector4, Tint)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 2) 通用 4-tap box 降采样（带 tint）
class FTrueGlowDownsamplePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowDownsamplePS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowDownsamplePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER(FVector4, InputUVScaleBias)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(FVector4, Tint)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 3) 可分离高斯（H/V 由 Direction 决定）
class FTrueGlowBlurPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowBlurPS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowBlurPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(FVector2D, Direction)
		SHADER_PARAMETER(uint32, Radius)
		SHADER_PARAMETER(float, Sigma)
		SHADER_PARAMETER(FVector2D, DirectionalScale)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 4) 9-tap tent 升采样并叠加：Out = High + Tent9(Low) * Weight
class FTrueGlowTentUpsampleAddPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowTentUpsampleAddPS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowTentUpsampleAddPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HighTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, HighSampler)
		SHADER_PARAMETER(FVector4, Weight)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 5) Streak：对称 exp 衰减方向模糊（横向拉光条 / 纵向加粗）
class FTrueGlowStreakPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowStreakPS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowStreakPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(FVector2D, Direction)
		SHADER_PARAMETER(float, StepPixels)
		SHADER_PARAMETER(uint32, Taps)
		SHADER_PARAMETER(float, Attenuation)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 6) Glare：多方向双半径星芒
class FTrueGlowGlarePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowGlarePS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowGlarePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(float, Radius1Pixels)
		SHADER_PARAMETER(float, Radius2Pixels)
		SHADER_PARAMETER(uint32, TapsPerDirection)
		SHADER_PARAMETER(uint32, DirectionCount)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 7) 全分辨率合成：Scene + Bloom*mul + Streak*mul + Glare*mul
class FTrueGlowCompositePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTrueGlowCompositePS);
	SHADER_USE_PARAMETER_STRUCT(FTrueGlowCompositePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Scene)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER(FVector4, SceneUVScaleBias)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BloomTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, BloomSampler)
		SHADER_PARAMETER(FVector4, BloomMul)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, StreakTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, StreakSampler)
		SHADER_PARAMETER(FVector4, StreakMul)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, StreakTexture2)
		SHADER_PARAMETER_SAMPLER(SamplerState, Streak2Sampler)
		SHADER_PARAMETER(FVector4, Streak2Mul)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GlareTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, GlareSampler)
		SHADER_PARAMETER(FVector4, GlareMul)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

IMPLEMENT_GLOBAL_SHADER(FTrueGlowBrightDownsamplePS, "/Plugin/TrueGlow/Private/TrueGlowBrightDownsample.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FTrueGlowDownsamplePS, "/Plugin/TrueGlow/Private/TrueGlowDownsample.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FTrueGlowBlurPS, "/Plugin/TrueGlow/Private/TrueGlowBlurGaussian.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FTrueGlowTentUpsampleAddPS, "/Plugin/TrueGlow/Private/TrueGlowTentUpsampleAdd.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FTrueGlowStreakPS, "/Plugin/TrueGlow/Private/TrueGlowStreak.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FTrueGlowGlarePS, "/Plugin/TrueGlow/Private/TrueGlowGlare.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FTrueGlowCompositePS, "/Plugin/TrueGlow/Private/TrueGlowComposite.usf", "MainPS", SF_Pixel);

#endif // KG_SHADERS_ENABLED
