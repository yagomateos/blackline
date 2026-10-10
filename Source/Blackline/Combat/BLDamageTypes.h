#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "BLDamageTypes.generated.h"

/**
 * Canal de trazado de las armas (DefaultEngine.ini, ECC_GameTraceChannel1 "Weapon").
 * Bloquea por defecto: el mundo lo bloquea; las cápsulas de los personajes lo ignoran (BLDamage::SetupCharacterCollision)
 * y su malla (perfil CharacterMesh + físicas del Physics Asset) lo bloquea, así el impacto da en un hueso concreto.
 */
#define ECC_BLWeapon ECC_GameTraceChannel1

/** Zona del cuerpo alcanzada (multiplica el daño). */
UENUM(BlueprintType)
enum class EBLHitZone : uint8
{
	None,
	Head,
	Torso,
	Limb,
};

class ACharacter;

namespace BLDamage
{
	/** Zona a partir del hueso del esqueleto del Mannequin (cabeza/cuello, columna/pelvis/clavículas, resto). */
	BLACKLINE_API EBLHitZone ZoneFromBone(FName Bone);

	/** Cápsula ignora el trazado de armas, la malla lo bloquea (impactos por hueso). */
	BLACKLINE_API void SetupCharacterCollision(ACharacter* Character);

	/** Trazado de un disparo por el canal Weapon. Atraviesa lo que no debe parar una bala aunque bloquee el canal
	 *  (el perfil Trigger de Unreal deja los canales propios en Block): formas de colisión que no son de un personaje
	 *  (volúmenes de checkpoint, triggers) y primitivas invisibles. */
	BLACKLINE_API bool WeaponTrace(const UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, FCollisionQueryParams Params);
}
