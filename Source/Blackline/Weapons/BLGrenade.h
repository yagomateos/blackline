#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLGrenade.generated.h"

class UPointLightComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;
class UBLSurfaceEffectsData;

/**
 * Granada de fragmentación M-6 (Bloque 11): física con rebotes, espoleta de tiempo, daño radial con caída
 * (bloqueado por paredes), impulso a ragdolls/objetos, destello, polvo y humo, cascotes, marca, sonido cercano o
 * lejano según la distancia y sacudida al jugador. La IA la "ve" (lista de granadas vivas) y huye gritando.
 */
UCLASS()
class BLACKLINE_API ABLGrenade : public AActor
{
	GENERATED_BODY()

public:
	ABLGrenade();

	/** Lanza la granada: velocidad inicial y quién la tiró (para el daño y las estadísticas). */
	void Launch(const FVector& Velocity, AController* InInstigator, float InFuse);
	virtual void Tick(float DeltaTime) override;

	bool HasExploded() const { return bExploded; }
	float GetTimeLeft() const { return FuseLeft; }
	int32 GetBounces() const { return Bounces; }

	/** Granadas vivas (para la IA). */
	static const TArray<TWeakObjectPtr<ABLGrenade>>& GetLive() { return Live; }
	/** Explosiones recientes (pruebas). */
	static int32 GetExplosionCount() { return ExplosionCount; }

	UPROPERTY(EditAnywhere, Category = "Grenade") float BaseDamage = 150.f;
	UPROPERTY(EditAnywhere, Category = "Grenade") float MinDamage = 12.f;
	UPROPERTY(EditAnywhere, Category = "Grenade") float InnerRadius = 200.f;
	UPROPERTY(EditAnywhere, Category = "Grenade") float OuterRadius = 650.f;
	UPROPERTY(EditAnywhere, Category = "Grenade") float ImpulseStrength = 900.f;
	/** Granada de humo: no hace daño; suelta una nube densa que tapa la vista (y la de la IA). */
	UPROPERTY(EditAnywhere, Category = "Grenade") bool bSmoke = false;

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION() void HandleHit(UPrimitiveComponent* HitComp, AActor* Other, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void Explode();

	UPROPERTY(VisibleAnywhere, Category = "Grenade") TObjectPtr<USphereComponent> Sphere;
	UPROPERTY(VisibleAnywhere, Category = "Grenade") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, Category = "Grenade") TObjectPtr<UPointLightComponent> Flash;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> ExplosionSounds;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> DistantSounds;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> BounceSounds;
	UPROPERTY() TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;

	TWeakObjectPtr<AController> InstigatorController;
	float FuseLeft = 3.5f;
	float FlashTime = -1.f;
	float LastBounceTime = -1.f;
	int32 Bounces = 0;
	bool bExploded = false;

	static TArray<TWeakObjectPtr<ABLGrenade>> Live;
	static int32 ExplosionCount;
};
