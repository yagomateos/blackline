#include "Combat/BLHitReactionComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"

UBLHitReactionComponent::UBLHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UBLHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	Mesh = Char ? Char->GetMesh() : GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh)
	{
		return;
	}
	MeshRelative = Mesh->GetRelativeTransform();
	SavedMeshProfile = Mesh->GetCollisionProfileName();
	SavedMeshCollision = Mesh->GetCollisionEnabled();

	PhysicalAnimation = NewObject<UPhysicalAnimationComponent>(GetOwner(), TEXT("BLPhysicalAnimation"));
	PhysicalAnimation->RegisterComponent();
	PhysicalAnimation->SetSkeletalMeshComponent(Mesh);
	FPhysicalAnimationData Data;
	Data.bIsLocalSimulation = false;
	Data.OrientationStrength = OrientationStrength;
	Data.AngularVelocityStrength = AngularVelocityStrength;
	Data.PositionStrength = OrientationStrength * 0.5f;
	Data.VelocityStrength = AngularVelocityStrength * 0.5f;
	PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(ReactionRootBone, Data, true);

	if (UBLHealthComponent* Health = GetOwner()->FindComponentByClass<UBLHealthComponent>())
	{
		Health->OnDamaged.AddDynamic(this, &UBLHitReactionComponent::HandleDamaged);
		Health->OnDeath.AddDynamic(this, &UBLHitReactionComponent::HandleDeath);
	}
}

void UBLHitReactionComponent::HandleDamaged(const FBLDamageInfo& Info)
{
	if (!Mesh || bRagdoll || Info.bKilled)
	{
		return;
	}
	if (!bSimulating)
	{
		// Las físicas necesitan colisión física en la malla (el perfil del personaje es solo de consulta)
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetAllBodiesBelowSimulatePhysics(ReactionRootBone, true, true);
		bSimulating = true;
	}
	Weight = 1.f;
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(ReactionRootBone, Weight, false, true);
	// Solo simulan los huesos desde ReactionRootBone hacia arriba: un tiro en la cadera o en las piernas empuja el
	// tronco desde abajo (antes el impulso se perdía: "has to have Simulate Physics enabled")
	FName Bone = Info.Bone.IsNone() ? ReactionRootBone : Info.Bone;
	const FBodyInstance* Body = Mesh->GetBodyInstance(Bone);
	if (!Body || !Body->IsInstanceSimulatingPhysics())
	{
		Bone = ReactionRootBone;
	}
	if (Mesh->GetBodyInstance(Bone))
	{
		Mesh->AddImpulseAtLocation(Info.Direction * Info.Amount * ImpulsePerDamage, Info.Location, Bone);
	}
}

void UBLHitReactionComponent::HandleDeath(const FBLDamageInfo& Info)
{
	if (!Mesh || bRagdoll)
	{
		return;
	}
	bRagdoll = true;
	bSimulating = false;
	Weight = 0.f;
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		Char->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Char->GetCharacterMovement()->DisableMovement();
		Char->GetCharacterMovement()->StopMovementImmediately();
	}
	if (PhysicalAnimation)
	{
		PhysicalAnimation->SetStrengthMultiplyer(0.f); // sin motores: cuerpo inerte
	}
	Mesh->SetCollisionProfileName(FName("Ragdoll"));
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetAllBodiesPhysicsBlendWeight(1.f);
	Mesh->WakeAllRigidBodies();
	const FName Bone = Info.Bone.IsNone() ? FName("spine_03") : Info.Bone;
	Mesh->AddImpulseAtLocation(Info.Direction * Info.Amount * DeathImpulsePerDamage, Info.Location, Bone);
}

void UBLHitReactionComponent::StopSimulation()
{
	if (Mesh && bSimulating)
	{
		Mesh->SetAllBodiesBelowSimulatePhysics(ReactionRootBone, false, true);
		Mesh->SetCollisionEnabled(SavedMeshCollision);
	}
	bSimulating = false;
	Weight = 0.f;
}

void UBLHitReactionComponent::ResetRagdoll()
{
	if (!Mesh)
	{
		return;
	}
	StopSimulation();
	if (bRagdoll)
	{
		Mesh->SetAllBodiesSimulatePhysics(false);
		Mesh->SetAllBodiesPhysicsBlendWeight(0.f);
		Mesh->SetCollisionProfileName(SavedMeshProfile);
		Mesh->SetCollisionEnabled(SavedMeshCollision);
		if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
		{
			Mesh->AttachToComponent(Char->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			Char->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Char->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
		Mesh->SetRelativeTransform(MeshRelative);
		if (PhysicalAnimation)
		{
			PhysicalAnimation->SetStrengthMultiplyer(1.f);
		}
	}
	bRagdoll = false;
}

void UBLHitReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bSimulating || bRagdoll || !Mesh)
	{
		return;
	}
	Weight = FMath::Max(0.f, Weight - DeltaTime / FMath::Max(RecoverTime, 0.05f));
	// Curva suave: la animación recupera el control al final
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(ReactionRootBone, FMath::SmoothStep(0.f, 1.f, Weight), false, true);
	if (Weight <= 0.f)
	{
		StopSimulation();
	}
}
