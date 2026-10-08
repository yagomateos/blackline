#include "Core/BLGameMode.h"

#include "Player/BLCharacter.h"
#include "Player/BLPlayerController.h"
#include "UI/BLHUD.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Sound/SoundBase.h"
#include "Sound/ReverbEffect.h"
#include "UObject/ConstructorHelpers.h"

ABLGameMode::ABLGameMode()
{
	DefaultPawnClass = ABLCharacter::StaticClass();
	PlayerControllerClass = ABLPlayerController::StaticClass();
	HUDClass = ABLHUD::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> Ambient(TEXT("/Game/Audio/Ambience/SW_Amb_WarCity_Loop.SW_Amb_WarCity_Loop"));
	if (Ambient.Succeeded())
	{
		AmbientLoop = Ambient.Object;
	}
	static ConstructorHelpers::FObjectFinder<UReverbEffect> Reverb(TEXT("/Game/Audio/Settings/RE_UrbanOutdoor.RE_UrbanOutdoor"));
	if (Reverb.Succeeded())
	{
		EnvironmentReverb = Reverb.Object;
	}
}

void ABLGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (EnvironmentReverb)
	{
		UGameplayStatics::ActivateReverbEffect(this, EnvironmentReverb, TEXT("Entorno"), 0.f, 1.f, 1.f);
	}
	if (AmbientLoop)
	{
		AmbientComponent = UGameplayStatics::SpawnSound2D(this, AmbientLoop, AmbientVolume, 1.f, 0.f, nullptr, false, false);
		if (AmbientComponent)
		{
			AmbientComponent->FadeIn(2.f, AmbientVolume);
		}
	}
}

void ABLGameMode::SetAmbienceScale(float Scale, float FadeTime)
{
	if (AmbientComponent)
	{
		AmbientComponent->AdjustVolume(FadeTime, Scale);
	}
}

AActor* ABLGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	FString Phase;
	if (FParse::Value(FCommandLine::Get(), TEXT("BLStart="), Phase) && GetWorld())
	{
		const FName Tag(*(TEXT("BLTest_Start_") + Phase));
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->Tags.Contains(Tag))
			{
				return *It;
			}
		}
	}
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}
