#include "Vehicles/BLBTR.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "Mission/BLMissionDirector.h"
#include "Environment/BLSmokeEmitter.h"
#include "Player/BLCharacter.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"

namespace
{
	const FVector TurretOffset(-20.f, 0.f, 215.f);
	const FVector GunOffset(60.f, 0.f, 27.f);
	const FVector MuzzleOffset(278.f, 0.f, 1.f);
}

ABLBTR::ABLBTR()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/BTR/SM_BTR_Hull.SM_BTR_Hull")));
	Hull->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Turret = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Turret"));
	Turret->SetupAttachment(Hull);
	Turret->SetRelativeLocation(TurretOffset);
	Turret->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/BTR/SM_BTR_Turret.SM_BTR_Turret")));
	Turret->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gun"));
	Gun->SetupAttachment(Turret);
	Gun->SetRelativeLocation(GunOffset);
	Gun->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/BTR/SM_BTR_Gun.SM_BTR_Gun")));
	Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Gun);
	MuzzleLight->SetRelativeLocation(MuzzleOffset + FVector(30.f, 0.f, 0.f));
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetAttenuationRadius(2500.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.62f, 0.32f));
	MuzzleLight->SetCastShadows(false);

	// Faro: se ve llegar por la calle a oscuras
	Headlight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Headlight"));
	Headlight->SetupAttachment(Hull);
	Headlight->SetRelativeLocation(FVector(420.f, 0.f, 130.f));
	Headlight->SetIntensityUnits(ELightUnits::Candelas);
	Headlight->SetIntensity(60.f);
	Headlight->SetAttenuationRadius(1800.f);
	Headlight->SetLightColor(FLinearColor(1.f, 0.9f, 0.7f));
	Headlight->SetCastShadows(false);

	Engine = CreateDefaultSubobject<UAudioComponent>(TEXT("Engine"));
	Engine->SetupAttachment(Hull);
	Engine->SetRelativeLocation(FVector(-250.f, 0.f, 120.f));
	Engine->bAutoActivate = false;
	Engine->SetSound(BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Vehicles/SW_BTR_Engine_Loop.SW_BTR_Engine_Loop")));

	for (int32 i = 1; i <= 3; ++i)
	{
		CannonSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Vehicles/SW_BTR_Cannon_%02d.SW_BTR_Cannon_%02d"), i, i)));
		ImpactSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_%02d.SW_Grenade_Explosion_%02d"), i, i)));
	}
	SurfaceEffects = BL::LoadDefault<UBLSurfaceEffectsData>(TEXT("/Game/FX/DA_SurfaceEffects.DA_SurfaceEffects"));
}

