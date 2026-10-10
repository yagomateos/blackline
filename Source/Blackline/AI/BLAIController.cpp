#include "AI/BLAIController.h"

#include "Blackline.h"
#include "AI/BLCoverPoint.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLSquadSubsystem.h"
#include "Audio/BLAudioSubsystem.h"
#include "Combat/BLHealthComponent.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLGrenade.h"
#include "Environment/BLSmokeEmitter.h"
#include "Environment/BLNightSettings.h"
#include "Mission/BLMissionDirector.h"

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<int32> CVarAIDebug(TEXT("bl.AI.Debug"), 0, TEXT("1 = muestra estado, consciencia y cobertura de cada enemigo"));

ABLAIController::ABLAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	Sight->SightRadius = SightRadius;
	Sight->LoseSightRadius = SightRadius + 600.f;
	Sight->PeripheralVisionAngleDegrees = PeripheralAngle;
	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = true;
	Sight->SetMaxAge(5.f);
	Perception->ConfigureSense(*Sight);

	UAISenseConfig_Hearing* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
	Hearing->HearingRange = HearingRange;
	Hearing->DetectionByAffiliation.bDetectEnemies = true;
	Hearing->DetectionByAffiliation.bDetectNeutrals = true;
	Hearing->DetectionByAffiliation.bDetectFriendlies = true;
	Hearing->SetMaxAge(3.f);
	Perception->ConfigureSense(*Hearing);

	UAISenseConfig_Damage* Damage = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("Damage"));
	Damage->SetMaxAge(5.f);
	Perception->ConfigureSense(*Damage);
	Perception->SetDominantSense(UAISense_Sight::StaticClass());
}

ABLEnemyCharacter* ABLAIController::Enemy() const
{
	return Cast<ABLEnemyCharacter>(GetPawn());
}

UBLSquadSubsystem* ABLAIController::Squad() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBLSquadSubsystem>() : nullptr;
}

void ABLAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ABLAIController::HandlePerception);
	if (ABLEnemyCharacter* E = Enemy())
	{
		E->GetHealth()->OnDamaged.AddUniqueDynamic(this, &ABLAIController::HandleDamaged);
		HomeLocation = E->GetActorLocation();
		HomeRotation = E->GetActorRotation();
	}
	if (UBLSquadSubsystem* S = Squad())
	{
		S->Register(this);
	}
	CoverMaxTime = FMath::FRandRange(12.f, 20.f);
	SetState(EBLAIState::Patrol);
}

void ABLAIController::OnUnPossess()
{
	if (UBLSquadSubsystem* S = Squad())
	{
		S->Unregister(this);
	}
	State = EBLAIState::Dead;
	Super::OnUnPossess();
}

const TCHAR* ABLAIController::StateName(EBLAIState S)
{
	switch (S)
	{
	case EBLAIState::Patrol: return TEXT("Patrulla");
	case EBLAIState::Suspicious: return TEXT("Sospecha");
	case EBLAIState::Investigate: return TEXT("Investiga");
	case EBLAIState::Combat: return TEXT("Combate");
	case EBLAIState::Search: return TEXT("Busca");
	default: return TEXT("Muerto");
	}
}

const TCHAR* ABLAIController::ActionName(EBLCombatAction A)
{
	switch (A)
	{
	case EBLCombatAction::MoveToCover: return TEXT("ACobertura");
	case EBLCombatAction::InCover: return TEXT("EnCobertura");
	case EBLCombatAction::Hold: return TEXT("Aguanta");
	case EBLCombatAction::Flank: return TEXT("Flanquea");
	case EBLCombatAction::Chase: return TEXT("Persigue");
	default: return TEXT("-");
	}
}

void ABLAIController::SetState(EBLAIState NewState)
{
	if (State == NewState && StateTime > 0.f)
	{
		return;
	}
	UE_LOG(LogBlackline, Log, TEXT("[AI] %s: %s -> %s (consciencia %.2f)"), *GetNameSafe(GetPawn()), StateName(State), StateName(NewState), Awareness);
	State = NewState;
	StateTime = 0.f;
	WaitTimer = 0.f;
	if (ABLEnemyCharacter* E = Enemy())
	{
		E->GetWeapon()->SetTriggerHeld(false);
		E->SetCrouchTarget(false);
		E->SetJog(NewState == EBLAIState::Combat);
	}
	if (NewState != EBLAIState::Combat)
	{
		SetAction(EBLCombatAction::None);
		if (UBLSquadSubsystem* S = Squad())
		{
			S->ReleaseAttackToken(this);
			S->ReleaseCover(this);
		}
		Cover = nullptr;
		bAtCover = false;
	}
}

void ABLAIController::SetAction(EBLCombatAction NewAction)
{
	Action = NewAction;
	ActionTime = 0.f;
	bPeeking = false;
	PhaseTimer = 0.f;
}

void ABLAIController::Bark(EBLBark Type, const TCHAR* Line)
{
	// El director de audio decide si suena (sin pisarse con otros) y con qué variación de la voz del enemigo
	ABLEnemyCharacter* E = Enemy();
	if (E && E->IsCorvane())
	{
		return;   // los operadores de Corvane no gritan: disciplina de radio (se les reconoce por el silencio)
	}
	UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this);
	if (E && Audio)
	{
		if (UAudioComponent* Voice = Audio->PlayBark(E, E->GetVoiceIndex(), Type, Line))
		{
			E->SetVoiceComponent(Voice);
		}
	}
}

