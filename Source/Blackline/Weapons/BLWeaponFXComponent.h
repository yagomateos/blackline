#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BLWeaponFXComponent.generated.h"

class UBLWeaponData;
class UBLDebrisPoolComponent;
class UDecalComponent;
class UPointLightComponent;
class USkeletalMeshComponent;
class USoundBase;
class UStaticMeshComponent;

/**
 * Efectos del arma: fogonazo (malla + luz), casquillos, impactos (decal, esquirlas, Niagara) y sonidos.
 * Si el portador es el jugador local, el arma suena en 2D (no espacializada) y los casquillos
 * se dibujan en primera persona.
 */
UCLASS(ClassGroup = (Blackline))
class BLACKLINE_API UBLWeaponFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLWeaponFXComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetWeapon(const UBLWeaponData* Data, USkeletalMeshComponent* InWeaponMesh);
	void PlayFire(const UBLWeaponData* Data);
	void PlayImpact(const UBLWeaponData* Data, const FHitResult& Hit);
	/** Ruido de manipulación (foley) del arma. */
	void PlayHandling(const UBLWeaponData* Data, float Volume = 1.f);
	/** Sonido del arma (recarga, gatillo en vacío...). */
	void PlayWeaponSound(USoundBase* Sound, float Volume = 1.f);


private:
	bool IsLocalPlayer() const;
	FTransform GetWeaponSocketTransform(FName Socket, const FVector& FallbackLocalOffset) const;

	TWeakObjectPtr<USkeletalMeshComponent> WeaponMesh;
	TWeakObjectPtr<const UBLWeaponData> CurrentData;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> MuzzleFlash;
	UPROPERTY() TObjectPtr<UPointLightComponent> MuzzleLight;
	UPROPERTY() TObjectPtr<UBLDebrisPoolComponent> Debris;
	float FlashRemaining = 0.f;
	/** Cola pendiente: suena al terminar el disparo suelto o la ráfaga. */
	bool bTailPending = false;
	float TimeSinceShot = 0.f;
	float TailDelay = 0.12f;
	FVector LastMuzzleLocation = FVector::ZeroVector;
};
