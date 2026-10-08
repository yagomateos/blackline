#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BLScriptLibrary.generated.h"

class UPhysicalMaterial;

/** Funciones de apoyo para los scripts de Python del editor (lo que la API de Python no expone bien). */
UCLASS()
class BLACKLINE_API UBLScriptLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Asigna SurfaceType<Index> (1..62) a un material físico: el enum se expone incompleto a Python. */
	UFUNCTION(BlueprintCallable, Category = "Blackline|Scripts")
	static void SetPhysicalSurface(UPhysicalMaterial* PhysMat, int32 Index);
};
