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
	if (PendingSlot != INDEX_NONE)
	{
		return HolsterDuration > 0.f ? FMath::Clamp(1.f - HolsterRemaining / HolsterDuration, 0.f, 1.f) : 1.f;
	}
	const UBLWeaponData* Data = GetWeaponData();
	return Data && Data->EquipTime > 0.f ? FMath::Clamp(EquipRemaining / Data->EquipTime, 0.f, 1.f) : 0.f;
}

bool UBLWeaponComponent::SwitchToSlot(int32 Index)
{
	const UBLWeaponData* Data = GetWeaponData();
	if (!Inventory.IsValidIndex(Index) || !Data)
	{
		return false;
	}
	if (PendingSlot != INDEX_NONE)
	{
		if (Index == CurrentIndex)
		{
			// Arrepentido a medio bajar: sube otra vez desde donde está
			EquipRemaining = GetEquipFraction() * Data->EquipTime;
			PendingSlot = INDEX_NONE;
			return true;
		}
		PendingSlot = Index;
		return true;
	}
	if (Index == CurrentIndex)
	{
		return false;
	}
	CancelReload();
	// Si aún estaba subiendo, empieza a bajar desde esa altura
	const float Lowered = GetEquipFraction();
	EquipRemaining = 0.f;
	HolsterDuration = FMath::Max(Data->HolsterTime, 0.05f);
	HolsterRemaining = HolsterDuration * (1.f - Lowered);
	PendingSlot = Index;
	bSemiLatch = false;
	ShotsInBurst = 0;
	if (FX)
	{
		FX->PlayHandling(Data, 0.7f);
	}
	return true;
}

int32 UBLWeaponComponent::FindWeapon(const UBLWeaponData* Data) const
{
	return Inventory.IndexOfByPredicate([Data](const FBLWeaponSlot& Slot) { return Slot.Data == Data; });
}

int32 UBLWeaponComponent::PickUpWeapon(const FBLWeaponSlot& NewSlot, int32 ReplaceIndex)
{
	if (!NewSlot.Data)
	{
		return INDEX_NONE;
	}
	if (Inventory.IsValidIndex(ReplaceIndex))
	{
		// La que se suelta desaparece de la mano y la nueva sube desde abajo
		CancelReload();
		PendingSlot = INDEX_NONE;
		Inventory[ReplaceIndex] = NewSlot;
		EquipSlot(ReplaceIndex);
		OnWeaponSwitched.Broadcast();
		return ReplaceIndex;
	}
	Inventory.Add(NewSlot);
	const int32 Index = Inventory.Num() - 1;
	if (!SwitchToSlot(Index))
	{
		EquipSlot(Index);
	}
	return Index;
}

int32 UBLWeaponComponent::AddReserveAmmo(int32 Index, int32 Amount)
{
	if (!Inventory.IsValidIndex(Index) || !Inventory[Index].Data)
	{
		return 0;
	}
	FBLWeaponSlot& Slot = Inventory[Index];
	const int32 Taken = FMath::Clamp(Slot.Data->MaxReserveAmmo - Slot.Reserve, 0, FMath::Max(Amount, 0));
	Slot.Reserve += Taken;
	return Taken;
}

