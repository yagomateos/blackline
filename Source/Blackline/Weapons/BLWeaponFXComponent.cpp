#include "Weapons/BLWeaponFXComponent.h"

#include "Weapons/BLDebrisPoolComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

UBLWeaponFXComponent::UBLWeaponFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UBLWeaponFXComponent::IsLocalPlayer() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

FTransform UBLWeaponFXComponent::GetWeaponSocketTransform(FName Socket, const FVector& FallbackLocalOffset) const
{
	// Transform en espacio de la malla del arma: posición del socket, ejes desde ForwardAxis (X = adelante)
	const USkeletalMeshComponent* Mesh = WeaponMesh.Get();
	const UBLWeaponData* Data = CurrentData.Get();
	const FVector Forward = Data ? Data->ForwardAxis : FVector::YAxisVector;
	const FQuat Axes = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();
	FVector Loc = FallbackLocalOffset;
	if (Mesh && Mesh->DoesSocketExist(Socket))
	{
		Loc = Mesh->GetSocketTransform(Socket, RTS_Component).GetLocation();
	}
	return FTransform(Axes, Loc);
}

void UBLWeaponFXComponent::SetWeapon(const UBLWeaponData* Data, USkeletalMeshComponent* InWeaponMesh)
{
	WeaponMesh = InWeaponMesh;
	CurrentData = Data;
	if (!Data || !InWeaponMesh)
	{
		return;
	}

	if (!Debris)
	{
		Debris = NewObject<UBLDebrisPoolComponent>(GetOwner(), TEXT("WeaponDebris"));
		Debris->RegisterComponent();
	}

	if (!MuzzleFlash)
	{
		MuzzleFlash = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("MuzzleFlash"));
		MuzzleFlash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MuzzleFlash->SetCastShadow(false);
		MuzzleFlash->SetMobility(EComponentMobility::Movable);
		MuzzleFlash->RegisterComponent();
	}
	if (!MuzzleLight)
	{
		MuzzleLight = NewObject<UPointLightComponent>(GetOwner(), TEXT("MuzzleLight"));
		MuzzleLight->SetCastShadows(false);
		MuzzleLight->SetAttenuationRadius(600.f);
		MuzzleLight->SetSourceRadius(4.f);
		MuzzleLight->SetMobility(EComponentMobility::Movable);
		MuzzleLight->RegisterComponent();
	}

	// El fogonazo comparte la forma de dibujarse del arma (primera persona o mundo)
	MuzzleFlash->FirstPersonPrimitiveType = InWeaponMesh->FirstPersonPrimitiveType;
	MuzzleFlash->SetOnlyOwnerSee(InWeaponMesh->bOnlyOwnerSee);
	MuzzleFlash->SetStaticMesh(Data->MuzzleFlashMesh);
	MuzzleFlash->AttachToComponent(InWeaponMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MuzzleFlash->SetRelativeTransform(GetWeaponSocketTransform(Data->MuzzleSocket, FVector(0.f, 50.f, 10.f)));
	MuzzleFlash->SetVisibility(false);

	MuzzleLight->AttachToComponent(InWeaponMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MuzzleLight->SetRelativeLocation(GetWeaponSocketTransform(Data->MuzzleSocket, FVector(0.f, 50.f, 10.f)).GetLocation());
	MuzzleLight->SetLightColor(Data->MuzzleLightColor);
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetVisibility(false);
}

void UBLWeaponFXComponent::PlayWeaponSound(USoundBase* Sound, float Volume)
{
	if (!Sound)
	{
		return;
	}
	if (IsLocalPlayer())
	{
		UGameplayStatics::PlaySound2D(this, Sound, Volume, FMath::FRandRange(0.97f, 1.03f));
	}
	else if (const USkeletalMeshComponent* Mesh = WeaponMesh.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Mesh->GetComponentLocation(), Volume, FMath::FRandRange(0.97f, 1.03f));
	}
}