// ---------------------------------------------------------------------------
// Percepción
// ---------------------------------------------------------------------------

void ABLAIController::HandlePerception(AActor* Actor, FAIStimulus Stimulus)
{
	ABLCharacter* Player = Cast<ABLCharacter>(Actor);
	if (State == EBLAIState::Dead || !Player || Player->IsDead())
	{
		return;
	}
	const FAISenseID Sense = Stimulus.Type;
	UE_LOG(LogBlackline, Log, TEXT("[AI] %s percibe %s (sentido %d, %d)"), *GetNameSafe(GetPawn()), *GetNameSafe(Actor), int32(Sense.Index), Stimulus.WasSuccessfullySensed());
	if (Sense == UAISense::GetSenseID<UAISense_Sight>())
	{
		bSeenByPerception = Stimulus.WasSuccessfullySensed();
		if (bSeenByPerception)
		{
			Target = Player;
		}
	}
	else if (Sense == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		const bool bShot = Stimulus.Tag == FName("Disparo");
		const bool bImpact = Stimulus.Tag == FName("Impacto");
		if (State == EBLAIState::Combat)
		{
			if (!bTargetVisible)
			{
				LastKnownLocation = Stimulus.StimulusLocation;
			}
			return;
		}
		const float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Stimulus.StimulusLocation);
		if (bImpact || (bShot && Dist < 2200.f))
		{
			// Le disparan o un tiro muy cerca: combate directo hacia el origen
			EnterCombat(Player, bImpact ? Player->GetActorLocation() : Stimulus.StimulusLocation, true);
		}
		else
		{
			InvestigateLocation = Stimulus.StimulusLocation;
			Target = Player;
			Bark(bShot ? EBLBark::HeardShots : EBLBark::Suspicious, bShot ? TEXT("¡Disparos! ¡Por allí!") : TEXT("¿Qué ha sido eso?"));
			SetState(EBLAIState::Investigate);
		}
	}
	else if (Sense == UAISense::GetSenseID<UAISense_Damage>() && Stimulus.WasSuccessfullySensed())
	{
		EnterCombat(Player, Player->GetActorLocation(), true);
	}
}

void ABLAIController::HandleDamaged(const FBLDamageInfo& Info)
{
	if (State == EBLAIState::Dead || Info.bKilled)
	{
		return;
	}
	ABLCharacter* Player = Cast<ABLCharacter>(Info.Causer);
	if (!Player && Info.Instigator)
	{
		Player = Cast<ABLCharacter>(Info.Instigator->GetPawn());
	}
	if (Player)
	{
		EnterCombat(Player, Info.SourceLocation, true);
		// Herido fuera de cobertura: buscarla ya
		if (Action != EBLCombatAction::MoveToCover && !(bAtCover && Cover.IsValid() && Cover->IsProtectedFrom(TargetEye())))
		{
			Cover = nullptr;
			bAtCover = false;
			SetAction(EBLCombatAction::None);
		}
		Bark(EBLBark::Hit, TEXT("¡Me han dado!"));
	}
}

void ABLAIController::OnSquadAlert(AActor* InTarget, const FVector& Location)
{
	if (State == EBLAIState::Dead || State == EBLAIState::Combat)
	{
		return;
	}
	EnterCombat(InTarget, Location, false);
}

void ABLAIController::EnterCombat(AActor* InTarget, const FVector& Location, bool bAlertSquad)
{
	Target = InTarget;
	LastKnownLocation = Location;
	Awareness = 1.f;
	if (State != EBLAIState::Combat)
	{
		Bark(EBLBark::Contact, TEXT("¡Contacto! ¡A cubierto!"));
		SetState(EBLAIState::Combat);
		AimError = AimErrorMax;
		TimeSinceSeen = bTargetVisible ? 0.f : 3.f;
	}
	if (bAlertSquad)
	{
		if (UBLSquadSubsystem* S = Squad())
		{
			S->ReportTarget(this, InTarget, Location);
		}
		if (ABLMissionDirector* D = ABLMissionDirector::Get(this); D && D->bAlarmOnDetection)
		{
			D->TriggerAlarm(TEXT("descubierto"));
		}
	}
}

FVector ABLAIController::TargetEye() const
{
	if (const ABLCharacter* P = Cast<ABLCharacter>(Target.Get()))
	{
		return P->GetCamera()->GetComponentLocation();
	}
	return LastKnownLocation + FVector(0.f, 0.f, 70.f);
}

FVector ABLAIController::TargetChest() const
{
	if (const AActor* T = Target.Get())
	{
		return T->GetActorLocation() + FVector(0.f, 0.f, 25.f);
	}
	return LastKnownLocation + FVector(0.f, 0.f, 25.f);
}

