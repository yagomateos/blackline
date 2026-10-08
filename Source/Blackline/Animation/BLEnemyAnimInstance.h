#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BLEnemyAnimInstance.generated.h"

class UAnimSequence;

/** Datos del hilo de juego para el de animación. */
struct FBLEnemyAnimState
{
	/** Ya hay datos del hilo de juego (la primera evaluación llega al registrar la malla, antes del primer Update). */
	bool bReady = false;
	TObjectPtr<UAnimSequence> Idle;
	TObjectPtr<UAnimSequence> WalkA, WalkB, JogA, JogB;   // las dos direcciones vecinas de cada marcha
	float DirBlend = 0.f;     // 0 = A, 1 = B
	float JogWeight = 0.f;    // 0 andar .. 1 trotar
	float MoveWeight = 0.f;   // 0 quieto .. 1 en marcha
	float LocoPhase = 0.f;    // fase normalizada común (pies sincronizados)
	float IdleTime = 0.f;
	TObjectPtr<UAnimSequence> Reload;
	float ReloadTime = 0.f;
	float ReloadWeight = 0.f;
	float AimPitch = 0.f;     // grados (+ arriba)
	float CrouchAlpha = 0.f;
	float Kick = 0.f;         // retroceso del disparo (muelle)
	float LeftHandAlpha = 1.f;
	FVector GripInWeapon = FVector::ZeroVector;
	FTransform HandGripRLocal = FTransform::Identity;
	FTransform HandGripLLocal = FTransform::Identity;
	bool bHasWeapon = false;
};

struct FBLEnemyAnimProxy : public FAnimInstanceProxy
{
	FBLEnemyAnimProxy() = default;
	explicit FBLEnemyAnimProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void ApplyProcedural(FPoseContext& Output) const;
	FBLEnemyAnimState State;
};

/**
 * Animación del miliciano en C++ (sin Anim Blueprint): locomoción de fusil en 8 direcciones (andar/trotar,
 * mezcla por velocidad y dirección relativa a donde mira), idle de apuntado, recarga en la parte superior,
 * agacharse procedural (pelvis abajo + IK de piernas), inclinación del torso para apuntar en vertical,
 * retroceso del disparo e IK de la mano izquierda al guardamanos del arma.
 */
UCLASS(Transient, NotBlueprintable)
class BLACKLINE_API UBLEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	const FBLEnemyAnimState& GetState() const { return State; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	FBLEnemyAnimState State;
	float KickValue = 0.f;
	float KickVelocity = 0.f;
};
