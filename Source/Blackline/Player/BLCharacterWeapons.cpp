// Cambio de arma, recarga procedural de la pistola P-17 y de la escopeta SG-12 (el resto de la recarga vive en
// BLCharacter.cpp).
#include "Player/BLCharacter.h"

#include "Blackline.h"
#include "Animation/BLFirstPersonAnimInstance.h"
#include "Animation/BLWeaponAnimInstance.h"
#include "Player/BLFirstPersonRigComponent.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLWeaponFXComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

bool ABLCharacter::CanSwitchWeapon() const
{
	return Weapon && !bDead && !IsMounted() && GrenadeTimer < 0.f && Weapon->GetWeaponCount() > 1;
}

void ABLCharacter::SwitchWeapon(int32 Index)
{
	if (CanSwitchWeapon() && Weapon->SwitchToSlot(Index))
	{
		PlayGear(0.4f);
	}
}

void ABLCharacter::CycleWeapon()
{
	if (CanSwitchWeapon() && Weapon->CycleWeapon())
	{
		PlayGear(0.4f);
	}
}

void ABLCharacter::TakeNewMagazine()
{
	const UBLWeaponData* Data = Weapon ? Weapon->GetWeaponData() : nullptr;
	const UBLFirstPersonAnimInstance* Anim = GetFirstPersonAnim();
	if (!Data || !Anim || !Data->MagazineMesh || WeaponMesh->GetBoneIndex(Data->MagazineBone) == INDEX_NONE)
	{
		return;
	}
	SetMagazineInHand(true);
	if (!HandMagazine)
	{
		return;
	}
	// En la mano tal y como quedará cuando la mano (su socket HandGrip_L) llegue a la base del puño: así entra
	// alineado sin saltos. Cargador en espacio del arma: ejes del arma, origen en el hueso "magazine".
	const FVector MagBone = WeaponMesh->GetSocketTransform(Data->MagazineBone, RTS_Component).GetLocation();
	const FVector MagPoint = WeaponMesh->DoesSocketExist(Data->MagSocket)
		? WeaponMesh->GetSocketTransform(Data->MagSocket, RTS_Component).GetLocation() : Data->MagLocalOffset;
	const FTransform HandSocketInWeapon(Anim->GetState().LeftGripInWeapon.GetRotation(), MagPoint);
	HandMagazine->SetRelativeTransform(FTransform(MagBone) * HandSocketInWeapon.Inverse() * Anim->GetState().HandGripLLocal);
}

