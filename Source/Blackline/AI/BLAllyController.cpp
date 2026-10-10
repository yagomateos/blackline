#include "AI/BLAllyController.h"

#include "AI/BLEnemyCharacter.h"
#include "Weapons/BLWeaponComponent.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

void ABLAllyController::TickFollow(ABLEnemyCharacter* Me, float DeltaTime)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}
	const float Dist = FVector::Dist(Me->GetActorLocation(), Player->GetActorLocation());
	// Muy lejos (checkpoint, caída): aparece detrás del jugador, como Varek en la misión 1
	if (Dist > 3500.f)
	{
		if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation Out;
			const FVector Side = Player->GetActorRightVector() * (Me->GetVoiceIndex() % 2 ? 150.f : -150.f);
			if (Nav->ProjectPointToNavigation(Player->GetActorLocation() - Player->GetActorForwardVector() * 300.f + Side, Out, FVector(300.f, 300.f, 300.f)))
			{
				Me->SetActorLocation(Out.Location + FVector(0.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
				StopMovement();
			}
		}
		return;
	}
	FollowTimer -= DeltaTime;
	if (Dist > 700.f && FollowTimer <= 0.f)
	{
		FollowTimer = 0.6f;
		Me->SetJog(Dist > 1200.f);
		MoveToActor(Player, 380.f, true, true, true, nullptr, true);
	}
	else if (Dist < 380.f && GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		StopMovement();
	}
}

AActor* ABLAllyController::GetTarget() const
{
	return Target.Get();
}

ABLEnemyCharacter* ABLAllyController::Body() const
{
	return Cast<ABLEnemyCharacter>(GetPawn());
}

bool ABLAllyController::CanSee(const ABLEnemyCharacter* Me, const AActor* Other) const
{
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLAllySight), false, Me);
	Q.AddIgnoredActor(Other);
	// Desde la altura de los ojos de pie (lo que verá al asomarse): agachado tras los sacos entre ráfagas no ve nada,
	// y si mirara desde ahí soltaría el blanco en cada pausa y la puntería no mejoraría nunca
	const FVector Eye = Me->GetActorLocation() + FVector(0.f, 0.f, 63.f);   // = EyeStand de ABLEnemyCharacter
	return !GetWorld()->LineTraceTestByChannel(Eye, Other->GetActorLocation() + FVector(0.f, 0.f, 40.f), ECC_Visibility, Q);
}

void ABLAllyController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ABLEnemyCharacter* Me = Body();
	if (!Me || Me->IsDead())
	{
		return;
	}
	UBLWeaponComponent* W = Me->GetWeapon();
	if (Me->bFollowPlayer)
	{
		TickFollow(Me, DeltaTime);
	}
	// Elegir blanco cada 0,4 s: el miliciano visible más cercano
	ScanTimer -= DeltaTime;
	if (ScanTimer <= 0.f)
	{
		ScanTimer = 0.4f;
		ABLEnemyCharacter* Best = nullptr;
		float BestDist = Range;
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			// Ni aliados ni civiles/el objetivo que hay que coger vivo
			if (*It == Me || It->IsDead() || It->EnemyRole == EBLEnemyRole::Ally || It->Tags.Contains(FName("BLVIP")) || It->Tags.Contains(FName("BLVarek")))
			{
				continue;
			}
			const float D = FVector::Dist(It->GetActorLocation(), Me->GetActorLocation());
			if (D < BestDist && CanSee(Me, *It))
			{
				Best = *It;
				BestDist = D;
			}
		}
		if (Best != Target.Get())
		{
			Target = Best;
			AimError = AimErrorStart;
		}
	}
	const ABLEnemyCharacter* T = Target.Get();
	if (!T || T->IsDead())
	{
		W->SetTriggerHeld(false);
		Me->SetCrouchTarget(false);
		Me->SetAim(Me->GetActorLocation() + Me->GetActorForwardVector() * 1000.f, false);
		return;
	}
	// Puntería que mejora mientras sigue al mismo blanco
	AimError = FMath::FInterpTo(AimError, AimErrorMin, DeltaTime, 0.5f);
	const int32 Shots = W->GetShotsFired();
	if (Shots != LastShots)
	{
		ShotsFired += Shots - LastShots;
		BurstLeft -= Shots - LastShots;
		LastShots = Shots;
		AimOffset = FMath::VRand() * FMath::FRandRange(0.2f, 1.f) * AimError;
	}
	Me->SetAim(T->GetActorLocation() + FVector(0.f, 0.f, 40.f) + AimOffset, true);
	if (W->GetMagazine() == 0)
	{
		W->SetTriggerHeld(false);
		W->StartReload();
		Me->SetCrouchTarget(true);
		return;
	}
	if (BurstLeft <= 0)
	{
		// A cubierto entre ráfagas
		W->SetTriggerHeld(false);
		Me->SetCrouchTarget(true);
		BurstPause -= DeltaTime;
		if (BurstPause <= 0.f)
		{
			BurstLeft = FMath::RandRange(3, 6);
			BurstPause = FMath::FRandRange(0.9f, 2.0f);
		}
		return;
	}
	Me->SetCrouchTarget(false);
	W->SetTriggerHeld(true);
}
