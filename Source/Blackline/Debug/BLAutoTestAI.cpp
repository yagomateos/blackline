// Prueba automática "AI" (Bloque 5): mapa /Game/Maps/M01/L_M01_AmanecerRoto con sus 12 milicianos.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLCoverPoint.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLSquadSubsystem.h"
#include "Combat/BLHealthComponent.h"
#include "Combat/BLHitReactionComponent.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	TArray<ABLEnemyCharacter*> Enemies(UWorld* World, const FString& Prefix = FString())
	{
		TArray<ABLEnemyCharacter*> Out;
		for (TActorIterator<ABLEnemyCharacter> It(World); It; ++It)
		{
			if (Prefix.IsEmpty() || It->GetName().Contains(Prefix)
				|| It->Tags.ContainsByPredicate([&Prefix](const FName& T) { return T.ToString().StartsWith(TEXT("BLEnemy_") + Prefix); }))
			{
				Out.Add(*It);
			}
		}
		return Out;
	}

	ABLEnemyCharacter* EnemyByTag(UWorld* World, const FString& Name)
	{
		for (TActorIterator<ABLEnemyCharacter> It(World); It; ++It)
		{
			if (It->Tags.Contains(FName(*(TEXT("BLEnemy_") + Name))))
			{
				return *It;
			}
		}
		return nullptr;
	}

	ABLAIController* AIOf(const ABLEnemyCharacter* E)
	{
		return E ? Cast<ABLAIController>(E->GetController()) : nullptr;
	}
}

