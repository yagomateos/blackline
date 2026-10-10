#include "Vehicles/BLDrone.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "Mission/BLMissionDirector.h"

#include "Components/AudioComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

namespace
{
	/** Brazos del cuadricóptero (la malla del cuerpo los lleva; aquí van las hélices). */
	const FVector PropOffsets[4] = { FVector(38.f, 38.f, 12.f), FVector(38.f, -38.f, 12.f), FVector(-38.f, 38.f, 12.f), FVector(-38.f, -38.f, 12.f) };
}

ABLDrone::ABLDrone()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/Drone/SM_Drone_Body.SM_Drone_Body")));
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Body->SetCanEverAffectNavigation(false);
	UStaticMesh* PropMesh = BL::LoadDefault<UStaticMesh>(TEXT("/Game/Vehicles/Drone/SM_Drone_Prop.SM_Drone_Prop"));
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Prop%d"), i));
		P->SetupAttachment(Body);
		P->SetRelativeLocation(PropOffsets[i]);
		P->SetStaticMesh(PropMesh);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Props.Add(P);
	}

	Spot = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spot"));
	Spot->SetupAttachment(Body);
	Spot->SetRelativeLocationAndRotation(FVector(10.f, 0.f, -8.f), FRotator(-55.f, 0.f, 0.f));
	Spot->SetIntensityUnits(ELightUnits::Candelas);
	Spot->SetIntensity(1800.f);
	Spot->SetAttenuationRadius(4200.f);
	Spot->SetLightColor(FLinearColor(0.85f, 0.92f, 1.f));
	Spot->SetCastShadows(false);
	Spot->SetVolumetricScatteringIntensity(2.5f);

	Buzz = CreateDefaultSubobject<UAudioComponent>(TEXT("Buzz"));
	Buzz->SetupAttachment(Body);
	Buzz->bAutoActivate = false;
	Buzz->SetSound(BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Vehicles/SW_Drone_Buzz_Loop.SW_Drone_Buzz_Loop")));
	CrashSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_02.SW_Grenade_Explosion_02"));
	AlertSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Vehicles/SW_Drone_Alert.SW_Drone_Alert"));
}

void ABLDrone::BeginPlay()
{
	Super::BeginPlay();
	// Aquí y no en el constructor: SetPhysMaterialOverride lee los materiales físicos y en el CDO falla (rompe el cocinado)
	if (UPhysicalMaterial* Metal = LoadObject<UPhysicalMaterial>(nullptr, TEXT("/Game/Environment/PhysicalMaterials/PM_Metal.PM_Metal")))
	{
		Body->SetPhysMaterialOverride(Metal);
	}
	Spot->SetOuterConeAngle(ConeAngle);
	Spot->SetInnerConeAngle(ConeAngle * 0.6f);
	if (Path.Num() > 0)
	{
		SetActorLocation(Path[0]);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ABLDrone::OnMissionActivate(FName Tag)
{
	if (State != EState::Hidden)
	{
		return;
	}
	State = EState::Patrol;
	PathIndex = Path.Num() > 1 ? 1 : 0;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	Buzz->Play(FMath::FRand() * 3.f);
	UE_LOG(LogBlackline, Log, TEXT("[Dron] Activado: %d puntos de ruta"), Path.Num());
}

void ABLDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PropSpin = FMath::Fmod(PropSpin + 2900.f * DeltaTime, 360.f);
	for (int32 i = 0; i < Props.Num() && State != EState::Wreck; ++i)
	{
		Props[i]->SetRelativeRotation(FRotator(0.f, (i % 2 ? -PropSpin : PropSpin), 0.f));
	}
	if (State == EState::Patrol)
	{
		TickPatrol(DeltaTime);
	}
	else if (State == EState::Falling)
	{
		TickFalling(DeltaTime);
	}
}

void ABLDrone::TickPatrol(float DeltaTime)
{
	// Vuelo: hacia el siguiente punto, frenando al llegar; ligero cabeceo/alabeo con la aceleración
	if (Path.Num() > 0)
	{
		const FVector Goal = Path[PathIndex];
		const FVector To = Goal - GetActorLocation();
		const FVector Want = To.GetSafeNormal() * FMath::Min(Speed, FMath::Sqrt(2.f * 300.f * To.Size()));
		const FVector Old = Velocity;
		Velocity = FMath::VInterpConstantTo(Velocity, Want, DeltaTime, 400.f);
		SetActorLocation(GetActorLocation() + Velocity * DeltaTime);
		if (To.Size() < 150.f)
		{
			PathIndex = (PathIndex + 1) % Path.Num();
		}
		const FVector Acc = (Velocity - Old) / FMath::Max(DeltaTime, 1e-3f);
		const float Yaw = Velocity.Size2D() > 50.f ? Velocity.Rotation().Yaw : GetActorRotation().Yaw;
		const FRotator Flat(0.f, Yaw, 0.f);
		const float Pitch = FMath::Clamp(-FVector::DotProduct(Velocity, Flat.Vector()) / Speed * 12.f, -15.f, 15.f);
		const float Roll = FMath::Clamp(FVector::DotProduct(Acc, FRotationMatrix(Flat).GetUnitAxis(EAxis::Y)) / 400.f * 10.f, -12.f, 12.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(Pitch, FMath::FixedTurn(GetActorRotation().Yaw, Yaw, 90.f * DeltaTime), Roll), DeltaTime, 3.f));
	}

	// Foco que barre: a izquierda y derecha y algo arriba y abajo (relativo al dron)
	SweepTime += DeltaTime;
	Spot->SetRelativeRotation(FRotator(-55.f + 12.f * FMath::Sin(SweepTime * 0.7f), 35.f * FMath::Sin(SweepTime * 0.45f), 0.f));

	// Detección: el jugador dentro del cono del foco, a tiro y sin nada en medio
	AlertCooldown -= DeltaTime;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* P = PC ? PC->GetPawn() : nullptr;
	bool bSeen = false;
	if (P)
	{
		const FVector From = Spot->GetComponentLocation();
		const FVector To = P->GetActorLocation() - From;
		const float D = To.Size();
		if (D < DetectRange && FVector::DotProduct(To / FMath::Max(D, 1.f), Spot->GetForwardVector()) > FMath::Cos(FMath::DegreesToRadians(ConeAngle)))
		{
			FCollisionQueryParams Q(SCENE_QUERY_STAT(BLDroneSight), false, this);
			Q.AddIgnoredActor(P);
			bSeen = !GetWorld()->LineTraceTestByChannel(From, P->GetActorLocation(), ECC_Visibility, Q);
		}
	}
	SpotTime = bSeen ? SpotTime + DeltaTime : FMath::Max(0.f, SpotTime - DeltaTime * 0.5f);
	if (bSeen && SpotTime >= DetectTime && AlertCooldown <= 0.f)
	{
		Detect(P);
	}
	// El foco se pone rojo mientras te tiene
	Spot->SetLightColor(FMath::Lerp(FLinearColor(0.85f, 0.92f, 1.f), FLinearColor(1.f, 0.25f, 0.2f), FMath::Clamp(SpotTime / DetectTime, 0.f, 1.f)));
}

