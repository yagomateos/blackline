#include "Mission/BLMortarBarrage.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "Player/BLCharacter.h"

#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"

ABLMortarBarrage::ABLMortarBarrage()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	WhistleSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/World/SW_Mortar_Whistle.SW_Mortar_Whistle"));
	for (int32 i = 1; i <= 3; ++i)
	{
		ExplosionSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_%02d.SW_Grenade_Explosion_%02d"), i, i)));
	}
}

void ABLMortarBarrage::OnMissionActivate(FName Tag)
{
	bActive = !Tag.ToString().EndsWith(TEXT("_Stop"));
	NextShot = 1.5f;
	UE_LOG(LogBlackline, Log, TEXT("[Mortero] %s %s"), *GetName(), bActive ? TEXT("empieza") : TEXT("para"));
}

void ABLMortarBarrage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int32 i = Incoming.Num() - 1; i >= 0; --i)
	{
		Incoming[i].Time -= DeltaTime;
		if (Incoming[i].Time <= 0.f)
		{
			const FVector P = Incoming[i].Point;
			Incoming.RemoveAt(i);
			Impact(P);
		}
	}
	if (!bActive)
	{
		return;
	}
	NextShot -= DeltaTime;
	if (NextShot <= 0.f)
	{
		NextShot = FMath::FRandRange(Interval.X, Interval.Y);
		Launch();
	}
}

void ABLMortarBarrage::Launch()
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* P = PC ? PC->GetPawn() : nullptr;
	const FVector C = GetActorLocation();
	FVector Point;
	if (P && FMath::FRand() < NearPlayerChance)
	{
		// Cerca del jugador para meter presión, nunca encima
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const float R = FMath::FRandRange(MinPlayerDistance, NearRadius);
		Point = P->GetActorLocation() + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
	}
	else
	{
		Point = C + FVector(FMath::FRandRange(-Extent.X, Extent.X), FMath::FRandRange(-Extent.Y, Extent.Y), 0.f);
	}
	// Al suelo (o a lo primero que haya debajo)
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLMortar), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0.f, 0.f, 3000.f), Point - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Q))
	{
		Point = Hit.ImpactPoint;
	}
	if (P && FVector::Dist2D(Point, P->GetActorLocation()) < MinPlayerDistance)
	{
		Point += (Point - P->GetActorLocation()).GetSafeNormal2D() * MinPlayerDistance;
	}
	Incoming.Add({ Point, WhistleTime });
	if (WhistleSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WhistleSound, Point + FVector(0.f, 0.f, 600.f), 1.f, FMath::FRandRange(0.92f, 1.06f));
	}
}

void ABLMortarBarrage::Impact(const FVector& Point)
{
	++Impacts;
	if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
	{
		FX->SpawnExplosion(Point, FVector::UpVector, nullptr);
	}
	UGameplayStatics::ApplyRadialDamageWithFalloff(this, Damage, 10.f, Point + FVector(0.f, 0.f, 30.f), InnerRadius, OuterRadius, 1.f,
		UDamageType::StaticClass(), TArray<AActor*>(), this, nullptr, ECC_Visibility);
	if (ExplosionSounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSounds[Impacts % ExplosionSounds.Num()], Point, 1.f, FMath::FRandRange(0.85f, 1.f));
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (ABLCharacter* Player = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr)
	{
		Player->OnNearbyExplosion(Point, FMath::Clamp(1.f - FVector::Dist(Player->GetActorLocation(), Point) / 3000.f, 0.f, 1.f));
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Point, 1.f, this, 6000.f, FName("Explosion"));
}
