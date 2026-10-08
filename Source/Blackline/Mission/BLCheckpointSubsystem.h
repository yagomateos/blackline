#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/BLWeaponComponent.h"
#include "BLCheckpointSubsystem.generated.h"

class ABLCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBLCheckpointSignature, FName, CheckpointId);

/**
 * Checkpoints de la misión: guarda dónde reaparece el jugador y con qué armas/munición.
 * El primero es el punto de inicio (lo registra el personaje en BeginPlay); los siguientes los
 * activan los ABLCheckpointVolume (y más adelante el sistema de misión, por fase).
 * La reaparición restaura salud llena + el inventario guardado.
 */
UCLASS()
class BLACKLINE_API UBLCheckpointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Guarda un checkpoint con el estado actual del jugador. Devuelve false si ya era el actual. */
	bool SaveCheckpoint(FName Id, const FTransform& RespawnTransform, const ABLCharacter* Player);

	/** Inicio de nivel: solo si aún no hay checkpoint. */
	void RegisterStart(const ABLCharacter* Player);

	/** Lleva al jugador al último checkpoint con salud llena y el inventario guardado. */
	void RespawnPlayer(ABLCharacter* Player);

	bool HasCheckpoint() const { return bHasCheckpoint; }
	FName GetCurrentId() const { return CurrentId; }
	const FTransform& GetRespawnTransform() const { return RespawnTransform; }
	/** Tiempo de juego del último checkpoint alcanzado (para el aviso del HUD). */
	float GetLastReachedTime() const { return LastReachedTime; }

	UPROPERTY(BlueprintAssignable, Category = "Checkpoint") FBLCheckpointSignature OnCheckpointReached;

private:
	bool bHasCheckpoint = false;
	FName CurrentId;
	FTransform RespawnTransform;
	TArray<FBLWeaponSlot> SavedInventory;
	int32 SavedWeaponIndex = INDEX_NONE;
	float LastReachedTime = -100.f;
};
