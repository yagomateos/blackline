#include "AI/BLSquadSubsystem.h"

#include "AI/BLAIController.h"
#include "AI/BLCoverPoint.h"
#include "AI/BLEnemyCharacter.h"

void UBLSquadSubsystem::Register(ABLAIController* AI)
{
	Members.AddUnique(AI);
}

void UBLSquadSubsystem::Unregister(ABLAIController* AI)
{
	Members.Remove(AI);
	ReleaseAttackToken(AI);
	ReleaseCover(AI);
}

bool UBLSquadSubsystem::RequestAttackToken(ABLAIController* AI)
{
	if (TokenHolders.Contains(AI))
	{
		return true;
	}
	if (TokenHolders.Num() >= MaxAttackTokens)
	{
		return false;
	}
	TokenHolders.Add(AI);
	MaxTokensSeen = FMath::Max(MaxTokensSeen, TokenHolders.Num());
	return true;
}

void UBLSquadSubsystem::ReleaseAttackToken(ABLAIController* AI)
{
	TokenHolders.Remove(AI);
}

bool UBLSquadSubsystem::IsCoverFree(const ABLCoverPoint* Cover, const ABLAIController* For) const
{
	const TWeakObjectPtr<ABLAIController>* Holder = Reservations.Find(Cover);
	return !Holder || !Holder->IsValid() || Holder->Get() == For;
}

void UBLSquadSubsystem::ReserveCover(ABLCoverPoint* Cover, ABLAIController* AI)
{
	ReleaseCover(AI);
	if (Cover)
	{
		Reservations.Add(Cover, AI);
	}
}

void UBLSquadSubsystem::ReleaseCover(ABLAIController* AI)
{
	for (auto It = Reservations.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid() || It.Value().Get() == AI)
		{
			It.RemoveCurrent();
		}
	}
}

void UBLSquadSubsystem::ReportTarget(ABLAIController* From, AActor* Target, const FVector& Location)
{
	const APawn* FromPawn = From ? From->GetPawn() : nullptr;
	const ABLEnemyCharacter* FromEnemy = Cast<ABLEnemyCharacter>(FromPawn);
	for (const TWeakObjectPtr<ABLAIController>& M : Members)
	{
		ABLAIController* AI = M.Get();
		if (!AI || AI == From || !AI->GetPawn())
		{
			continue;
		}
		const ABLEnemyCharacter* Other = Cast<ABLEnemyCharacter>(AI->GetPawn());
		const bool bSameSquad = FromEnemy && Other && !FromEnemy->SquadId.IsNone() && FromEnemy->SquadId == Other->SquadId;
		const bool bNear = FromPawn && FVector::Dist(FromPawn->GetActorLocation(), AI->GetPawn()->GetActorLocation()) < AlertRadius;
		if (bSameSquad || bNear)
		{
			AI->OnSquadAlert(Target, Location);
		}
	}
}

FVector UBLSquadSubsystem::GetMainAttackDirection(const AActor* Target) const
{
	FVector Sum = FVector::ZeroVector;
	for (const TWeakObjectPtr<ABLAIController>& M : Members)
	{
		const ABLAIController* AI = M.Get();
		if (AI && AI->GetPawn() && AI->IsInCombat() && !AI->IsFlanker() && Target)
		{
			Sum += (AI->GetPawn()->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		}
	}
	return Sum.GetSafeNormal2D();
}

void UBLSquadSubsystem::Tick(float DeltaTime)
{
	Members.RemoveAll([](const TWeakObjectPtr<ABLAIController>& M) { return !M.IsValid(); });
	TokenHolders.RemoveAll([](const TObjectPtr<ABLAIController>& T) { return !IsValid(T) || !T->GetPawn(); });

	// Flanqueo: con al menos 3 en combate, uno (el más alejado de ser útil disparando: sin turno) flanquea
	FlankTimer -= DeltaTime;
	if (FlankTimer > 0.f)
	{
		return;
	}
	FlankTimer = FlankInterval;
	TArray<ABLAIController*> Engaged;
	bool bHasFlanker = false;
	for (const TWeakObjectPtr<ABLAIController>& M : Members)
	{
		if (ABLAIController* AI = M.Get(); AI && AI->IsInCombat())
		{
			Engaged.Add(AI);
			bHasFlanker |= AI->IsFlanker();
		}
	}
	if (Engaged.Num() >= 3 && !bHasFlanker)
	{
		ABLAIController* Best = nullptr;
		for (ABLAIController* AI : Engaged)
		{
			if (!HasAttackToken(AI) && AI->CanStartFlank())
			{
				Best = AI;
				break;
			}
		}
		if (Best)
		{
			++FlankAssignments;
			if (!Best->StartFlank())
			{
				FlankTimer = 3.f;   // no había hueco: reintentar pronto (quizá con otro)
			}
		}
	}
}