bool ABLAIController::CheckLineOfSight() const
{
	const ABLEnemyCharacter* E = Enemy();
	const AActor* T = Target.Get();
	if (!E || !T)
	{
		return false;
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLAILos), false, E);
	Q.AddIgnoredActor(T);
	const FVector From = E->GetEyeLocation();
	// El humo tapa (granadas de humo del contraataque)
	if (ABLSmokeEmitter::BlocksSight(From, TargetChest()))
	{
		return false;
	}
	// Ve al jugador si ve la cabeza o el pecho (canal Visibility: los volúmenes y personajes no tapan)
	return !GetWorld()->LineTraceTestByChannel(From, TargetEye(), ECC_Visibility, Q)
		|| !GetWorld()->LineTraceTestByChannel(From, TargetChest(), ECC_Visibility, Q);
}

void ABLAIController::TickPerception(float DeltaTime)
{
	TimeSinceSeen += DeltaTime;
	const ABLCharacter* Player = Cast<ABLCharacter>(Target.Get());
	if (Player && Player->IsDead())
	{
		bTargetVisible = false;
		return;
	}
	// En combate la visibilidad se comprueba con trazados propios (no depende del cono de visión)
	LosTimer -= DeltaTime;
	if (LosTimer <= 0.f)
	{
		LosTimer = 0.15f;
		const bool bInCone = bSeenByPerception || State == EBLAIState::Combat;
		const float D = Player ? FVector::Dist(Player->GetActorLocation(), GetPawn()->GetActorLocation()) : 0.f;
		NightVisibility = Player ? ABLNightSettings::GetVisibility(GetWorld(), GetPawn()->GetActorLocation(), Player, D) : 1.f;
		bTargetVisible = Player && bInCone && NightVisibility > 0.f && CheckLineOfSight() && D < SightRadius + 600.f;
	}
	if (bTargetVisible)
	{
		TimeSinceSeen = 0.f;
		LastKnownLocation = Player->GetActorLocation();
	}

	// Consciencia: sube viéndolo (más rápido de cerca, de pie y en movimiento), baja si no lo ve
	if (State != EBLAIState::Combat)
	{
		if (bTargetVisible)
		{
			const float Dist = FVector::Dist(Player->GetActorLocation(), GetPawn()->GetActorLocation());
			float Rate = AwarenessRate * FMath::Clamp(1.f - Dist / SightRadius, 0.08f, 1.f);
			Rate *= Player->bIsCrouched ? 0.55f : 1.f;
			Rate *= Player->GetVelocity().Size2D() > 450.f ? 1.4f : 1.f;
			Rate *= Dist < 600.f ? 3.f : 1.f;   // casi encima: lo ve al instante
			Rate *= Dist < 600.f ? FMath::Max(NightVisibility, 0.6f) : NightVisibility;   // de noche, a oscuras cuesta verle
			Awareness += Rate * DeltaTime;
		}
		else
		{
			Awareness = FMath::Max(0.f, Awareness - 0.12f * DeltaTime);
		}
		if (Awareness >= 1.f && Player)
		{
			EnterCombat(const_cast<ABLCharacter*>(Player), Player->GetActorLocation(), true);
		}
		else if (Awareness > 0.3f && (State == EBLAIState::Patrol || State == EBLAIState::Search))
		{
			Bark(EBLBark::Suspicious, TEXT("¿Hay alguien ahí?"));
			SetState(EBLAIState::Suspicious);
		}
	}
	else if (bTargetVisible)
	{
		// Puntería: el error baja mientras lo tiene a la vista (más rápido si el jugador está quieto)
		const float Moving = Player->GetVelocity().Size2D() > 250.f ? 1.6f : 1.f;
		const float Skill = Enemy() && Enemy()->IsCorvane() ? 0.5f : 1.f;   // Corvane: mucho mejor puntería
		// Equilibrio medido con la prueba "Aguante" (2026-10-10): antes 3 milicianos a 20 m mataban 0,3 s después del primer
		// impacto; la puntería converge más despacio y con más error por distancia
		const float MinError = (AimErrorMin * Moving + FVector::Dist(Player->GetActorLocation(), GetPawn()->GetActorLocation()) * 0.015f) * Skill;
		AimError = FMath::FInterpTo(AimError, MinError, DeltaTime, 0.6f);
	}
	else
	{
		AimError = FMath::FInterpTo(AimError, AimErrorMax, DeltaTime, 0.4f);
	}
}

// ---------------------------------------------------------------------------
// Movimiento
// ---------------------------------------------------------------------------

bool ABLAIController::MoveToPoint(const FVector& Location, bool bJog)
{
	if (ABLEnemyCharacter* E = Enemy())
	{
		E->SetJog(bJog);
	}
	const EPathFollowingRequestResult::Type R = MoveToLocation(Location, 35.f, true, true, true, true);
	return R != EPathFollowingRequestResult::Failed;
}

bool ABLAIController::HasArrived(float Radius) const
{
	return GetMoveStatus() == EPathFollowingStatus::Idle;
}

void ABLAIController::LookAround(float DeltaTime, float Range)
{
	ABLEnemyCharacter* E = Enemy();
	LookTimer -= DeltaTime;
	if (LookTimer <= 0.f)
	{
		LookTimer = FMath::FRandRange(2.5f, 4.5f);
		LookYaw = FMath::FRandRange(-Range, Range);
	}
	const FRotator R(0.f, HomeRotation.Yaw + LookYaw, 0.f);
	const FRotator Cur = FMath::RInterpConstantTo(E->GetActorRotation(), R, DeltaTime, 60.f);
	E->SetActorRotation(Cur);
	E->SetAim(E->GetActorLocation() + Cur.Vector() * 1000.f, false);
}

