// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowViewExtension.h"
#include "TrueGlowShaders.h"
#include "TrueGlowShaderBridge.h"

#include "PostProcess/PostProcessMaterial.h"

#include "GlobalShader.h"
#include "PixelShaderUtils.h"
#include "RenderGraph.h"
#include "SceneView.h"
#include "RHI.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "GPUProfiler.h"

// stat gpu 里的命名分组（RealBloom 同款做法）
DECLARE_GPU_STAT_NAMED(TrueGlowBloom, TEXT("TrueGlow.Bloom"));
DECLARE_GPU_STAT_NAMED(TrueGlowStreak, TEXT("TrueGlow.Streak"));
DECLARE_GPU_STAT_NAMED(TrueGlowGlare, TEXT("TrueGlow.Glare"));
DECLARE_GPU_STAT_NAMED(TrueGlowFlare, TEXT("TrueGlow.Flare"));
DECLARE_GPU_STAT_NAMED(TrueGlowComposite, TEXT("TrueGlow.Composite"));

namespace
{
	FRDGTextureRef CreateGlowTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, const TCHAR* Name)
	{
		const FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(
			Extent, PF_FloatRGB, FClearValueBinding(FLinearColor::Black), TexCreate_RenderTargetable);
		return GraphBuilder.CreateTexture(Desc, Name);
	}

	FScreenPassTextureViewportParameters GetExactViewportParams(FIntPoint Extent)
	{
		return GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Extent));
	}

	FVector4 ToWeight4(float W) { return FVector4(W, W, W, 0.0f); }

	FVector4 ToTint4(const FLinearColor& Tint)
	{
		return FVector4(FMath::Clamp(Tint.R, 0.0f, 8.0f), FMath::Clamp(Tint.G, 0.0f, 8.0f), FMath::Clamp(Tint.B, 0.0f, 8.0f), 1.0f);
	}

	FIntPoint Halve(FIntPoint InSize)
	{
		FIntPoint Out = FIntPoint::DivideAndRoundUp(InSize, FIntPoint(2, 2));
		Out.X = FMath::Max(1, Out.X);
		Out.Y = FMath::Max(1, Out.Y);
		return Out;
	}
}

FTrueGlowViewExtension::FTrueGlowViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	Params = FTrueGlowShaderBridge::GetParamsSnapshot();
}

TSharedRef<FTrueGlowViewExtension, ESPMode::ThreadSafe> FTrueGlowViewExtension::Create()
{
	return FSceneViewExtensions::NewExtension<FTrueGlowViewExtension>();
}

void FTrueGlowViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	FScopeLock Lock(&ParamsCS);
	Params = FTrueGlowShaderBridge::GetParamsSnapshot();
}

bool FTrueGlowViewExtension::IsActiveThisFrameInContext(FSceneViewExtensionContext& Context) const
{
	return TrueGlowCVars::IsEnabled() && FTrueGlowShaderBridge::GetParamsSnapshot().bEnabled;
}

void FTrueGlowViewExtension::SubscribeToPostProcessingPass(
	EPostProcessingPass Pass,
	FAfterPassCallbackDelegateArray& InOutPassCallbacks,
	bool bIsPassEnabled)
{
	// MotionBlur 槽位的 after-pass 在 4.26 无条件执行（FPostProcessing::Process 中位于
	// TAA 之后、曝光/Bloom/tonemap 之前的 HDR 线性段），是唯一稳定的 HDR 注入点。
	if (Pass == EPostProcessingPass::MotionBlur)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(
			this, &FTrueGlowViewExtension::AfterMotionBlur_RenderThread));
	}
}

FScreenPassTexture FTrueGlowViewExtension::AfterMotionBlur_RenderThread(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FPostProcessMaterialInputs& Inputs)
{
	FScreenPassTexture SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);

	if (!SceneColor.IsValid() || SceneColor.Texture == nullptr)
	{
		return SceneColor;
	}

	FTrueGlowParams P;
	{
		FScopeLock Lock(&ParamsCS);
		P = Params;
	}

	const bool bSkipSceneCapture = View.bIsSceneCapture && !P.bAllowSceneCapture;
	if (!P.bEnabled || bSkipSceneCapture)
	{
		return SceneColor;
	}

	const FIntPoint FullSize = SceneColor.ViewRect.Size();
	if (FullSize.X < 8 || FullSize.Y < 8)
	{
		return SceneColor;
	}