void UBLAutoTestComponent::BuildAITest()
{
	// El jugador no muere durante la prueba (salvo el paso de daño)
	Char()->GetHealth()->bInvulnerable = true;
	auto Place = [this](const FVector& Loc, float Yaw, bool bCrouch)
	{
		ABLCharacter* C = Char();
		C->GetCharacterMovement()->StopMovementImmediately();
		if (bCrouch && !C->bIsCrouched) { C->Crouch(); }
		if (!bCrouch && C->bIsCrouched) { C->UnCrouch(); }
		C->SetActorLocation(Loc + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PC = C->GetController())
		{
			PC->SetControlRotation(FRotator(0.f, Yaw, 0.f));
		}
	};

	struct FAIState { TMap<TWeakObjectPtr<ABLEnemyCharacter>, FVector> Start; float HealthBefore = 0.f; int32 ShotsBefore = 0; };
	TSharedRef<FAIState> St = MakeShared<FAIState>();

	Steps.Add({ TEXT("Enemigos"), 1.0f, [this, Place]() { Place(FVector(300.f, -170.f, 0.f), 0.f, false); }, nullptr,
		[this](FString& D)
		{
			int32 N = 0, Ctrl = 0, Patrol = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld()))
			{
				++N;
				if (ABLAIController* AI = AIOf(E)) { ++Ctrl; Patrol += AI->GetState() == EBLAIState::Patrol ? 1 : 0; }
			}
			D = FString::Printf(TEXT("%d enemigos, %d con controlador, %d en patrulla"), N, Ctrl, Patrol);
			return N == 12 && Ctrl == 12 && Patrol == 12;
		} });

	Steps.Add({ TEXT("Patrulla"), 7.0f,
		[this, St]()
		{
			St->Start.Reset();
			for (ABLEnemyCharacter* E : Enemies(GetWorld())) { St->Start.Add(E, E->GetActorLocation()); }
		}, nullptr,
		[this, St](FString& D)
		{
			int32 Moving = 0, Patrollers = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld()))
			{
				if (E->PatrolPoints.Num() > 0)
				{
					++Patrollers;
					const FVector* S = St->Start.Find(E);
					Moving += S && FVector::Dist2D(*S, E->GetActorLocation()) > 300.f ? 1 : 0;
				}
			}
			D = FString::Printf(TEXT("%d de %d patrullas recorren su ruta (> 3 m en 7 s)"), Moving, Patrollers);
			return Patrollers > 0 && Moving == Patrollers;
		}, nullptr, 3.f });

	// Oído: tres disparos en la calle trasera (sin línea de visión) alertan al patio (a 30-40 m)
	Steps.Add({ TEXT("Oido"), 3.0f,
		[this, Place]() { Place(FVector(2900.f, 0.f, 0.f), 90.f, false); },
		[this](float T) { Char()->SetFireHeld(T > 0.3f && T < 0.62f); },
		[this](FString& D)
		{
			int32 Alerted = 0;
			FString Who;
			for (ABLEnemyCharacter* E : Enemies(GetWorld()))
			{
				const ABLAIController* AI = AIOf(E);
				if (AI && AI->GetState() != EBLAIState::Patrol)
				{
					++Alerted;
					Who += FString::Printf(TEXT("%s(%s) "), *E->Tags.Last().ToString().RightChop(8), ABLAIController::StateName(AI->GetState()));
				}
			}
			D = FString::Printf(TEXT("%d enemigos reaccionan al ruido: %s"), Alerted, *Who);
			return Alerted >= 1;
		}, [this]() { Char()->SetFireHeld(false); } });

	// Por la espalda y agachado: el guardia del local no lo detecta
	Steps.Add({ TEXT("NoDetecta"), 3.0f,
		[this, Place]() { Place(FVector(20800.f, 420.f, 0.f), 180.f, true); }, nullptr,
		[this](FString& D)
		{
			const ABLAIController* AI = AIOf(EnemyByTag(GetWorld(), TEXT("Calle_Local")));
			D = AI ? FString::Printf(TEXT("guardia: %s, consciencia %.2f"), ABLAIController::StateName(AI->GetState()), AI->GetAwareness()) : TEXT("sin guardia");
			return AI && AI->GetState() != EBLAIState::Combat;
		}, nullptr, 2.0f });

	// De frente y de pie: lo detecta y avisa a su escuadra
	Steps.Add({ TEXT("Detecta"), 3.0f,
		[this, Place]() { Place(FVector(19750.f, 420.f, 0.f), 0.f, false); }, nullptr,
		[this](FString& D)
		{
			const ABLAIController* AI = AIOf(EnemyByTag(GetWorld(), TEXT("Calle_Local")));
			int32 SquadCombat = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld(), TEXT("Calle")))
			{
				const ABLAIController* A = AIOf(E);
				SquadCombat += A && A->GetState() == EBLAIState::Combat ? 1 : 0;
			}
			const ABLEnemyCharacter* G = EnemyByTag(GetWorld(), TEXT("Calle_Local"));
			FVector EyeLoc = FVector::ZeroVector; FRotator EyeRot = FRotator::ZeroRotator;
			if (G) { G->GetActorEyesViewPoint(EyeLoc, EyeRot); }
			FHitResult Block;
			FCollisionObjectQueryParams Obj;
			Obj.AddObjectTypesToQuery(ECC_WorldStatic);
			Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
			const bool bBlocked = G && GetWorld()->LineTraceSingleByObjectType(Block, EyeLoc, Char()->GetCamera()->GetComponentLocation(), Obj, FCollisionQueryParams(NAME_None, false, G));
			UE_LOG(LogBlackline, Display, TEXT("[BLTest] LOS guardia->jugador bloqueada=%d por %s en %s"), bBlocked, *GetNameSafe(Block.GetActor()), *Block.ImpactPoint.ToCompactString());
			D = FString::Printf(TEXT("guardia: %s; escuadra 'Calle' en combate: %d/5 [guardia en %s yaw %.0f, ojos %s rot %.0f, jugador %s]"), AI ? ABLAIController::StateName(AI->GetState()) : TEXT("-"), SquadCombat,
				G ? *G->GetActorLocation().ToCompactString() : TEXT("-"), G ? G->GetActorRotation().Yaw : 0.f, *EyeLoc.ToCompactString(), EyeRot.Yaw, *Char()->GetActorLocation().ToCompactString());
			return AI && AI->GetState() == EBLAIState::Combat && SquadCombat >= 4;
		}, nullptr, 2.8f });

	// Coberturas: el jugador se pone a 20 m en la calle; los enemigos buscan cobertura
	Steps.Add({ TEXT("Coberturas"), 9.0f,
		[this, Place]() { Place(FVector(16400.f, 300.f, 0.f), 0.f, false); }, nullptr,
		[this](FString& D)
		{
			int32 InCover = 0, Protected = 0, Engaged = 0;
			const FVector Eye = Char()->GetCamera()->GetComponentLocation();
			for (ABLEnemyCharacter* E : Enemies(GetWorld(), TEXT("Calle")))
			{
				const ABLAIController* AI = AIOf(E);
				if (!AI || AI->GetState() != EBLAIState::Combat) { continue; }
				++Engaged;
				if (AI->IsAtCover() && AI->GetCover())
				{
					++InCover;
					Protected += AI->GetCover()->IsProtectedFrom(Eye) ? 1 : 0;
				}
			}
			D = FString::Printf(TEXT("%d en combate, %d en cobertura, %d protegidos del jugador"), Engaged, InCover, Protected);
			return InCover >= 2 && Protected >= 2;
		}, nullptr, 8.5f });

	// Disparan: sin invulnerabilidad unos segundos; turnos de disparo limitados
	Steps.Add({ TEXT("Disparan"), 8.0f,
		[this, St]()
		{
			St->HealthBefore = Char()->GetHealth()->GetHealth();
			St->ShotsBefore = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld())) { if (const ABLAIController* AI = AIOf(E)) { St->ShotsBefore += AI->GetShotsFired(); } }
			Char()->GetHealth()->bInvulnerable = false;
		},
		[this](float T)
		{
			// No morir en la prueba: se cura si baja de 30
			if (Char()->GetHealth()->GetHealth() < 30.f) { Char()->GetHealth()->ResetHealth(); }
		},
		[this, St](FString& D)
		{
			int32 Shots = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld())) { if (const ABLAIController* AI = AIOf(E)) { Shots += AI->GetShotsFired(); } }
			Shots -= St->ShotsBefore;
			const UBLSquadSubsystem* S = GetWorld()->GetSubsystem<UBLSquadSubsystem>();
			D = FString::Printf(TEXT("%d disparos enemigos, salud %.0f -> %.0f, turnos simultaneos max %d (limite %d)"), Shots, St->HealthBefore,
				Char()->GetHealth()->GetHealth(), S ? S->GetMaxTokensSeen() : -1, S ? S->MaxAttackTokens : -1);
			return Shots > 5 && S && S->GetMaxTokensSeen() <= S->MaxAttackTokens;
		},
		[this]() { Char()->GetHealth()->bInvulnerable = true; Char()->GetHealth()->ResetHealth(); }, 4.f });

	Steps.Add({ TEXT("Recarga"), 14.0f, nullptr, nullptr,
		[this](FString& D)
		{
			int32 Reloads = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld())) { if (const ABLAIController* AI = AIOf(E)) { Reloads += AI->GetReloads(); } }
			D = FString::Printf(TEXT("%d recargas"), Reloads);
			return Reloads >= 1;
		} });

	Steps.Add({ TEXT("Flanqueo"), 12.0f, nullptr, nullptr,
		[this](FString& D)
		{
			const UBLSquadSubsystem* S = GetWorld()->GetSubsystem<UBLSquadSubsystem>();
			int32 Flanks = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld())) { if (const ABLAIController* AI = AIOf(E)) { Flanks += AI->GetFlanksDone(); } }
			D = FString::Printf(TEXT("flanqueos asignados %d, iniciados %d"), S ? S->GetFlankAssignments() : 0, Flanks);
			return Flanks >= 1;
		}, nullptr, 6.f });

	// Perdido de vista: van a buscarlo
	Steps.Add({ TEXT("Persecucion"), 13.0f,
		[this, Place]() { Place(FVector(14200.f, -2480.f, 0.f), 90.f, true); }, nullptr,
		[this](FString& D)
		{
			int32 Hunting = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld(), TEXT("Calle")))
			{
				const ABLAIController* AI = AIOf(E);
				Hunting += AI && (AI->GetState() == EBLAIState::Search || (AI->GetState() == EBLAIState::Combat && AI->GetAction() == EBLCombatAction::Chase)) ? 1 : 0;
			}
			D = FString::Printf(TEXT("%d enemigos persiguen/buscan"), Hunting);
			return Hunting >= 1;
		} });

	// Retrato: un centinela del control visto de cerca a la luz del día (pose, arma, mano izquierda)
	Steps.Add({ TEXT("Retrato"), 1.6f,
		[this, Place]()
		{
			if (ABLEnemyCharacter* G = EnemyByTag(GetWorld(), TEXT("Control_SacosE")))
			{
				const FVector P = G->GetActorLocation() + FVector(260.f, 170.f, -94.f);
				Place(P, (G->GetActorLocation() - P).Rotation().Yaw, false);
				if (AController* PC = Char()->GetController()) { PC->SetControlRotation(FRotator(-8.f, (G->GetActorLocation() - P).Rotation().Yaw, 0.f)); }
			}
		}, nullptr, [](FString& D) { D = TEXT("captura"); return true; }, nullptr, 1.4f });

	// Bajas: la escuadra de la calle muere (daño directo del jugador) y cae en ragdoll
	Steps.Add({ TEXT("Bajas"), 2.0f,
		[this]()
		{
			for (ABLEnemyCharacter* E : Enemies(GetWorld(), TEXT("Calle")))
			{
				FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
				Ev.HitInfo.BoneName = FName("spine_03");
				Ev.HitInfo.ImpactPoint = E->GetActorLocation();
				E->TakeDamage(500.f, Ev, Char()->GetController(), Char());
			}
		}, nullptr,
		[this](FString& D)
		{
			int32 Dead = 0, Ragdoll = 0, N = 0;
			for (ABLEnemyCharacter* E : Enemies(GetWorld(), TEXT("Calle")))
			{
				++N;
				Dead += E->IsDead() ? 1 : 0;
				Ragdoll += E->GetHitReaction()->IsRagdoll() ? 1 : 0;
			}
			const UBLSquadSubsystem* S = GetWorld()->GetSubsystem<UBLSquadSubsystem>();
			D = FString::Printf(TEXT("%d/%d muertos, %d en ragdoll, quedan %d en la escuadra global"), Dead, N, Ragdoll, S ? S->GetMembers().Num() : -1);
			return N == 5 && Dead == 5 && Ragdoll == 5 && S && S->GetMembers().Num() == 7;
		}, nullptr, 1.5f });
}
