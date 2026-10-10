// Prueba automática "Disparo": balas reales del jugador contra los enemigos de cualquier mapa de misión.
//  Las pruebas de misión matan a los enemigos con TakeDamage directo; esta comprueba el camino completo del juego:
//  apuntar (ADS) → trazado del arma → impacto en la malla → daño → hitmarker. Para cada enemigo vivo y visible
//  coloca al jugador a 7-12 m con línea de vista, apunta al pecho y dispara tiros sueltos. Si la bala no le quita
//  vida, el detalle dice qué componente paró el trazado.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAllyController.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLVarek.h"
#include "Combat/BLDamageTypes.h"
#include "Combat/BLHealthComponent.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectIterator.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	FString DescribeHit(const FHitResult& Hit)
	{
		if (!Hit.bBlockingHit)
		{
			return TEXT("nada");
		}
		const UPrimitiveComponent* C = Hit.GetComponent();
		return FString::Printf(TEXT("%s.%s (%s, perfil %s) hueso %s a %.0f cm"), *GetNameSafe(Hit.GetActor()), *GetNameSafe(C),
			C ? *C->GetClass()->GetName() : TEXT("-"), C ? *C->GetCollisionProfileName().ToString() : TEXT("-"),
			*Hit.BoneName.ToString(), Hit.Distance);
	}

	FVector ChestOf(const ABLEnemyCharacter* E)
	{
		const USkeletalMeshComponent* M = E->GetMesh();
		return M && M->GetBoneIndex(TEXT("spine_03")) != INDEX_NONE ? M->GetBoneLocation(TEXT("spine_03")) : E->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	}
}

