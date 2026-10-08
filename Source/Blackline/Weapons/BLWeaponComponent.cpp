#include "Weapons/BLWeaponComponent.h"

#include "Blackline.h"
#include "Audio/BLAudioSubsystem.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLWeaponFXComponent.h"
#include "Weapons/BLWeaponOwner.h"

#include "Engine/DamageEvents.h"
#include "Combat/BLHealthComponent.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDebugTraces(TEXT("bl.Weapon.DebugTraces"), false, TEXT("Dibuja los trazados de los disparos."));
}

UBLWeaponComponent::UBLWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // después del movimiento y del rig: dispara desde la cámara final
}

void UBLWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	FX = GetOwner()->FindComponentByClass<UBLWeaponFXComponent>();
	if (!FX)
	{
		FX = NewObject<UBLWeaponFXComponent>(GetOwner(), TEXT("WeaponFX"));
		FX->RegisterComponent();
	}

	for (UBLWeaponData* Data : StartingWeapons)
	{
		AddWeapon(Data, CurrentIndex == INDEX_NONE);
	}
}

IBLWeaponOwner* UBLWeaponComponent::GetWeaponOwner() const
{
	return Cast<IBLWeaponOwner>(GetOwner());
}

FBLWeaponSlot* UBLWeaponComponent::CurrentSlot()
{
	return Inventory.IsValidIndex(CurrentIndex) ? &Inventory[CurrentIndex] : nullptr;
}

const FBLWeaponSlot* UBLWeaponComponent::CurrentSlot() const
{
	return Inventory.IsValidIndex(CurrentIndex) ? &Inventory[CurrentIndex] : nullptr;
}

const UBLWeaponData* UBLWeaponComponent::GetWeaponData() const
{
	const FBLWeaponSlot* Slot = CurrentSlot();
	return Slot ? Slot->Data.Get() : nullptr;
}

float UBLWeaponComponent::GetEquipFraction() const
{
	const UBLWeaponData* Data = GetWeaponData();
	return Data && Data->EquipTime > 0.f ? FMath::Clamp(EquipRemaining / Data->EquipTime, 0.f, 1.f) : 0.f;
}

int32 UBLWeaponComponent::GetMagazine() const
{
	const FBLWeaponSlot* Slot = CurrentSlot();
	return Slot ? Slot->Magazine : 0;
}

int32 UBLWeaponComponent::GetReserve() const
{
	const FBLWeaponSlot* Slot = CurrentSlot();
	return Slot ? Slot->Reserve : 0;
}

void UBLWeaponComponent::AddWeapon(UBLWeaponData* Data, bool bEquip)
{
	if (!Data)
	{
		UE_LOG(LogBlackline, Warning, TEXT("%s: AddWeapon sin datos"), *GetNameSafe(GetOwner()));
		return;
	}
	FBLWeaponSlot& Slot = Inventory.AddDefaulted_GetRef();
	Slot.Data = Data;
	Slot.Magazine = Data->MagazineSize;
	Slot.Reserve = Data->StartReserveAmmo;
	if (bEquip)
	{
		EquipSlot(Inventory.Num() - 1);
	}
}

void UBLWeaponComponent::EquipSlot(int32 Index)
{
	if (!Inventory.IsValidIndex(Index))
	{
		return;
	}
	CancelReload();
	CurrentIndex = Index;
	const UBLWeaponData* Data = Inventory[Index].Data;
	EquipRemaining = Data->EquipTime;
	ShotTimer = 0.f;
	Bloom = 0.f;
	ShotsInBurst = 0;

	if (IBLWeaponOwner* WeaponOwner = GetWeaponOwner())
	{
		WeaponOwner->OnWeaponEquipped(Data);
		if (FX)
		{
			FX->SetWeapon(Data, WeaponOwner->GetWeaponMeshComponent());
		}
	}
}

void UBLWeaponComponent::SetTriggerHeld(bool bHeld)
{
	if (bHeld && !bTriggerHeld)
	{
		bDryFiredThisPress = false;
		if (IBLWeaponOwner* WeaponOwner = GetWeaponOwner())
		{
			WeaponOwner->OnWeaponTriggerPressed();
		}
	}
	bTriggerHeld = bHeld;
	if (!bHeld)
	{
		bSemiLatch = false;
		ShotsInBurst = 0;
	}
}

