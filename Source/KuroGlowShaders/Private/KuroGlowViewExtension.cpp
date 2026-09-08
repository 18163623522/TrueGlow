// Copyright pengxiwei. All Rights Reserved.

#include "KuroGlowViewExtension.h"
#include "KuroGlowShaders.h"
#include "KuroGlowShaderBridge.h"

#include "PostProcess/PostProcessMaterial.h"

#include "GlobalShader.h"
#include "PixelShaderUtils.h"
#include "RenderGraph.h"
#include "SceneView.h"
#include "RHI.h"
#include "RHIStaticStates.h"
#include "Shader.h"

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

	FVector4 ToMul4(const FLinearColor& Tint, float Intensity)
	{
		return FVector4(Tint.R * Intensity, Tint.G * Intensity, Tint.B * Intensity, 0.0f);
	}

	FIntPoint Halve(FIntPoint InSize)
	{
		FIntPoint Out = FIntPoint::DivideAndRoundUp(InSize, FIntPoint(2, 2));
		Out.X = FMath::Max(1, Out.X);
		Out.Y = FMath::Max(1, Out.Y);
		return Out;
	}
}

FKuroGlowViewExtension::FKuroGlowViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	Params = FKuroGlowShaderBridge::GetParamsSnapshot();
}

TSharedRef<FKuroGlowViewExtension, ESPMode::ThreadSafe> FKuroGlowViewExtension::Create()
{
	return FSceneViewExtensions::NewExtension<FKuroGlowViewExtension>();
}

void FKuroGlowViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	FScopeLock Lock(&ParamsCS);
	Params = FKuroGlowShaderBridge::GetParamsSnapshot();
}

bool FKuroGlowViewExtension::IsActiveThisFrameInContext(FSceneViewExtensionContext& Context) const
{
	return KuroGlowCVars::IsEnabled() && FKuroGlowShaderBridge::GetParamsSnapshot().bEnabled;
}

void FKuroGlowViewExtension::SubscribeToPostProcessingPass(
	EPostProcessingPass Pass,
	FAfterPassCallbackDelegateArray& InOutPassCallbacks,
	bool bIsPassEnabled)
{
	// MotionBlur 槽位的 after-pass 在 4.26 无条件执行（FPostProcessing::Process 中位于
	// TAA 之后、曝光/Bloom/tonemap 之前的 HDR 线性段），是唯一稳定的 HDR 注入点。
	if (Pass == EPostProcessingPass::MotionBlur)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(
			this, &FKuroGlowViewExtension::AfterMotionBlur_RenderThread));
	}
}