bool UBLWeaponComponent::CycleWeapon()
{
	if (Inventory.Num() < 2)
	{
		return false;
	}
	const int32 From = PendingSlot != INDEX_NONE ? PendingSlot : CurrentIndex;
	return SwitchToSlot((From + 1) % Inventory.Num());
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
	CycleElapsed = -1.f;
	bCasingPending = false;
	bReloadQueued = false;

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
		bTriggerPressedDuringReload = bReloading;
		bReloadQueued = false;  // apretar el gatillo anula la recarga pendiente
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
	const bool bShells = Data->ReloadStyle == EBLReloadStyle::Shells;
	// Cartucho a cartucho: el de la recámara se puede reponer siempre (primero por la ventana si estaba vacía)
	const int32 Capacity = Data->MagazineSize + (Data->bChamberRound && (bShells || Slot->Magazine > 0) ? 1 : 0);
	if (Slot->Magazine >= Capacity || (Slot->Reserve <= 0 && !bInfiniteReserve))
	{
		return false;
	}
	if (CycleElapsed >= 0.f && CycleElapsed < Data->GetShotInterval())
	{
		bReloadQueued = true;  // aún bombeando: recarga en cuanto termine
		return false;
	}
	bReloadQueued = false;

	bReloading = true;
	bReloadEmpty = Slot->Magazine == 0;
	bAmmoInserted = false;
	bTriggerPressedDuringReload = false;
	ReloadElapsed = 0.f;
	NextReloadSound = 0;
	ShotsInBurst = 0;
	if (bShells)
	{
		const int32 Needed = Capacity - Slot->Magazine;
		ShellCount = bInfiniteReserve ? Needed : FMath::Min(Needed, Slot->Reserve);
		ShellsDone = 0;
		bPortLoad = bReloadEmpty;
		ReloadDuration = GetShellPhaseStart(ShellCount - (bPortLoad ? 1 : 0)) + Data->ShellReloadEndTime;
	}
	else
	{
		ReloadDuration = bReloadEmpty ? Data->EmptyReloadTime : Data->ReloadTime;
	}

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
	if (Data->ReloadStyle == EBLReloadStyle::Shells)
	{
		UpdateShellReload(Data, *Slot);
		return;
	}
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

float UBLWeaponComponent::GetShellPhaseStart(int32 Shell) const
{
	const UBLWeaponData* Data = GetWeaponData();
	return Data ? Data->ShellReloadStartTime + (bPortLoad ? Data->ShellPortLoadTime : 0.f) + Shell * Data->ShellInsertTime : 0.f;
}

EBLShellReloadPhase UBLWeaponComponent::GetShellReloadPhase(float& OutAlpha, int32& OutShell) const
{
	const UBLWeaponData* Data = GetWeaponData();
	OutAlpha = 0.f;
	OutShell = INDEX_NONE;
	if (!bReloading || !Data || Data->ReloadStyle != EBLReloadStyle::Shells)
	{
		return EBLShellReloadPhase::None;
	}
	float T = ReloadElapsed;
	if (T < Data->ShellReloadStartTime)
	{
		OutAlpha = T / FMath::Max(Data->ShellReloadStartTime, 0.01f);
		return EBLShellReloadPhase::Raise;
	}
	T -= Data->ShellReloadStartTime;
	if (bPortLoad)
	{
		if (T < Data->ShellPortLoadTime)
		{
			OutAlpha = T / FMath::Max(Data->ShellPortLoadTime, 0.01f);
			return EBLShellReloadPhase::PortLoad;
		}
		T -= Data->ShellPortLoadTime;
	}
	const int32 Tube = ShellCount - (bPortLoad ? 1 : 0);
	const float Ins = FMath::Max(Data->ShellInsertTime, 0.01f);
	const int32 K = FMath::FloorToInt(T / Ins);
	if (K < Tube)
	{
		OutShell = K;
		OutAlpha = (T - K * Ins) / Ins;
		return EBLShellReloadPhase::Shell;
	}
	OutAlpha = FMath::Clamp((T - Tube * Ins) / FMath::Max(Data->ShellReloadEndTime, 0.01f), 0.f, 1.f);
	return EBLShellReloadPhase::Lower;
}

/** Fracción de la fase de carga por la ventana en que se cierra la corredera. */
static constexpr float PortLoadCloseAlpha = 0.72f;

bool UBLWeaponComponent::IsReloadActionOpen() const
{
	const UBLWeaponData* Data = GetWeaponData();
	if (!bReloading || !Data)
	{
		return false;
	}
	if (Data->ReloadStyle == EBLReloadStyle::Shells)
	{
		return bPortLoad && ReloadElapsed < Data->ShellReloadStartTime + Data->ShellPortLoadTime * PortLoadCloseAlpha;
	}
	return bReloadEmpty && GetReloadProgress() < Data->BoltReleaseTime;
}

void UBLWeaponComponent::UpdateShellReload(const UBLWeaponData* Data, FBLWeaponSlot& Slot)
{
	// Sonidos: ReloadSounds en la fracción de la fase de subida, EmptyReloadSounds en la de carga por la ventana
	const int32 NumRaise = Data->ReloadSounds.Num();
	while (true)
	{
		const bool bRaise = NextReloadSound < NumRaise;
		const TArray<FBLTimedSound>& List = bRaise ? Data->ReloadSounds : Data->EmptyReloadSounds;
		const int32 Index = bRaise ? NextReloadSound : NextReloadSound - NumRaise;
		if (!List.IsValidIndex(Index) || (!bRaise && !bPortLoad))
		{
			break;
		}
		const float At = bRaise ? List[Index].Time * Data->ShellReloadStartTime
			: Data->ShellReloadStartTime + List[Index].Time * Data->ShellPortLoadTime;
		if (ReloadElapsed < At)
		{
			break;
		}
		if (FX)
		{
			FX->PlayWeaponSound(List[Index].Sound);
		}
		++NextReloadSound;
	}

	// Cada cartucho entra en su momento (el de la ventana, al 40 % de su fase)
	while (ShellsDone < ShellCount)
	{
		const int32 Tube = ShellsDone - (bPortLoad ? 1 : 0);
		const float At = Tube < 0 ? Data->ShellReloadStartTime + Data->ShellPortLoadTime * 0.4f
			: GetShellPhaseStart(Tube) + Data->ShellInsertTime * Data->ReloadAmmoInsertTime;
		if (ReloadElapsed < At)
		{
			break;
		}
		++ShellsDone;
		++Slot.Magazine;
		if (!bInfiniteReserve)
		{
			Slot.Reserve = FMath::Max(0, Slot.Reserve - 1);
		}
		if (FX && Tube >= 0)
		{
			FX->PlayWeaponSound(UBLWeaponData::PickRandom(Data->ShellInsertSounds));
		}
	}

	// Disparar interrumpe (cuando la recámara ya está cerrada con un cartucho)
	if (bTriggerPressedDuringReload && Slot.Magazine > 0 && !IsReloadActionOpen())
	{
		bTriggerPressedDuringReload = false;
		InterruptShellReload();
	}

	if (ReloadElapsed >= ReloadDuration)
	{
		FinishReload();
	}
}

void UBLWeaponComponent::InterruptShellReload()
{
	const UBLWeaponData* Data = GetWeaponData();
	float Alpha;
	int32 Shell;
	const EBLShellReloadPhase Phase = GetShellReloadPhase(Alpha, Shell);
	if (!Data || Phase == EBLShellReloadPhase::Lower || Phase == EBLShellReloadPhase::None)
	{
		return;
	}
	// El cartucho que estaba en la mano vuelve al cinturón; el arma baja desde donde está
	ShellCount = ShellsDone;
	const float LowerStart = GetShellPhaseStart(ShellCount - (bPortLoad ? 1 : 0));
	const float End = Data->ShellReloadEndTime;
	ReloadElapsed = Phase == EBLShellReloadPhase::Raise ? LowerStart + End * (1.f - Alpha) : LowerStart;
	ReloadDuration = LowerStart + End;
}

void UBLWeaponComponent::UpdateCycle(float DeltaTime, const UBLWeaponData* Data)
{
	if (CycleElapsed < 0.f)
	{
		return;
	}
	CycleElapsed += DeltaTime;
	const float Interval = Data->GetShotInterval();
	while (Data->CycleSounds.IsValidIndex(NextCycleSound) && CycleElapsed >= Data->CycleSounds[NextCycleSound].Time * Interval)
	{
		if (FX)
		{
			FX->PlayWeaponSound(Data->CycleSounds[NextCycleSound].Sound);
		}
		++NextCycleSound;
	}
	if (bCasingPending && CycleElapsed >= Data->CasingEjectTime * Interval)
	{
		bCasingPending = false;
		if (FX)
		{
			FX->EjectCasing(Data);
		}
	}
	if (CycleElapsed >= Interval && !bCasingPending && !Data->CycleSounds.IsValidIndex(NextCycleSound))
	{
		CycleElapsed = -1.f;
		if (bReloadQueued)
		{
			bReloadQueued = false;
			StartReload();
		}
	}
}

float UBLWeaponComponent::GetPelletCone() const
{
	const UBLWeaponData* Data = GetWeaponData();
	const IBLWeaponOwner* WeaponOwner = GetWeaponOwner();
	return Data && WeaponOwner ? FMath::Lerp(Data->PelletSpread, Data->PelletAimSpread, WeaponOwner->GetWeaponAimAlpha()) : 0.f;
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
		// En ADS moverse apenas abre el cono (se apunta andando despacio)
		Spread += Data->MoveSpread * FMath::Clamp(Speed / 420.f, 0.f, 1.5f) * (1.f - Aim * 0.85f);
		if (Pawn->GetMovementComponent() && Pawn->GetMovementComponent()->IsFalling())
		{
			Spread += Data->AirSpread;
		}
	}
	Spread += Bloom * FMath::Lerp(1.f, Data->AimBloomMultiplier, Aim);
	return Spread + GetPelletCone();
}

