// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

class UTexture2D;

namespace ThreeDensityLogo
{
	UTexture2D* LoadTexture(const FString& RelativeContentPath);
	TSharedPtr<FSlateBrush> GetLockupBrush();
	TSharedPtr<FSlateBrush> GetMarkBrush();
}