void ABLDrone::Detect(APawn* Player)
{
	++Detections;
	AlertCooldown = 8.f;
	SpotTime = 0.f;
	const FVector Loc = Player->GetActorLocation();
	int32 Alerted = 0;
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), Loc) < AlertRadius)
		{
			if (ABLAIController* AI = Cast<ABLAIController>(It->GetController()))
			{
				AI->OnSquadAlert(Player, Loc);
				++Alerted;
			}
		}
	}
	if (AlertSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AlertSound, GetActorLocation());
	}
	if (ABLMissionDirector* D = ABLMissionDirector::Get(this))
	{
		D->TriggerAlarm(TEXT("dron"));
	}
	UE_LOG(LogBlackline, Log, TEXT("[Dron] Te ha visto: %d milicianos avisados"), Alerted);
}

float ABLDrone::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (State != EState::Patrol)
	{
		return Applied;
	}
	Health -= Damage;
	if (Health <= 0.f)
	{
		// Derribado: se apaga el foco, el zumbido cae de tono y empieza a girar sobre sí mismo
		State = EState::Falling;
		Spot->SetIntensity(0.f);
		Buzz->SetPitchMultiplier(0.7f);
		Spin = FRotator(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(380.f, 560.f), FMath::FRandRange(-90.f, 90.f));
		Velocity += FVector(0.f, 0.f, 120.f);
		UE_LOG(LogBlackline, Log, TEXT("[Dron] Derribado"));
	}
	return Applied;
}

void ABLDrone::TickFalling(float DeltaTime)
{
	Velocity.Z -= 980.f * DeltaTime;
	const FVector From = GetActorLocation();
	const FVector To = From + Velocity * DeltaTime;
	AddActorWorldRotation(Spin * DeltaTime);
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLDroneFall), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Q))
	{
		SetActorLocation(Hit.ImpactPoint + Hit.ImpactNormal * 15.f);
		Shatter();
		return;
	}
	SetActorLocation(To);
}

void ABLDrone::Shatter()
{
	State = EState::Wreck;
	Buzz->Stop();
	if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
	{
		FX->SpawnExplosion(GetActorLocation(), FVector::UpVector, nullptr);
	}
	if (CrashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CrashSound, GetActorLocation(), 0.7f, 1.25f);
	}
	SetActorRotation(FRotator(FMath::FRandRange(-25.f, 25.f), GetActorRotation().Yaw, FMath::FRandRange(150.f, 180.f)));
	SetActorTickEnabled(false);
}
