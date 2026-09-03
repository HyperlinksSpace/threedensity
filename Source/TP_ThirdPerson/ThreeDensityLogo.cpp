// Copyright 2026 HyperlinksSpace. All Rights Reserved.

#include "ThreeDensityLogo.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateBrush.h"
#include "TP_ThirdPerson.h"

namespace
{
	UTexture2D* GLockupTexture = nullptr;
	UTexture2D* GMarkTexture = nullptr;
	TSharedPtr<FSlateBrush> GLockupBrush;
	TSharedPtr<FSlateBrush> GMarkBrush;

	UTexture2D* LoadPngTexture(const FString& AbsolutePath)
	{
		TArray<uint8> FileData;
		if (!FFileHelper::LoadFileToArray(FileData, *AbsolutePath) || FileData.Num() == 0)
		{
			UE_LOG(LogTP_ThirdPerson, Warning, TEXT("Logo file missing: %s"), *AbsolutePath);
			return nullptr;
		}

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
		{
			return nullptr;
		}

		TArray64<uint8> RawData;
		if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
		{
			return nullptr;
		}

		UTexture2D* Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
		if (!Texture)
		{
			return nullptr;
		}

		Texture->CompressionSettings = TC_EditorIcon;
		Texture->SRGB = true;
		Texture->AddToRoot();

		void* MipData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(MipData, RawData.GetData(), RawData.Num());
		Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}

	TSharedPtr<FSlateBrush> MakeBrush(UTexture2D* Texture, const FVector2D& Size)
	{
		if (!Texture)
		{
			return nullptr;
		}
		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Texture);
		Brush->ImageSize = Size;
		Brush->DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}
}

UTexture2D* ThreeDensityLogo::LoadTexture(const FString& RelativeContentPath)
{
	const FString Path = FPaths::ProjectContentDir() / RelativeContentPath;
	return LoadPngTexture(Path);
}

TSharedPtr<FSlateBrush> ThreeDensityLogo::GetLockupBrush()
{
	if (!GLockupBrush.IsValid())
	{
		GLockupTexture = LoadTexture(TEXT("UI/Logo.png"));
		GLockupBrush = MakeBrush(GLockupTexture, FVector2D(280.f, 88.f));
	}
	return GLockupBrush;
}

TSharedPtr<FSlateBrush> ThreeDensityLogo::GetMarkBrush()
{
	if (!GMarkBrush.IsValid())
	{
		GMarkTexture = LoadTexture(TEXT("UI/LogoMark.png"));
		GMarkBrush = MakeBrush(GMarkTexture, FVector2D(56.f, 56.f));
	}
	return GMarkBrush;
}
