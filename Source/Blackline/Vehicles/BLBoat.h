#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "BLBoat.generated.h"

class UAudioComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Lancha neumática semirrígida (misión 2). Dos usos:
 *  - Atrezo de la inserción (bStartDocked): amarrada y con el motor apagado; solo se mece.
 *  - Extracción (etiqueta "BLBoat"): escondida hasta que la activa un objetivo; llega por Path (el canal)
 *    frenando y se queda al pie del embarcadero meciéndose con el motor al ralentí. El interactuable con
 *    InteractTag ("Subir a la lancha") solo se habilita con la etiqueta "BLBoat_Board" (el objetivo de subir):
 *    si se pudiera usar antes, durante la defensa, el objetivo de subir no se cumpliría nunca.
 * El agua está a WaterZ: la lancha se mece (cabeceo, balanceo, subida) y levanta proa al acelerar.
 *  - Pilotaje (etiqueta "<...>_Drive", el objetivo siguiente a subir): el jugador toma el timón (ABLCharacter::BoardBoat)
 *    y la lleva con W/S (acelerador, marcha atrás) y A/D (timón). Choca con muelles, pilotes y muros invisibles
 *    (barrido de una caja del tamaño del casco contra WorldStatic). Si el jugador muere, al reaparecer vuelve al timón.
 */
UCLASS()
class BLACKLINE_API ABLBoat : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLBoat();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	UPROPERTY(EditAnywhere, Category = "Boat") TArray<FVector> Path;
	UPROPERTY(EditAnywhere, Category = "Boat") float Speed = 900.f;
	UPROPERTY(EditAnywhere, Category = "Boat") bool bStartDocked = false;
	UPROPERTY(EditAnywhere, Category = "Boat") FName InteractTag = TEXT("BLObjective_Boat");
	/** Dónde queda el interactuable respecto a la lancha (junto a la borda, del lado del muelle). */
	UPROPERTY(EditAnywhere, Category = "Boat") FVector BoardOffset = FVector(-60.f, -140.f, 60.f);

	// ---- Pilotaje ----
	UPROPERTY(EditAnywhere, Category = "Boat|Drive") float DriveMaxSpeed = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Boat|Drive") float DriveReverseSpeed = 350.f;
	UPROPERTY(EditAnywhere, Category = "Boat|Drive") float DriveAccel = 420.f;
	UPROPERTY(EditAnywhere, Category = "Boat|Drive") float DriveTurnRate = 48.f;
	/** Media caja del casco para los choques (largo, manga, alto). */
	UPROPERTY(EditAnywhere, Category = "Boat|Drive") FVector HullExtent = FVector(360.f, 150.f, 40.f);

	bool IsDocked() const { return State == EState::Docked; }
	bool IsActive() const { return State != EState::Hidden; }
	bool IsDriven() const { return State == EState::Driven; }
	float GetCurrentSpeed() const { return CurrentSpeed; }
	USceneComponent* GetDriverSeat() const { return DriverSeat; }
	/** Entrada del jugador este frame (se acumula y se consume en Tick). */
	void AddDriveInput(float Throttle, float Steer);
	void OnDriverLeft();

private:
	enum class EState : uint8 { Hidden, Approach, Docked, Driven };

	void EnableBoarding();
	void StartDriving();
	void TickDrive(float DeltaTime);

	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<UPointLightComponent> NavLight;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<UAudioComponent> Motor;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<USceneComponent> DriverSeat;

	EState State = EState::Hidden;
	int32 PathIndex = 1;
	float CurrentSpeed = 0.f;
	float Time = 0.f;
	float Bow = 0.f;
	bool bBoardRequested = false;
	FVector2D DriveInput = FVector2D::ZeroVector;
	float DriveYaw = 0.f;
	float BumpCooldown = 0.f;
};
