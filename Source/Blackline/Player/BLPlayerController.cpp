#include "Player/BLPlayerController.h"

#include "Blackline.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/BLCheckpointSubsystem.h"
#include "Player/BLCharacter.h"
#include "UI/BLMenuStyle.h"
#include "UI/SBLPauseMenu.h"
#include "Widgets/SWeakWidget.h"

ABLPlayerCameraManager::ABLPlayerCameraManager()
{
	ViewPitchMin = -80.f;
	ViewPitchMax = 80.f;
}

ABLPlayerController::ABLPlayerController()
{
	PlayerCameraManagerClass = ABLPlayerCameraManager::StaticClass();

	static const TCHAR* ContextPaths[] = {
		TEXT("/Game/Input/IMC_Default.IMC_Default"),        // WASD / salto / mando (pack de Epic)
		TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"),    // ratón (pack de Epic)
		TEXT("/Game/Input/IMC_Blackline.IMC_Blackline"),    // acciones propias (sprint, agacharse, lean, ADS...)
	};
	for (const TCHAR* Path : ContextPaths)
	{
		ConstructorHelpers::FObjectFinder<UInputMappingContext> Finder(Path);
		if (Finder.Succeeded())
		{
			DefaultMappingContexts.Add(Finder.Object);
		}
	}
}

void ABLPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ABLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}
	// Pausa: Esc o el botón de opciones del mando (funciona con el juego en pausa)
	for (const FKey& Key : { EKeys::Escape, EKeys::Gamepad_Special_Right })
	{
		FInputKeyBinding& B = InputComponent->BindKey(Key, IE_Pressed, this, &ABLPlayerController::TogglePauseMenu);
		B.bExecuteWhenPaused = true;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : DefaultMappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}
	}
	if (DefaultMappingContexts.Num() < 3)
	{
		UE_LOG(LogBlackline, Warning, TEXT("Faltan contextos de input (%d/3). ¿Se ejecutó Tools/UnrealPython/create_input_assets.py?"), DefaultMappingContexts.Num());
	}
}

// ---------------------------------------------------------------------------
// Menú de pausa
// ---------------------------------------------------------------------------

void ABLPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (PauseContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PauseContainer.ToSharedRef());
	}
	Super::EndPlay(Reason);
}

void ABLPlayerController::SetPauseMenu(bool bOpen)
{
	if (bOpen == IsPauseMenuOpen() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	if (bOpen)
	{
		SAssignNew(PauseMenu, SBLPauseMenu).Owner(this);
		PauseContainer = SNew(SWeakWidget).PossiblyNullContent(PauseMenu.ToSharedRef());
		GEngine->GameViewport->AddViewportWidgetContent(PauseContainer.ToSharedRef(), 20);
		SetPause(true);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(PauseMenu);
		SetInputMode(Mode);
		bShowMouseCursor = true;
		PauseMenu->FocusFirst();
		BLMenu::PlaySelect();
	}
	else
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PauseContainer.ToSharedRef());
		PauseMenu.Reset();
		PauseContainer.Reset();
		SetPause(false);
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		BLMenu::PlayBack();
	}
}

void ABLPlayerController::RestartFromCheckpoint()
{
	SetPauseMenu(false);
	ABLCharacter* C = Cast<ABLCharacter>(GetPawn());
	UBLCheckpointSubsystem* Checkpoints = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>();
	if (C && Checkpoints && Checkpoints->HasCheckpoint())
	{
		Checkpoints->RespawnPlayer(C);
	}
}

void ABLPlayerController::RestartMission()
{
	SetPauseMenu(false);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ABLPlayerController::QuitToMenu()
{
	SetPauseMenu(false);
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/Menu/L_MainMenu")));
}
