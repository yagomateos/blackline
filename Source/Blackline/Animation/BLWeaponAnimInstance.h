#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BLWeaponAnimInstance.generated.h"

class UBLWeaponComponent;

/** Desplazamientos de los huesos móviles del arma (espacio de la malla, cm). */
struct FBLWeaponMechState
{
	FName BoltBone;
	FName TriggerBone;
	FVector BoltOffset = FVector::ZeroVector;
	FVector TriggerOffset = FVector::ZeroVector;
};

struct FBLWeaponAnimProxy : public FAnimInstanceProxy
{
	FBLWeaponAnimProxy() = default;
	explicit FBLWeaponAnimProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	FBLWeaponMechState State;
};

/**
 * Mecánica del arma sin Anim Blueprint: cerrojo que cicla en cada disparo y queda abierto con el cargador
 * vacío (se suelta al golpear la retenida en la recarga en vacío) y gatillo que se mueve al apretar.
 * Lee el estado del UBLWeaponComponent del dueño, así sirve igual para el jugador y para la IA.
 */
UCLASS(Transient, NotBlueprintable)
class BLACKLINE_API UBLWeaponAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	const FBLWeaponMechState& GetMechState() const { return State; }
	/** 0 = cerrado, 1 = totalmente atrás. */
	float GetBoltAlpha() const { return BoltAlpha; }

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	UBLWeaponComponent* FindWeaponComponent() const;

	FBLWeaponMechState State;
	int32 LastShotsFired = INDEX_NONE;
	float CycleTime = 1.f;       // tiempo desde el último disparo
	float CycleDuration = 0.07f;
	bool bLockedOpen = false;
	float BoltAlpha = 0.f;
	float TriggerAlpha = 0.f;
};
