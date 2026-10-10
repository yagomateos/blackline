#include "Vehicles/BLHelicopter.h"

#include "Blackline.h"
#include "AI/BLEnemyCharacter.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "Mission/BLInteractable.h"
#include "Environment/BLSmokeEmitter.h"

#include "Components/AudioComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	const FVector RotorOffset(0.f, 0.f, 395.f);
	const FVector TailRotorOffset(-1090.f, 24.f, 380.f);
	const FVector DoorGunOffset(245.f, 150.f, 168.f);
	/** Donde se sube: junto a la puerta izquierda, en el peldaño. */
	const FVector BoardOffset(30.f, -165.f, 110.f);
}

ABLHelicopter::ABLHelicopter()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/Heli/SM_Heli_Body.SM_Heli_Body")));
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Rotor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rotor"));
	Rotor->SetupAttachment(Body);
	Rotor->SetRelativeLocation(RotorOffset);
	Rotor->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/Heli/SM_Heli_Rotor.SM_Heli_Rotor")));
	Rotor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rotor->SetCastShadow(true);

	TailRotor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailRotor"));
	TailRotor->SetupAttachment(Body);
	TailRotor->SetRelativeLocation(TailRotorOffset);
	TailRotor->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/Heli/SM_Heli_TailRotor.SM_Heli_TailRotor")));
	TailRotor->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Searchlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Searchlight"));
	Searchlight->SetupAttachment(Body);
	Searchlight->SetRelativeLocation(FVector(560.f, 0.f, 90.f));
	Searchlight->SetRelativeRotation(FRotator(-35.f, 0.f, 0.f));
	Searchlight->SetIntensityUnits(ELightUnits::Candelas);
	Searchlight->SetIntensity(900.f);
	Searchlight->SetAttenuationRadius(6000.f);
	Searchlight->SetInnerConeAngle(6.f);
	Searchlight->SetOuterConeAngle(14.f);
	Searchlight->SetLightColor(FLinearColor(0.9f, 0.95f, 1.f));
	Searchlight->SetCastShadows(false);

	RotorSound = CreateDefaultSubobject<UAudioComponent>(TEXT("RotorSound"));
	RotorSound->SetupAttachment(Body);
	RotorSound->SetRelativeLocation(RotorOffset);
	RotorSound->bAutoActivate = false;
	RotorSound->SetSound(BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Vehicles/SW_Heli_Rotor_Loop.SW_Heli_Rotor_Loop")));

	for (int32 i = 1; i <= 3; ++i)
	{
		DoorGunSounds.Add(BL::LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Vehicles/SW_Heli_DoorGun_%02d.SW_Heli_DoorGun_%02d"), i, i)));
	}
	SurfaceEffects = BL::LoadDefault<UBLSurfaceEffectsData>(TEXT("/Game/FX/DA_SurfaceEffects.DA_SurfaceEffects"));
}

void ABLHelicopter::BeginPlay()
{
	Super::BeginPlay();
	if (BodyMaterial)
	{
		for (int32 i = 0; i < Body->GetNumMaterials(); ++i)
		{
			if (Body->GetMaterialSlotNames().IsValidIndex(i) && Body->GetMaterialSlotNames()[i] == FName("MI_Heli_Paint"))
			{
				Body->SetMaterial(i, BodyMaterial);
			}
		}
	}
	if (Path.Num() > 0)
	{
		SetActorLocation(Path[0]);
		const FVector Next = Path.Num() > 1 ? Path[1] : HoverPoint;
		SetActorRotation(FRotator(0.f, (Next - Path[0]).Rotation().Yaw, 0.f));
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	// El sitio para subir no existe hasta que aterriza
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(InteractTag))
		{
			It->bEnabled = false;
		}
	}
}

void ABLHelicopter::OnMissionActivate(FName Tag)
{
	if (Tag.ToString().EndsWith(TEXT("_Land")))
	{
		bLandRequested = true;
		if (State == EState::Hidden)
		{
			// Activado directamente para aterrizar (pruebas o saltar fase): entra ya
			State = EState::Approach;
			SetActorHiddenInGame(false);
			SetActorEnableCollision(true);
			SetActorTickEnabled(true);
			RotorSound->Play(FMath::FRand() * 4.f);
		}
		UE_LOG(LogBlackline, Log, TEXT("[Heli] Orden de aterrizar"));
		return;
	}
	if (State == EState::Hidden)
	{
		State = EState::Waiting;
		StateTime = 0.f;
		SetActorTickEnabled(true);
		UE_LOG(LogBlackline, Log, TEXT("[Heli] Activado: llega en %.0f s"), ApproachDelay);
	}
}

