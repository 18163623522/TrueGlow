// Copyright pengxiwei. All Rights Reserved.

// KuroGlow 的 7 个全局像素着色器声明。
// 仅被 KuroGlowViewExtension.cpp include 一次（IMPLEMENT_GLOBAL_SHADER 要求单翻译单元）。

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ScreenPass.h" // FScreenPassTextureViewportParameters

// 临时二分开关已定位根因（模块必须 PostConfigInit 加载，早于 InitializeShaderTypes）
#define KG_SHADERS_ENABLED 1

#if KG_SHADERS_ENABLED

// ---------------------------------------------------------------------------
// 1) BrightPass + 降到 1/2 分辨率：4 tap，每 tap 先做软阈值再平均（保尖峰能量）
class FKuroGlowBrightDownsamplePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowBrightDownsamplePS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowBrightDownsamplePS, FGlobalShader);

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
class FKuroGlowDownsamplePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowDownsamplePS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowDownsamplePS, FGlobalShader);

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
class FKuroGlowBlurPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowBlurPS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowBlurPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(FVector2D, Direction)
		SHADER_PARAMETER(uint32, Radius)
		SHADER_PARAMETER(float, Sigma)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

// ---------------------------------------------------------------------------
// 4) 9-tap tent 升采样并叠加：Out = High + Tent9(Low) * Weight
class FKuroGlowTentUpsampleAddPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowTentUpsampleAddPS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowTentUpsampleAddPS, FGlobalShader);

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
class FKuroGlowStreakPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowStreakPS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowStreakPS, FGlobalShader);

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
class FKuroGlowGlarePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowGlarePS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowGlarePS, FGlobalShader);

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
class FKuroGlowCompositePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKuroGlowCompositePS);
	SHADER_USE_PARAMETER_STRUCT(FKuroGlowCompositePS, FGlobalShader);

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

IMPLEMENT_GLOBAL_SHADER(FKuroGlowBrightDownsamplePS, "/Plugin/KuroGlow/Private/KuroGlowBrightDownsample.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKuroGlowDownsamplePS, "/Plugin/KuroGlow/Private/KuroGlowDownsample.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKuroGlowBlurPS, "/Plugin/KuroGlow/Private/KuroGlowBlurGaussian.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKuroGlowTentUpsampleAddPS, "/Plugin/KuroGlow/Private/KuroGlowTentUpsampleAdd.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKuroGlowStreakPS, "/Plugin/KuroGlow/Private/KuroGlowStreak.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKuroGlowGlarePS, "/Plugin/KuroGlow/Private/KuroGlowGlare.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKuroGlowCompositePS, "/Plugin/KuroGlow/Private/KuroGlowComposite.usf", "MainPS", SF_Pixel);

#endif // KG_SHADERS_ENABLED
