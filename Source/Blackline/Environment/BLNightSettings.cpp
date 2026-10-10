#include "Environment/BLNightSettings.h"

#include "AI/BLEnemyCharacter.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"

#include "Components/LocalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

TWeakObjectPtr<ABLNightSettings> ABLNightSettings::Instance;

ABLNightSettings::ABLNightSettings()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABLNightSettings::BeginPlay()
{
	Super::BeginPlay();
	Instance = this;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(FName("BLLit")))
		{
			TArray<ULocalLightComponent*> Comps;
			It->GetComponents(Comps);
			for (ULocalLightComponent* L : Comps)
			{
				Lights.Add(L);
			}
		}
	}
}

void ABLNightSettings::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Instance.Get() == this)
	{
		Instance = nullptr;
	}
	Super::EndPlay(Reason);
}

void ABLNightSettings::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// El fogonazo del jugador: cualquier disparo le hace visible un segundo
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const ABLCharacter* P = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr;
	if (P && P->GetWeapon())
	{
		const int32 Shots = P->GetWeapon()->GetShotsFired();
		if (LastPlayerShots >= 0 && Shots != LastPlayerShots)
		{
			LastPlayerShotTime = GetWorld()->GetTimeSeconds();
		}
		LastPlayerShots = Shots;
	}
}

ABLNightSettings* ABLNightSettings::Find(const UWorld* World)
{
	ABLNightSettings* N = Instance.Get();
	return N && N->GetWorld() == World ? N : nullptr;
}

bool ABLNightSettings::InFlashlight(const FVector& Location) const
{
	const float CosCone = FMath::Cos(FMath::DegreesToRadians(24.f));
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const USpotLightComponent* F = It->GetFlashlight();
		if (!F || It->IsDead())
		{
			continue;
		}
		const FVector To = Location - F->GetComponentLocation();
		const float D = To.Size();
		if (D < FlashlightRange && FVector::DotProduct(To / FMath::Max(D, 1.f), F->GetForwardVector()) > CosCone)
		{
			return true;
		}
	}
	return false;
}

bool ABLNightSettings::IsLit(const UWorld* World, const FVector& Location)
{
	const ABLNightSettings* N = Find(World);
	if (!N)
	{
		return true;
	}
	for (const TWeakObjectPtr<ULocalLightComponent>& W : N->Lights)
	{
		const ULocalLightComponent* L = W.Get();
		if (!L || !L->IsVisible() || L->Intensity <= 0.f)
		{
			continue;
		}
		const FVector To = Location - L->GetComponentLocation();
		const float D = To.Size();
		if (D > L->AttenuationRadius * N->LitRadiusScale)
		{
			continue;
		}
		// Focos: solo dentro del cono
		if (const USpotLightComponent* Spot = Cast<USpotLightComponent>(L))
		{
			if (FVector::DotProduct(To / FMath::Max(D, 1.f), Spot->GetForwardVector()) < FMath::Cos(FMath::DegreesToRadians(Spot->OuterConeAngle)))
			{
				continue;
			}
		}
		return true;
	}
	return N->InFlashlight(Location);
}

float ABLNightSettings::GetVisibility(const UWorld* World, const FVector& Viewer, const AActor* Target, float Dist)
{
	const ABLNightSettings* N = Find(World);
	if (!N || !Target)
	{
		return 1.f;
	}
	const FVector Loc = Target->GetActorLocation();
	if (N->InFlashlight(Loc))
	{
		return N->FlashlightVisibility;
	}
	if (World->GetTimeSeconds() - N->LastPlayerShotTime < 1.f || IsLit(World, Loc))
	{
		return 1.f;
	}
	return Dist > N->DarkSightRadius ? 0.f : N->DarkVisibility;
}