void UBLAutoTestComponent::BuildFiringTest()
{
	ABLCharacter* C = Char();
	C->GetHealth()->bInvulnerable = true;
	UBLWeaponComponent* W = C->GetWeapon();

	struct FFireState
	{
		TWeakObjectPtr<ABLEnemyCharacter> Target;
		float HealthBefore = 0.f;
		int32 ShotsBefore = 0;
		FString Trace;
		FString LastShot;
		bool bPlaced = false;
		float TriggerAt = -1.f;
		int32 Tested = 0;
		int32 Hurt = 0;
	};
	TSharedRef<FFireState> St = MakeShared<FFireState>();
	W->OnHitConfirmed.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleHitConfirmed);
	W->OnShot.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleShot);

	// Enemigos candidatos (fijados al construir: los que aún duermen ocultos no cuentan)
	TArray<TWeakObjectPtr<ABLEnemyCharacter>> Targets;
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsA<ABLVarek>() || Cast<ABLAllyController>(It->GetController()) || It->IsDead() || It->IsHidden())
		{
			continue;
		}
		Targets.Add(*It);
		if (Targets.Num() >= 16)
		{
			break;
		}
	}

	// Busca una posición a 7-12 m con suelo y línea de vista limpia hasta el pecho (por el canal del arma)
	auto PlaceFacing = [this](ABLEnemyCharacter* E) -> bool
	{
		ABLCharacter* P = Char();
		const FVector Chest = ChestOf(E);
		const float EyeAboveFeet = P->GetCamera()->GetComponentLocation().Z - (P->GetActorLocation().Z - P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLTestPlace), false, P);
		Q.AddIgnoredActor(E);
		for (const float Dist : { 900.f, 700.f, 500.f, 350.f, 250.f, 1200.f })
		{
			for (int32 i = 0; i < 12; ++i)
			{
				const float Yaw = E->GetActorRotation().Yaw + i * 30.f;
				const FVector Probe = E->GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * Dist;
				FHitResult Floor;
				// Suelo cerca de la altura del enemigo (dentro de un edificio, no el tejado) y en su misma sala
				if (GetWorld()->LineTraceSingleByChannel(Floor, E->GetActorLocation(), Probe, ECC_Visibility, Q)
					|| !GetWorld()->LineTraceSingleByChannel(Floor, Probe + FVector(0.f, 0.f, 60.f), Probe - FVector(0.f, 0.f, 400.f), ECC_Visibility, Q)
					|| Floor.ImpactNormal.Z < 0.7f)
				{
					continue;
				}
				const FVector Eye = Floor.ImpactPoint + FVector(0.f, 0.f, EyeAboveFeet);
				FHitResult Block;
				// Con el mismo trazado que la bala: otro enemigo en medio también tapa
				if (BLDamage::WeaponTrace(GetWorld(), Block, Eye, Chest, Q)
					|| GetWorld()->SweepSingleByChannel(Block, Floor.ImpactPoint + FVector(0.f, 0.f, 96.f), Floor.ImpactPoint + FVector(0.f, 0.f, 97.f),
						FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(34.f, 90.f), Q))
				{
					continue;
				}
				P->GetCharacterMovement()->StopMovementImmediately();
				P->SetActorLocation(Floor.ImpactPoint + FVector(0.f, 0.f, P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f),
					false, nullptr, ETeleportType::TeleportPhysics);
				P->GetController()->SetControlRotation((Chest - Eye).Rotation());
				return true;
			}
		}
		return false;
	};

	// Censo: primitivas que paran balas sin tapar la vista (canal Weapon en Block, Visibility no): sospechosas de
	// comerse los disparos (volúmenes, triggers, colisiones invisibles)
	Steps.Add({ TEXT("Censo"), 0.2f, nullptr, nullptr,
		[this](FString& D)
		{
			TMap<FString, int32> Count;
			for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
			{
				UPrimitiveComponent* P = *It;
				if (P->GetWorld() != GetWorld() || !P->IsRegistered() || !P->IsQueryCollisionEnabled() || Cast<ACharacter>(P->GetOwner()))
				{
					continue;
				}
				if (P->GetCollisionResponseToChannel(ECC_BLWeapon) == ECR_Block && P->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
				{
					const bool bSkipped = (P->IsA<UShapeComponent>()) || (!P->IsA<USkeletalMeshComponent>() && (!P->IsVisible() || P->bHiddenInGame));
					++Count.FindOrAdd(FString::Printf(TEXT("%s/%s/%s%s"), *P->GetOwner()->GetClass()->GetName(), *P->GetClass()->GetName(),
						*P->GetCollisionProfileName().ToString(), bSkipped ? TEXT(" (WeaponTrace la atraviesa)") : TEXT(" (¡PARA BALAS!)")));
				}
			}
			for (const auto& KV : Count)
			{
				D += FString::Printf(TEXT("%s x%d; "), *KV.Key, KV.Value);
			}
			UE_LOG(LogBlackline, Display, TEXT("[BLTest] Censo: %s"), *D);
			return !D.Contains(TEXT("PARA BALAS"));
		} });

	Steps.Add({ TEXT("Enemigos"), 0.5f, nullptr, nullptr,
		[Targets, W](FString& D)
		{
			D = FString::Printf(TEXT("%d enemigos vivos y visibles a prueba, arma %s"), Targets.Num(), *GetNameSafe(W->GetWeaponData()));
			return Targets.Num() > 0 && W->GetWeaponData();
		} });

	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const TWeakObjectPtr<ABLEnemyCharacter> Target = Targets[Index];
		Steps.Add({ FString::Printf(TEXT("Bala_%d"), Index + 1), 2.5f,
			[this, St, Target, PlaceFacing, W]()
			{
				St->Target = Target;
				HitConfirms = 0;
				HitConfirmDamage = 0.f;
				St->Trace.Empty();
				St->TriggerAt = -1.f;
				// El blanco se queda quieto durante su paso (si no, puede meterse tras una cobertura al patrullar)
				if (Target.IsValid())
				{
					Target->GetCharacterMovement()->StopMovementImmediately();
					Target->GetCharacterMovement()->SetMovementMode(MOVE_None);
				}
				St->bPlaced = Target.IsValid() && !Target->IsDead() && PlaceFacing(Target.Get());
				St->HealthBefore = Target.IsValid() ? Target->GetHealth()->GetHealth() : 0.f;
				St->ShotsBefore = W->GetShotsFired();
				W->AddReserveAmmo(W->FindWeapon(W->GetWeaponData()), 999);
				Char()->SetAimHeld(true);
			},
			[this, St, W](float T)
			{
				ABLEnemyCharacter* E = St->Target.Get();
				if (!E || !St->bPlaced)
				{
					return;
				}
				// Sigue el pecho (el enemigo se mueve y reacciona); dispara al estar en ADS, tiro a tiro
				const FVector Eye = Char()->GetCamera()->GetComponentLocation();
				Char()->GetController()->SetControlRotation((ChestOf(E) - Eye).Rotation());
				const bool bAds = Char()->GetAimAlpha() > 0.95f;
				if (bAds && St->Trace.IsEmpty())
				{
					FVector Origin, Dir;
					Char()->GetWeaponAimView(Origin, Dir);
					FCollisionQueryParams Q(SCENE_QUERY_STAT(BLTestTrace), true, Char());
					FHitResult Hit;
					BLDamage::WeaponTrace(GetWorld(), Hit, Origin, Origin + Dir * 20000.f, Q);
					St->Trace = DescribeHit(Hit);
				}
				const int32 Fired = W->GetShotsFired() - St->ShotsBefore;
				const bool bHurt = E->GetHealth()->GetHealth() < St->HealthBefore;
				if (bAds && !bHurt && Fired < 3 && T - St->TriggerAt > 0.35f)
				{
					W->SetTriggerHeld(true);
					St->TriggerAt = T;
				}
				else if (T - St->TriggerAt > 0.04f)
				{
					W->SetTriggerHeld(false);
				}
			},
			[this, St, W](FString& D)
			{
				W->SetTriggerHeld(false);
				ABLEnemyCharacter* E = St->Target.Get();
				if (!St->bPlaced)
				{
					D = FString::Printf(TEXT("%s: sin posición con línea de vista (no cuenta)"), *GetNameSafe(E));
					return true;
				}
				const float Lost = E ? St->HealthBefore - E->GetHealth()->GetHealth() : 0.f;
				++St->Tested;
				St->Hurt += Lost > 0.f ? 1 : 0;
				D = FString::Printf(TEXT("%s: vida %.0f -> %.0f, disparos %d, hitmarkers %d (%.0f), trazado previo: %s; último impacto: %s"),
					*GetNameSafe(E), St->HealthBefore, E ? E->GetHealth()->GetHealth() : 0.f, W->GetShotsFired() - St->ShotsBefore,
					HitConfirms, HitConfirmDamage, *St->Trace, *LastHitInfo);
				return Lost > 0.f && HitConfirms > 0;
			},
			[this, St]()
			{
				Char()->SetAimHeld(false);
				if (ABLEnemyCharacter* E = St->Target.Get(); E && !E->IsDead())
				{
					E->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				}
			} });
	}

	Steps.Add({ TEXT("Total"), 0.2f, nullptr, nullptr,
		[St](FString& D)
		{
			D = FString::Printf(TEXT("%d de %d enemigos heridos con balas reales"), St->Hurt, St->Tested);
			return St->Tested > 0 && St->Hurt == St->Tested;
		} });
}
