// Pruebas automáticas de la granada M-6 (Bloque 11).
//  "Grenade"   (mapa L_Dev_Movement, estación "Dianas"): lanzar, rebotar, explotar, efectos, daño con caída, daño al jugador.
//  "GrenadeAI" (mapa de la misión): el miliciano que la ve grita "¡Granada!" y huye.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "Audio/BLAudioSubsystem.h"
#include "Combat/BLHealthComponent.h"
#include "Combat/BLTargetDummy.h"
#include "FX/BLFXSubsystem.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLGrenade.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	ABLTargetDummy* GrenadeDummy(UWorld* World, int32 Index)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(World, FName(*FString::Printf(TEXT("BLDummy_%d"), Index)), Found);
		return Found.Num() > 0 ? Cast<ABLTargetDummy>(Found[0]) : nullptr;
	}

	ABLGrenade* DropGrenade(UWorld* World, const FVector& Loc, AController* By, float Fuse)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.Instigator = By ? By->GetPawn() : nullptr;
		ABLGrenade* G = World->SpawnActor<ABLGrenade>(ABLGrenade::StaticClass(), Loc, FRotator::ZeroRotator, P);
		if (G)
		{
			G->Launch(FVector::ZeroVector, By, Fuse);
		}
		return G;
	}
}

void UBLAutoTestComponent::BuildGrenadeTest()
{
	struct FState
	{
		int32 Explosions = 0;
		int32 MaxBounces = 0;
		int32 MaxSprites = 0;
		float H0 = 0.f, H1 = 0.f, HPlayer = 0.f;
		float D0 = 0.f, D1 = 0.f;
	};
	TSharedRef<FState> St = MakeShared<FState>();
	Char()->GetHealth()->bInvulnerable = true;

	// Daño con caída: granada pegada a la diana 0; la 1 está más lejos (debe recibir menos o nada)
	Steps.Add({ TEXT("DanoCaida"), 1.6f,
		[this, St]()
		{
			TeleportToStart(TEXT("Dianas"));
			ABLTargetDummy* A = GrenadeDummy(GetWorld(), 1);
			ABLTargetDummy* B = GrenadeDummy(GetWorld(), 3);
			if (!A || !B) { return; }
			St->H0 = A->GetHealth()->GetHealth();
			St->H1 = B->GetHealth()->GetHealth();
			St->D0 = 110.f;
			St->D1 = FVector::Dist(A->GetActorLocation(), B->GetActorLocation());
			const FVector Away = (A->GetActorLocation() - B->GetActorLocation()).GetSafeNormal2D();
			DropGrenade(GetWorld(), A->GetActorLocation() + Away * St->D0 + FVector(0.f, 0.f, -60.f), Char()->GetController(), 0.4f);
		}, nullptr,
		[this, St](FString& D)
		{
			const ABLTargetDummy* A = GrenadeDummy(GetWorld(), 1);
			const ABLTargetDummy* B = GrenadeDummy(GetWorld(), 3);
			const float Da = A ? St->H0 - A->GetHealth()->GetHealth() : 0.f;
			const float Db = B ? St->H1 - B->GetHealth()->GetHealth() : 0.f;
			const bool bDeadA = A && A->GetHealth()->IsDead();
			D = FString::Printf(TEXT("diana a %.0f cm: daño %.0f%s; diana a %.0f cm: daño %.0f"), St->D0, Da, bDeadA ? TEXT(" (baja)") : TEXT(""), St->D1 + St->D0, Db);
			return A && B && (Da >= 60.f || bDeadA) && Db < Da;
		} });

	Steps.Add({ TEXT("Lanzar"), 0.8f,
		[this, St]()
		{
			TeleportToStart(TEXT("Dianas"));
			Char()->SetGrenades(2);
			St->Explosions = ABLGrenade::GetExplosionCount();
			if (AController* C = Char()->GetController()) { FRotator R = C->GetControlRotation(); R.Pitch = -12.f; C->SetControlRotation(R); }
			Char()->ThrowGrenade();
		}, nullptr,
		[this](FString& D)
		{
			const ABLGrenade* G = Char()->GetLastGrenade();
			D = FString::Printf(TEXT("granadas %d (esperado 1), granada en vuelo=%d"), Char()->GetGrenades(), G != nullptr);
			return Char()->GetGrenades() == 1 && G != nullptr;
		} });

	FStep Boom{ TEXT("Explota"), 5.0f, nullptr,
		[this, St](float)
		{
			if (const ABLGrenade* G = Char()->GetLastGrenade()) { St->MaxBounces = FMath::Max(St->MaxBounces, G->GetBounces()); }
			if (const UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>()) { St->MaxSprites = FMath::Max(St->MaxSprites, FX->GetActiveSprites()); }
		},
		[St](FString& D)
		{
			const int32 N = ABLGrenade::GetExplosionCount() - St->Explosions;
			D = FString::Printf(TEXT("explosiones %d, rebotes %d, polvo/humo %d sprites"), N, St->MaxBounces, St->MaxSprites);
			return N == 1 && St->MaxBounces >= 1 && St->MaxSprites >= 20;
		}, nullptr, 3.9f };
	Boom.Done = [St]() { return ABLGrenade::GetExplosionCount() > St->Explosions && St->MaxSprites >= 20; };
	Steps.Add(Boom);
	Steps.Add({ TEXT("Captura"), 0.5f, nullptr, nullptr, [](FString& D) { D = TEXT("captura de la explosión"); return true; }, nullptr, 0.15f });

	// Al jugador también le hace daño (a 3 m)
	Steps.Add({ TEXT("DanoJugador"), 1.4f,
		[this, St]()
		{
			UBLHealthComponent* H = Char()->GetHealth();
			H->bInvulnerable = false;
			H->DamageTakenMultiplier = 1.f;
			St->HPlayer = H->GetHealth();
			DropGrenade(GetWorld(), Char()->GetActorLocation() + Char()->GetActorForwardVector() * 300.f, nullptr, 0.3f);
			H->DamageTakenMultiplier = 0.55f;   // la dificultad actual del juego
		}, nullptr,
		[this, St](FString& D)
		{
			UBLHealthComponent* H = Char()->GetHealth();
			const float Lost = St->HPlayer - H->GetHealth();
			D = FString::Printf(TEXT("salud %.0f -> %.0f"), St->HPlayer, H->GetHealth());
			H->bInvulnerable = true;
			return Lost > 5.f && !H->IsDead();   // a 3 m hiere mucho pero no mata
		} });

	Steps.Add({ TEXT("SinGranadas"), 0.5f,
		[this]() { Char()->SetGrenades(0); Char()->ThrowGrenade(); }, nullptr,
		[this](FString& D)
		{
			D = FString::Printf(TEXT("lanzando=%d con 0 granadas"), Char()->IsThrowingGrenade());
			return !Char()->IsThrowingGrenade();
		} });
}

