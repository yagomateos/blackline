#include "Weapons/BLGrenade.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "Environment/BLSmokeEmitter.h"
#include "Player/BLCharacter.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"

TArray<TWeakObjectPtr<ABLGrenade>> ABLGrenade::Live;
int32 ABLGrenade::ExplosionCount = 0;

ABLGrenade::ABLGrenade()
{
	PrimaryActorTick.bCanEverTick = true;
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->InitSphereRadius(4.5f);
	Sphere->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);          // no se queda enganchada en las cápsulas
	Sphere->SetSimulatePhysics(true);
	Sphere->SetNotifyRigidBodyCollision(true);
	Sphere->SetLinearDamping(0.15f);
	Sphere->SetAngularDamping(1.5f);
	// Solo el dato (SetMassOverrideInKg recalcula la masa y lee el material físico: en el CDO falla y rompe el cocinado)
	Sphere->BodyInstance.SetMassOverride(0.45f, true);
	Sphere->BodyInstance.bUseCCD = true;
	RootComponent = Sphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Weapons/Grenade/SM_Grenade_M6.SM_Grenade_M6")));

	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Sphere);
	Flash->SetIntensity(0.f);
	Flash->SetAttenuationRadius(1800.f);
	Flash->SetLightColor(FLinearColor(1.f, 0.6f, 0.3f));
	Flash->SetCastShadows(false);

	for (int32 i = 1; i <= 3; ++i)
	{
		ExplosionSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_%02d.SW_Grenade_Explosion_%02d"), i, i)));
		BounceSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Bounce_%02d.SW_Grenade_Bounce_%02d"), i, i)));
		DistantSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Ambience/Distant/SW_Dist_Explosion_%02d.SW_Dist_Explosion_%02d"), i, i)));
	}
	SurfaceEffects = BL::LoadDefault<UBLSurfaceEffectsData>(TEXT("/Game/FX/DA_SurfaceEffects.DA_SurfaceEffects"));
}

void ABLGrenade::Launch(const FVector& Velocity, AController* InInstigator, float InFuse)
{
	InstigatorController = InInstigator;
	FuseLeft = InFuse;
	Sphere->SetPhysicsLinearVelocity(Velocity);
	Sphere->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 600.f);
	Sphere->OnComponentHit.AddUniqueDynamic(this, &ABLGrenade::HandleHit);
	Live.Add(this);
}

void ABLGrenade::EndPlay(const EEndPlayReason::Type Reason)
{
	Live.RemoveAll([this](const TWeakObjectPtr<ABLGrenade>& G) { return !G.IsValid() || G.Get() == this; });
	Super::EndPlay(Reason);
}

void ABLGrenade::HandleHit(UPrimitiveComponent* HitComp, AActor* Other, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float Speed = Sphere->GetPhysicsLinearVelocity().Size();
	const float Now = GetWorld()->GetTimeSeconds();
	if (Speed > 80.f && Now - LastBounceTime > 0.12f && BounceSounds.Num() > 0)
	{
		LastBounceTime = Now;
		++Bounces;
		UGameplayStatics::PlaySoundAtLocation(this, BounceSounds[FMath::RandHelper(BounceSounds.Num())], Hit.ImpactPoint, FMath::Clamp(Speed / 600.f, 0.25f, 1.f));
		// El ruido delata la granada a los enemigos cercanos
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), Hit.ImpactPoint, 0.4f, GetOwner(), 1200.f, FName("Granada"));
	}
}

void ABLGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (FlashTime >= 0.f)
	{
		FlashTime += DeltaTime;
		Flash->SetIntensity(80000.f * FMath::Max(0.f, 1.f - FlashTime / 0.25f));
		if (FlashTime > 0.3f)
		{
			Destroy();
		}
		return;
	}
	FuseLeft -= DeltaTime;
	if (FuseLeft <= 0.f && !bExploded)
	{
		Explode();
	}
}

