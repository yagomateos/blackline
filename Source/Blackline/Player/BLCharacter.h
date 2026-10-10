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
class ABLMountedGun;
class ABLBoat;
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
	/** Cambio de arma (1 = principal, 2 = secundaria; rueda / Y = siguiente). BLCharacterWeapons.cpp */
	void SwitchWeapon(int32 Index);
	void CycleWeapon();
	bool CanSwitchWeapon() const;

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

	// ---- Arma montada (misión 4): BLCharacterMount.cpp ----
	void MountGun(ABLMountedGun* Gun);
	void DismountGun();
	ABLMountedGun* GetMountedGun() const { return MountedGun.Get(); }
	bool IsMounted() const { return MountedGun.IsValid(); }

	// ---- Lancha (misión 2): el jugador la pilota de pie junto a la consola ----
	void BoardBoat(ABLBoat* Boat);
	/** Arma principal según EQUIPAMIENTO (o la que imponga la misión); antes de crear el inventario. */
	void ApplyLoadout();
	void LeaveBoat();
	bool IsDrivingBoat() const { return DrivenBoat.IsValid(); }
	ABLBoat* GetDrivenBoat() const { return DrivenBoat.Get(); }
	/** Pasajero (el helicóptero de extracción): dentro del vehículo, sin andar ni disparar; puede mirar. */
	void EnterVehicleSeat(USceneComponent* Seat, float LookYaw);
	bool IsInVehicleSeat() const { return VehicleSeat.IsValid(); }
	/** Montado en un arma, pilotando o de pasajero: no anda, no salta y no usa el arma propia. */
	bool IsInVehicleOrMount() const { return IsMounted() || IsDrivingBoat() || IsInVehicleSeat(); }
	bool IsFireBlockedUntilRelease() const { return bFireNeedsRelease; }
	void MountGunTrigger(bool bHeld);
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
	/** Opciones > Controles. */
	void SetLookOptions(float Sensitivity, bool bInvertY) { LookSensitivity = Sensitivity; bInvertLookY = bInvertY; }
	/** Lanzar granada (G / RB): quita la anilla, baja el arma y la lanza a los 0,35 s. */
	void ThrowGrenade();
	int32 GetGrenades() const { return Grenades; }
	int32 GetMaxGrenades() const { return MaxGrenades; }
	void SetGrenades(int32 N) { Grenades = FMath::Clamp(N, 0, MaxGrenades); }
	bool IsThrowingGrenade() const { return GrenadeTimer >= 0.f; }
	class ABLGrenade* GetLastGrenade() const { return LastGrenade.Get(); }
	/** Una explosión cercana sacude la cámara y el arma (Strength 0..1). */
	void OnNearbyExplosion(const FVector& Location, float Strength);
	UBLWeaponComponent* GetWeapon() const { return Weapon; }
	UBLFirstPersonAnimInstance* GetFirstPersonAnim() const;

	/** Vista de la IA: visible si se ve la cabeza, el torso o la cadera (no solo el centro de la cápsula). */
	virtual UAISense_Sight::EVisibilityResult CanBeSeenFrom(const FCanBeSeenFromContext& Context, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed,
		int32& OutNumberOfAsyncLosCheckRequested, float& OutSightStrength, int32* UserData = nullptr, const FOnPendingVisibilityQueryProcessedDelegate* Delegate = nullptr) override;

	// ---- Salud, daño y muerte (BLCharacterCombat.cpp) ----
	UBLHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const { return bDead; }
	float GetDeathTime() const { return DeathTime; }
	/** Daño del entorno (caída, agua): no lo reduce la dificultad (DamageTakenMultiplier). bLethal mata siempre. */
	void ApplyEnvironmentDamage(float Amount, bool bLethal, const FVector& Source);
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
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> GrenadeAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> SwapWeaponAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> PrimaryWeaponAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> SecondaryWeaponAction;

	// ---- Granada M-6 (Bloque 11) ----
	UPROPERTY(EditAnywhere, Category = "Grenade") int32 Grenades = 2;
	UPROPERTY(EditAnywhere, Category = "Grenade") int32 MaxGrenades = 3;
	UPROPERTY(EditAnywhere, Category = "Grenade") float GrenadeThrowSpeed = 1450.f;
	UPROPERTY(EditAnywhere, Category = "Grenade") float GrenadeFuse = 3.5f;
	UPROPERTY(EditAnywhere, Category = "Grenade") TObjectPtr<USoundBase> GrenadePinSound;
	UPROPERTY(EditAnywhere, Category = "Grenade") TObjectPtr<USoundBase> GrenadeThrowSound;

	/** Multiplicador de sensibilidad (se expondrá en Opciones). */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.05"))
	float LookSensitivity = 1.0f;
	/** Invertir el eje vertical (Opciones > Controles). */
	UPROPERTY(EditAnywhere, Category = "Input") bool bInvertLookY = false;

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
	/** Caída: sin daño por debajo de esta velocidad al tocar el suelo (cm/s; 1000 ≈ 5 m) y mortal desde FallLethalSpeed (≈ 15 m). */
	UPROPERTY(EditAnywhere, Category = "Health") float FallDamageMinSpeed = 1000.f;
	UPROPERTY(EditAnywhere, Category = "Health") float FallLethalSpeed = 1750.f;
	/** Cayendo más de esto sin tocar suelo (fuera del mapa, vacío) = muerte. */
	UPROPERTY(EditAnywhere, Category = "Health") float FallOutOfWorldTime = 3.5f;
	/** Agua profunda (mallas con M_Env_Water): con la cabeza por debajo de la superficie, muerte (ahogado). */
	TArray<FBox> WaterBoxes;
	float FallTime = 0.f;
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
	/** Recarga de pistola (BLCharacterWeapons.cpp): rellena la trayectoria de la mano y el movimiento del arma. */
	void UpdatePistolReload(const class UBLWeaponData* Data, float P, TArray<TPair<float, FVector>, TInlineAllocator<16>>& Keys, FVector& DynLoc, FRotator& DynRot);
	/** Cargador nuevo en la mano izquierda, colocado como quedará al meterlo en el puño. */
	void TakeNewMagazine();
	using FHandKeys = TArray<TPair<float, FVector>, TInlineAllocator<16>>;
	/** Trayectoria de la mano por puntos clave (Catmull-Rom: continua, sin paradas); Default fuera de los puntos. */
	static FVector EvalHandPath(const FHandKeys& Keys, float P, const FVector& Default);
	/** Recarga de escopeta cartucho a cartucho (BLCharacterWeapons.cpp): pose, mano izquierda y movimiento del arma. */
	void UpdateShellReload(const class UBLWeaponData* Data, float DeltaTime, float& ReloadPose, FVector& HandTarget, FVector& DynLoc, FRotator& DynRot);
	/** Desplazamiento actual del cerrojo / guardamanos (espacio de la malla del arma). */
	FVector GetWeaponBoltOffset() const;
	/** Cartucho en la mano izquierda (malla MagazineMesh del arma) en un punto del espacio del arma. */
	void SetShellInHand(const class UBLWeaponData* Data, bool bVisible, const FVector& LocationInWeapon = FVector::ZeroVector);
	/** Posición en la línea de tiempo de la recarga de cartuchos (fase + fracción) del frame anterior. */
	float LastShellPos = 0.f;
	FVector ShellHand = FVector::ZeroVector;

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

	/** Cargador (o cartucho) en la mano izquierda durante la recarga (copia de la malla del cargador). */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> HandMagazine;
	UStaticMeshComponent* EnsureHandMagazine();
	void SetMagazineInHand(bool bInHand);
	bool bIsMantling = false;
	float MantleAlpha = 0.f;
	float MantleTime = 0.f;
	float MantleDuration = 0.5f;
	FVector MantleStart = FVector::ZeroVector;
	FVector MantleMid = FVector::ZeroVector;
	FVector MantleEnd = FVector::ZeroVector;
	// Granada
	void TickGrenade(float DeltaTime);
	float GrenadeTimer = -1.f;
	bool bGrenadeReleased = false;
	TWeakObjectPtr<class ABLGrenade> LastGrenade;

	TWeakObjectPtr<ABLMountedGun> MountedGun;
	TWeakObjectPtr<ABLBoat> DrivenBoat;
	TWeakObjectPtr<USceneComponent> VehicleSeat;
	/** Tras montar/desmontar un arma: la pulsación de disparo en curso no cuenta hasta soltarla. */
	bool bFireNeedsRelease = false;
	bool bFireInputHeld = false;
	FVector2D SavedViewPitch = FVector2D(-89.9f, 89.9f);
	FVector2D SavedViewYaw = FVector2D(0.f, 359.999f);
};
