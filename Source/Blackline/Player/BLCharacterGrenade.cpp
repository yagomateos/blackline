// Granada del jugador (Bloque 11): anilla, el arma baja, lanzamiento y sacudida por explosiones cercanas.
#include "Player/BLCharacter.h"

#include "Blackline.h"
#include "Player/BLFirstPersonRigComponent.h"
#include "Weapons/BLGrenade.h"
#include "Weapons/BLWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float PinTime = 0.0f;        // anilla y palanca al pulsar
	constexpr float ReleaseTime = 0.35f;   // sale de la mano
	constexpr float RecoverTime = 0.85f;   // el arma vuelve a su sitio
}

void ABLCharacter::ThrowGrenade()
{
	if (bDead || IsInVehicleOrMount() || Grenades <= 0 || GrenadeTimer >= 0.f || bIsMantling || (GetWeapon() && GetWeapon()->IsReloading()))
	{
		return;
	}
	--Grenades;
	GrenadeTimer = 0.f;
	bGrenadeReleased = false;
	SetFireHeld(false);
	if (bIsSprinting)
	{
		SetSprintHeld(false);
	}
	if (GrenadePinSound)
	{
		UGameplayStatics::PlaySound2D(this, GrenadePinSound, 0.9f);
	}
	// El arma baja y se aparta para dejar sitio al brazo que lanza
	FirstPersonRig->AddWeaponKick(FVector(-6.f, 4.f, -14.f), FRotator(-18.f, 10.f, 25.f));
	UE_LOG(LogBlackline, Log, TEXT("[Granada] Lanzando (quedan %d)"), Grenades);
}

void ABLCharacter::TickGrenade(float DeltaTime)
{
	if (GrenadeTimer < 0.f)
	{
		return;
	}
	GrenadeTimer += DeltaTime;
	if (!bGrenadeReleased && GrenadeTimer >= ReleaseTime)
	{
		bGrenadeReleased = true;
		FVector Origin;
		FVector Dir;
		GetWeaponAimView(Origin, Dir);
		const FRotator View = Dir.Rotation();
		// Sale de delante y a la derecha de la cara, con algo de arco hacia arriba
		const FVector Start = Origin + View.RotateVector(FVector(45.f, 18.f, -8.f));
		FHitResult Block;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLGrenadeSpawn), false, this);
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Block, Origin, Start, ECC_Visibility, Q);
		const FVector SpawnAt = bBlocked ? Block.ImpactPoint - Dir * 10.f : Start;
		const FVector Velocity = (Dir + FVector(0.f, 0.f, 0.18f)).GetSafeNormal() * GrenadeThrowSpeed + GetVelocity() * 0.5f;
		FActorSpawnParameters P;
		P.Owner = this;
		P.Instigator = this;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABLGrenade* G = GetWorld()->SpawnActor<ABLGrenade>(ABLGrenade::StaticClass(), SpawnAt, View, P))
		{
			G->Launch(Velocity, GetController(), GrenadeFuse - ReleaseTime);
			LastGrenade = G;
		}
		if (GrenadeThrowSound)
		{
			UGameplayStatics::PlaySound2D(this, GrenadeThrowSound, 0.8f);
		}
		FirstPersonRig->AddCameraKick(FRotator(-1.2f, 0.8f, 0.f));
	}
	if (GrenadeTimer >= RecoverTime)
	{
		GrenadeTimer = -1.f;
	}
}

void ABLCharacter::OnNearbyExplosion(const FVector& Location, float Strength)
{
	if (Strength <= 0.01f || bDead)
	{
		return;
	}
	const float S = Strength * Strength;
	FirstPersonRig->AddCameraKick(FRotator(-7.f * S, FMath::FRandRange(-4.f, 4.f) * S, FMath::FRandRange(-6.f, 6.f) * S));
	FirstPersonRig->AddWeaponKick(FVector(-3.f, 0.f, -5.f) * S, FRotator(-6.f, 0.f, FMath::FRandRange(-8.f, 8.f)) * S);
}
