// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"
#include "KuroGlowTypes.h"

/**
 * KuroGlow 视图扩展：
 * 订阅 EPostProcessingPass::MotionBlur 的 after-pass 回调（该调用点在 TAA 之后、
 * 曝光/bloom/tonemap 之前的 HDR 线性段，且无条件执行），在回调里构建整条辉光 RDG 管线。
 * 本类所在模块必须以 PostConfigInit 相位加载（IMPLEMENT_GLOBAL_SHADER 的硬性要求）。
 */
class FKuroGlowViewExtension : public FSceneViewExtensionBase
{
public:
	FKuroGlowViewExtension(const FAutoRegister& AutoRegister);

	static TSharedRef<FKuroGlowViewExtension, ESPMode::ThreadSafe> Create();

	// ISceneViewExtension 纯虚接口
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}

	virtual bool IsActiveThisFrameInContext(FSceneViewExtensionContext& Context) const override;

	/**
	 * 引擎在 FPostProcessing::Process 开头对每个扩展调用；我们只认领 MotionBlur 槽位。
	 * 回调本体在渲染线程执行。
	 */
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	FScreenPassTexture AfterMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

	/** 游戏线程在 BeginRenderViewFamily 里刷新，渲染线程回调里加锁拷贝。 */
	FCriticalSection ParamsCS;
	FKuroGlowParams Params;
};