bool UBLWeaponComponent::StartReload()
{
	FBLWeaponSlot* Slot = CurrentSlot();
	if (!Slot || bReloading || IsEquipping())
	{
		return false;
	}
	const UBLWeaponData* Data = Slot->Data;
	const int32 Capacity = Data->MagazineSize + (Data->bChamberRound && Slot->Magazine > 0 ? 1 : 0);
	if (Slot->Magazine >= Capacity || (Slot->Reserve <= 0 && !bInfiniteReserve))
	{
		return false;
	}

	bReloading = true;
	bReloadEmpty = Slot->Magazine == 0;
	bAmmoInserted = false;
	ReloadElapsed = 0.f;
	ReloadDuration = bReloadEmpty ? Data->EmptyReloadTime : Data->ReloadTime;
	NextReloadSound = 0;
	ShotsInBurst = 0;

	if (IBLWeaponOwner* WeaponOwner = GetWeaponOwner())
	{
		WeaponOwner->OnWeaponReloadStarted(Data, ReloadDuration, bReloadEmpty);
	}
	return true;
}

void UBLWeaponComponent::CancelReload()
{
	if (!bReloading)
	{
		return;
	}
	bReloading = false;
	if (IBLWeaponOwner* WeaponOwner = GetWeaponOwner())
	{
		WeaponOwner->OnWeaponReloadEnded(false);
	}
}

void UBLWeaponComponent::FinishReload()
{
	bReloading = false;
	if (IBLWeaponOwner* WeaponOwner = GetWeaponOwner())
	{
		WeaponOwner->OnWeaponReloadEnded(true);
	}
	OnReloadFinished.Broadcast();
}

void UBLWeaponComponent::UpdateReload(float DeltaTime)
{
	FBLWeaponSlot* Slot = CurrentSlot();
	if (!bReloading || !Slot)
	{
		return;
	}
	const UBLWeaponData* Data = Slot->Data;
	ReloadElapsed += DeltaTime;
	const float T = ReloadElapsed / FMath::Max(ReloadDuration, 0.01f);

	const TArray<FBLTimedSound>& Sounds = bReloadEmpty && Data->EmptyReloadSounds.Num() > 0 ? Data->EmptyReloadSounds : Data->ReloadSounds;
	while (Sounds.IsValidIndex(NextReloadSound) && T >= Sounds[NextReloadSound].Time)
	{
		if (FX)
		{
			FX->PlayWeaponSound(Sounds[NextReloadSound].Sound);
		}
		++NextReloadSound;
	}

	if (!bAmmoInserted && T >= Data->ReloadAmmoInsertTime)
	{
		bAmmoInserted = true;
		// Táctica: queda una bala en la recámara además del cargador nuevo
		const int32 Capacity = Data->MagazineSize + (Data->bChamberRound && !bReloadEmpty ? 1 : 0);
		const int32 Needed = FMath::Max(0, Capacity - Slot->Magazine);
		const int32 Taken = bInfiniteReserve ? Needed : FMath::Min(Needed, Slot->Reserve);
		Slot->Magazine += Taken;
		if (!bInfiniteReserve)
		{
			Slot->Reserve -= Taken;
		}
	}

	if (T >= 1.f)
	{
		FinishReload();
	}
}

bool UBLWeaponComponent::CanFireNow() const
{
	const FBLWeaponSlot* Slot = CurrentSlot();
	return Slot && Slot->Magazine > 0 && !bReloading && !IsEquipping()
		&& UnblockedTime >= Slot->Data->SprintToFireTime
		&& !(Slot->Data->FireMode == EBLFireMode::Semi && bSemiLatch);
}

float UBLWeaponComponent::GetCurrentSpread() const
{
	const FBLWeaponSlot* Slot = CurrentSlot();
	const IBLWeaponOwner* WeaponOwner = GetWeaponOwner();
	if (!Slot || !WeaponOwner)
	{
		return 0.f;
	}
	const UBLWeaponData* Data = Slot->Data;
	const float Aim = WeaponOwner->GetWeaponAimAlpha();
	float Spread = FMath::Lerp(Data->HipSpread, Data->AimSpread, Aim);

	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		const float Speed = Pawn->GetVelocity().Size2D();
		Spread += Data->MoveSpread * FMath::Clamp(Speed / 420.f, 0.f, 1.5f) * (1.f - Aim * 0.7f);
		if (Pawn->GetMovementComponent() && Pawn->GetMovementComponent()->IsFalling())
		{
			Spread += Data->AirSpread;
		}
	}
	Spread += Bloom * FMath::Lerp(1.f, Data->AimBloomMultiplier, Aim);
	return Spread;
}

void UBLWeaponComponent::RestoreInventory(const TArray<FBLWeaponSlot>& Snapshot, int32 EquipIndex)
{
	SetTriggerHeld(false);
	CancelReload();
	Inventory = Snapshot;
	PendingRecoil = FVector2D::ZeroVector;
	RecoverablePitch = 0.f;
	Bloom = 0.f;
	CurrentIndex = INDEX_NONE;
	if (Inventory.IsValidIndex(EquipIndex))
	{
		EquipSlot(EquipIndex);
	}
}

