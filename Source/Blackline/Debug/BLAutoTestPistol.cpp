// Prueba automática de la pistola P-17 y del cambio de arma.
//  "Pistol" (mapa L_Dev_Movement, estación "Tiro": muro a 10 m):
//   inventario, cambio AR-7 -> P-17 (tiempo), manos en el puño, semiautomática y cadencia, retroceso y recuperación,
//   ADS centrada, recarga táctica (el cargador cae y queda en el suelo), vaciar (corredera abierta) y recarga en vacío
//   (montar la corredera), cambio a mitad de recarga, AR-7 de nuevo, rueda, arrepentirse a medio cambio, sprint y checkpoint.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Animation/BLFirstPersonAnimInstance.h"
#include "Animation/BLWeaponAnimInstance.h"
#include "Player/BLCharacter.h"
#include "Player/BLFirstPersonRigComponent.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLWeaponFXComponent.h"

#include "AnimationRuntime.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"

void UBLAutoTestComponent::BuildPistolTest()
{
	UBLWeaponComponent* W = Char()->GetWeapon();
	W->OnShot.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleShot);
	ShotScreenshotAt = 1;

	auto IsPistol = [W]() { const UBLWeaponData* D = W->GetWeaponData(); return D && D->GetName() == TEXT("DA_P17"); };
	auto Pitch = [this]() { return Char()->GetController() ? Char()->GetController()->GetControlRotation().GetNormalized().Pitch : 0.f; };
	auto BeginShots = [this, W, Pitch]()
	{
		ShotsAtStart = W->GetShotsFired();
		HitsCount = 0;
		MagAtStart = W->GetMagazine();
		ReserveAtStart = W->GetReserve();
		PitchAtStart = PeakPitch = Pitch();
	};
	// Desplazamiento real del hueso de la corredera respecto a la pose de referencia (cm)
	auto SlideMoved = [this, W]()
	{
		const USkeletalMeshComponent* WM = Char()->GetWeaponMesh();
		const int32 Bi = WM->GetBoneIndex(W->GetWeaponData()->BoltBone);
		return Bi == INDEX_NONE ? -1.f : float(FVector::Dist(WM->GetBoneTransform(Bi, FTransform::Identity).GetLocation(),
			FAnimationRuntime::GetComponentSpaceTransformRefPose(WM->GetSkeletalMeshAsset()->GetRefSkeleton(), Bi).GetLocation()));
	};
	auto Dropped = [this]()
	{
		const UBLWeaponFXComponent* FX = Char()->FindComponentByClass<UBLWeaponFXComponent>();
		return FX ? FX->GetDroppedMagazineCount() : -1;
	};
	TSharedRef<float> SwitchTime = MakeShared<float>(-1.f);
	TSharedRef<float> SlideOpen = MakeShared<float>(0.f);
	TSharedRef<int32> DroppedBefore = MakeShared<int32>(0);
	TSharedRef<int32> AR7Mag = MakeShared<int32>(0);

	Steps.Add({ TEXT("Inventario"), 1.5f,
		[this]() { TeleportToStart(TEXT("Tiro")); },
		nullptr,
		[W, AR7Mag](FString& D)
		{
			const FBLWeaponSlot* S1 = W->GetSlot(1);
			*AR7Mag = W->GetMagazine();
			D = FString::Printf(TEXT("%d armas; en la mano %s (%d); secundaria %s %d/%d"), W->GetWeaponCount(), *GetNameSafe(W->GetWeaponData()), W->GetMagazine(),
				S1 ? *GetNameSafe(S1->Data) : TEXT("-"), S1 ? S1->Magazine : 0, S1 ? S1->Reserve : 0);
			return W->GetWeaponCount() == 2 && W->GetCurrentIndex() == 0 && S1 && S1->Data && S1->Magazine == 17 && S1->Reserve == 51;
		} });

	Steps.Add({ TEXT("Cambio"), 1.3f,
		[this, SwitchTime]() { *SwitchTime = -1.f; Char()->SwitchWeapon(1); },
		[W, IsPistol, SwitchTime](float T) { if (*SwitchTime < 0.f && IsPistol() && !W->IsEquipping()) { *SwitchTime = T; } },
		[W, IsPistol, SwitchTime](FString& D)
		{
			D = FString::Printf(TEXT("%s lista en %.2f s (bajar 0,30 + subir 0,45)"), *GetNameSafe(W->GetWeaponData()), *SwitchTime);
			return IsPistol() && *SwitchTime > 0.6f && *SwitchTime < 0.95f;
		},
		nullptr, 1.2f });

	Steps.Add({ TEXT("Manos"), 0.3f, nullptr, nullptr,
		[this, W](FString& D)
		{
			const USkeletalMeshComponent* FP = Char()->GetFirstPersonMesh();
			const USkeletalMeshComponent* WM = Char()->GetWeaponMesh();
			const float Left = FVector::Dist(FP->GetSocketLocation(TEXT("HandGrip_L")), WM->GetSocketLocation(W->GetWeaponData()->LeftHandSocket));
			const float Right = FVector::Dist(FP->GetSocketLocation(TEXT("HandGrip_R")), WM->GetComponentTransform().TransformPosition(W->GetWeaponData()->RightHandOffset));
			D = FString::Printf(TEXT("mano izquierda a %.1f cm del agarre, derecha a %.1f cm del puño (esperado < 3)"), Left, Right);
			// Dónde quedan los huesos de las manos en espacio del arma (ajuste del encuadre: no deben tapar la corredera)
			for (const TCHAR* Bone : { TEXT("hand_r"), TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r"), TEXT("index_metacarpal_r"),
				TEXT("index_01_r"), TEXT("middle_01_r"), TEXT("pinky_01_r"), TEXT("hand_l"), TEXT("thumb_03_l"), TEXT("index_01_l"), TEXT("middle_03_l") })
			{
				const int32 Bi = FP->GetBoneIndex(Bone);
				if (Bi != INDEX_NONE)
				{
					const FVector L = WM->GetComponentTransform().InverseTransformPosition(FP->GetBoneLocation(Bone));
					D += FString::Printf(TEXT(" %s=(%.1f,%.1f,%.1f)"), Bone, L.X, L.Y, L.Z);
				}
			}
			return Left < 3.f && Right < 3.f;
		} });

	Steps.Add({ TEXT("Semi"), 1.0f,
		[this, BeginShots]() { BeginShots(); Char()->SetFireHeld(true); },
		nullptr,
		[this, W](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - ShotsAtStart;
			D = FString::Printf(TEXT("gatillo apretado 1 s: %d disparo(s) (esperado 1), impactos %d"), Shots, HitsCount);
			return Shots == 1 && HitsCount == 1;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("Cadencia"), 1.4f,
		[this, BeginShots]() { BeginShots(); },
		[this, Pitch](float T) { Char()->SetFireHeld(FMath::Fmod(T, 0.2f) < 0.1f); PeakPitch = FMath::Max(PeakPitch, Pitch()); },
		[this, W](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - ShotsAtStart;
			const float Rise = PeakPitch - PitchAtStart;
			D = FString::Printf(TEXT("%d disparos pulsando cada 0,2 s (esperado 6-8), impactos %d, cargador %d->%d, retroceso +%.1f grados"),
				Shots, HitsCount, MagAtStart, W->GetMagazine(), Rise);
			return Shots >= 6 && Shots <= 8 && HitsCount == Shots && W->GetMagazine() == MagAtStart - Shots && Rise > 0.8f && Rise < 10.f;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("Recuperacion"), 0.8f, nullptr, nullptr,
		[this, Pitch](FString& D)
		{
			const float Now = Pitch() - PitchAtStart;
			const float Peak = PeakPitch - PitchAtStart;
			// En semiautomática se recupera entre disparos: queda solo la parte no recuperable (15 %)
			D = FString::Printf(TEXT("mira +%.1f -> +%.1f grados"), Peak, Now);
			return Now < Peak - 0.3f && Now > -0.5f;
		} });

	Steps.Add({ TEXT("ADS"), 1.2f,
		[this]() { TeleportToStart(TEXT("Tiro")); Char()->SetAimHeld(true); },
		nullptr,
		[this, W](FString& D)
		{
			// La mira (logica) debe quedar en el eje de la cámara
			const UCameraComponent* Cam = Char()->GetCamera();
			const FVector S = Char()->GetWeaponMesh()->GetSocketLocation(W->GetWeaponData()->SightSocket) - Cam->GetComponentLocation();
			const FVector F = Cam->GetForwardVector();
			const float Off = (S - F * FVector::DotProduct(S, F)).Size();
			D = FString::Printf(TEXT("aim %.2f dispersión %.2f grados; mira a %.1f cm del ojo y %.2f cm del eje"), Char()->GetAimAlpha(), W->GetCurrentSpread(),
				FVector::DotProduct(S, F), Off);
			return Char()->GetAimAlpha() > 0.98f && W->GetCurrentSpread() < 0.5f && Off < 0.5f;
		},
		nullptr, 1.0f });

	Steps.Add({ TEXT("DisparoADS"), 0.8f,
		[this, BeginShots]() { BeginShots(); PendingShotScreenshot = TEXT("DisparoADS"); Char()->SetFireHeld(true); },
		nullptr,
		[this, W](FString& D)
		{
			D = FString::Printf(TEXT("%d disparo en ADS, impactos %d"), W->GetShotsFired() - ShotsAtStart, HitsCount);
			return W->GetShotsFired() - ShotsAtStart == 1 && HitsCount == 1;
		},
		[this]() { Char()->SetFireHeld(false); Char()->SetAimHeld(false); } });

	Steps.Add({ TEXT("RecargaTactica"), 2.1f,
		[this, W, Dropped, DroppedBefore]() { MagAtStart = W->GetMagazine(); ReserveAtStart = W->GetReserve(); *DroppedBefore = Dropped(); bJumpPressed = false; bSawMantle = false; MaxSprites = 0; Char()->DoReload(); },
		[this](float T)
		{
			// 1,55 s: cae el cargador a 0,11 s, coge otro a ~0,56 s, lo mete a 0,85 s
			if (T > 0.16f && !bJumpPressed) { bJumpPressed = true; Screenshot(TEXT("Recarga_Cae")); }
			if (T > 0.62f && !bSawMantle) { bSawMantle = true; Screenshot(TEXT("Recarga_Nuevo")); }
			if (T > 0.8f && MaxSprites == 0) { MaxSprites = 1; Screenshot(TEXT("Recarga_Mete")); }
		},
		[W, Dropped, DroppedBefore](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d (17+1) reserva %d recargando=%d; cargadores en el suelo %d -> %d"), W->GetMagazine(), W->GetReserve(), W->IsReloading(), *DroppedBefore, Dropped());
			return W->GetMagazine() == 18 && !W->IsReloading() && Dropped() == *DroppedBefore + 1;
		} });

	TSharedRef<float> EmptyAt = MakeShared<float>(-1.f);
	Steps.Add({ TEXT("Vaciar"), 5.0f,
		[this, SlideOpen, EmptyAt]() { *SlideOpen = 0.f; *EmptyAt = -1.f; bJumpPressed = false; },
		[this, W, SlideMoved, SlideOpen, EmptyAt](float T)
		{
			if (W->GetMagazine() > 0 && *EmptyAt < 0.f) { Char()->SetFireHeld(FMath::Fmod(T, 0.2f) < 0.1f); return; }
			if (*EmptyAt < 0.f) { *EmptyAt = T; }
			if (T < *EmptyAt + 0.6f)
			{
				Char()->SetFireHeld(false);
				*SlideOpen = FMath::Max(*SlideOpen, SlideMoved());
				if (T > *EmptyAt + 0.4f && !bJumpPressed) { bJumpPressed = true; Screenshot(TEXT("Vaciar_Abierta")); }
			}
			else if (!W->IsReloading()) { Char()->SetFireHeld(true); }   // gatillo en vacío -> recarga automática
		},
		[W, SlideOpen](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d, corredera abierta %.2f cm (recorrido %.1f), recarga automática=%d"), W->GetMagazine(), *SlideOpen, W->GetWeaponData()->BoltTravel, W->IsReloading());
			return W->GetMagazine() == 0 && *SlideOpen >= W->GetWeaponData()->BoltTravel * 0.9f && W->IsReloading();
		},
		[this]() { Char()->SetFireHeld(false); }, -1.f,
		[W]() { return W->IsReloading(); } });

	Steps.Add({ TEXT("RecargaVacio"), 2.2f,
		[this]() { bJumpPressed = false; bSawMantle = false; },
		[this](float T)
		{
			// 1,9 s desde ahora: cae a 0,13 s, mete el cargador a 1,05 s, agarra la corredera a 1,35 s y la suelta a 1,46 s
			if (T > 0.5f && !bJumpPressed) { bJumpPressed = true; Screenshot(TEXT("Vacio_Bolsa")); }
			if (T > 1.38f && !bSawMantle) { bSawMantle = true; Screenshot(TEXT("Vacio_Corredera")); }
		},
		[W, SlideMoved](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d reserva %d, corredera %.2f cm"), W->GetMagazine(), W->GetReserve(), SlideMoved());
			return W->GetMagazine() == 17 && !W->IsReloading() && SlideMoved() < 0.05f;
		} });

	Steps.Add({ TEXT("CambioEnRecarga"), 1.6f,
		[this, W]() { MagAtStart = W->GetMagazine(); Char()->DoReload(); bJumpPressed = false; },
		[this](float T) { if (T > 0.3f && !bJumpPressed) { bJumpPressed = true; Char()->SwitchWeapon(0); } },
		[W, AR7Mag](FString& D)
		{
			const FBLWeaponSlot* P = W->GetSlot(1);
			D = FString::Printf(TEXT("en la mano %s %d (antes %d), pistola %d sin recargar, recargando=%d"), *GetNameSafe(W->GetWeaponData()), W->GetMagazine(), *AR7Mag, P ? P->Magazine : -1, W->IsReloading());
			return W->GetCurrentIndex() == 0 && W->GetMagazine() == *AR7Mag && P && P->Magazine == 17 && !W->IsReloading() && !W->IsEquipping();
		} });

	Steps.Add({ TEXT("RafagaAR7"), 1.0f,
		[this, BeginShots]() { BeginShots(); Char()->SetFireHeld(true); },
		nullptr,
		[this, W](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - ShotsAtStart;
			D = FString::Printf(TEXT("AR-7 tras el cambio: %d disparos en 1 s (esperado 12-14), impactos %d"), Shots, HitsCount);
			return Shots >= 12 && Shots <= 14 && HitsCount == Shots;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("Rueda"), 1.2f,
		[this]() { Char()->CycleWeapon(); },
		nullptr,
		[W, IsPistol](FString& D)
		{
			D = FString::Printf(TEXT("siguiente arma: %s"), *GetNameSafe(W->GetWeaponData()));
			return IsPistol() && !W->IsEquipping();
		} });

	Steps.Add({ TEXT("Arrepentido"), 1.0f,
		[this]() { bJumpPressed = false; Char()->SwitchWeapon(0); },
		[this](float T) { if (T > 0.15f && !bJumpPressed) { bJumpPressed = true; Char()->SwitchWeapon(1); } },
		[W, IsPistol](FString& D)
		{
			D = FString::Printf(TEXT("pide el AR-7 y a 0,15 s la pistola otra vez: %s, equipando=%d"), *GetNameSafe(W->GetWeaponData()), W->IsEquipping());
			return IsPistol() && !W->IsEquipping();
		} });

	Steps.Add({ TEXT("Sprint"), 1.2f,
		[this]() { TeleportToStart(TEXT("Tiro")); Char()->SetSprintHeld(true); },
		[this](float) { Char()->DoMove(0.f, 1.f); },
		[this](FString& D)
		{
			D = FString::Printf(TEXT("sprint con la pistola: %d (alfa %.2f)"), Char()->IsSprinting(), Char()->GetSprintAlpha());
			return Char()->IsSprinting();
		},
		[this]() { Char()->SetSprintHeld(false); }, 1.0f });

	Steps.Add({ TEXT("Checkpoint"), 1.2f,
		[this, W]()
		{
			// Guardar con la pistola en la mano y restaurar: vuelve la pistola con su munición
			MagAtStart = W->GetMagazine();
			const TArray<FBLWeaponSlot> Snap = W->GetInventorySnapshot();
			W->SwitchToSlot(0);
			W->RestoreInventory(Snap, 1);
		},
		nullptr,
		[this, W, IsPistol](FString& D)
		{
			D = FString::Printf(TEXT("restaurado: %d armas, %s %d (guardado %d)"), W->GetWeaponCount(), *GetNameSafe(W->GetWeaponData()), W->GetMagazine(), MagAtStart);
			return W->GetWeaponCount() == 2 && IsPistol() && W->GetMagazine() == MagAtStart && !W->IsEquipping();
		},
		nullptr, 1.0f });
}

// Ajuste visual (no es una prueba de regresión): giros candidatos de la mano izquierda en el puño, vistos de lado
// (pistola girada para enseñar su lado izquierdo) y en ADS. "-BLTest=PistolHands"; capturas PistolHands_<n>_{Lado,ADS}.
void UBLAutoTestComponent::BuildPistolHandsTest()
{
	static const FRotator Candidates[] = {
		FRotator(90.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f), FRotator(0.f, 0.f, 90.f), FRotator(0.f, 0.f, -90.f),
		FRotator(90.f, 0.f, 90.f), FRotator(90.f, 0.f, -90.f), FRotator(0.f, 90.f, 0.f), FRotator(0.f, -90.f, 0.f),
		FRotator(0.f, 180.f, 0.f), FRotator(0.f, 0.f, 180.f) };
	// Afinar alrededor de un giro: "-BLGripRot=P,Y,R" prueba ese y variaciones de ±25° en cada eje
	FString Override;
	TArray<FRotator> List(Candidates, UE_ARRAY_COUNT(Candidates));
	if (FParse::Value(FCommandLine::Get(), TEXT("BLGripRot="), Override))
	{
		TArray<FString> Parts;
		Override.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() == 3)
		{
			const FRotator Base(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
			List = { Base, Base + FRotator(25.f, 0.f, 0.f), Base - FRotator(25.f, 0.f, 0.f), Base + FRotator(0.f, 25.f, 0.f),
				Base - FRotator(0.f, 25.f, 0.f), Base + FRotator(0.f, 0.f, 25.f), Base - FRotator(0.f, 0.f, 25.f) };
		}
	}

	Steps.Add({ TEXT("Equipar"), 1.6f,
		[this]() { TeleportToStart(TEXT("Tiro")); Char()->SwitchWeapon(1); },
		nullptr,
		[this](FString& D) { D = GetNameSafe(Char()->GetWeapon()->GetWeaponData()); return D == TEXT("DA_P17"); } });

	TSharedRef<FBLWeaponPoses> Saved = MakeShared<FBLWeaponPoses>();
	for (int32 i = 0; i < List.Num(); ++i)
	{
		const FRotator R = List[i];
		auto SetGrip = [this, R]()
		{
			UBLFirstPersonAnimInstance* Anim = Char()->GetFirstPersonAnim();
			const FVector Loc = Anim->GetState().LeftGripInWeapon.GetLocation();
			Anim->SetLeftHandGrip(FTransform(R, Loc), true);
		};
		Steps.Add({ FString::Printf(TEXT("%d_Lado"), i), 0.7f,
			[this, SetGrip, Saved]()
			{
				SetGrip();
				*Saved = Char()->GetFirstPersonRig()->Poses;
				Char()->GetFirstPersonRig()->Poses.HipLocation = FVector(4.f, -2.f, -1.f);
				Char()->GetFirstPersonRig()->Poses.HipRotation = FRotator(0.f, -80.f, 0.f);   // enseña el lado izquierdo
			},
			nullptr,
			[R](FString& D) { D = R.ToCompactString(); return true; },
			[this, Saved]() { Char()->GetFirstPersonRig()->Poses = *Saved; }, 0.6f });
		Steps.Add({ FString::Printf(TEXT("%d_ADS"), i), 0.8f,
			[this, SetGrip]() { SetGrip(); Char()->SetAimHeld(true); },
			nullptr,
			[R](FString& D) { D = R.ToCompactString(); return true; },
			[this]() { Char()->SetAimHeld(false); }, 0.7f });
	}
}
