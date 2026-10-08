#include "FX/BLFXSubsystem.h"

#include "Weapons/BLDebrisPoolComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr int32 SpriteCustomFloats = 5; // opacidad, fotograma, R, G, B
	constexpr int32 SparkCustomFloats = 4;  // intensidad, R, G, B

	USoundBase* PickSound(const TArray<TObjectPtr<USoundBase>>& Sounds)
	{
		return Sounds.Num() > 0 ? Sounds[FMath::RandHelper(Sounds.Num())].Get() : nullptr;
	}
}

bool UBLFXSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UBLFXSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBLFXSubsystem, STATGROUP_Tickables);
}

void UBLFXSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Sprites.SetNum(MaxSprites);
	Sparks.SetNum(MaxSparks);
	EnsureActor();
}

void UBLFXSubsystem::Deinitialize()
{
	FXActor = nullptr;
	Super::Deinitialize();
}

void UBLFXSubsystem::EnsureActor()
{
	if (FXActor || !GetWorld())
	{
		return;
	}
	QuadMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/FX/Meshes/SM_FX_Quad.SM_FX_Quad"));
	SpriteMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FX/Materials/M_FX_Dust.M_FX_Dust"));
	SparkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FX/Materials/M_FX_Spark.M_FX_Spark"));

	FActorSpawnParameters Params;
	Params.Name = TEXT("BLFXManager");
	Params.ObjectFlags = RF_Transient;
	FXActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	USceneComponent* Root = NewObject<USceneComponent>(FXActor, TEXT("Root"));
	FXActor->SetRootComponent(Root);
	Root->RegisterComponent();

	auto MakeISM = [this](const TCHAR* Name, UMaterialInterface* Material, int32 CustomFloats, int32 Count)
	{
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(FXActor, Name);
		ISM->SetStaticMesh(QuadMesh);
		if (Material)
		{
			ISM->SetMaterial(0, Material);
		}
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		ISM->SetUsingAbsoluteLocation(true);
		ISM->SetUsingAbsoluteRotation(true);
		ISM->SetUsingAbsoluteScale(true);
		ISM->bNeverDistanceCull = true;
		ISM->SetupAttachment(FXActor->GetRootComponent());
		ISM->RegisterComponent();
		ISM->SetNumCustomDataFloats(CustomFloats);
		TArray<FTransform> Initial;
		Initial.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), Count);
		ISM->AddInstances(Initial, false, true);
		// Bounds enormes: las instancias se mueven por todo el mapa cada frame
		ISM->SetCullDistances(0, 0);
		return ISM;
	};
	SpriteISM = MakeISM(TEXT("Sprites"), SpriteMaterial, SpriteCustomFloats, MaxSprites);
	SparkISM = MakeISM(TEXT("Sparks"), SparkMaterial, SparkCustomFloats, MaxSparks);

	Debris = NewObject<UBLDebrisPoolComponent>(FXActor, TEXT("ImpactDebris"));
	Debris->MaxPerMesh = 64;
	Debris->RegisterComponent();
}

FVector UBLFXSubsystem::GetViewLocation() const
{
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			return PC->PlayerCameraManager->GetCameraLocation();
		}
	}
	return FVector::ZeroVector;
}

int32 UBLFXSubsystem::GetActiveSprites() const { return AliveSprites; }
int32 UBLFXSubsystem::GetActiveSparks() const { return AliveSparks; }

void UBLFXSubsystem::AddSprite(const FSprite& S)
{
	if (Sprites.Num() == 0)
	{
		return;
	}
	FSprite& Slot = Sprites[NextSprite];
	NextSprite = (NextSprite + 1) % Sprites.Num();
	AliveSprites += Slot.bAlive ? 0 : 1;
	Slot = S;
	Slot.bAlive = true;
}

void UBLFXSubsystem::AddSpark(const FSpark& S)
{
	if (Sparks.Num() == 0)
	{
		return;
	}
	FSpark& Slot = Sparks[NextSpark];
	NextSpark = (NextSpark + 1) % Sparks.Num();
	AliveSparks += Slot.bAlive ? 0 : 1;
	Slot = S;
	Slot.bAlive = true;
}

