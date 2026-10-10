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
	/** Habla en persona (Varek): sin clics de radio ni atenuación de la mezcla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio") bool bInPerson = false;
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
	/** Aguantar las oleadas (Waves) hasta eliminarlas todas y al menos MinDuration segundos (Bloque 11). */
	Defend,
	/** Usar todos los interactuables con la etiqueta TargetTag, en cualquier orden (misión 2: las fotos). */
	InteractAll,
	/** Destruir o inutilizar el actor con la etiqueta TargetTag (IBLDestructibleTarget; misión 5: el helicóptero). */
	Destroy,
};

/** Una oleada del contraataque: milicianos que aparecen en los TargetPoints con alguna de SpawnTags y van a por el jugador. */
USTRUCT(BlueprintType)
struct FBLWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") TArray<FName> SpawnTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") int32 Count = 4;
	/** Segundos mínimos desde la oleada anterior (y además que queden pocos vivos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") float Delay = 6.f;
	/** Uno de ellos lanza una granada de humo hacia el jugador al llegar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") bool bSmoke = false;
	/** Radio al empezar la oleada. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") TArray<FBLRadioLine> Radio;
	/** Los primeros N de la oleada son tiradores de supresión (ametralladora). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") int32 Gunners = 0;
	/** Los siguientes N son operadores de Corvane (misión 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") int32 Operators = 0;
	/** Llegan con linterna (noche). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave") bool bFlashlights = false;
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
	/** Defend: oleadas y duración mínima. Los enemigos de las oleadas van a la escuadra SquadId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") TArray<FBLWave> Waves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") float MinDuration = 60.f;
	/** Defend: nueva oleada cuando quedan como mucho estos vivos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") int32 MaxAliveForNextWave = 2;
	/** Reach: exige además estar a la altura del punto (azoteas: el punto de abajo no cuenta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") bool bCheckHeight = false;
	/** Defend: progreso a mostrar con el texto ("transmisión" -> "... (transmisión 63 %)"), por tiempo sobre MinDuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") FString ProgressLabel;
	/** Segundos para cumplirlo (0 = sin límite). Si se acaban: misión fallida con FailText y se repite la fase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") float TimeLimit = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") FString FailText;
	/** Al empezar el objetivo se activan los actores IBLActivatable con estas etiquetas (blindado, helicóptero). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective") TArray<FName> ActivateTags;
};
