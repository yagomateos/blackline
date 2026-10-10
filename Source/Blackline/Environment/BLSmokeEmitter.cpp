#include "Environment/BLSmokeEmitter.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/BLHealthComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UInstancedStaticMeshComponent* MakeSpriteISM(AActor* Owner, const TCHAR* Name, UStaticMesh* Quad, UMaterialInterface* Mat)
	{
		UInstancedStaticMeshComponent* ISM = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetStaticMesh(Quad);
		ISM->SetMaterial(0, Mat);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		ISM->SetCanEverAffectNavigation(false);
		ISM->NumCustomDataFloats = 5;
		ISM->SetMobility(EComponentMobility::Movable);
		return ISM;
	}
}

ABLSmokeEmitter::ABLSmokeEmitter()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Quad(TEXT("/Game/FX/Meshes/SM_FX_Quad.SM_FX_Quad"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dust(TEXT("/Game/FX/Materials/M_FX_Dust.M_FX_Dust"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Flame(TEXT("/Game/FX/Materials/M_FX_Flame.M_FX_Flame"));
	static ConstructorHelpers::FObjectFinder<USoundBase> FireLoop(TEXT("/Game/Audio/Ambience/Zones/SW_AmbZ_Fire_Loop.SW_AmbZ_Fire_Loop"));
	SmokeISM = MakeSpriteISM(this, TEXT("Smoke"), Quad.Object, Dust.Object);
	SmokeISM->SetupAttachment(RootComponent);
	FlameISM = MakeSpriteISM(this, TEXT("Flames"), Quad.Object, Flame.Object);
	FlameISM->SetupAttachment(RootComponent);

	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(RootComponent);
	FireLight->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	FireLight->SetLightColor(FLinearColor(1.f, 0.48f, 0.16f));
	FireLight->SetAttenuationRadius(1300.f);
	FireLight->SetCastShadows(false);
	FireLight->SetVisibility(false);

	FireSound = CreateDefaultSubobject<UAudioComponent>(TEXT("FireSound"));
	FireSound->SetupAttachment(RootComponent);
	FireSound->SetSound(FireLoop.Object);
	FireSound->bAutoActivate = false;
}

int32 ABLSmokeEmitter::GetAliveParticles() const
{
	int32 N = 0;
	for (const FPuff& P : Puffs) { N += P.bAlive ? 1 : 0; }
	for (const FPuff& P : Flames) { N += P.bAlive ? 1 : 0; }
	return N;
}

TArray<TWeakObjectPtr<ABLSmokeEmitter>> ABLSmokeEmitter::SightBlockers;

void ABLSmokeEmitter::ConfigureSmokeScreen()
{
	MaxParticles = 30;
	Life = 14.f;
	StartSize = FVector2D(180.f, 260.f);
	EndSize = FVector2D(620.f, 900.f);
	RiseSpeed = 18.f;
	Wind = FVector(12.f, 6.f, 0.f);
	Color = FLinearColor(0.62f, 0.62f, 0.6f);
	Opacity = 0.82f;
	SpawnRadius = 220.f;
	SpawnDuration = 22.f;
	SightBlockRadius = 380.f;
}

bool ABLSmokeEmitter::BlocksSight(const FVector& From, const FVector& To)
{
	for (const TWeakObjectPtr<ABLSmokeEmitter>& S : SightBlockers)
	{
		const ABLSmokeEmitter* E = S.Get();
		if (!E || E->SightBlockRadius <= 0.f)
		{
			continue;
		}
		// La nube crece al principio y se deshace al dejar de emitir
		const float Grow = FMath::Clamp(E->Age / 2.5f, 0.f, 1.f);
		const float Fade = E->SpawnDuration > 0.f ? FMath::Clamp(1.f - (E->Age - E->SpawnDuration) / 6.f, 0.f, 1.f) : 1.f;
		const float R = E->SightBlockRadius * Grow * Fade;
		const FVector C = E->GetActorLocation() + FVector(0.f, 0.f, 120.f);
		if (R > 50.f && FMath::PointDistToSegment(C, From, To) < R)
		{
			return true;
		}
	}
	return false;
}

int32 ABLSmokeEmitter::GetActiveSmokeScreens()
{
	SightBlockers.RemoveAll([](const TWeakObjectPtr<ABLSmokeEmitter>& S) { return !S.IsValid(); });
	return SightBlockers.Num();
}

void ABLSmokeEmitter::EndPlay(const EEndPlayReason::Type Reason)
{
	SightBlockers.RemoveAll([this](const TWeakObjectPtr<ABLSmokeEmitter>& S) { return !S.IsValid() || S.Get() == this; });
	Super::EndPlay(Reason);
}

void ABLSmokeEmitter::BeginPlay()
{
	Super::BeginPlay();
	if (SightBlockRadius > 0.f)
	{
		SightBlockers.Add(this);
	}
	Puffs.SetNum(MaxParticles);
	SmokeISM->ClearInstances();
	for (int32 i = 0; i < MaxParticles; ++i)
	{
		SmokeISM->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
	}
	// La columna ya está ahí al empezar (no se ve nacer el humo); la cortina de una granada, en cambio, crece
	for (int32 i = 0; i < (SpawnDuration > 0.f ? 0 : MaxParticles); ++i)
	{
		SpawnPuff(true);
	}
	if (bFire)
	{
		Flames.SetNum(MaxFlames);
		FlameISM->ClearInstances();
		for (int32 i = 0; i < MaxFlames; ++i)
		{
			FlameISM->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
		}
		FireLight->SetVisibility(true);
		FireLight->SetAttenuationRadius(FireLightRadius);
		FireSound->Play(FMath::FRand() * 10.f);
	}
}

void ABLSmokeEmitter::SpawnPuff(bool bPrewarm)
{
	FPuff& P = Puffs[NextPuff];
	NextPuff = (NextPuff + 1) % Puffs.Num();
	const FVector2D R = FMath::RandPointInCircle(SpawnRadius);
	P.Location = GetActorLocation() + FVector(R.X, R.Y, bFire ? FireExtent.Z + 60.f : 0.f);
	P.Velocity = FVector(FMath::FRandRange(-0.15f, 0.15f) * RiseSpeed, FMath::FRandRange(-0.15f, 0.15f) * RiseSpeed, RiseSpeed * FMath::FRandRange(0.8f, 1.2f));
	P.Life = Life * FMath::FRandRange(0.8f, 1.2f);
	P.Age = bPrewarm ? FMath::FRand() * P.Life : 0.f;
	P.Size0 = FMath::FRandRange(StartSize.X, StartSize.Y);
	P.Size1 = FMath::FRandRange(EndSize.X, EndSize.Y);
	P.Frame = FMath::RandRange(0, 3);
	P.Roll = FMath::FRandRange(0.f, 2.f * PI);
	P.Spin = FMath::FRandRange(-0.08f, 0.08f);
	P.Shade = FMath::FRandRange(0.8f, 1.15f);
	P.bAlive = true;
	if (bPrewarm)
	{
		// Posición aproximada que tendría tras Age segundos (subida que frena + deriva del viento)
		const float T = P.Age;
		P.Location += FVector(0.f, 0.f, P.Velocity.Z * T) + Wind * T * 0.8f;
	}
}

void ABLSmokeEmitter::SpawnFlame()
{
	FPuff& P = Flames[NextFlame];
	NextFlame = (NextFlame + 1) % Flames.Num();
	P.Location = GetActorLocation() + FVector(FMath::FRandRange(-FireExtent.X, FireExtent.X), FMath::FRandRange(-FireExtent.Y, FireExtent.Y), FireExtent.Z);
	P.Velocity = FVector(FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(80.f, 150.f));
	P.Life = FMath::FRandRange(0.45f, 0.8f);
	P.Age = 0.f;
	P.Size0 = FlameSize * FMath::FRandRange(0.7f, 1.1f);
	P.Size1 = FlameSize * FMath::FRandRange(0.3f, 0.5f);
	P.Frame = FMath::RandRange(0, 3);
	P.Roll = FMath::FRandRange(-0.4f, 0.4f);
	P.Spin = FMath::FRandRange(-0.6f, 0.6f);
	P.Shade = FMath::FRandRange(0.7f, 1.2f);
	P.bAlive = true;
}

void ABLSmokeEmitter::UpdateSprites(TArray<FPuff>& List, UInstancedStaticMeshComponent* ISM, float DeltaTime, const FVector& View, bool bFlames)
{
	Scratch.Reset(List.Num());
	for (int32 i = 0; i < List.Num(); ++i)
	{
		FPuff& P = List[i];
		if (P.bAlive)
		{
			P.Age += DeltaTime;
			P.bAlive = P.Age < P.Life;
		}
		if (!P.bAlive)
		{
			Scratch.Add(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
			ISM->SetCustomDataValue(i, 0, 0.f, false);
			continue;
		}
		const float T = P.Age / P.Life;
		if (bFlames)
		{
			P.Velocity.Z += 120.f * DeltaTime;   // la llama acelera al subir y se estrecha
		}
		else
		{
			// Frena en horizontal; la subida (flotación) se mantiene
			const float Damp = FMath::Exp(-DeltaTime / 3.f);
			P.Velocity.X *= Damp;
			P.Velocity.Y *= Damp;
			// El viento empuja más arriba
			P.Location += Wind * DeltaTime * FMath::Clamp((P.Location.Z - GetActorLocation().Z) / 1500.f, 0.2f, 1.6f);
		}
		P.Location += P.Velocity * DeltaTime;
		P.Roll += P.Spin * DeltaTime;
		const float Size = FMath::Lerp(P.Size0, P.Size1, bFlames ? T : 1.f - FMath::Pow(1.f - T, 2.f));
		const float Alpha = bFlames
			? FMath::Clamp(T / 0.15f, 0.f, 1.f) * (1.f - T) * P.Shade
			: Opacity * FMath::Clamp(T / 0.12f, 0.f, 1.f) * FMath::Pow(1.f - T, 1.3f);
		const FVector ToView = (View - P.Location).GetSafeNormal();
		const FQuat Facing = FRotationMatrix::MakeFromX(ToView).ToQuat() * FQuat(FVector::XAxisVector, P.Roll);
		// Las llamas son más altas que anchas
		Scratch.Add(FTransform(Facing, P.Location - GetActorLocation(), FVector(1.f, Size / 100.f, Size / 100.f * (bFlames ? 1.6f : 1.f))));
		const FLinearColor C = bFlames ? FLinearColor(1.f, FMath::Lerp(0.75f, 0.3f, T), FMath::Lerp(0.35f, 0.05f, T)) : Color * P.Shade;
		ISM->SetCustomDataValue(i, 0, Alpha, false);
		ISM->SetCustomDataValue(i, 1, P.Frame, false);
		ISM->SetCustomDataValue(i, 2, C.R, false);
		ISM->SetCustomDataValue(i, 3, C.G, false);
		ISM->SetCustomDataValue(i, 4, C.B, false);
	}
	ISM->BatchUpdateInstancesTransforms(0, Scratch, false, true);
	ISM->MarkRenderStateDirty();
}

void ABLSmokeEmitter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager || Puffs.Num() == 0)
	{
		return;
	}
	const FVector View = PC->PlayerCameraManager->GetCameraLocation();
	Age += DeltaTime;
	const bool bEmitting = SpawnDuration <= 0.f || Age < SpawnDuration;
	if (!bEmitting && GetAliveParticles() == 0)
	{
		Destroy();
		return;
	}

	// La cortina de humo sale de golpe del bote (primeros 3 s) y luego se mantiene
	const float Burst = SpawnDuration > 0.f && Age < 3.f ? 5.f : 1.f;
	SpawnAccum += bEmitting ? DeltaTime * Burst * Puffs.Num() / FMath::Max(Life, 0.1f) : 0.f;
	while (SpawnAccum >= 1.f)
	{
		SpawnAccum -= 1.f;
		SpawnPuff(false);
	}
	UpdateSprites(Puffs, SmokeISM, DeltaTime, View, false);

	if (bFire && Flames.Num() > 0)
	{
		ApplyFireDamage(DeltaTime);
		FlameAccum += DeltaTime * Flames.Num() / 0.6f;
		while (FlameAccum >= 1.f)
		{
			FlameAccum -= 1.f;
			SpawnFlame();
		}
		UpdateSprites(Flames, FlameISM, DeltaTime, View, true);
		// Parpadeo: suma de senos a distinta frecuencia (no periódico a simple vista)
		FlickerTime += DeltaTime;
		const float F = 0.78f + 0.12f * FMath::Sin(FlickerTime * 11.f) + 0.07f * FMath::Sin(FlickerTime * 23.7f + 1.3f) + 0.05f * FMath::Sin(FlickerTime * 4.1f);
		FireLight->SetIntensity(LightIntensity * F);
		FireLight->SetRelativeLocation(FVector(FMath::Sin(FlickerTime * 7.f) * 8.f, FMath::Cos(FlickerTime * 5.3f) * 8.f, 90.f));
	}
}

void ABLSmokeEmitter::ApplyFireDamage(float DeltaTime)
{
	if (FireDamagePerSecond <= 0.f)
	{
		return;
	}
	FireDamageTimer += DeltaTime;
	if (FireDamageTimer < 0.25f)
	{
		return;
	}
	const float Step = FireDamageTimer;
	FireDamageTimer = 0.f;
	// Caja de las llamas: la base de FireExtent ampliada y hasta 1,8 m de alto (de pie dentro del fuego)
	const FVector Half(FireExtent.X + FireDamageMargin, FireExtent.Y + FireDamageMargin, 90.f);
	const FVector Center = GetActorLocation() + GetActorRotation().RotateVector(FVector(0.f, 0.f, 90.f - 20.f));
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, GetActorQuat(), Objects, FCollisionShape::MakeBox(Half));
	TSet<UBLHealthComponent*> Burned;
	for (const FOverlapResult& O : Overlaps)
	{
		APawn* Pawn = Cast<APawn>(O.GetActor());
		UBLHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UBLHealthComponent>() : nullptr;
		if (!Health || Health->IsDead() || Burned.Contains(Health))
		{
			continue;
		}
		Burned.Add(Health);
		FBLDamageInfo Info;
		Info.Amount = FireDamagePerSecond * Step;
		Info.Zone = EBLHitZone::Torso;
		Info.Location = Pawn->GetActorLocation();
		Info.Direction = (Pawn->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		Info.SourceLocation = GetActorLocation();
		Info.Causer = this;
		FireDamageDealt += Health->ApplyDamage(Info);
	}
}
