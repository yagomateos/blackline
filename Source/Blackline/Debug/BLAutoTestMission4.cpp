// Prueba automática "Mission4" (misión 4 "Fuego cruzado"): mapa /Game/Maps/M04/L_M04_FuegoCruzado.
// Puesto bajo mortero, defensa con aliados (enlace %), ametralladora montada (montar, disparar, calor, bajarse),
// carga sobre el puente, blindado marcado y destruido por los cazas, cargas, voladura del tramo, vado, retirada.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAllyController.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"
#include "Mission/BLMortarBarrage.h"
#include "Mission/BLScriptedMover.h"
#include "Mission/BLStrikeDesignator.h"
#include "Player/BLCharacter.h"
#include "Vehicles/BLBTR.h"
#include "Weapons/BLMountedGun.h"

#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildMission4Test()
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
	auto KillWaves = [this](float Grace)
	{
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (It->Tags.Contains(FName("BLWaveEnemy")) && !It->IsDead() && It->GetGameTimeSinceCreation() > Grace)
			{
				FPointDamageEvent Ev(2000.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
				It->TakeDamage(2000.f, Ev, Char()->GetController(), Char());
			}
		}
	};
	auto UseTagged = [this](FName Tag)
	{
		for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
		{
			if (It->Tags.Contains(Tag) && It->CanInteract(Char()))
			{
				It->Use(Char());
			}
		}
	};
	// Una pulsación de F (pulsar un frame, soltar el siguiente), como el jugador; mantenerla montaría y desmontaría
	auto PressF = [this](TSharedRef<int32> St, float T)
	{
		// Un solo flanco de subida (si se repitiera, montado bajaría) y 0,3 s pulsado: soltarlo al frame siguiente
		// dependía del orden de tick entre la prueba y el personaje
		if (T > 0.3f && *St == 0) { Char()->SetInteractHeld(true); *St = 1; }
		else if (T > 0.6f && *St == 1) { Char()->SetInteractHeld(false); *St = 2; }
	};
	auto Gun = [this]() -> ABLMountedGun* { for (TActorIterator<ABLMountedGun> It(GetWorld()); It; ++It) { return *It; } return nullptr; };
	auto BTR = [this]() -> ABLBTR* { for (TActorIterator<ABLBTR> It(GetWorld()); It; ++It) { return *It; } return nullptr; };

	Steps.Add({ TEXT("Briefing"), 3.0f, nullptr, nullptr, [IndexIs](FString& D) { return IndexIs(0, D); }, nullptr, 2.5f });
	Steps.Add({ TEXT("Puesto"), 4.0f, [Place]() { Place(FVector(-900.f, 900.f, 0.f), 0.f); }, nullptr,
		[this, IndexIs](FString& D)
		{
			int32 Shells = 0;
			for (TActorIterator<ABLMortarBarrage> It(GetWorld()); It; ++It) { Shells += It->GetImpacts(); }
			const bool bIdx = IndexIs(1, D);
			D += FString::Printf(TEXT(" | morteros caídos %d"), Shells);
			return bIdx && Shells >= 1;
		}, nullptr, 3.f });

	// Defensa con los aliados disparando; el texto lleva el % del enlace
	TSharedRef<int32> AllyShots = MakeShared<int32>(0);
	TSharedRef<FString> Shown = MakeShared<FString>();
	FStep Def1{ TEXT("Defensa"), 120.f, [Place]() { Place(FVector(-500.f, 0.f, 0.f), 0.f); },
		[this, KillWaves, AllyShots, Shown](float T)
		{
			int32 N = 0;
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (const ABLAllyController* A = Cast<ABLAllyController>(It->GetController())) { N += A->GetShotsFired(); }
			}
			*AllyShots = FMath::Max(*AllyShots, N);
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this); M && M->GetCurrentIndex() == 1 && T > 10.f && Shown->IsEmpty())
			{
				*Shown = M->GetObjectiveDisplayText();
			}
			KillWaves(9.f);     // los deja cruzar un rato para que les disparen los aliados
		}, nullptr, nullptr, 20.f };
	Def1.Verify = [this, Director, AllyShots, Shown](FString& D)
	{
		const ABLMissionDirector* M = Director();
		int32 Allies = 0, WithTarget = 0;
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (const ABLAllyController* A = Cast<ABLAllyController>(It->GetController())) { ++Allies; WithTarget += A->GetTarget() != nullptr; }
		}
		D = FString::Printf(TEXT("aliados dispararon %d (aliados %d, con blanco %d), texto \"%s\", objetivo %d"), *AllyShots, Allies, WithTarget,
			**Shown, M ? M->GetCurrentIndex() + 1 : 0);
		return M && M->GetCurrentIndex() == 2 && *AllyShots >= 10 && Shown->Contains(TEXT("enlace"));
	};
	Def1.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 2; };
	Steps.Add(Def1);

	// Ametralladora: montarla como el jugador (mirarla y F), disparar, calor, y la carga sobre el puente
	Steps.Add({ TEXT("Montar"), 1.5f,
		[this, Place, Gun]()
		{
			if (const ABLMountedGun* G = Gun())
			{
				const FVector C = G->GetInteractLocation();
				const FVector From = G->GetSeatLocation() - G->GetActorForwardVector() * 30.f;
				Place(From, (C - From).Rotation().Yaw);
				// La inclinación, desde los ojos (From está a ras de suelo: desde ahí miraba 40° por encima del arma)
				const FVector Eye = From + FVector(0.f, 0.f, Char()->GetPawnViewLocation().Z - Char()->GetActorLocation().Z
					+ Char()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f);
				if (AController* PC = Char()->GetController()) { PC->SetControlRotation((C - Eye).Rotation()); }
			}
		},
		[PressF, St = MakeShared<int32>(0)](float T) { PressF(St, T); },
		[this, IndexIs](FString& D)
		{
			const bool bIdx = IndexIs(3, D);
			const ABLInteractable* F = Char()->GetFocusedInteractable();
			D += FString::Printf(TEXT(" | montado=%d, enfocado %s"), Char()->IsMounted(), F ? *F->GetName() : TEXT("ninguno"));
			return bIdx && Char()->IsMounted();
		}, [this]() { Char()->SetInteractHeld(false); } });
	TSharedRef<float> MaxHeat = MakeShared<float>(0.f);
	FStep Charge{ TEXT("CargaMG"), 120.f, nullptr,
		[this, KillWaves, Gun, MaxHeat](float T)
		{
			// Ráfagas largas (4 s y 1 s de pausa): con ráfagas cortas no llega a calentarse, que es lo que se busca
			Char()->SetFireHeld(FMath::Fmod(T, 5.f) < 4.f);
			if (const ABLMountedGun* G = Gun()) { *MaxHeat = FMath::Max(*MaxHeat, G->GetHeat()); }
			KillWaves(5.f);
		}, nullptr, [this]() { Char()->SetFireHeld(false); }, 8.f };
	Charge.Verify = [Director, Gun, MaxHeat](FString& D)
	{
		const ABLMountedGun* G = Gun();
		D = FString::Printf(TEXT("disparos de la ametralladora %d, calor máx %.2f, objetivo %d"), G ? G->GetShotsFired() : 0, *MaxHeat,
			Director() ? Director()->GetCurrentIndex() + 1 : 0);
		return G && G->GetShotsFired() >= 40 && *MaxHeat > 0.3f && Director() && Director()->GetCurrentIndex() == 4;
	};
	Charge.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 4; };
	Steps.Add(Charge);
	Steps.Add({ TEXT("Bajarse"), 1.0f, nullptr, [PressF, St = MakeShared<int32>(0)](float T) { PressF(St, T); },
		[this](FString& D) { D = FString::Printf(TEXT("montado=%d"), Char()->IsMounted()); return !Char()->IsMounted(); },
		[this]() { Char()->SetInteractHeld(false); } });

	// Blindado en el puente -> marcarlo -> los cazas -> destruido
	FStep Armor{ TEXT("Blindado"), 60.f, [Place]() { Place(FVector(-600.f, -1700.f, 0.f), 10.f); },
		[UseTagged, BTR](float T) { if (BTR() && BTR()->HasArrived()) { UseTagged(FName("BLObjective_Designador")); } },
		[BTR, Director](FString& D)
		{
			const ABLBTR* B = BTR();
			D = FString::Printf(TEXT("blindado en el puente=%d destruido=%d, objetivo %d"), B && B->HasArrived(), B && B->IsVehicleDestroyed(),
				Director() ? Director()->GetCurrentIndex() + 1 : 0);
			return B && B->IsVehicleDestroyed() && Director() && Director()->GetCurrentIndex() == 5;
		}, nullptr, -1.f };
	Armor.Done = [BTR]() { return BTR() && BTR()->IsVehicleDestroyed(); };
	Steps.Add(Armor);
	Steps.Add({ TEXT("Cargas"), 2.0f, [UseTagged]() { UseTagged(FName("BLObjective_Carga")); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(6, D); } });
	auto Span = [this]() -> ABLScriptedMover* { for (TActorIterator<ABLScriptedMover> It(GetWorld()); It; ++It) { if (It->Tags.Contains(FName("BLBridge"))) { return *It; } } return nullptr; };
	Steps.Add({ TEXT("Voladura"), 4.0f, [Place, UseTagged]() { Place(FVector(-600.f, -200.f, 0.f), 0.f); UseTagged(FName("BLObjective_Detonador")); }, nullptr,
		[Span, IndexIs](FString& D)
		{
			const bool bIdx = IndexIs(7, D);
			const ABLScriptedMover* S = Span();
			D += FString::Printf(TEXT(" | tramo cayendo=%d (z %.0f)"), S && S->HasFallen(), S ? S->GetActorLocation().Z : 0.f);
			return bIdx && S && S->HasFallen() && S->GetActorLocation().Z < -100.f;
		}, nullptr, 2.5f });
	FStep Ford{ TEXT("Vado"), 120.f, [Place]() { Place(FVector(-1200.f, -1500.f, 0.f), -120.f); },
		[KillWaves](float T) { KillWaves(4.f); }, [IndexIs](FString& D) { return IndexIs(8, D); }, nullptr, 15.f };
	Ford.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 8; };
	Steps.Add(Ford);
	Steps.Add({ TEXT("Retirada"), 1.5f, [Place]() { Place(FVector(-6200.f, -1100.f, 0.f), 180.f); }, nullptr,
		[Director](FString& D) { D = FString::Printf(TEXT("objetivos pendientes=%d"), Director() && Director()->GetCurrentObjective() ? 1 : 0); return Director() && !Director()->GetCurrentObjective(); } });
	FStep Done{ TEXT("Completada"), 40.0f, nullptr, nullptr,
		[Director](FString& D) { D = Director() ? FString::Printf(TEXT("completada=%d"), Director()->IsMissionComplete()) : TEXT("-"); return Director() && Director()->IsMissionComplete(); } };
	Done.Done = [Director]() { return Director() && Director()->IsMissionComplete() && Director()->GetCompleteAge() > 2.5f; };
	Steps.Add(Done);
	Steps.Add({ TEXT("Resumen"), 0.6f, nullptr, nullptr, [](FString& D) { D = TEXT("captura"); return true; }, nullptr, 0.3f });
}
