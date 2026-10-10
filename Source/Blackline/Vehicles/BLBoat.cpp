#include "Vehicles/BLBoat.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "Mission/BLInteractable.h"
#include "Player/BLCharacter.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABLBoat::ABLBoat()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/Boat/SM_Boat_RHIB.SM_Boat_RHIB")));
	Hull->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Hull->SetCanEverAffectNavigation(false);

	// Luz de navegación tapada (roja, muy tenue: se ve de cerca)
	NavLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("NavLight"));
	NavLight->SetupAttachment(Hull);
	NavLight->SetRelativeLocation(FVector(-180.f, 0.f, 150.f));
	NavLight->SetIntensityUnits(ELightUnits::Candelas);
	NavLight->SetIntensity(4.f);
	NavLight->SetAttenuationRadius(500.f);
	NavLight->SetLightColor(FLinearColor(1.f, 0.15f, 0.1f));
	NavLight->SetCastShadows(false);

	Motor = CreateDefaultSubobject<UAudioComponent>(TEXT("Motor"));
	Motor->SetupAttachment(Hull);
	Motor->SetRelativeLocation(FVector(-300.f, 0.f, 60.f));
	Motor->bAutoActivate = false;
	Motor->SetSound(BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Vehicles/SW_Boat_Outboard_Loop.SW_Boat_Outboard_Loop")));

	// De pie a popa, junto a la consola (el centro de la cápsula: cubierta + media altura)
	DriverSeat = CreateDefaultSubobject<USceneComponent>(TEXT("DriverSeat"));
	DriverSeat->SetupAttachment(Hull);
	DriverSeat->SetRelativeLocation(FVector(-170.f, 0.f, 118.f));
}

void ABLBoat::BeginPlay()
{
	Super::BeginPlay();
	Time = FMath::FRand() * 10.f;
	if (bStartDocked)
	{
		State = EState::Docked;
		NavLight->SetVisibility(false);
		return;
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
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(InteractTag))
		{
			It->bEnabled = false;
		}
	}
}

void ABLBoat::OnMissionActivate(FName Tag)
{
	if (Tag.ToString().EndsWith(TEXT("_Drive")))
	{
		StartDriving();
		return;
	}
	if (Tag.ToString().EndsWith(TEXT("_Board")))
	{
		bBoardRequested = true;
		if (State == EState::Docked)
		{
			EnableBoarding();
			return;
		}
	}
	if (State != EState::Hidden)
	{
		return;
	}
	State = Path.Num() > 1 ? EState::Approach : EState::Docked;
	PathIndex = 1;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	Motor->Play(FMath::FRand() * 3.f);
	UE_LOG(LogBlackline, Log, TEXT("[Lancha] En camino (%d puntos)"), Path.Num());
}

void ABLBoat::EnableBoarding()
{
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(InteractTag))
		{
			It->SetActorLocation(GetActorTransform().TransformPosition(BoardOffset));
			It->bEnabled = true;
		}
	}
	UE_LOG(LogBlackline, Log, TEXT("[Lancha] Se puede subir"));
}

void ABLBoat::StartDriving()
{
	// El interactuable de subir ya no hace falta
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(InteractTag))
		{
			It->bEnabled = false;
		}
	}
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (!Motor->IsPlaying())
	{
		Motor->Play();
	}
	State = EState::Driven;
	CurrentSpeed = 0.f;
	DriveYaw = GetActorRotation().Yaw;
	if (ABLCharacter* P = Cast<ABLCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		P->BoardBoat(this);
	}
	UE_LOG(LogBlackline, Log, TEXT("[Lancha] Pilotaje: W/S acelerador, A/D timón"));
}

void ABLBoat::AddDriveInput(float Throttle, float Steer)
{
	DriveInput.X = FMath::Clamp(DriveInput.X + Throttle, -1.f, 1.f);
	DriveInput.Y = FMath::Clamp(DriveInput.Y + Steer, -1.f, 1.f);
}

void ABLBoat::OnDriverLeft()
{
	CurrentSpeed = 0.f;
	DriveInput = FVector2D::ZeroVector;
}