// ---------------------------------------------------------------------------
// Estados fuera de combate
// ---------------------------------------------------------------------------

void ABLAIController::TickPatrol(float DeltaTime)
{
	ABLEnemyCharacter* E = Enemy();
	if (E->PatrolPoints.Num() == 0)
	{
		// Centinela: vigila en su sitio girando la cabeza (el cuerpo)
		if (FVector::Dist2D(E->GetActorLocation(), HomeLocation) > 100.f)
		{
			if (GetMoveStatus() == EPathFollowingStatus::Idle)
			{
				MoveToPoint(HomeLocation, false);
			}
			E->SetAim(E->GetActorLocation() + E->GetActorForwardVector() * 1000.f, false);
			return;
		}
		LookAround(DeltaTime, 55.f);
		return;
	}
	// Ruta: anda hasta cada punto, espera y mira alrededor
	if (WaitTimer > 0.f)
	{
		WaitTimer -= DeltaTime;
		HomeRotation = E->GetActorRotation();
		E->SetAim(E->GetActorLocation() + E->GetActorForwardVector() * 1000.f, false);
		return;
	}
	const FVector Goal = E->PatrolPoints[PatrolIndex % E->PatrolPoints.Num()];
	if (FVector::Dist2D(E->GetActorLocation(), Goal) < 80.f)
	{
		PatrolIndex = (PatrolIndex + 1) % E->PatrolPoints.Num();
		WaitTimer = FMath::FRandRange(1.5f, 4.f);
		return;
	}
	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		MoveToPoint(Goal, false);
	}
	E->SetAim(E->GetActorLocation() + E->GetVelocity().GetSafeNormal2D() * 1000.f, false);
}

void ABLAIController::TickSuspicious(float DeltaTime)
{
	// Se para y mira hacia lo que vio; si la consciencia se apaga, va a mirar
	ABLEnemyCharacter* E = Enemy();
	StopMovement();
	E->SetAim(LastKnownLocation + FVector(0.f, 0.f, 60.f), true);
	if (Awareness <= 0.05f || StateTime > 6.f)
	{
		InvestigateLocation = LastKnownLocation;
		SetState(EBLAIState::Investigate);
	}
}

void ABLAIController::TickInvestigate(float DeltaTime)
{
	ABLEnemyCharacter* E = Enemy();
	if (StateTime < 0.1f || (GetMoveStatus() == EPathFollowingStatus::Idle && WaitTimer <= 0.f && FVector::Dist2D(E->GetActorLocation(), InvestigateLocation) > 200.f && StateTime < 1.f))
	{
		MoveToPoint(InvestigateLocation, false);
	}
	if (GetMoveStatus() == EPathFollowingStatus::Idle && StateTime > 0.5f)
	{
		// Llegó (o no puede llegar): mira alrededor y vuelve
		if (WaitTimer <= 0.f)
		{
			WaitTimer = 6.f;
			HomeRotation = E->GetActorRotation();
		}
		WaitTimer -= DeltaTime;
		LookAround(DeltaTime, 120.f);
		if (WaitTimer <= 0.1f)
		{
			Bark(EBLBark::GiveUp, TEXT("Nada. Vuelvo a mi puesto."));
			HomeRotation = E->PatrolPoints.Num() == 0 ? HomeRotation : HomeRotation;
			SetState(EBLAIState::Patrol);
		}
		return;
	}
	E->SetAim(InvestigateLocation + FVector(0.f, 0.f, 60.f), true);
}

void ABLAIController::TickSearch(float DeltaTime)
{
	ABLEnemyCharacter* E = Enemy();
	if (StateTime < 0.05f)
	{
		SearchPoints.Reset();
		if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			for (int32 i = 0; i < 3; ++i)
			{
				FNavLocation P;
				if (Nav->GetRandomReachablePointInRadius(LastKnownLocation, 900.f, P))
				{
					SearchPoints.Add(P.Location);
				}
			}
		}
	}
	if (GetMoveStatus() == EPathFollowingStatus::Idle && SearchPoints.Num() > 0)
	{
		MoveToPoint(SearchPoints.Pop(), false);
	}
	const FVector Dir = E->GetVelocity().GetSafeNormal2D();
	E->SetAim(E->GetActorLocation() + (Dir.IsNearlyZero() ? E->GetActorForwardVector() : Dir) * 1000.f + FVector(0.f, 0.f, 60.f), true);
	if (StateTime > SearchTime)
	{
		Bark(EBLBark::Lost, TEXT("Lo hemos perdido. Atentos."));
		Target = nullptr;
		Awareness = 0.f;
		SetState(EBLAIState::Patrol);
	}
}

// ---------------------------------------------------------------------------
// Combate
// ---------------------------------------------------------------------------

