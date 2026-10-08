#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLCheckpointVolume.generated.h"

class UBoxComponent;
class UArrowComponent;

/**
 * Zona de checkpoint: al entrar el jugador se guarda el progreso. Reaparece en la flecha (RespawnPoint).
 */
UCLASS()
class BLACKLINE_API ABLCheckpointVolume : public AActor
{
	GENERATED_BODY()

public:
	ABLCheckpointVolume();

	/** Identificador (si está vacío se usa el nombre del actor). */
	UPROPERTY(EditAnywhere, Category = "Checkpoint") FName CheckpointId;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UBoxComponent> Box;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UArrowComponent> RespawnPoint;

private:
	UFUNCTION() void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