void ABLCharacter::UpdatePistolReload(const UBLWeaponData* Data, float P, TArray<TPair<float, FVector>, TInlineAllocator<16>>& Keys, FVector& DynLoc, FRotator& DynRot)
{
	const UBLFirstPersonAnimInstance* Anim = GetFirstPersonAnim();
	const USkeletalMeshComponent* WMesh = WeaponMesh;
	auto SocketOr = [WMesh](FName Socket, const FVector& Fallback)
	{
		return WMesh->DoesSocketExist(Socket) ? WMesh->GetSocketTransform(Socket, RTS_Component).GetLocation() : Fallback;
	};
	const bool bEmpty = Weapon->IsReloadEmpty();
	const float In = Data->ReloadAmmoInsertTime;
	const FVector Grip = Anim->GetState().LeftGripInWeapon.GetLocation();
	const FVector Mag = SocketOr(Data->MagSocket, Data->MagLocalOffset);
	const FVector MagTop = SocketOr(Data->MagazineBone, Mag + FVector(0.f, 3.f, 11.f));
	const FVector MagAxis = (Mag - MagTop).GetSafeNormal();                 // hacia fuera del puño
	const FVector Off = Grip + FVector(3.5f, -1.f, -5.f);                  // la mano se aparta: el cargador cae solo
	const FVector Pouch = Mag + FVector(10.f, -8.f, -30.f);                // portacargadores (fuera de cámara)
	const FVector Rear = SocketOr(Data->ChargingHandleSocket, Data->ChargingHandleLocalOffset);  // estrías traseras

	// Línea de tiempo (fracción): suelta, coge otro del cinturón, lo mete con un golpe; en vacío monta la corredera
	Keys.Add({ 0.00f, Grip });
	Keys.Add({ 0.08f, Off });
	Keys.Add({ 0.30f, Pouch });
	Keys.Add({ 0.37f, Pouch + FVector(0.f, 3.f, 2.f) });
	Keys.Add({ In - 0.10f, Mag + MagAxis * 8.f + FVector(2.f, 0.f, 0.f) });  // bajo el puño, alineado con su eje
	Keys.Add({ In - 0.03f, Mag + MagAxis * 3.f });
	Keys.Add({ In, Mag });
	Keys.Add({ In + 0.04f, Mag - MagAxis * 1.f });                         // golpe con la palma para asentarlo
	if (bEmpty)
	{
		Keys.Add({ In + 0.11f, Rear + FVector(1.5f, 0.f, 4.f) });          // por encima de la corredera
		Keys.Add({ In + 0.16f, Rear });                                      // la agarra
		Keys.Add({ In + 0.22f, Rear + FVector(0.f, -4.5f, 0.f) });          // tira atrás y la suelta
		Keys.Add({ In + 0.28f, Rear + FVector(3.f, -7.f, -2.f) });
		Keys.Add({ 0.90f, Grip + FVector(2.f, -2.f, -3.f) });
		Keys.Add({ 0.97f, Grip });
	}
	else
	{
		Keys.Add({ 0.74f, Grip + FVector(2.f, -2.f, -3.f) });
		Keys.Add({ 0.86f, Grip });
	}
	Keys.Add({ 1.01f, Grip });

	// Movimiento del arma ligado a la mano (espacio de cámara)
	auto Bell = [P](float A, float B) { return P > A && P < B ? FMath::Sin(PI * (P - A) / (B - A)) : 0.f; };
	DynRot.Roll += 8.f * Bell(0.03f, 0.2f);                  // se inclina para que caiga el cargador
	DynRot.Pitch += 5.f * Bell(In - 0.12f, In + 0.04f);      // hacia la mano que mete el nuevo
	if (bEmpty)
	{
		DynLoc.X += 2.5f * Bell(In + 0.13f, In + 0.26f);     // la derecha empuja adelante mientras la izquierda tira
		DynRot.Yaw -= 4.f * Bell(In + 0.1f, In + 0.26f);
	}

	auto Crossed = [this, P](float T) { return LastReloadProgress < T && P >= T; };
	if (Crossed(0.07f))
	{
		// Botón del cargador: cae por su eje (más la inercia del jugador)
		WeaponMesh->HideBoneByName(Data->MagazineBone, PBO_None);
		if (UBLWeaponFXComponent* WFX = FindComponentByClass<UBLWeaponFXComponent>())
		{
			const FVector Down = WeaponMesh->GetComponentTransform().TransformVectorNoScale(MagAxis);
			WFX->DropMagazine(Data, Down * 160.f + GetVelocity());
		}
		FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, 10.f), FRotator(4.f, 0.f, 0.f));
	}
	if (Crossed(0.28f) || Crossed(0.8f))
	{
		if (UBLWeaponFXComponent* WFX = FindComponentByClass<UBLWeaponFXComponent>())
		{
			WFX->PlayHandling(Data, 0.7f);
		}
	}
	if (Crossed(0.36f))
	{
		TakeNewMagazine();
	}
	if (Crossed(In))
	{
		SetMagazineInHand(false);
		FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, 40.f), FRotator(14.f, 0.f, -4.f));
		FirstPersonRig->AddCameraKick(FRotator(4.f, 0.f, 1.f));
	}
	if (Crossed(In + 0.04f))
	{
		FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, 18.f), FRotator(6.f, 0.f, 0.f));
	}
	if (bEmpty && Crossed(Data->BoltReleaseTime))
	{
		// La corredera vuelve a su sitio de golpe
		FirstPersonRig->AddWeaponKick(FVector(-30.f, 0.f, 5.f), FRotator(5.f, 3.f, 2.f));
		FirstPersonRig->AddCameraKick(FRotator(3.f, 0.f, -2.f));
	}
}

// ---------------------------------------------------------------------------
// Recarga de escopeta y mano en el guardamanos
// ---------------------------------------------------------------------------

