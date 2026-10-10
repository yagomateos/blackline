// ABLCharacter: arma montada (misión 4). El jugador se coloca detrás del afuste, no se mueve, la mirada queda limitada
// al arco del arma (límites del PlayerCameraManager) y el gatillo dispara la ametralladora. F (interactuar) la suelta.
#include "Player/BLCharacter.h"

#include "Blackline.h"
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
	}
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
