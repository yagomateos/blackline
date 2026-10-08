#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/BLDamageTypes.h"
#include "BLWeaponComponent.generated.h"

class UBLWeaponData;
class UBLWeaponFXComponent;
class IBLWeaponOwner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBLWeaponShotSignature, const FHitResult&, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBLWeaponEventSignature);
/** Impacto confirmado en algo con salud (hitmarker): zona, daño aplicado y si lo mató. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBLHitConfirmSignature, EBLHitZone, Zone, float, Damage, bool, bKilled);

/** Arma en el inventario con su munición. */
USTRUCT(BlueprintType)
struct FBLWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UBLWeaponData> Data;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Magazine = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Reserve = 0;
};

/**
 * Lógica de armas: inventario, disparo hitscan, cadencia, munición, recarga, dispersión y retroceso.
 * Válido para el jugador y para la IA: el portador implementa IBLWeaponOwner.
 * Los efectos (fogonazo, casquillos, impactos, sonido) los reproduce UBLWeaponFXComponent.
 */
UCLASS(ClassGroup = (Blackline), meta = (BlueprintSpawnableComponent))
class BLACKLINE_API UBLWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLWeaponComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Añade un arma al inventario y la equipa si no hay ninguna. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AddWeapon(UBLWeaponData* Data, bool bEquip = true);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipSlot(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetTriggerHeld(bool bHeld);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool StartReload();

	/** Cancela la recarga; si la munición ya entró se conserva. */
	void CancelReload();

	/** Copia del inventario (checkpoints) y restauración: cancela acciones y vuelve a equipar el arma activa. */
	TArray<FBLWeaponSlot> GetInventorySnapshot() const { return Inventory; }
	int32 GetCurrentIndex() const { return CurrentIndex; }
	void RestoreInventory(const TArray<FBLWeaponSlot>& Snapshot, int32 EquipIndex);

	// ---- Estado ----
	const UBLWeaponData* GetWeaponData() const;
	int32 GetMagazine() const;
	int32 GetReserve() const;
	bool IsReloading() const { return bReloading; }
	bool IsEquipping() const { return EquipRemaining > 0.f; }
	bool IsReloadEmpty() const { return bReloadEmpty; }
	/** 1 al empezar a equipar .. 0 al terminar. */
	float GetEquipFraction() const;
	bool IsTriggerHeld() const { return bTriggerHeld; }
	/** Semiángulo actual del cono de dispersión (grados), para la mira del HUD. */
	float GetCurrentSpread() const;
	int32 GetShotsFired() const { return ShotsFired; }
	float GetReloadProgress() const { return bReloading && ReloadDuration > 0.f ? ReloadElapsed / ReloadDuration : 0.f; }

	/** Cada disparo (con el resultado del trazado; bBlockingHit=false si no tocó nada). */
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponShotSignature OnShot;
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponEventSignature OnReloadFinished;
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponEventSignature OnDryFire;
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLHitConfirmSignature OnHitConfirmed;

	/** Armas con las que empieza (las añade en BeginPlay). */
	UPROPERTY(EditAnywhere, Category = "Weapon") TArray<TObjectPtr<UBLWeaponData>> StartingWeapons;

	/** Munición infinita en la reserva (pruebas). */
	UPROPERTY(EditAnywhere, Category = "Weapon|Debug") bool bInfiniteReserve = false;

private:
	IBLWeaponOwner* GetWeaponOwner() const;
	FBLWeaponSlot* CurrentSlot();
	const FBLWeaponSlot* CurrentSlot() const;
	bool CanFireNow() const;
	void FireShot();
	void UpdateReload(float DeltaTime);
	void UpdateRecoil(float DeltaTime);
	void FinishReload();
	float ComputeDamage(const FHitResult& Hit, float Distance) const;

	UPROPERTY(VisibleAnywhere, Category = "Weapon") TArray<FBLWeaponSlot> Inventory;
	int32 CurrentIndex = INDEX_NONE;

	UPROPERTY() TObjectPtr<UBLWeaponFXComponent> FX;

	bool bTriggerHeld = false;
	bool bSemiLatch = false;       // semi: hay que soltar el gatillo entre disparos
	bool bDryFiredThisPress = false;
	float ShotTimer = 0.f;         // >0 = aún no se puede disparar el siguiente
	float UnblockedTime = 0.f;     // tiempo desde que el portador dejó de estar bloqueado (sprint)
	float EquipRemaining = 0.f;
	int32 ShotsInBurst = 0;
	int32 ShotsFired = 0;
	float Bloom = 0.f;

	bool bReloading = false;
	bool bReloadEmpty = false;
	bool bAmmoInserted = false;
	float ReloadElapsed = 0.f;
	float ReloadDuration = 0.f;
	int32 NextReloadSound = 0;

	// Retroceso de la mira (grados): pendiente de aplicar y recuperable
	FVector2D PendingRecoil = FVector2D::ZeroVector;   // (pitch, yaw)
	float RecoverablePitch = 0.f;
	float TimeSinceShot = 100.f;
	float LastControlPitch = 0.f;
	bool bHasLastControlPitch = false;
};
