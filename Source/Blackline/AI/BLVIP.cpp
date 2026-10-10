#include "AI/BLVIP.h"

#include "Blackline.h"
#include "Combat/BLHealthComponent.h"
#include "Core/BLAssetUtils.h"
#include "Mission/BLDestructibleTarget.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"

ABLVIP::ABLVIP(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ABLVIPController::StaticClass();
	GetHealth()->bInvulnerable = false;     // hay que cogerle vivo: si le matan, se falla
	GetHealth()->MaxHealth = 100.f;
	JogSpeed = 440.f;
	Tags.Remove(FName("BLVarek"));
	Tags.Add(FName("BLVIP"));
	// Traje oscuro (el uniforme negro de Corvane sirve de traje) en vez de la ropa de Varek
	if (UMaterialInterface* Body = BL::LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Corvane_Body.MI_Corvane_Body")))
	{
		GetMesh()->SetMaterial(0, Body);
	}
	if (UMaterialInterface* Sleeves = BL::LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Varek_Sleeves.MI_Varek_Sleeves")))
	{
		GetMesh()->SetMaterial(1, Sleeves);
	}
}

void ABLVIP::BeginPlay()
{
	Super::BeginPlay();
	Free();   // no está retenido: de pie en su despacho
}

float ABLVIP::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!Cast<APlayerController>(EventInstigator))
	{
		return 0.f;
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

// ---------------------------------------------------------------------------

ABLVIPController::ABLVIPController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABLVIPController::OnMissionActivate(FName Tag)
{
	if (State == EState::Idle)
	{
		State = EState::Fleeing;
		PathIndex = 0;
		UE_LOG(LogBlackline, Log, TEXT("[VIP] Huye hacia el helipuerto"));
	}
}

bool ABLVIPController::IsHeliDown() const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(EscapeHeliTag))
		{
			if (const IBLDestructibleTarget* T = Cast<IBLDestructibleTarget>(*It))
			{
				return T->IsTargetDestroyed();
			}
		}
	}
	return false;
}

void ABLVIPController::Surrender()
{
	State = EState::Surrendered;
	StopMovement();
	ABLVIP* V = Cast<ABLVIP>(GetPawn());
	if (!V)
	{
		return;
	}
	V->SetCrouchTarget(true);     // de rodillas, manos a la vista
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(ArrestTag))
		{
			It->SetActorLocation(V->GetActorLocation() + V->GetActorForwardVector() * 60.f);
			It->bEnabled = true;
		}
	}
	UE_LOG(LogBlackline, Log, TEXT("[VIP] Se rinde"));
}

void ABLVIPController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ABLVIP* V = Cast<ABLVIP>(GetPawn());
	if (!V || V->IsDead())
	{
		return;
	}
	// El interactuable de detenerle no existe hasta que se rinde
	if (State != EState::Surrendered)
	{
		for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
		{
			if (It->Tags.Contains(ArrestTag))
			{
				It->bEnabled = false;
			}
		}
	}
	switch (State)
	{
	case EState::Fleeing:
	{
		if (IsHeliDown())
		{
			Surrender();
			break;
		}
		const TArray<FVector>& P = V->PatrolPoints;
		if (!P.IsValidIndex(PathIndex))
		{
			State = EState::Waiting;
			WaitTime = 0.f;
			V->SetCrouchTarget(true);
			break;
		}
		V->SetJog(true);
		RepathTimer -= DeltaTime;
		if (RepathTimer <= 0.f || GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			RepathTimer = 1.f;
			MoveToLocation(P[PathIndex], 60.f, false, true, true, true, nullptr, true);
		}
		if (FVector::Dist2D(V->GetActorLocation(), P[PathIndex]) < 120.f)
		{
			++PathIndex;
			RepathTimer = 0.f;
		}
		break;
	}
	case EState::Waiting:
		WaitTime += DeltaTime;
		if (IsHeliDown())
		{
			Surrender();
		}
		else if (WaitTime > EscapeTime)
		{
			if (ABLMissionDirector* D = ABLMissionDirector::Get(this))
			{
				D->FailMission(TEXT("El inglés ha escapado en el helicóptero de Corvane."));
			}
			WaitTime = -1000.f;
		}
		break;
	default:
		break;
	}
}