void UBLAutoTestComponent::BuildGrenadeAITest()
{
	Char()->GetHealth()->bInvulnerable = true;
	struct FState { TWeakObjectPtr<ABLEnemyCharacter> Enemy; FVector Start = FVector::ZeroVector; int32 Barks = 0; };
	TSharedRef<FState> St = MakeShared<FState>();

	Steps.Add({ TEXT("Granada"), 1.5f,
		[this, St]()
		{
			ABLCharacter* C = Char();
			C->GetCharacterMovement()->StopMovementImmediately();
			C->SetActorLocation(FVector(9600.f, -4000.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(FName("BLEnemy_Control_Caseta"))) { St->Enemy = *It; }
			}
			if (ABLEnemyCharacter* E = St->Enemy.Get())
			{
				St->Start = E->GetActorLocation();
				St->Barks = UBLAudioSubsystem::Get(this) ? UBLAudioSubsystem::Get(this)->GetBarksPlayed(EBLBark::Grenade) : 0;
				DropGrenade(GetWorld(), St->Start + FVector(90.f, 40.f, -40.f), C->GetController(), 2.4f);
			}
		}, nullptr,
		[this, St](FString& D)
		{
			const ABLEnemyCharacter* E = St->Enemy.Get();
			const ABLAIController* AI = E ? Cast<ABLAIController>(E->GetController()) : nullptr;
			const UBLAudioSubsystem* A = UBLAudioSubsystem::Get(this);
			const int32 Barks = A ? A->GetBarksPlayed(EBLBark::Grenade) - St->Barks : 0;
			D = FString::Printf(TEXT("huidas %d, huyendo=%d, grito ¡Granada! %d"), AI ? AI->GetGrenadeEscapes() : 0, AI && AI->IsEscapingGrenade(), Barks);
			return AI && AI->GetGrenadeEscapes() >= 1 && Barks >= 1;
		} });

	Steps.Add({ TEXT("Aleja"), 1.6f, nullptr, nullptr,
		[St](FString& D)
		{
			const ABLEnemyCharacter* E = St->Enemy.Get();
			const float Moved = E ? FVector::Dist2D(E->GetActorLocation(), St->Start) : 0.f;
			D = FString::Printf(TEXT("se aleja %.0f cm de la granada antes de que explote; vivo=%d"), Moved, E && !E->IsDead());
			return E && Moved >= 250.f;
		} });
}
