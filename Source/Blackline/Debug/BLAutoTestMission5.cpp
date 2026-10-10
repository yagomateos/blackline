// Pruebas automáticas de la misión 5 "Línea negra" (mapa /Game/Maps/M05/L_M05_LineaNegra):
//  - "Mission5": de principio a fin (equipo que sigue, terminal, tiempo límite, termita, descarga, el inglés huye,
//    helicóptero de Corvane inutilizado, rendición, detención, extracción, fin de campaña).
//  - "Mission5Fallo": condición de fallo (matar al inglés -> misión fallida) y el contador de tiempo.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAllyController.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLVIP.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"
#include "Vehicles/BLHelicopter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr float FH_TEST = 320.f;   // altura de planta de la capitanía (build_m05_terminal.py)
	ABLVIP* FindVIP(UWorld* W) { for (TActorIterator<ABLVIP> It(W); It; ++It) { return *It; } return nullptr; }
	ABLHelicopter* FindHeli(UWorld* W, FName Tag) { for (TActorIterator<ABLHelicopter> It(W); It; ++It) { if (It->Tags.Contains(Tag)) { return *It; } } return nullptr; }
}

void UBLAutoTestComponent::BuildMission5Test()
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
			if ((Squad.IsNone() ? It->Tags.Contains(FName("BLWaveEnemy")) : It->SquadId == Squad) && !It->IsDead())
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

	Steps.Add({ TEXT("Briefing"), 3.0f, nullptr, nullptr, [IndexIs](FString& D) { return IndexIs(0, D); }, nullptr, 2.5f });
	// El equipo sigue: el jugador avanza 20 m y los dos compañeros llegan detrás
	Steps.Add({ TEXT("Equipo"), 9.0f, [Place]() { Place(FVector(1700.f, 0.f, 0.f), 0.f); }, nullptr,   // dentro del radio de BLObj_Terminal (1500, 0; 5 m)
		[this, IndexIs](FString& D)
		{
			int32 Near = 0;
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				Near += It->bFollowPlayer && FVector::Dist(It->GetActorLocation(), Char()->GetActorLocation()) < 1000.f ? 1 : 0;
			}
			const bool bIdx = IndexIs(1, D);
			D += FString::Printf(TEXT(" | compañeros cerca %d/2"), Near);
			return bIdx && Near == 2;
		}, nullptr, 8.f });
	Steps.Add({ TEXT("Terminal"), 1.5f, [KillSquad]() { KillSquad(FName("Terminal")); }, nullptr, [IndexIs](FString& D) { return IndexIs(2, D); } });
	// Tiempo límite: el texto lleva la cuenta atrás; se llega antes de que acabe
	TSharedRef<FString> Shown = MakeShared<FString>();
	Steps.Add({ TEXT("Contrarreloj"), 3.0f, nullptr,
		[this, Shown](float T) { if (T > 1.5f && Shown->IsEmpty() && ABLMissionDirector::Get(this)) { *Shown = ABLMissionDirector::Get(this)->GetObjectiveDisplayText(); } },
		[Director, Shown](FString& D)
		{
			const float Left = Director() ? Director()->GetTimeLeft() : -1.f;
			D = FString::Printf(TEXT("\"%s\", quedan %.0f s"), **Shown, Left);
			return Left > 100.f && Left < 120.f && Shown->Contains(TEXT(":"));
		} });
	Steps.Add({ TEXT("Servidores"), 1.5f, [Place, KillSquad]() { KillSquad(FName("Capitania")); Place(FVector(13000.f, -1250.f, FH_TEST), -90.f); }, nullptr,
		[IndexIs](FString& D) { return IndexIs(3, D); } });
	Steps.Add({ TEXT("Termita"), 2.0f, [UseTagged]() { UseTagged(FName("BLObjective_Termita")); }, nullptr, [IndexIs](FString& D) { return IndexIs(4, D); } });
	FStep Download{ TEXT("Descarga"), 120.f, nullptr, [KillSquad](float T) { KillSquad(NAME_None); }, nullptr, nullptr, 20.f };
	Download.Verify = [IndexIs](FString& D) { return IndexIs(5, D); };
	Download.Done = [Director]() { return Director() && Director()->GetCurrentIndex() >= 5; };
	Steps.Add(Download);
	// La azotea: el inglés huye al helipuerto y llega el helicóptero de Corvane
	FStep Roof{ TEXT("Azotea"), 40.f, [Place]() { Place(FVector(13300.f, 500.f, 3.f * FH_TEST), -150.f, 10.f); }, nullptr,
		[this, IndexIs](FString& D)
		{
			const ABLVIP* V = FindVIP(GetWorld());
			const ABLHelicopter* H = FindHeli(GetWorld(), FName("BLHeliCorvane"));
			const bool bIdx = IndexIs(6, D);
			D += FString::Printf(TEXT(" | inglés en la azotea=%d, helicóptero en estacionario=%d"), V && V->GetActorLocation().Z > 3.f * FH_TEST, H && H->IsHovering());
			return bIdx && V && V->GetActorLocation().Z > 3.f * FH_TEST && H && H->IsHovering();
		}, nullptr, 25.f };
	Roof.Done = [this]() { const ABLVIP* V = FindVIP(GetWorld()); const ABLHelicopter* H = FindHeli(GetWorld(), FName("BLHeliCorvane")); return V && V->GetActorLocation().Z > 3.f * FH_TEST && H && H->IsHovering(); };
	Steps.Add(Roof);
	// A tiros contra el helicóptero hasta inutilizarlo -> aterrizaje forzoso y rendición
	FStep Heli{ TEXT("Helicoptero"), 20.f, nullptr,
		[this](float T)
		{
			if (ABLHelicopter* H = FindHeli(GetWorld(), FName("BLHeliCorvane")); H && T > 0.5f && !H->IsTargetDestroyed())
			{
				FPointDamageEvent Ev(150.f, FHitResult(), FVector(0.f, 0.f, 1.f), nullptr);
				H->TakeDamage(150.f, Ev, Char()->GetController(), Char());
			}
		},
		[this, IndexIs](FString& D)
		{
			const ABLVIPController* VC = FindVIP(GetWorld()) ? Cast<ABLVIPController>(FindVIP(GetWorld())->GetController()) : nullptr;
			const bool bIdx = IndexIs(7, D);
			D += FString::Printf(TEXT(" | rendido=%d"), VC && VC->HasSurrendered());
			return bIdx && VC && VC->HasSurrendered();
		}, nullptr, 6.f };
	Heli.Done = [this]() { const ABLVIP* V = FindVIP(GetWorld()); const ABLVIPController* VC = V ? Cast<ABLVIPController>(V->GetController()) : nullptr; return VC && VC->HasSurrendered(); };
	Steps.Add(Heli);
	Steps.Add({ TEXT("Detencion"), 2.0f, [UseTagged]() { UseTagged(FName("BLObjective_VIP")); }, nullptr, [IndexIs](FString& D) { return IndexIs(8, D); } });
	FStep Exfil{ TEXT("Extraccion"), 60.f, nullptr,
		[this, UseTagged](float T) { if (const ABLHelicopter* H = FindHeli(GetWorld(), FName("BLHeli")); H && H->IsLanded()) { UseTagged(FName("BLObjective_Heli")); } },
		[Director](FString& D) { D = FString::Printf(TEXT("pendientes=%d"), Director() && Director()->GetCurrentObjective() ? 1 : 0); return Director() && !Director()->GetCurrentObjective(); },
		nullptr, -1.f };
	Exfil.Done = [Director]() { return Director() && !Director()->GetCurrentObjective(); };
	Steps.Add(Exfil);
	FStep Done{ TEXT("FinCampana"), 40.0f, nullptr, nullptr,
		[Director](FString& D)
		{
			const ABLMissionDirector* M = Director();
			D = M ? FString::Printf(TEXT("completada=%d, final de campaña=%d, fallida=%d"), M->IsMissionComplete(), M->bCampaignFinale, M->IsMissionFailed()) : TEXT("-");
			return M && M->IsMissionComplete() && M->bCampaignFinale && !M->IsMissionFailed();
		} };
	Done.Done = [Director]() { return Director() && Director()->IsMissionComplete() && Director()->GetCompleteAge() > 2.5f; };
	Steps.Add(Done);
	Steps.Add({ TEXT("Resumen"), 0.6f, nullptr, nullptr, [](FString& D) { D = TEXT("captura"); return true; }, nullptr, 0.3f });
}

