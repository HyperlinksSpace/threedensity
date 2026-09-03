// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#include "ThreeDensityGameUserSettings.h"
#include "TP_ThirdPerson.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "RHI.h"
#include "RHIGlobals.h"

namespace ThreeDensityGraphics
{
	static constexpr int32 CurrentSettingsVersion = 3;
}

UThreeDensityGameUserSettings::UThreeDensityGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVSyncEnabled(false);
	SetFrameRateLimit(120.0f);
}

UThreeDensityGameUserSettings* UThreeDensityGameUserSettings::GetThreeDensitySettings()
{
	return Cast<UThreeDensityGameUserSettings>(GEngine ? GEngine->GetGameUserSettings() : nullptr);
}

EThreeDensityGraphicsPreset UThreeDensityGameUserSettings::RecommendPreset() const
{
	const FString Gpu = GRHIAdapterName.ToLower();
	const uint64 RamGB = FPlatformMemory::GetConstants().TotalPhysical / (1024ull * 1024ull * 1024ull);
	const int32 Cores = FPlatformMisc::NumberOfCores();

	const bool bLaptop = Gpu.Contains(TEXT("laptop")) || Gpu.Contains(TEXT("mobile"));
	const bool bIntegrated =
		(Gpu.Contains(TEXT("intel")) && !Gpu.Contains(TEXT("arc"))) ||
		Gpu.Contains(TEXT("uhd")) ||
		Gpu.Contains(TEXT("iris")) ||
		Gpu.Contains(TEXT("radeon graphics")) ||
		Gpu.Contains(TEXT("vega"));

	const bool bEntry =
		Gpu.Contains(TEXT("1650")) || Gpu.Contains(TEXT("1660")) || Gpu.Contains(TEXT("1050")) ||
		Gpu.Contains(TEXT("1060")) || Gpu.Contains(TEXT("2050")) || Gpu.Contains(TEXT("3050")) ||
		Gpu.Contains(TEXT("4050")) || Gpu.Contains(TEXT("mx ")) || Gpu.Contains(TEXT("mx1")) ||
		Gpu.Contains(TEXT("gtx 16"));

	const bool bMid =
		Gpu.Contains(TEXT("2060")) || Gpu.Contains(TEXT("2070")) || Gpu.Contains(TEXT("3060")) ||
		Gpu.Contains(TEXT("3070")) || Gpu.Contains(TEXT("4060")) || Gpu.Contains(TEXT("6600")) ||
		Gpu.Contains(TEXT("6700")) || Gpu.Contains(TEXT("7600"));

	const bool bHigh =
		Gpu.Contains(TEXT("3080")) || Gpu.Contains(TEXT("3090")) || Gpu.Contains(TEXT("4070")) ||
		Gpu.Contains(TEXT("4080")) || Gpu.Contains(TEXT("4090")) || Gpu.Contains(TEXT("5070")) ||
		Gpu.Contains(TEXT("5080")) || Gpu.Contains(TEXT("5090")) || Gpu.Contains(TEXT("6800")) ||
		Gpu.Contains(TEXT("6900")) || Gpu.Contains(TEXT("7900")) || Gpu.Contains(TEXT("9070"));

	if (bIntegrated || RamGB < 6 || Cores < 4)
	{
		return EThreeDensityGraphicsPreset::Low;
	}

	// Entry laptop GPUs (e.g. RTX 3050 Ti): Medium = sharp look without Lumen cost.
	if (bLaptop && bEntry)
	{
		return EThreeDensityGraphicsPreset::Medium;
	}
	if (bEntry)
	{
		return RamGB >= 12 ? EThreeDensityGraphicsPreset::High : EThreeDensityGraphicsPreset::Medium;
	}
	if (bLaptop && bMid)
	{
		return EThreeDensityGraphicsPreset::High;
	}
	if (bHigh && !bLaptop && RamGB >= 16)
	{
		return EThreeDensityGraphicsPreset::Epic;
	}
	if (bMid || bHigh)
	{
		return EThreeDensityGraphicsPreset::High;
	}
	if (bLaptop || RamGB < 16)
	{
		return EThreeDensityGraphicsPreset::Medium;
	}
	return EThreeDensityGraphicsPreset::High;
}