FVector ABLCharacter::EvalHandPath(const FHandKeys& Keys, float P, const FVector& Default)
{
	for (int32 k = 0; k + 1 < Keys.Num(); ++k)
	{
		if (P >= Keys[k].Key && P < Keys[k + 1].Key)
		{
			const float A = (P - Keys[k].Key) / (Keys[k + 1].Key - Keys[k].Key);
			const FVector& P0 = Keys[FMath::Max(k - 1, 0)].Value;
			const FVector& P1 = Keys[k].Value;
			const FVector& P2 = Keys[k + 1].Value;
			const FVector& P3 = Keys[FMath::Min(k + 2, Keys.Num() - 1)].Value;
			const float A2 = A * A, A3 = A2 * A;
			return 0.5f * ((2.f * P1) + (-P0 + P2) * A + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * A2 + (-P0 + 3.f * P1 - 3.f * P2 + P3) * A3);
		}
	}
	return Default;
}

UStaticMeshComponent* ABLCharacter::EnsureHandMagazine()
{
	if (!HandMagazine)
	{
		HandMagazine = NewObject<UStaticMeshComponent>(this, TEXT("HandMagazine"));
		HandMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HandMagazine->SetCastShadow(false);
		HandMagazine->SetOnlyOwnerSee(true);
		HandMagazine->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		HandMagazine->RegisterComponent();
	}
	return HandMagazine;
}

FVector ABLCharacter::GetWeaponBoltOffset() const
{
	const UBLWeaponAnimInstance* WAnim = WeaponMesh ? Cast<UBLWeaponAnimInstance>(WeaponMesh->GetAnimInstance()) : nullptr;
	return WAnim ? WAnim->GetMechState().BoltOffset : FVector::ZeroVector;
}

void ABLCharacter::SetShellInHand(const UBLWeaponData* Data, bool bVisible, const FVector& LocationInWeapon)
{
	if (!bVisible || !Data || !Data->MagazineMesh)
	{
		if (HandMagazine && HandMagazine->GetAttachParent() == WeaponMesh)
		{
			HandMagazine->SetVisibility(false);
		}
		return;
	}
	UStaticMeshComponent* Shell = EnsureHandMagazine();
	if (Shell->GetStaticMesh() != Data->MagazineMesh)
	{
		Shell->SetStaticMesh(Data->MagazineMesh);
	}
	if (Shell->GetAttachParent() != WeaponMesh)
	{
		Shell->AttachToComponent(WeaponMesh, FAttachmentTransformRules::KeepRelativeTransform);
	}
	// El cartucho (eje +X, origen en el culote) apunta hacia delante, como entra en el depósito
	Shell->SetRelativeLocationAndRotation(LocationInWeapon, FRotator(0.f, 90.f, 0.f));
	Shell->SetVisibility(true);
}

