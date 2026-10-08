#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "BLPlayerController.generated.h"

class UInputMappingContext;

/** Límites de pitch de la cámara FPS. */
UCLASS()
class BLACKLINE_API ABLPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	ABLPlayerCameraManager();
};

UCLASS()
class BLACKLINE_API ABLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABLPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Contextos de input que se añaden al jugador local (orden = prioridad 0). */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};
