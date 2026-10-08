#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BLTargetDummy.generated.h"

class UBLHealthComponent;
class UBLHitReactionComponent;
struct FBLDamageInfo;

/**
 * Diana de entrenamiento: maniquí con salud, daño por zonas, reacción física y ragdoll al morir.
 * Reaparece en su sitio tras RespawnDelay. Sirve para validar el combate antes de la IA (Bloque 5),
 * que reutilizará los mismos componentes (salud + reacción).
 */
UCLASS()
class BLACKLINE_API ABLTargetDummy : public ACharacter
{
	GENERATED_BODY()

public:
	ABLTargetDummy(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	UBLHealthComponent* GetHealth() const { return Health; }
	UBLHitReactionComponent* GetHitReaction() const { return HitReaction; }

	/** Vuelve a ponerse en pie con la salud llena. */
	UFUNCTION(BlueprintCallable, Category = "Dummy") void Revive();

	/** 0 = no reaparece. */
	UPROPERTY(EditAnywhere, Category = "Dummy") float RespawnDelay = 5.f;
	UPROPERTY(EditAnywhere, Category = "Dummy") TObjectPtr<UAnimationAsset> IdleAnim;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UBLHealthComponent> Health;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UBLHitReactionComponent> HitReaction;

private:
	UFUNCTION() void HandleDeath(const FBLDamageInfo& Info);

	FTransform SpawnTransform;
	FTimerHandle RespawnTimer;
};
