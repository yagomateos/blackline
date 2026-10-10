#pragma once

#include "CoreMinimal.h"
#include "Mission/BLInteractable.h"
#include "BLMountedGun.generated.h"

class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;
class UBLSurfaceEffectsData;

/**
 * Ametralladora pesada montada (misión 4, búnker de la cabeza de puente). F para montarla y otra vez F para bajarse.
 * Montado, el jugador no se mueve, se agacha detrás del arma con los ojos en su línea de mira (GetSightHeight), la mirada
 * queda limitada al arco del afuste (YawLimit/Pitch) y el cañón apunta al punto que marca la mira (trazado desde la
 * cámara hasta Range). Cada bala sale de la boca del cañón hacia ese punto (lo que se ve es por donde va), con
 * dispersión, daño, impactos por material, fogonazo, retroceso y **recalentamiento** (bloqueo de OverheatLock s).
 * El actor (Mesh = trípode) mira hacia +X; Gun es el cuerpo del arma, con pivote en el muñón.
 */
UCLASS()
class BLACKLINE_API ABLMountedGun : public ABLInteractable
{
	GENERATED_BODY()

public:
	ABLMountedGun();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Use(ABLCharacter* InUser) override;
	virtual bool CanInteract(const ABLCharacter* InUser) const override;
	virtual FVector GetInteractLocation() const override;

	UPROPERTY(EditAnywhere, Category = "Gun") float YawLimit = 55.f;
	/** Arco vertical: lo que deja ver la tronera del búnker de M4 con los ojos en la línea de mira (-7°..+9°). */
	UPROPERTY(EditAnywhere, Category = "Gun") float PitchMin = -7.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float PitchMax = 8.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float RoundsPerMinute = 620.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float Damage = 42.f;
	/** Dispersión en frío; con el arma caliente sube hasta el doble. 0,3° = ~50 cm a 100 m. */
	UPROPERTY(EditAnywhere, Category = "Gun") float SpreadDegrees = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Gun") float Range = 30000.f;
	/** Altura de la línea de mira sobre el muñón: ahí quedan los ojos del tirador. */
	UPROPERTY(EditAnywhere, Category = "Gun") float SightHeight = 16.f;
	/** ~10 disparos/s: se recalienta con ~9 s de fuego continuo; a ráfagas cortas no llega (enfría 0,3/s, un 30 % mientras dispara). */
	UPROPERTY(EditAnywhere, Category = "Gun") float HeatPerShot = 0.02f;
	UPROPERTY(EditAnywhere, Category = "Gun") float CoolRate = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Gun") float OverheatLock = 2.5f;
	/** Dónde se coloca el tirador (detrás del arma) y altura del muñón. */
	UPROPERTY(EditAnywhere, Category = "Gun") FVector SeatOffset = FVector(-115.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Gun") float PivotHeight = 118.f;
	UPROPERTY(EditAnywhere, Category = "Gun") FVector MuzzleOffset = FVector(150.f, 0.f, 6.f);
	UPROPERTY(EditAnywhere, Category = "Gun") TArray<TObjectPtr<USoundBase>> FireSounds;
	UPROPERTY(EditAnywhere, Category = "Gun") TObjectPtr<USoundBase> OverheatSound;

	void Mount(ABLCharacter* InUser);
	void Dismount();
	void SetTriggerHeld(bool bHeld) { bTrigger = bHeld; }
	ABLCharacter* GetUser() const { return User.Get(); }
	float GetHeat() const { return Heat; }
	bool IsOverheated() const { return OverheatTime > 0.f; }
	int32 GetShotsFired() const { return ShotsFired; }
	FVector GetSeatLocation() const;
	float GetBaseYaw() const { return GetActorRotation().Yaw; }
	/** Altura de los ojos del tirador sobre el suelo del afuste. */
	float GetSightHeight() const { return PivotHeight + SightHeight; }
	FVector GetMuzzleLocation() const;
	FVector GetBarrelDirection() const;
	/** Punto al que apunta la mira (lo que hay bajo la retícula, o a Range). */
	FVector GetAimPoint() const { return AimPoint; }
	FVector GetLastImpact() const { return LastImpact; }

private:
	void FireShot();

	UPROPERTY(VisibleAnywhere, Category = "Gun") TObjectPtr<UStaticMeshComponent> Gun;
	UPROPERTY(VisibleAnywhere, Category = "Gun") TObjectPtr<UPointLightComponent> Flash;
	UPROPERTY() TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;

	TWeakObjectPtr<ABLCharacter> User;
	bool bTrigger = false;
	bool bAnnounced = false;
	float FireTimer = 0.f;
	float Heat = 0.f;
	float OverheatTime = 0.f;
	float FlashTime = -1.f;
	float Kick = 0.f;
	int32 ShotsFired = 0;
	FVector AimPoint = FVector::ZeroVector;
	FVector LastImpact = FVector::ZeroVector;

	void UpdateAimPoint();
};
