#include "AI/BLVarek.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLSquadSubsystem.h"
#include "Combat/BLHealthComponent.h"
#include "Core/BLAssetUtils.h"
#include "Weapons/BLWeaponComponent.h"
#include "Mission/BLInteractable.h"
#include "EngineUtils.h"

#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

ABLVarek::ABLVarek(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ABLVarekController::StaticClass();
	GetWeapon()->StartingWeapons.Empty();
	GetHealth()->bInvulnerable = true;
	WalkSpeed = 180.f;
	JogSpeed = 420.f;

	const TCHAR* Base = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/");
	IdleAnim = BL::LoadDefault<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	WalkAnims.Reset();
	JogAnims.Reset();
	for (const TCHAR* Dir : { TEXT("Fwd"), TEXT("Fwd_Right"), TEXT("Right"), TEXT("Bwd_Right"), TEXT("Bwd"), TEXT("Bwd_Left"), TEXT("Left"), TEXT("Fwd_Left") })
	{
		WalkAnims.Add(BL::LoadDefault<UAnimSequence>(*FString::Printf(TEXT("%sWalk/MF_Unarmed_Walk_%s.MF_Unarmed_Walk_%s"), Base, Dir, Dir)));
		JogAnims.Add(BL::LoadDefault<UAnimSequence>(*FString::Printf(TEXT("%sJog/MF_Unarmed_Jog_%s.MF_Unarmed_Jog_%s"), Base, Dir, Dir)));
	}
	ReloadAnim = nullptr;
	if (UMaterialInterface* Body = BL::LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Varek_Body.MI_Varek_Body")))
	{
		GetMesh()->SetMaterial(0, Body);
	}
	if (UMaterialInterface* Sleeves = BL::LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Varek_Sleeves.MI_Varek_Sleeves")))
	{
		GetMesh()->SetMaterial(1, Sleeves);
	}
	GetWeaponMesh()->SetVisibility(false);
	Vest->SetVisibility(false);
	Armband->SetVisibility(false);
	Tags.Add(FName("BLVarek"));
}

void ABLVarek::BeginPlay()
{
	Super::BeginPlay();
	Helmet->SetVisibility(false);
	SetCrouchTarget(true);   // de rodillas, retenido
	UE_LOG(LogBlackline, Log, TEXT("[Varek] Materiales: %s / %s"), *GetNameSafe(GetMesh()->GetMaterial(0)), *GetNameSafe(GetMesh()->GetMaterial(1)));
	// El interactuable "Liberar a Varek" está a su lado
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(FName("BLObjective_Varek")))
		{
			It->OnUsed.AddUniqueDynamic(this, &ABLVarek::HandleFreed);
		}
	}
}

void ABLVarek::HandleFreed(ABLInteractable* Interactable, ABLCharacter* User)
{
	Free();
}

void ABLVarek::Free()
{
	if (bFree)
	{
		return;
	}
	bFree = true;
	SetCrouchTarget(false);
	UE_LOG(LogBlackline, Log, TEXT("[Varek] Liberado"));
}

// ---------------------------------------------------------------------------

ABLVarekController::ABLVarekController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABLVarekController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ABLVarek* V = Cast<ABLVarek>(GetPawn());
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!V || !Player || !V->IsFree())
	{
		return;
	}
	const float Dist = FVector::Dist(V->GetActorLocation(), Player->GetActorLocation());

	// Separados (reaparición en un checkpoint, caída...): aparece detrás del jugador
	if (Dist > 3500.f)
	{
		if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation Out;
			if (Nav->ProjectPointToNavigation(Player->GetActorLocation() - Player->GetActorForwardVector() * 250.f, Out, FVector(300.f, 300.f, 300.f)))
			{
				V->SetActorLocation(Out.Location + FVector(0.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
				StopMovement();
			}
		}
		return;
	}

	// Cerca de un tiroteo se agacha
	bool bCombatNear = false;
	if (const UBLSquadSubsystem* Squad = GetWorld()->GetSubsystem<UBLSquadSubsystem>())
	{
		for (const TWeakObjectPtr<ABLAIController>& AI : Squad->GetMembers())
		{
			if (AI.IsValid() && AI->IsInCombat() && AI->GetPawn() && FVector::Dist(AI->GetPawn()->GetActorLocation(), V->GetActorLocation()) < 2500.f)
			{
				bCombatNear = true;
				break;
			}
		}
	}
	V->SetCrouchTarget(bCombatNear && Dist < 500.f);

	RepathTimer -= DeltaTime;
	if (Dist > 450.f && RepathTimer <= 0.f)
	{
		RepathTimer = 0.5f;
		V->SetJog(Dist > 800.f);
		MoveToActor(Player, 250.f, true, true, true, nullptr, true);
	}
	else if (Dist < 250.f && GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		StopMovement();
	}
	// Mira hacia el jugador cuando está parado
	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		const FVector To = (Player->GetActorLocation() - V->GetActorLocation()).GetSafeNormal2D();
		V->SetActorRotation(FMath::RInterpTo(V->GetActorRotation(), To.Rotation(), DeltaTime, 4.f));
	}
}
