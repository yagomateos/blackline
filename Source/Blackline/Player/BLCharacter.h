#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapons/BLWeaponOwner.h"
#include "Perception/AISightTargetInterface.h"
#include "Weapons/BLWeaponComponent.h"
#include "Combat/BLHealthComponent.h"
#include "BLCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputAction;
class UBLFirstPersonRigComponent;
class UBLFirstPersonAnimInstance;
class UBLWeaponComponent;
class UBLWeaponData;
class UBLSurfaceEffectsData;
class USoundBase;
class UAudioComponent;
class ABLInteractable;
struct FInputActionValue;

/**
 * Personaje jugable en primera persona.
 *
 * Jerarquía de componentes:
 *   Capsule
 *    ├─ CameraRoot (altura de ojos suavizada + pitch del control + inclinación)
 *    │   └─ Camera (bob / impulsos de cámara, lo mueve BLFirstPersonRigComponent)
 *    │       ├─ WeaponRoot (sway / bob / poses / ADS) ─ WeaponMesh
 *    │       └─ FirstPersonMesh (Mannequin, solo lo ve el jugador; las manos siguen al arma por IK)
 *    └─ Mesh (cuerpo completo para el mundo; para el jugador solo proyecta sombra)
 *
 * Las acciones (DoMove, DoJump, SetSprintHeld...) son públicas para que las use el input,
 * la UI y el piloto automático de pruebas (UBLAutoTestComponent).
 */
UCLASS()
class BLACKLINE_API ABLCharacter : public ACharacter, public IBLWeaponOwner, public IAISightTargetInterface
{
	GENERATED_BODY()

public:
	ABLCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnJumped_Implementation() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// ---- Acciones ----
	void DoMove(float Right, float Forward);
	void DoLook(float Yaw, float Pitch);
	/** Salta, o encarama si hay un obstáculo delante a altura válida. */
	void DoJumpOrMantle();
	void DoStopJump();
	void SetSprintHeld(bool bHeld);
	void ToggleCrouch();
	void SetLeanLeftHeld(bool bHeld);
	void SetLeanRightHeld(bool bHeld);
	void SetAimHeld(bool bHeld);
	void SetFireHeld(bool bHeld);
	void DoReload();
	/** Mantener para usar el objeto enfocado (F); con la misión completada, reinicia. */
	void SetInteractHeld(bool bHeld);

	// ---- IBLWeaponOwner ----
	virtual void GetWeaponAimView(FVector& OutOrigin, FVector& OutDirection) const override;
	virtual USkeletalMeshComponent* GetWeaponMeshComponent() const override { return WeaponMesh; }
	virtual bool IsWeaponBlocked() const override;
	virtual float GetWeaponAimAlpha() const override { return AimAlpha; }
	virtual void OnWeaponTriggerPressed() override;
	virtual void OnWeaponEquipped(const UBLWeaponData* Data) override;
	virtual void OnWeaponFired(const UBLWeaponData* Data) override;
	virtual void OnWeaponReloadStarted(const UBLWeaponData* Data, float Duration, bool bEmpty) override;
	virtual void OnWeaponReloadEnded(bool bCompleted) override;

	// ---- Estado (lo consulta el rig de cámara, la UI y las pruebas) ----
	bool IsSprinting() const { return bIsSprinting; }
	bool IsAiming() const { return bIsAiming; }
	bool IsMantling() const { return bIsMantling; }
	float GetLeanAlpha() const { return LeanAlpha; }        // -1 izquierda .. 1 derecha
	float GetAimAlpha() const { return AimAlpha; }          // 0 cadera .. 1 ADS
	float GetSprintAlpha() const { return SprintAlpha; }
	float GetMantleAlpha() const { return MantleAlpha; }
	float GetEyeHeightFromFloor() const { return SmoothedEyeHeight; }
	FVector2D GetMoveInput() const { return LastMoveInput; }
	/** Delta de mirada acumulado en el frame actual (para el sway del arma). */
	FVector2D GetLookDeltaThisFrame() const { return LookDeltaThisFrame; }

	UCameraComponent* GetCamera() const { return Camera; }
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	UBLFirstPersonRigComponent* GetFirstPersonRig() const { return FirstPersonRig; }
	UBLWeaponComponent* GetWeapon() const { return Weapon; }
	UBLFirstPersonAnimInstance* GetFirstPersonAnim() const;

