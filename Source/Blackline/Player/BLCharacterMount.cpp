// ABLCharacter: arma montada (misión 4). El jugador se coloca detrás del afuste, no se mueve, la mirada queda limitada
// al arco del arma (límites del PlayerCameraManager) y el gatillo dispara la ametralladora. F (interactuar) la suelta.
#include "Player/BLCharacter.h"

#include "Blackline.h"
#include "Vehicles/BLBoat.h"
#include "Weapons/BLMountedGun.h"
#include "Weapons/BLWeaponComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

void ABLCharacter::MountGun(ABLMountedGun* Gun)
{
	if (!Gun || IsMounted() || bDead)
	{
		return;
	}
	MountedGun = Gun;
	if (Weapon)
	{
		Weapon->SetTriggerHeld(false);
		Weapon->CancelReload();
	}
	bFireNeedsRelease = bFireInputHeld;   // la ametralladora no dispara con una pulsación que venía del fusil
	bSprintHeld = false;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	GetCharacterMovement()->StopMovementImmediately();
	SetActorLocation(Gun->GetSeatLocation() + FVector(0.f, 0.f, GetDefaultHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	// Se oculta el arma propia (y los brazos de primera persona, que la sujetan)
	if (WeaponMesh) { WeaponMesh->SetVisibility(false, true); }
	if (FirstPersonMesh) { FirstPersonMesh->SetVisibility(false, true); }
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(FRotator(0.f, Gun->GetBaseYaw(), 0.f));
		if (APlayerCameraManager* Cam = PC->PlayerCameraManager)
		{
			SavedViewPitch = FVector2D(Cam->ViewPitchMin, Cam->ViewPitchMax);
			SavedViewYaw = FVector2D(Cam->ViewYawMin, Cam->ViewYawMax);
			Cam->ViewPitchMin = Gun->PitchMin;
			Cam->ViewPitchMax = Gun->PitchMax;
			Cam->ViewYawMin = FRotator::ClampAxis(Gun->GetBaseYaw() - Gun->YawLimit);
			Cam->ViewYawMax = FRotator::ClampAxis(Gun->GetBaseYaw() + Gun->YawLimit);
		}
	}
	Gun->Mount(this);
}

void ABLCharacter::DismountGun()
{
	ABLMountedGun* Gun = MountedGun.Get();
	MountedGun = nullptr;
	// Estado de disparo limpio: la ametralladora deja de disparar y el fusil no hereda el gatillo apretado
	if (Gun)
	{
		Gun->SetTriggerHeld(false);
	}
	if (Weapon)
	{
		Weapon->SetTriggerHeld(false);
	}
	bFireNeedsRelease = bFireInputHeld;
	if (WeaponMesh) { WeaponMesh->SetVisibility(true, true); }
	if (FirstPersonMesh) { FirstPersonMesh->SetVisibility(true, true); }
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APlayerCameraManager* Cam = PC->PlayerCameraManager)
		{
			Cam->ViewPitchMin = SavedViewPitch.X;
			Cam->ViewPitchMax = SavedViewPitch.Y;
			Cam->ViewYawMin = SavedViewYaw.X;
			Cam->ViewYawMax = SavedViewYaw.Y;
		}
	}
	if (Gun)
	{
		Gun->Dismount();
	}
}

void ABLCharacter::MountGunTrigger(bool bHeld)
{
	if (ABLMountedGun* Gun = MountedGun.Get())
	{
		Gun->SetTriggerHeld(bHeld && !bDead);
	}
}

// ---------------------------------------------------------------------------
// Lancha (misión 2): de pie junto a la consola, sin arma en la mano; W/S y A/D la pilotan
// ---------------------------------------------------------------------------

void ABLCharacter::BoardBoat(ABLBoat* Boat)
{
	if (!Boat || bDead || IsInVehicleOrMount())
	{
		return;
	}
	DrivenBoat = Boat;
	if (Weapon)
	{
		Weapon->SetTriggerHeld(false);
		Weapon->CancelReload();
	}
	bSprintHeld = false;
	bAimHeld = false;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	if (WeaponMesh) { WeaponMesh->SetVisibility(false, true); }
	if (FirstPersonMesh) { FirstPersonMesh->SetVisibility(false, true); }
	AttachToComponent(Boat->GetDriverSeat(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(FRotator(-8.f, Boat->GetActorRotation().Yaw, 0.f));
	}
	UE_LOG(LogBlackline, Log, TEXT("[Lancha] El jugador sube y toma el timón"));
}

void ABLCharacter::LeaveBoat()
{
	if (!DrivenBoat.IsValid())
	{
		return;
	}
	ABLBoat* Boat = DrivenBoat.Get();
	DrivenBoat = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (WeaponMesh) { WeaponMesh->SetVisibility(true, true); }
	if (FirstPersonMesh) { FirstPersonMesh->SetVisibility(true, true); }
	Boat->OnDriverLeft();
}

void ABLCharacter::EnterVehicleSeat(USceneComponent* Seat, float LookYaw)
{
	if (!Seat || bDead || IsInVehicleOrMount())
	{
		return;
	}
	VehicleSeat = Seat;
	if (Weapon)
	{
		Weapon->SetTriggerHeld(false);
		Weapon->CancelReload();
	}
	bSprintHeld = false;
	bAimHeld = false;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(FRotator(-5.f, LookYaw, 0.f));
	}
}