bool ABLHelicopter::FlyTo(const FVector& Goal, float DeltaTime, float MaxSpeed, float Tolerance, bool bFaceGoal)
{
	const FVector Loc = GetActorLocation();
	const FVector To = Goal - Loc;
	const float Dist = To.Size();
	const float Decel = 520.f;
	const FVector Want = To.GetSafeNormal() * FMath::Min(MaxSpeed, FMath::Sqrt(2.f * Decel * FMath::Max(Dist - Tolerance * 0.5f, 0.f)));
	const FVector OldVel = Velocity;
	Velocity = FMath::VInterpConstantTo(Velocity, Want, DeltaTime, 750.f);
	SetActorLocation(Loc + Velocity * DeltaTime);

	// Rumbo: hacia donde va mientras corre; HoverYaw al pararse
	const float Speed2D = Velocity.Size2D();
	const float WantYaw = (bFaceGoal && Speed2D > 500.f) ? Velocity.Rotation().Yaw : HoverYaw;
	const float Yaw = FMath::FixedTurn(GetActorRotation().Yaw, WantYaw, (Speed2D > 500.f ? 25.f : 35.f) * DeltaTime);
	// Actitud: morro abajo al acelerar/correr, alabeo en los giros y frenadas laterales
	const FVector Accel = (Velocity - OldVel) / FMath::Max(DeltaTime, 1e-3f);
	const FRotator YawRot(0.f, Yaw, 0.f);
	const float FwdVel = FVector::DotProduct(Velocity, YawRot.Vector());
	const float FwdAcc = FVector::DotProduct(Accel, YawRot.Vector());
	const float SideAcc = FVector::DotProduct(Accel, FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y));
	const FRotator WantAtt(FMath::Clamp(-FwdVel / 2600.f * 10.f - FwdAcc / 750.f * 6.f, -16.f, 12.f), 0.f, FMath::Clamp(SideAcc / 750.f * 12.f, -18.f, 18.f));
	Attitude = FMath::RInterpTo(Attitude, WantAtt, DeltaTime, 2.5f);
	SetActorRotation(FRotator(Attitude.Pitch, Yaw, Attitude.Roll));
	return Dist < Tolerance && Velocity.Size() < 120.f;
}

void ABLHelicopter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	StateTime += DeltaTime;
	switch (State)
	{
	case EState::Waiting:
		if (StateTime >= ApproachDelay)
		{
			State = EState::Approach;
			StateTime = 0.f;
			PathIndex = 1;
			SetActorHiddenInGame(false);
			SetActorEnableCollision(true);
			RotorSound->Play(FMath::FRand() * 4.f);
			UE_LOG(LogBlackline, Log, TEXT("[Heli] Entra desde %s"), *GetActorLocation().ToCompactString());
		}
		return;
	case EState::Approach:
	{
		const bool bOnPath = Path.IsValidIndex(PathIndex);
		const FVector Goal = bOnPath ? Path[PathIndex] : HoverPoint;
		if (FlyTo(Goal, DeltaTime, Speed, bOnPath ? 600.f : 80.f, true) || (bOnPath && FVector::Dist(GetActorLocation(), Goal) < 900.f))
		{
			if (bOnPath)
			{
				++PathIndex;
			}
			else
			{
				State = EState::Hover;
				StateTime = 0.f;
				UE_LOG(LogBlackline, Log, TEXT("[Heli] En estacionario"));
			}
		}
		break;
	}
	case EState::Hover:
		FlyTo(HoverPoint + FVector(FMath::Sin(Bob * 0.6f) * 40.f, FMath::Sin(Bob * 0.37f) * 50.f, FMath::Sin(Bob * 0.9f) * 25.f), DeltaTime, 400.f, 80.f, false);
		TickDoorGun(DeltaTime);
		if (bLandRequested && StateTime > 1.f)
		{
			State = EState::Landing;
			StateTime = 0.f;
		}
		break;
	case EState::Landing:
		TickDoorGun(DeltaTime);
		if (FlyTo(LandPoint, DeltaTime, 650.f, 12.f, false))
		{
			SetActorLocation(LandPoint);
			Velocity = FVector::ZeroVector;
			State = EState::Landed;
			StateTime = 0.f;
			EnableBoarding();
		}
		break;
	case EState::Disabled:
	{
		// Aterrizaje forzoso: baja girando despacio sobre sí mismo hasta LandPoint y se queda humeando
		DisabledTime += DeltaTime;
		RotorRate = FMath::FInterpTo(RotorRate, 260.f, DeltaTime, 0.4f);
		const FVector Loc = GetActorLocation();
		FVector To = LandPoint - Loc;
		if (To.Size() > 10.f)
		{
			const FVector Step = To.GetSafeNormal() * FMath::Min(To.Size(), 380.f * DeltaTime);
			SetActorLocation(Loc + Step);
			SetActorRotation(FRotator(FMath::Sin(DisabledTime * 2.3f) * 6.f, GetActorRotation().Yaw + 40.f * DeltaTime, FMath::Sin(DisabledTime * 1.7f) * 8.f));
		}
		else
		{
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(4.f, GetActorRotation().Yaw, -6.f), DeltaTime, 2.f));
		}
		RotorSound->SetPitchMultiplier(FMath::FInterpTo(RotorSound->PitchMultiplier, 0.45f, DeltaTime, 0.5f));
		break;
	}
	case EState::Landed:
		Attitude = FMath::RInterpTo(Attitude, FRotator::ZeroRotator, DeltaTime, 3.f);
		SetActorRotation(FRotator(Attitude.Pitch, GetActorRotation().Yaw, Attitude.Roll));
		break;
	default:
		break;
	}
	Bob += DeltaTime;

	// Rotores (el principal a ~3 vueltas/s visibles: más rápido parpadea con el muestreo de los fotogramas)
	RotorSpin = FMath::Fmod(RotorSpin + RotorRate * DeltaTime, 360.f);
	Rotor->SetRelativeRotation(FRotator(0.f, RotorSpin, 0.f));
	TailRotor->SetRelativeRotation(FRotator(FMath::Fmod(RotorSpin * 4.7f, 360.f), 0.f, 0.f));
}

