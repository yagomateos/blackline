#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLSmokeEmitter.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

/**
 * Humo y fuego ambiental (Bloque 8) con el mismo sistema barato que los impactos: sprites en un
 * InstancedStaticMesh orientados a la cámara, con opacidad/fotograma/color por instancia (M_FX_Dust).
 *  - Columna de humo lejana: pocos sprites enormes que suben y se los lleva el viento (ciudad en guerra).
 *  - Fuego (bFire): llamas aditivas (M_FX_Flame), luz que parpadea y sonido; el humo sale de encima.
 * Coste fijo: MaxParticles sprites (+ llamas), un draw call por tipo.
 */
UCLASS()
class BLACKLINE_API ABLSmokeEmitter : public AActor
{
	GENERATED_BODY()

public:
	ABLSmokeEmitter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Smoke") int32 MaxParticles = 36;
	/** Vida de cada bocanada (s). Spawn = MaxParticles / Life. */
	UPROPERTY(EditAnywhere, Category = "Smoke") float Life = 36.f;
	UPROPERTY(EditAnywhere, Category = "Smoke") FVector2D StartSize = FVector2D(400.f, 700.f);
	UPROPERTY(EditAnywhere, Category = "Smoke") FVector2D EndSize = FVector2D(2200.f, 3200.f);
	/** Velocidad de subida inicial (cm/s) y viento (cm/s, crece con la altura). */
	UPROPERTY(EditAnywhere, Category = "Smoke") float RiseSpeed = 160.f;
	UPROPERTY(EditAnywhere, Category = "Smoke") FVector Wind = FVector(70.f, 25.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Smoke") FLinearColor Color = FLinearColor(0.16f, 0.15f, 0.14f);
	UPROPERTY(EditAnywhere, Category = "Smoke") float Opacity = 0.55f;
	UPROPERTY(EditAnywhere, Category = "Smoke") float SpawnRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "Fire") bool bFire = false;
	UPROPERTY(EditAnywhere, Category = "Fire") int32 MaxFlames = 18;
	UPROPERTY(EditAnywhere, Category = "Fire") FVector FireExtent = FVector(120.f, 60.f, 10.f);
	UPROPERTY(EditAnywhere, Category = "Fire") float FlameSize = 90.f;
	UPROPERTY(EditAnywhere, Category = "Fire") float LightIntensity = 9000.f;
	/** Alcance de la luz del fuego (la antorcha de la refinería ilumina medio nivel). */
	UPROPERTY(EditAnywhere, Category = "Fire") float FireLightRadius = 1300.f;
	/** Daño por segundo a quien está dentro de las llamas (jugador y enemigos; 0 = no quema). Caja: FireExtent más
	 *  FireDamageMargin en horizontal y hasta 1,8 m de alto. */
	UPROPERTY(EditAnywhere, Category = "Fire") float FireDamagePerSecond = 40.f;
	UPROPERTY(EditAnywhere, Category = "Fire") float FireDamageMargin = 45.f;
	/** Daño de fuego hecho (pruebas). */
	float GetFireDamageDealt() const { return FireDamageDealt; }

	int32 GetAliveParticles() const;

	/** Cortina de humo (granada): densa, baja, emite SpawnDuration segundos y tapa la visión de la IA. */
	void ConfigureSmokeScreen();
	/** true si el segmento atraviesa alguna nube que tapa la vista. */
	static bool BlocksSight(const FVector& From, const FVector& To);
	static int32 GetActiveSmokeScreens();

	/** Segundos emitiendo (0 = siempre). Al acabar y apagarse las partículas, el actor se destruye. */
	UPROPERTY(EditAnywhere, Category = "Smoke") float SpawnDuration = 0.f;
	/** Radio (cm) en el que la nube tapa la vista de la IA (0 = no tapa). */
	UPROPERTY(EditAnywhere, Category = "Smoke") float SightBlockRadius = 0.f;

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	float Age = 0.f;
	static TArray<TWeakObjectPtr<ABLSmokeEmitter>> SightBlockers;

	struct FPuff
	{
		FVector Location;
		FVector Velocity;
		float Age = 0.f;
		float Life = 1.f;
		float Size0 = 1.f;
		float Size1 = 1.f;
		float Frame = 0.f;
		float Roll = 0.f;
		float Spin = 0.f;
		float Shade = 1.f;
		bool bAlive = false;
	};

	void SpawnPuff(bool bPrewarm);
	void ApplyFireDamage(float DeltaTime);
	float FireDamageTimer = 0.f;
	float FireDamageDealt = 0.f;
	void SpawnFlame();
	void UpdateSprites(TArray<FPuff>& List, UInstancedStaticMeshComponent* ISM, float DeltaTime, const FVector& View, bool bFlames);

	UPROPERTY(VisibleAnywhere, Category = "Smoke") TObjectPtr<UInstancedStaticMeshComponent> SmokeISM;
	UPROPERTY(VisibleAnywhere, Category = "Fire") TObjectPtr<UInstancedStaticMeshComponent> FlameISM;
	UPROPERTY(VisibleAnywhere, Category = "Fire") TObjectPtr<UPointLightComponent> FireLight;
	UPROPERTY(VisibleAnywhere, Category = "Fire") TObjectPtr<UAudioComponent> FireSound;

	TArray<FPuff> Puffs;
	TArray<FPuff> Flames;
	TArray<FTransform> Scratch;
	int32 NextPuff = 0;
	int32 NextFlame = 0;
	float SpawnAccum = 0.f;
	float FlameAccum = 0.f;
	float FlickerTime = 0.f;
};