void ABLCharacter::UpdateShellReload(const UBLWeaponData* Data, float DeltaTime, float& ReloadPose, FVector& HandTarget, FVector& DynLoc, FRotator& DynRot)
{
	const UBLFirstPersonAnimInstance* Anim = GetFirstPersonAnim();
	float A = 0.f;
	int32 ShellIndex = INDEX_NONE;
	const EBLShellReloadPhase Phase = Weapon->GetShellReloadPhase(A, ShellIndex);
	if (!Anim || Phase == EBLShellReloadPhase::None)
	{
		return;
	}

	// Puntos en espacio del arma: portilla de carga (abajo), ventana de expulsión (derecha), cartuchera del cinturón
	const USkeletalMeshComponent* WMesh = WeaponMesh;
	auto SocketOr = [WMesh](FName Socket, const FVector& Fallback)
	{
		return WMesh->DoesSocketExist(Socket) ? WMesh->GetSocketTransform(Socket, RTS_Component).GetLocation() : Fallback;
	};
	const FVector Grip = Anim->GetState().LeftGripInWeapon.GetLocation();
	const FVector Port = SocketOr(Data->MagSocket, Data->MagLocalOffset);
	const FVector EjectPort = SocketOr(Data->EjectSocket, Data->EjectLocalOffset);
	const FVector Below = Port + FVector(0.f, -4.f, -7.f);      // bajo la portilla, con el cartucho alineado
	const FVector Push = Port + FVector(0.f, 2.5f, -2.f);        // el pulgar lo empuja dentro del depósito
	const FVector Belt = Port + FVector(9.f, -10.f, -32.f);      // cartuchera (fuera de cámara)
	const FVector OverPort = EjectPort + FVector(-4.f, -1.f, 4.f);
	const FVector ShellInFingers(0.f, -1.5f, 3.f);               // culote del cartucho respecto a la palma
	const float In = Data->ReloadAmmoInsertTime;

	FHandKeys Keys;
	float Pos = 0.f;            // posición en la línea de tiempo (para los golpes)
	bool bShellVisible = false;
	switch (Phase)
	{
	case EBLShellReloadPhase::Raise:
		Pos = A;
		ReloadPose = FMath::SmoothStep(0.f, 1.f, A);
		Keys.Add({ 0.f, Grip });
		Keys.Add({ 1.01f, Below });
		break;
	case EBLShellReloadPhase::PortLoad:
	{
		// En vacío: cartucho por la ventana con el arma girada y se cierra la corredera
		Pos = 1.f + A;
		ReloadPose = 1.f - 0.6f * FMath::SmoothStep(0.f, 0.25f, A) * (1.f - FMath::SmoothStep(0.75f, 1.f, A));
		const FVector PumpGrip = Grip + GetWeaponBoltOffset();
		Keys.Add({ 0.00f, Below });
		Keys.Add({ 0.18f, Belt });
		Keys.Add({ 0.26f, Belt + FVector(0.f, 2.f, 3.f) });
		Keys.Add({ 0.40f, OverPort });                           // lo deja caer en la recámara
		Keys.Add({ 0.55f, PumpGrip + FVector(0.f, 0.f, -3.f) });
		Keys.Add({ 0.62f, PumpGrip });                           // agarra el guardamanos (atrás)
		Keys.Add({ 0.72f, Grip });                               // lo cierra de golpe
		Keys.Add({ 0.85f, Grip });
		Keys.Add({ 1.01f, Below });
		bShellVisible = A > 0.22f && A < 0.4f;
		auto Bell = [A](float S, float E) { return A > S && A < E ? FMath::Sin(PI * (A - S) / (E - S)) : 0.f; };
		DynRot.Roll -= 55.f * Bell(0.05f, 0.75f);                // la ventana de expulsión mira arriba
		DynRot.Yaw -= 8.f * Bell(0.05f, 0.75f);
		break;
	}
	case EBLShellReloadPhase::Shell:
		Pos = 2.f + ShellIndex + A;
		ReloadPose = 1.f;
		Keys.Add({ 0.00f, Below });
		Keys.Add({ 0.22f, Belt });
		Keys.Add({ 0.32f, Belt + FVector(0.f, 2.f, 3.f) });      // coge un cartucho
		Keys.Add({ In - 0.17f, Below });
		Keys.Add({ In, Push });                                  // lo empuja dentro
		Keys.Add({ In + 0.12f, Below + FVector(0.f, 1.f, 1.f) });
		Keys.Add({ 1.01f, Below });
		bShellVisible = A > 0.3f && A < In;
		break;
	case EBLShellReloadPhase::Lower:
	default:
		Pos = 100.f + A;
		ReloadPose = 1.f - FMath::SmoothStep(0.f, 1.f, A);
		Keys.Add({ 0.f, Below });
		Keys.Add({ 1.01f, Grip });
		break;
	}
	const FVector Target = EvalHandPath(Keys, A, Grip);
	// Suavizado: al interrumpir la recarga la mano no salta a la siguiente fase
	ShellHand = LastShellPos <= 0.f ? Target : FMath::VInterpTo(ShellHand, Target, DeltaTime, 28.f);
	HandTarget = ShellHand;
	SetShellInHand(Data, bShellVisible, ShellHand + ShellInFingers);

	auto Crossed = [this, Pos](float T) { return LastShellPos < T && Pos >= T; };
	UBLWeaponFXComponent* WFX = FindComponentByClass<UBLWeaponFXComponent>();
	if (Phase == EBLShellReloadPhase::Shell)
	{
		const float Base = 2.f + ShellIndex;
		if (Crossed(Base + 0.22f) && WFX)
		{
			WFX->PlayHandling(Data, 0.45f);  // la mano en la cartuchera
		}
		if (Crossed(Base + In))
		{
			FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, 12.f), FRotator(3.f, 0.f, -2.f));
			FirstPersonRig->AddCameraKick(FRotator(0.6f, 0.f, 0.f));
		}
	}
	if (Crossed(1.4f))
	{
		FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, -8.f), FRotator(-2.f, 0.f, 0.f));
	}
	if (Crossed(1.72f))
	{
		// La corredera se cierra de golpe
		FirstPersonRig->AddWeaponKick(FVector(-30.f, 0.f, 6.f), FRotator(5.f, 3.f, 2.f));
		FirstPersonRig->AddCameraKick(FRotator(3.f, 0.f, -2.f));
	}
	LastShellPos = Pos;
}
