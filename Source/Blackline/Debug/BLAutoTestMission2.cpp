// Prueba automática "Mission2" (misión 2 "Manifiesto"): mapa /Game/Maps/M02/L_M02_Manifiesto.
// Recorre la misión entera: noche (visibilidad), valla, tanques, almacén, fotos (F mantenido y las demás), alarma
// (focos, tirador de supresión), defensa, dron (derribo), tubería de gas, zanja, muelle, lancha y resumen.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Environment/BLAlarmLight.h"
#include "Environment/BLNightSettings.h"
#include "Mission/BLHazardEvent.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"
#include "Vehicles/BLBoat.h"
#include "Vehicles/BLDrone.h"

#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildMission2Test()
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
		const FBLObjective* O = M ? M->GetCurrentObjective() : nullptr;
		D = FString::Printf(TEXT("objetivo %d: %s"), M ? M->GetCurrentIndex() + 1 : 0, O ? *M->GetObjectiveDisplayText() : TEXT("-"));
		return M && M->GetCurrentIndex() == Expected;
	};
	// Elimina a los milicianos de las oleadas que lleven más de Grace segundos vivos (simula la defensa)
	auto KillWaves = [this](float Grace, float GunnerGrace)
	{
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			const float G = It->IsGunner() ? GunnerGrace : Grace;
			if (It->Tags.Contains(FName("BLWaveEnemy")) && !It->IsDead() && It->GetGameTimeSinceCreation() > G)
			{
				FPointDamageEvent Ev(800.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
				It->TakeDamage(800.f, Ev, Char()->GetController(), Char());
			}
		}
	};

	Steps.Add({ TEXT("Briefing"), 3.0f, nullptr, nullptr,
		[Director, IndexIs](FString& D)
		{
			FString Speaker, Text;
			float Alpha;
			const ABLMissionDirector* M = Director();
			const bool bSub = M && M->GetSubtitle(Speaker, Text, Alpha);
			const bool bIndex = IndexIs(0, D);
			D += FString::Printf(TEXT(" | radio: %s"), bSub ? *(Speaker + TEXT(": ") + Text.Left(40)) : TEXT("(nada)"));
			return bIndex && bSub;
		}, nullptr, 2.5f });

	// Noche: a oscuras (entre muretes) se ve poco; bajo una farola de la avenida, como de día
	Steps.Add({ TEXT("Noche"), 1.0f, [Place]() { Place(FVector(2700.f, -2200.f, 0.f), 0.f); }, nullptr,
		[this](FString& D)
		{
			const float Dark = ABLNightSettings::GetVisibility(GetWorld(), FVector(2700.f, -500.f, 150.f), Char(), 1700.f);
			const bool bLit = ABLNightSettings::IsLit(GetWorld(), FVector(2700.f, -980.f, 0.f));
			const float Far = ABLNightSettings::GetVisibility(GetWorld(), FVector(2700.f, 2000.f, 150.f), Char(), 4200.f);
			D = FString::Printf(TEXT("a oscuras %.2f, lejos %.2f, bajo la farola iluminado=%d"), Dark, Far, bLit);
			return Dark > 0.f && Dark < 0.6f && Far == 0.f && bLit;
		}, nullptr, 0.6f });

	Steps.Add({ TEXT("Valla"), 1.5f, [Place]() { Place(FVector(650.f, 600.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(1, D); } });
	Steps.Add({ TEXT("Tanques"), 1.5f, [Place]() { Place(FVector(6400.f, 0.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(2, D); } });
	Steps.Add({ TEXT("Almacen"), 1.5f, [Place]() { Place(FVector(10850.f, 1800.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(3, D); } });

	// Primera foto como el jugador: mirarla desde 1,2 m y mantener F (en la penumbra del almacén)
	Steps.Add({ TEXT("Foto"), 2.6f,
		[this, Place]()
		{
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(FName("BLObjective_Foto")) && !It->IsUsed())
				{
					const FVector T = It->GetActorLocation();
					const FVector From(T.X + 120.f, T.Y + 20.f, 0.f);
					Place(From, (T - From).Rotation().Yaw, -10.f);
					break;
				}
			}
		},
		[this](float T) { Char()->SetInteractHeld(T > 0.3f && T < 2.3f); },
		[Director](FString& D)
		{
			D = FString::Printf(TEXT("fotos %d, objetivo: %s"), ABLInteractable::GetPhotosTaken(), Director() ? *Director()->GetObjectiveDisplayText() : TEXT("-"));
			return ABLInteractable::GetPhotosTaken() == 1;
		}, [this]() { Char()->SetInteractHeld(false); }, 2.0f });
	// Las otras dos
	Steps.Add({ TEXT("Fotos"), 2.0f,
		[this]()
		{
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(FName("BLObjective_Foto")) && It->CanInteract(Char()))
				{
					It->Use(Char());
				}
			}
		}, nullptr,
		[this, Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			int32 On = 0;
			for (TActorIterator<ABLAlarmLight> It(GetWorld()); It; ++It) { On += It->IsOn() ? 1 : 0; }
			D = FString::Printf(TEXT("fotos %d, alarma=%d, focos encendidos %d, objetivo %d"), ABLInteractable::GetPhotosTaken(),
				M && M->IsAlarmActive(), On, M ? M->GetCurrentIndex() + 1 : 0);
			return ABLInteractable::GetPhotosTaken() == 3 && M && M->IsAlarmActive() && On >= 5 && M->GetCurrentIndex() == 4;
		} });

	// Alarma: tirador de supresión en la pasarela, refuerzos con linterna; deja disparar a la ametralladora
	TSharedRef<int32> GunnerShots = MakeShared<int32>(0);
	TSharedRef<int32> MaxWaves = MakeShared<int32>(0);
	TSharedRef<bool> SawFlashlight = MakeShared<bool>(false);
	FStep Defend{ TEXT("Alarma"), 120.0f,
		[Place]() { Place(FVector(13900.f, -1500.f, 0.f), -144.f, 20.f); },    // con línea de tiro limpia desde la pasarela
		[this, GunnerShots, MaxWaves, SawFlashlight, KillWaves](float T)
		{
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this); M && M->GetCurrentIndex() == 4)
			{
				*MaxWaves = FMath::Max(*MaxWaves, M->GetWavesSpawned());
			}
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->IsGunner())
				{
					if (const ABLAIController* AI = Cast<ABLAIController>(It->GetController()))
					{
						*GunnerShots = FMath::Max(*GunnerShots, AI->GetShotsFired());
					}
				}
				*SawFlashlight |= It->Tags.Contains(FName("BLWaveEnemy")) && It->GetFlashlight() != nullptr;
			}
			KillWaves(4.f, 9.f);
		}, nullptr, nullptr, 6.f };
	Defend.Verify = [Director, GunnerShots, MaxWaves, SawFlashlight](FString& D)
	{
		const ABLMissionDirector* M = Director();
		D = FString::Printf(TEXT("oleadas %d/3, disparos de la ametralladora %d, linternas en refuerzos=%d, objetivo %d"),
			*MaxWaves, *GunnerShots, *SawFlashlight, M ? M->GetCurrentIndex() + 1 : 0);
		return M && *MaxWaves == 3 && *GunnerShots >= 8 && *SawFlashlight && M->GetCurrentIndex() == 5;
	};
	Defend.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 5; };
	Steps.Add(Defend);

	// Puerta trasera: aparece el dron; se derriba a tiros
	auto Drone = [this]() -> ABLDrone* { for (TActorIterator<ABLDrone> It(GetWorld()); It; ++It) { return *It; } return nullptr; };
	Steps.Add({ TEXT("Trasera"), 2.5f, [Place]() { Place(FVector(14900.f, -1500.f, 0.f), 20.f, 25.f); }, nullptr,
		[IndexIs, Drone](FString& D)
		{
			const bool bIdx = IndexIs(6, D);
			D += FString::Printf(TEXT(" | dron activo=%d"), Drone() && Drone()->IsActive());
			return bIdx && Drone() && Drone()->IsActive();
		}, nullptr, 2.2f });
	Steps.Add({ TEXT("Dron"), 8.0f,
		[this, Drone]()
		{
			if (ABLDrone* Dr = Drone())
			{
				FPointDamageEvent Ev(300.f, FHitResult(), FVector(0.f, 0.f, 1.f), nullptr);
				Dr->TakeDamage(300.f, Ev, Char()->GetController(), Char());
			}
		}, nullptr,
		[Drone](FString& D)
		{
			const ABLDrone* Dr = Drone();
			D = FString::Printf(TEXT("derribado=%d, estrellado=%d (a %.0f cm)"), Dr && Dr->IsDestroyed(), Dr && Dr->IsWrecked(), Dr ? Dr->GetActorLocation().Z : -1.f);
			return Dr && Dr->IsWrecked();
		}, nullptr, -1.f, [Drone]() { return Drone() && Drone()->IsWrecked(); } });

	// La tubería de gas revienta al acercarse y corta la calle; rodeo por la zanja
	auto Hazard = [this]() -> ABLHazardEvent* { for (TActorIterator<ABLHazardEvent> It(GetWorld()); It; ++It) { return *It; } return nullptr; };
	Steps.Add({ TEXT("Gas"), 3.0f, [Place]() { Place(FVector(15900.f, 0.f, 0.f), 0.f); }, nullptr,
		[this, Hazard](FString& D)
		{
			FHitResult Hit;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(BLTestGas), false, Char());
			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, FVector(17000.f, 0.f, 120.f), FVector(17900.f, 0.f, 120.f), ECC_Pawn, Q);
			D = FString::Printf(TEXT("estallido=%d, calle cortada=%d (%s)"), Hazard() && Hazard()->HasFired(), bBlocked, bBlocked ? *GetNameSafe(Hit.GetActor()) : TEXT("-"));
			return Hazard() && Hazard()->HasFired() && bBlocked;
		}, nullptr, 1.5f, [Hazard]() { return Hazard() && Hazard()->HasFired(); } });
	Steps.Add({ TEXT("Zanja"), 1.5f, [Place]() { Place(FVector(17500.f, 2490.f, -160.f), 0.f, -5.f); }, nullptr,
		[this](FString& D)
		{
			const float Z = Char()->GetActorLocation().Z - Char()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			D = FString::Printf(TEXT("pies a %.0f cm (fondo de la zanja)"), Z);
			return Z > -180.f && Z < -130.f;
		}, nullptr, 1.0f });
	Steps.Add({ TEXT("Proceso"), 1.5f, [Place]() { Place(FVector(20300.f, 600.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(7, D); } });

	// Muelle: defensa hasta que llega la lancha; luego subir
	auto Boat = [this]() -> ABLBoat* { for (TActorIterator<ABLBoat> It(GetWorld()); It; ++It) { if (It->Tags.Contains(FName("BLBoat"))) { return *It; } } return nullptr; };
	TSharedRef<int32> MaxWavesQ = MakeShared<int32>(0);
	FStep Quay{ TEXT("Muelle"), 120.0f,
		[Place]() { Place(FVector(21500.f, 0.f, 0.f), 180.f); },
		[this, MaxWavesQ, KillWaves](float T)
		{
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this); M && M->GetCurrentIndex() == 7)
			{
				*MaxWavesQ = FMath::Max(*MaxWavesQ, M->GetWavesSpawned());
			}
			KillWaves(4.f, 6.f);
		}, nullptr, nullptr, 25.f };
	Quay.Verify = [Director, Boat, MaxWavesQ](FString& D)
	{
		const ABLMissionDirector* M = Director();
		D = FString::Printf(TEXT("oleadas %d/2, lancha atracada=%d, objetivo %d"), *MaxWavesQ, Boat() && Boat()->IsDocked(), M ? M->GetCurrentIndex() + 1 : 0);
		return M && *MaxWavesQ == 2 && Boat() && Boat()->IsDocked() && M->GetCurrentIndex() == 8;
	};
	Quay.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 8; };
	Steps.Add(Quay);
	FStep Board{ TEXT("Lancha"), 20.0f, nullptr,
		[this](float T)
		{
			for (TActorIterator<ABLInteractable> It(GetWorld()); It && T > 1.f; ++It)
			{
				if (It->Tags.Contains(FName("BLObjective_Boat")) && It->CanInteract(Char()))
				{
					It->Use(Char());
				}
			}
		},
		[Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			D = FString::Printf(TEXT("objetivos pendientes=%d"), M && M->GetCurrentObjective() ? 1 : 0);
			return M && !M->GetCurrentObjective();
		} };
	Board.Done = [Director]() { return Director() && !Director()->GetCurrentObjective(); };
	Steps.Add(Board);

	FStep Done{ TEXT("Completada"), 40.0f, nullptr, nullptr,
		[Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			D = M ? FString::Printf(TEXT("completada=%d tiempo %.0f s, alarma=%d"), M->IsMissionComplete(), M->GetMissionTime(), M->IsAlarmActive()) : TEXT("sin director");
			return M && M->IsMissionComplete();
		} };
	Done.Done = [Director]() { return Director() && Director()->IsMissionComplete() && Director()->GetCompleteAge() > 2.5f; };
	Steps.Add(Done);
	Steps.Add({ TEXT("Resumen"), 0.6f, nullptr, nullptr, [](FString& D) { D = TEXT("captura"); return true; }, nullptr, 0.3f });
}