void ABLBTR::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(TargetTag))
		{
			Targets.Add(*It);
		}
	}
	// De oeste a este (el nivel pone el del derrumbe en medio: disparo 2)
	Targets.Sort([](const TWeakObjectPtr<AActor>& A, const TWeakObjectPtr<AActor>& B) { return A->GetActorLocation().X < B->GetActorLocation().X; });
	// Escombros de la escalera: escondidos hasta el derrumbe
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(BlockTag))
		{
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}
	if (Path.Num() > 0)
	{
		SetActorLocation(Path[0]);
		if (Path.Num() > 1)
		{
			SetActorRotation(FRotator(0.f, (Path[1] - Path[0]).Rotation().Yaw, 0.f));
		}
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ABLBTR::OnMissionActivate(FName Tag)
{
	if (State != EState::Hidden)
	{
		return;
	}
	State = Path.Num() > 1 ? EState::Driving : EState::Firing;
	StateTime = 0.f;
	PathIndex = 1;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	Engine->Play(FMath::FRand() * 4.f);
	UE_LOG(LogBlackline, Log, TEXT("[BTR] Activado (%s): %d puntos de ruta, %d blancos"), *Tag.ToString(), Path.Num(), Targets.Num());
}

void ABLBTR::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	StateTime += DeltaTime;
	switch (State)
	{
	case EState::Driving:
		TickDrive(DeltaTime);
		break;
	case EState::Firing:
		if (Targets.IsValidIndex(TargetIndex) && Targets[TargetIndex].IsValid())
		{
			AimPoint = Targets[TargetIndex]->GetActorLocation();
		}
		NextShot -= DeltaTime;
		if (TickAim(DeltaTime) && NextShot <= 0.f && StateTime > 1.5f)
		{
			Fire();
			TargetIndex = Targets.Num() > 0 ? (TargetIndex + 1) % Targets.Num() : 0;
			NextShot = ShotInterval;
			if (ShotsFired >= ScriptedShots)
			{
				State = EState::Suppress;
				StateTime = 0.f;
				NextShot = FMath::FRandRange(SuppressInterval.X, SuppressInterval.Y);
			}
		}
		break;
	case EState::Suppress:
	{
		NextShot -= DeltaTime;
		FVector T;
		if (PickSuppressTarget(T))
		{
			AimPoint = T;
			if (TickAim(DeltaTime) && NextShot <= 0.f)
			{
				Fire();
				NextShot = FMath::FRandRange(SuppressInterval.X, SuppressInterval.Y);
			}
		}
		break;
	}
	default:
		break;
	}

	// Fogonazo y retroceso del cañón
	if (FlashTime >= 0.f)
	{
		FlashTime += DeltaTime;
		MuzzleLight->SetIntensity(FlashTime < 0.08f ? 120000.f * (1.f - FlashTime / 0.08f) : 0.f);
		if (FlashTime > 0.1f)
		{
			FlashTime = -1.f;
		}
	}
	Recoil = FMath::FInterpTo(Recoil, 0.f, DeltaTime, 6.f);
	Gun->SetRelativeLocation(GunOffset - FVector(Recoil * 25.f, 0.f, 0.f));
	Hull->SetRelativeRotation(FRotator(-Recoil * 1.2f, 0.f, 0.f));

	// La fachada que cae: al cabo de unos segundos se congela (ya no cuesta física)
	if (Falling.Num() > 0)
	{
		FallingTime += DeltaTime;
		if (FallingTime > 8.f)
		{
			for (const TWeakObjectPtr<UPrimitiveComponent>& C : Falling)
			{
				if (C.IsValid())
				{
					C->SetSimulatePhysics(false);
				}
			}
			Falling.Reset();
		}
	}
}

void ABLBTR::DestroyVehicle()
{
	if (State == EState::Destroyed)
	{
		return;
	}
	State = EState::Destroyed;
	Engine->FadeOut(0.5f, 0.f);
	Headlight->SetVisibility(false);
	MuzzleLight->SetIntensity(0.f);
	// Torreta girada y cañón caído; arde con humo negro
	Turret->SetRelativeRotation(Turret->GetRelativeRotation() + FRotator(0.f, 25.f, 0.f));
	Gun->SetRelativeRotation(FRotator(-7.f, 0.f, 0.f));
	const FTransform At(GetActorLocation() + FVector(0.f, 0.f, 200.f));
	if (ABLSmokeEmitter* Flames = GetWorld()->SpawnActorDeferred<ABLSmokeEmitter>(ABLSmokeEmitter::StaticClass(), At, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Flames->bFire = true;
		Flames->FireExtent = FVector(260.f, 110.f, 30.f);
		Flames->FlameSize = 110.f;
		Flames->LightIntensity = 12000.f;
		Flames->FireLightRadius = 2200.f;
		Flames->MaxParticles = 26;
		Flames->Life = 10.f;
		Flames->StartSize = FVector2D(90.f, 140.f);
		Flames->EndSize = FVector2D(700.f, 1100.f);
		Flames->RiseSpeed = 170.f;
		Flames->SpawnRadius = 120.f;
		Flames->Color = FLinearColor(0.05f, 0.045f, 0.04f);
		Flames->Opacity = 0.7f;
		UGameplayStatics::FinishSpawningActor(Flames, At);
	}
	UE_LOG(LogBlackline, Log, TEXT("[BTR] Destruido"));
}

void ABLBTR::TickDrive(float DeltaTime)
{
	if (!Path.IsValidIndex(PathIndex))
	{
		State = EState::Firing;
		StateTime = 0.f;
		return;
	}
	const FVector Loc = GetActorLocation();
	const FVector To = Path[PathIndex] - Loc;
	const float Dist = To.Size2D();
	const bool bLast = PathIndex == Path.Num() - 1;
	// Frena al llegar al último punto
	const float Target = bLast ? FMath::Min(Speed, FMath::Sqrt(2.f * 220.f * FMath::Max(Dist - 20.f, 0.f))) : Speed;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, Target, DeltaTime, CurrentSpeed < Target ? 180.f : 320.f);
	const float Yaw = FMath::FixedTurn(GetActorRotation().Yaw, To.Rotation().Yaw, 35.f * DeltaTime);
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
	const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
	FVector NewLoc = Loc + Fwd * CurrentSpeed * DeltaTime;
	NewLoc.Z = FMath::FInterpTo(Loc.Z, Path[PathIndex].Z, DeltaTime, 2.f);
	SetActorLocation(NewLoc);
	if (Dist < (bLast ? 30.f : 250.f))
	{
		++PathIndex;
		if (bLast)
		{
			CurrentSpeed = 0.f;
			State = EState::Firing;
			StateTime = 0.f;
			UE_LOG(LogBlackline, Log, TEXT("[BTR] En posición en %s"), *GetActorLocation().ToCompactString());
		}
	}
	// Sonido: el motor sube de vueltas con la velocidad
	Engine->SetPitchMultiplier(0.85f + 0.3f * CurrentSpeed / FMath::Max(Speed, 1.f));
}

