#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BLDestructibleTarget.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UBLDestructibleTarget : public UInterface
{
	GENERATED_BODY()
};

/** Algo que un objetivo "Destroy" pide destruir o inutilizar (misión 5: el helicóptero de Corvane; el BTR también vale). */
class IBLDestructibleTarget
{
	GENERATED_BODY()

public:
	virtual bool IsTargetDestroyed() const = 0;
};