FScreenPassTexture FKuroGlowViewExtension::AfterMotionBlur_RenderThread(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FPostProcessMaterialInputs& Inputs)
{
	FScreenPassTexture SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);

	if (!SceneColor.IsValid() || SceneColor.Texture == nullptr)
	{
		return SceneColor;
	}

	FKuroGlowParams P;
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
		UE_LOG(LogTemp, Log, TEXT("KuroGlow: first AfterMotionBlur pass executing (%dx%d)."), FullSize.X, FullSize.Y);
	}

	const FIntPoint HalfSize = Halve(FullSize);
	const FIntPoint QuarterSize = Halve(HalfSize);

	const FScreenPassTextureViewportParameters SceneParams =
		GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneColor));

	const bool bNeedBright = P.bBloomEnabled || P.bStreakEnabled || P.bGlareEnabled;

	RDG_EVENT_SCOPE(GraphBuilder, "KuroGlow");

	// 禁用通道的黑色占位（clear binding 为黑，无需真正渲染）
	FRDGTextureRef BlackDummy = CreateGlowTexture(GraphBuilder, FIntPoint(4, 4), TEXT("KuroGlow.Black"));

	// ------------------------------------------------------------------
	// 0) BrightPass：软阈值 + 4-tap 降到 1/2
	FRDGTextureRef BrightHalf = BlackDummy;
	if (bNeedBright)
	{
		const FScreenPassTextureViewportParameters HalfParams = GetExactViewportParams(HalfSize);
		const FScreenPassTextureViewportTransform Transform =
			GetScreenPassTextureViewportTransform(SceneParams, HalfParams);

		FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, HalfSize, TEXT("KuroGlow.BrightHalf"));

		FKuroGlowBrightDownsamplePS::FParameters* Prm =
			GraphBuilder.AllocParameters<FKuroGlowBrightDownsamplePS::FParameters>();
		Prm->Input = SceneParams;
		Prm->Output = HalfParams;
		Prm->InputUVScaleBias = FVector4(Transform.Scale.X, Transform.Scale.Y, Transform.Bias.X, Transform.Bias.Y);
		Prm->InputTexture = SceneColor.Texture;
		Prm->InputSampler = BilinearClampSampler;
		Prm->Threshold = P.BloomThreshold;
		Prm->Knee = P.BloomKnee;
		Prm->Tint = FVector4(P.BloomLevelTints[0].R, P.BloomLevelTints[0].G, P.BloomLevelTints[0].B, 1.0f);
		Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FKuroGlowBrightDownsamplePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("KuroGlow.BrightPass %dx%d", HalfSize.X, HalfSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, HalfSize));

		BrightHalf = Target;
	}

	// 通用降采样 lambda（作用于我们自己的等尺寸纹理，UV 变换恒等）
	auto AddDownsamplePass = [&](FRDGTextureRef Src, FIntPoint SrcExtent, FRDGTextureRef Dst, FIntPoint DstExtent, const FVector4& Tint, const TCHAR* Name)
	{
		FKuroGlowDownsamplePS::FParameters* Prm = GraphBuilder.AllocParameters<FKuroGlowDownsamplePS::FParameters>();
		Prm->Input = GetExactViewportParams(SrcExtent);
		Prm->Output = GetExactViewportParams(DstExtent);
		Prm->InputUVScaleBias = FVector4(1, 1, 0, 0);
		Prm->InputTexture = Src;
		Prm->InputSampler = BilinearClampSampler;
		Prm->Tint = Tint;
		Prm->RenderTargets[0] = FRenderTargetBinding(Dst, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FKuroGlowDownsamplePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("KuroGlow.%s %dx%d", Name, DstExtent.X, DstExtent.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, DstExtent));
	};

	// ------------------------------------------------------------------
	// 1) 1/4 亮部基底（streak / glare 共用）
	FRDGTextureRef QuarterBase = nullptr;
	if (P.bStreakEnabled || P.bGlareEnabled)
	{
		QuarterBase = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("KuroGlow.QuarterBase"));
		AddDownsamplePass(BrightHalf, HalfSize, QuarterBase, QuarterSize, FVector4(1, 1, 1, 1), TEXT("Quarter"));
	}

	// ------------------------------------------------------------------
	// 2) Bloom 金字塔：降采样链 + 每级高斯 + tent 升采样合并
	FRDGTextureRef BloomResult = BlackDummy;
	if (P.bBloomEnabled)
	{
		const int32 LevelCount = FMath::Clamp(P.BloomLevels, 1, 6);

		TArray<FRDGTextureRef, TInlineAllocator<6>> LevelTextures;
		TArray<FIntPoint, TInlineAllocator<6>> LevelExtents;
		LevelTextures.Add(BrightHalf);
		LevelExtents.Add(HalfSize);

		for (int32 i = 1; i < LevelCount; ++i)
		{
			const FIntPoint Extent = Halve(LevelExtents.Last());
			FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, Extent, *FString::Printf(TEXT("KuroGlow.L%d"), i));
			const FLinearColor& Tint = P.BloomLevelTints[i];
			AddDownsamplePass(LevelTextures.Last(), LevelExtents.Last(), Target, Extent,
				FVector4(Tint.R, Tint.G, Tint.B, 1.0f), *FString::Printf(TEXT("Down.L%d"), i));
			LevelTextures.Add(Target);
			LevelExtents.Add(Extent);
		}

		// 每级可分离高斯（H→V 两 pass）
		const int32 BlurRadius = FMath::Clamp(FMath::RoundToInt(P.BloomBlurRadius), 0, 6);
		if (BlurRadius > 0)
		{
			const float Sigma = FMath::Max(0.5f, BlurRadius * 0.5f);
			for (int32 i = 0; i < LevelCount; ++i)
			{
				FRDGTextureRef Current = LevelTextures[i];
				const FIntPoint Extent = LevelExtents[i];

				for (int32 Axis = 0; Axis < 2; ++Axis)
				{
					const FVector2D Direction = Axis == 0 ? FVector2D(1, 0) : FVector2D(0, 1);
					FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, Extent,
						*FString::Printf(TEXT("KuroGlow.L%d.Blur%c"), i, Axis == 0 ? TEXT('H') : TEXT('V')));

					FKuroGlowBlurPS::FParameters* Prm = GraphBuilder.AllocParameters<FKuroGlowBlurPS::FParameters>();
					Prm->Input = GetExactViewportParams(Extent);
					Prm->Output = GetExactViewportParams(Extent);
					Prm->InputTexture = Current;
					Prm->InputSampler = BilinearClampSampler;
					Prm->Direction = Direction;
					Prm->Radius = BlurRadius;
					Prm->Sigma = Sigma;
					Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

					TShaderMapRef<FKuroGlowBlurPS> Shader(ShaderMap);
					FPixelShaderUtils::AddFullscreenPass(
						GraphBuilder, ShaderMap,
						RDG_EVENT_NAME("KuroGlow.Blur.L%d.%c %dx%d", i, Axis == 0 ? TEXT('H') : TEXT('V'), Extent.X, Extent.Y),
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
				*FString::Printf(TEXT("KuroGlow.Merge.L%d"), i));

			FKuroGlowTentUpsampleAddPS::FParameters* Prm =
				GraphBuilder.AllocParameters<FKuroGlowTentUpsampleAddPS::FParameters>();
			Prm->Input = GetExactViewportParams(LevelExtents[i + 1]);
			Prm->Output = GetExactViewportParams(Extent);
			Prm->InputTexture = Current;           // 粗级
			Prm->InputSampler = BilinearClampSampler;
			Prm->HighTexture = LevelTextures[i];   // 细级（同输出分辨率）
			Prm->HighSampler = BilinearClampSampler;
			Prm->Weight = ToWeight4(P.BloomLevelWeights[i + 1]);
			Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

			TShaderMapRef<FKuroGlowTentUpsampleAddPS> Shader(ShaderMap);
			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder, ShaderMap,
				RDG_EVENT_NAME("KuroGlow.Merge.L%d %dx%d", i, Extent.X, Extent.Y),
				Shader, Prm, FIntRect(FIntPoint::ZeroValue, Extent));

			Current = Target;
		}
		BloomResult = Current;
	}

	// ------------------------------------------------------------------
	// 3) Streak：横向衰减模糊迭代 + 一次纵向加粗
	FRDGTextureRef StreakResult = BlackDummy;
	if (P.bStreakEnabled && QuarterBase)
	{
		const float HeightScale = static_cast<float>(QuarterSize.Y) / 1080.0f;
		const int32 StreakTaps = 12;
		const int32 Passes = FMath::Clamp(P.StreakPasses, 1, 8);
		const float StepPixels = FMath::Max(1.0f, P.StreakLength * HeightScale / (Passes * StreakTaps));

		auto AddStreakPass = [&](FRDGTextureRef Src, FRDGTextureRef Dst, const FVector2D& Direction, float StepPx, int32 Taps, float Attenuation, const TCHAR* Name)
		{
			FKuroGlowStreakPS::FParameters* Prm = GraphBuilder.AllocParameters<FKuroGlowStreakPS::FParameters>();
			Prm->Input = GetExactViewportParams(QuarterSize);
			Prm->Output = GetExactViewportParams(QuarterSize);
			Prm->InputTexture = Src;
			Prm->InputSampler = BilinearClampSampler;
			Prm->Direction = Direction;
			Prm->StepPixels = StepPx;
			Prm->Taps = Taps;
			Prm->Attenuation = Attenuation;
			Prm->RenderTargets[0] = FRenderTargetBinding(Dst, ERenderTargetLoadAction::ENoAction);

			TShaderMapRef<FKuroGlowStreakPS> Shader(ShaderMap);
			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder, ShaderMap,
				RDG_EVENT_NAME("KuroGlow.Streak.%s %dx%d", Name, QuarterSize.X, QuarterSize.Y),
				Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
		};

		FRDGTextureRef Current = QuarterBase;
		for (int32 i = 0; i < Passes; ++i)
		{
			FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize,
				*FString::Printf(TEXT("KuroGlow.Streak%d"), i));
			AddStreakPass(Current, Target, FVector2D(1, 0), StepPixels, StreakTaps, P.StreakAttenuation,
				*FString::Printf(TEXT("H%d"), i));
			Current = Target;
		}

		// 纵向加粗（thickness）
		FRDGTextureRef Thick = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("KuroGlow.StreakThick"));
		AddStreakPass(Current, Thick, FVector2D(0, 1),
			FMath::Max(1.0f, P.StreakThickness * HeightScale), 2, 0.5f, TEXT("V"));
		StreakResult = Thick;
	}

	// ------------------------------------------------------------------
	// 4) Glare：多方向双半径星芒
	FRDGTextureRef GlareResult = BlackDummy;
	if (P.bGlareEnabled && QuarterBase)
	{
		const float HeightScale = static_cast<float>(QuarterSize.Y) / 1080.0f;
		FRDGTextureRef Target = CreateGlowTexture(GraphBuilder, QuarterSize, TEXT("KuroGlow.Glare"));

		FKuroGlowGlarePS::FParameters* Prm = GraphBuilder.AllocParameters<FKuroGlowGlarePS::FParameters>();
		Prm->Input = GetExactViewportParams(QuarterSize);
		Prm->Output = GetExactViewportParams(QuarterSize);
		Prm->InputTexture = QuarterBase;
		Prm->InputSampler = BilinearClampSampler;
		Prm->Radius1Pixels = FMath::Max(2.0f, P.GlareRadius1 * HeightScale);
		Prm->Radius2Pixels = FMath::Max(4.0f, P.GlareRadius2 * HeightScale);
		Prm->TapsPerDirection = FMath::Clamp(P.GlareTaps, 2, 16);
		Prm->DirectionCount = (P.GlareDirections == 6) ? 6u : 4u;
		Prm->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FKuroGlowGlarePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("KuroGlow.Glare %dx%d", QuarterSize.X, QuarterSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, QuarterSize));
		GlareResult = Target;
	}

	// ------------------------------------------------------------------
	// 5) 全分辨率合成（输出纹理与视图等尺寸，ViewRect 重定为全幅）
	FRDGTextureRef OutTexture = CreateGlowTexture(GraphBuilder, FullSize, TEXT("KuroGlow.SceneColorOut"));
	{
		const FScreenPassTextureViewportParameters OutParams = GetExactViewportParams(FullSize);
		const FScreenPassTextureViewportTransform Transform =
			GetScreenPassTextureViewportTransform(SceneParams, OutParams);

		FKuroGlowCompositePS::FParameters* Prm = GraphBuilder.AllocParameters<FKuroGlowCompositePS::FParameters>();
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
		Prm->StreakTexture = StreakResult;
		Prm->StreakSampler = BilinearClampSampler;
		Prm->StreakMul = P.bStreakEnabled
			? ToMul4(P.StreakTint, FMath::Max(0.0f, P.StreakIntensity))
			: FVector4(0, 0, 0, 0);
		Prm->GlareTexture = GlareResult;
		Prm->GlareSampler = BilinearClampSampler;
		Prm->GlareMul = P.bGlareEnabled
			? ToMul4(P.GlareTint, FMath::Max(0.0f, P.GlareIntensity))
			: FVector4(0, 0, 0, 0);
		Prm->RenderTargets[0] = FRenderTargetBinding(OutTexture, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FKuroGlowCompositePS> Shader(ShaderMap);
		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder, ShaderMap,
			RDG_EVENT_NAME("KuroGlow.Composite %dx%d", FullSize.X, FullSize.Y),
			Shader, Prm, FIntRect(FIntPoint::ZeroValue, FullSize));
	}

	return FScreenPassTexture(OutTexture, FIntRect(FIntPoint::ZeroValue, FullSize));
#endif // KG_SHADERS_ENABLED
}
