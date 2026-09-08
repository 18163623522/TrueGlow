// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KuroGlowTypes.h"

/**
 * KuroGlowShaders（PostConfigInit、无 UCLASS）与 KuroGlow（Default、含 UKuroGlowSettings）
 * 之间的参数桥：KuroGlow 模块启动时注册 provider，渲染线程扩展每帧取快照。
 * 设置 provider 之前使用 FKuroGlowParams 默认值。
 */
class KUROGLOWSHADERS_API FKuroGlowShaderBridge
{
public:
	typedef TFunction<FKuroGlowParams()> FParamsProvider;

	/** 游戏线程调用（KuroGlow 模块 StartupModule）；传空函数重置为默认值。 */
	static void SetParamsProvider(FParamsProvider InProvider);

	/** 扩展在游戏线程 BeginRenderViewFamily 里调用。 */
	static FKuroGlowParams GetParamsSnapshot();
};
