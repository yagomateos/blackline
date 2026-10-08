#include "Player/BLPlayerController.h"

#include "Blackline.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

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
