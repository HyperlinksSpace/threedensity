// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#include "ThreeDensityGameInstance.h"
#include "ThreeDensityGameUserSettings.h"

void UThreeDensityGameInstance::OnStart()
{
	Super::OnStart();

	if (UThreeDensityGameUserSettings* Settings = UThreeDensityGameUserSettings::GetThreeDensitySettings())
	{
		Settings->ApplyOnStartup();
	}
}