void ABLBoat::TickDrive(float DeltaTime)
{
	ABLCharacter* P = Cast<ABLCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	// Si el jugador ha muerto y reaparecido, vuelve al timón
	if (P && !P->IsDead() && P->GetDrivenBoat() != this)
	{
		P->BoardBoat(this);
	}
	const bool bDriver = P && P->GetDrivenBoat() == this;
	const float Throttle = bDriver ? DriveInput.X : 0.f;
	const float Steer = bDriver ? DriveInput.Y : 0.f;
	DriveInput = FVector2D::ZeroVector;

	const float Want = Throttle > 0.f ? Throttle * DriveMaxSpeed : Throttle < 0.f ? Throttle * DriveReverseSpeed : 0.f;
	// Sin gas el agua frena sola; a contramarcha frena fuerte
	const bool bCounter = !FMath::IsNearlyZero(CurrentSpeed) && !FMath::IsNearlyZero(Want) && FMath::Sign(Want) != FMath::Sign(CurrentSpeed);
	const float Rate = FMath::IsNearlyZero(Throttle) ? DriveAccel * 0.6f : (bCounter ? DriveAccel * 1.8f : DriveAccel);
	const float OldSpeed = CurrentSpeed;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, Want, DeltaTime, Rate);
	// El timón gira más con velocidad (algo parado, por el empuje del motor); marcha atrás invierte
	const float TurnFactor = FMath::Clamp(FMath::Abs(CurrentSpeed) / 500.f, 0.35f, 1.f) * (CurrentSpeed < -10.f ? -1.f : 1.f);
	DriveYaw += Steer * DriveTurnRate * TurnFactor * DeltaTime;

	const FRotator Rot(0.f, DriveYaw, 0.f);
	const FVector Loc = GetActorLocation();
	const FVector Delta = Rot.Vector() * CurrentSpeed * DeltaTime;
	// Choques: caja del casco por encima del agua contra lo estático (muelles, pilotes, muros invisibles)
	const FVector Lift(0.f, 0.f, HullExtent.Z + 20.f);
	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLBoatDrive), false, this);
	if (P)
	{
		Q.AddIgnoredActor(P);
	}
	FHitResult Hit;
	FVector New = Loc + Delta;
	if (!Delta.IsNearlyZero() && GetWorld()->SweepSingleByObjectType(Hit, Loc + Lift, New + Lift, Rot.Quaternion(), Obj, FCollisionShape::MakeBox(HullExtent), Q))
	{
		if (Hit.bStartPenetrating)
		{
			// Ya tocando (al salir del embarcadero): solo deja alejarse de la superficie
			const bool bAway = Hit.ImpactNormal.IsNearlyZero() || FVector::DotProduct(Delta, Hit.ImpactNormal) > 0.f;
			New = bAway ? New : Loc;
		}
		else
		{
			// Desliza a lo largo de lo que toca y pierde velocidad
			const FVector Remaining = Delta * (1.f - Hit.Time);
			New = Loc + Delta * FMath::Max(Hit.Time - 0.02f, 0.f) + FVector::VectorPlaneProject(Remaining, Hit.ImpactNormal.GetSafeNormal2D()) * 0.5f;
			if (BumpCooldown <= 0.f && FMath::Abs(CurrentSpeed) > 300.f)
			{
				UE_LOG(LogBlackline, Log, TEXT("[Lancha] Choque con %s a %.0f cm/s"), *GetNameSafe(Hit.GetActor()), CurrentSpeed);
				BumpCooldown = 0.6f;
			}
			CurrentSpeed *= 0.55f;
		}
	}
	BumpCooldown -= DeltaTime;
	New.Z = Loc.Z;
	SetActorLocationAndRotation(New, Rot);

	Bow = FMath::FInterpTo(Bow, FMath::Clamp(CurrentSpeed / DriveMaxSpeed * 6.f + (CurrentSpeed - OldSpeed) / FMath::Max(DeltaTime, 1e-3f) * 0.006f, -2.f, 8.f), DeltaTime, 2.f);
	Motor->SetPitchMultiplier(0.75f + 0.65f * FMath::Abs(CurrentSpeed) / DriveMaxSpeed + 0.15f * FMath::Abs(Throttle));
}

void ABLBoat::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	if (State == EState::Hidden)
	{
		return;
	}
	if (State == EState::Driven)
	{
		TickDrive(DeltaTime);
	}
	else if (State == EState::Approach && Path.IsValidIndex(PathIndex))
	{
		const FVector Loc = GetActorLocation();
		const FVector To = Path[PathIndex] - Loc;
		const float Dist = To.Size2D();
		const bool bLast = PathIndex == Path.Num() - 1;
		const float Want = bLast ? FMath::Min(Speed, FMath::Sqrt(2.f * 160.f * FMath::Max(Dist - 10.f, 0.f))) : Speed;
		const float OldSpeed = CurrentSpeed;
		CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, Want, DeltaTime, 260.f);
		const float Yaw = FMath::FixedTurn(GetActorRotation().Yaw, To.Rotation().Yaw, 30.f * DeltaTime);
		FVector New = Loc + FRotator(0.f, Yaw, 0.f).Vector() * CurrentSpeed * DeltaTime;
		New.Z = Path[PathIndex].Z;
		SetActorLocation(New);
		SetActorRotation(FRotator(GetActorRotation().Pitch, Yaw, GetActorRotation().Roll));
		// Proa arriba al acelerar y a velocidad de planeo
		Bow = FMath::FInterpTo(Bow, FMath::Clamp(CurrentSpeed / Speed * 4.f + (CurrentSpeed - OldSpeed) / FMath::Max(DeltaTime, 1e-3f) * 0.01f, -2.f, 7.f), DeltaTime, 2.f);
		Motor->SetPitchMultiplier(0.8f + 0.5f * CurrentSpeed / Speed);
		if (Dist < (bLast ? 25.f : 300.f))
		{
			++PathIndex;
			if (bLast)
			{
				State = EState::Docked;
				CurrentSpeed = 0.f;
				UE_LOG(LogBlackline, Log, TEXT("[Lancha] Al pie del embarcadero"));
				if (bBoardRequested)
				{
					EnableBoarding();
				}
			}
		}
	}
	else if (State == EState::Docked)
	{
		Bow = FMath::FInterpTo(Bow, 0.f, DeltaTime, 1.5f);
		Motor->SetPitchMultiplier(FMath::FInterpTo(Motor->PitchMultiplier, 0.75f, DeltaTime, 1.f));
	}
	// Mecerse en el agua: dos olas cruzadas
	const float Pitch = Bow + 1.2f * FMath::Sin(Time * 1.1f) + 0.5f * FMath::Sin(Time * 2.3f + 1.f);
	const float Roll = 1.8f * FMath::Sin(Time * 0.8f + 0.4f) + 0.6f * FMath::Sin(Time * 1.9f);
	Hull->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 4.f * FMath::Sin(Time * 1.3f)), FRotator(Pitch, 0.f, Roll));
}
