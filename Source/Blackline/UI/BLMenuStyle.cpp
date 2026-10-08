#include "UI/BLMenuStyle.h"

#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace BLMenu
{
	namespace
	{
		const UFont* LoadFont(const TCHAR* Path)
		{
			static TMap<FString, TStrongObjectPtr<UFont>> Cache;
			if (TStrongObjectPtr<UFont>* F = Cache.Find(Path))
			{
				return F->Get();
			}
			UFont* Font = LoadObject<UFont>(nullptr, Path);
			Cache.Add(Path, TStrongObjectPtr<UFont>(Font));
			return Font;
		}

		void Play(const TCHAR* Name, float Volume)
		{
			static TMap<FString, TStrongObjectPtr<USoundBase>> Cache;
			TStrongObjectPtr<USoundBase>* Found = Cache.Find(Name);
			if (!Found)
			{
				const FString Path = FString::Printf(TEXT("/Game/Audio/UI/%s.%s"), Name, Name);
				Found = &Cache.Add(Name, TStrongObjectPtr<USoundBase>(LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet)));
			}
			UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
			if (World && Found->IsValid())
			{
				UGameplayStatics::PlaySound2D(World, Found->Get(), Volume);
			}
		}
	}

	FSlateFontInfo Mono(int32 Size)
	{
		return FSlateFontInfo(LoadFont(TEXT("/Engine/EngineFonts/DroidSansMono.DroidSansMono")), Size);
	}

	FSlateFontInfo Sans(int32 Size, bool bBold)
	{
		return FSlateFontInfo(LoadFont(TEXT("/Engine/EngineFonts/Roboto.Roboto")), Size, bBold ? FName("Bold") : FName("Regular"));
	}

	const FSlateBrush* White()
	{
		return FCoreStyle::Get().GetBrush("WhiteBrush");
	}

	void PlayMove() { Play(TEXT("SW_UI_MenuMove"), 0.6f); }
	void PlaySelect() { Play(TEXT("SW_UI_MenuSelect"), 0.8f); }
	void PlayBack() { Play(TEXT("SW_UI_MenuBack"), 0.6f); }
	void PlayTick() { Play(TEXT("SW_UI_MenuTick"), 0.6f); }
}
