#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "BLMortarBarrage.generated.h"

class USoundBase;

/**
 * Fuego de mortero sobre una zona (misión 4). Activado por un objetivo (su etiqueta) empieza a caer un proyectil cada
 * Interval segundos dentro de la caja (actor ± Extent); con la etiqueta "<tag>_Stop" se detiene. Cada proyectil
 * silba WhistleTime segundos antes de caer (se oye dónde va a caer), explota con daño radial que las paredes paran,
 * polvo, sonido y sacudida. Parte de los proyectiles caen cerca del jugador (nunca encima: MinPlayerDistance).
 */
UCLASS()
class BLACKLINE_API ABLMortarBarrage : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLMortarBarrage();

	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	UPROPERTY(EditAnywhere, Category = "Mortar") FVector Extent = FVector(3000.f, 2500.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Mortar") FVector2D Interval = FVector2D(2.5f, 5.f);
	UPROPERTY(EditAnywhere, Category = "Mortar") float WhistleTime = 1.4f;
	UPROPERTY(EditAnywhere, Category = "Mortar") float Damage = 130.f;
	UPROPERTY(EditAnywhere, Category = "Mortar") float InnerRadius = 220.f;
	UPROPERTY(EditAnywhere, Category = "Mortar") float OuterRadius = 750.f;
	/** Probabilidad de que caiga cerca del jugador (entre MinPlayerDistance y NearRadius). */
	UPROPERTY(EditAnywhere, Category = "Mortar") float NearPlayerChance = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Mortar") float MinPlayerDistance = 500.f;
	UPROPERTY(EditAnywhere, Category = "Mortar") float NearRadius = 1600.f;
	UPROPERTY(EditAnywhere, Category = "Mortar") TObjectPtr<USoundBase> WhistleSound;
	UPROPERTY(EditAnywhere, Category = "Mortar") TArray<TObjectPtr<USoundBase>> ExplosionSounds;

	bool IsFiring() const { return bActive; }
	int32 GetImpacts() const { return Impacts; }

private:
	void Launch();
	void Impact(const FVector& Point);

	bool bActive = false;
	float NextShot = 1.f;
	int32 Impacts = 0;
	struct FShell { FVector Point; float Time; };
	TArray<FShell> Incoming;
};