ABLCoverPoint* ABLAIController::FindCover(bool bFlank) const
{
	const ABLEnemyCharacter* E = Enemy();
	const UBLSquadSubsystem* S = Squad();
	if (!E || !S)
	{
		return nullptr;
	}
	const FVector Me = E->GetActorLocation();
	const FVector Threat = LastKnownLocation;
	const FVector ThreatEye = Target.IsValid() ? TargetEye() : LastKnownLocation + FVector(0.f, 0.f, 70.f);
	const FVector MainDir = S->GetMainAttackDirection(Target.Get());

	struct FCandidate { ABLCoverPoint* Point; float Score; };
	TArray<FCandidate> Candidates;
	for (TActorIterator<ABLCoverPoint> It(GetWorld()); It; ++It)
	{
		ABLCoverPoint* C = *It;
		const FVector P = C->GetActorLocation();
		const float DSelf = FVector::Dist2D(P, Me);
		const float DThreat = FVector::Dist2D(P, Threat);
		if (DSelf > (bFlank ? 3200.f : 2600.f) || DThreat < 500.f || DThreat > 4000.f || !S->IsCoverFree(C, this))
		{
			continue;
		}
		// Separación con los compañeros
		bool bCrowded = false;
		for (const TWeakObjectPtr<ABLAIController>& M : S->GetMembers())
		{
			if (M.IsValid() && M.Get() != this && M->GetPawn() && FVector::Dist2D(M->GetPawn()->GetActorLocation(), P) < 220.f)
			{
				bCrowded = true;
				break;
			}
		}
		if (bCrowded)
		{
			continue;
		}
		float Score = -DSelf - FMath::Abs(DThreat - 1500.f) * 0.5f;
		if (bFlank)
		{
			if (MainDir.IsNearlyZero())
			{
				continue;
			}
			const float Cos = FVector::DotProduct((P - Threat).GetSafeNormal2D(), MainDir);
			if (Cos > 0.65f || DThreat > 2800.f)   // al menos ~50° fuera de la dirección principal
			{
				continue;
			}
			Score += (1.f - Cos) * 1200.f;
		}
		Candidates.Add({ C, Score });
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Score > B.Score; });

	// Comprobar protección, posición de disparo y que se pueda llegar (solo para los mejores)
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	int32 Checked = 0, NotProtected = 0, NoFire = 0, NoPath = 0;
	for (const FCandidate& C : Candidates)
	{
		if (++Checked > (bFlank ? 24 : 12))
		{
			break;
		}
		FVector FirePos;
		bool bStand;
		if (!C.Point->IsProtectedFrom(ThreatEye))
		{
			++NotProtected;
			continue;
		}
		if (!C.Point->FindFirePosition(ThreatEye, FirePos, bStand))
		{
			++NoFire;
			continue;
		}
		if (Nav)
		{
			const UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), Me, C.Point->GetActorLocation(), GetPawn());
			if (!Path || !Path->IsValid() || Path->IsPartial() || Path->GetPathLength() > (bFlank ? 4000.f : 3200.f))
			{
				++NoPath;
				continue;
			}
		}
		return C.Point;
	}
	if (bFlank)
	{
		UE_LOG(LogBlackline, Log, TEXT("[AI] %s: flanqueo sin cobertura (%d candidatos; sin proteccion %d, sin tiro %d, sin ruta %d; dir principal %s)"),
			*GetNameSafe(GetPawn()), Candidates.Num(), NotProtected, NoFire, NoPath, *MainDir.ToCompactString());
	}
	return nullptr;
}

bool ABLAIController::StartFlank()
{
	if (Enemy() && Enemy()->IsStatic())
	{
		return false;   // la ametralladora y el tirador se quedan en su sitio
	}
	ABLCoverPoint* C = FindCover(true);
	if (!C)
	{
		UE_LOG(LogBlackline, Log, TEXT("[AI] %s: sin cobertura de flanqueo"), *GetNameSafe(GetPawn()));
	}
	if (C)
	{
		Cover = C;
		Squad()->ReserveCover(C, this);
		Squad()->ReleaseAttackToken(this);
		bAtCover = false;
		SetAction(EBLCombatAction::Flank);
		Enemy()->SetCrouchTarget(false);
		Enemy()->GetWeapon()->SetTriggerHeld(false);
		MoveToPoint(C->GetActorLocation(), true);
		++FlanksDone;
		Bark(EBLBark::Flank, TEXT("¡Lo rodeo por el flanco!"));
		return true;
	}
	return false;
}

