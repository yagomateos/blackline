#include "Weapons/BLMountedGun.h"

#include "Blackline.h"
#include "Combat/BLDamageTypes.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "Player/BLCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"

ABLMountedGun::ABLMountedGun()
{
	PrimaryActorTick.bCanEverTick = true;
	Prompt = TEXT("Usar la ametralladora");
	HoldTime = 0.f;
	bPickup = false;
	UseSound = nullptr;
	Mesh->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Weapons/MountedGun/SM_MG_Tripod.SM_MG_Tripod")));
	Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gun"));
	Gun->SetupAttachment(Mesh);
	Gun->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Weapons/MountedGun/SM_MG_Gun.SM_MG_Gun")));
	Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Gun);
	Flash->SetIntensity(0.f);
	Flash->SetAttenuationRadius(1500.f);
	Flash->SetLightColor(FLinearColor(1.f, 0.65f, 0.3f));
	Flash->SetCastShadows(false);
	for (int32 i = 1; i <= 4; ++i)
	{
		FireSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Weapons/AR7/SW_AR7_Fire_Close_%02d.SW_AR7_Fire_Close_%02d"), i, i)));
	}
	OverheatSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/World/SW_MG_Overheat.SW_MG_Overheat"));
	SurfaceEffects = BL::LoadDefault<UBLSurfaceEffectsData>(TEXT("/Game/FX/DA_SurfaceEffects.DA_SurfaceEffects"));
}

void ABLMountedGun::BeginPlay()
{
	Super::BeginPlay();
	Gun->SetRelativeLocation(FVector(0.f, 0.f, PivotHeight));
	Flash->SetRelativeLocation(MuzzleOffset + FVector(25.f, 0.f, 0.f));
}

FVector ABLMountedGun::GetInteractLocation() const
{
	return GetActorTransform().TransformPosition(FVector(-40.f, 0.f, PivotHeight));
}

FVector ABLMountedGun::GetSeatLocation() const
{
	return GetActorTransform().TransformPosition(SeatOffset);
}

FVector ABLMountedGun::GetMuzzleLocation() const
{
	return Gun->GetComponentTransform().TransformPosition(MuzzleOffset);
}

FVector ABLMountedGun::GetBarrelDirection() const
{
	return Gun->GetForwardVector();
}

void ABLMountedGun::UpdateAimPoint()
{
	ABLCharacter* U = User.Get();
	if (!U)
	{
		return;
	}
	const FVector Eye = U->GetCamera()->GetComponentLocation();
	const FVector Dir = U->GetControlRotation().Vector();
	// Lo que queda por delante de la boca (el borde de la tronera) no cuenta: el cañón ya está más allá
	const float Skip = FVector::Dist(Eye, GetMuzzleLocation()) + 30.f;
	const FVector Start = Eye + Dir * Skip;
	const FVector End = Eye + Dir * Range;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLMountedGunAim), true, this);
	Q.AddIgnoredActor(U);
	AimPoint = BLDamage::WeaponTrace(GetWorld(), Hit, Start, End, Q) ? Hit.ImpactPoint : End;
}

bool ABLMountedGun::CanInteract(const ABLCharacter* InUser) const
{
	// Se puede montar todas las veces que haga falta (no se "gasta" como otros interactuables)
	return bEnabled && !User.IsValid();
}

void ABLMountedGun::Use(ABLCharacter* InUser)
{
	if (!CanInteract(InUser) || !InUser)
	{
		return;
	}
	InUser->MountGun(this);
	if (!bAnnounced)
	{
		bAnnounced = true;
		OnUsed.Broadcast(this, InUser);     // objetivo "toma la ametralladora"
	}
}

void ABLMountedGun::Mount(ABLCharacter* InUser)
{
	User = InUser;
	bTrigger = false;
	SetActorTickEnabled(true);
	UE_LOG(LogBlackline, Log, TEXT("[MG] Montada"));
}

void ABLMountedGun::Dismount()
{
	User = nullptr;
	bTrigger = false;
	UE_LOG(LogBlackline, Log, TEXT("[MG] Desmontada (%d disparos)"), ShotsFired);
}

void ABLMountedGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Enfriado (también desmontada) y fogonazo
	Heat = FMath::Max(0.f, Heat - CoolRate * DeltaTime * (bTrigger ? 0.3f : 1.f));
	if (OverheatTime > 0.f)
	{
		OverheatTime -= DeltaTime;
	}
	if (FlashTime >= 0.f)
	{
		FlashTime += DeltaTime;
		Flash->SetIntensity(FlashTime < 0.04f ? 40000.f : 0.f);
		if (FlashTime > 0.05f)
		{
			FlashTime = -1.f;
		}
	}
	Kick = FMath::FInterpTo(Kick, 0.f, DeltaTime, 14.f);
	ABLCharacter* U = User.Get();
	if (!U)
	{
		Gun->SetRelativeRotation(FMath::RInterpTo(Gun->GetRelativeRotation(), FRotator(-6.f, 0.f, 0.f), DeltaTime, 3.f));
		return;
	}
	// El cañón apunta al punto que marca la mira (desde el muñón, en el espacio del afuste)
	UpdateAimPoint();
	const FVector Pivot = Mesh->GetComponentTransform().TransformPosition(FVector(0.f, 0.f, PivotHeight));
	const FRotator LocalAim = Mesh->GetComponentTransform().InverseTransformVectorNoScale(AimPoint - Pivot).Rotation();
	const FRotator Rel(FMath::Clamp(FRotator::NormalizeAxis(LocalAim.Pitch), PitchMin - 2.f, PitchMax + 2.f),
		FMath::Clamp(FRotator::NormalizeAxis(LocalAim.Yaw), -YawLimit - 2.f, YawLimit + 2.f), 0.f);
	Gun->SetRelativeRotation(Rel + FRotator(Kick * 1.5f, 0.f, 0.f));
	Gun->SetRelativeLocation(FVector(-Kick * 4.f, 0.f, PivotHeight));
	if (bTrigger && OverheatTime <= 0.f)
	{
		FireTimer -= DeltaTime;
		while (FireTimer <= 0.f)
		{
			FireShot();
			FireTimer += 60.f / RoundsPerMinute;
			if (Heat >= 1.f)
			{
				OverheatTime = OverheatLock;
				Heat = 1.f;
				if (OverheatSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, OverheatSound, Gun->GetComponentLocation());
				}
				break;
			}
		}
	}
	else
	{
		FireTimer = FMath::Max(FireTimer, 0.f);
	}
}

void ABLMountedGun::FireShot()
{
	ABLCharacter* U = User.Get();
	if (!U)
	{
		return;
	}
	++ShotsFired;
	Heat += HeatPerShot;
	FlashTime = 0.f;
	Kick = FMath::Min(Kick + 0.35f, 1.f);
	// Desde la boca del cañón hacia el punto de la mira (lo que se ve es por donde va la bala), con dispersión
	UpdateAimPoint();
	FVector Origin = GetMuzzleLocation();
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLMountedGun), true, this);
	Q.AddIgnoredActor(U);
	Q.bReturnPhysicalMaterial = true;
	// Si la boca queda tapada (el borde de la tronera a mucha depresión), la bala sale desde los ojos
	FHitResult Block;
	const FVector Eye = U->GetCamera()->GetComponentLocation();
	if (GetWorld()->LineTraceSingleByChannel(Block, Eye, Origin, ECC_BLWeapon, Q))
	{
		Origin = Eye;
	}
	const FVector Dir = FMath::VRandCone((AimPoint - Origin).GetSafeNormal(), FMath::DegreesToRadians(SpreadDegrees * (1.f + Heat)));
	if (BLDamage::WeaponTrace(GetWorld(), Hit, Origin, Origin + Dir * Range, Q))
	{
		LastImpact = Hit.ImpactPoint;
		if (Hit.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, Dir, Hit, U->GetController(), this, UDamageType::StaticClass());
		}
		if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>(); FX && SurfaceEffects)
		{
			FX->SpawnImpact(SurfaceEffects->Get(UBLSurfaceEffectsData::ResolveSurface(Hit)), Hit, Dir);
		}
	}
	if (FireSounds.Num() > 0)
	{
		UGameplayStatics::PlaySound2D(this, FireSounds[ShotsFired % FireSounds.Num()], 0.9f, FMath::FRandRange(0.74f, 0.8f));
	}
	// Retroceso de cámara: sube y tiembla un poco
	if (APlayerController* PC = Cast<APlayerController>(U->GetController()))
	{
		PC->AddPitchInput(-FMath::FRandRange(0.08f, 0.16f));
		PC->AddYawInput(FMath::FRandRange(-0.08f, 0.08f));
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Gun->GetComponentLocation(), 1.f, U, 7000.f, FName("Gunfire"));
}