void UBLWeaponFXComponent::PlayFire(const UBLWeaponData* Data)
{
	USkeletalMeshComponent* Mesh = WeaponMesh.Get();
	if (!Data || !Mesh)
	{
		return;
	}

	// ---- Sonido por capas: cercano en cada tiro; la cola al terminar (ver Tick) ----
	LastMuzzleLocation = Mesh->GetComponentLocation();
	if (IsLocalPlayer())
	{
		UGameplayStatics::PlaySound2D(this, UBLWeaponData::PickRandom(Data->FireSounds), 1.f, FMath::FRandRange(0.98f, 1.02f));
	}
	else
	{
		// Otros tiradores: mezcla de potencia constante entre la capa cercana y la lejana según la distancia
		// al oyente (de 0,6x a 1,5x DistantFireDistance), así no se nota el salto de una a otra
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const float Dist = PC && PC->PlayerCameraManager ? FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), LastMuzzleLocation) : 0.f;
		const float D = Data->DistantFireDistance;
		const float FarW = FMath::Clamp((Dist - 0.6f * D) / (0.9f * D), 0.f, 1.f);
		const float Pitch = FMath::FRandRange(0.97f, 1.03f);
		if (FarW < 0.97f)
		{
			UGameplayStatics::PlaySoundAtLocation(this, UBLWeaponData::PickRandom(Data->FireSounds3D), LastMuzzleLocation, FMath::Sqrt(1.f - FarW), Pitch);
		}
		if (FarW > 0.03f)
		{
			UGameplayStatics::PlaySoundAtLocation(this, UBLWeaponData::PickRandom(Data->FireDistantSounds), LastMuzzleLocation, FMath::Sqrt(FarW), Pitch);
		}
	}
	bTailPending = true;
	TimeSinceShot = 0.f;
	TailDelay = Data->GetShotInterval() * 1.5f;

	// ---- Fogonazo: forma, giro y tamaño aleatorios en cada disparo ----
	if (MuzzleFlash && Data->MuzzleFlashMesh)
	{
		FTransform Rel = GetWeaponSocketTransform(Data->MuzzleSocket, FVector(0.f, 50.f, 10.f));
		Rel.SetRotation(Rel.GetRotation() * FQuat(FVector::XAxisVector, FMath::FRandRange(0.f, 2.f * PI)));
		const float S = FMath::FRandRange(0.6f, 0.85f);
		Rel.SetScale3D(FVector(FMath::FRandRange(0.7f, 1.2f) * S, S, S));
		MuzzleFlash->SetRelativeTransform(Rel);
		MuzzleFlash->SetVisibility(true);
	}
	if (MuzzleLight)
	{
		MuzzleLight->SetIntensity(Data->MuzzleLightIntensity * FMath::FRandRange(0.8f, 1.1f));
		MuzzleLight->SetVisibility(true);
	}
	FlashRemaining = Data->MuzzleFlashDuration;

	// ---- Casquillo: sale hacia la derecha y arriba de la ventana de expulsión ----
	if (Debris && Data->CasingMesh)
	{
		const FTransform Eject = GetWeaponSocketTransform(Data->EjectSocket, Data->EjectLocalOffset) * Mesh->GetComponentTransform();
		const FQuat Q = Eject.GetRotation();
		const FVector Fwd = Q.GetAxisX(), Right = Q.GetAxisY(), Up = Q.GetAxisZ();
		const APawn* Pawn = Cast<APawn>(GetOwner());
		const FVector Inherit = Pawn ? Pawn->GetVelocity() : FVector::ZeroVector;

		UBLDebrisPoolComponent::FSpawnParams P;
		P.Mesh = Data->CasingMesh;
		P.bFirstPerson = Mesh->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson;
		P.Location = Eject.GetLocation();
		P.Rotation = FQuat(Up, FMath::FRandRange(-0.3f, 0.3f)) * Q;
		P.Velocity = Inherit + Right * FMath::FRandRange(260.f, 340.f) + Up * FMath::FRandRange(120.f, 200.f) - Fwd * FMath::FRandRange(0.f, 60.f);
		P.AngularVelocity = Up * FMath::FRandRange(15.f, 25.f) + Fwd * FMath::FRandRange(-8.f, 8.f);
		P.Lifetime = 1.4f;
		P.BounceSound = UBLWeaponData::PickRandom(Data->CasingSounds);
		P.BounceVolume = 0.6f;
		P.SurfaceSounds = Data->SurfaceEffects;
		Debris->Spawn(P);
	}
}

void UBLWeaponFXComponent::PlayImpact(const UBLWeaponData* Data, const FHitResult& Hit)
{
	if (!Data || !Data->SurfaceEffects)
	{
		return;
	}
	UBLFXSubsystem* FXSys = GetWorld()->GetSubsystem<UBLFXSubsystem>();
	if (!FXSys)
	{
		return;
	}
	const EPhysicalSurface Surface = UBLSurfaceEffectsData::ResolveSurface(Hit);
	FBLSurfaceEffect Effect = Data->SurfaceEffects->Get(Surface);
	if (!FMath::IsNearlyEqual(Data->ImpactScale, 1.f))
	{
		Effect.DustCount = FMath::RoundToInt(Effect.DustCount * Data->ImpactScale);
		Effect.SparkCount = FMath::RoundToInt(Effect.SparkCount * Data->ImpactScale);
		Effect.DebrisCount = FMath::RoundToInt(Effect.DebrisCount * Data->ImpactScale);
	}
	FXSys->SpawnImpact(Effect, Hit, (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal());
}

void UBLWeaponFXComponent::PlayHandling(const UBLWeaponData* Data, float Volume)
{
	if (Data)
	{
		PlayWeaponSound(UBLWeaponData::PickRandom(Data->HandlingSounds), Volume);
	}
}

void UBLWeaponFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceShot += DeltaTime;
	if (bTailPending && TimeSinceShot >= TailDelay)
	{
		bTailPending = false;
		if (const UBLWeaponData* Data = CurrentData.Get())
		{
			if (IsLocalPlayer())
			{
				UGameplayStatics::PlaySound2D(this, UBLWeaponData::PickRandom(Data->FireTailSounds), 0.85f, FMath::FRandRange(0.97f, 1.03f));
			}
			else
			{
				UGameplayStatics::PlaySoundAtLocation(this, UBLWeaponData::PickRandom(Data->FireTailSounds), LastMuzzleLocation, 0.7f);
			}
		}
	}

	if (FlashRemaining > 0.f)
	{
		FlashRemaining -= DeltaTime;
		if (FlashRemaining <= 0.f)
		{
			if (MuzzleFlash)
			{
				MuzzleFlash->SetVisibility(false);
			}
			if (MuzzleLight)
			{
				MuzzleLight->SetVisibility(false);
			}
		}
	}
}