	/** Vista de la IA: visible si se ve la cabeza, el torso o la cadera (no solo el centro de la cápsula). */
	virtual UAISense_Sight::EVisibilityResult CanBeSeenFrom(const FCanBeSeenFromContext& Context, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed,
		int32& OutNumberOfAsyncLosCheckRequested, float& OutSightStrength, int32* UserData = nullptr, const FOnPendingVisibilityQueryProcessedDelegate* Delegate = nullptr) override;

	// ---- Salud, daño y muerte (BLCharacterCombat.cpp) ----
	UBLHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const { return bDead; }
	float GetDeathTime() const { return DeathTime; }
	/** Reaparición (la llama UBLCheckpointSubsystem): salud llena, inventario guardado, cámara de pie. */
	void RespawnAt(const FTransform& Transform, const TArray<FBLWeaponSlot>& Inventory, int32 WeaponIndex);

	/** Indicador direccional de daño (HUD): origen del daño en el mundo, momento y fuerza 0..1. */
	struct FDamageIndicator
	{
		FVector Source = FVector::ZeroVector;
		float Time = 0.f;
		float Strength = 1.f;
	};
	const TArray<FDamageIndicator>& GetDamageIndicators() const { return DamageIndicators; }
	/** Hitmarker (HUD): segundos desde el último impacto confirmado. */
	float GetHitMarkerAge() const;
	bool IsHitMarkerKill() const { return bHitMarkerKill; }
	EBLHitZone GetHitMarkerZone() const { return HitMarkerZone; }
	// ---- Interacción (BLCharacterInteraction.cpp) ----
	ABLInteractable* GetFocusedInteractable() const;
	float GetInteractProgress() const { return InteractProgress; }

	/** Destello rojo del último daño recibido (0..1). */
	float GetDamageFlash() const { return DamageFlash; }

protected:
	// ---- Componentes ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> CameraRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBLFirstPersonRigComponent> FirstPersonRig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBLWeaponComponent> Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBLHealthComponent> Health;

	// ---- Input ----
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> MouseLookAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> LeanLeftAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> LeanRightAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> AimAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> FireAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> InteractAction;

	/** Multiplicador de sensibilidad (se expondrá en Opciones). */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.05"))
	float LookSensitivity = 1.0f;

	// ---- Movimiento ----
	UPROPERTY(EditAnywhere, Category = "Movement") float WalkSpeed = 420.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float SprintSpeed = 660.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float CrouchSpeed = 210.f;
	/** Multiplicador de velocidad mientras se apunta (ADS). */
	UPROPERTY(EditAnywhere, Category = "Movement") float AimSpeedMultiplier = 0.6f;
	/** El sprint solo se activa si el input hacia delante supera este valor. */
	UPROPERTY(EditAnywhere, Category = "Movement") float SprintForwardThreshold = 0.5f;

	/** Altura de ojos sobre el suelo (cm). */
	UPROPERTY(EditAnywhere, Category = "Movement|Stance") float EyeHeightStanding = 164.f;
	UPROPERTY(EditAnywhere, Category = "Movement|Stance") float EyeHeightCrouched = 104.f;
	UPROPERTY(EditAnywhere, Category = "Movement|Stance") float EyeHeightInterpSpeed = 12.f;

	// ---- Inclinación (lean) ----
	UPROPERTY(EditAnywhere, Category = "Lean") float LeanOffset = 32.f;     // cm laterales
	UPROPERTY(EditAnywhere, Category = "Lean") float LeanRoll = 11.f;       // grados
	UPROPERTY(EditAnywhere, Category = "Lean") float LeanInterpSpeed = 9.f;
	UPROPERTY(EditAnywhere, Category = "Lean") float LeanProbeRadius = 14.f;

	// ---- ADS ----
	UPROPERTY(EditAnywhere, Category = "Aim") float AimInterpSpeed = 14.f;

	// ---- Mantle (encaramarse) ----
	UPROPERTY(EditAnywhere, Category = "Mantle") float MantleMinHeight = 45.f;
	UPROPERTY(EditAnywhere, Category = "Mantle") float MantleMaxHeight = 165.f;
	/** Distancia máxima a la pared, desde el borde de la cápsula. */
	UPROPERTY(EditAnywhere, Category = "Mantle") float MantleReach = 55.f;
	UPROPERTY(EditAnywhere, Category = "Mantle") float MantleDurationLow = 0.38f;
	UPROPERTY(EditAnywhere, Category = "Mantle") float MantleDurationHigh = 0.62f;

	// ---- Retroceso visual ----
	/** Variación aleatoria del kick lateral del arma (±, fracción del valor del arma). */
	UPROPERTY(EditAnywhere, Category = "Weapon") float WeaponKickRandomness = 0.6f;

