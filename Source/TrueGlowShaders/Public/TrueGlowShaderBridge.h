// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrueGlowTypes.h"

/**
 * TrueGlowShaders（PostConfigInit、无 UCLASS）与 TrueGlow（Default、含 UTrueGlowSettings）
 * 之间的参数桥：TrueGlow 模块启动时注册 provider，渲染线程扩展每帧取快照。
 * 设置 provider 之前使用 FTrueGlowParams 默认值。
 */
class TRUEGLOWSHADERS_API FTrueGlowShaderBridge
{
public:
	typedef TFunction<FTrueGlowParams()> FParamsProvider;

	/** 游戏线程调用（TrueGlow 模块 StartupModule）；传空函数重置为默认值。 */
	static void SetParamsProvider(FParamsProvider InProvider);

	/** 扩展在游戏线程 BeginRenderViewFamily 里调用。 */
	static FTrueGlowParams GetParamsSnapshot();
};