float UBLWeaponComponent::ComputeDamage(const FHitResult& Hit, float Distance) const
{
	const UBLWeaponData* Data = GetWeaponData();
	const float FalloffAlpha = FMath::GetRangePct(Data->FalloffStart, Data->FalloffEnd, Distance);
	float Damage = Data->Damage * FMath::Lerp(1.f, Data->FalloffMinMultiplier, FMath::Clamp(FalloffAlpha, 0.f, 1.f));
	// Zona del cuerpo (solo si se alcanzó un hueso de un personaje)
	if (!Hit.BoneName.IsNone())
	{
		switch (BLDamage::ZoneFromBone(Hit.BoneName))
		{
		case EBLHitZone::Head: Damage *= Data->HeadshotMultiplier; break;
		case EBLHitZone::Limb: Damage *= Data->LimbMultiplier; break;
		default: break;
		}
	}
	return Damage;
}

void UBLWeaponComponent::FireShot()
{
	FBLWeaponSlot* Slot = CurrentSlot();
	IBLWeaponOwner* WeaponOwner = GetWeaponOwner();
	if (!Slot || !WeaponOwner)
	{
		return;
	}
	const UBLWeaponData* Data = Slot->Data;

	--Slot->Magazine;
	++ShotsInBurst;
	++ShotsFired;
	TimeSinceShot = 0.f;

	// ---- Trazado ----
	FVector Origin, Direction;
	WeaponOwner->GetWeaponAimView(Origin, Direction);
	const float SpreadRad = FMath::DegreesToRadians(GetCurrentSpread());
	const FVector ShotDir = SpreadRad > KINDA_SMALL_NUMBER ? FMath::VRandCone(Direction, SpreadRad) : Direction;
	const FVector End = Origin + ShotDir * Data->MaxRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BLWeaponTrace), true, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_BLWeapon, Params);
	if (!Hit.bBlockingHit)
	{
		Hit.TraceStart = Origin;
		Hit.TraceEnd = End;
		Hit.Location = Hit.ImpactPoint = End;
	}

	if (CVarDebugTraces.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), Origin, Hit.ImpactPoint, FColor::Orange, false, 2.f, 0, 0.3f);
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 6.f, FColor::Red, false, 2.f);
	}

	// ---- Ruido para la IA: el disparo se oye lejos; los impactos se notan cerca ("me están disparando") ----
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Origin, 1.f, GetOwner(), Data->NoiseRange, FName("Disparo"));
	if (Hit.bBlockingHit)
	{
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), Hit.ImpactPoint, 0.6f, GetOwner(), Data->ImpactNoiseRange, FName("Impacto"));
	}

	// ---- Daño (sin fuego amigo entre la IA: las balas de un miliciano no hieren a otro) ----
	const APawn* ShooterPawn = Cast<APawn>(GetOwner());
	const APawn* HitPawn = Hit.GetActor() ? Cast<APawn>(Hit.GetActor()) : nullptr;
	const bool bFriendlyAI = ShooterPawn && HitPawn && !ShooterPawn->IsPlayerControlled() && !HitPawn->IsPlayerControlled();
	if (Hit.bBlockingHit && Hit.GetActor() && !bFriendlyAI)
	{
		const float Damage = ComputeDamage(Hit, Hit.Distance);
		APawn* Pawn = Cast<APawn>(GetOwner());
		UBLHealthComponent* TargetHealth = Hit.GetActor()->FindComponentByClass<UBLHealthComponent>();
		const bool bWasAlive = TargetHealth && !TargetHealth->IsDead();
		const float HealthBefore = TargetHealth ? TargetHealth->GetHealth() : 0.f;
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, ShotDir, Hit,
			Pawn ? Pawn->GetController() : nullptr, GetOwner(), UDamageType::StaticClass());
		if (bWasAlive)
		{
			OnHitConfirmed.Broadcast(BLDamage::ZoneFromBone(Hit.BoneName), HealthBefore - TargetHealth->GetHealth(), TargetHealth->IsDead());
		}
	}

	// ---- Balas de la IA que pasan rozando al jugador: chasquido supersónico ----
	if (ShooterPawn && !ShooterPawn->IsPlayerControlled())
	{
		if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this))
		{
			Audio->NotifyBulletPass(Origin, Hit.ImpactPoint, Hit.GetActor());
		}
	}

	// ---- Dispersión y retroceso ----
	Bloom = FMath::Min(Bloom + Data->SpreadPerShot, Data->MaxBloom);

	const float Aim = WeaponOwner->GetWeaponAimAlpha();
	const float Ramp = FMath::Lerp(0.7f, 1.f, FMath::Clamp(float(ShotsInBurst - 1) / FMath::Max(Data->RecoilRampShots, 1), 0.f, 1.f));
	const float First = ShotsInBurst == 1 ? Data->RecoilFirstShotMultiplier : 1.f;
	const float AimMult = FMath::Lerp(1.f, Data->RecoilAimMultiplier, Aim);
	const float Vertical = Data->RecoilVertical * Ramp * First * AimMult;
	const float Horizontal = (FMath::FRandRange(-1.f, 1.f) + Data->RecoilHorizontalBias) * Data->RecoilHorizontal * AimMult;
	PendingRecoil += FVector2D(Vertical, Horizontal);

	WeaponOwner->OnWeaponFired(Data);
	if (FX)
	{
		FX->PlayFire(Data);
		if (Hit.bBlockingHit)
		{
			FX->PlayImpact(Data, Hit);
		}
	}
	OnShot.Broadcast(Hit);

	if (Data->FireMode == EBLFireMode::Semi)
	{
		bSemiLatch = true;
	}
}

