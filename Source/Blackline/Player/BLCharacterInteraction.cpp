// ABLCharacter: interacción contextual con un botón (Bloque 6).
#include "Player/BLCharacter.h"

#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"

#include "Camera/CameraComponent.h"
#include "EngineUtils.h"

namespace
{
	constexpr float InteractRange = 210.f;     // cm desde la cámara
	constexpr float InteractAngle = 24.f;      // grados respecto a donde se mira
}

void ABLCharacter::SetInteractHeld(bool bHeld)
{
	// Con la misión completada, el botón de interacción reinicia
	if (bHeld)
	{
		if (ABLMissionDirector* Director = ABLMissionDirector::Get(this); Director && Director->IsMissionComplete() && Director->GetCompleteAge() > 2.f)
		{
			Director->RestartMission();
			return;
		}
	}
	bInteractHeld = bHeld && !bDead;
	if (!bInteractHeld)
	{
		InteractProgress = 0.f;
	}
}

ABLInteractable* ABLCharacter::GetFocusedInteractable() const
{
	return FocusedInteractable.Get();
}

void ABLCharacter::UpdateInteraction(float DeltaTime)
{
	// El objeto más centrado en la vista dentro del alcance (los objetos pequeños, como el disco, no exigen apuntar exacto)
	ABLInteractable* Best = nullptr;
	if (!bDead)
	{
		const FVector Eye = Camera->GetComponentLocation();
		const FVector Fwd = Camera->GetForwardVector();
		float BestCos = FMath::Cos(FMath::DegreesToRadians(InteractAngle));
		for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
		{
			if (!It->CanInteract(this))
			{
				continue;
			}
			const FVector To = It->GetActorLocation() - Eye;
			const float Dist = To.Size();
			const float Cos = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
			if (Dist > InteractRange || Cos < BestCos)
			{
				continue;
			}
			FCollisionQueryParams Q(SCENE_QUERY_STAT(BLInteract), false, this);
			Q.AddIgnoredActor(*It);
			if (GetWorld()->LineTraceTestByChannel(Eye, It->GetActorLocation(), ECC_Visibility, Q))
			{
				continue;   // hay algo en medio
			}
			Best = *It;
			BestCos = Cos;
		}
	}
	if (Best != FocusedInteractable.Get())
	{
		FocusedInteractable = Best;
		InteractProgress = 0.f;
	}
	if (!Best || !bInteractHeld)
	{
		InteractProgress = 0.f;
		return;
	}
	InteractProgress += Best->HoldTime > 0.f ? DeltaTime / Best->HoldTime : 1.f;
	if (InteractProgress >= 1.f)
	{
		InteractProgress = 0.f;
		bInteractHeld = false;
		Best->Use(this);
		FocusedInteractable = nullptr;
	}
}
