#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/BLVarek.h"
#include "Mission/BLActivatable.h"
#include "BLVIP.generated.h"

/**
 * "El inglés" (misión 5): responsable de Corvane en Kessra. Mismo cuerpo civil y animaciones sin arma que Varek,
 * con traje oscuro; **vulnerable** (si muere, la misión falla: lleva la etiqueta "BLVIP"). Lo mueve ABLVIPController.
 */
UCLASS()
class BLACKLINE_API ABLVIP : public ABLVarek
{
	GENERATED_BODY()

public:
	ABLVIP(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	/** Solo le hace daño el jugador (un tiro perdido de un miliciano no debe hacer fallar la misión). */
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};

/**
 * Al activarse (etiqueta del objetivo) corre por su ruta (PatrolPoints del personaje) hasta el helipuerto y espera
 * agachado al helicóptero de Corvane (EscapeHeliTag). Si el helicóptero queda inutilizado, se rinde: de rodillas, y se
 * habilita el interactuable con ArrestTag junto a él. Si pasan EscapeTime segundos en el helipuerto con el helicóptero
 * entero, escapa: misión fallida.
 */
UCLASS()
class BLACKLINE_API ABLVIPController : public AAIController, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLVIPController();
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	UPROPERTY(EditAnywhere, Category = "VIP") FName EscapeHeliTag = TEXT("BLHeliCorvane");
	UPROPERTY(EditAnywhere, Category = "VIP") FName ArrestTag = TEXT("BLObjective_VIP");
	UPROPERTY(EditAnywhere, Category = "VIP") float EscapeTime = 45.f;

	bool IsFleeing() const { return State == EState::Fleeing; }
	bool HasSurrendered() const { return State == EState::Surrendered; }

private:
	enum class EState : uint8 { Idle, Fleeing, Waiting, Surrendered };
	bool IsHeliDown() const;
	void Surrender();

	EState State = EState::Idle;
	int32 PathIndex = 0;
	float WaitTime = 0.f;
	float RepathTimer = 0.f;
};
