#include "Vehicles/BLBoat.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "Mission/BLInteractable.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
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

void ABLBoat::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	if (State == EState::Hidden)
	{
		return;
	}
	if (State == EState::Approach && Path.IsValidIndex(PathIndex))
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
	else
	{
		Bow = FMath::FInterpTo(Bow, 0.f, DeltaTime, 1.5f);
		Motor->SetPitchMultiplier(FMath::FInterpTo(Motor->PitchMultiplier, 0.75f, DeltaTime, 1.f));
	}
	// Mecerse en el agua: dos olas cruzadas
	const float Pitch = Bow + 1.2f * FMath::Sin(Time * 1.1f) + 0.5f * FMath::Sin(Time * 2.3f + 1.f);
	const float Roll = 1.8f * FMath::Sin(Time * 0.8f + 0.4f) + 0.6f * FMath::Sin(Time * 1.9f);
	Hull->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 4.f * FMath::Sin(Time * 1.3f)), FRotator(Pitch, 0.f, Roll));
}