void UBLFXSubsystem::SpawnImpact(const FBLSurfaceEffect& Effect, const FHitResult& Hit, const FVector& ShotDir)
{
	EnsureActor();
	const FVector N = Hit.ImpactNormal.GetSafeNormal();
	const FVector P = Hit.ImpactPoint + N * 1.5f;
	const FVector Reflected = FMath::GetReflectionVector(ShotDir, N);

	// ---- Polvo: nube que crece, frena, sube un poco y se disipa ----
	for (int32 i = 0; i < Effect.DustCount; ++i)
	{
		FSprite S;
		const FVector Dir = FMath::VRandCone((N + FVector::UpVector * 0.25f).GetSafeNormal(), FMath::DegreesToRadians(55.f));
		S.Location = P + FMath::VRand() * 2.f;
		S.Velocity = Dir * Effect.DustSpeed * FMath::FRandRange(0.3f, 1.f);
		S.Life = FMath::FRandRange(Effect.DustLife.X, Effect.DustLife.Y);
		S.StartSize = Effect.DustSize.X * FMath::FRandRange(0.7f, 1.2f);
		S.EndSize = Effect.DustSize.Y * FMath::FRandRange(0.7f, 1.2f);
		S.Opacity = Effect.DustOpacity * FMath::FRandRange(0.7f, 1.f);
		S.Drag = 3.5f;
		S.Rise = 18.f;
		S.Frame = float(FMath::RandRange(0, 3));
		S.Spin = FMath::FRandRange(-0.8f, 0.8f);
		S.Roll = FMath::FRandRange(0.f, 2.f * PI);
		S.Color = Effect.DustColor * FMath::FRandRange(0.85f, 1.1f);
		AddSprite(S);
	}
	// ---- Chorro: polvo rápido y denso a lo largo de la normal (primeros 0,3 s) ----
	for (int32 i = 0; i < Effect.JetCount; ++i)
	{
		FSprite S;
		const FVector Dir = FMath::VRandCone((N * 0.8f + Reflected * 0.2f).GetSafeNormal(), FMath::DegreesToRadians(18.f));
		S.Location = P;
		S.Velocity = Dir * Effect.JetSpeed * FMath::FRandRange(0.5f, 1.2f);
		S.Life = FMath::FRandRange(0.35f, 0.65f);
		S.StartSize = 8.f;
		S.EndSize = FMath::FRandRange(35.f, 55.f);
		S.Opacity = 1.f;
		S.Drag = 7.f;
		S.Rise = 0.f;
		S.Frame = float(FMath::RandRange(0, 3));
		S.Roll = FMath::FRandRange(0.f, 2.f * PI);
		S.Color = Effect.DustColor * 1.1f;
		AddSprite(S);
	}
	// ---- Chispas ----
	for (int32 i = 0; i < Effect.SparkCount; ++i)
	{
		FSpark S;
		const FVector Dir = FMath::VRandCone((Reflected * 0.6f + N * 0.4f).GetSafeNormal(), FMath::DegreesToRadians(40.f));
		S.Location = P;
		S.Velocity = Dir * Effect.SparkSpeed * FMath::FRandRange(0.4f, 1.3f);
		S.Life = FMath::FRandRange(0.1f, 0.35f);
		S.Intensity = FMath::FRandRange(0.6f, 1.f);
		S.Color = Effect.SparkColor;
		AddSpark(S);
	}
	// ---- Escombros ----
	if (Debris && Effect.DebrisMesh)
	{
		for (int32 i = 0; i < Effect.DebrisCount; ++i)
		{
			UBLDebrisPoolComponent::FSpawnParams D;
			D.Mesh = Effect.DebrisMesh;
			D.Location = P;
			D.Rotation = FMath::VRand().ToOrientationQuat();
			D.Velocity = FMath::VRandCone((N + Reflected * 0.4f).GetSafeNormal(), FMath::DegreesToRadians(40.f)) * Effect.DebrisSpeed * FMath::FRandRange(0.4f, 1.2f);
			D.AngularVelocity = FMath::VRand() * FMath::FRandRange(10.f, 35.f);
			D.Lifetime = FMath::FRandRange(1.5f, 3.f);
			D.Scale = FMath::FRandRange(Effect.DebrisScale.X, Effect.DebrisScale.Y);
			Debris->Spawn(D);
		}
	}
	// ---- Marca ----
	if (Effect.Decal)
	{
		FRotator DecalRot = (-N).Rotation();
		DecalRot.Roll = FMath::FRandRange(-180.f, 180.f);
		const float Size = Effect.DecalSize * FMath::FRandRange(0.8f, 1.2f);
		const FVector DecalSize(5.f, Size, Size);
		UDecalComponent* Decal = Hit.GetComponent()
			? UGameplayStatics::SpawnDecalAttached(Effect.Decal, DecalSize, Hit.GetComponent(), Hit.BoneName, Hit.ImpactPoint, DecalRot, EAttachLocation::KeepWorldPosition, DecalLifetime)
			: UGameplayStatics::SpawnDecalAtLocation(this, Effect.Decal, DecalSize, Hit.ImpactPoint, DecalRot, DecalLifetime);
		if (Decal)
		{
			Decal->SetFadeScreenSize(0.0006f);
			Decal->SetFadeOut(DecalLifetime - 4.f, 4.f, false);
			Decals.RemoveAll([](const TWeakObjectPtr<UDecalComponent>& D) { return !D.IsValid(); });
			Decals.Add(Decal);
			while (Decals.Num() > MaxDecals)
			{
				if (UDecalComponent* Oldest = Decals[0].Get())
				{
					Oldest->DestroyComponent();
				}
				Decals.RemoveAt(0);
			}
		}
	}
	// ---- Salpicadura detrás (sangre en la pared o el suelo cercanos) ----
	if (Effect.SplatterDecal && Effect.SplatterDistance > 0.f)
	{
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLSplatter), false, Hit.GetActor());
		FHitResult Behind;
		const FVector Dir = (ShotDir + FVector(0.f, 0.f, -0.25f)).GetSafeNormal();
		if (GetWorld()->LineTraceSingleByChannel(Behind, Hit.ImpactPoint, Hit.ImpactPoint + Dir * Effect.SplatterDistance, ECC_Visibility, Q)
			&& Behind.GetComponent() && Behind.GetComponent()->Mobility != EComponentMobility::Movable)
		{
			FRotator Rot = (-Behind.ImpactNormal).Rotation();
			Rot.Roll = FMath::FRandRange(-180.f, 180.f);
			// Más pequeña cuanto más lejos
			const float Size = Effect.SplatterSize * FMath::FRandRange(0.7f, 1.3f) * FMath::Lerp(1.f, 0.6f, Behind.Time);
			if (UDecalComponent* Splat = UGameplayStatics::SpawnDecalAtLocation(this, Effect.SplatterDecal, FVector(8.f, Size, Size), Behind.ImpactPoint, Rot, DecalLifetime))
			{
				Splat->SetFadeScreenSize(0.0006f);
				Splat->SetFadeOut(DecalLifetime - 4.f, 4.f, false);
				Decals.Add(Splat);
			}
		}
	}
	// ---- Sonido ----
	if (USoundBase* Sound = PickSound(Effect.ImpactSounds))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Hit.ImpactPoint, 1.f, FMath::FRandRange(0.92f, 1.08f));
	}
}

