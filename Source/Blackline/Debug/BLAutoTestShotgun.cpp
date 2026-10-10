// Prueba automática de la escopeta SG-12 y de las armas del suelo.
//  "Shotgun" (mapa L_Dev_Movement, estaciones "Tiro" y "Dianas"):
//   recogerla del suelo (cambio por el AR-7, que queda en el suelo), manos, perdigones, bombeo (guardamanos y mano
//   izquierda), daño a un maniquí, ADS, recarga táctica cartucho a cartucho, disparar interrumpe la recarga, vaciar
//   (corredera atrás), recarga en vacío por la ventana, cambio a la pistola y vuelta, munición del suelo, devolver el arma.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Animation/BLFirstPersonAnimInstance.h"
#include "Animation/BLWeaponAnimInstance.h"
#include "Combat/BLHealthComponent.h"
#include "Combat/BLTargetDummy.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLWeaponPickup.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	ABLWeaponPickup* FindPickup(UWorld* World, const FString& DataName)
	{
		for (TActorIterator<ABLWeaponPickup> It(World); It; ++It)
		{
			if (IsValid(*It) && GetNameSafe(It->WeaponData) == DataName)
			{
				return *It;
			}
		}
		return nullptr;
	}
}

void UBLAutoTestComponent::BuildShotgunTest()
{
	UBLWeaponComponent* W = Char()->GetWeapon();
	W->OnShot.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleShot);
	Char()->GetHealth()->DamageTakenMultiplier = 1.f;

	struct FState
	{
		int32 Shots0 = 0;
		int32 Mag0 = 0;
		int32 Reserve0 = 0;
		float PumpMax = 0.f;
		float HandOnPump = 100.f;
		int32 PelletHits = 0;
		float HealthBefore = 0.f;
		float ShotAt = -1.f;
		bool bA = false;
		bool bB = false;
		bool bC = false;
		int32 Captures = 0;
	};
	TSharedRef<FState> St = MakeShared<FState>();
	UBLWeaponData* SG12 = LoadObject<UBLWeaponData>(nullptr, TEXT("/Game/Weapons/SG12/DA_SG12.DA_SG12"));

	auto IsShotgun = [W]() { return GetNameSafe(W->GetWeaponData()) == TEXT("DA_SG12"); };
	auto Pump = [this]()
	{
		const UBLWeaponAnimInstance* WA = Cast<UBLWeaponAnimInstance>(Char()->GetWeaponMesh()->GetAnimInstance());
		return WA ? WA->GetMechState().BoltOffset : FVector::ZeroVector;
	};
	// Distancia de la mano izquierda (socket del Mannequin) al agarre del guardamanos desplazado con el bombeo
	auto HandToForend = [this, Pump]()
	{
		const USkeletalMeshComponent* WM = Char()->GetWeaponMesh();
		const FVector Grip = Char()->GetFirstPersonAnim()->GetState().LeftGripInWeapon.GetLocation() + Pump();
		return float(FVector::Dist(Char()->GetFirstPersonMesh()->GetSocketLocation(TEXT("HandGrip_L")), WM->GetComponentTransform().TransformPosition(Grip)));
	};
	auto Look = [this](const FVector& Target)
	{
		const FVector Eye = Char()->GetCamera()->GetComponentLocation();
		Char()->GetController()->SetControlRotation((Target - Eye).Rotation());
	};
	auto LookForward = [this]() { Char()->GetController()->SetControlRotation(Char()->GetActorRotation()); };
	auto Begin = [this, W, St]()
	{
		St->Shots0 = ShotsAtStart = W->GetShotsFired();
		St->Mag0 = W->GetMagazine();
		St->Reserve0 = W->GetReserve();
		HitsCount = 0;
	};
	// Pulsar el gatillo un instante (la escopeta dispara al apretar y hay que soltarlo para el siguiente)
	auto Tap = [this](float T, float At) { Char()->SetFireHeld(T >= At && T < At + 0.08f); };

	// ---- Recoger del suelo ----
	Steps.Add({ TEXT("Recoger"), 2.2f,
		[this, SG12, St]()
		{
			TeleportToStart(TEXT("Tiro"));
			const FVector Spot = Char()->GetActorLocation() + Char()->GetActorForwardVector() * 80.f - FVector(0.f, 0.f, 90.f);
			FBLWeaponSlot Slot;
			Slot.Data = SG12;
			Slot.Magazine = SG12 ? SG12->MagazineSize : 0;
			Slot.Reserve = SG12 ? SG12->StartReserveAmmo : 0;
			ABLWeaponPickup::SpawnDrop(Char(), Slot, Spot);
			St->bA = false;
		},
		[this, St, Look](float T)
		{
			if (ABLWeaponPickup* P = FindPickup(GetWorld(), TEXT("DA_SG12")))
			{
				Look(P->GetInteractLocation());
				if (T > 0.3f && !St->bA) { St->bA = true; Screenshot(TEXT("Shotgun_Suelo")); }
				Char()->SetInteractHeld(T > 0.4f);
			}
			else
			{
				Char()->SetInteractHeld(false);
			}
		},
		[this, W, IsShotgun](FString& D)
		{
			const ABLWeaponPickup* AR = FindPickup(GetWorld(), TEXT("DA_AR7"));
			D = FString::Printf(TEXT("%d armas, en la mano %s %d/%d (hueco %d); AR-7 en el suelo=%d (%d+%d); SG-12 en el suelo=%d"), W->GetWeaponCount(),
				*GetNameSafe(W->GetWeaponData()), W->GetMagazine(), W->GetReserve(), W->GetCurrentIndex(), AR != nullptr, AR ? AR->Magazine : 0,
				AR ? AR->Reserve : 0, FindPickup(GetWorld(), TEXT("DA_SG12")) != nullptr);
			return W->GetWeaponCount() == 2 && IsShotgun() && W->GetCurrentIndex() == 0 && W->GetMagazine() == 7 && W->GetReserve() == 21
				&& AR && !FindPickup(GetWorld(), TEXT("DA_SG12")) && !W->IsEquipping();
		},
		[this]() { Char()->SetInteractHeld(false); } });

	Steps.Add({ TEXT("Manos"), 0.6f, [this, LookForward]() { TeleportToStart(TEXT("Tiro")); LookForward(); }, nullptr,
		[this, W, HandToForend](FString& D)
		{
			const USkeletalMeshComponent* WM = Char()->GetWeaponMesh();
			const float Right = FVector::Dist(Char()->GetFirstPersonMesh()->GetSocketLocation(TEXT("HandGrip_R")),
				WM->GetComponentTransform().TransformPosition(W->GetWeaponData()->RightHandOffset));
			D = FString::Printf(TEXT("mano izquierda a %.1f cm del guardamanos, derecha a %.1f cm del pistolete (esperado < 3)"), HandToForend(), Right);
			return HandToForend() < 3.f && Right < 3.f;
		}, nullptr, 0.4f });

	// ---- Disparo: perdigones y bombeo ----
	Steps.Add({ TEXT("Perdigones"), 1.1f,
		[this, St, Begin]() { Begin(); St->PumpMax = 0.f; St->HandOnPump = 100.f; St->PelletHits = 0; PendingShotScreenshot = TEXT("Shotgun_Disparo"); ShotScreenshotAt = 1; },
		[this, W, St, Tap, Pump, HandToForend](float T)
		{
			Tap(T, 0.f);
			if (W->GetShotsFired() > St->Shots0 && St->PelletHits == 0) { St->PelletHits = W->GetLastPelletHits(); }
			const float P = Pump().Size();
			if (P > St->PumpMax) { St->PumpMax = P; St->HandOnPump = HandToForend(); }
			if (T > 0.36f && St->Captures == 0) { St->Captures = 1; Screenshot(TEXT("Shotgun_Bombeo")); }
			// Segunda pulsación a mitad del bombeo: no debe disparar
			if (T > 0.3f && T < 0.38f) { Char()->SetFireHeld(true); }
		},
		[W, St](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - St->Shots0;
			D = FString::Printf(TEXT("%d disparo (2ª pulsación a 0,3 s ignorada), %d/9 perdigones en el muro a 10 m, guardamanos %.1f cm atrás (recorrido %.0f), "
				"mano a %.1f cm de él, cargador %d->%d"), Shots, St->PelletHits, St->PumpMax, W->GetWeaponData()->BoltTravel, St->HandOnPump, St->Mag0, W->GetMagazine());
			return Shots == 1 && St->PelletHits >= 7 && St->PumpMax > W->GetWeaponData()->BoltTravel * 0.9f && St->HandOnPump < 4.f
				&& W->GetMagazine() == St->Mag0 - 1;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("Maniqui"), 1.4f,
		[this, St]()
		{
			TeleportToStart(TEXT("Dianas"));
			St->bA = false;
			St->HealthBefore = 0.f;
			TArray<AActor*> Found;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("BLDummy_2")), Found);
			if (const ABLTargetDummy* Dm = Found.Num() > 0 ? Cast<ABLTargetDummy>(Found[0]) : nullptr)
			{
				St->HealthBefore = Dm->GetHealth()->GetHealth();
			}
		},
		[this, St, Look](float T)
		{
			TArray<AActor*> Found;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("BLDummy_2")), Found);
			const ABLTargetDummy* Dm = Found.Num() > 0 ? Cast<ABLTargetDummy>(Found[0]) : nullptr;
			if (Dm && !St->bA)
			{
				Look(Dm->GetMesh()->GetBoneLocation(TEXT("spine_03")));
			}
			if (T > 0.6f && !St->bA) { St->bA = true; Char()->SetFireHeld(true); }
			else if (St->bA) { Char()->SetFireHeld(false); }
		},
		[this, St](FString& D)
		{
			TArray<AActor*> Found;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("BLDummy_2")), Found);
			const ABLTargetDummy* Dm = Found.Num() > 0 ? Cast<ABLTargetDummy>(Found[0]) : nullptr;
			const float Lost = Dm ? St->HealthBefore - Dm->GetHealth()->GetHealth() : 0.f;
			const bool bDead = Dm && Dm->GetHealth()->IsDead();
			D = FString::Printf(TEXT("maniquí a %.0f m: daño %.0f de %.0f, muerto=%d, hitmarker hace %.2f s"),
				Dm ? FVector::Dist(Dm->GetActorLocation(), Char()->GetActorLocation()) / 100.f : -1.f, Lost, St->HealthBefore, bDead, Char()->GetHitMarkerAge());
			return Dm && (Lost >= 48.f || bDead) && Char()->GetHitMarkerAge() < 1.2f;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("ADS"), 1.3f,
		[this, LookForward]() { TeleportToStart(TEXT("Tiro")); LookForward(); Char()->SetAimHeld(true); },
		nullptr,
		[this, W](FString& D)
		{
			const UCameraComponent* Cam = Char()->GetCamera();
			const FVector S = Char()->GetWeaponMesh()->GetSocketLocation(W->GetWeaponData()->SightSocket) - Cam->GetComponentLocation();
			const FVector F = Cam->GetForwardVector();
			const float Off = (S - F * FVector::DotProduct(S, F)).Size();
			D = FString::Printf(TEXT("aim %.2f, cono %.2f grados; anillo a %.1f cm del ojo y %.2f cm del eje"), Char()->GetAimAlpha(), W->GetCurrentSpread(),
				FVector::DotProduct(S, F), Off);
			return Char()->GetAimAlpha() > 0.98f && Off < 0.5f && W->GetCurrentSpread() < 3.f;
		},
		[this]() { Char()->SetAimHeld(false); }, 1.1f });

	// ---- Recarga cartucho a cartucho ----
	Steps.Add({ TEXT("Gastar2"), 1.9f, [Begin]() { Begin(); },
		[this](float T) { Char()->SetFireHeld((T >= 0.05f && T < 0.13f) || (T >= 0.95f && T < 1.03f)); },
		[W, St](FString& D)
		{
			D = FString::Printf(TEXT("%d disparos, cargador %d"), W->GetShotsFired() - St->Shots0, W->GetMagazine());
			return W->GetShotsFired() - St->Shots0 == 2;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("RecargaTactica"), 4.0f,
		[this, St, Begin]() { Begin(); St->Captures = 0; Char()->DoReload(); },
		[this, St](float T)
		{
			// 0,35 s girar + 0,5 s por cartucho (entra al 72 %) + 0,35 s volver
			if (T > 0.6f && St->Captures == 0) { St->Captures = 1; Screenshot(TEXT("Shotgun_Cinturon")); }
			if (T > 0.82f && St->Captures == 1) { St->Captures = 2; Screenshot(TEXT("Shotgun_Portilla")); }
		},
		[W, St](FString& D)
		{
			const int32 Loaded = W->GetMagazine() - St->Mag0;
			D = FString::Printf(TEXT("cargador %d->%d (7+1), reserva %d->%d, recargando=%d"), St->Mag0, W->GetMagazine(), St->Reserve0, W->GetReserve(), W->IsReloading());
			return W->GetMagazine() == 8 && St->Reserve0 - W->GetReserve() == Loaded && !W->IsReloading();
		},
		nullptr, -1.f, [W]() { return !W->IsReloading(); } });

	Steps.Add({ TEXT("Gastar3"), 2.6f, [Begin]() { Begin(); },
		[this](float T) { Char()->SetFireHeld(FMath::Fmod(T, 0.9f) < 0.08f); },
		[W, St](FString& D)
		{
			D = FString::Printf(TEXT("%d disparos, cargador %d"), W->GetShotsFired() - St->Shots0, W->GetMagazine());
			return W->GetShotsFired() - St->Shots0 == 3 && W->GetMagazine() == 5;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("Interrumpir"), 2.0f,
		[this, St, Begin]() { Begin(); St->ShotAt = -1.f; Char()->DoReload(); },
		[this, W, St](float T)
		{
			// El primer cartucho entra a 0,71 s; disparar a 0,9 s corta la recarga y la escopeta dispara al volver
			if (St->ShotAt < 0.f && W->GetShotsFired() > St->Shots0) { St->ShotAt = T; }
			Char()->SetFireHeld(T > 0.9f && St->ShotAt < 0.f);
		},
		[W, St](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d -> %d (1 cartucho y 1 disparo), disparo a %.2f s, recargando=%d"), St->Mag0, W->GetMagazine(), St->ShotAt, W->IsReloading());
			return W->GetMagazine() == St->Mag0 && St->ShotAt > 0.9f && St->ShotAt < 1.6f && !W->IsReloading();
		},
		[this]() { Char()->SetFireHeld(false); } });

	// ---- Vaciar y recarga en vacío ----
	Steps.Add({ TEXT("Vaciar"), 7.0f,
		[St, Begin]() { Begin(); St->PumpMax = 0.f; St->ShotAt = -1.f; St->bB = false; },
		[this, W, St, Pump](float T)
		{
			if (W->GetMagazine() > 0 && St->ShotAt < 0.f) { Char()->SetFireHeld(FMath::Fmod(T, 0.9f) < 0.08f); return; }
			if (St->ShotAt < 0.f) { St->ShotAt = T; }
			const float Since = T - St->ShotAt;
			if (Since < 1.2f)
			{
				Char()->SetFireHeld(false);
				if (Since > 0.9f) { St->PumpMax = FMath::Max(St->PumpMax, Pump().Size()); }
				if (Since > 1.0f && !St->bB) { St->bB = true; Screenshot(TEXT("Shotgun_Abierta")); }
			}
			else if (!W->IsReloading()) { Char()->SetFireHeld(Since < 1.3f); }   // gatillo en vacío -> recarga automática
		},
		[W, St](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d, guardamanos atrás %.1f cm tras el último disparo, recarga automática=%d"), W->GetMagazine(), St->PumpMax, W->IsReloading());
			return W->GetMagazine() == 0 && St->PumpMax > W->GetWeaponData()->BoltTravel * 0.9f && W->IsReloading();
		},
		[this]() { Char()->SetFireHeld(false); }, -1.f, [W]() { return W->IsReloading(); } });

	Steps.Add({ TEXT("RecargaVacio"), 6.0f,
		[St, Begin]() { Begin(); St->Captures = 0; St->bC = false; },
		[this, W, St](float T)
		{
			// Girar 0,35 + por la ventana 0,95 (cierra a 1,03 s) + 0,5 por cartucho + volver 0,35
			if (T > 0.55f && St->Captures == 0) { St->Captures = 1; Screenshot(TEXT("Shotgun_Ventana")); }
			if (T > 0.95f && St->Captures == 1) { St->Captures = 2; Screenshot(TEXT("Shotgun_Cierra")); }
			St->bC |= W->GetMagazine() >= 1 && !W->IsReloadActionOpen() && W->IsReloading();
		},
		[W, St, Pump](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d (7+1), reserva %d->%d, guardamanos %.2f cm, cerrada con un cartucho antes del depósito=%d"),
				W->GetMagazine(), St->Reserve0, W->GetReserve(), Pump().Size(), St->bC);
			return W->GetMagazine() == FMath::Min(8, St->Reserve0) && !W->IsReloading() && Pump().Size() < 0.1f && St->bC;
		},
		nullptr, -1.f, [W]() { return !W->IsReloading(); } });

	// ---- Cambio de arma ----
	Steps.Add({ TEXT("Pistola"), 2.4f,
		[this, St, W]() { St->Mag0 = W->GetMagazine(); St->bA = false; Char()->SwitchWeapon(1); },
		[this, W, St](float T) { if (T > 1.1f && !St->bA) { St->bA = true; Char()->SwitchWeapon(0); } },
		[W, St, IsShotgun](FString& D)
		{
			const FBLWeaponSlot* P = W->GetSlot(1);
			D = FString::Printf(TEXT("pistola %s y vuelta: %s %d (antes %d)"), P ? *GetNameSafe(P->Data) : TEXT("-"), *GetNameSafe(W->GetWeaponData()), W->GetMagazine(), St->Mag0);
			return IsShotgun() && W->GetMagazine() == St->Mag0 && !W->IsEquipping() && P && GetNameSafe(P->Data) == TEXT("DA_P17");
		} });

	// ---- Munición y devolver el arma ----
	Steps.Add({ TEXT("Municion"), 1.6f,
		[this, SG12, St, W]()
		{
			TeleportToStart(TEXT("Tiro"));
			St->Reserve0 = W->GetReserve();
			FBLWeaponSlot Slot;
			Slot.Data = SG12;
			Slot.Magazine = 3;
			Slot.Reserve = 6;
			ABLWeaponPickup::SpawnDrop(Char(), Slot, Char()->GetActorLocation() + Char()->GetActorForwardVector() * 80.f - FVector(0.f, 0.f, 90.f));
		},
		[this, Look](float T)
		{
			if (ABLWeaponPickup* P = FindPickup(GetWorld(), TEXT("DA_SG12"))) { Look(P->GetInteractLocation()); Char()->SetInteractHeld(T > 0.3f); }
			else { Char()->SetInteractHeld(false); }
		},
		[this, W, St](FString& D)
		{
			D = FString::Printf(TEXT("reserva %d -> %d (+9), escopeta del suelo=%d"), St->Reserve0, W->GetReserve(), FindPickup(GetWorld(), TEXT("DA_SG12")) != nullptr);
			return W->GetReserve() == FMath::Min(St->Reserve0 + 9, W->GetWeaponData()->MaxReserveAmmo) && !FindPickup(GetWorld(), TEXT("DA_SG12"));
		},
		[this]() { Char()->SetInteractHeld(false); } });

	Steps.Add({ TEXT("Devolver"), 2.4f,
		[this, St, W]() { St->Mag0 = W->GetMagazine(); St->Reserve0 = W->GetReserve(); },
		[this, Look](float T)
		{
			if (ABLWeaponPickup* P = FindPickup(GetWorld(), TEXT("DA_AR7")))
			{
				Look(P->GetInteractLocation());
				Char()->SetInteractHeld(T > 0.2f && T < 1.0f);
			}
			else
			{
				Char()->SetInteractHeld(false);
			}
		},
		[this, W, St](FString& D)
		{
			const ABLWeaponPickup* SG = FindPickup(GetWorld(), TEXT("DA_SG12"));
			D = FString::Printf(TEXT("en la mano %s %d/%d; escopeta en el suelo con %d+%d (llevaba %d+%d); AR-7 del suelo=%d"),
				*GetNameSafe(W->GetWeaponData()), W->GetMagazine(), W->GetReserve(), SG ? SG->Magazine : -1, SG ? SG->Reserve : -1,
				St->Mag0, St->Reserve0, FindPickup(GetWorld(), TEXT("DA_AR7")) != nullptr);
			return GetNameSafe(W->GetWeaponData()) == TEXT("DA_AR7") && SG && SG->Magazine == St->Mag0 && SG->Reserve == St->Reserve0
				&& !FindPickup(GetWorld(), TEXT("DA_AR7")) && W->GetWeaponCount() == 2;
		},
		[this]() { Char()->SetInteractHeld(false); }, 2.0f });
}
