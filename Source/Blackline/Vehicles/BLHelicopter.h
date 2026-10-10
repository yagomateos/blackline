#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "Mission/BLDestructibleTarget.h"
#include "BLHelicopter.generated.h"

class ABLCharacter;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class USoundBase;
class USpotLightComponent;
class UStaticMeshComponent;
class UBLSurfaceEffectsData;

/**
 * Helicóptero de extracción (Bloque 11, fase 9). Escondido hasta que lo activa un objetivo:
 *  - "BLHeli": llega por Path (desde el mar) y se queda en HoverPoint; el ametrallador de puerta dispara ráfagas
 *    a los milicianos que ve (cobertura mientras el equipo defiende la zona de aterrizaje).
 *  - "BLHeli_Land": baja a LandPoint; al tocar suelo se habilita el interactuable con InteractTag ("Subir").
 * Rotores girando, sonido del rotor que se oye desde lejos, foco del morro, alabeo y cabeceo al maniobrar.
 */
UCLASS()
class BLACKLINE_API ABLHelicopter : public AActor, public IBLActivatable, public IBLDestructibleTarget
{
	GENERATED_BODY()

public:
	ABLHelicopter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;
	/** Pruebas: lo deja visible, quieto en el aire (o posado) con el rotor a Rate grados/s. */
	void ShowForTest(float Rate, bool bLanded);
	float GetRotorRate() const { return RotorRate; }
	bool IsRotorDiscVisible() const;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual bool IsTargetDestroyed() const override { return State == EState::Disabled; }

	UPROPERTY(EditAnywhere, Category = "Heli") TArray<FVector> Path;
	UPROPERTY(EditAnywhere, Category = "Heli") FVector HoverPoint = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Heli") FVector LandPoint = FVector::ZeroVector;
	/** Orientación en el estacionario y en tierra (la puerta del ametrallador, lado derecho, hacia la amenaza). */
	UPROPERTY(EditAnywhere, Category = "Heli") float HoverYaw = 180.f;
	UPROPERTY(EditAnywhere, Category = "Heli") float Speed = 2600.f;
	/** Segundos desde que se activa hasta que aparece (lo anuncia la radio). */
	UPROPERTY(EditAnywhere, Category = "Heli") float ApproachDelay = 6.f;
	UPROPERTY(EditAnywhere, Category = "Heli") FName InteractTag = TEXT("BLObjective_Heli");
	UPROPERTY(EditAnywhere, Category = "Heli") float DoorGunRange = 6000.f;
	UPROPERTY(EditAnywhere, Category = "Heli") FVector2D DoorGunInterval = FVector2D(2.5f, 4.f);
	UPROPERTY(EditAnywhere, Category = "Heli") float DoorGunDamage = 70.f;
	UPROPERTY(EditAnywhere, Category = "Heli") bool bDoorGun = true;
	/** Misión 5 (el de Corvane): se puede inutilizar a tiros; humea y hace un aterrizaje forzoso en LandPoint. */
	UPROPERTY(EditAnywhere, Category = "Heli") bool bDamageable = false;
	UPROPERTY(EditAnywhere, Category = "Heli") float Health = 700.f;
	/** Pintura del casco (el de Corvane es negro). */
	UPROPERTY(EditAnywhere, Category = "Heli") TObjectPtr<UMaterialInterface> BodyMaterial;

	bool IsLanded() const { return State == EState::Landed; }
	bool IsHovering() const { return State == EState::Hover; }
	bool IsActive() const { return State != EState::Hidden; }
	int32 GetBurstsFired() const { return Bursts; }

private:
	enum class EState : uint8 { Hidden, Waiting, Approach, Hover, Landing, Landed, Disabled };

	/** Vuela hacia Goal frenando al llegar; true al estar encima (< Tolerance). */
	bool FlyTo(const FVector& Goal, float DeltaTime, float MaxSpeed, float Tolerance, bool bFaceGoal);
	void TickDoorGun(float DeltaTime);
	void EnableBoarding();
	UFUNCTION() void HandleBoarded(class ABLInteractable* Interactable, ABLCharacter* User);

	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<UStaticMeshComponent> Rotor;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<UStaticMeshComponent> TailRotor;
	/** Discos de desenfoque: a régimen de vuelo se ve el disco (como en una cámara) y no las palas nítidas. */
	/** Dentro de la cabina, junto a la puerta izquierda: ahí queda el jugador al subir. */
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<USceneComponent> CabinSpot;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<class UPointLightComponent> CabinLight;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<UStaticMeshComponent> RotorDisc;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<UStaticMeshComponent> TailRotorDisc;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RotorDiscMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TailRotorDiscMID;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<USpotLightComponent> Searchlight;
	UPROPERTY(VisibleAnywhere, Category = "Heli") TObjectPtr<UAudioComponent> RotorSound;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> DoorGunSounds;
	UPROPERTY() TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;

	EState State = EState::Hidden;
	bool bLandRequested = false;
	int32 PathIndex = 0;
	float StateTime = 0.f;
	float NextBurst = 3.f;
	int32 Bursts = 0;
	float RotorSpin = 0.f;
	float Bob = 0.f;
	FVector Velocity = FVector::ZeroVector;
	FRotator Attitude = FRotator::ZeroRotator;
	float DisabledTime = 0.f;
	/** Grados/s del rotor principal: 1800 = 300 rpm (régimen real de vuelo); el disco de desenfoque lo hace creíble. */
	float RotorRate = 1800.f;
	float DiscSpin = 0.f;

	void UpdateRotorVisuals(float DeltaTime);
};
