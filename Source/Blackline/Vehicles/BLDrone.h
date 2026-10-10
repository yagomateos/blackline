#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "BLDrone.generated.h"

class UAudioComponent;
class USoundBase;
class USpotLightComponent;
class UStaticMeshComponent;

/**
 * Dron de reconocimiento de Corvane (misión 2). Escondido hasta que lo activa un objetivo (etiqueta "BLDrone").
 * Recorre Path en bucle a su altura, con el foco barriendo hacia abajo. Si ve al jugador DetectTime seguidos
 * dentro del cono del foco, marca su posición a todos los milicianos a menos de AlertRadius (y salta la alarma).
 * No dispara. Se derriba a tiros (Health): chispas, humo, cae girando y revienta contra el suelo.
 */
UCLASS()
class BLACKLINE_API ABLDrone : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLDrone();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, Category = "Drone") TArray<FVector> Path;
	UPROPERTY(EditAnywhere, Category = "Drone") float Speed = 520.f;
	UPROPERTY(EditAnywhere, Category = "Drone") float Health = 120.f;
	UPROPERTY(EditAnywhere, Category = "Drone") float DetectRange = 3200.f;
	UPROPERTY(EditAnywhere, Category = "Drone") float DetectTime = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Drone") float AlertRadius = 7000.f;
	/** Ángulo del cono del foco (también el de detección). */
	UPROPERTY(EditAnywhere, Category = "Drone") float ConeAngle = 20.f;

	bool IsActive() const { return State != EState::Hidden; }
	bool IsDestroyed() const { return State == EState::Falling || State == EState::Wreck; }
	bool IsWrecked() const { return State == EState::Wreck; }
	int32 GetDetections() const { return Detections; }
	float GetSpotProgress() const { return SpotTime / FMath::Max(DetectTime, 0.01f); }

private:
	enum class EState : uint8 { Hidden, Patrol, Falling, Wreck };

	void TickPatrol(float DeltaTime);
	void TickFalling(float DeltaTime);
	void Detect(class APawn* Player);
	void Shatter();

	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TArray<TObjectPtr<UStaticMeshComponent>> Props;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<USpotLightComponent> Spot;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<UAudioComponent> Buzz;
	UPROPERTY() TObjectPtr<USoundBase> CrashSound;
	UPROPERTY() TObjectPtr<USoundBase> AlertSound;

	EState State = EState::Hidden;
	int32 PathIndex = 0;
	FVector Velocity = FVector::ZeroVector;
	float SweepTime = 0.f;
	float SpotTime = 0.f;
	float AlertCooldown = 0.f;
	float PropSpin = 0.f;
	int32 Detections = 0;
	FRotator Spin = FRotator::ZeroRotator;
};