	// ---- Sonido del personaje ----
	/** Pasos y casquillos por superficie (DA_SurfaceEffects). */
	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;
	/** Roce de ropa y equipo (sprint, ADS, saltos). */
	UPROPERTY(EditAnywhere, Category = "Sound") TArray<TObjectPtr<USoundBase>> GearSounds;
	UPROPERTY(EditAnywhere, Category = "Sound") float FootstepVolume = 0.55f;

	UFUNCTION() void HandleFootstep(bool bLeftFoot, float Intensity);
	void PlayFootstep(float Volume);
	void PlayGear(float Volume);

	// ---- Daño y muerte ----
	/** Segundos desde la muerte hasta reaparecer en el checkpoint. */
	UPROPERTY(EditAnywhere, Category = "Health") float RespawnDelay = 4.f;
	/** Viñeta del post-proceso del nivel (la de daño se suma a esta). */
	UPROPERTY(EditAnywhere, Category = "Health") float BaseVignette = 0.45f;
	/** Impulso de cámara por cada 25 de daño recibido (grados/s). */
	UPROPERTY(EditAnywhere, Category = "Health") FRotator DamageCameraKick = FRotator(10.f, 6.f, 12.f);
	UPROPERTY(EditAnywhere, Category = "Sound") TArray<TObjectPtr<USoundBase>> HurtSounds;
	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<USoundBase> HitmarkerSound;
	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<USoundBase> HitmarkerKillSound;
	/** Latido en bucle con salud baja. */
	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<USoundBase> HeartbeatSound;

	// ---- Animación del cuerpo para el mundo (provisional: idle en bucle) ----
	UPROPERTY(EditAnywhere, Category = "Animation") TObjectPtr<UAnimationAsset> BodyIdleAnim;

private:
	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);

	void UpdateSprint(float DeltaTime);
	void UpdateStance(float DeltaTime);
	void UpdateLean(float DeltaTime);
	void UpdateAim(float DeltaTime);
	void UpdateMantle(float DeltaTime);
	void UpdateCameraRoot();
	void UpdateProceduralWeaponActions(float DeltaTime);

	// Interacción
	void UpdateInteraction(float DeltaTime);
	TWeakObjectPtr<ABLInteractable> FocusedInteractable;
	float InteractProgress = 0.f;
	bool bInteractHeld = false;

	// Combate (BLCharacterCombat.cpp)
	void InitCombat();
	void UpdateCombat(float DeltaTime);
	void ApplyDeathPose(float DeltaTime);
	UFUNCTION() void HandleDamaged(const FBLDamageInfo& Info);
	UFUNCTION() void HandleDeath(const FBLDamageInfo& Info);
	UFUNCTION() void HandleHitConfirmed(EBLHitZone Zone, float Damage, bool bKilled);

	bool bDead = false;
	float DeathTime = 0.f;
	float DeathRollSign = 1.f;
	bool bDeathFadeStarted = false;
	bool bRespawnRequested = false;
	float DamageFlash = 0.f;
	TArray<FDamageIndicator> DamageIndicators;
	float HitMarkerTime = -100.f;
	bool bHitMarkerKill = false;
	EBLHitZone HitMarkerZone = EBLHitZone::None;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> HeartbeatAudio;
	bool TryStartMantle();
	bool CanSprint() const;

	FVector2D LastMoveInput = FVector2D::ZeroVector;
	FVector2D PendingMoveInput = FVector2D::ZeroVector;
	FVector2D LookDeltaThisFrame = FVector2D::ZeroVector;

	bool bSprintHeld = false;
	bool bIsSprinting = false;
	float SprintAlpha = 0.f;

	bool bAimHeld = false;
	bool bIsAiming = false;
	float AimAlpha = 0.f;

	bool bLeanLeftHeld = false;
	bool bLeanRightHeld = false;
	float LeanAlpha = 0.f;

	float SmoothedEyeHeight = 164.f;

	float LastReloadProgress = 0.f;
	bool bWasSprinting = false;
	bool bWasAiming = false;

	/** Cargador en la mano izquierda durante la recarga (copia de la malla del cargador). */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> HandMagazine;
	void SetMagazineInHand(bool bInHand);
	bool bIsMantling = false;
	float MantleAlpha = 0.f;
	float MantleTime = 0.f;
	float MantleDuration = 0.5f;
	FVector MantleStart = FVector::ZeroVector;
	FVector MantleMid = FVector::ZeroVector;
	FVector MantleEnd = FVector::ZeroVector;
};