bool ABLBTR::TickAim(float DeltaTime)
{
	const FVector Pivot = Gun->GetComponentLocation();
	const FRotator Want = (AimPoint - Pivot).Rotation();
	const float HullYaw = GetActorRotation().Yaw;
	// Torreta: giro en guiñada (40 grados/s); cañón: elevación (-8..+35)
	FRotator TR = Turret->GetRelativeRotation();
	TR.Yaw = FMath::FixedTurn(TR.Yaw, FRotator::NormalizeAxis(Want.Yaw - HullYaw), 40.f * DeltaTime);
	Turret->SetRelativeRotation(TR);
	FRotator GR = Gun->GetRelativeRotation();
	GR.Pitch = FMath::FixedTurn(GR.Pitch, FMath::Clamp(Want.Pitch, -8.f, 35.f), 20.f * DeltaTime);
	Gun->SetRelativeRotation(GR);
	const FVector Dir = Gun->GetForwardVector();
	return FVector::DotProduct(Dir, (AimPoint - Pivot).GetSafeNormal()) > 0.9985f;
}

FVector ABLBTR::GetMuzzleLocation() const
{
	return Gun->GetComponentTransform().TransformPosition(MuzzleOffset);
}

bool ABLBTR::PickSuppressTarget(FVector& Out) const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* P = PC ? PC->GetPawn() : nullptr;
	if (!P || P->GetActorLocation().X > SuppressMaxPlayerX)
	{
		return false;
	}
	const FVector Eye = P->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLBTRSight), false, this);
	Q.AddIgnoredActor(P);
	if (GetWorld()->LineTraceTestByChannel(GetMuzzleLocation(), Eye, ECC_Visibility, Q))
	{
		return false;   // a cubierto: no gasta munición
	}
	// Apunta cerca, no al jugador: a 3,5-6 m por delante/detrás (la amenaza es el empujón para que se mueva)
	const FVector Side = FVector::CrossProduct((Eye - GetActorLocation()).GetSafeNormal2D(), FVector::UpVector);
	const float Seed = FMath::Fmod(GetWorld()->GetTimeSeconds() * 7.31f, 1.f);
	Out = Eye + Side * (Seed < 0.5f ? -1.f : 1.f) * FMath::Lerp(350.f, 600.f, Seed) + FVector(0.f, 0.f, 120.f);
	return true;
}

