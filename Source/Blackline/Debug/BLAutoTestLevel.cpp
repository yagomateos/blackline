// Prueba automática "Level" (Bloque 4): mapa /Game/Maps/M01/L_M01_AmanecerRoto.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Mission/BLCheckpointSubsystem.h"
#include "Mission/BLCheckpointVolume.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "EngineUtils.h"
#include "Player/BLCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

namespace
{
	FVector StartLocation(UWorld* World, const FString& Name)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(World, FName(*(TEXT("BLTest_Start_") + Name)), Found);
		return Found.Num() > 0 ? Found[0]->GetActorLocation() : FVector::ZeroVector;
	}
}

void UBLAutoTestComponent::BuildLevelTest()
{
	// Esta prueba es de geometría y navegación: sin enemigos (la IA tiene su propia prueba)
	Steps.Add({ TEXT("SinIA"), 0.2f,
		[this]()
		{
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (AController* C = It->GetController()) { C->Destroy(); }
				It->Destroy();
			}
			Char()->GetHealth()->bInvulnerable = true;
		}, nullptr, [](FString& D) { D = TEXT("enemigos retirados"); return true; } });

	// ---- Navegación guardada en el mapa: ruta completa del furgón al objetivo ----
	Steps.Add({ TEXT("Navegacion"), 0.3f, nullptr, nullptr,
		[this](FString& D)
		{
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			const FVector A = StartLocation(GetWorld(), TEXT("Fase1")) + FVector(0.f, 0.f, 90.f);
			const FVector B = StartLocation(GetWorld(), TEXT("Objetivo")) + FVector(0.f, 0.f, 90.f);
			UNavigationPath* Path = Nav ? Nav->FindPathToLocationSynchronously(GetWorld(), A, B) : nullptr;
			const int32 N = Path ? Path->PathPoints.Num() : 0;
			const bool bPartial = Path && Path->IsPartial();
			D = FString::Printf(TEXT("ruta furgon -> objetivo: %d puntos, %.0f m, parcial=%d"), N, Path ? Path->GetPathLength() / 100.f : 0.f, bPartial);
			return N > 2 && !bPartial;
		} });

	// ---- Recorrido a pie (sprint) siguiendo la ruta de navegación: el nivel se puede atravesar y se cruzan los checkpoints ----
	struct FWalk
	{
		TArray<FVector> Points;
		int32 Next = 1;
		FVector LastCheckPos = FVector::ZeroVector;
		float LastCheckTime = 0.f;
		bool bStuck = false;
		bool bArrived = false;
		TArray<FName> Checkpoints;
		float Distance = 0.f;
		FVector PrevPos = FVector::ZeroVector;
	};
	TSharedRef<FWalk> Walk = MakeShared<FWalk>();
	FStep WalkStep{ TEXT("Recorrido"), 120.f,
		[this, Walk]()
		{
			*Walk = FWalk();
			TeleportToStart(TEXT("Fase1"));
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			const FVector B = StartLocation(GetWorld(), TEXT("Objetivo")) + FVector(0.f, 0.f, 90.f);
			if (UNavigationPath* Path = Nav ? Nav->FindPathToLocationSynchronously(GetWorld(), Char()->GetActorLocation(), B) : nullptr)
			{
				Walk->Points = Path->PathPoints;
			}
			Walk->LastCheckPos = Walk->PrevPos = Char()->GetActorLocation();
			if (UBLCheckpointSubsystem* C = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>())
			{
				C->OnCheckpointReached.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleCheckpoint);
			}
			ReachedCheckpoints.Reset();
		},
		[this, Walk](float T)
		{
			if (Walk->bArrived || Walk->bStuck || !Walk->Points.IsValidIndex(Walk->Next))
			{
				Char()->SetSprintHeld(false);
				return;
			}
			const FVector Pos = Char()->GetActorLocation();
			Walk->Distance += FVector::Dist2D(Pos, Walk->PrevPos);
			Walk->PrevPos = Pos;
			// Avanzar al siguiente punto al llegar (o al pasarlo de largo)
			while (Walk->Points.IsValidIndex(Walk->Next) && FVector::Dist2D(Pos, Walk->Points[Walk->Next]) < 90.f)
			{
				++Walk->Next;
			}
			if (!Walk->Points.IsValidIndex(Walk->Next))
			{
				Walk->bArrived = true;
				return;
			}
			const FVector To = (Walk->Points[Walk->Next] - Pos).GetSafeNormal2D();
			if (AController* C = Char()->GetController())
			{
				C->SetControlRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
			}
			Char()->SetSprintHeld(true);
			Char()->DoMove(0.f, 1.f);
			// Atascado: menos de 60 cm en 3 s
			if (T - Walk->LastCheckTime > 3.f)
			{
				Walk->bStuck = FVector::Dist2D(Pos, Walk->LastCheckPos) < 60.f;
				Walk->LastCheckPos = Pos;
				Walk->LastCheckTime = T;
			}
		},
		[this, Walk](FString& D)
		{
			FString Cps;
			for (const FName& N : ReachedCheckpoints) { Cps += N.ToString() + TEXT(" "); }
			D = FString::Printf(TEXT("%s tras %.0f m (punto %d/%d) en %s; checkpoints: %s"), Walk->bArrived ? TEXT("llega al objetivo") : Walk->bStuck ? TEXT("ATASCADO") : TEXT("sin llegar"),
				Walk->Distance / 100.f, Walk->Next, Walk->Points.Num(), *Char()->GetActorLocation().ToCompactString(), *Cps);
			return Walk->bArrived && ReachedCheckpoints.Contains(FName("Infiltracion")) && ReachedCheckpoints.Contains(FName("Contacto"))
				&& ReachedCheckpoints.Contains(FName("Calle")) && ReachedCheckpoints.Contains(FName("Objetivo"));
		},
		[this]() { Char()->SetSprintHeld(false); }, 30.f };
	WalkStep.Done = [Walk]() { return Walk->bArrived || Walk->bStuck; };
	Steps.Add(WalkStep);

	// ---- Vistas de cada fase: captura (con Lumen ya asentado) y fps medios tras 1 s ----
	struct FView { FString Name; float Pitch; };
	for (const FView& V : { FView{ TEXT("Fase1"), -2.f }, FView{ TEXT("Fase2"), -3.f }, FView{ TEXT("Fase3"), -2.f }, FView{ TEXT("Fase4"), -2.f }, FView{ TEXT("Objetivo"), -6.f } })
	{
		TSharedRef<FVector2D> Acc = MakeShared<FVector2D>(0.f, 0.f);   // (suma de dt, frames)
		Steps.Add({ TEXT("Vista") + V.Name, 3.0f,
			[this, V, Acc]()
			{
				*Acc = FVector2D::ZeroVector;
				TeleportToStart(V.Name);
				if (AController* C = Char()->GetController())
				{
					FRotator R = C->GetControlRotation();
					R.Pitch = V.Pitch;
					C->SetControlRotation(R);
				}
			},
			[this, Acc](float T)
			{
				if (T > 1.0f)
				{
					Acc->X += GetWorld()->GetDeltaSeconds();
					Acc->Y += 1.f;
				}
			},
			[Acc](FString& D)
			{
				const float Ms = Acc->Y > 0.f ? Acc->X / Acc->Y * 1000.f : 999.f;
				D = FString::Printf(TEXT("%.2f ms (%.0f fps) [objetivo 60 fps a 1080p, minimo 45]"), Ms, 1000.f / Ms);
				return Ms < 1000.f / 45.f;
			}, nullptr, 2.6f });
	}
}

void UBLAutoTestComponent::HandleCheckpoint(FName Id)
{
	ReachedCheckpoints.AddUnique(Id);
}
