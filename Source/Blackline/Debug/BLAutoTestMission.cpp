// Prueba automática "Mission" (Bloque 6): mapa /Game/Maps/M01/L_M01_AmanecerRoto.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLVarek.h"
#include "Environment/BLSmokeEmitter.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"
#include "Vehicles/BLBTR.h"
#include "Vehicles/BLHelicopter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildMissionTest()
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
		D = FString::Printf(TEXT("objetivo %d: %s"), M ? M->GetCurrentIndex() + 1 : 0, O ? *O->Text : TEXT("-"));
		return M && M->GetCurrentIndex() == Expected;
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
			return bIndex && bSub && Speaker == TEXT("TORRE");
		}, nullptr, 2.6f });

	Steps.Add({ TEXT("Callejon"), 1.5f, [Place]() { Place(FVector(3050.f, -900.f, 0.f), -90.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(1, D); } });

	Steps.Add({ TEXT("Puerta"), 1.5f, [Place]() { Place(FVector(8900.f, -4100.f, 0.f), 90.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(2, D); } });

	Steps.Add({ TEXT("Marcador"), 1.2f, [Place]() { Place(FVector(9600.f, -3900.f, 0.f), 80.f); }, nullptr,
		[Director](FString& D)
		{
			FVector M;
			const bool bOk = Director() && Director()->GetMarkerLocation(M);
			D = bOk ? FString::Printf(TEXT("marcador en %s"), *M.ToCompactString()) : TEXT("sin marcador");
			return bOk && FVector::Dist2D(M, FVector(9700.f, -2250.f, 0.f)) < 50.f;
		}, nullptr, 1.0f });

	Steps.Add({ TEXT("Control"), 2.0f,
		[this]()
		{
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->SquadId == FName("Control"))
				{
					FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
					It->TakeDamage(500.f, Ev, Char()->GetController(), Char());
				}
			}
		}, nullptr,
		[IndexIs](FString& D) { return IndexIs(3, D); } });

	Steps.Add({ TEXT("Local"), 1.5f, [Place]() { Place(FVector(19650.f, 600.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(4, D); } });

	// Recoger el disco: mirarlo desde 1,3 m y mantener F
	Steps.Add({ TEXT("Interaccion"), 2.2f,
		[this, Place]()
		{
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
			{
				if (!It->Tags.Contains(FName("BLObjective_Disco")))
				{
					continue;
				}
				const FVector Disk = It->GetActorLocation();
				const FVector From(Disk.X - 110.f, Disk.Y - 30.f, 0.f);
				Place(From, (Disk - From).Rotation().Yaw, -32.f);
			}
		},
		[this](float T) { Char()->SetInteractHeld(T > 0.3f && T < 2.0f); },
		[this](FString& D)
		{
			bool bUsed = false;
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It) { bUsed |= It->IsUsed() && It->Tags.Contains(FName("BLObjective_Disco")); }
			const ABLMissionDirector* M = ABLMissionDirector::Get(this);
			D = FString::Printf(TEXT("disco recogido=%d, objetivo %d"), bUsed, M ? M->GetCurrentIndex() + 1 : 0);
			return bUsed && M && M->GetCurrentIndex() == 5;
		}, [this]() { Char()->SetInteractHeld(false); }, 0.9f });

	// ---- Bloque 11: bloque de viviendas y Varek ----
	Steps.Add({ TEXT("Bloque"), 1.5f, [Place]() { Place(FVector(18410.f, 1500.f, 0.f), 180.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(6, D); } });

	Steps.Add({ TEXT("Despeja"), 2.0f,
		[this]()
		{
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->SquadId == FName("Bloque"))
				{
					FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
					It->TakeDamage(500.f, Ev, Char()->GetController(), Char());
				}
			}
		}, nullptr,
		[IndexIs](FString& D) { return IndexIs(7, D); } });

	Steps.Add({ TEXT("Varek"), 3.0f,
		[this, Place]()
		{
			for (TActorIterator<ABLVarek> It(GetWorld()); It; ++It)
			{
				const FVector V = It->GetActorLocation();
				const FVector From(V.X + 120.f, V.Y + 20.f, 640.f);
				Place(From, (V - From).Rotation().Yaw, -30.f);
			}
		},
		[this](float T) { Char()->SetInteractHeld(T > 0.3f && T < 2.8f); },
		[this](FString& D)
		{
			bool bFree = false;
			for (TActorIterator<ABLVarek> It(GetWorld()); It; ++It) { bFree |= It->IsFree(); }
			const ABLMissionDirector* M = ABLMissionDirector::Get(this);
			D = FString::Printf(TEXT("Varek libre=%d, objetivo %d"), bFree, M ? M->GetCurrentIndex() + 1 : 0);
			return bFree && M && M->GetCurrentIndex() == 8;   // empieza el contraataque
		}, [this]() { Char()->SetInteractHeld(false); }, 2.9f });

	Steps.Add({ TEXT("SigueJugador"), 5.0f, [Place]() { Place(FVector(18700.f, 2025.f, 640.f), 0.f); }, nullptr,
		[this](FString& D)
		{
			float Dist = 1e9f;
			for (TActorIterator<ABLVarek> It(GetWorld()); It; ++It) { Dist = FVector::Dist(It->GetActorLocation(), Char()->GetActorLocation()); }
			D = FString::Printf(TEXT("Varek a %.0f cm del jugador"), Dist);
			return Dist < 550.f;
		}, nullptr, 4.5f });

	// ---- Fase 7: contraataque (se simula la defensa eliminando a cada miliciano que llega) ----
	TSharedRef<int32> MaxSmoke = MakeShared<int32>(0);
	TSharedRef<int32> MaxWaves = MakeShared<int32>(0);
	FStep Defend{ TEXT("Contraataque"), 130.0f,
		[Place]() { Place(FVector(18200.f, 1420.f, 640.f), -90.f, -12.f); },   // ventana de la 2.ª planta que da a la calle
		[this, MaxSmoke, MaxWaves](float T)
		{
			*MaxSmoke = FMath::Max(*MaxSmoke, ABLSmokeEmitter::GetActiveSmokeScreens());
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this); M && M->GetCurrentIndex() == 8)
			{
				*MaxWaves = FMath::Max(*MaxWaves, M->GetWavesSpawned());
			}
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				// Les da 4 s de vida para que lancen el humo y se muevan
				if (It->Tags.Contains(FName("BLWaveEnemy")) && !It->IsDead() && It->GetGameTimeSinceCreation() > 4.f)
				{
					FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
					It->TakeDamage(500.f, Ev, Char()->GetController(), Char());
				}
			}
		},
		nullptr, nullptr, 17.f };
	Defend.Verify = [this, MaxSmoke, MaxWaves](FString& D)
	{
		const ABLMissionDirector* M = ABLMissionDirector::Get(this);
		D = FString::Printf(TEXT("oleadas %d/3, humo visto %d nubes, objetivo %d"), *MaxWaves, *MaxSmoke, M ? M->GetCurrentIndex() + 1 : 0);
		return M && *MaxWaves == 3 && *MaxSmoke >= 1 && M->GetCurrentIndex() == 9;
	};
	Defend.Done = [this]() { const ABLMissionDirector* M = ABLMissionDirector::Get(this); return M && M->GetCurrentIndex() >= 9; };
	Steps.Add(Defend);

	// ---- Fase 8: el blindado entra, dispara y derriba la fachada; se sube a la azotea y se cruza por los tejados ----
	auto BTR = [this]() -> ABLBTR* { for (TActorIterator<ABLBTR> It(GetWorld()); It; ++It) { return *It; } return nullptr; };
	FStep Armor{ TEXT("Blindado"), 50.0f,
		[Place]() { Place(FVector(17400.f, 1420.f, 960.f), -170.f, -6.f); },   // azotea, al oeste del tramo que cae
		nullptr,
		[this, BTR, Director](FString& D)
		{
			const ABLBTR* B = BTR();
			const ABLMissionDirector* M = Director();
			int32 Blocks = 0;
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				Blocks += It->Tags.Contains(FName("BLCollapseBlock")) && !It->IsHidden() ? 1 : 0;
			}
			D = FString::Printf(TEXT("blindado activo=%d en posición=%d disparos=%d derrumbe=%d escombros visibles=%d, objetivo %d"),
				B && B->IsActive(), B && B->HasArrived(), B ? B->GetShotsFired() : 0, B && B->HasCollapsed(), Blocks, M ? M->GetCurrentIndex() + 1 : 0);
			return B && B->HasArrived() && B->GetShotsFired() >= 3 && B->HasCollapsed() && Blocks == 3 && M && M->GetCurrentIndex() == 10;
		}, nullptr, 15.f };
	Armor.Done = [BTR]() { const ABLBTR* B = BTR(); return B && B->GetShotsFired() >= 3; };
	Steps.Add(Armor);
	Steps.Add({ TEXT("Pasarela"), 2.0f, [Place]() { Place(FVector(19560.f, 2450.f, 960.f), 0.f, -8.f); }, nullptr,
		[this](FString& D)
		{
			// La pasarela aguanta (no se cae al callejón) y se ve la azotea de C4
			const float Z = Char()->GetActorLocation().Z - Char()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			D = FString::Printf(TEXT("pies a %.0f cm"), Z);
			return Z > 930.f;
		}, nullptr, 1.2f });
	Steps.Add({ TEXT("Incendios"), 1.5f, [Place]() { Place(FVector(21185.f, 1700.f, 0.f), 90.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(11, D); } });

	// ---- Fase 9: zona de aterrizaje, defensa con el helicóptero y extracción ----
	auto Heli = [this]() -> ABLHelicopter* { for (TActorIterator<ABLHelicopter> It(GetWorld()); It; ++It) { return *It; } return nullptr; };
	Steps.Add({ TEXT("ZonaAterrizaje"), 1.5f, [Place]() { Place(FVector(25000.f, 1300.f, 0.f), 0.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(12, D); } });
	TSharedRef<bool> SawHover = MakeShared<bool>(false);
	TSharedRef<int32> MaxWavesLZ = MakeShared<int32>(0);
	FStep DefendLZ{ TEXT("DefensaLZ"), 120.0f,
		[Place]() { Place(FVector(23000.f, 1500.f, 0.f), -6.f, 22.f); },
		[this, Heli, SawHover, MaxWavesLZ](float T)
		{
			const ABLHelicopter* H = Heli();
			*SawHover |= H && H->IsHovering();
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this); M && M->GetCurrentIndex() == 12)
			{
				*MaxWavesLZ = FMath::Max(*MaxWavesLZ, M->GetWavesSpawned());
			}
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				// Con el helicóptero en estacionario los deja vivir 14 s (que dispare el ametrallador); antes, 4 s
				const float Grace = H && H->IsHovering() ? 14.f : 4.f;
				if (It->Tags.Contains(FName("BLWaveEnemy")) && !It->IsDead() && It->GetGameTimeSinceCreation() > Grace)
				{
					FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
					It->TakeDamage(500.f, Ev, Char()->GetController(), Char());
				}
			}
		},
		nullptr, nullptr, 40.f };
	DefendLZ.Verify = [Director, Heli, SawHover, MaxWavesLZ](FString& D)
	{
		const ABLMissionDirector* M = Director();
		const ABLHelicopter* H = Heli();
		D = FString::Printf(TEXT("oleadas %d/3, helicóptero activo=%d estacionario visto=%d ráfagas=%d, objetivo %d"),
			*MaxWavesLZ, H && H->IsActive(), *SawHover, H ? H->GetBurstsFired() : 0, M ? M->GetCurrentIndex() + 1 : 0);
		return M && M->GetCurrentIndex() == 13 && H && *SawHover && *MaxWavesLZ == 3 && H->GetBurstsFired() >= 1;
	};
	DefendLZ.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 13; };
	Steps.Add(DefendLZ);
	// Subir como un jugador: junto a la puerta izquierda, mirando al hueco y manteniendo F (detección real: alcance,
	// ángulo y línea de vista; antes la prueba llamaba a Use() y no veía que la cabina maciza impedía subir)
	TSharedRef<bool> Placed = MakeShared<bool>(false);
	TSharedRef<float> PlacedAt = MakeShared<float>(-1.f);
	TSharedRef<bool> DoorShot = MakeShared<bool>(false);
	FStep Board{ TEXT("Helicoptero"), 40.0f,
		[Place, Placed]() { *Placed = false; Place(FVector(24300.f, 300.f, 0.f), 55.f, 4.f); },
		[this, Heli, Placed, PlacedAt, DoorShot](float T)
		{
			ABLHelicopter* H = Heli();
			if (H && Char()->IsInVehicleSeat())
			{
				Char()->SetInteractHeld(false);
				if (!bScreenshotTaken) { bScreenshotTaken = true; Screenshot(TEXT("HeliDentro")); }
				return;
			}
			if (!H || !H->IsLanded())
			{
				Char()->SetInteractHeld(false);
				return;
			}
			ABLCharacter* C = Char();
			if (!*Placed)
			{
				*Placed = true;
				*PlacedAt = -1.f;
				const FVector Door = H->GetActorTransform().TransformPosition(FVector(25.f, -185.f, 0.f));
				C->GetCharacterMovement()->StopMovementImmediately();
				C->SetActorLocation(Door + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
			}
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(FName("BLObjective_Heli")))
				{
					C->GetController()->SetControlRotation((It->GetInteractLocation() - C->GetCamera()->GetComponentLocation()).Rotation());
				}
			}
			// Primero una captura de la puerta abierta (sin pulsar), luego mantener F
			if (*PlacedAt < 0.f) { *PlacedAt = T; }
			if (T - *PlacedAt < 1.2f)
			{
				if (T - *PlacedAt > 1.0f && !*DoorShot) { *DoorShot = true; Screenshot(TEXT("HeliPuerta")); }
				return;
			}
			C->SetInteractHeld(C->GetFocusedInteractable() != nullptr);
		},
		[this, Director, Heli](FString& D)
		{
			const ABLMissionDirector* M = Director();
			const ABLHelicopter* H = Heli();
			D = FString::Printf(TEXT("en tierra=%d, dentro de la cabina=%d, objetivos pendientes=%d"), H && H->IsLanded(), Char()->IsInVehicleSeat(), M && M->GetCurrentObjective() ? 1 : 0);
			return H && H->IsLanded() && Char()->IsInVehicleSeat() && M && !M->GetCurrentObjective();
		}, nullptr, -1.f };
	Board.Done = [Director]() { return Director() && !Director()->GetCurrentObjective(); };
	Steps.Add(Board);

	FStep Done{ TEXT("Completada"), 60.0f, nullptr, nullptr,
		[Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			D = M ? FString::Printf(TEXT("completada=%d tiempo %.0f s, bajas %d, caidas %d"), M->IsMissionComplete(), M->GetMissionTime(), M->GetKills(), M->GetDeaths()) : TEXT("sin director");
			return M && M->IsMissionComplete();
		}, nullptr, -1.f };
	Done.Done = [Director]() { return Director() && Director()->IsMissionComplete() && Director()->GetCompleteAge() > 2.5f; };
	Steps.Add(Done);
	Steps.Add({ TEXT("Resumen"), 0.6f, nullptr, nullptr, [](FString& D) { D = TEXT("captura"); return true; }, nullptr, 0.3f });
}
