// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "ThreeDensityGameUserSettings.generated.h"

UENUM()
enum class EThreeDensityGraphicsPreset : uint8
{
	Low = 0,
	Medium = 1,
	High = 2,
	Epic = 3
};

/**
 * Saves graphics choices, hardware defaults, and runtime FPS benchmarks.
 */
UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class UThreeDensityGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:

	UThreeDensityGameUserSettings(const FObjectInitializer& ObjectInitializer);

	static UThreeDensityGameUserSettings* GetThreeDensitySettings();

	/** Detect GPU/RAM and pick Low–Epic. */
	EThreeDensityGraphicsPreset RecommendPreset() const;

	FString GetDetectedHardwareSummary() const;

	FString GetBenchmarkSummary() const;

	/** First launch / settings version bump: auto-pick. Later: restore. */
	void ApplyOnStartup();

	void ApplyGraphicsPreset(EThreeDensityGraphicsPreset Preset, bool bFromAutoDetect);

	void SetLumenEnabled(bool bEnabled);
	bool IsLumenEnabled() const { return bLumenEnabled; }

	void SetFrameCap(int32 InMaxFPS);
	int32 GetFrameCap() const { return MaxFPS; }

	EThreeDensityGraphicsPreset GetGraphicsPreset() const { return GraphicsPreset; }

	bool WasAutoDetected() const { return bLastApplyWasAuto; }

	/** Record measured average FPS and optionally drop quality if too slow. */
	void ApplyBenchmarkResult(float AverageFPS, bool bAutoAdjust);

	float GetLastBenchmarkFPS() const { return LastBenchmarkFPS; }

	virtual void ApplyNonResolutionSettings() override;

protected:

	void ApplyRendererProfile();

	UPROPERTY(config)
	bool bHasAppliedHardwareProfile = false;

	/** Bump to force re-auto-detect after quality retunes. */
	UPROPERTY(config)
	int32 GraphicsSettingsVersion = 0;

	UPROPERTY(config)
	EThreeDensityGraphicsPreset GraphicsPreset = EThreeDensityGraphicsPreset::Medium;

	UPROPERTY(config)
	bool bLumenEnabled = false;

	UPROPERTY(config)
	int32 MaxFPS = 120;

	UPROPERTY(config)
	bool bLastApplyWasAuto = true;

	UPROPERTY(config)
	float LastBenchmarkFPS = 0.0f;
};
