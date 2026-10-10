#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BLActivatable.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UBLActivatable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Actor que despierta un objetivo de la misión (Bloque 11): el blindado que entra por la calle, el helicóptero
 * que viene a por el equipo... El director llama a OnMissionActivate al empezar un objetivo con ActivateTags,
 * con la etiqueta que lo ha activado (un mismo actor puede tener varias fases: "BLHeli" llega, "BLHeli_Land" aterriza).
 */
class IBLActivatable
{
	GENERATED_BODY()

public:
	virtual void OnMissionActivate(FName Tag) = 0;
};
