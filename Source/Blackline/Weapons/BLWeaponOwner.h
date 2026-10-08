#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BLWeaponOwner.generated.h"

class USkeletalMeshComponent;
class UBLWeaponData;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UBLWeaponOwner : public UInterface
{
	GENERATED_BODY()
};

/**
 * Lo implementa quien lleva un UBLWeaponComponent (jugador y, en el Bloque 5, la IA).
 * Separa la lógica del arma de quién la empuña.
 */
class BLACKLINE_API IBLWeaponOwner
{
	GENERATED_BODY()

public:
	/** Origen y dirección del disparo (cámara para el jugador, ojos para la IA). */
	virtual void GetWeaponAimView(FVector& OutOrigin, FVector& OutDirection) const = 0;

	/** Malla visible del arma (de ella salen fogonazo y casquillos). */
	virtual USkeletalMeshComponent* GetWeaponMeshComponent() const = 0;

	/** true si el portador no puede disparar ahora (sprint, mantle...). */
	virtual bool IsWeaponBlocked() const = 0;

	/** 0 cadera .. 1 apuntando. */
	virtual float GetWeaponAimAlpha() const = 0;

	/** El componente avisa de que quiere disparar (el portador puede cancelar el sprint). */
	virtual void OnWeaponTriggerPressed() {}

	/** Se equipó un arma: el portador ajusta malla, animaciones, mira... */
	virtual void OnWeaponEquipped(const UBLWeaponData* Data) {}

	/** Disparo realizado: el portador aplica retroceso visual. */
	virtual void OnWeaponFired(const UBLWeaponData* Data) {}

	/** Empieza / termina la recarga (duración real ya ajustada). */
	virtual void OnWeaponReloadStarted(const UBLWeaponData* Data, float Duration, bool bEmpty) {}
	virtual void OnWeaponReloadEnded(bool bCompleted) {}
};
