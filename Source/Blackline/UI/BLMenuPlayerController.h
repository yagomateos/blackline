#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "BLMenuPlayerController.generated.h"

class SBLMainMenu;
class UAudioComponent;
class USoundBase;

/** Mapa del menú principal (L_MainMenu): sin personaje, solo el menú. */
UCLASS()
class BLACKLINE_API ABLMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABLMenuGameMode();
	virtual void BeginPlay() override;
};

/**
 * Controlador del menú principal: muestra SBLMainMenu, pone la música (SW_Mus_Menu_Loop) y lanza la misión.
 * Prueba automática: -BLTest=Menu (recorre las páginas, cambia opciones y comprueba que se guardan, hace capturas
 * y despliega la misión 1 desde el patio).
 */
UCLASS()
class BLACKLINE_API ABLMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABLMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;

	/** Carga la misión 1 empezando en la fase indicada (0 = inserción ... 4 = local de Varek). */
	void StartMission(int32 StartPhase);
	void QuitGame();
	bool IsLoading() const { return LoadTimer >= 0.f; }

	UPROPERTY(EditAnywhere, Category = "Menu") TObjectPtr<USoundBase> MenuMusic;

private:
	void BuildTest();

	TSharedPtr<SBLMainMenu> Menu;
	TSharedPtr<SWidget> MenuContainer;
	UPROPERTY() TObjectPtr<UAudioComponent> Music;
	float LoadTimer = -1.f;
	int32 FramesShown = 0;
	FString PendingLevelOptions;

	// Prueba automática del menú
	struct FMenuStep { FString Name; float Duration; TFunction<void()> Begin; TFunction<bool(FString&)> Verify; };
	TArray<FMenuStep> TestSteps;
	int32 TestIndex = INDEX_NONE;
	float TestTime = 0.f;
	TArray<FString> TestResults;
	int32 TestPassed = 0;
	int32 TestFailed = 0;
};
