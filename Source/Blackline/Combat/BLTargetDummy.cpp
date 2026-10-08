#include "Combat/BLTargetDummy.h"

#include "Combat/BLDamageTypes.h"
#include "Combat/BLHealthComponent.h"
#include "Combat/BLHitReactionComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABLTargetDummy::ABLTargetDummy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::Disabled;

	Health = CreateDefaultSubobject<UBLHealthComponent>(TEXT("Health"));
	Health->RegenRate = 0.f; // las dianas no regeneran
	HitReaction = CreateDefaultSubobject<UBLHitReactionComponent>(TEXT("HitReaction"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<UAnimationAsset> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	if (Manny.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(Manny.Object);
	}
	IdleAnim = Idle.Object;
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -92.f), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetCapsuleComponent()->InitCapsuleSize(34.f, 92.f);
}

void ABLTargetDummy::BeginPlay()
{
	Super::BeginPlay();
	SpawnTransform = GetActorTransform();
	BLDamage::SetupCharacterCollision(this);
	if (IdleAnim)
	{
		GetMesh()->PlayAnimation(IdleAnim, true);
	}
	Health->OnDeath.AddDynamic(this, &ABLTargetDummy::HandleDeath);
}

void ABLTargetDummy::HandleDeath(const FBLDamageInfo& Info)
{
	if (RespawnDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ABLTargetDummy::Revive, RespawnDelay, false);
	}
}

void ABLTargetDummy::Revive()
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::ResetPhysics);
	HitReaction->ResetRagdoll();
	BLDamage::SetupCharacterCollision(this);
	Health->ResetHealth();
	if (IdleAnim)
	{
		GetMesh()->PlayAnimation(IdleAnim, true);
	}
}
