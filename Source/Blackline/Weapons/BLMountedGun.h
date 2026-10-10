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
 * Montado, el jugador no se mueve, la mirada queda limitada al arco del afuste (YawLimit/Pitch) y el arma sigue la
 * mirada; el gatillo dispara la ametralladora: trazado desde la cámara con dispersión, daño por bala, impactos por
 * material, fogonazo, retroceso de cámara y **recalentamiento** (si se pasa, se bloquea OverheatLock segundos).
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
	UPROPERTY(EditAnywhere, Category = "Gun") float PitchMin = -14.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float PitchMax = 22.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float RoundsPerMinute = 620.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float Damage = 42.f;
	UPROPERTY(EditAnywhere, Category = "Gun") float SpreadDegrees = 0.7f;
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
};
