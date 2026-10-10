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

/** Fase de la recarga cartucho a cartucho (EBLReloadStyle::Shells). */
enum class EBLShellReloadPhase : uint8
{
	None,
	Raise,      // el arma gira para enseñar la portilla de carga
	PortLoad,   // en vacío: un cartucho por la ventana de expulsión y se cierra la corredera
	Shell,      // un cartucho al depósito
	Lower       // vuelve a la posición de tiro
};

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

	/** Equipa al instante (sube el arma desde abajo). Para cambiar de arma jugando, SwitchToSlot. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipSlot(int32 Index);

	/** Cambio de arma: baja la actual (HolsterTime), cambia y sube la nueva (EquipTime). Cancela la recarga
	 *  (la munición que ya entró se conserva). Volver a pedir la actual a medio bajar la sube de nuevo. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool SwitchToSlot(int32 Index);
	/** Siguiente arma del inventario (rueda / botón Y). */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CycleWeapon();
	bool IsSwitching() const { return PendingSlot != INDEX_NONE; }
	int32 GetPendingSlot() const { return PendingSlot; }
	int32 GetWeaponCount() const { return Inventory.Num(); }
	const FBLWeaponSlot* GetSlot(int32 Index) const { return Inventory.IsValidIndex(Index) ? &Inventory[Index] : nullptr; }
	/** Hueco del inventario con esa arma (INDEX_NONE si no la lleva). */
	int32 FindWeapon(const UBLWeaponData* Data) const;
	/** Arma recogida del suelo: sustituye el hueco ReplaceIndex (la sube al instante) o, con INDEX_NONE, se añade y se
	 *  cambia a ella. Devuelve su hueco. */
	int32 PickUpWeapon(const FBLWeaponSlot& NewSlot, int32 ReplaceIndex);
	/** Munición a la reserva de un arma del inventario (hasta MaxReserveAmmo). Devuelve la que cabe. */
	int32 AddReserveAmmo(int32 Index, int32 Amount);

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
	/** Subiendo el arma o bajándola para cambiar: no se puede disparar ni recargar. */
	bool IsEquipping() const { return EquipRemaining > 0.f || PendingSlot != INDEX_NONE; }
	bool IsReloadEmpty() const { return bReloadEmpty; }
	/** Cuánto está bajada el arma: 1 al empezar a equipar .. 0 al terminar (y 0 -> 1 al bajarla para cambiar). */
	float GetEquipFraction() const;
	bool IsTriggerHeld() const { return bTriggerHeld; }
	/** Semiángulo actual del cono de dispersión (grados), para la mira del HUD. */
	float GetCurrentSpread() const;
	int32 GetShotsFired() const { return ShotsFired; }
	/** Perdigones (o la bala) del último disparo que dieron en algo. */
	int32 GetLastPelletHits() const { return LastPelletHits; }
	float GetReloadProgress() const { return bReloading && ReloadDuration > 0.f ? ReloadElapsed / ReloadDuration : 0.f; }
	/** Recarga cartucho a cartucho: fase actual, fracción dentro de ella (0..1) y cartucho (0 = el primero al depósito). */
	EBLShellReloadPhase GetShellReloadPhase(float& OutAlpha, int32& OutShell) const;
	/** Cartuchos que faltan por meter en esta recarga (incluido el que está en la mano). */
	int32 GetShellsRemaining() const { return bReloading ? ShellCount - ShellsDone : 0; }
	/** La acción queda abierta (cerrojo atrás / corredera atrás) mientras dura la recarga en vacío hasta cerrarla. */
	bool IsReloadActionOpen() const;

	/** Cada disparo (con el resultado del trazado; bBlockingHit=false si no tocó nada). */
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponShotSignature OnShot;
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponEventSignature OnReloadFinished;
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponEventSignature OnDryFire;
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLHitConfirmSignature OnHitConfirmed;
	/** Arma equipada tras un cambio (índice del inventario). */
	UPROPERTY(BlueprintAssignable, Category = "Weapon") FBLWeaponEventSignature OnWeaponSwitched;

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
	void UpdateShellReload(const UBLWeaponData* Data, FBLWeaponSlot& Slot);
	/** Disparo que interrumpe la recarga de cartuchos: deja de meter y vuelve a la posición de tiro. */
	void InterruptShellReload();
	float GetShellPhaseStart(int32 Shell) const;
	void UpdateCycle(float DeltaTime, const UBLWeaponData* Data);
	float GetPelletCone() const;
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
	int32 PendingSlot = INDEX_NONE;
	float HolsterRemaining = 0.f;
	float HolsterDuration = 0.f;
	int32 ShotsInBurst = 0;
	int32 ShotsFired = 0;
	int32 LastPelletHits = 0;
	float Bloom = 0.f;

	bool bReloading = false;
	bool bReloadEmpty = false;
	bool bAmmoInserted = false;
	float ReloadElapsed = 0.f;
	float ReloadDuration = 0.f;
	int32 NextReloadSound = 0;
	// Cartucho a cartucho: cartuchos de esta recarga (el primero por la ventana si estaba vacía) y metidos ya
	int32 ShellCount = 0;
	int32 ShellsDone = 0;
	bool bPortLoad = false;
	// Corredera: bombeo en curso tras un disparo (<0 = ninguno)
	float CycleElapsed = -1.f;
	// Recarga pedida durante el bombeo: empieza en cuanto termina (si no se dispara antes)
	bool bReloadQueued = false;
	int32 NextCycleSound = 0;
	bool bCasingPending = false;
	bool bTriggerPressedDuringReload = false;

	// Retroceso de la mira (grados): pendiente de aplicar y recuperable
	FVector2D PendingRecoil = FVector2D::ZeroVector;   // (pitch, yaw)
	float RecoverablePitch = 0.f;
	float TimeSinceShot = 100.f;
	float LastControlPitch = 0.f;
	bool bHasLastControlPitch = false;
};
