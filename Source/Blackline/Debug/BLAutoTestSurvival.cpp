// Prueba automática "Aguante" (mapa L_Dev_Movement, zona despejada x 4000): cuánto aguanta el jugador quieto y a la
// vista ante milicianos con su IA normal (detectan, apuntan, disparan por turnos). Mide, en dificultad Veterano:
//  - cuándo recibe el primer impacto (tiempo de reacción que tiene el jugador),
//  - cuánto tarda en acumular 100 de daño desde el primer impacto (lo que tarda en morir si no se cubre),
//  - disparos e impactos de la IA (puntería real).
// La salud se infla para medir sin morir; el daño que cuenta es el ya reducido por la dificultad.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Player/BLCharacter.h"
#include "UI/BLUserSettings.h"
#include "Weapons/BLWeaponComponent.h"

#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildSurvivalTest()
{
	struct FState
	{
		TArray<TWeakObjectPtr<ABLEnemyCharacter>> Enemies;
		float FirstHit = -1.f;
		float Dead100 = -1.f;
		int32 Hits = 0;
		float LastHealth = 0.f;
		int32 EnemyShots0 = 0;
	};
	TSharedRef<FState> St = MakeShared<FState>();
	const float Big = 100000.f;
	UBLHealthComponent* H = Char()->GetHealth();
	H->MaxHealth = Big;
	H->RegenRate = 0.f;

	auto EnemyShots = [St]()
	{
		int32 N = 0;
		for (const TWeakObjectPtr<ABLEnemyCharacter>& E : St->Enemies)
		{
			if (E.IsValid()) { N += E->GetWeapon()->GetShotsFired(); }
		}
		return N;
	};

	auto AddCase = [this, St, H, Big, EnemyShots](int32 Count, float Meters)
	{
		const FString Name = FString::Printf(TEXT("%dx%.0fm"), Count, Meters);
		Steps.Add({ Name, 25.f,
			[this, St, H, Big, Count, Meters]()
			{
				ABLCharacter* C = Char();
				const FVector Spot(4000.f, -4200.f, 0.f);
				C->GetCharacterMovement()->StopMovementImmediately();
				C->SetActorLocation(Spot + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
				C->GetController()->SetControlRotation(FRotator(0.f, 90.f, 0.f));
				H->ResetHealth(Big);
				St->LastHealth = Big;
				St->FirstHit = St->Dead100 = -1.f;
				St->Hits = 0;
				St->Enemies.Reset();
				for (int32 i = 0; i < Count; ++i)
				{
					const FVector At = Spot + FVector((i - (Count - 1) * 0.5f) * 350.f, Meters * 100.f, 96.f);
					FActorSpawnParameters P;
					P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					if (ABLEnemyCharacter* E = GetWorld()->SpawnActor<ABLEnemyCharacter>(ABLEnemyCharacter::StaticClass(), At, FRotator(0.f, -90.f, 0.f), P))
					{
						St->Enemies.Add(E);
					}
				}
				St->EnemyShots0 = 0;
			},
			[this, St, H, Big](float T)
			{
				const float Health = H->GetHealth();
				if (Health < St->LastHealth - 0.01f)
				{
					++St->Hits;
					if (St->FirstHit < 0.f) { St->FirstHit = T; }
				}
				St->LastHealth = Health;
				if (St->Dead100 < 0.f && Big - Health >= 100.f) { St->Dead100 = T; }
			},
			[this, St, H, Big, EnemyShots, Count, Meters](FString& D)
			{
				const int32 Shots = EnemyShots();
				const float Taken = Big - H->GetHealth();
				const float TTK = St->Dead100 >= 0.f && St->FirstHit >= 0.f ? St->Dead100 - St->FirstHit : -1.f;
				D = FString::Printf(TEXT("primer impacto a %.1f s; muerte (100 de daño) %.1f s después del primer impacto; %d impactos de %d disparos (%.0f %%), daño total %.0f en %.0f s; daño recibido %.0f %% (%s)"),
					St->FirstHit, TTK, St->Hits, Shots, Shots > 0 ? 100.f * St->Hits / Shots : 0.f, Taken, 25.f,
					100.f * UBLUserSettings::PlayerDamageTaken(1), UBLUserSettings::DifficultyName(1));
				UE_LOG(LogBlackline, Display, TEXT("[BLTest] Aguante %dx%.0fm: TTK %.2f s"), Count, Meters, TTK);
				return St->FirstHit >= 0.f;
			},
			[St]()
			{
				for (const TWeakObjectPtr<ABLEnemyCharacter>& E : St->Enemies)
				{
					if (E.IsValid()) { E->Destroy(); }
				}
				St->Enemies.Reset();
			} });
	};
	AddCase(1, 20.f);
	AddCase(3, 20.f);
	AddCase(3, 40.f);
	AddCase(2, 30.f);

	// Feedback visual: salud baja (28) y un tirador a la derecha; el daño casi no baja la salud para poder ver los efectos
	TSharedRef<TWeakObjectPtr<ABLEnemyCharacter>> Shooter = MakeShared<TWeakObjectPtr<ABLEnemyCharacter>>();
	Steps.Add({ TEXT("Feedback"), 6.f,
		[this, H, Shooter]()
		{
			ABLCharacter* C = Char();
			const FVector Spot(4000.f, -4200.f, 0.f);
			C->SetActorLocation(Spot + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
			C->GetController()->SetControlRotation(FRotator(0.f, 90.f, 0.f));
			H->MaxHealth = 100.f;
			H->ResetHealth(28.f);
			H->DamageTakenMultiplier = 0.02f;
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			const FVector At = Spot + FRotator(0.f, 90.f + 60.f, 0.f).Vector() * 1500.f + FVector(0.f, 0.f, 96.f);
			*Shooter = GetWorld()->SpawnActor<ABLEnemyCharacter>(ABLEnemyCharacter::StaticClass(), At, (Spot - At).Rotation(), P);
		},
		[this, Shooter](float T)
		{
			if (T > 3.5f && !bScreenshotTaken && Char()->GetDamageIndicators().Num() > 0)
			{
				bScreenshotTaken = true;
				Screenshot(TEXT("Feedback"));
			}
		},
		[this](FString& D)
		{
			D = FString::Printf(TEXT("indicadores de dirección %d, destello %.2f, salud %.0f"), Char()->GetDamageIndicators().Num(), Char()->GetDamageFlash(), Char()->GetHealth()->GetHealth());
			return bScreenshotTaken;
		},
		[Shooter]() { if (Shooter->IsValid()) { (*Shooter)->Destroy(); } } });
}