void ABLAIController::TickShooting(float DeltaTime, bool bAllowed)
{
	ABLEnemyCharacter* E = Enemy();
	UBLWeaponComponent* W = E->GetWeapon();
	const int32 Shots = W->GetShotsFired();
	if (Shots != LastWeaponShots)
	{
		ShotsFired += Shots - LastWeaponShots;
		BurstLeft -= Shots - LastWeaponShots;
		LastWeaponShots = Shots;
		// Cada disparo apunta a un punto distinto dentro del error actual
		AimOffset = FMath::VRand() * FMath::FRandRange(0.2f, 1.f) * AimError;
	}
	// Fuego de supresión: si lo acaba de perder de vista, sigue disparando un poco a donde estaba
	// (la ametralladora, mucho más: barre la cobertura donde se metió)
	const bool bGunner = E->IsGunner();
	const bool bSniper = E->IsSniper();
	// Tirador: tarda en fijar (el láser avisa) y no suprime; pierde el blanco poco a poco si se esconde
	SniperLock = bTargetVisible ? SniperLock + DeltaTime : FMath::Max(0.f, SniperLock - DeltaTime * 0.5f);
	if (bSniper)
	{
		AimOffset = AimOffset.GetClampedToMaxSize(FMath::Lerp(90.f, 8.f, FMath::Clamp(SniperLock / 1.8f, 0.f, 1.f)));
	}
	const bool bSuppress = !bSniper && !bTargetVisible && TimeSinceSeen < (bGunner ? 6.f : 1.5f);
	const bool bCanShoot = bAllowed && (bTargetVisible || bSuppress) && !W->IsReloading() && (!bSniper || SniperLock >= 1.8f);
	E->SetAim((bTargetVisible ? TargetChest() : LastKnownLocation + FVector(0.f, 0.f, 25.f)) + AimOffset, true);
	if (!bCanShoot)
	{
		W->SetTriggerHeld(false);
		return;
	}
	if (BurstLeft <= 0)
	{
		W->SetTriggerHeld(false);
		BurstPause -= DeltaTime;
		if (BurstPause <= 0.f)
		{
			BurstLeft = bSniper ? 1 : bGunner ? FMath::RandRange(8, 15) : FMath::RandRange(2, 5);
			BurstPause = bSniper ? FMath::FRandRange(1.4f, 2.1f) : bGunner ? FMath::FRandRange(0.9f, 1.8f) : FMath::FRandRange(0.6f, 1.3f);
		}
		return;
	}
	W->SetTriggerHeld(true);
}

