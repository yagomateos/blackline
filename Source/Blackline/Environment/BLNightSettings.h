#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLNightSettings.generated.h"

class ULocalLightComponent;

/**
 * Noche (misión 2): la oscuridad esconde. Un actor por nivel; si no hay ninguno, todo se ve como de día.
 * La IA multiplica su consciencia por GetVisibility():
 *  - a oscuras, poco (DarkVisibility) y nada más allá de DarkSightRadius;
 *  - dentro de la luz de una lámpara con la etiqueta "BLLit" (farolas, focos de la alarma), como de día;
 *  - en el cono de una linterna de la IA, más que de día;
 *  - el fogonazo de tu arma te delata durante un segundo.
 */
UCLASS()
class BLACKLINE_API ABLNightSettings : public AActor
{
	GENERATED_BODY()

public:
	ABLNightSettings();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Multiplicador de consciencia del que mira desde Viewer a Target (a Dist cm). 0 = no lo ve. 1 sin noche. */
	static float GetVisibility(const UWorld* World, const FVector& Viewer, const AActor* Target, float Dist);
	/** true si la posición está iluminada por alguna lámpara "BLLit" encendida (o por una linterna). */
	static bool IsLit(const UWorld* World, const FVector& Location);

	UPROPERTY(EditAnywhere, Category = "Night") float DarkVisibility = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Night") float DarkSightRadius = 2600.f;
	/** Fracción del radio de atenuación de una lámpara que ilumina lo bastante para verte. */
	UPROPERTY(EditAnywhere, Category = "Night") float LitRadiusScale = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Night") float FlashlightRange = 2800.f;
	UPROPERTY(EditAnywhere, Category = "Night") float FlashlightVisibility = 1.6f;

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	static ABLNightSettings* Find(const UWorld* World);
	bool InFlashlight(const FVector& Location) const;

	TArray<TWeakObjectPtr<ULocalLightComponent>> Lights;
	float LastPlayerShotTime = -100.f;
	int32 LastPlayerShots = -1;
	static TWeakObjectPtr<ABLNightSettings> Instance;
};
