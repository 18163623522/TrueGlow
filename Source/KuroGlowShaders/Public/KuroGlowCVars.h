// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** 控制台变量入口（定义在 KuroGlowShaders 模块）。返回 <0 表示"不覆盖 Settings"。 */
namespace KuroGlowCVars
{
	KUROGLOWSHADERS_API bool IsEnabled();
	KUROGLOWSHADERS_API float BloomIntensityOverride();
	KUROGLOWSHADERS_API float BloomThresholdOverride();
	KUROGLOWSHADERS_API int32 BloomLevelsOverride();
	KUROGLOWSHADERS_API float StreakIntensityOverride();
	KUROGLOWSHADERS_API float StreakLengthOverride();
	KUROGLOWSHADERS_API float GlareIntensityOverride();
}
