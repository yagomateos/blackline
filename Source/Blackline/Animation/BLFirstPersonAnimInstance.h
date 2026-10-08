#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BLFirstPersonAnimInstance.generated.h"

class UAnimSequence;

/** Datos copiados del hilo de juego al de animación en cada actualización. */
struct FBLFirstPersonAnimState
{
	TObjectPtr<UAnimSequence> BaseAnim;
	float BaseTime = 0.f;
	TObjectPtr<UAnimSequence> ActionAnim;
	float ActionTime = 0.f;
	float ActionWeight = 0.f;
	float LeftHandIKAlpha = 0.f;
	FTransform LeftGripInWeapon = FTransform::Identity;
	/** Recarga procedural: la mano izquierda va a este punto del arma (peso 0..1). */
	float LeftHandOverrideAlpha = 0.f;
	FVector LeftHandOverrideLocation = FVector::ZeroVector;
	/** Arma en espacio de componente de la malla FP (la coloca el rig). Ambas manos la siguen por IK. */
	FTransform WeaponInComponent = FTransform::Identity;
	bool bHasWeapon = false;
	FTransform HandGripRLocal = FTransform::Identity;  // socket HandGrip_R relativo a hand_r
	FTransform HandGripLLocal = FTransform::Identity;  // socket HandGrip_L relativo a hand_l
};

struct FBLFirstPersonAnimProxy : public FAnimInstanceProxy
{
	FBLFirstPersonAnimProxy() = default;
	explicit FBLFirstPersonAnimProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void ApplyHandIK(FPoseContext& Output) const;

	FBLFirstPersonAnimState State;
};

/**
 * Animación de los brazos en primera persona, sin Anim Blueprint (todo en C++):
 *  - Pose base en bucle (idle de fusil).
 *  - Una acción encima (recarga, equipar) con fundido de entrada/salida.
 *  - IK de dos huesos de ambas manos al arma: el rig coloca el arma delante de la cámara y las manos
 *    la siguen (el cuerpo no se mueve). La izquierda se desactiva con la curva "DisableLHandIK" de la
 *    acción o se lleva a otro punto del arma (recarga procedural).
 * El movimiento de cámara/arma (bob, sway, retroceso) es procedural y vive en UBLFirstPersonRigComponent.
 */
UCLASS(Transient, NotBlueprintable)
class BLACKLINE_API UBLFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetBaseAnim(UAnimSequence* Anim);
	/** Reproduce una acción ajustada a Duration segundos. */
	void PlayAction(UAnimSequence* Anim, float Duration, float BlendIn = 0.15f, float BlendOut = 0.25f);
	void StopAction(float BlendOut = 0.2f);
	bool IsActionPlaying() const { return ActionAnim != nullptr && !bActionStopping; }
	/** 0..1 peso de la acción actual. */
	float GetActionWeight() const { return ActionWeight; }

	/** Agarre de la mano izquierda en espacio de la malla del arma. bValid=false desactiva la IK. */
	void SetLeftHandGrip(const FTransform& GripInWeapon, bool bValid);

	/** Transform del arma en espacio de componente (mano derecha al pistolete, izquierda al guardamanos). */
	void SetWeaponTransform(const FTransform& WeaponInComponent, bool bValid);

	/** Lleva la mano izquierda (IK) a un punto del arma en vez del agarre (recarga procedural). */
	void SetLeftHandOverride(float Alpha, const FVector& LocationInWeapon);

	const FBLFirstPersonAnimState& GetState() const { return State; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	UPROPERTY() TObjectPtr<UAnimSequence> BaseAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> ActionAnim;

	FBLFirstPersonAnimState State;
	float BaseTime = 0.f;
	float ActionTime = 0.f;
	float ActionRate = 1.f;
	float ActionWeight = 0.f;
	float ActionBlendIn = 0.15f;
	float ActionBlendOut = 0.25f;
	bool bActionStopping = false;
	bool bHasGrip = false;
	float LeftHandIKAlpha = 0.f;
};
