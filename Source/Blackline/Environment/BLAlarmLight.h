#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLActivatable.h"
#include "BLAlarmLight.generated.h"

class UAudioComponent;
class USoundBase;
class USpotLightComponent;
class UStaticMeshComponent;

/**
 * Foco de la refinería (misión 2). Apagado hasta que salta la alarma (etiqueta "BLAlarm"): se enciende con un
 * parpadeo de arranque, suena la sirena si tiene y, como lleva la etiqueta "BLLit", ilumina al jugador para la
 * IA (ABLNightSettings). Con bStartOn es un foco normal encendido desde el principio.
 * El foco apunta según la rotación del actor (X = dirección del haz).
 */
UCLASS()
class BLACKLINE_API ABLAlarmLight : public AActor, public IBLActivatable
{
	GENERATED_BODY()

public:
	ABLAlarmLight();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMissionActivate(FName Tag) override;

	UPROPERTY(EditAnywhere, Category = "Light") bool bStartOn = false;
	/** Candelas. */
	UPROPERTY(EditAnywhere, Category = "Light") float Intensity = 2500.f;
	UPROPERTY(EditAnywhere, Category = "Light") float Radius = 5000.f;
	UPROPERTY(EditAnywhere, Category = "Light") float ConeAngle = 32.f;
	UPROPERTY(EditAnywhere, Category = "Light") bool bShadows = false;
	/** Dónde está la lámpara respecto al actor (la torre de focos la tiene a 11,5 m) y su inclinación hacia abajo. */
	UPROPERTY(EditAnywhere, Category = "Light") FVector LightOffset = FVector(40.f, 0.f, 1140.f);
	UPROPERTY(EditAnywhere, Category = "Light") float LightPitch = -30.f;
	/** Sirena en bucle (3D) al saltar la alarma. */
	UPROPERTY(EditAnywhere, Category = "Light") TObjectPtr<USoundBase> SirenSound;

	bool IsOn() const { return bOn; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Light") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Light") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, Category = "Light") TObjectPtr<USpotLightComponent> Light;
	UPROPERTY(VisibleAnywhere, Category = "Light") TObjectPtr<UAudioComponent> Siren;

	bool bOn = false;
	float OnTime = -1.f;
};
