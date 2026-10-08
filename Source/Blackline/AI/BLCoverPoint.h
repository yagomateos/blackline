#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLCoverPoint.generated.h"

class UArrowComponent;

/**
 * Punto de cobertura colocado en el nivel (a mano o por el script del nivel junto a cada cobertura).
 * La flecha apunta hacia el objeto que cubre (hacia la amenaza). La protección real se comprueba contra
 * la posición del enemigo con trazados, así un mismo punto vale o no según desde dónde llegue el jugador.
 *  - Baja (sacos, jersey, coches): agachado queda oculto; de pie dispara por encima.
 *  - Alta (T-wall, contenedor, esquina): de pie queda oculto; se asoma de lado para disparar.
 */
UCLASS()
class BLACKLINE_API ABLCoverPoint : public AActor
{
	GENERATED_BODY()

public:
	ABLCoverPoint();

	/** Cobertura baja (agacharse) o alta (asomarse de lado). */
	UPROPERTY(EditAnywhere, Category = "Cover") bool bLowCover = true;

	/** Altura de los ojos oculto / disparando (cm sobre el suelo). */
	static constexpr float CrouchEye = 95.f;
	static constexpr float StandEye = 155.f;

	/** ¿Oculta a un personaje en este punto de alguien que mira desde ThreatEye? */
	bool IsProtectedFrom(const FVector& ThreatEye) const;

	/** Posición desde la que disparar a ThreatPoint (de pie encima de la baja, o asomado al lado de la alta).
	 *  Devuelve false si desde ningún sitio cercano se ve el objetivo. */
	bool FindFirePosition(const FVector& ThreatPoint, FVector& OutLocation, bool& bOutStand) const;

	FVector GetGroundLocation() const { return GetActorLocation(); }

	UPROPERTY() TObjectPtr<AActor> Occupant;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UArrowComponent> Arrow;

private:
	bool HasLineOfSight(const FVector& From, const FVector& To) const;
};
