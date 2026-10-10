#include "Mission/BLDoor.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "Core/BLAssetUtils.h"
#include "FX/BLFXSubsystem.h"
#include "Player/BLCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"

ABLDoor::ABLDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	Prompt = TEXT("Abrir");
	HoldTime = 0.f;
	bPickup = false;
	UseSound = nullptr;
	Mesh->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Environment/OldTown/SM_Door_Wood.SM_Door_Wood")));
	Mesh->SetCanEverAffectNavigation(false);
	Charge = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Charge"));
	Charge->SetupAttachment(Mesh);
	Charge->SetStaticMesh(BL::LoadDefault<UStaticMesh>(TEXT("/Game/Environment/OldTown/SM_BreachCharge.SM_BreachCharge")));
	Charge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Charge->SetVisibility(false);
	OpenSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/World/SW_Door_Open.SW_Door_Open"));
	KickSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/World/SW_Door_Kick.SW_Door_Kick"));
	BeepSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/World/SW_Breach_Beep.SW_Breach_Beep"));
	BlastSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_01.SW_Grenade_Explosion_01"));
}

void ABLDoor::BeginPlay()
{
	Super::BeginPlay();
	ClosedRotation = GetActorRotation();
	if (bBarred)
	{
		Prompt = TEXT("Colocar carga de brecha");
		HoldTime = 1.2f;
	}
	PrimaryActorTick.TickInterval = 0.f;
}

FVector ABLDoor::GetCenter() const
{
	return GetActorTransform().TransformPosition(FVector(0.f, 50.f, 105.f));
}

bool ABLDoor::CanInteract(const ABLCharacter* User) const
{
	return ABLInteractable::CanInteract(User) && ChargeTime < 0.f && !bOpen;
}

void ABLDoor::Use(ABLCharacter* User)
{
	if (!CanInteract(User))
	{
		return;
	}
	// Primero el padre (marca la puerta como usada y avisa al director: objetivos "abre la puerta"); después ya no
	// pasaría su CanInteract, que es el de la puerta y falla en cuanto hay carga puesta o la hoja está abierta
	Super::Use(User);
	if (bBarred)
	{
		// Carga en la cara del jugador; cuenta atrás con pitidos que se aceleran
		const FVector Local = GetActorTransform().InverseTransformPosition(User->GetActorLocation());
		Charge->SetRelativeLocation(FVector(Local.X > 0.f ? 5.f : -5.f, 50.f, 110.f));
		Charge->SetRelativeRotation(FRotator(0.f, Local.X > 0.f ? 0.f : 180.f, 0.f));
		Charge->SetVisibility(true);
		ChargeTime = 0.f;
		NextBeep = 0.f;
		Breacher = User;
		UE_LOG(LogBlackline, Log, TEXT("[Puerta] %s: carga colocada"), *GetName());
	}
	else
	{
		Open(User->GetActorLocation(), User->IsSprinting());
	}
}

void ABLDoor::Open(const FVector& From, bool bKick)
{
	if (bOpen)
	{
		return;
	}
	bOpen = true;
	// Se abre hacia el lado contrario a quien empuja (la hoja va en +Y desde la bisagra)
	const FVector Local = GetActorTransform().InverseTransformPosition(From);
	TargetAngle = Local.X > 0.f ? OpenAngle : -OpenAngle;
	Speed = bKick ? 650.f : 140.f;
	if (USoundBase* S = bKick ? KickSound : OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, GetCenter(), bKick ? 1.f : 0.6f, FMath::FRandRange(0.94f, 1.06f));
	}
	if (bKick)
	{
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetCenter(), 1.f, nullptr, 2500.f, FName("Door"));
		// Quien estuviera pegado al otro lado se lleva la puerta encima
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), GetCenter()) < 220.f)
			{
				if (ABLAIController* AI = Cast<ABLAIController>(It->GetController()))
				{
					AI->Stun(1.2f);
				}
			}
		}
	}
}

void ABLDoor::Detonate()
{
	ChargeTime = -1.f;
	bBreached = true;
	bOpen = true;
	Charge->SetVisibility(false);
	const FVector C = GetCenter();
	const ABLCharacter* User = Breacher.Get();
	const FVector Away = User ? (C - User->GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
	if (UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
	{
		FX->SpawnExplosion(C - FVector(0.f, 0.f, 80.f), FVector::UpVector, nullptr);
	}
	if (BlastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BlastSound, C, 0.9f, 1.15f);
	}
	// La hoja sale despedida hacia dentro
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);
	Mesh->AddImpulse(Away * 900.f + FVector(0.f, 0.f, 200.f), NAME_None, true);
	Mesh->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-400.f, 400.f), 0.f), NAME_None, true);
	// Daño a quien esté pegado a la puerta (por los dos lados) y aturdimiento de la habitación
	UGameplayStatics::ApplyRadialDamage(this, 60.f, C, 170.f, UDamageType::StaticClass(), TArray<AActor*>{ this }, this, nullptr, false, ECC_Visibility);
	int32 Stunned = 0;
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || FVector::Dist(It->GetActorLocation(), C) > StunRadius)
		{
			continue;
		}
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLBreach), false, this);
		Q.AddIgnoredActor(*It);
		if (!GetWorld()->LineTraceTestByChannel(C, It->GetActorLocation() + FVector(0.f, 0.f, 50.f), ECC_Visibility, Q))
		{
			if (ABLAIController* AI = Cast<ABLAIController>(It->GetController()))
			{
				AI->Stun(StunTime);
				if (ABLCharacter* B = Breacher.Get())
				{
					AI->OnSquadAlert(B, B->GetActorLocation());
				}
				++Stunned;
			}
		}
	}
	if (ABLCharacter* P = Breacher.Get())
	{
		P->OnNearbyExplosion(C, FMath::Clamp(1.f - FVector::Dist(P->GetActorLocation(), C) / 1500.f, 0.3f, 1.f));
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), C, 1.f, nullptr, 5000.f, FName("Explosion"));
	UE_LOG(LogBlackline, Log, TEXT("[Puerta] %s: brecha (%d aturdidos)"), *GetName(), Stunned);
}

void ABLDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (ChargeTime >= 0.f)
	{
		ChargeTime += DeltaTime;
		NextBeep -= DeltaTime;
		if (NextBeep <= 0.f && BeepSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, BeepSound, GetCenter(), 0.7f, 1.f + ChargeTime * 0.15f);
			NextBeep = FMath::Lerp(0.6f, 0.12f, FMath::Clamp(ChargeTime / ChargeDelay, 0.f, 1.f));
		}
		if (ChargeTime >= ChargeDelay)
		{
			Detonate();
		}
		return;
	}
	if (bBreached)
	{
		return;
	}
	// Abriéndose (o ya abierta): gira la hoja hasta su ángulo
	if (bOpen && !FMath::IsNearlyEqual(Angle, TargetAngle))
	{
		Angle = FMath::FixedTurn(Angle, TargetAngle, Speed * DeltaTime);
		SetActorRotation(ClosedRotation + FRotator(0.f, Angle, 0.f));
		return;
	}
	// La IA abre al llegar (cada 0,2 s)
	AITimer -= DeltaTime;
	if (!bOpen && !bBarred && AITimer <= 0.f)
	{
		AITimer = 0.2f;
		const FVector C = GetCenter();
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead() && FVector::Dist2D(It->GetActorLocation(), C) < 150.f && It->GetVelocity().Size2D() > 40.f)
			{
				Open(It->GetActorLocation(), false);
				break;
			}
		}
	}
}
