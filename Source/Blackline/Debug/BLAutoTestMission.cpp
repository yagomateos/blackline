// Prueba automática "Mission" (Bloque 6): mapa /Game/Maps/M01/L_M01_AmanecerRoto.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLInteractable.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"

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
				const FVector Disk = It->GetActorLocation();
				const FVector From(Disk.X - 110.f, Disk.Y - 30.f, 0.f);
				Place(From, (Disk - From).Rotation().Yaw, -32.f);
			}
		},
		[this](float T) { Char()->SetInteractHeld(T > 0.3f && T < 2.0f); },
		[this](FString& D)
		{
			bool bUsed = false;
			for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It) { bUsed |= It->IsUsed(); }
			const ABLMissionDirector* M = ABLMissionDirector::Get(this);
			D = FString::Printf(TEXT("disco recogido=%d, objetivos pendientes=%d"), bUsed, M && M->GetCurrentObjective() ? 1 : 0);
			return bUsed && M && !M->GetCurrentObjective();
		}, [this]() { Char()->SetInteractHeld(false); }, 0.9f });

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
