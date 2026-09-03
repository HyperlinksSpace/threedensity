// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#include "ThreeDensityGameUserSettings.h"
#include "TP_ThirdPerson.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "RHI.h"
#include "RHIGlobals.h"

UThreeDensityGameUserSettings::UThreeDensityGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVSyncEnabled(true);
	SetFrameRateLimit(60.0f);
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

	if (bIntegrated || RamGB < 8 || Cores < 4)
	{
		return EThreeDensityGraphicsPreset::Low;
	}
	if (bLaptop && bEntry)
	{
		return EThreeDensityGraphicsPreset::Low;
	}
	if (bEntry)
	{
		return EThreeDensityGraphicsPreset::Medium;
	}
	if (bLaptop && bMid)
	{
		return EThreeDensityGraphicsPreset::Medium;
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

void UThreeDensityGameUserSettings::ApplyOnStartup()
{
	LoadSettings(true);

	if (!bHasAppliedHardwareProfile)
	{
		ApplyGraphicsPreset(RecommendPreset(), true);
		return;
	}

	ApplyGraphicsPreset(GraphicsPreset, bLastApplyWasAuto);
}

void UThreeDensityGameUserSettings::ApplyGraphicsPreset(EThreeDensityGraphicsPreset Preset, bool bFromAutoDetect)
{
	GraphicsPreset = Preset;
	bLastApplyWasAuto = bFromAutoDetect;
	bHasAppliedHardwareProfile = true;

	const int32 Level = static_cast<int32>(Preset);
	SetOverallScalabilityLevel(Level);

	switch (Preset)
	{
	case EThreeDensityGraphicsPreset::Low:
		SetResolutionScaleNormalized(0.62f);
		bLumenEnabled = false;
		MaxFPS = 60;
		SetVSyncEnabled(true);
		break;
	case EThreeDensityGraphicsPreset::Medium:
		SetResolutionScaleNormalized(0.78f);
		bLumenEnabled = false;
		MaxFPS = 60;
		SetVSyncEnabled(true);
		break;
	case EThreeDensityGraphicsPreset::High:
		SetResolutionScaleNormalized(0.90f);
		bLumenEnabled = true;
		MaxFPS = 0;
		SetVSyncEnabled(false);
		break;
	case EThreeDensityGraphicsPreset::Epic:
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
		Level, bFromAutoDetect ? TEXT("yes") : TEXT("no"),
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

	const int32 Preset = static_cast<int32>(GraphicsPreset);
	SetInt(TEXT("r.MotionBlurQuality"), 0);
	SetInt(TEXT("r.DefaultFeature.MotionBlur"), 0);
	SetInt(TEXT("r.DynamicGlobalIlluminationMethod"), bLumenEnabled ? 1 : 0);
	SetInt(TEXT("r.ReflectionMethod"), bLumenEnabled ? 1 : 0);
	SetInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), bLumenEnabled ? 1 : 0);
	SetInt(TEXT("r.Lumen.Reflections.Allow"), bLumenEnabled && Preset >= 2 ? 1 : 0);
	SetInt(TEXT("r.Shadow.Virtual.Enable"), Preset >= 2 ? 1 : 0);
	SetInt(TEXT("r.Streaming.PoolSize"), Preset == 0 ? 280 : Preset == 1 ? 480 : Preset == 2 ? 700 : 1000);
	SetInt(TEXT("r.AntiAliasingMethod"), Preset <= 1 ? 1 : 2);
	SetInt(TEXT("r.Nanite.MaxPixelsPerEdge"), Preset == 0 ? 4 : 1);
	SetInt(TEXT("t.MaxFPS"), MaxFPS);
	SetInt(TEXT("r.VSync"), IsVSyncEnabled() ? 1 : 0);
}
