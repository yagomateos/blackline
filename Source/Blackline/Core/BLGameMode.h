#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BLGameMode.generated.h"

class USoundBase;
class UAudioComponent;
class UReverbEffect;

UCLASS()
class BLACKLINE_API ABLGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABLGameMode();

	virtual void BeginPlay() override;
	/** -BLStart=<Fase>: aparece en el TargetPoint "BLTest_Start_<Fase>" (Fase1..Fase4, Objetivo) en vez del PlayerStart. */
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	/** Ambiente de fondo en bucle (ciudad en guerra). Más adelante lo definirá cada mapa/zona. */
	UPROPERTY(EditAnywhere, Category = "Audio") TObjectPtr<USoundBase> AmbientLoop;
	UPROPERTY(EditAnywhere, Category = "Audio") float AmbientVolume = 0.5f;
	/** Reverberación del entorno (exterior urbano). Afecta a los sonidos 3D con envío de reverb. */
	UPROPERTY(EditAnywhere, Category = "Audio") TObjectPtr<UReverbEffect> EnvironmentReverb;

	/** Escala el ambiente de fondo (p. ej. dentro de un local se oye menos la ciudad). */
	void SetAmbienceScale(float Scale, float FadeTime);

private:
	UPROPERTY() TObjectPtr<UAudioComponent> AmbientComponent;
};
