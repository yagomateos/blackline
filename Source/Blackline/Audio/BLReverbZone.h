#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLReverbZone.generated.h"

class UBoxComponent;
class UReverbEffect;

/**
 * Zona acústica (Bloque 7): mientras la cámara del jugador está dentro de la caja se activa su reverb
 * (callejón estrecho, interior...) por encima de la del exterior, y el ambiente de fondo baja (AmbienceScale).
 * Se usa en vez de AAudioVolume porque las caja se colocan y prueban por script sin brushes.
 */
UCLASS()
class BLACKLINE_API ABLReverbZone : public AActor
{
	GENERATED_BODY()

public:
	ABLReverbZone();

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, Category = "Audio") TObjectPtr<UBoxComponent> Box;
	UPROPERTY(EditAnywhere, Category = "Audio") TObjectPtr<UReverbEffect> Reverb;
	UPROPERTY(EditAnywhere, Category = "Audio") float ReverbVolume = 1.f;
	/** Prioridad sobre otras zonas solapadas (la del exterior es 0). */
	UPROPERTY(EditAnywhere, Category = "Audio") float Priority = 1.f;
	/** Volumen del ambiente de ciudad dentro de la zona (1 = sin cambio). */
	UPROPERTY(EditAnywhere, Category = "Audio") float AmbienceScale = 1.f;
	UPROPERTY(EditAnywhere, Category = "Audio") float FadeTime = 0.6f;

	bool IsListenerInside() const { return bInside; }
	/** Zona activa con más prioridad que contiene al oyente (para pruebas). */
	static ABLReverbZone* GetActiveZone(const UObject* WorldContext);

private:
	void SetInside(bool bNewInside);
	bool bInside = false;
};
