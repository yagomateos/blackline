#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/BLSpring.h"
#include "Weapons/BLWeaponData.h"
#include "BLFirstPersonRigComponent.generated.h"

class ABLCharacter;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBLFootstepSignature, bool, bLeftFoot, float, Intensity);

/**
 * Movimiento procedural de cámara y arma en primera persona.
 *
 * Cámara: head bob suave ligado a los pasos, inclinación al strafe, hundimiento al aterrizar,
 *         impulsos externos (retroceso, daño) y FOV de ADS.
 * Arma:   sway por inercia al mirar, bob, inclinación al strafe, pose de sprint, alineación ADS,
 *         reacción a salto/aterrizaje y bajada durante el mantle.
 *
 * Todo se escala con CameraMotionScale / WeaponMotionScale (accesibilidad: reducir movimiento).
 * Lo actualiza ABLCharacter::Tick para garantizar el orden (después del movimiento).
 */
UCLASS(ClassGroup = (Blackline), meta = (BlueprintSpawnableComponent))
class BLACKLINE_API UBLFirstPersonRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLFirstPersonRigComponent();

	void Initialize(ABLCharacter* InCharacter, UCameraComponent* InCamera, USceneComponent* InWeaponRoot, USceneComponent* InWeaponMesh);
	void UpdateRig(float DeltaTime);

	void NotifyLanded(float VerticalSpeed);
	void NotifyJumped();
	void NotifyMantleStarted(float HeightAlpha);

	/** Mira del arma equipada: socket (solo se usa su posición) o desplazamiento, y eje adelante de la malla. */
	void ConfigureWeaponSight(FName InSightSocket, const FVector& InSightLocalOffset, const FVector& InForwardAxis, float InAimDistance);
	/** Poses de cadera/sprint/mantle/recarga/equipar del arma equipada. */
	void ConfigureWeaponPoses(const FBLWeaponPoses& InPoses) { Poses = InPoses; }

	/** Cada disparo: temblor de cámara de alta frecuencia y golpe de FOV (además del retroceso). */
	void NotifyShot(float Strength);
	/** Al agacharse/levantarse el arma "cae" un poco. */
	void NotifyCrouchChanged(bool bCrouched);
	/** Movimiento extra del arma durante la recarga (golpes, tirones), en espacio de cámara. */
	void SetReloadDynamics(const FVector& Location, const FRotator& Rotation) { ReloadDynLoc = Location; ReloadDynRot = Rotation; }

	/** Pesos de las poses procedurales de recarga y de equipar (0..1). */
	void SetActionPoses(float InReloadAlpha, float InEquipAlpha) { ReloadAlpha = InReloadAlpha; EquipAlpha = InEquipAlpha; }

	/** Congela la calibración de ADS (durante recarga/equipar, para que se vea el movimiento de la animación). */
	void SetCalibrationFrozen(bool bFrozen) { bCalibrationFrozen = bFrozen; }

	/** Impulso rotacional de cámara (pitch/yaw/roll en grados/s). Para retroceso y daño. */
	UFUNCTION(BlueprintCallable, Category = "Rig")
	void AddCameraKick(const FRotator& Kick);

	/** Impulso al arma: posición (cm/s) y rotación (grados/s). Para retroceso visual. */
	UFUNCTION(BlueprintCallable, Category = "Rig")
	void AddWeaponKick(const FVector& Location, const FRotator& Rotation);

	/** Se dispara en cada paso (lo usará el audio de pisadas). */
	UPROPERTY(BlueprintAssignable, Category = "Rig")
	FBLFootstepSignature OnFootstep;

	// ---- Accesibilidad ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig|Accessibility", meta = (ClampMin = "0", ClampMax = "1"))
	float CameraMotionScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig|Accessibility", meta = (ClampMin = "0", ClampMax = "1"))
	float WeaponMotionScale = 1.f;

	// ---- FOV ----
	UPROPERTY(EditAnywhere, Category = "Rig|FOV") float BaseFOV = 90.f;
	UPROPERTY(EditAnywhere, Category = "Rig|FOV") float AimFOV = 68.f;
	UPROPERTY(EditAnywhere, Category = "Rig|FOV") float SprintFOVBonus = 4.f;
	UPROPERTY(EditAnywhere, Category = "Rig|FOV") float WeaponFOV = 50.f;
	UPROPERTY(EditAnywhere, Category = "Rig|FOV") float WeaponAimFOV = 44.f;

	// ---- Calibración de ADS ----
	// En ADS el punto de mira del arma se coloca automáticamente en el eje de la cámara, a AimDistance cm,
	// con el eje "adelante" del arma alineado con la vista. Así cualquier arma/animación queda centrada.
	/** Socket de mira del arma (si existe tiene prioridad sobre SightLocalOffset). */
	UPROPERTY(EditAnywhere, Category = "Rig|Aim") FName SightSocket = FName("Sight");
	/** Punto de mira en espacio local de la malla del arma (si no hay socket). */
	UPROPERTY(EditAnywhere, Category = "Rig|Aim") FVector SightLocalOffset = FVector(0.f, 0.f, 17.f);
	/** Eje "adelante" de la malla del arma (las armas de Epic apuntan a +Y). */
	UPROPERTY(EditAnywhere, Category = "Rig|Aim") FVector WeaponForwardAxis = FVector(0.f, 1.f, 0.f);
	/** Distancia ojo-mira en ADS (cm). */
	UPROPERTY(EditAnywhere, Category = "Rig|Aim") float AimDistance = 14.f;

	// ---- Poses del arma (las de cada arma, FBLWeaponPoses en su UBLWeaponData; por defecto las del AR-7) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig|Poses") FBLWeaponPoses Poses;

	// ---- Bob (balanceo al andar) ----
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float WalkStride = 150.f;   // cm por paso
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float SprintStride = 175.f;
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float CameraBobVertical = 0.35f;  // cm
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float CameraBobRoll = 0.25f;      // grados
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float WeaponBobVertical = 0.55f;
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float WeaponBobHorizontal = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float WeaponBobRoll = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float SprintBobMultiplier = 2.2f;
	UPROPERTY(EditAnywhere, Category = "Rig|Bob") float AimBobMultiplier = 0.2f;

	// ---- Sway (inercia al mirar) ----
	UPROPERTY(EditAnywhere, Category = "Rig|Sway") float SwayRotationAmount = 0.9f;   // grados por unidad de input
	UPROPERTY(EditAnywhere, Category = "Rig|Sway") float SwayLocationAmount = 0.25f;  // cm por unidad de input
	UPROPERTY(EditAnywhere, Category = "Rig|Sway") float SwayMaxRotation = 5.f;
	UPROPERTY(EditAnywhere, Category = "Rig|Sway") float SwayFrequency = 4.5f;
	UPROPERTY(EditAnywhere, Category = "Rig|Sway") float SwayDamping = 0.65f;
	UPROPERTY(EditAnywhere, Category = "Rig|Sway") float AimSwayMultiplier = 0.25f;

	// ---- Strafe ----
	UPROPERTY(EditAnywhere, Category = "Rig|Strafe") float CameraStrafeRoll = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Rig|Strafe") float WeaponStrafeRoll = 3.5f;

	// ---- Disparo: cámara ----
	/** Temblor de cámara por disparo (grados/s de impulso, aleatorio). */
	UPROPERTY(EditAnywhere, Category = "Rig|Shot") float ShotCameraShake = 9.f;
	/** Golpe de FOV por disparo (grados, se recupera solo). */
	UPROPERTY(EditAnywhere, Category = "Rig|Shot") float ShotFovPunch = 0.9f;

	// ---- Inercia, respiración y transiciones ----
	/** Retraso del arma con la aceleración (cm por cm/s²). */
	UPROPERTY(EditAnywhere, Category = "Rig|Inertia") float WeaponInertia = 0.0022f;
	UPROPERTY(EditAnywhere, Category = "Rig|Inertia") float WeaponInertiaRoll = 0.0018f;
	/** Respiración en reposo (cm / grados); en ADS se reduce a AimBreathMultiplier. */
	UPROPERTY(EditAnywhere, Category = "Rig|Breath") float BreathLocation = 0.18f;
	UPROPERTY(EditAnywhere, Category = "Rig|Breath") float BreathRotation = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Rig|Breath") float AimBreathMultiplier = 0.3f;
	/** Al subir/bajar el arma a ADS: giro y caída en mitad de la transición. */
	UPROPERTY(EditAnywhere, Category = "Rig|Aim") float AimTransitionRoll = -7.f;
	UPROPERTY(EditAnywhere, Category = "Rig|Aim") float AimTransitionDip = -1.5f;
	/** Inclinación de cámara durante la recarga (la cabeza acompaña al arma). */
	UPROPERTY(EditAnywhere, Category = "Rig|Poses") FRotator ReloadCameraTilt = FRotator(-2.f, 0.f, -2.5f);

	// ---- Saltos y aterrizajes ----
	UPROPERTY(EditAnywhere, Category = "Rig|Landing") float LandingDipScale = 0.012f;  // cm/s de impulso por cm/s de caída
	UPROPERTY(EditAnywhere, Category = "Rig|Landing") float LandingMinSpeed = 250.f;