void UBLFXSubsystem::UpdateSprites(float DeltaTime, const FVector& View)
{
	if (!SpriteISM || AliveSprites == 0)
	{
		return;
	}
	ScratchTransforms.Reset(Sprites.Num());
	for (int32 i = 0; i < Sprites.Num(); ++i)
	{
		FSprite& S = Sprites[i];
		float Opacity = 0.f;
		if (S.bAlive)
		{
			S.Age += DeltaTime;
			if (S.Age >= S.Life)
			{
				S.bAlive = false;
				--AliveSprites;
			}
		}
		if (!S.bAlive)
		{
			ScratchTransforms.Add(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
			SpriteISM->SetCustomDataValue(i, 0, 0.f, false);
			continue;
		}
		S.Velocity *= FMath::Exp(-S.Drag * DeltaTime);
		S.Velocity.Z += S.Rise * DeltaTime;
		S.Location += S.Velocity * DeltaTime;
		S.Roll += S.Spin * DeltaTime;

		const float T = S.Age / S.Life;
		// Crece rápido al principio y luego despacio (como una nube que se expande y frena)
		const float Size = FMath::Lerp(S.StartSize, S.EndSize, 1.f - FMath::Pow(1.f - T, 3.f));
		// Aparece en 0,03 s y se desvanece de forma suave
		Opacity = S.Opacity * FMath::Clamp(S.Age / 0.03f, 0.f, 1.f) * FMath::Pow(1.f - T, 1.5f);

		const FVector ToView = (View - S.Location).GetSafeNormal();
		const FQuat Facing = FRotationMatrix::MakeFromX(ToView).ToQuat() * FQuat(FVector::XAxisVector, S.Roll);
		ScratchTransforms.Add(FTransform(Facing, S.Location, FVector(1.f, Size / 100.f, Size / 100.f)));
		SpriteISM->SetCustomDataValue(i, 0, Opacity, false);
		SpriteISM->SetCustomDataValue(i, 1, S.Frame, false);
		SpriteISM->SetCustomDataValue(i, 2, S.Color.R, false);
		SpriteISM->SetCustomDataValue(i, 3, S.Color.G, false);
		SpriteISM->SetCustomDataValue(i, 4, S.Color.B, false);
	}
	SpriteISM->BatchUpdateInstancesTransforms(0, ScratchTransforms, true, true);
	SpriteISM->MarkRenderStateDirty(); // envía también los datos por instancia (opacidad, color)
}

void UBLFXSubsystem::UpdateSparks(float DeltaTime, const FVector& View)
{
	if (!SparkISM || AliveSparks == 0)
	{
		return;
	}
	ScratchTransforms.Reset(Sparks.Num());
	for (int32 i = 0; i < Sparks.Num(); ++i)
	{
		FSpark& S = Sparks[i];
		if (S.bAlive)
		{
			S.Age += DeltaTime;
			if (S.Age >= S.Life)
			{
				S.bAlive = false;
				--AliveSparks;
			}
		}
		if (!S.bAlive)
		{
			ScratchTransforms.Add(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
			SparkISM->SetCustomDataValue(i, 0, 0.f, false);
			continue;
		}
		S.Velocity.Z -= 980.f * DeltaTime;
		S.Velocity *= FMath::Exp(-1.5f * DeltaTime);
		S.Location += S.Velocity * DeltaTime;

		// Estela orientada a la velocidad, girada hacia la cámara (billboard axial)
		const float Speed = S.Velocity.Size();
		const FVector Dir = Speed > 1.f ? S.Velocity / Speed : FVector::UpVector;
		const FVector ToView = (View - S.Location).GetSafeNormal();
		const FQuat Q = FRotationMatrix::MakeFromZX(Dir, ToView).ToQuat();
		const float Length = FMath::Clamp(Speed * 0.03f, 4.f, 35.f);
		const float Fade = 1.f - S.Age / S.Life;
		ScratchTransforms.Add(FTransform(Q, S.Location - Dir * Length * 0.5f, FVector(1.f, 0.8f / 100.f, Length / 100.f)));
		SparkISM->SetCustomDataValue(i, 0, S.Intensity * Fade, false);
		SparkISM->SetCustomDataValue(i, 1, S.Color.R, false);
		SparkISM->SetCustomDataValue(i, 2, S.Color.G, false);
		SparkISM->SetCustomDataValue(i, 3, S.Color.B, false);
	}
	SparkISM->BatchUpdateInstancesTransforms(0, ScratchTransforms, true, true);
	SparkISM->MarkRenderStateDirty();
}

void UBLFXSubsystem::Tick(float DeltaTime)
{
	if (!FXActor)
	{
		return;
	}
	const FVector View = GetViewLocation();
	UpdateSprites(DeltaTime, View);
	UpdateSparks(DeltaTime, View);
}
