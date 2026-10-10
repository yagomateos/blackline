#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BLDebrisPoolComponent.generated.h"

class UStaticMesh;
class UInstancedStaticMeshComponent;
class USoundBase;
class UBLSurfaceEffectsData;

/**
 * Partículas de malla baratas (casquillos, esquirlas): balística simple con un rebote contra el suelo,
 * dibujadas con un InstancedStaticMesh por malla. Sin físicas ni actores por partícula.
 * Con bFirstPerson se dibujan con la proyección de primera persona (para que los casquillos salgan
 * de la ventana de expulsión del arma FP).
 */
UCLASS(ClassGroup = (Blackline))
class BLACKLINE_API UBLDebrisPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLDebrisPoolComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	struct FSpawnParams
	{
		UStaticMesh* Mesh = nullptr;
		bool bFirstPerson = false;
		FVector Location = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Velocity = FVector::ZeroVector;
		/** Eje * velocidad angular (rad/s). */
		FVector AngularVelocity = FVector::ZeroVector;
		float Lifetime = 1.5f;
		float Scale = 1.f;
		float Gravity = 980.f;
		float Drag = 0.5f;
		/** Encoger al final de la vida (casquillos); un cargador que se releva a un objeto físico no. */
		bool bShrinkAtEnd = true;
		/** Sonido en el primer rebote (casquillo contra el suelo). */
		USoundBase* BounceSound = nullptr;
		float BounceVolume = 1.f;
		/** Si se indica, el sonido del rebote sale de los casquillos de la superficie del suelo. */
		const UBLSurfaceEffectsData* SurfaceSounds = nullptr;
	};

	void Spawn(const FSpawnParams& Params);

	/** Máximo de partículas vivas por malla (las más antiguas se reciclan). */
	UPROPERTY(EditAnywhere, Category = "Debris") int32 MaxPerMesh = 48;

private:
	struct FParticle
	{
		FVector Location;
		FQuat Rotation;
		FVector Velocity;
		FVector AngularVelocity;
		float Age = 0.f;
		float Lifetime = 1.f;
		float Scale = 1.f;
		float Gravity = 980.f;
		float Drag = 0.5f;
		float GroundZ = -1e9f;
		FVector GroundNormal = FVector::UpVector;
		int32 Bounces = 0;
		USoundBase* BounceSound = nullptr;
		float BounceVolume = 1.f;
		bool bAlive = false;
		bool bShrink = true;
	};

	struct FPool
	{
		TObjectPtr<UInstancedStaticMeshComponent> ISM;
		TArray<FParticle> Particles;
		int32 Next = 0;
		int32 Alive = 0;
	};

	FPool& GetPool(UStaticMesh* Mesh, bool bFirstPerson);

	TMap<TPair<TObjectPtr<UStaticMesh>, bool>, FPool> Pools;
	TArray<FTransform> ScratchTransforms;
};
