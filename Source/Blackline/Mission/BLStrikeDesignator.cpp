#include "Mission/BLStrikeDesignator.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "Mission/BLActivatable.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"
#include "Vehicles/BLBTR.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABLStrikeDesignator::ABLStrikeDesignator()
{
	PrimaryActorTick.bCanEverTick = true;
	Prompt = TEXT("Marcar el blindado para la aviación");
	HoldTime = 3.f;
	bPickup = false;
	UseSound = nullptr;
	for (int32 i = 1; i <= 3; ++i)
	{
		BlastSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_%02d.SW_Grenade_Explosion_%02d"), i, i)));
	}
}

AActor* ABLStrikeDesignator::FindTarget() const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(TargetTag) && !It->IsHidden())
		{
			return *It;
		}
	}
	return nullptr;
}

bool ABLStrikeDesignator::CanInteract(const ABLCharacter* User) const
{
	// Solo con el blindado en el puente (si no, no hay nada que marcar)
	const ABLBTR* B = Cast<ABLBTR>(FindTarget());
	return ABLInteractable::CanInteract(User) && B && B->HasArrived() && !B->IsVehicleDestroyed();
}

void ABLStrikeDesignator::Use(ABLCharacter* User)
{
	if (!CanInteract(User))
	{
		return;
	}
	Timer = 0.f;
	if (ABLMissionDirector* D = ABLMissionDirector::Get(this))
	{
		D->PlayRadio(MarkRadio);
	}
	UE_LOG(LogBlackline, Log, TEXT("[Apoyo] Blindado marcado: cazas en %.0f s"), Delay);
	Super::Use(User);
}

void ABLStrikeDesignator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Timer < 0.f || bStruck)
	{
		return;
	}
	Timer += DeltaTime;
	if (!bJets && Timer >= Delay)
	{
		bJets = true;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->Tags.Contains(JetTag))
			{
				if (IBLActivatable* A = Cast<IBLActivatable>(*It))
				{
					A->OnMissionActivate(JetTag);
				}
			}
		}
	}
	// La pasada: cinco explosiones en 1 s a lo largo del blindado y, con la última, el blindado revienta
	AActor* T = FindTarget();
	if (bJets && T && Timer >= Delay + StrikeDelay)
	{
		NextBlast -= DeltaTime;
		if (NextBlast <= 0.f)
		{
			NextBlast = 0.2f;
			const FVector P = T->GetActorLocation() + T->GetActorForwardVector() * FMath::Lerp(-600.f, 500.f, Blasts / 4.f)
				+ FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 50.f);
			if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
			{
				FX->SpawnExplosion(P, FVector::UpVector, nullptr);
			}
			if (BlastSounds.Num() > 0)
			{
				UGameplayStatics::PlaySoundAtLocation(this, BlastSounds[Blasts % BlastSounds.Num()], P, 1.f, 0.75f);
			}
			UGameplayStatics::ApplyRadialDamage(this, 400.f, P, 600.f, UDamageType::StaticClass(), TArray<AActor*>{ T }, this, nullptr, true, ECC_Visibility);
			if (++Blasts >= 5)
			{
				bStruck = true;
				if (ABLBTR* B = Cast<ABLBTR>(T))
				{
					B->DestroyVehicle();
				}
				const APlayerController* PC = GetWorld()->GetFirstPlayerController();
				if (ABLCharacter* Player = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr)
				{
					Player->OnNearbyExplosion(P, FMath::Clamp(1.f - FVector::Dist(Player->GetActorLocation(), P) / 6000.f, 0.25f, 1.f));
				}
				UE_LOG(LogBlackline, Log, TEXT("[Apoyo] Pasada sobre el blindado"));
			}
		}
	}
}
