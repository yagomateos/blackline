// ABLCharacter: salud, daño recibido, muerte, reaparición y feedback de combate (Bloque 3).
#include "Player/BLCharacter.h"

#include "Blackline.h"
#include "Combat/BLDamageTypes.h"
#include "Mission/BLCheckpointSubsystem.h"
#include "Player/BLFirstPersonRigComponent.h"
#include "Weapons/BLWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	/** Duración de la caída de la cámara al morir (s). */
	constexpr float DeathFallTime = 0.85f;
	/** Momento en que empieza el fundido a negro y su duración. */
	constexpr float DeathFadeStart = 1.6f;
	constexpr float DeathFadeTime = 1.4f;
	/** Vida de los indicadores direccionales de daño. */
	constexpr float IndicatorLife = 1.8f;
}

void ABLCharacter::InitCombat()
{
	BLDamage::SetupCharacterCollision(this);
	Health->OnDamaged.AddUniqueDynamic(this, &ABLCharacter::HandleDamaged);
	Health->OnDeath.AddUniqueDynamic(this, &ABLCharacter::HandleDeath);
	Weapon->OnHitConfirmed.AddUniqueDynamic(this, &ABLCharacter::HandleHitConfirmed);

	if (HeartbeatSound)
	{
		HeartbeatAudio = UGameplayStatics::CreateSound2D(this, HeartbeatSound, 1.f, 1.f, 0.f, nullptr, false, false);
		if (HeartbeatAudio)
		{
			HeartbeatAudio->bAutoDestroy = false;
		}
	}
	if (IsPlayerControlled())
	{
		if (UBLCheckpointSubsystem* Checkpoints = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>())
		{
			Checkpoints->RegisterStart(this);
		}
	}
}

UAISense_Sight::EVisibilityResult ABLCharacter::CanBeSeenFrom(const FCanBeSeenFromContext& Context, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed,
	int32& OutNumberOfAsyncLosCheckRequested, float& OutSightStrength, int32* UserData, const FOnPendingVisibilityQueryProcessedDelegate* Delegate)
{
	OutNumberOfAsyncLosCheckRequested = 0;
	OutNumberOfLoSChecksPerformed = 0;
	OutSightStrength = 0.f;
	if (bDead)
	{
		return UAISense_Sight::EVisibilityResult::NotVisible;
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLSightTarget), false, Context.IgnoreActor);
	Q.AddIgnoredActor(this);
	const FVector Points[3] = { Camera->GetComponentLocation(), GetActorLocation() + FVector(0.f, 0.f, 25.f), GetActorLocation() - FVector(0.f, 0.f, 20.f) };
	for (const FVector& P : Points)
	{
		++OutNumberOfLoSChecksPerformed;
		if (!GetWorld()->LineTraceTestByChannel(Context.ObserverLocation, P, ECC_Visibility, Q))
		{
			OutSeenLocation = P;
			OutSightStrength = 1.f;
			return UAISense_Sight::EVisibilityResult::Visible;
		}
	}
	return UAISense_Sight::EVisibilityResult::NotVisible;
}

float ABLCharacter::GetHitMarkerAge() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() - HitMarkerTime : 100.f;
}

// ---------------------------------------------------------------------------
// Daño recibido
// ---------------------------------------------------------------------------

void ABLCharacter::HandleDamaged(const FBLDamageInfo& Info)
{
	const float Strength = FMath::Clamp(Info.Amount / 25.f, 0.35f, 1.6f);

	// Lado del que viene el daño respecto a la vista: el golpe empuja la cámara en sentido contrario
	const FRotator View = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FVector ToSource = (Info.SourceLocation - GetActorLocation()).GetSafeNormal2D();
	const float Side = FVector::DotProduct(ToSource, FRotationMatrix(FRotator(0.f, View.Yaw, 0.f)).GetUnitAxis(EAxis::Y));
	const float Front = FVector::DotProduct(ToSource, FRotationMatrix(FRotator(0.f, View.Yaw, 0.f)).GetUnitAxis(EAxis::X));
	const float Rand = FMath::FRandRange(0.8f, 1.2f);
	FirstPersonRig->AddCameraKick(FRotator(DamageCameraKick.Pitch * (0.5f + 0.5f * Front) * Strength * Rand,
		-DamageCameraKick.Yaw * Side * Strength * Rand, -DamageCameraKick.Roll * (Side >= 0.f ? 1.f : -1.f) * Strength * Rand));
	FirstPersonRig->AddWeaponKick(FVector(-20.f, -12.f * Side, -15.f) * Strength, FRotator(-12.f, 8.f * Side, -10.f * Side) * Strength);

	DamageFlash = FMath::Min(1.f, DamageFlash + 0.55f * Strength);
	FDamageIndicator Indicator;
	Indicator.Source = Info.SourceLocation;
	Indicator.Time = GetWorld()->GetTimeSeconds();
	Indicator.Strength = FMath::Clamp(Strength, 0.4f, 1.f);
	DamageIndicators.Add(Indicator);

	if (HurtSounds.Num() > 0)
	{
		UGameplayStatics::PlaySound2D(this, HurtSounds[FMath::RandHelper(HurtSounds.Num())], FMath::Lerp(0.6f, 1.f, FMath::Min(Strength, 1.f)));
	}
}