void UBLAutoTestComponent::BuildMission5FailTest()
{
	// Lanzar con -BLStart=Azotea: si el jugador mata al inglés, la misión falla (y se recarga la fase)
	Char()->GetHealth()->bInvulnerable = true;
	Steps.Add({ TEXT("Arranque"), 3.0f, nullptr, nullptr,
		[this](FString& D)
		{
			const ABLMissionDirector* M = ABLMissionDirector::Get(this);
			D = FString::Printf(TEXT("objetivo %d"), M ? M->GetCurrentIndex() + 1 : 0);
			return M && M->GetCurrentIndex() == 5 && FindVIP(GetWorld());
		} });
	Steps.Add({ TEXT("DisparoAjeno"), 1.0f,
		[this]()
		{
			// Un tiro de alguien que no es el jugador no le hace nada (no se falla por fuego cruzado)
			if (ABLVIP* V = FindVIP(GetWorld()))
			{
				FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
				V->TakeDamage(500.f, Ev, nullptr, nullptr);
			}
		}, nullptr,
		[this](FString& D) { const ABLVIP* V = FindVIP(GetWorld()); D = FString::Printf(TEXT("vivo=%d"), V && !V->IsDead()); return V && !V->IsDead(); } });
	Steps.Add({ TEXT("Muerte"), 2.0f,
		[this]()
		{
			if (ABLVIP* V = FindVIP(GetWorld()))
			{
				FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
				V->TakeDamage(500.f, Ev, Char()->GetController(), Char());
			}
		}, nullptr,
		[this](FString& D)
		{
			const ABLMissionDirector* M = ABLMissionDirector::Get(this);
			D = M ? FString::Printf(TEXT("fallida=%d: %s"), M->IsMissionFailed(), *M->GetFailReason()) : TEXT("-");
			return M && M->IsMissionFailed();
		}, nullptr, 1.5f });
}
