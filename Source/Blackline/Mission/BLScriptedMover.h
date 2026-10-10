#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "BLScriptedMover.generated.h"

class UAudioComponent;
class USoundBase;
class UStaticMeshComponent;

/**
 * Actor guionizado genérico (misión 3): al activarse (etiqueta del objetivo) suena StartSound una vez y, si tiene
 * Path, recorre los puntos acelerando despacio y girando poco a poco (el barco sin bandera que zarpa), con LoopSound
 * mientras se mueve. Sin Path sirve para un sonido situado que se dispara con la misión (las campanas).
 * La malla (Mesh) la pone el nivel.
 */
UCLASS()
class BLACKLINE_API ABLScriptedMover : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLScriptedMover();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	UPROPERTY(EditAnywhere, Category = "Mover") TArray<FVector> Path;
	UPROPERTY(EditAnywhere, Category = "Mover") float Speed = 300.f;
	UPROPERTY(EditAnywhere, Category = "Mover") float Acceleration = 25.f;
	/** Grados por segundo al girar hacia el siguiente punto. */
	UPROPERTY(EditAnywhere, Category = "Mover") float TurnRate = 3.f;
	UPROPERTY(EditAnywhere, Category = "Mover") float StartDelay = 0.f;
	UPROPERTY(EditAnywhere, Category = "Mover") bool bHiddenUntilActive = false;
	/** Al llegar al último punto se esconde (se pierde en la niebla). */
	UPROPERTY(EditAnywhere, Category = "Mover") bool bHideAtEnd = false;
	UPROPERTY(EditAnywhere, Category = "Mover") TObjectPtr<USoundBase> StartSound;
	UPROPERTY(EditAnywhere, Category = "Mover") float StartSoundVolume = 1.f;
	UPROPERTY(EditAnywhere, Category = "Mover") TObjectPtr<USoundBase> LoopSound;
	/**
	 * Voladura (misión 4, el tramo del puente): explosiones en BlastPoints, daño alrededor y la malla cae FallDepth
	 * con la gravedad, girando hasta FallRotation. Caída guiada y no con física: el tramo apoya justo en el estribo y en
	 * la pila, y con física se quedaba encima.
	 */
	UPROPERTY(EditAnywhere, Category = "Mover") bool bPhysicsFall = false;
	UPROPERTY(EditAnywhere, Category = "Mover") float FallDepth = 800.f;
	UPROPERTY(EditAnywhere, Category = "Mover") FRotator FallRotation = FRotator(-5.f, 0.f, 7.f);
	UPROPERTY(EditAnywhere, Category = "Mover") TArray<FVector> BlastPoints;
	UPROPERTY(EditAnywhere, Category = "Mover") float BlastDamage = 500.f;
	UPROPERTY(EditAnywhere, Category = "Mover") float BlastRadius = 700.f;

	bool HasStarted() const { return bActive; }
	bool HasArrived() const { return bArrived; }
	bool HasFallen() const { return bFalling; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Mover") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, Category = "Mover") TObjectPtr<UAudioComponent> Loop;

	bool bActive = false;
	bool bArrived = false;
	float Delay = 0.f;
	float CurrentSpeed = 0.f;
	int32 Index = 0;
	bool bFalling = false;
	float FallTime = 0.f;
	FVector FallStart = FVector::ZeroVector;
	FRotator FallStartRot = FRotator::ZeroRotator;
};
