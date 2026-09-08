// Copyright pengxiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** 控制台变量入口（定义在 TrueGlowShaders 模块）。返回 <0 表示"不覆盖 Settings"。 */
namespace TrueGlowCVars
{
	TRUEGLOWSHADERS_API bool IsEnabled();
	TRUEGLOWSHADERS_API float BloomIntensityOverride();
	TRUEGLOWSHADERS_API float BloomThresholdOverride();
	TRUEGLOWSHADERS_API int32 BloomLevelsOverride();
	TRUEGLOWSHADERS_API float StreakIntensityOverride();
	TRUEGLOWSHADERS_API float StreakLengthOverride();
	TRUEGLOWSHADERS_API float GlareIntensityOverride();
}