FString UThreeDensityGameUserSettings::GetDetectedHardwareSummary() const
{
	const uint64 RamGB = FPlatformMemory::GetConstants().TotalPhysical / (1024ull * 1024ull * 1024ull);
	const TCHAR* PresetNames[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
	const int32 Idx = FMath::Clamp(static_cast<int32>(RecommendPreset()), 0, 3);
	return FString::Printf(TEXT("%s  ·  %llu GB RAM  ·  recommended %s"),
		GRHIAdapterName.IsEmpty() ? TEXT("Unknown GPU") : *GRHIAdapterName,
		RamGB,
		PresetNames[Idx]);
}

FString UThreeDensityGameUserSettings::GetBenchmarkSummary() const
{
	if (LastBenchmarkFPS <= 0.0f)
	{
		return TEXT("No FPS benchmark yet — runs automatically a few seconds after launch, or press BENCHMARK.");
	}

	const TCHAR* PresetNames[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
	const int32 Idx = FMath::Clamp(static_cast<int32>(GraphicsPreset), 0, 3);
	return FString::Printf(TEXT("Last benchmark: %.0f FPS on %s%s"),
		LastBenchmarkFPS,
		PresetNames[Idx],
		bLastApplyWasAuto ? TEXT(" (auto)") : TEXT(""));
}

void UThreeDensityGameUserSettings::ApplyOnStartup()
{
	LoadSettings(true);

	const bool bNeedsProfile =
		!bHasAppliedHardwareProfile ||
		GraphicsSettingsVersion < ThreeDensityGraphics::CurrentSettingsVersion;

	if (bNeedsProfile)
	{
		ApplyGraphicsPreset(RecommendPreset(), true);
		GraphicsSettingsVersion = ThreeDensityGraphics::CurrentSettingsVersion;
		SaveSettings();
		return;
	}

	ApplyGraphicsPreset(GraphicsPreset, bLastApplyWasAuto);
}

void UThreeDensityGameUserSettings::ApplyGraphicsPreset(EThreeDensityGraphicsPreset Preset, bool bFromAutoDetect)
{
	GraphicsPreset = Preset;
	bLastApplyWasAuto = bFromAutoDetect;
	bHasAppliedHardwareProfile = true;
	GraphicsSettingsVersion = ThreeDensityGraphics::CurrentSettingsVersion;

	switch (Preset)
	{
	case EThreeDensityGraphicsPreset::Low:
		// Smooth first — lighter meshes/effects, no Lumen.
		SetOverallScalabilityLevel(0);
		SetResolutionScaleNormalized(0.72f);
		bLumenEnabled = false;
		MaxFPS = 120;
		SetVSyncEnabled(false);
		break;
	case EThreeDensityGraphicsPreset::Medium:
		// Target for mid/entry GPUs: High-looking materials/shadows, no Lumen, uncapped-ish 120.
		SetOverallScalabilityLevel(2);
		SetResolutionScaleNormalized(0.88f);
		bLumenEnabled = false;
		MaxFPS = 120;
		SetVSyncEnabled(false);
		break;
	case EThreeDensityGraphicsPreset::High:
		SetOverallScalabilityLevel(2);
		SetResolutionScaleNormalized(0.92f);
		bLumenEnabled = true;
		MaxFPS = 0;
		SetVSyncEnabled(false);
		break;
	case EThreeDensityGraphicsPreset::Epic:
		SetOverallScalabilityLevel(3);
		SetResolutionScaleNormalized(1.0f);
		bLumenEnabled = true;
		MaxFPS = 0;
		SetVSyncEnabled(false);
		break;
	}

	SetFrameRateLimit(static_cast<float>(MaxFPS));
	ApplySettings(false);
	SaveSettings();

	UE_LOG(LogTP_ThirdPerson, Log, TEXT("Graphics preset %d (auto=%s) Lumen=%s FPS cap=%d"),
		static_cast<int32>(Preset), bFromAutoDetect ? TEXT("yes") : TEXT("no"),
		bLumenEnabled ? TEXT("on") : TEXT("off"), MaxFPS);
}

void UThreeDensityGameUserSettings::SetLumenEnabled(bool bEnabled)
{
	bLumenEnabled = bEnabled;
	bLastApplyWasAuto = false;
	ApplyRendererProfile();
	SaveSettings();
}

void UThreeDensityGameUserSettings::SetFrameCap(int32 InMaxFPS)
{
	MaxFPS = FMath::Max(0, InMaxFPS);
	SetFrameRateLimit(static_cast<float>(MaxFPS));
	bLastApplyWasAuto = false;
	ApplyNonResolutionSettings();
	SaveSettings();
}

void UThreeDensityGameUserSettings::ApplyBenchmarkResult(float AverageFPS, bool bAutoAdjust)
{
	LastBenchmarkFPS = AverageFPS;
	bLastApplyWasAuto = bAutoAdjust || bLastApplyWasAuto;

	UE_LOG(LogTP_ThirdPerson, Log, TEXT("FPS benchmark result: %.1f (autoAdjust=%s, preset=%d)"),
		AverageFPS, bAutoAdjust ? TEXT("yes") : TEXT("no"), static_cast<int32>(GraphicsPreset));

	if (bAutoAdjust)
	{
		const int32 Current = static_cast<int32>(GraphicsPreset);
		if (AverageFPS < 40.0f && Current > 0)
		{
			ApplyGraphicsPreset(static_cast<EThreeDensityGraphicsPreset>(Current - 1), true);
		}
		else if (AverageFPS < 50.0f && Current >= 2 && bLumenEnabled)
		{
			// Keep High scalability but drop Lumen for smoother frames.
			bLumenEnabled = false;
			MaxFPS = 120;
			SetFrameRateLimit(120.0f);
			ApplyRendererProfile();
			SaveSettings();
		}
		else if (AverageFPS >= 85.0f && Current < 2 && !bLumenEnabled)
		{
			// Headroom: bump visuals one step without jumping straight to Epic Lumen.
			ApplyGraphicsPreset(static_cast<EThreeDensityGraphicsPreset>(Current + 1), true);
		}
	}

	SaveSettings();
}

void UThreeDensityGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();
	ApplyRendererProfile();
}

void UThreeDensityGameUserSettings::ApplyRendererProfile()
{
	auto SetInt = [](const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByGameSetting);
		}
	};
	auto SetFloat = [](const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByGameSetting);
		}
	};

	const int32 Preset = static_cast<int32>(GraphicsPreset);

	SetInt(TEXT("r.MotionBlurQuality"), 0);
	SetInt(TEXT("r.DefaultFeature.MotionBlur"), 0);
	SetInt(TEXT("r.DynamicGlobalIlluminationMethod"), bLumenEnabled ? 1 : 0);
	SetInt(TEXT("r.ReflectionMethod"), bLumenEnabled ? 1 : 0);
	SetInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), bLumenEnabled ? 1 : 0);
	SetInt(TEXT("r.Lumen.Reflections.Allow"), bLumenEnabled && Preset >= 3 ? 1 : 0);
	SetInt(TEXT("r.Lumen.TraceMeshSDFs"), 0);
	SetInt(TEXT("r.Shadow.Virtual.Enable"), (bLumenEnabled && Preset >= 2) ? 1 : 0);
	SetInt(TEXT("r.Shadow.CSM.MaxCascades"), Preset <= 1 ? 2 : 4);
	SetInt(TEXT("r.Streaming.PoolSize"), Preset == 0 ? 320 : Preset == 1 ? 520 : Preset == 2 ? 700 : 1000);
	SetInt(TEXT("r.AntiAliasingMethod"), Preset == 0 ? 1 : 2); // FXAA on Low, TAA otherwise
	SetInt(TEXT("r.Nanite.MaxPixelsPerEdge"), Preset == 0 ? 2 : 1);
	SetInt(TEXT("r.BloomQuality"), Preset == 0 ? 1 : 3);
	SetInt(TEXT("r.DepthOfFieldQuality"), 0);
	SetInt(TEXT("r.AmbientOcclusionLevels"), Preset == 0 ? 0 : 1);
	SetFloat(TEXT("r.ScreenPercentage"), GetResolutionScaleNormalized() * 100.0f);
	SetInt(TEXT("t.MaxFPS"), MaxFPS);
	SetInt(TEXT("r.VSync"), IsVSyncEnabled() ? 1 : 0);

	if (bLumenEnabled && Preset == 2)
	{
		// Soften Lumen on High so mid GPUs stay playable.
		SetInt(TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"), 2);
		SetInt(TEXT("r.Lumen.ScreenProbeGather.RadianceCache.NumProbesToTraceBudget"), 100);
	}
}