void ABLCharacter::HandleDeath(const FBLDamageInfo& Info)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	DeathTime = 0.f;
	bDeathFadeStarted = false;
	bRespawnRequested = false;
	const FVector ToSource = (Info.SourceLocation - GetActorLocation()).GetSafeNormal2D();
	DeathRollSign = FVector::DotProduct(ToSource, GetActorRightVector()) >= 0.f ? -1.f : 1.f; // cae alejándose del tirador

	// Soltar todo y dejar de responder al input
	SetFireHeld(false);
	SetAimHeld(false);
	SetSprintHeld(false);
	SetLeanLeftHeld(false);
	SetLeanRightHeld(false);
	Weapon->SetTriggerHeld(false);
	Weapon->CancelReload();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}
	UE_LOG(LogBlackline, Log, TEXT("Jugador muerto (%.0f de daño, zona %d)"), Info.Amount, int32(Info.Zone));
}

void ABLCharacter::RespawnAt(const FTransform& Transform, const TArray<FBLWeaponSlot>& Inventory, int32 WeaponIndex)
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	SetActorTransform(Transform, false, nullptr, ETeleportType::ResetPhysics);
	if (Controller)
	{
		Controller->SetControlRotation(FRotator(0.f, Transform.Rotator().Yaw, 0.f));
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Health->ResetHealth();
	if (Inventory.Num() > 0)
	{
		Weapon->RestoreInventory(Inventory, WeaponIndex);
	}
	bDead = false;
	DeathTime = 0.f;
	DamageFlash = 0.f;
	DamageIndicators.Reset();
	SmoothedEyeHeight = EyeHeightStanding;
	WeaponMesh->SetVisibility(true);
	FirstPersonMesh->SetVisibility(true);
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		EnableInput(PC);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.8f, FLinearColor::Black, false, false);
		}
	}
	UE_LOG(LogBlackline, Log, TEXT("Jugador reaparece en %s"), *Transform.GetLocation().ToString());
}

void ABLCharacter::HandleHitConfirmed(EBLHitZone Zone, float Damage, bool bKilled)
{
	HitMarkerTime = GetWorld()->GetTimeSeconds();
	bHitMarkerKill = bKilled;
	HitMarkerZone = Zone;
	if (USoundBase* Sound = bKilled ? HitmarkerKillSound : HitmarkerSound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, Zone == EBLHitZone::Head ? 1.f : 0.8f);
	}
}

// ---------------------------------------------------------------------------
// Tick: feedback de salud (post-proceso, latido), muerte y reaparición
// ---------------------------------------------------------------------------

void ABLCharacter::UpdateCombat(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();
	DamageIndicators.RemoveAll([Now](const FDamageIndicator& I) { return Now - I.Time > IndicatorLife; });
	DamageFlash = FMath::Max(0.f, DamageFlash - DeltaTime * 1.6f);

	// Pantalla dañada: viñeta más cerrada y tinte rojo oscuro según la salud que falta + destello del golpe
	const float Frac = Health->GetHealthFraction();
	const float Low = FMath::SmoothStep(0.6f, 0.15f, Frac);
	const float DeadDim = bDead ? FMath::Clamp(DeathTime / DeathFallTime, 0.f, 1.f) : 0.f;
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = BaseVignette + 0.65f * Low + 0.35f * DamageFlash + 0.5f * DeadDim;
	PP.bOverride_SceneColorTint = true;
	const float Red = FMath::Clamp(0.3f * Low + 0.3f * DamageFlash + 0.25f * DeadDim, 0.f, 0.6f);
	PP.SceneColorTint = FLinearColor(1.f, 1.f - Red, 1.f - Red * 0.95f);
	Camera->PostProcessBlendWeight = 1.f;

	if (HeartbeatAudio)
	{
		const float Beat = bDead ? 0.f : Low;
		if (Beat > 0.02f && !HeartbeatAudio->IsPlaying())
		{
			HeartbeatAudio->Play();
		}
		else if (Beat <= 0.02f && HeartbeatAudio->IsPlaying())
		{
			HeartbeatAudio->Stop();
		}
		HeartbeatAudio->SetVolumeMultiplier(FMath::Max(Beat, 0.01f));
	}

	if (!bDead)
	{
		return;
	}
	DeathTime += DeltaTime;
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!bDeathFadeStarted && DeathTime >= DeathFadeStart)
	{
		bDeathFadeStarted = true;
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, DeathFadeTime, FLinearColor::Black, false, true);
		}
	}
	if (!bRespawnRequested && DeathTime >= RespawnDelay)
	{
		bRespawnRequested = true;
		if (UBLCheckpointSubsystem* Checkpoints = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>())
		{
			Checkpoints->RespawnPlayer(this);
		}
	}
}

void ABLCharacter::ApplyDeathPose(float DeltaTime)
{
	if (!bDead)
	{
		return;
	}
	// El arma cae y gira fuera de cuadro (los brazos la siguen por IK); luego se ocultan
	const float A = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(DeathTime / (DeathFallTime * 0.8f), 0.f, 1.f));
	WeaponRoot->AddRelativeLocation(FVector(-8.f, 10.f * DeathRollSign, -55.f) * A);
	WeaponRoot->AddRelativeRotation(FRotator(-35.f, 0.f, 45.f * DeathRollSign) * A);
	if (A >= 0.99f)
	{
		WeaponMesh->SetVisibility(false);
		FirstPersonMesh->SetVisibility(false);
	}
}