float ABLHelicopter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (!bDamageable || State == EState::Disabled || State == EState::Hidden || State == EState::Waiting)
	{
		return Applied;
	}
	Health -= Damage;
	if (Health <= 0.f)
	{
		State = EState::Disabled;
		DisabledTime = 0.f;
		Searchlight->SetVisibility(false);
		// Humo negro del motor (sigue al helicóptero)
		const FTransform At(GetActorLocation() + FVector(-100.f, 0.f, 330.f));
		if (ABLSmokeEmitter* S = GetWorld()->SpawnActorDeferred<ABLSmokeEmitter>(ABLSmokeEmitter::StaticClass(), At, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			S->MaxParticles = 20;
			S->Life = 6.f;
			S->StartSize = FVector2D(60.f, 100.f);
			S->EndSize = FVector2D(400.f, 700.f);
			S->RiseSpeed = 120.f;
			S->SpawnRadius = 40.f;
			S->Color = FLinearColor(0.04f, 0.035f, 0.03f);
			S->Opacity = 0.7f;
			UGameplayStatics::FinishSpawningActor(S, At);
			S->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		}
		UE_LOG(LogBlackline, Log, TEXT("[Heli] Inutilizado: aterrizaje forzoso"));
	}
	return Applied;
}

void ABLHelicopter::TickDoorGun(float DeltaTime)
{
	if (!bDoorGun)
	{
		return;
	}
	NextBurst -= DeltaTime;
	if (NextBurst > 0.f)
	{
		return;
	}
	NextBurst = FMath::FRandRange(DoorGunInterval.X, DoorGunInterval.Y);
	const FVector Gun = GetActorTransform().TransformPosition(DoorGunOffset);
	const FVector Right = GetActorRightVector();
	ABLEnemyCharacter* Best = nullptr;
	float BestDist = DoorGunRange;
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || It->EnemyRole == EBLEnemyRole::Ally || It->Tags.Contains(FName("BLVIP")) || It->Tags.Contains(FName("BLVarek")))
		{
			continue;
		}
		const FVector Chest = It->GetActorLocation() + FVector(0.f, 0.f, 40.f);
		const float D = FVector::Dist(Gun, Chest);
		// Solo el lado de la puerta (el ametrallador no dispara a través del fuselaje)
		if (D > BestDist || FVector::DotProduct((Chest - Gun).GetSafeNormal(), Right) < 0.2f)
		{
			continue;
		}
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLHeliGun), false, this);
		Q.AddIgnoredActor(*It);
		if (GetWorld()->LineTraceTestByChannel(Gun, Chest, ECC_Visibility, Q))
		{
			continue;
		}
		Best = *It;
		BestDist = D;
	}
	if (!Best)
	{
		return;
	}
	++Bursts;
	const FVector Chest = Best->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	if (DoorGunSounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DoorGunSounds[Bursts % DoorGunSounds.Num()], Gun, 1.f, FMath::FRandRange(0.96f, 1.04f));
	}
	// Impactos alrededor del blanco (la ráfaga levanta polvo aunque no todo acierte)
	UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>();
	for (int32 i = 0; i < 6 && FX && SurfaceEffects; ++i)
	{
		const FVector Dir = FMath::VRandCone((Chest - Gun).GetSafeNormal(), FMath::DegreesToRadians(2.2f));
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLHeliGunFX), false, this);
		Q.bReturnPhysicalMaterial = true;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Gun, Gun + Dir * (BestDist + 800.f), ECC_Visibility, Q) && Hit.GetActor() != Best)
		{
			FX->SpawnImpact(SurfaceEffects->Get(UBLSurfaceEffectsData::ResolveSurface(Hit)), Hit, Dir);
		}
	}
	FPointDamageEvent Ev(DoorGunDamage, FHitResult(), (Chest - Gun).GetSafeNormal(), nullptr);
	Best->TakeDamage(DoorGunDamage, Ev, nullptr, this);
	UE_LOG(LogBlackline, Log, TEXT("[Heli] Ráfaga a %s (%.0f m)"), *Best->GetName(), BestDist / 100.f);
}

void ABLHelicopter::EnableBoarding()
{
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(InteractTag))
		{
			It->SetActorLocation(GetActorTransform().TransformPosition(BoardOffset));
			It->bEnabled = true;
		}
	}
	UE_LOG(LogBlackline, Log, TEXT("[Heli] En tierra: se puede subir"));
}
