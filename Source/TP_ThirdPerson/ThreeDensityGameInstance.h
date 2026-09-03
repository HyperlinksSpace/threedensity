// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "ThreeDensityGameInstance.generated.h"

UCLASS()
class UThreeDensityGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void OnStart() override;
	virtual void Shutdown() override;

	/** Start a short FPS sample and auto-tune if needed. */
	UFUNCTION(BlueprintCallable, Category="Performance")
	void StartPerformanceBenchmark(bool bAutoAdjust = true);

protected:

	bool BenchmarkTick(float DeltaTime);
	void FinishBenchmark();

	bool bBenchmarking = false;
	bool bBenchmarkAutoAdjust = true;
	float BenchmarkAccumTime = 0.0f;
	float BenchmarkSampleTime = 0.0f;
	int32 BenchmarkSampleCount = 0;
	FTSTicker::FDelegateHandle BenchmarkTickerHandle;
};
