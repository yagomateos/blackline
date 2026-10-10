// Prueba automática "Distancias" (mapa L_M04_FuegoCruzado): balas reales del jugador contra enemigos a 20, 50, 100 y
// 140 m sobre el tablero del puente. Enemigos sin IA (quietos o desplazándose de lado a 2 m/s), apuntando con la mira
// (ADS) a la cabeza o al pecho y disparando tiro a tiro como un jugador. Comprueba:
//  - un impacto confirmado en la cabeza (AR-7 y P-17) mata en el acto a cualquier distancia;
//  - al torso, cuántos impactos hacen falta (el daño no se pierde con la distancia más allá de lo razonable);
//  - que los impactos en el cuerpo se registran (hitmarker = daño real).
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLDamageTypes.h"
#include "Combat/BLHealthComponent.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildRangeTest()
{
	UBLWeaponComponent* W = Char()->GetWeapon();
	Char()->GetHealth()->bInvulnerable = true;
	W->OnHitConfirmed.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleRangeHit);
	W->bInfiniteReserve = true;

	// Fuera de la prueba: los enemigos y oleadas de la misión no deben tapar ni disparar
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	struct FState
	{
		TWeakObjectPtr<ABLEnemyCharacter> Target;
		FVector Base = FVector::ZeroVector;
		int32 Shots0 = 0;
		float NextShot = 0.f;
		int32 HitsAtKill = 0;
		bool bDead = false;
		float HealthBefore = 0.f;
	};
	TSharedRef<FState> St = MakeShared<FState>();
	const FVector PlayerSpot(-100.f, 0.f, 0.f);

	auto FloorAt = [this](const FVector& XY) -> FVector
	{
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(BLRangeFloor), false);
		if (GetWorld()->LineTraceSingleByChannel(Hit, XY + FVector(0.f, 0.f, 600.f), XY - FVector(0.f, 0.f, 1500.f), ECC_Visibility, Q))
		{
			return Hit.ImpactPoint;
		}
		return XY;
	};

	auto AddCase = [this, W, St, PlayerSpot, FloorAt](const FString& WeaponName, float Meters, bool bHead, bool bMoving)
	{
		const FString Name = FString::Printf(TEXT("%s_%s_%.0fm%s"), *WeaponName, bHead ? TEXT("Cabeza") : TEXT("Torso"), Meters, bMoving ? TEXT("_Movil") : TEXT(""));
		Steps.Add({ Name, 14.f,
			[this, W, St, PlayerSpot, FloorAt, WeaponName, Meters]()
			{
				// Arma: AR-7 en la ranura 0, P-17 en la 1
				const int32 Slot = WeaponName == TEXT("P17") ? 1 : 0;
				if (W->GetCurrentIndex() != Slot)
				{
					W->SwitchToSlot(Slot);
				}
				ABLCharacter* C = Char();
				C->GetCharacterMovement()->StopMovementImmediately();
				const FVector Feet = FloorAt(PlayerSpot);
				C->SetActorLocation(Feet + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
				C->SetAimHeld(true);
				// Enemigo sin IA (no dispara ni se mueve solo), de frente al jugador
				const FVector At = FloorAt(PlayerSpot + FVector(Meters * 100.f, 0.f, 0.f)) + FVector(0.f, 0.f, 96.f);
				FActorSpawnParameters P;
				P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				P.bDeferConstruction = true;
				ABLEnemyCharacter* E = GetWorld()->SpawnActor<ABLEnemyCharacter>(ABLEnemyCharacter::StaticClass(), At, FRotator(0.f, 180.f, 0.f), P);
				if (E)
				{
					E->AutoPossessAI = EAutoPossessAI::Disabled;
					E->FinishSpawning(FTransform(FRotator(0.f, 180.f, 0.f), At));
				}
				St->Target = E;
				St->Base = At;
				St->Shots0 = W->GetShotsFired();
				St->NextShot = 1.2f;   // equipar y apuntar
				St->HitsAtKill = 0;
				St->bDead = false;
				St->HealthBefore = E ? E->GetHealth()->GetHealth() : 0.f;
				RangeHits = 0;
				RangeHeadHits = 0;
				RangeKillZone = EBLHitZone::Torso;
				RangeKillHit = -1;
			},
			[this, W, St, bHead, bMoving](float T)
			{
				ABLEnemyCharacter* E = St->Target.Get();
				if (!E)
				{
					return;
				}
				if (bMoving)
				{
					// De lado a 2 m/s, ida y vuelta de 3 m
					E->SetActorLocation(St->Base + FVector(0.f, 150.f * FMath::Sin(T * 2.f / 1.5f), 0.f));
				}
				if (E->GetHealth()->IsDead())
				{
					Char()->SetFireHeld(false);
					return;
				}
				// La mira sobre el hueso (seguimiento perfecto: lo que se prueba es el registro del impacto, no la puntería)
				const FVector Aim = E->GetMesh()->GetBoneLocation(bHead ? FName("head") : FName("spine_03")) + (bHead ? FVector(0.f, 0.f, 4.f) : FVector::ZeroVector);
				Char()->GetController()->SetControlRotation((Aim - Char()->GetCamera()->GetComponentLocation()).Rotation());
				// Tiro a tiro cada 0,3 s (deja recuperar la mira)
				const bool bFire = T >= St->NextShot && T < St->NextShot + 0.05f;
				Char()->SetFireHeld(bFire);
				if (T >= St->NextShot + 0.05f)
				{
					St->NextShot += 0.3f;
				}
			},
			[this, W, St, Name, bHead](FString& D)
			{
				const ABLEnemyCharacter* E = St->Target.Get();
				const int32 Shots = W->GetShotsFired() - St->Shots0;
				const bool bDead = E && E->GetHealth()->IsDead();
				D = FString::Printf(TEXT("muerto=%d tras %d disparos y %d impactos (%d a la cabeza); impacto que lo mató: %s n.º %d"),
					bDead, Shots, RangeHits, RangeHeadHits, RangeKillHit < 0 ? TEXT("-") : (RangeKillZone == EBLHitZone::Head ? TEXT("cabeza") : RangeKillZone == EBLHitZone::Limb ? TEXT("extremidad") : TEXT("torso")), RangeKillHit);
				if (bHead)
				{
					// El primer impacto en la cabeza tiene que matar
					return bDead && RangeKillZone == EBLHitZone::Head && RangeHeadHits == 1;
				}
				return bDead && RangeHits > 0;
			},
			[this, St]()
			{
				Char()->SetFireHeld(false);
				if (ABLEnemyCharacter* E = St->Target.Get())
				{
					E->Destroy();
				}
			} });
		Steps.Last().Done = [St]() { return St->Target.IsValid() && St->Target->GetHealth()->IsDead(); };
	};

	for (const float M : { 20.f, 50.f, 100.f, 140.f })
	{
		AddCase(TEXT("AR7"), M, true, false);
	}
	AddCase(TEXT("AR7"), 50.f, true, true);
	AddCase(TEXT("AR7"), 100.f, true, true);
	for (const float M : { 20.f, 50.f, 100.f, 140.f })
	{
		AddCase(TEXT("AR7"), M, false, false);
	}
	AddCase(TEXT("P17"), 20.f, true, false);
	AddCase(TEXT("P17"), 50.f, true, false);
	AddCase(TEXT("P17"), 20.f, false, false);
}
