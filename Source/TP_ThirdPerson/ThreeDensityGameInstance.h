// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ThreeDensityGameInstance.generated.h"

UCLASS()
class UThreeDensityGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void OnStart() override;
};
