#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "BLFXSubsystem.generated.h"

class UInstancedStaticMeshComponent;
class UBLDebrisPoolComponent;
class UMaterialInterface;
class UStaticMesh;
class UDecalComponent;

/**
 * Efectos de impacto del mundo: polvo/humo (sprites iluminados con atlas), chispas (aditivas,
 * estiradas según la velocidad), escombros (pool de mallas), decals y sonido por superficie.
 *
 * Sistema de partículas propio y barato: dos InstancedStaticMesh (un solo draw call cada uno) con
 * datos por instancia (opacidad, fotograma del atlas, color). Presupuesto fijo (MaxSprites/MaxSparks):
 * los más antiguos se reciclan, así el coste no crece en tiroteos largos.
 */
UCLASS()
class BLACKLINE_API UBLFXSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Impacto completo (partículas, escombros, decal y sonido). ShotDir = dirección de la bala. */
	void SpawnImpact(const FBLSurfaceEffect& Effect, const FHitResult& Hit, const FVector& ShotDir);

	UBLDebrisPoolComponent* GetDebrisPool() const { return Debris; }

	/** Partículas vivas (pruebas y depuración). */
	int32 GetActiveSprites() const;
	int32 GetActiveSparks() const;
	/** Marcas y salpicaduras vivas. */
	int32 GetActiveDecals() const { return Decals.FilterByPredicate([](const TWeakObjectPtr<UDecalComponent>& D) { return D.IsValid(); }).Num(); }

	static constexpr int32 MaxSprites = 160;
	static constexpr int32 MaxSparks = 128;
	static constexpr int32 MaxDecals = 80;
	static constexpr float DecalLifetime = 40.f;

private:
	struct FSprite
	{
		FVector Location;
		FVector Velocity;
		float Age = 0.f;
		float Life = 1.f;
		float StartSize = 10.f;
		float EndSize = 50.f;
		float Opacity = 0.5f;
		float Drag = 3.f;
		float Rise = 15.f;      // flotación (cm/s²)
		float Frame = 0.f;      // celda del atlas 0..3
		float Spin = 0.f;
		float Roll = 0.f;
		FLinearColor Color = FLinearColor::White;
		bool bAlive = false;
	};

	struct FSpark
	{
		FVector Location;
		FVector Velocity;
		float Age = 0.f;
		float Life = 0.2f;
		float Intensity = 1.f;
		FLinearColor Color = FLinearColor(1.f, 0.6f, 0.2f);
		bool bAlive = false;
	};

	void EnsureActor();
	FVector GetViewLocation() const;
	void AddSprite(const FSprite& S);
	void AddSpark(const FSpark& S);
	void UpdateSprites(float DeltaTime, const FVector& View);
	void UpdateSparks(float DeltaTime, const FVector& View);

	UPROPERTY() TObjectPtr<AActor> FXActor;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SpriteISM;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SparkISM;
	UPROPERTY() TObjectPtr<UBLDebrisPoolComponent> Debris;
	UPROPERTY() TObjectPtr<UStaticMesh> QuadMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> SpriteMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> SparkMaterial;

	TArray<FSprite> Sprites;
	TArray<FSpark> Sparks;
	int32 NextSprite = 0;
	int32 NextSpark = 0;
	int32 AliveSprites = 0;
	int32 AliveSparks = 0;
	TArray<TWeakObjectPtr<UDecalComponent>> Decals;
	TArray<FTransform> ScratchTransforms;
	TArray<float> ScratchCustomData;
};