void UBLWeaponComponent::UpdateRecoil(float DeltaTime)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	const UBLWeaponData* Data = GetWeaponData();
	// El retroceso sobre la mira es solo del jugador; la IA lo simula con su error de puntería
	if (!Controller || !Data || !Pawn->IsPlayerControlled())
	{
		PendingRecoil = FVector2D::ZeroVector;
		return;
	}

	FRotator ControlRot = Controller->GetControlRotation();

	// Si el jugador bajó la mira para compensar, esa parte ya no se "recupera" (evita sobrecompensar)
	if (bHasLastControlPitch)
	{
		const float PlayerPitchDelta = FRotator::NormalizeAxis(ControlRot.Pitch - LastControlPitch);
		if (PlayerPitchDelta < 0.f)
		{
			RecoverablePitch = FMath::Max(0.f, RecoverablePitch + PlayerPitchDelta);
		}
	}

	// Aplicar el retroceso pendiente con algo de suavizado (tiene "peso", no es un salto)
	const float ApplyAlpha = 1.f - FMath::Exp(-Data->RecoilApplySpeed * DeltaTime);
	const FVector2D Step = PendingRecoil * ApplyAlpha;
	PendingRecoil -= Step;
	float DeltaPitch = Step.X;
	const float DeltaYaw = Step.Y;
	RecoverablePitch += Step.X * Data->RecoilRecoveryFraction;

	// Recuperación al dejar de disparar
	TimeSinceShot += DeltaTime;
	if (TimeSinceShot > Data->RecoilRecoveryDelay && RecoverablePitch > 0.f)
	{
		const float Recover = RecoverablePitch * (1.f - FMath::Exp(-Data->RecoilRecoverySpeed * DeltaTime));
		RecoverablePitch -= Recover;
		DeltaPitch -= Recover;
	}

	if (!FMath::IsNearlyZero(DeltaPitch) || !FMath::IsNearlyZero(DeltaYaw))
	{
		ControlRot.Pitch += DeltaPitch;
		ControlRot.Yaw += DeltaYaw;
		Controller->SetControlRotation(ControlRot);
	}
	LastControlPitch = Controller->GetControlRotation().Pitch;
	bHasLastControlPitch = true;
}

void UBLWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FBLWeaponSlot* Slot = CurrentSlot();
	IBLWeaponOwner* WeaponOwner = GetWeaponOwner();
	if (!Slot || !WeaponOwner)
	{
		return;
	}
	const UBLWeaponData* Data = Slot->Data;

	EquipRemaining = FMath::Max(0.f, EquipRemaining - DeltaTime);
	UnblockedTime = WeaponOwner->IsWeaponBlocked() ? 0.f : UnblockedTime + DeltaTime;
	Bloom = FMath::Max(0.f, Bloom - Data->BloomRecovery * DeltaTime * (bTriggerHeld ? 0.35f : 1.f));

	UpdateReload(DeltaTime);

	// ---- Disparo: acumulador de tiempo para respetar la cadencia con cualquier frame rate ----
	ShotTimer -= DeltaTime;
	if (bTriggerHeld)
	{
		if (Slot->Magazine == 0 && !bReloading)
		{
			if (!bDryFiredThisPress)
			{
				bDryFiredThisPress = true;
				if (FX)
				{
					FX->PlayWeaponSound(Data->DryFireSound);
				}
				OnDryFire.Broadcast();
				StartReload(); // recarga automática al apretar el gatillo sin balas
			}
		}
		else
		{
			int32 Guard = 0;
			while (ShotTimer <= 0.f && CanFireNow() && Guard++ < 4)
			{
				FireShot();
				ShotTimer += Data->GetShotInterval();
			}
		}
	}
	if (!bTriggerHeld || !CanFireNow())
	{
		ShotTimer = FMath::Max(ShotTimer, 0.f);
	}

	UpdateRecoil(DeltaTime);
}