void ABLAIController::TickCombat(float DeltaTime)
{
	ABLEnemyCharacter* E = Enemy();
	UBLWeaponComponent* W = E->GetWeapon();
	UBLSquadSubsystem* S = Squad();
	const ABLCharacter* Player = Cast<ABLCharacter>(Target.Get());
	if (!Player || Player->IsDead())
	{
		W->SetTriggerHeld(false);
		E->SetAim(LastKnownLocation + FVector(0.f, 0.f, 60.f), true);
		if (StateTime > 4.f)
		{
			SetState(EBLAIState::Search);
		}
		return;
	}
	ActionTime += DeltaTime;

	// Recargar: cuando quedan pocas balas y no está disparando con turno, o vacío
	bReloading = W->IsReloading();
	const int32 Mag = W->GetMagazine();
	if (!bReloading && (Mag == 0 || (Mag <= 6 && !bPeeking && Action != EBLCombatAction::Hold)))
	{
		W->SetTriggerHeld(false);
		if (W->StartReload())
		{
			++Reloads;
			bReloading = true;
			Bark(EBLBark::Reload, TEXT("¡Recargando, cubridme!"));
			S->ReleaseAttackToken(this);
			bPeeking = false;
		}
	}

	// Perdido de vista mucho tiempo: ir a buscarlo (la ametralladora no deja su puesto)
	if (TimeSinceSeen > ChaseDelay && !E->IsStatic() && Action != EBLCombatAction::Chase && Action != EBLCombatAction::Flank)
	{
		S->ReleaseAttackToken(this);
		S->ReleaseCover(this);
		Cover = nullptr;
		bAtCover = false;
		SetAction(EBLCombatAction::Chase);
		E->SetCrouchTarget(false);
		MoveToPoint(LastKnownLocation, true);
		Bark(EBLBark::Chase, TEXT("¡Se esconde! Voy a por él."));
	}

	switch (Action)
	{
	case EBLCombatAction::None:
	case EBLCombatAction::Hold:
	{
		// Buscar cobertura cada poco; mientras, disparar desde donde está (sin turno no dispara)
		RepathTimer -= DeltaTime;
		if (Action == EBLCombatAction::None || RepathTimer <= 0.f)
		{
			RepathTimer = 1.5f;
			if (ABLCoverPoint* C = E->IsStatic() ? nullptr : FindCover(false))
			{
				Cover = C;
				S->ReserveCover(C, this);
				bAtCover = false;
				SetAction(EBLCombatAction::MoveToCover);
				E->SetCrouchTarget(false);
				MoveToPoint(C->GetActorLocation(), true);
				break;
			}
			if (Action == EBLCombatAction::None)
			{
				SetAction(EBLCombatAction::Hold);
				StopMovement();
				// Sin cobertura y lejos: acercarse un poco
				if (!E->IsStatic() && FVector::Dist(E->GetActorLocation(), LastKnownLocation) > 2800.f)
				{
					MoveToPoint(E->GetActorLocation() + (LastKnownLocation - E->GetActorLocation()).GetSafeNormal() * 800.f, true);
				}
			}
		}
		// La ametralladora no espera turno: su trabajo es no dejarte asomar
		TickShooting(DeltaTime, !bReloading && (E->IsGunner() || E->IsCorvane() || S->RequestAttackToken(this)));
		break;
	}
	case EBLCombatAction::MoveToCover:
	case EBLCombatAction::Flank:
	{
		// Corre a la cobertura apuntando al objetivo (no dispara en carrera)
		W->SetTriggerHeld(false);
		E->SetAim((bTargetVisible ? TargetChest() : LastKnownLocation + FVector(0.f, 0.f, 25.f)), true);
		if (!Cover.IsValid() || ActionTime > 12.f)
		{
			SetAction(EBLCombatAction::None);
			break;
		}
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			if (FVector::Dist2D(E->GetActorLocation(), Cover->GetActorLocation()) < 120.f)
			{
				bAtCover = true;
				CoverTime = 0.f;
				CoverMaxTime = FMath::FRandRange(12.f, 20.f);
				SetAction(EBLCombatAction::InCover);
				PhaseTimer = FMath::FRandRange(0.6f, 1.6f);
			}
			else
			{
				MoveToPoint(Cover->GetActorLocation(), true);
			}
		}
		break;
	}
	case EBLCombatAction::InCover:
	{
		ABLCoverPoint* C = Cover.Get();
		if (!C)
		{
			SetAction(EBLCombatAction::None);
			break;
		}
		CoverTime += DeltaTime;
		// Cobertura inútil (el jugador ha flanqueado) o demasiado tiempo: cambiar de sitio
		if (!C->IsProtectedFrom(TargetEye()) || CoverTime > CoverMaxTime)
		{
			S->ReleaseAttackToken(this);
			S->ReleaseCover(this);
			Cover = nullptr;
			bAtCover = false;
			E->SetCrouchTarget(false);
			SetAction(EBLCombatAction::None);
			Bark(EBLBark::Reposition, TEXT("¡Me tiene a tiro, cambio de posición!"));
			break;
		}
		PhaseTimer -= DeltaTime;
		if (!bPeeking)
		{
			// Escondido: agachado tras una baja; en la alta, pegado a ella
			E->SetCrouchTarget(C->bLowCover);
			if (FVector::Dist2D(E->GetActorLocation(), C->GetActorLocation()) > 60.f && GetMoveStatus() == EPathFollowingStatus::Idle)
			{
				MoveToPoint(C->GetActorLocation(), false);
			}
			W->SetTriggerHeld(false);
			E->SetAim(LastKnownLocation + FVector(0.f, 0.f, 25.f), true);
			if (PhaseTimer <= 0.f && !bReloading && (E->IsCorvane() || S->RequestAttackToken(this)))
			{
				// Asomarse: ponerse de pie o salir al lado
				if (C->FindFirePosition(TargetEye(), FirePosition, bFireStand))
				{
					bPeeking = true;
					PhaseTimer = FMath::FRandRange(2.2f, 4.f);
					BurstLeft = 0;
					BurstPause = FMath::FRandRange(0.15f, 0.4f);
					if (FVector::Dist2D(FirePosition, E->GetActorLocation()) > 40.f)
					{
						MoveToPoint(FirePosition, false);
					}
				}
				else
				{
					S->ReleaseAttackToken(this);
					PhaseTimer = 1.f;
				}
			}
		}
		else
		{
			E->SetCrouchTarget(!bFireStand);
			const bool bInPosition = GetMoveStatus() == EPathFollowingStatus::Idle;
			TickShooting(DeltaTime, bInPosition && !bReloading);
			if (PhaseTimer <= 0.f || bReloading)
			{
				bPeeking = false;
				W->SetTriggerHeld(false);
				S->ReleaseAttackToken(this);
				PhaseTimer = FMath::FRandRange(1.2f, 2.6f);
				MoveToPoint(C->GetActorLocation(), false);
			}
		}
		break;
	}
	case EBLCombatAction::Chase:
	{
		W->SetTriggerHeld(false);
		E->SetCrouchTarget(false);
		E->SetAim(LastKnownLocation + FVector(0.f, 0.f, 25.f), true);
		if (bTargetVisible)
		{
			StopMovement();
			SetAction(EBLCombatAction::None);
		}
		else if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			SetState(EBLAIState::Search);
		}
		break;
	}
	}
}

// ---------------------------------------------------------------------------

void ABLAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ABLEnemyCharacter* E = Enemy();
	if (!E || E->IsDead() || State == EBLAIState::Dead)
	{
		return;
	}
	StateTime += DeltaTime;
	TickPerception(DeltaTime);
	// Aturdido por una carga de brecha: ni dispara ni se mueve (se tapa)
	if (StunTime > 0.f)
	{
		StunTime -= DeltaTime;
		E->GetWeapon()->SetTriggerHeld(false);
		StopMovement();
		E->SetCrouchTarget(true);
		E->SetAim(E->GetActorLocation() + E->GetActorForwardVector() * 300.f + FVector(0.f, 0.f, -150.f), false);
		return;
	}
	if (E->IsOperator() && State == EBLAIState::Combat)
	{
		TickOperatorGrenade(DeltaTime);
	}
	if (TickGrenadeEscape(DeltaTime))
	{
		return;   // huyendo de una granada: lo demás espera
	}
	switch (State)
	{
	case EBLAIState::Patrol: TickPatrol(DeltaTime); break;
	case EBLAIState::Suspicious: TickSuspicious(DeltaTime); break;
	case EBLAIState::Investigate: TickInvestigate(DeltaTime); break;
	case EBLAIState::Combat: TickCombat(DeltaTime); break;
	case EBLAIState::Search: TickSearch(DeltaTime); break;
	default: break;
	}

	if (CVarAIDebug.GetValueOnGameThread())
	{
		const FString Txt = FString::Printf(TEXT("%s %s\nconsc %.2f err %.0f %s"), StateName(State),
			State == EBLAIState::Combat ? ActionName(Action) : TEXT(""), Awareness, AimError, Squad() && Squad()->HasAttackToken(this) ? TEXT("TURNO") : TEXT(""));
		DrawDebugString(GetWorld(), E->GetActorLocation() + FVector(0.f, 0.f, 120.f), Txt, nullptr, FColor::Yellow, 0.f, true, 1.1f);
		if (Cover.IsValid())
		{
			DrawDebugLine(GetWorld(), E->GetActorLocation(), Cover->GetActorLocation(), FColor::Green, false, 0.f, 0, 1.5f);
		}
	}
}

