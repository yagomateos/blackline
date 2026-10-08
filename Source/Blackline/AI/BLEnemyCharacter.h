#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapons/BLWeaponOwner.h"
#include "BLEnemyCharacter.generated.h"

class UBLHealthComponent;
class UBLHitReactionComponent;
class UBLWeaponComponent;
class UAnimSequence;
class UAudioComponent;
class UBLSurfaceEffectsData;
struct FBLDamageInfo;

/**
 * Miliciano de la Columna Vesk (Bloque 5). Cuerpo Mannequin con uniforme propio (provisional, Bloque 8),
 * AR-7 con el mismo UBLWeaponComponent que el jugador, salud, reacción física y ragdoll.
 * Lo controla ABLAIController (percepción + estados); aquí solo está el "cuerpo": velocidad, agacharse,
 * hacia dónde apunta y la animación (UBLEnemyAnimInstance, en C++).
 */
UCLASS()
class BLACKLINE_API ABLEnemyCharacter : public ACharacter, public IBLWeaponOwner
{
	GENERATED_BODY()

public:
	ABLEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ---- Configuración en el nivel ----
	/** Escuadra (los de la misma escuadra se avisan entre sí aunque estén lejos). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy") FName SquadId;
	/** Ruta de patrulla (posiciones en el mundo). Vacía = centinela que vigila en su sitio. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy") TArray<FVector> PatrolPoints;
	UPROPERTY(EditAnywhere, Category = "Enemy|Movement") float WalkSpeed = 170.f;
	UPROPERTY(EditAnywhere, Category = "Enemy|Movement") float JogSpeed = 390.f;
	/** Voz de los barks (0-2). -1 = se elige según el nombre (cada miliciano suena distinto, siempre igual). */
	UPROPERTY(EditAnywhere, Category = "Enemy|Audio") int32 VoiceIndex = -1;
	/** Volumen de los pasos (andando; al trotar suenan más). Delatan al enemigo cercano. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Audio") float FootstepVolume = 0.75f;

	// ---- Órdenes del controlador ----
	/** Punto al que apunta el arma (con el error de puntería ya aplicado) y si está en guardia/apuntando. */
	void SetAim(const FVector& Point, bool bInAiming);
	void SetCrouchTarget(bool bCrouch) { bWantsCrouch = bCrouch; }
	void SetJog(bool bJog);

	// ---- Estado ----
	bool IsDead() const;
	bool IsAiming() const { return bAiming; }
	float GetCrouchAlpha() const { return CrouchAlpha; }
	FVector GetAimPoint() const { return AimPoint; }
	FVector GetEyeLocation() const;
	UBLHealthComponent* GetHealth() const { return Health; }
	UBLWeaponComponent* GetWeapon() const { return Weapon; }
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	UBLHitReactionComponent* GetHitReaction() const { return HitReaction; }
	/** Golpe de retroceso del último disparo (lo consume la animación). */
	float ConsumeFireKick() { const float K = FireKick; FireKick = 0.f; return K; }
	UAnimSequence* GetReloadAnim() const { return ReloadAnim; }
	float GetReloadTimeLeft() const { return ReloadTimeLeft; }
	float GetReloadDuration() const { return ReloadDuration; }
	int32 GetVoiceIndex() const { return VoiceIndex; }
	/** Frase que está diciendo (se corta si muere). */
	void SetVoiceComponent(UAudioComponent* Component);
	int32 GetFootstepsPlayed() const { return FootstepsPlayed; }

	// ---- IBLWeaponOwner ----
	virtual void GetWeaponAimView(FVector& OutOrigin, FVector& OutDirection) const override;
	virtual USkeletalMeshComponent* GetWeaponMeshComponent() const override { return WeaponMesh; }
	virtual bool IsWeaponBlocked() const override;
	virtual float GetWeaponAimAlpha() const override { return 0.6f; }
	virtual void OnWeaponEquipped(const UBLWeaponData* Data) override;
	virtual void OnWeaponFired(const UBLWeaponData* Data) override;
	virtual void OnWeaponReloadStarted(const UBLWeaponData* Data, float Duration, bool bEmpty) override;
	virtual void OnWeaponReloadEnded(bool bCompleted) override;

	// ---- Animaciones (Epic, fusil): las usa UBLEnemyAnimInstance ----
	UPROPERTY(EditAnywhere, Category = "Animation") TObjectPtr<UAnimSequence> IdleAnim;
	/** 8 direcciones: Fwd, Fwd_Right, Right, Bwd_Right, Bwd, Bwd_Left, Left, Fwd_Left. */
	UPROPERTY(EditAnywhere, Category = "Animation") TArray<TObjectPtr<UAnimSequence>> WalkAnims;
	UPROPERTY(EditAnywhere, Category = "Animation") TArray<TObjectPtr<UAnimSequence>> JogAnims;
	UPROPERTY(EditAnywhere, Category = "Animation") TObjectPtr<UAnimSequence> ReloadAnim;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UBLHealthComponent> Health;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UBLHitReactionComponent> HitReaction;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UBLWeaponComponent> Weapon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	/** Equipo (Bloque 8): piezas rígidas enganchadas a huesos (gen_militia_gear.py). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UStaticMeshComponent> Vest;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UStaticMeshComponent> Helmet;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UStaticMeshComponent> Armband;

private:
	UFUNCTION() void HandleDeath(const FBLDamageInfo& Info);
	void TickFootsteps(float DeltaTime);
	/** Coloca una pieza modelada en el espacio de la malla (pose de referencia) sobre su hueso. */
	void FitGearToBone(UStaticMeshComponent* Piece, FName Bone);
	/** Sonido de la superficie bajo el personaje (pasos, caída del cuerpo). */
	void PlaySurfaceSound(float Volume, float Pitch);

	UPROPERTY() TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;
	TWeakObjectPtr<UAudioComponent> VoiceComponent;
	float StepDistance = 0.f;
	int32 FootstepsPlayed = 0;

	FVector AimPoint = FVector::ZeroVector;
	bool bAiming = false;
	bool bWantsCrouch = false;
	float CrouchAlpha = 0.f;
	float FireKick = 0.f;
	float ReloadTimeLeft = 0.f;
	float ReloadDuration = 0.f;
};
