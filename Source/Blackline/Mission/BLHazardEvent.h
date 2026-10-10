#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "Mission/BLMissionTypes.h"
#include "BLHazardEvent.generated.h"

class USoundBase;
class ABLSmokeEmitter;

/**
 * Evento guionizado "ruta cortada" (misión 2: revienta una tubería de gas). Lo arma un objetivo (ActivateTags);
 * estalla cuando el jugador se acerca a menos de TriggerRadius (o a los Delay segundos si TriggerRadius = 0):
 * explosión en cada FirePoints, llamas que se quedan ardiendo (ABLSmokeEmitter con fuego), los actores con
 * BlockTag aparecen y bloquean (muro de fuego invisible, escombros), daño por fuego a quien se meta dentro y radio.
 */
UCLASS()
class BLACKLINE_API ABLHazardEvent : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLHazardEvent();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	UPROPERTY(EditAnywhere, Category = "Hazard") TArray<FVector> FirePoints;
	/** Distancia del jugador al actor que lo hace estallar (0 = a los Delay segundos de activarse). */
	UPROPERTY(EditAnywhere, Category = "Hazard") float TriggerRadius = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Hazard") float Delay = 0.f;
	UPROPERTY(EditAnywhere, Category = "Hazard") FName BlockTag = TEXT("BLHazardBlock");
	UPROPERTY(EditAnywhere, Category = "Hazard") float FireRadius = 180.f;
	UPROPERTY(EditAnywhere, Category = "Hazard") float FireDamagePerSecond = 45.f;
	UPROPERTY(EditAnywhere, Category = "Hazard") FVector FlameExtent = FVector(140.f, 140.f, 40.f);
	UPROPERTY(EditAnywhere, Category = "Hazard") TArray<FBLRadioLine> Radio;
	UPROPERTY(EditAnywhere, Category = "Hazard") TObjectPtr<USoundBase> BlastSound;

	bool IsArmed() const { return bArmed; }
	bool HasFired() const { return bFired; }

private:
	void Fire();

	bool bArmed = false;
	bool bFired = false;
	float ArmedTime = 0.f;
	float DamageTimer = 0.f;
	TArray<TWeakObjectPtr<ABLSmokeEmitter>> Fires;
	/** Bloqueos y si eran invisibles en el nivel (un muro invisible sigue invisible al activarse). */
	TArray<TWeakObjectPtr<AActor>> Blocks;
	TArray<bool> BlockWasHidden;
};
