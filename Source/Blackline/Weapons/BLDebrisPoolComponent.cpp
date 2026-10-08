#include "Weapons/BLDebrisPoolComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "FX/BLSurfaceEffects.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

UBLDebrisPoolComponent::UBLDebrisPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

UBLDebrisPoolComponent::FPool& UBLDebrisPoolComponent::GetPool(UStaticMesh* Mesh, bool bFirstPerson)
{
	const TPair<TObjectPtr<UStaticMesh>, bool> Key(Mesh, bFirstPerson);
	if (FPool* Existing = Pools.Find(Key))
	{
		return *Existing;
	}

	FPool& Pool = Pools.Add(Key);
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(GetOwner());
	ISM->SetStaticMesh(Mesh);
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCastShadow(!bFirstPerson);
	// Coordenadas de mundo: el componente no sigue al actor
	ISM->SetUsingAbsoluteLocation(true);
	ISM->SetUsingAbsoluteRotation(true);
	ISM->SetUsingAbsoluteScale(true);
	ISM->SetWorldTransform(FTransform::Identity);
	if (bFirstPerson)
	{
		ISM->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	}
	ISM->RegisterComponent();

	// Instancias fijas: las inactivas tienen escala 0
	TArray<FTransform> Initial;
	Initial.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), MaxPerMesh);
	ISM->AddInstances(Initial, false, true);

	Pool.ISM = ISM;
	Pool.Particles.SetNum(MaxPerMesh);
	return Pool;
}

void UBLDebrisPoolComponent::Spawn(const FSpawnParams& Params)
{
	if (!Params.Mesh)
	{
		return;
	}
	FPool& Pool = GetPool(Params.Mesh, Params.bFirstPerson);
	FParticle& P = Pool.Particles[Pool.Next];
	Pool.Next = (Pool.Next + 1) % Pool.Particles.Num();
	if (!P.bAlive)
	{
		++Pool.Alive;
	}

	P = FParticle();
	P.bAlive = true;
	P.Location = Params.Location;
	P.Rotation = Params.Rotation;
	P.Velocity = Params.Velocity;
	P.AngularVelocity = Params.AngularVelocity;
	P.Lifetime = Params.Lifetime;
	P.Scale = Params.Scale;
	P.Gravity = Params.Gravity;
	P.Drag = Params.Drag;
	P.BounceSound = Params.BounceSound;
	P.BounceVolume = Params.BounceVolume;

	// Un único trazado al nacer: altura del suelo bajo la partícula
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BLDebrisGround), false, GetOwner());
	Query.bReturnPhysicalMaterial = Params.SurfaceSounds != nullptr;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Params.Location, Params.Location - FVector(0.f, 0.f, 400.f), ECC_Visibility, Query))
	{
		P.GroundZ = Hit.ImpactPoint.Z;
		P.GroundNormal = Hit.ImpactNormal;
		if (Params.SurfaceSounds)
		{
			const TArray<TObjectPtr<USoundBase>>& Sounds = Params.SurfaceSounds->Get(UBLSurfaceEffectsData::ResolveSurface(Hit)).CasingSounds;
			if (Sounds.Num() > 0)
			{
				P.BounceSound = Sounds[FMath::RandHelper(Sounds.Num())];
			}
		}
	}
}

void UBLDebrisPoolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (auto& Pair : Pools)
	{
		FPool& Pool = Pair.Value;
		if (Pool.Alive == 0 || !Pool.ISM)
		{
			continue;
		}

		ScratchTransforms.Reset(Pool.Particles.Num());
		for (FParticle& P : Pool.Particles)
		{
			if (P.bAlive)
			{
				P.Age += DeltaTime;
				if (P.Age >= P.Lifetime)
				{
					P.bAlive = false;
					--Pool.Alive;
				}
			}
			if (!P.bAlive)
			{
				ScratchTransforms.Add(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
				continue;
			}

			P.Velocity.Z -= P.Gravity * DeltaTime;
			P.Velocity *= FMath::Exp(-P.Drag * DeltaTime);
			P.Location += P.Velocity * DeltaTime;
			const float AngSpeed = P.AngularVelocity.Size();
			if (AngSpeed > KINDA_SMALL_NUMBER)
			{
				P.Rotation = FQuat(P.AngularVelocity / AngSpeed, AngSpeed * DeltaTime) * P.Rotation;
			}

			// Rebote contra el plano del suelo calculado al nacer
			const float Radius = 0.5f * P.Scale;
			if (P.Location.Z - Radius < P.GroundZ && P.Velocity.Z < 0.f)
			{
				P.Location.Z = P.GroundZ + Radius;
				if (P.Bounces == 0 && P.BounceSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, P.BounceSound, P.Location, P.BounceVolume, FMath::FRandRange(0.9f, 1.15f));
				}
				++P.Bounces;
				P.Velocity = FVector(P.Velocity.X * 0.4f, P.Velocity.Y * 0.4f, -P.Velocity.Z * (P.Bounces < 3 ? 0.3f : 0.f));
				P.AngularVelocity *= 0.4f;
			}

			// Encoger al final de la vida
			const float Fade = FMath::Clamp((P.Lifetime - P.Age) / 0.15f, 0.f, 1.f);
			ScratchTransforms.Add(FTransform(P.Rotation, P.Location, FVector(P.Scale * Fade)));
		}
		Pool.ISM->BatchUpdateInstancesTransforms(0, ScratchTransforms, true, true);
	}
}
