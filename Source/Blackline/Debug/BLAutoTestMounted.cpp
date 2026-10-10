// Prueba automática "MountedGun" (mapa L_M04_FuegoCruzado): la ametralladora del búnker.
//  Montar (F real) → los ojos bajan a la línea de mira → el cañón apunta al punto de la mira (dos inclinaciones) →
//  precisión (ráfagas, compensando el retroceso como un jugador) → bajarse con el gatillo apretado: ni la ametralladora
//  ni el fusil disparan → soltar → el fusil dispara normal → tres ciclos rápidos de montar y bajar sin disparos fantasma.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLInteractable.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLMountedGun.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildMountedGunTest()
{
	UBLWeaponComponent* W = Char()->GetWeapon();
	Char()->GetHealth()->bInvulnerable = true;   // los morteros y la defensa siguen activos alrededor

	auto Gun = [this]() -> ABLMountedGun* { for (TActorIterator<ABLMountedGun> It(GetWorld()); It; ++It) { return *It; } return nullptr; };
	struct FState
	{
		int32 F = 0;
		int32 RifleShots0 = 0;
		int32 GunShots0 = 0;
		int32 GunShotsAtDismount = 0;
		float SumErr = 0.f;
		int32 Samples = 0;
		FVector LastSeen = FVector::ZeroVector;
		float AimDist = 0.f;
		FRotator Aim = FRotator::ZeroRotator;
		int32 Cycles = 0;
		float MaxAlignErr = 0.f;
	};
	TSharedRef<FState> St = MakeShared<FState>();

	// Detrás del arma mirando a su punto de uso (como el jugador antes de pulsar F)
	auto Approach = [this, Gun]()
	{
		ABLMountedGun* G = Gun();
		if (!G)
		{
			return;
		}
		G->bEnabled = true;
		ABLCharacter* C = Char();
		const FVector From = G->GetSeatLocation() - G->GetActorForwardVector() * 30.f;
		C->GetCharacterMovement()->StopMovementImmediately();
		C->SetActorLocation(From + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	};
	auto LookAtGun = [this, Gun]()
	{
		if (const ABLMountedGun* G = Gun(); G && Char()->GetController())
		{
			Char()->GetController()->SetControlRotation((G->GetInteractLocation() - Char()->GetCamera()->GetComponentLocation()).Rotation());
		}
	};
	// Una pulsación de F (flanco de subida a 0,2 s, se suelta a 0,4 s)
	auto PressF = [this](TSharedRef<FState> S, float T)
	{
		if (T > 0.2f && S->F == 0) { Char()->SetInteractHeld(true); S->F = 1; }
		else if (T > 0.4f && S->F == 1) { Char()->SetInteractHeld(false); S->F = 2; }
	};
	auto AimRel = [this, Gun](float Pitch, float Yaw)
	{
		if (const ABLMountedGun* G = Gun(); G && Char()->GetController())
		{
			Char()->GetController()->SetControlRotation(FRotator(Pitch, G->GetBaseYaw() + Yaw, 0.f));
		}
	};

	Steps.Add({ TEXT("Montar"), 2.0f, [Approach, St]() { Approach(); St->F = 0; },
		[LookAtGun, PressF, St](float T) { if (!St->F) { LookAtGun(); } PressF(St, T); },
		[this, Gun](FString& D)
		{
			const ABLMountedGun* G = Gun();
			const float Eye = Char()->GetCamera()->GetComponentLocation().Z - (G ? G->GetActorLocation().Z : 0.f);
			D = FString::Printf(TEXT("montado=%d, ojos a %.0f cm del suelo (línea de mira %.0f)"), Char()->IsMounted(), Eye, G ? G->GetSightHeight() : 0.f);
			return G && Char()->IsMounted() && FMath::Abs(Eye - G->GetSightHeight()) < 8.f;
		}, [this]() { Char()->SetInteractHeld(false); } });

	// El cañón apunta al punto de la mira (ángulo entre el cañón y la recta boca → punto apuntado)
	for (const float Pitch : { 0.f, -5.f, 6.f })
	{
		Steps.Add({ FString::Printf(TEXT("Alineacion%+.0f"), Pitch), 0.8f, [AimRel, Pitch]() { AimRel(Pitch, 12.f); }, [AimRel, Pitch](float) { AimRel(Pitch, 12.f); },
			[Gun, St](FString& D)
			{
				const ABLMountedGun* G = Gun();
				if (!G)
				{
					return false;
				}
				const FVector ToAim = (G->GetAimPoint() - G->GetMuzzleLocation()).GetSafeNormal();
				const float Err = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToAim, G->GetBarrelDirection()), -1.f, 1.f)));
				St->MaxAlignErr = FMath::Max(St->MaxAlignErr, Err);
				D = FString::Printf(TEXT("cañón a %.2f° de la recta boca-punto apuntado (punto a %.0f m)"), Err, FVector::Dist(G->GetAimPoint(), G->GetMuzzleLocation()) / 100.f);
				return Err < 1.0f;
			} });
	}

	// Precisión: ráfagas largas a un punto fijo, recolocando la mira tras cada retroceso (lo que hace el jugador)
	Steps.Add({ TEXT("Precision"), 2.5f,
		[this, AimRel, St, Gun]() { AimRel(-1.5f, 0.f); St->SumErr = 0.f; St->Samples = 0; St->LastSeen = Gun() ? Gun()->GetLastImpact() : FVector::ZeroVector; St->Aim = Char()->GetControlRotation(); },
		[this, Gun, St](float T)
		{
			ABLMountedGun* G = Gun();
			Char()->GetController()->SetControlRotation(St->Aim);
			Char()->SetFireHeld(T > 0.3f && T < 2.2f);
			if (G && G->GetLastImpact() != St->LastSeen && !G->GetLastImpact().IsZero())
			{
				St->LastSeen = G->GetLastImpact();
				const FVector Muzzle = G->GetMuzzleLocation();
				const float Dist = FVector::Dist(G->GetAimPoint(), Muzzle);
				const FVector A = (G->GetAimPoint() - Muzzle).GetSafeNormal();
				const FVector B = (G->GetLastImpact() - Muzzle).GetSafeNormal();
				St->SumErr += FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B), -1.f, 1.f)));
				++St->Samples;
				St->AimDist = Dist;
			}
		},
		[St](FString& D)
		{
			const float Mean = St->Samples > 0 ? St->SumErr / St->Samples : 99.f;
			D = FString::Printf(TEXT("%d impactos a ~%.0f m, error medio %.2f° (%.0f cm)"), St->Samples, St->AimDist / 100.f, Mean,
				FMath::Tan(FMath::DegreesToRadians(Mean)) * St->AimDist);
			return St->Samples >= 8 && Mean < 0.8f;
		}, [this]() { Char()->SetFireHeld(false); } });

	// Bajarse con el gatillo apretado: la ametralladora para y el fusil no hereda la pulsación
	Steps.Add({ TEXT("BajarDisparando"), 2.5f,
		[this, W, Gun, St]() { St->F = 0; St->RifleShots0 = W->GetShotsFired(); St->GunShots0 = Gun() ? Gun()->GetShotsFired() : 0; Char()->SetFireHeld(true); },
		[this, PressF, St, Gun](float T)
		{
			if (T > 0.8f) { PressF(St, T - 0.8f); }
			if (St->F == 1 && !Char()->IsMounted()) { St->GunShotsAtDismount = Gun() ? Gun()->GetShotsFired() : 0; }
		},
		[this, W, Gun, St](FString& D)
		{
			const int32 Rifle = W->GetShotsFired() - St->RifleShots0;
			const int32 GunNow = Gun() ? Gun()->GetShotsFired() : 0;
			D = FString::Printf(TEXT("ametralladora %d disparos montado y %d después de bajarse; fusil %d (gatillo aún apretado), montado=%d"),
				St->GunShotsAtDismount - St->GunShots0, GunNow - St->GunShotsAtDismount, Rifle, Char()->IsMounted());
			return !Char()->IsMounted() && St->GunShotsAtDismount > St->GunShots0 && GunNow - St->GunShotsAtDismount <= 1 && Rifle == 0;
		}, [this]() { Char()->SetInteractHeld(false); } });
	Steps.Add({ TEXT("Soltar"), 1.0f, [this, W, St]() { Char()->SetFireHeld(false); St->RifleShots0 = W->GetShotsFired(); }, nullptr,
		[this, W, St](FString& D)
		{
			const int32 Rifle = W->GetShotsFired() - St->RifleShots0;
			D = FString::Printf(TEXT("fusil %d disparos con el gatillo suelto, en la mano %s, apretar bloqueado=%d"), Rifle, *GetNameSafe(W->GetWeaponData()), Char()->IsFireBlockedUntilRelease());
			return Rifle == 0 && !Char()->IsFireBlockedUntilRelease() && Char()->GetWeaponMesh()->IsVisible();
		} });
	Steps.Add({ TEXT("FusilNormal"), 0.8f, [this, W, St]() { St->RifleShots0 = W->GetShotsFired(); Char()->SetFireHeld(true); }, nullptr,
		[this, W, St](FString& D)
		{
			const int32 Rifle = W->GetShotsFired() - St->RifleShots0;
			D = FString::Printf(TEXT("fusil %d disparos en 0,8 s al apretar de nuevo"), Rifle);
			return Rifle >= 4;
		}, [this]() { Char()->SetFireHeld(false); } });

	// Tres ciclos rápidos de montar y bajar (con disparos en medio): ningún disparo fantasma después
	for (int32 Cycle = 0; Cycle < 3; ++Cycle)
	{
		Steps.Add({ FString::Printf(TEXT("Ciclo%d"), Cycle + 1), 2.6f,
			[Approach, St]() { Approach(); St->F = 0; },
			[this, LookAtGun, PressF, St](float T)
			{
				if (T < 0.6f)
				{
					if (!St->F) { LookAtGun(); }
					PressF(St, T);
					return;
				}
				if (T < 0.65f) { St->F = 0; }
				Char()->SetFireHeld(T > 0.8f && T < 1.4f && Char()->IsMounted());
				if (T > 1.6f) { PressF(St, T - 1.6f); }
			},
			[this](FString& D) { D = FString::Printf(TEXT("montado=%d"), Char()->IsMounted()); return !Char()->IsMounted(); },
			[this]() { Char()->SetFireHeld(false); Char()->SetInteractHeld(false); } });
	}
	Steps.Add({ TEXT("SinFantasmas"), 1.5f, [this, W, Gun, St]() { St->RifleShots0 = W->GetShotsFired(); St->GunShots0 = Gun() ? Gun()->GetShotsFired() : 0; }, nullptr,
		[this, W, Gun, St](FString& D)
		{
			const int32 Rifle = W->GetShotsFired() - St->RifleShots0;
			const int32 G = (Gun() ? Gun()->GetShotsFired() : 0) - St->GunShots0;
			D = FString::Printf(TEXT("tras 3 ciclos: fusil %d y ametralladora %d disparos sin tocar nada; desalineación máx %.2f°"), Rifle, G, St->MaxAlignErr);
			return Rifle == 0 && G == 0 && !Char()->IsMounted();
		} });
}
