#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "BLPlayerController.generated.h"

class UInputMappingContext;
class SBLPauseMenu;

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

	// ---- Menú de pausa (Bloque 10) ----
	void SetPauseMenu(bool bOpen);
	bool IsPauseMenuOpen() const { return PauseMenu.IsValid(); }
	void RestartFromCheckpoint();
	void RestartMission();
	void QuitToMenu();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void TogglePauseMenu() { SetPauseMenu(!IsPauseMenuOpen()); }
	virtual void SetupInputComponent() override;

	/** Contextos de input que se añaden al jugador local (orden = prioridad 0). */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

private:
	TSharedPtr<SBLPauseMenu> PauseMenu;
	TSharedPtr<SWidget> PauseContainer;
};