void UBLWeaponComponent::RestoreInventory(const TArray<FBLWeaponSlot>& Snapshot, int32 EquipIndex)
{
	SetTriggerHeld(false);
	CancelReload();
	PendingSlot = INDEX_NONE;
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

	// ---- Trazado: un rayo por perdigón (fusil y pistola: uno). El cono del disparo lleva la dispersión normal y
	//      cada perdigón se abre además en el cono propio del arma ----
	FVector Origin, Direction;
	WeaponOwner->GetWeaponAimView(Origin, Direction);
	const float PelletCone = GetPelletCone();
	const float SpreadRad = FMath::DegreesToRadians(FMath::Max(GetCurrentSpread() - PelletCone, 0.f));
	const FVector ShotDir = SpreadRad > KINDA_SMALL_NUMBER ? FMath::VRandCone(Direction, SpreadRad) : Direction;
	const float PelletRad = FMath::DegreesToRadians(PelletCone);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BLWeaponTrace), true, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	APawn* ShooterPawn = Cast<APawn>(GetOwner());

	// Daño por blanco: los perdigones que alcanzan a la misma víctima cuentan como un impacto en el hitmarker
	struct FTargetHit
	{
		UBLHealthComponent* Health = nullptr;
		float HealthBefore = 0.f;
		EBLHitZone Zone = EBLHitZone::Torso;
	};
	TArray<FTargetHit, TInlineAllocator<4>> Targets;
	FHitResult FirstHit;
	const int32 Pellets = FMath::Max(Data->PelletCount, 1);
	LastPelletHits = 0;
	for (int32 Pellet = 0; Pellet < Pellets; ++Pellet)
	{
		const FVector Dir = Pellets > 1 && PelletRad > KINDA_SMALL_NUMBER ? FMath::VRandCone(ShotDir, PelletRad) : ShotDir;
		const FVector End = Origin + Dir * Data->MaxRange;
		FHitResult Hit;
		BLDamage::WeaponTrace(GetWorld(), Hit, Origin, End, Params);
		if (!Hit.bBlockingHit)
		{
			Hit.TraceStart = Origin;
			Hit.TraceEnd = End;
			Hit.Location = Hit.ImpactPoint = End;
		}
		if (Pellet == 0)
		{
			FirstHit = Hit;
		}
		LastPelletHits += Hit.bBlockingHit ? 1 : 0;

		if (CVarDebugTraces.GetValueOnGameThread())
		{
			DrawDebugLine(GetWorld(), Origin, Hit.ImpactPoint, FColor::Orange, false, 2.f, 0, 0.3f);
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 6.f, FColor::Red, false, 2.f);
		}

		// ---- Daño (sin fuego amigo entre la IA: las balas de un miliciano no hieren a otro) ----
		const APawn* HitPawn = Hit.GetActor() ? Cast<APawn>(Hit.GetActor()) : nullptr;
		const bool bFriendlyAI = ShooterPawn && HitPawn && !ShooterPawn->IsPlayerControlled() && !HitPawn->IsPlayerControlled();
		if (Hit.bBlockingHit && Hit.GetActor() && !bFriendlyAI)
		{
			UBLHealthComponent* TargetHealth = Hit.GetActor()->FindComponentByClass<UBLHealthComponent>();
			if (TargetHealth && !TargetHealth->IsDead())
			{
				FTargetHit* Entry = Targets.FindByPredicate([TargetHealth](const FTargetHit& T) { return T.Health == TargetHealth; });
				const EBLHitZone Zone = BLDamage::ZoneFromBone(Hit.BoneName);
				if (!Entry)
				{
					Entry = &Targets.AddDefaulted_GetRef();
					Entry->Health = TargetHealth;
					Entry->HealthBefore = TargetHealth->GetHealth();
					Entry->Zone = Zone;
				}
				else if (Zone == EBLHitZone::Head)
				{
					Entry->Zone = Zone;  // el hitmarker enseña la mejor zona
				}
			}
			float Amount = ComputeDamage(Hit, Hit.Distance);
			// Tiro del jugador a la cabeza de un enemigo con un arma de una sola bala: letal (sea cual sea el blindaje del
			// cuerpo o la distancia). Aquí se sabe el blanco; ComputeDamage solo conoce el arma
			const APawn* Victim = Cast<APawn>(Hit.GetActor());
			if (TargetHealth && !TargetHealth->IsDead() && Data->bHeadshotKills && Pellets == 1 && ShooterPawn && ShooterPawn->IsPlayerControlled()
				&& Victim && !Victim->IsPlayerControlled() && BLDamage::ZoneFromBone(Hit.BoneName) == EBLHitZone::Head)
			{
				Amount = FMath::Max(Amount, TargetHealth->GetHealth() / FMath::Max(TargetHealth->DamageTakenMultiplier, 0.01f) + 1.f);
			}
			UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Amount, Dir, Hit,
				ShooterPawn ? ShooterPawn->GetController() : nullptr, GetOwner(), UDamageType::StaticClass());
		}
		if (FX && Hit.bBlockingHit)
		{
			FX->PlayImpact(Data, Hit);
		}
	}
	for (const FTargetHit& T : Targets)
	{
		OnHitConfirmed.Broadcast(T.Zone, T.HealthBefore - T.Health->GetHealth(), T.Health->IsDead());
	}

	// ---- Ruido para la IA: el disparo se oye lejos; los impactos se notan cerca ("me están disparando") ----
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Origin, 1.f, GetOwner(), Data->NoiseRange, FName("Disparo"));
	if (FirstHit.bBlockingHit)
	{
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), FirstHit.ImpactPoint, 0.6f, GetOwner(), Data->ImpactNoiseRange, FName("Impacto"));
	}

	// ---- Balas de la IA que pasan rozando al jugador: chasquido supersónico ----
	if (ShooterPawn && !ShooterPawn->IsPlayerControlled())
	{
		if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this))
		{
			Audio->NotifyBulletPass(Origin, FirstHit.ImpactPoint, FirstHit.GetActor());
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
	}
	OnShot.Broadcast(FirstHit);

	if (Data->FireMode != EBLFireMode::Auto)
	{
		bSemiLatch = true;
	}
	if (Data->FireMode == EBLFireMode::Pump || Data->CasingEjectTime > 0.f)
	{
		CycleElapsed = 0.f;
		NextCycleSound = 0;
		bCasingPending = Data->CasingEjectTime > 0.f;
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
	if (PendingSlot != INDEX_NONE)
	{
		HolsterRemaining -= DeltaTime;
		if (HolsterRemaining <= 0.f)
		{
			// Abajo del todo: cambia de malla (fuera de la vista) y sube la nueva
			const int32 NewIndex = PendingSlot;
			PendingSlot = INDEX_NONE;
			EquipSlot(NewIndex);
			if (FX)
			{
				FX->PlayHandling(Inventory[NewIndex].Data, 0.8f);
			}
			OnWeaponSwitched.Broadcast();
			return;
		}
	}
	UnblockedTime = WeaponOwner->IsWeaponBlocked() ? 0.f : UnblockedTime + DeltaTime;
	Bloom = FMath::Max(0.f, Bloom - Data->BloomRecovery * DeltaTime * (bTriggerHeld ? 0.35f : 1.f));

	UpdateReload(DeltaTime);
	UpdateCycle(DeltaTime, Data);

	// ---- Disparo: acumulador de tiempo para respetar la cadencia con cualquier frame rate ----
	ShotTimer -= DeltaTime;
	if (bTriggerHeld)
	{
		// (corredera: el gatillo que sigue apretado tras el último disparo no "chasca"; hace falta otra pulsación)
		if (Slot->Magazine == 0 && !bReloading && PendingSlot == INDEX_NONE && !(Data->FireMode == EBLFireMode::Pump && bSemiLatch))
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
