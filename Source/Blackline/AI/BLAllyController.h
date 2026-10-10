#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BLAllyController.generated.h"

class ABLEnemyCharacter;

/**
 * Soldado aliado del ejército de Varania (misión 4). Sencillo a propósito: no se mueve de su puesto; busca al
 * miliciano visible más cercano, le apunta con un error que baja con el tiempo, dispara ráfagas cortas y se agacha
 * entre ráfagas (asomarse y esconderse). Recarga cuando vacía. La IA enemiga no lo ve como objetivo (solo al jugador),
 * pero sus balas sí pueden alcanzarle.
 * Se asigna a un ABLEnemyCharacter con EnemyRole = Ally poniéndole AIControllerClass = ABLAllyController en el nivel.
 */
UCLASS()
class BLACKLINE_API ABLAllyController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

	/** Los milicianos aparecen al otro lado del puente (~100 m): a esa distancia la puntería es mala, como debe ser. */
	UPROPERTY(EditAnywhere, Category = "Ally") float Range = 12000.f;
	UPROPERTY(EditAnywhere, Category = "Ally") float AimErrorStart = 220.f;
	UPROPERTY(EditAnywhere, Category = "Ally") float AimErrorMin = 45.f;

	int32 GetShotsFired() const { return ShotsFired; }
	AActor* GetTarget() const;

private:
	ABLEnemyCharacter* Body() const;
	bool CanSee(const ABLEnemyCharacter* Me, const AActor* Other) const;
	/** bFollowPlayer: va detrás del jugador (a 4-7 m) y dispara mientras. */
	void TickFollow(ABLEnemyCharacter* Me, float DeltaTime);

	TWeakObjectPtr<ABLEnemyCharacter> Target;
	float ScanTimer = 0.f;
	float FollowTimer = 0.f;
	float AimError = 220.f;
	int32 BurstLeft = 0;
	float BurstPause = 1.f;
	int32 LastShots = 0;
	int32 ShotsFired = 0;
	FVector AimOffset = FVector::ZeroVector;
};
