#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/BLEnemyCharacter.h"
#include "BLVarek.generated.h"

/**
 * Tomas Varek, el informante (Bloque 11, fase 6). Civil sin arma: mismo cuerpo y animación en C++ que el miliciano
 * pero con las animaciones sin arma de Epic, ropa civil y sin equipo. Empieza de rodillas (retenido); al liberarlo
 * (interactuable "Liberar a Varek") sigue al jugador con ABLVarekController. No muere (rehén protegido; ver MEMORIA).
 */
UCLASS()
class BLACKLINE_API ABLVarek : public ABLEnemyCharacter
{
	GENERATED_BODY()

public:
	ABLVarek(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;

	void Free();
	bool IsFree() const { return bFree; }

private:
	UFUNCTION() void HandleFreed(class ABLInteractable* Interactable, class ABLCharacter* User);
	bool bFree = false;
};

/** Sigue al jugador a 2,5–4,5 m; trota si se queda atrás, se agacha cerca de un tiroteo y reaparece a su lado si se separan mucho. */
UCLASS()
class BLACKLINE_API ABLVarekController : public AAIController
{
	GENERATED_BODY()

public:
	ABLVarekController();
	virtual void Tick(float DeltaTime) override;

private:
	float RepathTimer = 0.f;
};
