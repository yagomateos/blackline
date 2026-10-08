#include "Core/BLGameMode.h"

#include "Player/BLCharacter.h"
#include "Player/BLPlayerController.h"
#include "UI/BLHUD.h"
#include "UI/BLUserSettings.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLPlayerController.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"
#include "Blackline.h"
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
	UBLUserSettings::Get()->ApplyAudio(this);
	if (UGameplayStatics::HasOption(OptionsString, TEXT("BLMenuTest")))
	{
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, this, &ABLGameMode::VerifyMenuDeploy, 4.f, false);
	}
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
	const FString Phase = GetStartPhase(this);
	if (!Phase.IsEmpty() && GetWorld())
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

FString ABLGameMode::GetStartPhase(const UObject* WorldContext)
{
	FString Phase;
	if (FParse::Value(FCommandLine::Get(), TEXT("BLStart="), Phase))
	{
		return Phase;
	}
	const AGameModeBase* GM = WorldContext ? UGameplayStatics::GetGameMode(WorldContext) : nullptr;
	return GM ? UGameplayStatics::ParseOption(GM->OptionsString, TEXT("BLStart")) : FString();
}

void ABLGameMode::VerifyMenuDeploy()
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const ABLMissionDirector* M = ABLMissionDirector::Get(this);
	const FString Phase = GetStartPhase(this);
	FVector Expected = FVector::ZeroVector;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(FName(*(TEXT("BLTest_Start_") + Phase)))) { Expected = It->GetActorLocation(); }
	}
	const float Dist = Pawn ? FVector::Dist2D(Pawn->GetActorLocation(), Expected) : 1e9f;
	const bool bOk = Pawn && Dist < 300.f && M && M->GetCurrentIndex() == 1;
	const FString Line = FString::Printf(TEXT("%s  %-14s fase %s, jugador a %.0f cm del inicio, objetivo %d"),
		bOk ? TEXT("PASS") : TEXT("FAIL"), TEXT("Despliegue"), *Phase, Dist, M ? M->GetCurrentIndex() + 1 : 0);
	UE_LOG(LogBlackline, Display, TEXT("[BLTest] %s"), *Line);
	const FString Path = FPaths::ProjectSavedDir() / TEXT("BLTest/Menu_results.txt");
	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *Path);
	Lines.Add(Line);
	// Menú de pausa: se abre, el juego queda en pausa; captura y cierre (con el reloj real: en pausa no hay temporizadores)
	ABLPlayerController* BPC = Cast<ABLPlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPC)
	{
		BPC->SetPauseMenu(true);
	}
	const bool bPaused = BPC && BPC->IsPauseMenuOpen() && GetWorld()->IsPaused();
	Lines.Add(FString::Printf(TEXT("%s  %-14s menú abierto=%d, juego en pausa=%d"), bPaused ? TEXT("PASS") : TEXT("FAIL"), TEXT("Pausa"),
		BPC && BPC->IsPauseMenuOpen(), GetWorld()->IsPaused()));
	Lines.Add(FString::Printf(TEXT("RESUMEN Menu: despliegue %s, pausa %s"), bOk ? TEXT("OK") : TEXT("FALLIDO"), bPaused ? TEXT("OK") : TEXT("FALLIDA")));
	FFileHelper::SaveStringArrayToFile(Lines, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("BLTest/Menu_Pausa.png"), true, false);
	TWeakObjectPtr<ABLGameMode> Self(this);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self](float) -> bool
	{
		if (Self.IsValid())
		{
			UKismetSystemLibrary::QuitGame(Self.Get(), nullptr, EQuitPreference::Quit, false);
		}
		return false;
	}), 1.5f);
}
