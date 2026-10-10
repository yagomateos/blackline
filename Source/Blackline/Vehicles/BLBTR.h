#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "Mission/BLMissionTypes.h"
#include "BLBTR.generated.h"

class UAudioComponent;
class UPointLightComponent;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;
class UBLSurfaceEffectsData;

/**
 * Blindado 8x8 de la Columna (Bloque 11, fase 8). Escondido hasta que un objetivo lo activa (etiqueta "BLBTR"):
 * entra por la calle siguiendo Path, para, gira la torreta y dispara ScriptedShots cañonazos a los TargetPoints
 * con la etiqueta TargetTag (fachada del bloque). En el disparo CollapseShot se viene abajo parte de la fachada:
 * los actores con CollapseTag pasan a simular física y aparecen los escombros con BlockTag (tapan la escalera).
 * Después "suprime": cada pocos segundos dispara cerca del jugador si lo ve (falla a propósito, para meter prisa).
 */
UCLASS()
class BLACKLINE_API ABLBTR : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLBTR();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	/** Puntos del recorrido (mundo). El primero es donde espera escondido. */
	UPROPERTY(EditAnywhere, Category = "BTR") TArray<FVector> Path;
	UPROPERTY(EditAnywhere, Category = "BTR") float Speed = 450.f;
	UPROPERTY(EditAnywhere, Category = "BTR") FName TargetTag = TEXT("BLBTR_Target");
	UPROPERTY(EditAnywhere, Category = "BTR") int32 ScriptedShots = 3;
	UPROPERTY(EditAnywhere, Category = "BTR") float ShotInterval = 3.2f;
	/** Disparo (1..ScriptedShots) que derrumba la fachada (0 = ninguno). */
	UPROPERTY(EditAnywhere, Category = "BTR") int32 CollapseShot = 2;
	UPROPERTY(EditAnywhere, Category = "BTR") FName CollapseTag = TEXT("BLCollapse");
	UPROPERTY(EditAnywhere, Category = "BTR") FName BlockTag = TEXT("BLCollapseBlock");
	/** Frase al derrumbarse (Varek: "¡La escalera se ha venido abajo!"). */
	UPROPERTY(EditAnywhere, Category = "BTR") TArray<FBLRadioLine> CollapseRadio;
	/** Supresión: deja de disparar cuando el jugador pasa de esta X (ya está fuera de su calle). */
	UPROPERTY(EditAnywhere, Category = "BTR") float SuppressMaxPlayerX = 21000.f;
	UPROPERTY(EditAnywhere, Category = "BTR") FVector2D SuppressInterval = FVector2D(4.5f, 7.5f);
	UPROPERTY(EditAnywhere, Category = "BTR") float ShellDamage = 140.f;
	UPROPERTY(EditAnywhere, Category = "BTR") float ShellInnerRadius = 220.f;
	UPROPERTY(EditAnywhere, Category = "BTR") float ShellOuterRadius = 650.f;

	bool IsActive() const { return State != EState::Hidden; }
	bool HasArrived() const { return State == EState::Firing || State == EState::Suppress; }
	int32 GetShotsFired() const { return ShotsFired; }
	bool HasCollapsed() const { return bCollapsed; }
	/** Destruido por la aviación (misión 4): para, arde y deja de disparar. */
	void DestroyVehicle();
	bool IsVehicleDestroyed() const { return State == EState::Destroyed; }

private:
	enum class EState : uint8 { Hidden, Driving, Firing, Suppress, Destroyed };

	void TickDrive(float DeltaTime);
	/** Gira torreta y cañón hacia AimPoint; true si ya apunta. */
	bool TickAim(float DeltaTime);
	void Fire();
	void Collapse(const FVector& Impact);
	FVector GetMuzzleLocation() const;
	bool PickSuppressTarget(FVector& Out) const;

	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<UStaticMeshComponent> Turret;
	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<UStaticMeshComponent> Gun;
	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<UPointLightComponent> MuzzleLight;
	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<UPointLightComponent> Headlight;
	UPROPERTY(VisibleAnywhere, Category = "BTR") TObjectPtr<UAudioComponent> Engine;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> CannonSounds;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> ImpactSounds;
	UPROPERTY() TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;

	EState State = EState::Hidden;
	int32 PathIndex = 1;
	float CurrentSpeed = 0.f;
	float StateTime = 0.f;
	float NextShot = 0.f;
	float FlashTime = -1.f;
	float Recoil = 0.f;
	int32 ShotsFired = 0;
	int32 TargetIndex = 0;
	bool bCollapsed = false;
	FVector AimPoint = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> Targets;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> Falling;
	float FallingTime = 0.f;
};