#if !KG_SHADERS_ENABLED
	// 二分定位中：透传
	return SceneColor;
#else

	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	FRHISamplerState* BilinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	static bool bLoggedFirstFrame = false;
	if (!bLoggedFirstFrame)
	{
		bLoggedFirstFrame = true;
		UE_LOG(LogTemp, Log, TEXT("TrueGlow: first AfterMotionBlur pass executing (%dx%d)."), FullSize.X, FullSize.Y);
	}

	const FIntPoint HalfSize = Halve(FullSize);
	const FIntPoint QuarterSize = Halve(HalfSize);

	const FScreenPassTextureViewportParameters SceneParams =
		GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneColor));

	const bool bNeedBright = P.bBloomEnabled || P.bStreakEnabled || P.bStreakVerticalEnabled || P.bGlareEnabled;

	RDG_EVENT_SCOPE(GraphBuilder, "TrueGlow");

	// 禁用通道的黑色占位（clear binding 为黑，无需真正渲染）
	FRDGTextureRef BlackDummy = CreateGlowTexture(GraphBuilder, FIntPoint(4, 4), TEXT("TrueGlow.Black"));

	// ------------------------------------------------------------------
	// 0) BrightPass：软阈值 + 4-tap 降到 1/2（bloom 链用；streak 独立阈值时再跑一次）
	auto AddBrightHalfPass = [&](float Threshold, float Knee, const FVector4& Tint, const TCHAR* Name) -> FRDGTextureRef
	{
		const FScreenPassTextureViewportParameters HalfParams = GetExactViewportParams(HalfSize);
		const FScreenPassTextureViewportTransform Transform =
			GetScreenPassTextureViewportTransform(SceneParams, HalfParams);

		FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, HalfSize, Name);

		FTrueGlowBrightDownsamplePS::FParameters* Prm =
			GraphBuilder.AllocParameters<FTrueGlowBrightDownsamplePS::FParameters>();
		Prm->Input = SceneParams;
		Prm->Output = HalfParams;
		Prm->InputUVScaleBias = FVector4(Transform.Scale.X, Transform.Scale.Y, Transform.Bias.X, Transform.Bias.Y);
		Prm->InputTexture = SceneColor.Texture;
		Prm->InputSampler = BilinearClampSampler;
		Prm->Threshold = Threshold;
		Prm->Knee = Knee;
		Prm->Tint = Tint;
		Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FTrueGlowBrightDownsamplePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("TrueGlow.%s %dx%d", Name, HalfSize.X, HalfSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, HalfSize));
		return Target;
	};

	FRDGTextureRef BrightHalf = BlackDummy;
	if (bNeedBright)
	{
		BrightHalf = AddBrightHalfPass(P.BloomThreshold, P.BloomKnee,
			FVector4(
				P.BloomLevelTints[0].R * P.BloomBrightMultiplier,
				P.BloomLevelTints[0].G * P.BloomBrightMultiplier,
				P.BloomLevelTints[0].B * P.BloomBrightMultiplier,
				1.0f),
			TEXT("BrightHalf"));
	}

	// 通用降采样 lambda（作用于我们自己的等尺寸纹理，UV 变换恒等）
	auto AddDownsamplePass = [&](FRDGTextureRef Src, FIntPoint SrcExtent, FRDGTextureRef Dst, FIntPoint DstExtent, const FVector4& Tint, const TCHAR* Name)
	{
		FTrueGlowDownsamplePS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowDownsamplePS::FParameters>();
		Prm->Input = GetExactViewportParams(SrcExtent);
		Prm->Output = GetExactViewportParams(DstExtent);
		Prm->InputUVScaleBias = FVector4(1, 1, 0, 0);
		Prm->InputTexture = Src;
		Prm->InputSampler = BilinearClampSampler;
		Prm->Tint = Tint;
		Prm->RenderTargets[0] = FRenderTargetBinding(Dst, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FTrueGlowDownsamplePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("TrueGlow.%s %dx%d", Name, DstExtent.X, DstExtent.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, DstExtent));
	};

	// ------------------------------------------------------------------
	// 1) 1/4 亮部基底：glare 与默认 streak 用 bloom 阈值版；独立阈值时 streak 走专属链
	const bool bStreakFamily = P.bStreakEnabled || P.bStreakVerticalEnabled;
	FRDGTextureRef QuarterBase = nullptr;
	FRDGTextureRef StreakQuarterBase = nullptr;
	if (bStreakFamily || P.bGlareEnabled)
	{
		if (P.bGlareEnabled || !P.bStreakOwnThreshold)
		{
			QuarterBase = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.QuarterBase"));
			AddDownsamplePass(BrightHalf, HalfSize, QuarterBase, QuarterSize, FVector4(1, 1, 1, 1), TEXT("Quarter"));
		}

		if (bStreakFamily && P.bStreakOwnThreshold)
		{
			// 独立阈值链：SceneColor → 阈值亮部(½) → ¼（RealBloom 同款独立阈值能力）
			FRDGTextureRef StreakBrightHalf = AddBrightHalfPass(P.StreakThreshold, P.BloomKnee,
				FVector4(1, 1, 1, 1), TEXT("StreakBrightHalf"));
			StreakQuarterBase = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.StreakQuarterBase"));
			AddDownsamplePass(StreakBrightHalf, HalfSize, StreakQuarterBase, QuarterSize, FVector4(1, 1, 1, 1), TEXT("StreakQuarter"));
		}
	}
	FRDGTextureRef StreakBase = StreakQuarterBase ? StreakQuarterBase : QuarterBase;

	// Mip 级联链（断层根治 + 星芒镜复用）：StreakBase → 3 级降采样
	FRDGTextureRef MipTex[4] = { nullptr, nullptr, nullptr, nullptr };
	FIntPoint MipSz[4];
	if (StreakBase)
	{
		MipTex[0] = StreakBase;
		MipSz[0] = QuarterSize;
		for (int32 m = 1; m < 4; ++m)
		{
			MipSz[m] = Halve(MipSz[m - 1]);
			MipTex[m] = CreateGlowTexture(GraphBuilder, MipSz[m], *FString::Printf(TEXT("TrueGlow.StreakMip%d"), m));
			AddDownsamplePass(MipTex[m - 1], MipSz[m - 1], MipTex[m], MipSz[m], FVector4(1, 1, 1, 1),
				*FString::Printf(TEXT("StreakMip%d"), m));
		}
	}

	// Mip 级联主 pass：几何 tap 全距离覆盖（长光条无珠链断层）
	auto AddMipStreakPass = [&](
		FRDGTextureRef Dst, const FVector2D& Direction, float MaxDistTexels, const TCHAR* Name,
		FRDGTextureRef AccumSrc = nullptr,
		const FLinearColor& AccumMul = FLinearColor(0, 0, 0, 0),
		const FLinearColor& SelfTint = FLinearColor(1, 1, 1, 1))
	{
		const int32 MipTaps = 24;
		const float D0 = 1.5f;
		const float Growth = FMath::Pow(FMath::Max(MaxDistTexels, D0 * 2.0f) / D0, 1.0f / (MipTaps - 1));

		FTrueGlowStreakMipPS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowStreakMipPS::FParameters>();
		Prm->Input = GetExactViewportParams(QuarterSize);
		Prm->Output = GetExactViewportParams(QuarterSize);
		Prm->StreakMip0 = MipTex[0];
		Prm->StreakMip0Sampler = BilinearClampSampler;
		Prm->StreakMip1 = MipTex[1];
		Prm->StreakMip1Sampler = BilinearClampSampler;
		Prm->StreakMip2 = MipTex[2];
		Prm->StreakMip2Sampler = BilinearClampSampler;
		Prm->StreakMip3 = MipTex[3];
		Prm->StreakMip3Sampler = BilinearClampSampler;
		Prm->Direction = Direction;
		Prm->D0 = D0;
		Prm->Growth = Growth;
		Prm->Taps = MipTaps;
		Prm->Attenuation = P.StreakAttenuation;
		Prm->AccumTexture = AccumSrc;
		Prm->AccumSampler = BilinearClampSampler;
		Prm->AccumMul = FVector4(AccumMul.R, AccumMul.G, AccumMul.B, 1.0f);
		Prm->SelfTint = FVector4(SelfTint.R, SelfTint.G, SelfTint.B, 1.0f);
		Prm->RenderTargets[0] = FRenderTargetBinding(Dst, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FTrueGlowStreakMipPS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("TrueGlow.StreakMip.%s %dx%d", Name, QuarterSize.X, QuarterSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
	};

	// ------------------------------------------------------------------
	// 2) Bloom 金字塔：降采样链 + 每级高斯 + tent 升采样合并
	FRDGTextureRef BloomResult = BlackDummy;
	if (P.bBloomEnabled)
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowBloom);
		const int32 LevelCount = FMath::Clamp(P.BloomLevels, 1, 6);

		TArray<FRDGTextureRef, TInlineAllocator<6>> LevelTextures;
		TArray<FIntPoint, TInlineAllocator<6>> LevelExtents;
		LevelTextures.Add(BrightHalf);
		LevelExtents.Add(HalfSize);

		for (int32 i = 1; i < LevelCount; ++i)
		{
			const FIntPoint Extent = Halve(LevelExtents.Last());
			FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, Extent, *FString::Printf(TEXT("TrueGlow.L%d"), i));
			const FLinearColor& Tint = P.BloomLevelTints[i];
			AddDownsamplePass(LevelTextures.Last(), LevelExtents.Last(), Target, Extent,
				FVector4(Tint.R, Tint.G, Tint.B, 1.0f), *FString::Printf(TEXT("Down.L%d"), i));
			LevelTextures.Add(Target);
			LevelExtents.Add(Extent);
		}

		// 每级可分离高斯（H→V×迭代）；Fast 模式跳过保帧率；X/Y 各向异性缩放
		const int32 BlurRadius = FMath::Clamp(FMath::RoundToInt(P.BloomBlurRadius), 0, 8);
		const int32 BlurIterations = FMath::Clamp(P.GaussianIterations, 1, 2);
		if (BlurRadius > 0 && !P.bBloomFastMode)
		{
			const float Sigma = FMath::Max(0.5f, BlurRadius * 0.75f);
			for (int32 i = 0; i < LevelCount; ++i)
			{
				FRDGTextureRef Current = LevelTextures[i];
				const FIntPoint Extent = LevelExtents[i];

				for (int32 Iter = 0; Iter < BlurIterations; ++Iter)
				for (int32 Axis = 0; Axis < 2; ++Axis)
				{
					const FVector2D Direction = Axis == 0 ? FVector2D(1, 0) : FVector2D(0, 1);
					FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, Extent,
						*FString::Printf(TEXT("TrueGlow.L%d.Blur%c%d"), i, Axis == 0 ? TEXT('H') : TEXT('V'), Iter));

					FTrueGlowBlurPS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowBlurPS::FParameters>();
					Prm->Input = GetExactViewportParams(Extent);
					Prm->Output = GetExactViewportParams(Extent);
					Prm->InputTexture = Current;
					Prm->InputSampler = BilinearClampSampler;
					Prm->Direction = Direction;
					Prm->Radius = BlurRadius;
					Prm->Sigma = Sigma;
					Prm->DirectionalScale = P.BloomScale;
					Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

					TShaderMapRef<FTrueGlowBlurPS> Shader(ShaderMap);
					FPixelShaderUtils::AddFullscreenPass(
						GraphBuilder, ShaderMap,
						RDG_EVENT_NAME("TrueGlow.Blur.L%d.%c %dx%d", i, Axis == 0 ? TEXT('H') : TEXT('V'), Extent.X, Extent.Y),
						Shader, Prm, FIntRect(FIntPoint::ZeroValue, Extent));

					Current = Target;
				}
				LevelTextures[i] = Current;
			}
		}

		// tent 升采样合并：Coarse -> Fine
		FRDGTextureRef Current = LevelTextures[LevelCount - 1];
		for (int32 i = LevelCount - 2; i >= 0; --i)
		{
			const FIntPoint Extent = LevelExtents[i];
			FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, Extent,
				*FString::Printf(TEXT("TrueGlow.Merge.L%d"), i));

			FTrueGlowTentUpsampleAddPS::FParameters* Prm =
				GraphBuilder.AllocParameters<FTrueGlowTentUpsampleAddPS::FParameters>();
			Prm->Input = GetExactViewportParams(LevelExtents[i + 1]);
			Prm->Output = GetExactViewportParams(Extent);
			Prm->InputTexture = Current;           // 粗级
			Prm->InputSampler = BilinearClampSampler;
			Prm->HighTexture = LevelTextures[i];   // 细级（同输出分辨率）
			Prm->HighSampler = BilinearClampSampler;
			Prm->Weight = ToWeight4(P.BloomLevelWeights[i + 1]);
			Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

			TShaderMapRef<FTrueGlowTentUpsampleAddPS> Shader(ShaderMap);
			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder, ShaderMap,
				RDG_EVENT_NAME("TrueGlow.Merge.L%d %dx%d", i, Extent.X, Extent.Y),
				Shader, Prm, FIntRect(FIntPoint::ZeroValue, Extent));

			Current = Target;
		}
		BloomResult = Current;
	}

	// ------------------------------------------------------------------
	// 3) Streak：横向衰减模糊迭代 + 一次纵向加粗；可选独立纵向光条组
	FRDGTextureRef StreakResult = BlackDummy;
	FRDGTextureRef StreakResultV = BlackDummy;
	if ((P.bStreakEnabled || P.bStreakVerticalEnabled) && StreakBase)
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowStreak);
		const float HeightScale = static_cast<float>(QuarterSize.Y) / 1080.0f;
		const int32 StreakTaps = 12;
		const int32 Passes = FMath::Clamp(P.StreakPasses, 1, 8);


		auto AddStreakPass = [&](FRDGTextureRef Src, FRDGTextureRef Dst, const FVector2D& Direction, float StepPx, int32 Taps, float Attenuation, const TCHAR* Name)
		{
			FTrueGlowStreakPS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowStreakPS::FParameters>();
			Prm->Input = GetExactViewportParams(QuarterSize);
			Prm->Output = GetExactViewportParams(QuarterSize);
			Prm->InputTexture = Src;
			Prm->InputSampler = BilinearClampSampler;
			Prm->Direction = Direction;
			Prm->StepPixels = StepPx;
			Prm->Taps = Taps;
			Prm->Attenuation = Attenuation;
			Prm->RenderTargets[0] = FRenderTargetBinding(Dst, ERenderTargetLoadAction::ENoAction);

			TShaderMapRef<FTrueGlowStreakPS> Shader(ShaderMap);
			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder, ShaderMap,
				RDG_EVENT_NAME("TrueGlow.Streak.%s %dx%d", Name, QuarterSize.X, QuarterSize.Y),
				Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
		};

		if (P.bStreakEnabled)
		{
			const float MaxDistH = FMath::Max(4.0f, P.StreakLength * HeightScale);

			FRDGTextureRef Current = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.StreakH"));
			AddMipStreakPass(Current, FVector2D(1, 0), MaxDistH, TEXT("H"));
			// 平滑迭代（小步长无断层风险）
			for (int32 i = 1; i < Passes; ++i)
			{
				FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize,
					*FString::Printf(TEXT("TrueGlow.StreakH%d"), i));
				AddStreakPass(Current, Target, FVector2D(1, 0), 2.0f, StreakTaps, P.StreakAttenuation,
					*FString::Printf(TEXT("Hs%d"), i));
				Current = Target;
			}

			// 纵向定宽（thickness，可亚像素）
			FRDGTextureRef Thick = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.StreakThick"));
			AddStreakPass(Current, Thick, FVector2D(0, 1),
				FMath::Max(0.25f, P.StreakThickness * HeightScale), 2, 0.5f, TEXT("V"));
			StreakResult = Thick;
		}

		// 独立纵向光条（灯管上下漏光；衰减/tint/迭代与横向共用）
		if (P.bStreakVerticalEnabled)
		{
			const float MaxDistV = FMath::Max(4.0f, P.StreakVerticalLength * HeightScale);

			FRDGTextureRef Current = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.StreakV"));
			AddMipStreakPass(Current, FVector2D(0, 1), MaxDistV, TEXT("W"));
			for (int32 i = 1; i < Passes; ++i)
			{
				FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize,
					*FString::Printf(TEXT("TrueGlow.StreakV%d"), i));
				AddStreakPass(Current, Target, FVector2D(0, 1), 2.0f, StreakTaps, P.StreakAttenuation,
					*FString::Printf(TEXT("Ws%d"), i));
				Current = Target;
			}

			// 横向收窄（对称于横向组的 thickness，可亚像素）
			FRDGTextureRef Thin = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.StreakVThin"));
			AddStreakPass(Current, Thin, FVector2D(1, 0),
				FMath::Max(0.25f, P.StreakThickness * HeightScale), 2, 0.5f, TEXT("Thin"));
			StreakResultV = Thin;
		}
	}

	// ------------------------------------------------------------------
	// 4) Glare：多方向双半径星芒
	FRDGTextureRef GlareResult = BlackDummy;
	if (P.bGlareEnabled && QuarterBase)
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowGlare);
		const float HeightScale = static_cast<float>(QuarterSize.Y) / 1080.0f;
		FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.Glare"));

		FTrueGlowGlarePS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowGlarePS::FParameters>();
		Prm->Input = GetExactViewportParams(QuarterSize);
		Prm->Output = GetExactViewportParams(QuarterSize);
		Prm->InputTexture = QuarterBase;
		Prm->InputSampler = BilinearClampSampler;
		Prm->Radius1Pixels = FMath::Max(2.0f, P.GlareRadius1 * HeightScale);
		Prm->Radius2Pixels = FMath::Max(4.0f, P.GlareRadius2 * HeightScale);
		Prm->TapsPerDirection = FMath::Clamp(P.GlareTaps, 2, 16);
		Prm->DirectionCount = (P.GlareDirections == 6) ? 6u : 4u;
		Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FTrueGlowGlarePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("TrueGlow.Glare %dx%d", QuarterSize.X, QuarterSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
		GlareResult = Target;
	}

	// ------------------------------------------------------------------
	// 4') 星芒镜 Star Filter：多条任意角度长芒（Mip 级联链式叠加，无断层）
	//     与 4'') 镜头光斑共享 TrueGlow.Flare 统计
	FRDGTextureRef StarAccum = nullptr;
	if (P.bStarFilter && P.StarFilterIntensity > 0.001f && (StreakBase || QuarterBase))
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowFlare);
		const float SFHeightScale = static_cast<float>(QuarterSize.Y) / 1080.0f;
		const float SFMaxDist = FMath::Max(4.0f, P.StarFilterLength * SFHeightScale);
		const int32 Lines = FMath::Clamp(P.StarFilterRays, 4, 8) / 2;   // N 芒 = N/2 条过中心直线
		const float RotRad = P.StarFilterRotation * (PI / 180.0f);

		for (int32 Li = 0; Li < Lines; ++Li)
		{
			const float Ang = RotRad + Li * PI / Lines;
			FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize,
				*FString::Printf(TEXT("TrueGlow.StarRay%d"), Li));
			AddMipStreakPass(Target, FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)), SFMaxDist,
				*FString::Printf(TEXT("Ray%d"), Li),
				StarAccum, Li > 0 ? FLinearColor(1, 1, 1, 1) : FLinearColor(0, 0, 0, 0),
				P.StarFilterTint);
			StarAccum = Target;
		}
	}

	// ------------------------------------------------------------------
	// 4'') 镜头光斑：幻影 Ghost + 光环 Halo + 多边形光圈 + 光谱扇（含星芒镜累加）
	FRDGTextureRef FlareResult = BlackDummy;
	{
		FRDGTextureRef FlareBase = StreakBase ? StreakBase : QuarterBase;
		const bool bWantFlare = FlareBase &&
			(P.GhostIntensity > 0.001f || P.HaloIntensity > 0.001f ||
			 P.PolyIrisIntensity > 0.001f || P.FanIntensity > 0.001f || StarAccum);
		if (bWantFlare)
		{
			RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowFlare);
			FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.Flare"));

			FTrueGlowFlarePS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowFlarePS::FParameters>();
			Prm->Input = GetExactViewportParams(QuarterSize);
			Prm->Output = GetExactViewportParams(QuarterSize);
			Prm->InputTexture = FlareBase;
			Prm->InputSampler = BilinearClampSampler;
			Prm->GhostCount = FMath::Clamp(P.GhostCount, 1, 8);
			Prm->GhostSpacing = P.GhostSpacing;
			Prm->GhostDispersal = P.GhostDispersal;
			Prm->GhostIntensity = P.GhostIntensity;
			Prm->HaloRadius = P.HaloRadius;
			Prm->HaloIntensity = P.HaloIntensity;
			Prm->PolyIrisSides = FMath::Clamp(P.PolyIrisSides, 5, 9);
			Prm->PolyIrisRotation = P.PolyIrisRotation;
			Prm->PolyIrisRadius = P.PolyIrisRadius;
			Prm->PolyIrisIntensity = P.PolyIrisIntensity;
			Prm->FanCount = FMath::Clamp(P.FanCount, 3, 12);
			Prm->FanSpread = P.FanSpread;
			Prm->FanRadius = P.FanRadius;
			Prm->FanIntensity = P.FanIntensity;
			Prm->AccumTexture = StarAccum;
			Prm->AccumSampler = BilinearClampSampler;
			Prm->AccumWeight = StarAccum ? FMath::Max(0.0f, P.StarFilterIntensity) : 0.0f;
			Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

			TShaderMapRef<FTrueGlowFlarePS> Shader(ShaderMap);
			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder, ShaderMap,
				RDG_EVENT_NAME("TrueGlow.Flare %dx%d", QuarterSize.X, QuarterSize.Y),
				Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
			FlareResult = Target;
		}
		else if (StarAccum)
		{
			FlareResult = StarAccum;   // 只有星芒镜时直接输出
		}
	}

	// ------------------------------------------------------------------
	// 4''') 光束 GodRays：屏幕空间径向体积光（源 = 亮部基底，½ 分辨率）
	FRDGTextureRef GodRaysResult = BlackDummy;
	if (P.bGodRays && P.GodRaysIntensity > 0.001f && StreakBase)
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowFlare);
		FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("TrueGlow.GodRays"));

		FTrueGlowGodRaysPS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowGodRaysPS::FParameters>();
		Prm->Input = GetExactViewportParams(QuarterSize);
		Prm->Output = GetExactViewportParams(QuarterSize);
		Prm->InputTexture = StreakBase;
		Prm->InputSampler = BilinearClampSampler;
		Prm->LightPos = P.GodRaysLightPos;
		Prm->RayLength = P.GodRaysLength;
		Prm->Decay = P.GodRaysDecay;
		Prm->Density = P.GodRaysDensity;
		Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FTrueGlowGodRaysPS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("TrueGlow.GodRays %dx%d", QuarterSize.X, QuarterSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
		GodRaysResult = Target;
	}

	// ------------------------------------------------------------------
	// 5) 全分辨率合成（输出纹理与视图等尺寸，ViewRect 重定为全幅）
	FRDGTextureRef OutTexture = CreateGlowTexture(GraphBuilder, FullSize, TEXT("TrueGlow.SceneColorOut"));
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, TrueGlowComposite);
		const FScreenPassTextureViewportParameters OutParams = GetExactViewportParams(FullSize);
		const FScreenPassTextureViewportTransform Transform =
			GetScreenPassTextureViewportTransform(SceneParams, OutParams);

		FTrueGlowCompositePS::FParameters* Prm = GraphBuilder.AllocParameters<FTrueGlowCompositePS::FParameters>();
		Prm->Scene = SceneParams;
		Prm->Output = OutParams;
		Prm->SceneUVScaleBias = FVector4(Transform.Scale.X, Transform.Scale.Y, Transform.Bias.X, Transform.Bias.Y);
		Prm->SceneTexture = SceneColor.Texture;
		Prm->SceneSampler = BilinearClampSampler;
		Prm->BloomTexture = BloomResult;
		Prm->BloomSampler = BilinearClampSampler;
		Prm->BloomMul = P.bBloomEnabled
			? ToWeight4(FMath::Max(0.0f, P.BloomIntensity))
			: FVector4(0, 0, 0, 0);
		Prm->StreakDualLineSepUV = (P.bStreakDualLine && P.bStreakEnabled)
			? FVector2D(0.0f, P.StreakDualLineSeparation / 1080.0f)
			: FVector2D(0, 0);
		Prm->StreakDualLineIntensity = (P.bStreakDualLine && P.bStreakEnabled)
			? FMath::Clamp(P.StreakDualLineIntensity, 0.0f, 1.0f)
			: 0.0f;
		Prm->BloomDispersion = P.ChromaticDispersion;
		Prm->FilmSoftIntensity = P.FilmSoftIntensity;
		Prm->FilmSoftRadius = P.FilmSoftRadius;
		Prm->DualTintStrength = P.DualTintStrength;
		Prm->WarmCoreColor = ToTint4(P.WarmCoreColor);
		Prm->CoolFringeColor = ToTint4(P.CoolFringeColor);
		Prm->LensDirtIntensity = P.bLensDirt ? P.LensDirtIntensity : 0.0f;
		Prm->StreakTexture = StreakResult;
		Prm->StreakSampler = BilinearClampSampler;
		Prm->StreakTint = ToTint4(P.StreakTint);
		Prm->StreakIntensity = P.bStreakEnabled ? FMath::Max(0.0f, P.StreakIntensity) : 0.0f;
		Prm->StreakTexture2 = StreakResultV;
		Prm->Streak2Sampler = BilinearClampSampler;
		Prm->Streak2Tint = ToTint4(P.StreakTint);
		Prm->Streak2Intensity = P.bStreakVerticalEnabled ? FMath::Max(0.0f, P.StreakVerticalIntensity) : 0.0f;
		Prm->GlareTexture = GlareResult;
		Prm->GlareSampler = BilinearClampSampler;
		Prm->GlareTint = ToTint4(P.GlareTint);
		Prm->GlareIntensity = P.bGlareEnabled ? FMath::Max(0.0f, P.GlareIntensity) : 0.0f;
		Prm->FlareTexture = FlareResult;
		Prm->FlareSampler = BilinearClampSampler;
		Prm->FlareTint = ToTint4(P.FlareTint);
		Prm->FlareIntensity = (P.GhostIntensity > 0.001f || P.HaloIntensity > 0.001f || P.PolyIrisIntensity > 0.001f || P.FanIntensity > 0.001f || P.bStarFilter) ? 1.0f : 0.0f;
		Prm->GodRaysTexture = GodRaysResult;
		Prm->GodRaysSampler = BilinearClampSampler;
		Prm->GodRaysTint = ToTint4(P.GodRaysTint);
		Prm->GodRaysIntensity = (P.bGodRays && StreakBase) ? FMath::Max(0.0f, P.GodRaysIntensity) : 0.0f;
		Prm->RenderTargets[0] = FRenderTargetBinding(OutTexture, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FTrueGlowCompositePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("TrueGlow.Composite %dx%d", FullSize.X, FullSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, FullSize));
	}

	return FScreenPassTexture(OutTexture, FIntRect(FIntPoint::ZeroValue, FullSize));
#endif // KG_SHADERS_ENABLED
}