void ABLBTR::Fire()
{
	++ShotsFired;
	FlashTime = 0.f;
	Recoil = 1.f;
	const FVector Muzzle = GetMuzzleLocation();
	const FVector Dir = Gun->GetForwardVector();
	const FVector Spread = FMath::VRandCone(Dir, FMath::DegreesToRadians(0.4f));

	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLBTRShell), false, this);
	Q.bReturnPhysicalMaterial = true;
	const FVector End = Muzzle + Spread * 15000.f;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, End, ECC_Visibility, Q);
	const FVector Impact = bHit ? Hit.ImpactPoint : End;
	const FVector Normal = bHit ? FVector(Hit.ImpactNormal) : -Spread;

	// Daño en la explosión (las paredes protegen)
	UGameplayStatics::ApplyRadialDamageWithFalloff(this, ShellDamage, 10.f, Impact + Normal * 20.f, ShellInnerRadius, ShellOuterRadius, 1.f,
		UDamageType::StaticClass(), TArray<AActor*>{ this }, this, nullptr, ECC_Visibility);

	if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
	{
		const FBLSurfaceEffect* Surface = SurfaceEffects ? &SurfaceEffects->Get(bHit ? UBLSurfaceEffectsData::ResolveSurface(Hit) : EPhysicalSurface(SurfaceType1)) : nullptr;
		FX->SpawnExplosion(Impact, Normal, Surface);
		// Boca del cañón: bocanada de polvo y gases
		FX->SpawnExplosion(Muzzle + Dir * 60.f, Dir, nullptr);
	}

	// Sonido: el cañonazo en la boca, la explosión en el impacto
	if (CannonSounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CannonSounds[ShotsFired % CannonSounds.Num()], Muzzle, 1.f, FMath::FRandRange(0.95f, 1.04f));
	}
	if (ImpactSounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSounds[ShotsFired % ImpactSounds.Num()], Impact, 0.9f, FMath::FRandRange(0.9f, 1.f));
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (ABLCharacter* Player = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr)
	{
		const float D = FVector::Dist(Player->GetActorLocation(), Impact);
		Player->OnNearbyExplosion(Impact, FMath::Clamp(1.f - D / 3000.f, 0.f, 1.f));
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Muzzle, 1.f, this, 9000.f, FName("Explosion"));
	UE_LOG(LogBlackline, Log, TEXT("[BTR] Disparo %d -> %s"), ShotsFired, *Impact.ToCompactString());

	if (ShotsFired == CollapseShot && !bCollapsed)
	{
		Collapse(Impact);
	}
}

void ABLBTR::Collapse(const FVector& Impact)
{
	bCollapsed = true;
	int32 Pieces = 0;
	UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(CollapseTag))
		{
			// Trozos de fachada: caen hacia la calle girando
			if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(It->GetRootComponent()))
			{
				C->SetMobility(EComponentMobility::Movable);
				C->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
				C->SetSimulatePhysics(true);
				const FVector Out = (It->GetActorLocation() - Impact).GetSafeNormal2D() + FVector(0.f, -0.6f, 0.2f);
				C->AddImpulse(Out.GetSafeNormal() * FMath::FRandRange(250.f, 500.f), NAME_None, true);
				C->AddAngularImpulseInDegrees(FMath::VRand() * FMath::FRandRange(60.f, 180.f), NAME_None, true);
				Falling.Add(C);
				++Pieces;
				if (FX && Pieces % 2 == 1)
				{
					FX->SpawnExplosion(It->GetActorLocation(), FVector::UpVector, nullptr);
				}
			}
		}
		else if (It->Tags.Contains(BlockTag))
		{
			// Solo tapa la escalera si el jugador ya está por encima (si no, se quedaría encerrado abajo)
			const APlayerController* PC = GetWorld()->GetFirstPlayerController();
			const APawn* P = PC ? PC->GetPawn() : nullptr;
			if (!P || P->GetActorLocation().Z - 96.f < It->GetActorLocation().Z + 300.f)
			{
				continue;
			}
			It->SetActorHiddenInGame(false);
			It->SetActorEnableCollision(true);
			if (FX)
			{
				FX->SpawnExplosion(It->GetActorLocation(), FVector::UpVector, nullptr);
			}
		}
	}
	FallingTime = 0.f;
	if (ImpactSounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSounds[0], Impact, 1.f, 0.7f);   // rugido grave del derrumbe
	}
	if (ABLMissionDirector* D = ABLMissionDirector::Get(this))
	{
		D->PlayRadio(CollapseRadio);
	}
	UE_LOG(LogBlackline, Log, TEXT("[BTR] Derrumbe: %d trozos de fachada"), Pieces);
}
