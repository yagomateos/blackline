#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/BLHealthComponent.h"
#include "BLHitReactionComponent.generated.h"

class UPhysicalAnimationComponent;
class USkeletalMeshComponent;

/**
 * Reacción física a los impactos de un personaje (dianas ahora, IA en el Bloque 5):
 *  - Golpe: los huesos por debajo de ReactionRootBone pasan a simulación guiada por la animación
 *    (Physical Animation) y reciben un impulso en el hueso alcanzado; el peso vuelve a 0 en RecoverTime.
 *  - Muerte: ragdoll completo con el impulso del último disparo (cápsula y movimiento desactivados).
 * Escucha al UBLHealthComponent del dueño. Sin Anim Blueprint: funciona sobre cualquier animación.
 */
UCLASS(ClassGroup = (Blackline), meta = (BlueprintSpawnableComponent))
class BLACKLINE_API UBLHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLHitReactionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Peso actual de la reacción física (0..1). */
	float GetReactionWeight() const { return Weight; }
	bool IsRagdoll() const { return bRagdoll; }

	/** Deshace el ragdoll y vuelve a la animación (reaparición de dianas). */
	void ResetRagdoll();

	/** Hueso desde el que se simula en los golpes (incluido). */
	UPROPERTY(EditAnywhere, Category = "HitReaction") FName ReactionRootBone = FName("spine_01");
	/** Impulso por punto de daño (cm/s·kg aprox.) en los golpes y al morir. */
	UPROPERTY(EditAnywhere, Category = "HitReaction") float ImpulsePerDamage = 260.f;
	UPROPERTY(EditAnywhere, Category = "HitReaction") float DeathImpulsePerDamage = 420.f;
	UPROPERTY(EditAnywhere, Category = "HitReaction") float RecoverTime = 0.45f;
	/** Fuerza de los motores que devuelven el cuerpo a la animación. */
	UPROPERTY(EditAnywhere, Category = "HitReaction") float OrientationStrength = 1200.f;
	UPROPERTY(EditAnywhere, Category = "HitReaction") float AngularVelocityStrength = 120.f;

private:
	UFUNCTION() void HandleDamaged(const FBLDamageInfo& Info);
	UFUNCTION() void HandleDeath(const FBLDamageInfo& Info);
	void StopSimulation();

	UPROPERTY() TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimation;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Mesh;

	float Weight = 0.f;
	bool bSimulating = false;
	bool bRagdoll = false;
	FTransform MeshRelative;
	TEnumAsByte<ECollisionEnabled::Type> SavedMeshCollision = ECollisionEnabled::QueryOnly;
	FName SavedMeshProfile;
};
