#include "Mission/BLHazardEvent.h"

#include "Blackline.h"
#include "Core/BLAssetUtils.h"
#include "Environment/BLSmokeEmitter.h"
#include "FX/BLFXSubsystem.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"

#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABLHazardEvent::ABLHazardEvent()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BlastSound = BL::LoadDefault<USoundBase>(TEXT("/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_03.SW_Grenade_Explosion_03"));
}

void ABLHazardEvent::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(BlockTag))
		{
			Blocks.Add(*It);
			BlockWasHidden.Add(It->IsHidden());
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}
	SetActorTickEnabled(false);
}

void ABLHazardEvent::OnMissionActivate(FName Tag)
{
	if (bArmed)
	{
		return;
	}
	bArmed = true;
	ArmedTime = 0.f;
	SetActorTickEnabled(true);
	UE_LOG(LogBlackline, Log, TEXT("[Evento] %s armado"), *GetName());
}

void ABLHazardEvent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bFired)
	{
		ArmedTime += DeltaTime;
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const APawn* P = PC ? PC->GetPawn() : nullptr;
		const bool bNear = TriggerRadius > 0.f && P && FVector::Dist(P->GetActorLocation(), GetActorLocation()) < TriggerRadius;
		if (ArmedTime >= Delay && (TriggerRadius <= 0.f || bNear))
		{
			Fire();
		}
		return;
	}
	// Daño por fuego dentro de las llamas (las paredes protegen)
	DamageTimer -= DeltaTime;
	if (DamageTimer <= 0.f)
	{
		DamageTimer = 0.25f;
		for (const FVector& Point : FirePoints)
		{
			UGameplayStatics::ApplyRadialDamage(this, FireDamagePerSecond * 0.25f, Point + FVector(0.f, 0.f, 60.f), FireRadius,
				UDamageType::StaticClass(), TArray<AActor*>(), this, nullptr, true, ECC_Visibility);
		}
	}
}

void ABLHazardEvent::Fire()
{
	bFired = true;
	UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>();
	for (int32 i = 0; i < FirePoints.Num(); ++i)
	{
		const FVector& Point = FirePoints[i];
		if (FX)
		{
			FX->SpawnExplosion(Point, FVector::UpVector, nullptr);
		}
		// Llamas que se quedan: fuego grande con humo negro
		const FTransform At(Point);
		if (ABLSmokeEmitter* E = GetWorld()->SpawnActorDeferred<ABLSmokeEmitter>(ABLSmokeEmitter::StaticClass(), At, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			E->bFire = true;
			E->FireDamagePerSecond = 0.f;   // el daño lo hace el propio evento (FireRadius), no cada llama
			E->FireExtent = FlameExtent;
			E->FlameSize = 120.f;
			E->MaxFlames = 22;
			E->LightIntensity = i == 0 ? 14000.f : 0.f;   // una sola luz (cuesta), las demás solo llamas
			E->MaxParticles = 18;
			E->Life = 8.f;
			E->StartSize = FVector2D(90.f, 150.f);
			E->EndSize = FVector2D(600.f, 900.f);
			E->RiseSpeed = 200.f;
			E->SpawnRadius = 90.f;
			E->Color = FLinearColor(0.06f, 0.055f, 0.05f);
			E->Opacity = 0.6f;
			UGameplayStatics::FinishSpawningActor(E, At);
			Fires.Add(E);
		}
	}
	for (int32 i = 0; i < Blocks.Num(); ++i)
	{
		if (AActor* B = Blocks[i].Get())
		{
			B->SetActorHiddenInGame(BlockWasHidden[i]);
			B->SetActorEnableCollision(true);
		}
	}
	if (BlastSound && FirePoints.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BlastSound, FirePoints[0], 1.f, 0.8f);
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (ABLCharacter* Player = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr)
	{
		const float D = FirePoints.Num() > 0 ? FVector::Dist(Player->GetActorLocation(), FirePoints[0]) : 5000.f;
		Player->OnNearbyExplosion(FirePoints.Num() > 0 ? FirePoints[0] : GetActorLocation(), FMath::Clamp(1.f - D / 3000.f, 0.2f, 1.f));
	}
	if (ABLMissionDirector* D = ABLMissionDirector::Get(this))
	{
		D->PlayRadio(Radio);
	}
	UE_LOG(LogBlackline, Log, TEXT("[Evento] %s: estalla (%d focos de fuego)"), *GetName(), FirePoints.Num());
}