private:
	TWeakObjectPtr<ABLCharacter> Character;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<USceneComponent> WeaponRoot;
	TWeakObjectPtr<USceneComponent> WeaponMesh;

	/** Transform de WeaponRoot (relativo a cámara) que centra la mira. Se suaviza para absorber el idle. */
	FTransform CalibratedAimTransform = FTransform::Identity;
	bool bHasCalibration = false;
	bool bCalibrationFrozen = false;
	float ReloadAlpha = 0.f;
	float EquipAlpha = 0.f;
	FVector ReloadDynLoc = FVector::ZeroVector;
	FRotator ReloadDynRot = FRotator::ZeroRotator;
	FBLSpringVector CameraShakeSpring;    // rotación rápida por disparo
	FBLSpringVector FovSpring;            // X = golpe de FOV
	FBLSpringVector WeaponInertiaSpring;  // posición por aceleración
	FBLSpringVector WeaponInertiaRotSpring;
	FVector LastLocalVelocity = FVector::ZeroVector;
	float BreathTime = 0.f;
	/** Punto de mira en espacio de WeaponRoot (último calculado). */
	FTransform SightInWeaponRoot = FTransform::Identity;

	void UpdateAimCalibration(float DeltaTime);

	float BobPhase = 0.f;
	float BobWeight = 0.f;
	float StrafeAlpha = 0.f;
	bool bLastStepLeft = false;

	FBLSpringVector CameraOffsetSpring;   // posición (dip de aterrizaje)
	FBLSpringVector CameraKickSpring;     // rotación (pitch, yaw, roll) de impulsos
	FBLSpringVector WeaponSwaySpring;     // rotación por sway
	FBLSpringVector WeaponSwayLocSpring;  // posición por sway
	FBLSpringVector WeaponKickLocSpring;  // posición por impulsos
	FBLSpringVector WeaponKickRotSpring;  // rotación por impulsos
};