void ABLGrenade::Explode()
{
	if (bSmoke)
	{
		// Humo: la nube sale del bote en el suelo y dura ~25 s
		bExploded = true;
		Live.RemoveAll([this](const TWeakObjectPtr<ABLGrenade>& G) { return !G.IsValid() || G.Get() == this; });
		const FTransform At(GetActorLocation());
		if (ABLSmokeEmitter* Cloud = GetWorld()->SpawnActorDeferred<ABLSmokeEmitter>(ABLSmokeEmitter::StaticClass(), At, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Cloud->ConfigureSmokeScreen();
			UGameplayStatics::FinishSpawningActor(Cloud, At);
		}
		if (BounceSounds.Num() > 0)
		{
			UGameplayStatics::PlaySoundAtLocation(this, BounceSounds[0], GetActorLocation(), 0.8f, 0.6f);
		}
		++ExplosionCount;
		Destroy();
		return;
	}
	bExploded = true;
	++ExplosionCount;
	Live.RemoveAll([this](const TWeakObjectPtr<ABLGrenade>& G) { return !G.IsValid() || G.Get() == this; });
	const FVector Loc = GetActorLocation() + FVector(0.f, 0.f, 15.f);
	Sphere->SetSimulatePhysics(false);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false);
	FlashTime = 0.f;

	// Daño radial con caída; las paredes protegen (canal Visibility)
	APawn* InstigatorPawn = InstigatorController.IsValid() ? InstigatorController->GetPawn() : nullptr;
	UGameplayStatics::ApplyRadialDamageWithFalloff(this, BaseDamage, MinDamage, Loc, InnerRadius, OuterRadius, 1.f,
		UDamageType::StaticClass(), TArray<AActor*>(), InstigatorPawn ? static_cast<AActor*>(InstigatorPawn) : this, InstigatorController.Get(), ECC_Visibility);

	// Impulso a lo que simula física (ragdolls, armas caídas, cajas)
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Loc, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects), FCollisionShape::MakeSphere(OuterRadius));
	for (const FOverlapResult& O : Overlaps)
	{
		if (UPrimitiveComponent* C = O.GetComponent())
		{
			if (C->IsSimulatingPhysics() && C != Sphere)
			{
				C->AddRadialImpulse(Loc, OuterRadius, ImpulseStrength, RIF_Linear, true);
			}
		}
	}

	// Efectos según el suelo
	FHitResult Ground;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLGrenadeGround), false, this);
	Q.bReturnPhysicalMaterial = true;
	const bool bGround = GetWorld()->LineTraceSingleByChannel(Ground, Loc, Loc - FVector(0.f, 0.f, 120.f), ECC_Visibility, Q);
	const FBLSurfaceEffect* Surface = SurfaceEffects ? &SurfaceEffects->Get(bGround ? UBLSurfaceEffectsData::ResolveSurface(Ground) : EPhysicalSurface(SurfaceType1)) : nullptr;
	if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
	{
		FX->SpawnExplosion(bGround ? Ground.ImpactPoint : Loc, bGround ? Ground.ImpactNormal : FVector::UpVector, Surface);
	}

	// Sonido: cercano o lejano según el oyente; y sacudida al jugador
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const float Dist = PC && PC->PlayerCameraManager ? FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), Loc) : 0.f;
	const TArray<TObjectPtr<USoundBase>>& Sounds = Dist < 4500.f ? ExplosionSounds : DistantSounds;
	if (Sounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sounds[FMath::RandHelper(Sounds.Num())], Loc, 1.f, FMath::FRandRange(0.94f, 1.05f));
	}
	if (ABLCharacter* Player = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr)
	{
		Player->OnNearbyExplosion(Loc, FMath::Clamp(1.f - Dist / 2500.f, 0.f, 1.f));
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Loc, 1.f, InstigatorPawn ? static_cast<AActor*>(InstigatorPawn) : this, 6000.f, FName("Explosion"));
	UE_LOG(LogBlackline, Log, TEXT("[Granada] Explota en %s (a %.0f cm del jugador)"), *Loc.ToCompactString(), Dist);
}
