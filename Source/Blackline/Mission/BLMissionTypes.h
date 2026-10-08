#pragma once

#include "CoreMinimal.h"
#include "BLMissionTypes.generated.h"

class USoundBase;

/** Una frase de radio (con subtítulo). */
USTRUCT(BlueprintType)
struct FBLRadioLine
{
	GENERATED_BODY()

	/** Quién habla (indicativo): "TORRE", "SABLE 2-1"... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio") FString Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio") FString Text;
	/** Segundos en pantalla (0 = según la longitud del texto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio") float Duration = 0.f;
	/** Voz grabada (Bloque 7). Si existe, la frase dura lo que la voz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio") TObjectPtr<USoundBase> Voice;
};

UENUM(BlueprintType)
enum class EBLObjectiveType : uint8
{
	/** Llegar a un punto (TargetPoint con la etiqueta TargetTag) a menos de Radius. */
	Reach,
	/** Eliminar a todos los enemigos de la escuadra SquadId. */
	ClearSquad,
	/** Usar el interactuable con la etiqueta TargetTag. */
	Interact,
};

/** Un objetivo de la misión. */
USTRUCT(BlueprintType)
struct FBLObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") FString Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") EBLObjectiveType Type = EBLObjectiveType::Reach;
	/** Etiqueta del actor objetivo (punto o interactuable); también es donde se dibuja el marcador. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") FName TargetTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") FName SquadId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") float Radius = 400.f;
	/** Mostrar el marcador con distancia (los objetivos de sigilo pueden ir sin él). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") bool bShowMarker = true;
	/** Radio al empezar y al completar el objetivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") TArray<FBLRadioLine> RadioOnStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") TArray<FBLRadioLine> RadioOnComplete;
};
