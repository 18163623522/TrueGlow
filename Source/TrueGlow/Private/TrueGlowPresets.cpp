// Copyright pengxiwei. All Rights Reserved.

#include "TrueGlowSettings.h"

namespace TrueGlowPresets
{

static void DualTintDefaults(UTrueGlowSettings& S)
{
	S.DualTintStrength = 0.0f;
	S.WarmCoreColor = FLinearColor(1.00f, 0.95f, 0.85f, 1.0f);
	S.CoolFringeColor = FLinearColor(0.80f, 0.90f, 1.15f, 1.0f);
}

void Apply(ETrueGlowPreset Preset, UTrueGlowSettings& S)
{
	// 公共默认（鸣潮基线）
	S.bEnabled = true;
	S.bBloomEnabled = true;
	S.BloomThreshold = 1.0f;
	S.BloomKnee = 0.6f;
	S.BloomBrightMultiplier = 1.0f;
	S.BloomLevels = 6;
	S.BloomBlurRadius = 3.0f;
	S.GaussianIterations = 2;
	S.bBloomFastMode = false;
	S.BloomKernelShape = ETrueGlowKernelShape::Gaussian;
	S.BloomShapeMix = 0.0f;
	S.BloomShapeRadius = 8.0f;
	S.ChromaticDispersion = 0.18f;
	S.bLensDirt = false;
	S.LensDirtIntensity = 1.0f;
	S.FilmSoftIntensity = 0.0f;
	S.FilmSoftRadius = 3.0f;
	DualTintDefaults(S);
	S.bStreakDualLine = false;
	S.StreakDualLineSeparation = 12.0f;
	S.StreakDualLineIntensity = 0.5f;
	S.BloomScale = FVector2D(1.0f, 1.0f);

	S.BloomLevelWeights = { 0.60f, 0.70f, 0.80f, 0.95f, 1.10f, 1.25f, 1.40f, 1.55f };
	S.BloomLevelTints = {
		FLinearColor(0.95f, 0.98f, 1.00f, 1.0f),
		FLinearColor(0.90f, 0.95f, 1.05f, 1.0f),
		FLinearColor(0.85f, 0.92f, 1.10f, 1.0f),
		FLinearColor(0.80f, 0.90f, 1.15f, 1.0f),
		FLinearColor(0.75f, 0.87f, 1.20f, 1.0f),
		FLinearColor(0.70f, 0.85f, 1.25f, 1.0f),
		FLinearColor(0.65f, 0.83f, 1.30f, 1.0f),
		FLinearColor(0.60f, 0.80f, 1.35f, 1.0f),
	};

	S.bStreakEnabled = true;
	S.StreakTint = FLinearColor(0.55f, 0.75f, 1.00f, 1.0f);
	S.StreakThickness = 2.0f;
	S.StreakAttenuation = 0.35f;
	S.StreakPasses = 4;
	S.bStreakOwnThreshold = false;
	S.StreakThreshold = 2.0f;
	S.bStreakVerticalEnabled = false;
	S.StreakVerticalIntensity = 0.25f;
	S.StreakVerticalLength = 240.0f;

	S.bGodRays = false;
	S.GodRaysIntensity = 1.0f;
	S.GodRaysLightPos = FVector2D(0.5f, 0.35f);
	S.GodRaysLength = 0.9f;
	S.GodRaysDecay = 0.965f;
	S.GodRaysDensity = 1.0f;
	S.GodRaysTint = FLinearColor(1, 1, 1, 1);

	S.bSpeedLines = false;
	S.SpeedLinesIntensity = 0.6f;
	S.SpeedLinesAngle = -20.0f;
	S.SpeedLinesCount = 5;
	S.SpeedLinesSpacing = 14.0f;
	S.SpeedLinesLength = 520.0f;
	S.SpeedLinesThickness = 1.5f;
	S.SpeedLinesTint = FLinearColor(1, 1, 1, 1);

	S.GhostIntensity = 0.0f;
	S.GhostCount = 4;
	S.GhostSpacing = 0.45f;
	S.GhostDispersal = 1.0f;
	S.HaloIntensity = 0.0f;
	S.HaloRadius = 0.12f;
	S.FlareTint = FLinearColor(1, 1, 1, 1);
	S.bStarFilter = false;
	S.StarFilterIntensity = 0.8f;
	S.StarFilterRays = 4;
	S.StarFilterRotation = 15.0f;
	S.StarFilterLength = 720.0f;
	S.StarFilterTint = FLinearColor(1, 1, 1, 1);
	S.PolyIrisIntensity = 0.0f;
	S.PolyIrisSides = 6;
	S.PolyIrisRotation = 0.0f;
	S.PolyIrisRadius = 0.10f;
	S.FanIntensity = 0.0f;
	S.FanCount = 7;
	S.FanSpread = 40.0f;
	S.FanRadius = 0.18f;

	S.bGlareEnabled = true;
	S.GlareTint = FLinearColor(1, 1, 1, 1);
	S.GlareDirections = 4;
	S.GlareRadius1 = 24.0f;
	S.GlareRadius2 = 72.0f;
	S.GlareTaps = 6;

	switch (Preset)
	{
	case ETrueGlowPreset::WuWa:
		S.BloomIntensity = 1.00f;
		S.ChromaticDispersion = 0.18f;
		S.FilmSoftIntensity = 0.35f;
		S.DualTintStrength = 0.35f;
		S.StreakIntensity = 0.35f;
		S.StreakLength = 480.0f;
		S.GlareIntensity = 0.25f;
		break;

	case ETrueGlowPreset::Neon:
		S.BloomIntensity = 1.40f;
		S.BloomBrightMultiplier = 1.3f;
		S.StreakIntensity = 0.70f;
		S.StreakLength = 720.0f;
		S.StreakTint = FLinearColor(0.35f, 0.60f, 1.00f, 1.0f);
		S.StreakAttenuation = 0.25f;
		S.StreakPasses = 6;
		S.bStreakVerticalEnabled = true;   // 灯管上下漏光
		S.StreakVerticalIntensity = 0.40f;
		S.StreakVerticalLength = 300.0f;
		S.ChromaticDispersion = 0.30f;
		S.GhostIntensity = 0.35f;         // 幻影+光环
		S.HaloIntensity = 0.20f;
		S.bStarFilter = true;            // 星芒镜
		S.StarFilterIntensity = 0.6f;
		S.PolyIrisIntensity = 0.4f;      // 六边形光圈
		S.FanIntensity = 0.3f;           // 光谱扇
		S.bLensDirt = true;               // 脏镜头
		S.LensDirtIntensity = 0.8f;
		S.FilmSoftIntensity = 0.50f;
		S.DualTintStrength = 0.45f;
		S.bStreakDualLine = true;         // 双线横光
		S.StreakDualLineIntensity = 0.45f;
		S.StreakDualLineSeparation = 16.0f;
		S.GlareIntensity = 0.35f;
		S.GlareTint = FLinearColor(0.85f, 0.95f, 1.00f, 1.0f);
		break;

	case ETrueGlowPreset::Subtle:
		S.BloomIntensity = 0.60f;
		S.ChromaticDispersion = 0.08f;
		S.FilmSoftIntensity = 0.20f;
		S.DualTintStrength = 0.15f;
		S.BloomBlurRadius = 1.0f;
		S.StreakIntensity = 0.12f;
		S.StreakLength = 240.0f;
		S.GlareIntensity = 0.10f;
		break;

	case ETrueGlowPreset::EnergyBlue:
		// 参考图①：∞ 能量环——大范围蓝晕 + 横向冰蓝光条 + 交叉点亮芒 + 彩虹边 + 小幻影
		S.BloomIntensity = 1.5f;
		S.BloomBrightMultiplier = 1.1f;
		S.ChromaticDispersion = 0.25f;
		S.FilmSoftIntensity = 0.40f;
		S.DualTintStrength = 0.30f;
		S.StreakIntensity = 0.50f;
		S.StreakLength = 600.0f;
		S.StreakTint = FLinearColor(0.55f, 0.75f, 1.00f, 1.0f);
		S.bStarFilter = true;
		S.StarFilterIntensity = 0.4f;
		S.StarFilterRays = 6;
		S.GhostIntensity = 0.30f;
		S.HaloIntensity = 0.10f;
		S.GlareIntensity = 0.35f;
		break;

	case ETrueGlowPreset::BallisticRed:
		// 参考图②：红色弹道——过曝白核 + 红雾散射 + 横向红光条 + 斜向平行速度线
		S.BloomIntensity = 2.0f;
		S.BloomThreshold = 1.2f;
		S.BloomBrightMultiplier = 1.5f;
		S.ChromaticDispersion = 0.10f;
		S.FilmSoftIntensity = 0.60f;
		S.DualTintStrength = 0.25f;
		S.StreakIntensity = 0.60f;
		S.StreakLength = 560.0f;
		S.StreakTint = FLinearColor(1.00f, 0.30f, 0.20f, 1.0f);
		S.GhostIntensity = 0.20f;
		S.GlareIntensity = 0.20f;
		S.bSpeedLines = true;
		S.SpeedLinesIntensity = 0.7f;
		S.SpeedLinesAngle = -20.0f;
		S.SpeedLinesCount = 5;
		S.SpeedLinesSpacing = 14.0f;
		S.SpeedLinesLength = 520.0f;
		S.SpeedLinesTint = FLinearColor(1.0f, 0.25f, 0.15f, 1.0f);
		break;

	case ETrueGlowPreset::RedSun:
		// 参考图③：红日穿云——8 级超宽红雾 + 低阈值 + 横向长拉丝 + 云隙光束
		S.BloomIntensity = 2.5f;
		S.BloomThreshold = 0.6f;
		S.BloomBrightMultiplier = 1.2f;
		S.BloomLevels = 8;
		S.ChromaticDispersion = 0.15f;
		S.FilmSoftIntensity = 0.80f;
		S.DualTintStrength = 0.45f;
		S.StreakIntensity = 0.55f;
		S.StreakLength = 800.0f;
		S.StreakTint = FLinearColor(1.00f, 0.35f, 0.25f, 1.0f);
		S.GhostIntensity = 0.15f;
		S.bGodRays = true;
		S.GodRaysIntensity = 0.8f;
		S.GodRaysTint = FLinearColor(1.0f, 0.55f, 0.35f, 1.0f);
		S.GlareIntensity = 0.15f;
		break;

	case ETrueGlowPreset::TechBlue:
		// 参考图④：蓝色能量弹——白炽光核 + 蓝色体积雾 + 弹头光束 + 横向蓝光条
		S.BloomIntensity = 1.8f;
		S.BloomBrightMultiplier = 1.2f;
		S.ChromaticDispersion = 0.20f;
		S.FilmSoftIntensity = 0.50f;
		S.DualTintStrength = 0.30f;
		S.StreakIntensity = 0.60f;
		S.StreakLength = 520.0f;
		S.StreakTint = FLinearColor(0.45f, 0.70f, 1.00f, 1.0f);
		S.bStarFilter = true;
		S.StarFilterIntensity = 0.25f;
		S.StarFilterRays = 4;
		S.GhostIntensity = 0.25f;
		S.HaloIntensity = 0.15f;
		S.bGodRays = true;
		S.GodRaysIntensity = 0.7f;
		S.GodRaysTint = FLinearColor(0.50f, 0.75f, 1.00f, 1.0f);
		S.GlareIntensity = 0.30f;
		break;

	case ETrueGlowPreset::Custom:
	default:
		break;
	}
}

bool ApplyByName(const FString& PresetName, UTrueGlowSettings& OutSettings)
{
	const FString Name = PresetName.ToLower();

	ETrueGlowPreset Preset = ETrueGlowPreset::Custom;
	if (Name == TEXT("wuwa") || Name == TEXT("鸣潮"))
	{
		Preset = ETrueGlowPreset::WuWa;
	}
	else if (Name == TEXT("neon"))
	{
		Preset = ETrueGlowPreset::Neon;
	}
	else if (Name == TEXT("subtle"))
	{
		Preset = ETrueGlowPreset::Subtle;
	}
	else if (Name == TEXT("energy") || Name == TEXT("energyblue"))
	{
		Preset = ETrueGlowPreset::EnergyBlue;
	}
	else if (Name == TEXT("ballistic") || Name == TEXT("ballisticred"))
	{
		Preset = ETrueGlowPreset::BallisticRed;
	}
	else if (Name == TEXT("redsun"))
	{
		Preset = ETrueGlowPreset::RedSun;
	}
	else if (Name == TEXT("tech") || Name == TEXT("techblue"))
	{
		Preset = ETrueGlowPreset::TechBlue;
	}
	else
	{
		return false;
	}

	Apply(Preset, OutSettings);
	OutSettings.Preset = Preset;
	return true;
}

} // namespace TrueGlowPresets
