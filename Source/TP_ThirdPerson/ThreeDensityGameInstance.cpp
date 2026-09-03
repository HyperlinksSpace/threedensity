// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#include "ThreeDensityGameInstance.h"
#include "ThreeDensityGameUserSettings.h"
#include "TP_ThirdPerson.h"
#include "Containers/Ticker.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UThreeDensityGameInstance::OnStart()
{
	Super::OnStart();

	if (UThreeDensityGameUserSettings* Settings = UThreeDensityGameUserSettings::GetThreeDensitySettings())
	{
		Settings->ApplyOnStartup();
	}

	// Let shaders/streaming settle, then measure real FPS.
	if (UWorld* World = GetWorld())
	{
		FTimerHandle DelayHandle;
		World->GetTimerManager().SetTimer(
			DelayHandle,
			FTimerDelegate::CreateUObject(this, &UThreeDensityGameInstance::StartPerformanceBenchmark, true),
			5.0f,
			false);
	}
}

void UThreeDensityGameInstance::Shutdown()
{
	if (BenchmarkTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(BenchmarkTickerHandle);
		BenchmarkTickerHandle.Reset();
	}
	Super::Shutdown();
}

void UThreeDensityGameInstance::StartPerformanceBenchmark(bool bAutoAdjust)
{
	if (bBenchmarking)
	{
		return;
	}

	bBenchmarking = true;
	bBenchmarkAutoAdjust = bAutoAdjust;
	BenchmarkAccumTime = 0.0f;
	BenchmarkSampleTime = 0.0f;
	BenchmarkSampleCount = 0;

	if (BenchmarkTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(BenchmarkTickerHandle);
	}

	BenchmarkTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UThreeDensityGameInstance::BenchmarkTick));

	UE_LOG(LogTP_ThirdPerson, Log, TEXT("Performance benchmark started (autoAdjust=%s)"),
		bAutoAdjust ? TEXT("yes") : TEXT("no"));
}

bool UThreeDensityGameInstance::BenchmarkTick(float DeltaTime)
{
	if (!bBenchmarking)
	{
		return false;
	}

	// Skip hitchy first frames after load.
	BenchmarkAccumTime += DeltaTime;
	if (BenchmarkAccumTime < 0.75f)
	{
		return true;
	}

	BenchmarkSampleTime += DeltaTime;
	++BenchmarkSampleCount;

	if (BenchmarkSampleTime >= 3.0f && BenchmarkSampleCount > 0)
	{
		FinishBenchmark();
		return false;
	}

	return true;
}

void UThreeDensityGameInstance::FinishBenchmark()
{
	bBenchmarking = false;
	if (BenchmarkTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(BenchmarkTickerHandle);
		BenchmarkTickerHandle.Reset();
	}

	const float AvgFPS = BenchmarkSampleCount / FMath::Max(BenchmarkSampleTime, KINDA_SMALL_NUMBER);

	if (UThreeDensityGameUserSettings* Settings = UThreeDensityGameUserSettings::GetThreeDensitySettings())
	{
		Settings->ApplyBenchmarkResult(AvgFPS, bBenchmarkAutoAdjust);
	}
}
