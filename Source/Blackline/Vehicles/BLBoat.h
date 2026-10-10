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

	bool IsDocked() const { return State == EState::Docked; }
	bool IsActive() const { return State != EState::Hidden; }

private:
	enum class EState : uint8 { Hidden, Approach, Docked };

	void EnableBoarding();

	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<UPointLightComponent> NavLight;
	UPROPERTY(VisibleAnywhere, Category = "Boat") TObjectPtr<UAudioComponent> Motor;

	EState State = EState::Hidden;
	int32 PathIndex = 1;
	float CurrentSpeed = 0.f;
	float Time = 0.f;
	float Bow = 0.f;
	bool bBoardRequested = false;
};