void ABLAIController::Stun(float Seconds)
{
	StunTime = FMath::Max(StunTime, Seconds);
	UE_LOG(LogBlackline, Log, TEXT("[AI] %s aturdido %.1f s"), *GetNameSafe(GetPawn()), Seconds);
}

void ABLAIController::TickOperatorGrenade(float DeltaTime)
{
	// El operador saca al jugador de la cobertura: si lleva un rato escondido a media distancia, granada
	GrenadeCooldown -= DeltaTime;
	ABLEnemyCharacter* E = Enemy();
	const float Dist = FVector::Dist(E->GetActorLocation(), LastKnownLocation);
	if (GrenadesLeft <= 0 || GrenadeCooldown > 0.f || bTargetVisible || TimeSinceSeen < 2.5f || TimeSinceSeen > 10.f || Dist < 800.f || Dist > 2600.f)
	{
		return;
	}
	const FVector Start = E->GetActorLocation() + FVector(0.f, 0.f, 70.f) + E->GetActorForwardVector() * 40.f;
	FVector Velocity;
	if (!UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, Velocity, Start, LastKnownLocation + FVector(0.f, 0.f, 20.f), 0.f, 0.55f))
	{
		GrenadeCooldown = 2.f;
		return;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.Instigator = E;
	if (ABLGrenade* G = GetWorld()->SpawnActor<ABLGrenade>(ABLGrenade::StaticClass(), Start, FRotator::ZeroRotator, P))
	{
		G->Launch(Velocity, this, 2.6f);
		--GrenadesLeft;
		++GrenadesThrown;
		GrenadeCooldown = FMath::FRandRange(9.f, 14.f);
		UE_LOG(LogBlackline, Log, TEXT("[AI] %s lanza una granada a %.0f m"), *GetNameSafe(E), Dist / 100.f);
	}
}

bool ABLAIController::TickGrenadeEscape(float DeltaTime)
{
	ABLEnemyCharacter* E = Enemy();
	if (EscapeTime > 0.f)
	{
		EscapeTime -= DeltaTime;
		if (EscapeTime <= 0.f)
		{
			// Pasada la explosión vuelve a lo suyo (en combate busca otra cobertura)
			if (State == EBLAIState::Combat)
			{
				SetAction(EBLCombatAction::None);
			}
		}
		return EscapeTime > 0.f;
	}
	for (const TWeakObjectPtr<ABLGrenade>& G : ABLGrenade::GetLive())
	{
		if (!G.IsValid() || G->HasExploded() || G->bSmoke || EscapedGrenades.Contains(G))
		{
			continue;
		}
		const FVector GL = G->GetActorLocation();
		const float Dist = FVector::Dist(GL, E->GetActorLocation());
		// La ve o la oye rebotar: a menos de 7 m y en el último segundo y medio... o casi a los pies
		if (Dist < 700.f && (G->GetTimeLeft() < 2.6f || Dist < 300.f))
		{
			EscapedGrenades.Add(G);
			FVector Away = (E->GetActorLocation() - GL).GetSafeNormal2D();
			if (Away.IsNearlyZero())
			{
				Away = FMath::VRand().GetSafeNormal2D();
			}
			FVector Dest = E->GetActorLocation() + Away * 900.f;
			if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation Out;
				if (Nav->ProjectPointToNavigation(Dest, Out, FVector(400.f, 400.f, 200.f)))
				{
					Dest = Out.Location;
				}
			}
			E->GetWeapon()->SetTriggerHeld(false);
			E->SetCrouchTarget(false);
			if (UBLSquadSubsystem* S = Squad())
			{
				S->ReleaseAttackToken(this);
				S->ReleaseCover(this);
			}
			bAtCover = false;
			bPeeking = false;
			MoveToPoint(Dest, true);
			EscapeTime = FMath::Max(G->GetTimeLeft() + 0.6f, 1.2f);
			++GrenadeEscapes;
			Bark(EBLBark::Grenade, TEXT("¡Granada!"));
			// Si ya estaba en guardia, ahora sabe dónde está el que la tiró
			if (State != EBLAIState::Combat && G->GetInstigator())
			{
				EnterCombat(G->GetInstigator(), G->GetInstigator()->GetActorLocation(), true);
			}
			return true;
		}
	}
	return false;
}
