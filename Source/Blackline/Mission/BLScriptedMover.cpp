#include "Mission/BLScriptedMover.h"

#include "Blackline.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FX/BLFXSubsystem.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABLScriptedMover::ABLScriptedMover()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Loop = CreateDefaultSubobject<UAudioComponent>(TEXT("Loop"));
	Loop->SetupAttachment(Mesh);
	Loop->bAutoActivate = false;
}

void ABLScriptedMover::BeginPlay()
{
	Super::BeginPlay();
	if (LoopSound)
	{
		Loop->SetSound(LoopSound);
	}
	if (bHiddenUntilActive)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
	SetActorTickEnabled(false);
}

void ABLScriptedMover::OnMissionActivate(FName Tag)
{
	if (bActive)
	{
		return;
	}
	bActive = true;
	Delay = StartDelay;
	Index = 0;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (StartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StartSound, GetActorLocation(), StartSoundVolume);
	}
	if (bPhysicsFall)
	{
		// Voladura: las cargas, el daño a quien esté encima y el tramo que se parte y cae
		UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>();
		for (const FVector& P : BlastPoints)
		{
			if (FX)
			{
				FX->SpawnExplosion(P, FVector::UpVector, nullptr);
			}
			UGameplayStatics::ApplyRadialDamage(this, BlastDamage, P, BlastRadius, UDamageType::StaticClass(), TArray<AActor*>{ this }, this, nullptr, true, ECC_Visibility);
		}
		FallStart = GetActorLocation();
		FallStartRot = GetActorRotation();
		bFalling = true;
		FallTime = 0.f;
		SetActorTickEnabled(true);
		return;
	}
	if (Path.Num() > 0)
	{
		SetActorTickEnabled(true);
		if (LoopSound)
		{
			Loop->Play();
		}
	}
	else
	{
		bArrived = true;
	}
	UE_LOG(LogBlackline, Log, TEXT("[Guion] %s activado (%s)"), *GetName(), *Tag.ToString());
}

void ABLScriptedMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bFalling)
	{
		// Caída libre (con un arranque de 3 m/s de las cargas) hasta FallDepth; el giro acompaña a la caída
		FallTime += DeltaTime;
		const float Drop = FMath::Min(FallDepth, 300.f * FallTime + 0.5f * 980.f * FallTime * FallTime);
		const float A = FallDepth > 0.f ? Drop / FallDepth : 1.f;
		SetActorLocationAndRotation(FallStart - FVector(0.f, 0.f, Drop), FallStartRot + FallRotation * A, false, nullptr, ETeleportType::TeleportPhysics);
		if (Drop >= FallDepth)
		{
			SetActorTickEnabled(false);
			if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
			{
				FX->SpawnExplosion(GetActorLocation(), FVector::UpVector, nullptr);   // el golpe contra el agua
			}
			UE_LOG(LogBlackline, Log, TEXT("[Guion] %s: tramo en el río (%.0f cm)"), *GetName(), Drop);
		}
		return;
	}
	if (Delay > 0.f)
	{
		Delay -= DeltaTime;
		return;
	}
	if (!Path.IsValidIndex(Index))
	{
		return;
	}
	const FVector Loc = GetActorLocation();
	const FVector To = Path[Index] - Loc;
	const bool bLast = Index == Path.Num() - 1;
	const float Want = bLast ? FMath::Min(Speed, FMath::Sqrt(2.f * Acceleration * FMath::Max(To.Size2D() - 50.f, 0.f))) : Speed;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, Want, DeltaTime, Acceleration);
	const float Yaw = FMath::FixedTurn(GetActorRotation().Yaw, To.Rotation().Yaw, TurnRate * DeltaTime);
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
	// Avanza en la dirección del rumbo (un barco no se desliza de lado)
	FVector Step = FRotator(0.f, Yaw, 0.f).Vector() * CurrentSpeed * DeltaTime;
	Step.Z = FMath::Clamp(To.Z, -50.f * DeltaTime, 50.f * DeltaTime);
	SetActorLocation(Loc + Step);
	if (To.Size2D() < (bLast ? 80.f : 600.f))
	{
		++Index;
		if (!Path.IsValidIndex(Index))
		{
			bArrived = true;
			Loop->FadeOut(3.f, 0.f);
			if (bHideAtEnd)
			{
				SetActorHiddenInGame(true);
				SetActorEnableCollision(false);
			}
			SetActorTickEnabled(false);
			UE_LOG(LogBlackline, Log, TEXT("[Guion] %s: fin del recorrido"), *GetName());
		}
	}
}
