// Prueba automática "Mission3" (misión 3 "Ría"): mapa /Game/Maps/M03/L_M03_Ria.
// Recorre la misión: puente, casco viejo, casas (puerta normal + brecha con aturdimiento), cuaderno, tirador con láser,
// pescadores, operadores de Corvane (equipo propio), baliza, el barco zarpa, defensa, furgón y resumen.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLDoor.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"
#include "Mission/BLScriptedMover.h"
#include "Player/BLCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildMission3Test()
{
	Char()->GetHealth()->bInvulnerable = true;
	auto Director = [this]() { return ABLMissionDirector::Get(this); };
	auto Place = [this](const FVector& Loc, float Yaw, float Pitch = 0.f)
	{
		ABLCharacter* C = Char();
		C->GetCharacterMovement()->StopMovementImmediately();
		C->SetActorLocation(Loc + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PC = C->GetController()) { PC->SetControlRotation(FRotator(Pitch, Yaw, 0.f)); }
	};
	auto IndexIs = [Director](int32 Expected, FString& D)
	{
		const ABLMissionDirector* M = Director();
		D = FString::Printf(TEXT("objetivo %d: %s"), M ? M->GetCurrentIndex() + 1 : 0, M && M->GetCurrentObjective() ? *M->GetObjectiveDisplayText() : TEXT("-"));
		return M && M->GetCurrentIndex() == Expected;
	};
	auto KillSquad = [this](FName Squad)
	{
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (It->SquadId == Squad && !It->IsDead())
			{
				FPointDamageEvent Ev(2000.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
				It->TakeDamage(2000.f, Ev, Char()->GetController(), Char());
			}
		}
	};
	auto Door = [this]() -> ABLDoor* { for (TActorIterator<ABLDoor> It(GetWorld()); It; ++It) { if (It->Tags.Contains(FName("BLObjective_PuertaPractico"))) { return *It; } } return nullptr; };

	Steps.Add({ TEXT("Briefing"), 3.0f, nullptr, nullptr, [IndexIs](FString& D) { return IndexIs(0, D); }, nullptr, 2.5f });
	Steps.Add({ TEXT("Puente"), 1.5f, [Place]() { Place(FVector(-700.f, 600.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(1, D); }, nullptr, 0.8f });
	Steps.Add({ TEXT("Casco"), 1.5f, [Place]() { Place(FVector(2100.f, 1300.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(2, D); } });
	Steps.Add({ TEXT("Casas"), 1.5f, [KillSquad]() { KillSquad(FName("Casas")); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(3, D); } });

	// Brecha: mirar la puerta atrancada desde fuera, mantener F, esperar la explosión; los de dentro quedan aturdidos
	TSharedRef<int32> MaxStunned = MakeShared<int32>(0);
	FStep Breach{ TEXT("Brecha"), 7.0f,
		[Place, Door]()
		{
			if (const ABLDoor* Dr = Door())
			{
				const FVector C = Dr->GetInteractLocation();
				const FVector From(C.X, C.Y - 160.f, 0.f);
				Place(From, (C - From).Rotation().Yaw, -5.f);
			}
		},
		[this, MaxStunned](float T)
		{
			Char()->SetInteractHeld(T > 0.2f && T < 1.8f);
			if (T > 1.8f)
			{
				// Apartarse de la puerta antes de que reviente
				Char()->GetCharacterMovement()->StopMovementImmediately();
			}
			int32 N = 0;
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				const ABLAIController* AI = Cast<ABLAIController>(It->GetController());
				N += AI && AI->IsStunned() ? 1 : 0;
			}
			*MaxStunned = FMath::Max(*MaxStunned, N);
		},
		[Door, Director, MaxStunned](FString& D)
		{
			const ABLDoor* Dr = Door();
			D = FString::Printf(TEXT("brecha=%d, aturdidos %d, objetivo %d"), Dr && Dr->IsBreached(), *MaxStunned, Director() ? Director()->GetCurrentIndex() + 1 : 0);
			return Dr && Dr->IsBreached() && *MaxStunned >= 2 && Director() && Director()->GetCurrentIndex() == 4;
		}, [this]() { Char()->SetInteractHeld(false); }, 4.4f };
	Breach.Done = [Door]() { return Door() && Door()->IsBreached(); };
	Steps.Add(Breach);
	Steps.Add({ TEXT("DespejaCasa"), 1.5f, [KillSquad]() { KillSquad(FName("Casa3")); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(5, D); } });
	Steps.Add({ TEXT("Cuaderno"), 2.0f,
		[this]()
		{
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(FName("BLObjective_Cuaderno")))
				{
					It->Use(Char());
				}
			}
		}, nullptr, [IndexIs](FString& D) { return IndexIs(6, D); } });

	// Campanario: el tirador fija con el láser al jugador en la plaza; luego se le elimina
	TSharedRef<float> MaxLock = MakeShared<float>(0.f);
	TSharedRef<bool> SawLaser = MakeShared<bool>(false);
	FStep Sniper{ TEXT("Tirador"), 15.0f,
		[Place]() { Place(FVector(7300.f, 450.f, 0.f), 90.f, 25.f); },     // en la plaza (delante de la fuente), mirando al campanario
		[this, MaxLock, SawLaser](float T)
		{
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->IsSniper() && !It->IsDead())
				{
					*SawLaser |= It->IsLaserOn();
					if (const ABLAIController* AI = Cast<ABLAIController>(It->GetController()))
					{
						*MaxLock = FMath::Max(*MaxLock, AI->GetSniperLock());
					}
				}
			}
		},
		[SawLaser, MaxLock](FString& D)
		{
			D = FString::Printf(TEXT("láser visto=%d, fijación máx %.1f s"), *SawLaser, *MaxLock);
			return *SawLaser && *MaxLock >= 1.8f;
		}, nullptr, 6.f };
	Sniper.Done = [SawLaser, MaxLock]() { return *SawLaser && *MaxLock >= 2.0f; };
	Steps.Add(Sniper);
	Steps.Add({ TEXT("Campanario"), 1.5f, [KillSquad]() { KillSquad(FName("Campanario")); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(7, D); } });
	Steps.Add({ TEXT("Pescadores"), 1.5f, [Place]() { Place(FVector(10000.f, -1600.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(8, D); } });

	// Operadores de Corvane: equipo propio y silencio; combate un rato (granadas si te escondes) y se les elimina
	TSharedRef<int32> Grenades = MakeShared<int32>(0);
	FStep OpsStep{ TEXT("Operadores"), 14.0f,
		[Place]() { Place(FVector(11700.f, -1500.f, 0.f), -30.f); },
		[this, Grenades](float T)
		{
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (const ABLAIController* AI = Cast<ABLAIController>(It->GetController()); AI && It->IsOperator())
				{
					*Grenades = FMath::Max(*Grenades, AI->GetGrenadesThrown());
				}
			}
		},
		[this, Grenades](FString& D)
		{
			int32 NumOps = 0;
			bool bLook = true;
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->IsOperator())
				{
					++NumOps;
					bLook &= It->GetHealth()->GetMaxHealth() > 120.f;
				}
			}
			D = FString::Printf(TEXT("operadores %d (blindados=%d), granadas lanzadas %d"), NumOps, bLook, *Grenades);
			return NumOps >= 5 && bLook;
		}, nullptr, 3.f };
	Steps.Add(OpsStep);
	Steps.Add({ TEXT("Espigon"), 1.5f, [KillSquad]() { KillSquad(FName("Corvane")); KillSquad(FName("Pescadores")); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(9, D); } });

	// Baliza (F mantenido 4 s junto a la amarra) -> el barco zarpa; defensa mientras se aleja
	auto Ship = [this]() -> ABLScriptedMover* { for (TActorIterator<ABLScriptedMover> It(GetWorld()); It; ++It) { if (It->Tags.Contains(FName("BLShip"))) { return *It; } } return nullptr; };
	TSharedRef<FVector> ShipStart = MakeShared<FVector>(FVector::ZeroVector);
	FStep Beacon{ TEXT("Baliza"), 6.0f,
		[this, Place, Ship, ShipStart]()
		{
			Place(FVector(13900.f, -3040.f, 0.f), -90.f, -20.f);
			if (const ABLScriptedMover* S = Ship()) { *ShipStart = S->GetActorLocation(); }
		},
		[this](float T) { Char()->SetInteractHeld(T > 0.2f && T < 5.0f); },
		[IndexIs](FString& D) { return IndexIs(10, D); }, [this]() { Char()->SetInteractHeld(false); }, 2.f };
	Beacon.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 10; };
	Steps.Add(Beacon);
	TSharedRef<int32> MaxWaves = MakeShared<int32>(0);
	FStep Defend{ TEXT("Zarpa"), 90.0f,
		[Place]() { Place(FVector(12800.f, -2600.f, 0.f), -60.f); },
		[this, MaxWaves](float T)
		{
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this); M && M->GetCurrentIndex() == 10)
			{
				*MaxWaves = FMath::Max(*MaxWaves, M->GetWavesSpawned());
			}
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(FName("BLWaveEnemy")) && !It->IsDead() && It->GetGameTimeSinceCreation() > 4.f)
				{
					FPointDamageEvent Ev(2000.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
					It->TakeDamage(2000.f, Ev, Char()->GetController(), Char());
				}
			}
		}, nullptr, nullptr, 12.f };
	Defend.Verify = [Director, Ship, ShipStart, MaxWaves](FString& D)
	{
		const ABLMissionDirector* M = Director();
		const ABLScriptedMover* S = Ship();
		const float Moved = S ? FVector::Dist(S->GetActorLocation(), *ShipStart) : 0.f;
		D = FString::Printf(TEXT("oleadas %d/2, barco zarpó=%d (%.0f m), objetivo %d"), *MaxWaves, S && S->HasStarted(), Moved / 100.f, M ? M->GetCurrentIndex() + 1 : 0);
		return M && *MaxWaves == 2 && S && S->HasStarted() && Moved > 500.f && M->GetCurrentIndex() == 11;
	};
	Defend.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 11; };
	Steps.Add(Defend);
	Steps.Add({ TEXT("Furgon"), 1.5f, [Place]() { Place(FVector(13300.f, 2300.f, 0.f), 90.f); }, nullptr,
		[Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			D = FString::Printf(TEXT("objetivos pendientes=%d"), M && M->GetCurrentObjective() ? 1 : 0);
			return M && !M->GetCurrentObjective();
		} });
	FStep Done{ TEXT("Completada"), 40.0f, nullptr, nullptr,
		[Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			D = M ? FString::Printf(TEXT("completada=%d tiempo %.0f s"), M->IsMissionComplete(), M->GetMissionTime()) : TEXT("sin director");
			return M && M->IsMissionComplete();
		} };
	Done.Done = [Director]() { return Director() && Director()->IsMissionComplete() && Director()->GetCompleteAge() > 2.5f; };
	Steps.Add(Done);
	Steps.Add({ TEXT("Resumen"), 0.6f, nullptr, nullptr, [](FString& D) { D = TEXT("captura"); return true; }, nullptr, 0.3f });
}
