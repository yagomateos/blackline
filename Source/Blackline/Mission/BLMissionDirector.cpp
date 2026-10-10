#include "Mission/BLMissionDirector.h"

#include "Mission/BLCampaignProgress.h"

#include "Blackline.h"
#include "Audio/BLAudioSubsystem.h"
#include "Core/BLGameMode.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLVarek.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLActivatable.h"
#include "Mission/BLDestructibleTarget.h"
#include "Mission/BLInteractable.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"

#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "AI/BLAIController.h"
#include "Engine/TargetPoint.h"
#include "Weapons/BLGrenade.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ABLMissionDirector::ABLMissionDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<USoundBase> In(TEXT("/Game/Audio/Radio/SW_Radio_In.SW_Radio_In"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Out(TEXT("/Game/Audio/Radio/SW_Radio_Out.SW_Radio_Out"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Obj(TEXT("/Game/Audio/UI/SW_UI_Objective.SW_UI_Objective"));
	RadioInSound = In.Object;
	RadioOutSound = Out.Object;
	ObjectiveSound = Obj.Object;
}

ABLMissionDirector* ABLMissionDirector::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ABLMissionDirector> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ABLMissionDirector::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		It->OnUsed.AddUniqueDynamic(this, &ABLMissionDirector::HandleInteract);
	}
}

ABLCharacter* ABLMissionDirector::Player() const
{
	return Cast<ABLCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

AActor* ABLMissionDirector::FindTarget(FName Tag) const
{
	if (Tag.IsNone())
	{
		return nullptr;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(this, Tag, Found);
	return Found.Num() > 0 ? Found[0] : nullptr;
}

bool ABLMissionDirector::GetMarkerLocation(FVector& Out) const
{
	const FBLObjective* O = GetCurrentObjective();
	if (!O || !O->bShowMarker || bComplete)
	{
		return false;
	}
	if (O->Type == EBLObjectiveType::InteractAll)
	{
		// El que queda más cerca del jugador
		const ABLCharacter* P = Player();
		const ABLInteractable* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
		{
			const float D = P ? FVector::DistSquared(P->GetActorLocation(), It->GetActorLocation()) : 0.f;
			if (It->Tags.Contains(O->TargetTag) && !It->IsUsed() && D < BestDist)
			{
				Best = *It;
				BestDist = D;
			}
		}
		if (Best)
		{
			Out = Best->GetActorLocation() + FVector(0.f, 0.f, 25.f);
			return true;
		}
		return false;
	}
	if (const AActor* T = FindTarget(O->TargetTag))
	{
		Out = T->GetActorLocation() + FVector(0.f, 0.f, O->Type == EBLObjectiveType::Interact ? 25.f : 120.f);
		return true;
	}
	return false;
}

int32 ABLMissionDirector::GetShots() const
{
	const ABLCharacter* P = Player();
	return P && P->GetWeapon() ? P->GetWeapon()->GetShotsFired() - ShotsAtStart : 0;
}

void ABLMissionDirector::HandleHit(EBLHitZone Zone, float Damage, bool bKilled)
{
	if (bComplete)
	{
		return;
	}
	++Hits;
	Headshots += Zone == EBLHitZone::Head ? 1 : 0;
	Kills += bKilled ? 1 : 0;
}

void ABLMissionDirector::HandlePlayerDeath(const FBLDamageInfo& Info)
{
	++Deaths;
}

void ABLMissionDirector::HandleInteract(ABLInteractable* Interactable, ABLCharacter* User)
{
	const FBLObjective* O = GetCurrentObjective();
	if (O && O->Type == EBLObjectiveType::Interact && Interactable && Interactable->Tags.Contains(O->TargetTag))
	{
		bInteractDone = true;
	}
	if (Interactable)
	{
		QueueRadio(Interactable->UseRadio, CurrentIndex);
	}
}

void ABLMissionDirector::QueueRadio(const TArray<FBLRadioLine>& Lines, int32 ForObjective)
{
	for (const FBLRadioLine& L : Lines)
	{
		RadioQueue.Add(L);
		RadioOwners.Add(ForObjective);
	}
}

bool ABLMissionDirector::GetSubtitle(FString& OutSpeaker, FString& OutText, float& OutAlpha) const
{
	if (!bLineActive)
	{
		return false;
	}
	OutSpeaker = CurrentLine.Speaker;
	OutText = CurrentLine.Text;
	OutAlpha = FMath::Min(LineTime / 0.15f, 1.f) * FMath::Min((LineDuration - LineTime) / 0.3f, 1.f);
	return true;
}

void ABLMissionDirector::TickRadio(float DeltaTime)
{
	if (bLineActive)
	{
		LineTime += DeltaTime;
		if (VoiceDelay >= 0.f && LineTime >= VoiceDelay)
		{
			VoiceDelay = -1.f;
			VoiceComponent = UGameplayStatics::SpawnSound2D(this, CurrentLine.Voice, 1.f);
			++VoiceLinesPlayed;
		}
		if (LineTime >= LineDuration)
		{
			bLineActive = false;
			RadioGap = 0.35f;
			if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this))
			{
				Audio->SetRadioTalking(false);
			}
			if (RadioOutSound && !CurrentLine.bInPerson)
			{
				UGameplayStatics::PlaySound2D(this, RadioOutSound, 0.6f);
			}
		}
		return;
	}
	RadioGap -= DeltaTime;
	if (RadioGap > 0.f || RadioQueue.Num() == 0)
	{
		return;
	}
	CurrentLine = RadioQueue[0];
	RadioQueue.RemoveAt(0);
	RadioOwners.RemoveAt(0);
	// Duración por longitud: ~15 caracteres por segundo, mínimo 2,2 s
	LineDuration = CurrentLine.Duration > 0.f ? CurrentLine.Duration : FMath::Max(2.2f, CurrentLine.Text.Len() / 15.f + 0.8f);
	if (CurrentLine.Voice)
	{
		// Con voz: el subtítulo dura lo que la frase (+ el clic y un respiro)
		VoiceDelay = 0.12f;
		LineDuration = VoiceDelay + CurrentLine.Voice->GetDuration() + 0.3f;
	}
	LineTime = 0.f;
	bLineActive = true;
	if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this); Audio && !CurrentLine.bInPerson)
	{
		Audio->SetRadioTalking(true);
	}
	if (RadioInSound && !CurrentLine.bInPerson)
	{
		UGameplayStatics::PlaySound2D(this, RadioInSound, 0.6f);
	}
	UE_LOG(LogBlackline, Log, TEXT("[Radio] %s: %s"), *CurrentLine.Speaker, *CurrentLine.Text);
}

void ABLMissionDirector::StartObjective(int32 Index)
{
	CurrentIndex = Index;
	ObjectiveAge = 0.f;
	bInteractDone = false;
	WavesSpawned = 0;
	WaveTimer = 0.f;
	WaveEnemies.Reset();
	if (const FBLObjective* O = GetCurrentObjective())
	{
		QueueRadio(O->RadioOnStart, Index);
		for (const FName& Tag : O->ActivateTags)
		{
			if (Tag == AlarmTag)
			{
				TriggerAlarm(TEXT("objetivo"));
				continue;
			}
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains(Tag))
				{
					if (IBLActivatable* A = Cast<IBLActivatable>(*It))
					{
						A->OnMissionActivate(Tag);
					}
				}
			}
		}
		if (ObjectiveSound)
		{
			UGameplayStatics::PlaySound2D(this, ObjectiveSound, 0.7f);
		}
		UE_LOG(LogBlackline, Log, TEXT("[Mision] Objetivo %d: %s"), Index + 1, *O->Text);
	}
}

bool ABLMissionDirector::IsObjectiveDone(const FBLObjective& O) const
{
	switch (O.Type)
	{
	case EBLObjectiveType::Reach:
	{
		const ABLCharacter* P = Player();
		const AActor* T = FindTarget(O.TargetTag);
		return P && T && FVector::Dist2D(P->GetActorLocation(), T->GetActorLocation()) < O.Radius
			&& (!O.bCheckHeight || FMath::Abs(P->GetActorLocation().Z - 96.f - T->GetActorLocation().Z) < 250.f);
	}
	case EBLObjectiveType::ClearSquad:
	{
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (It->SquadId == O.SquadId && !It->IsDead())
			{
				return false;
			}
		}
		return true;
	}
	case EBLObjectiveType::Interact:
		return bInteractDone;
	case EBLObjectiveType::Defend:
		return WavesSpawned >= O.Waves.Num() && GetWaveEnemiesAlive() == 0 && ObjectiveAge >= O.MinDuration;
	case EBLObjectiveType::Destroy:
	{
		const IBLDestructibleTarget* T = Cast<IBLDestructibleTarget>(FindTarget(O.TargetTag));
		return T && T->IsTargetDestroyed();
	}
	case EBLObjectiveType::InteractAll:
	{
		int32 Total = 0, Used = 0;
		CountInteractables(O.TargetTag, Total, Used);
		return Total > 0 && Used == Total;
	}
	}
	return false;
}

void ABLMissionDirector::CompleteCurrentObjective()
{
	const FBLObjective* O = GetCurrentObjective();
	if (!O)
	{
		return;
	}
	UE_LOG(LogBlackline, Log, TEXT("[Mision] Cumplido: %s"), *O->Text);
	// Las instrucciones pendientes de este objetivo ya no tienen sentido (el jugador fue más rápido que la radio)
	for (int32 i = RadioQueue.Num() - 1; i >= 0; --i)
	{
		if (RadioOwners[i] == CurrentIndex)
		{
			RadioQueue.RemoveAt(i);
			RadioOwners.RemoveAt(i);
		}
	}
	QueueRadio(O->RadioOnComplete);
	if (Objectives.IsValidIndex(CurrentIndex + 1))
	{
		StartObjective(CurrentIndex + 1);
	}
	else
	{
		CurrentIndex = INDEX_NONE;
		bAllObjectivesDone = true;
		QueueRadio(Debriefing);
	}
}

float ABLMissionDirector::GetTimeLeft() const
{
	const FBLObjective* O = GetCurrentObjective();
	return O && O->TimeLimit > 0.f ? O->TimeLimit - ObjectiveAge : -1.f;
}

void ABLMissionDirector::FailMission(const FString& Reason)
{
	if (bFailed || bComplete)
	{
		return;
	}
	bFailed = true;
	FailAge = 0.f;
	FailReason = Reason;
	RadioQueue.Reset();
	RadioOwners.Reset();
	UE_LOG(LogBlackline, Log, TEXT("[Mision] FALLIDA: %s (objetivo %d)"), *Reason, CurrentIndex + 1);
}

void ABLMissionDirector::TriggerAlarm(const TCHAR* Reason)
{
	if (bAlarm)
	{
		return;
	}
	bAlarm = true;
	AlarmTime = ElapsedTime;
	UE_LOG(LogBlackline, Log, TEXT("[Mision] ALARMA (%s)"), Reason);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(AlarmTag))
		{
			if (IBLActivatable* A = Cast<IBLActivatable>(*It))
			{
				A->OnMissionActivate(AlarmTag);
			}
		}
	}
	// La radio de "te han visto" solo si te han descubierto (si la dispara el guion, ya la lleva el objetivo)
	if (FCString::Strcmp(Reason, TEXT("objetivo")) != 0)
	{
		QueueRadio(AlarmRadio);
	}
	// Los milicianos cercanos acuden a la última posición del jugador
	if (ABLCharacter* P = Player())
	{
		const FVector Loc = P->GetActorLocation();
		for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), Loc) < AlarmAlertRadius)
			{
				if (ABLAIController* AI = Cast<ABLAIController>(It->GetController()))
				{
					AI->OnSquadAlert(P, Loc);
				}
			}
		}
	}
}

int32 ABLMissionDirector::GetNextMissionIndex() const
{
	const int32 Current = BLCampaign::FindMission(UGameplayStatics::GetCurrentLevelName(this));
	return Current != INDEX_NONE && !bCampaignFinale && Current + 1 < BLCampaign::NumMissions() ? Current + 1 : INDEX_NONE;
}

void ABLMissionDirector::ContinueAfterMission()
{
	const int32 Next = GetNextMissionIndex();
	const FString Map = Next != INDEX_NONE ? BLCampaign::MissionMap(Next) : FString(TEXT("/Game/Maps/Menu/L_MainMenu"));
	UGameplayStatics::OpenLevel(this, FName(*Map));
}

void ABLMissionDirector::RestartMission()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ABLMissionDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bBoundPlayer)
	{
		if (ABLCharacter* P = Player())
		{
			bBoundPlayer = true;
			P->GetWeapon()->OnHitConfirmed.AddUniqueDynamic(this, &ABLMissionDirector::HandleHit);
			P->GetHealth()->OnDeath.AddUniqueDynamic(this, &ABLMissionDirector::HandlePlayerDeath);
			ShotsAtStart = P->GetWeapon()->GetShotsFired();
		}
	}
	TickRadio(DeltaTime);
	if (bComplete)
	{
		CompleteAge += DeltaTime;
		return;
	}
	ElapsedTime += DeltaTime;
	if (!bStarted)
	{
		StartDelay -= DeltaTime;
		if (StartDelay <= 0.f)
		{
			bStarted = true;
			// -BLStart=<Fase> (jugar_fase.bat): se salta el briefing y empieza en el objetivo de esa fase
			static const TMap<FString, int32> PhaseObjectiveM01 = { { TEXT("Fase2"), 1 }, { TEXT("Fase3"), 2 }, { TEXT("Fase4"), 3 }, { TEXT("Objetivo"), 4 },
				{ TEXT("Bloque"), 5 }, { TEXT("Azotea"), 9 }, { TEXT("Muelle"), 11 } };
			const TMap<FString, int32>& Phases = PhaseObjectives.Num() > 0 ? static_cast<const TMap<FString, int32>&>(PhaseObjectives) : PhaseObjectiveM01;
			const int32* Skip = Phases.Find(ABLGameMode::GetStartPhase(this));
			if (Skip && Objectives.IsValidIndex(*Skip))
			{
				StartObjective(*Skip);
				// Fases posteriores al rescate: Varek ya va con el equipo
				const ABLCharacter* P = Player();
				for (TActorIterator<ABLVarek> It(GetWorld()); It && P && *Skip >= 8; ++It)
				{
					It->Free();
					It->SetActorLocation(P->GetActorLocation() - P->GetActorForwardVector() * 180.f + P->GetActorRightVector() * 60.f, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
			else
			{
				QueueRadio(Briefing);
				StartObjective(0);
			}
		}
		return;
	}
	if (bFailed)
	{
		// Pantalla de misión fallida y, a los 5 s, se repite desde la fase más cercana (el mundo vuelve a su estado)
		FailAge += DeltaTime;
		if (FailAge > 5.f)
		{
			FString Phase;
			int32 Best = -1;
			for (const TPair<FString, int32>& P : PhaseObjectives)
			{
				if (P.Value <= CurrentIndex && P.Value > Best)
				{
					Best = P.Value;
					Phase = P.Key;
				}
			}
			UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true, Phase.IsEmpty() ? FString() : TEXT("BLStart=") + Phase);
			bFailed = false;
		}
		return;
	}
	ObjectiveAge += DeltaTime;
	// Condiciones de fallo: el tiempo del objetivo y el personaje que había que coger vivo
	if (const FBLObjective* TO = GetCurrentObjective(); TO && TO->TimeLimit > 0.f && ObjectiveAge > TO->TimeLimit && !IsObjectiveDone(*TO))
	{
		FailMission(TO->FailText.IsEmpty() ? FString(TEXT("Se acabó el tiempo.")) : TO->FailText);
		return;
	}
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(VIPTag) && It->IsDead())
		{
			FailMission(VIPDeathText);
			return;
		}
	}
	if (const FBLObjective* DO = GetCurrentObjective(); DO && DO->Type == EBLObjectiveType::Defend)
	{
		TickDefend(DeltaTime, *DO);
	}
	if (const FBLObjective* O = GetCurrentObjective())
	{
		if (ObjectiveAge > 0.5f && IsObjectiveDone(*O))
		{
			CompleteCurrentObjective();
		}
	}
	else if (bAllObjectivesDone && RadioQueue.Num() == 0 && !bLineActive)
	{
		// La radio de cierre ya sonó: pantalla de misión completada
		bComplete = true;
		FinalTime = ElapsedTime;
		if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this))
		{
			Audio->OnMissionComplete();
		}
		CompleteAge = 0.f;
		BLCampaign::MarkCompleted(BLCampaign::FindMission(UGameplayStatics::GetCurrentLevelName(this)), FinalTime);
		UE_LOG(LogBlackline, Log, TEXT("[Mision] %s completada en %.0f s: %d bajas, %d/%d impactos"), *MissionName, FinalTime, Kills, Hits, GetShots());
	}
}

void ABLMissionDirector::CountInteractables(FName Tag, int32& OutTotal, int32& OutUsed) const
{
	OutTotal = OutUsed = 0;
	for (TActorIterator<ABLInteractable> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(Tag))
		{
			++OutTotal;
			OutUsed += It->IsUsed() ? 1 : 0;
		}
	}
}

bool ABLMissionDirector::IsVoicePlaying() const
{
	return VoiceComponent && VoiceComponent->IsPlaying();
}

// ---------------------------------------------------------------------------
// Defender (contraataque, Bloque 11)
// ---------------------------------------------------------------------------

int32 ABLMissionDirector::GetWaveEnemiesAlive() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABLEnemyCharacter>& E : WaveEnemies)
	{
		N += E.IsValid() && !E->IsDead() ? 1 : 0;
	}
	return N;
}

FString ABLMissionDirector::GetObjectiveDisplayText() const
{
	const FBLObjective* O = GetCurrentObjective();
	if (!O)
	{
		return FString();
	}
	if (O->Type == EBLObjectiveType::Defend && !O->ProgressLabel.IsEmpty())
	{
		// 100 % solo cuando de verdad se cumple (tiempo y oleadas)
		const int32 Pct = FMath::Min(FMath::FloorToInt(100.f * ObjectiveAge / FMath::Max(O->MinDuration, 1.f)), 99);
		return FString::Printf(TEXT("%s (%s %d %%)"), *O->Text, *O->ProgressLabel, Pct);
	}
	if (O->Type == EBLObjectiveType::Defend && O->Waves.Num() > 0)
	{
		return FString::Printf(TEXT("%s (oleada %d/%d)"), *O->Text, FMath::Max(WavesSpawned, 1), O->Waves.Num());
	}
	if (O->Type == EBLObjectiveType::InteractAll)
	{
		int32 Total = 0, Used = 0;
		CountInteractables(O->TargetTag, Total, Used);
		return FString::Printf(TEXT("%s (%d/%d)"), *O->Text, Used, Total);
	}
	if (O->TimeLimit > 0.f)
	{
		const int32 Left = FMath::Max(0, FMath::CeilToInt(GetTimeLeft()));
		return FString::Printf(TEXT("%s (%d:%02d)"), *O->Text, Left / 60, Left % 60);
	}
	return O->Text;
}

void ABLMissionDirector::TickDefend(float DeltaTime, const FBLObjective& O)
{
	if (WavesSpawned >= O.Waves.Num())
	{
		return;
	}
	WaveTimer += DeltaTime;
	const FBLWave& Next = O.Waves[WavesSpawned];
	if (WaveTimer >= Next.Delay && GetWaveEnemiesAlive() <= O.MaxAliveForNextWave)
	{
		WaveTimer = 0.f;
		SpawnWave(O, Next);
		++WavesSpawned;
	}
}

void ABLMissionDirector::SpawnWave(const FBLObjective& O, const FBLWave& W)
{
	TArray<AActor*> Points;
	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		for (const FName& Tag : W.SpawnTags)
		{
			if (It->Tags.Contains(Tag))
			{
				Points.Add(*It);
			}
		}
	}
	ABLCharacter* P = Player();
	if (Points.Num() == 0 || !P)
	{
		return;
	}
	QueueRadio(W.Radio, CurrentIndex);
	for (int32 i = 0; i < W.Count; ++i)
	{
		const AActor* Pt = Points[i % Points.Num()];
		const FVector Loc = Pt->GetActorLocation() + FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 96.f);
		const FRotator Rot = (P->GetActorLocation() - Loc).GetSafeNormal2D().Rotation();
		// Diferido: el papel (ametralladora) y la linterna se leen en el BeginPlay del miliciano
		const FTransform At(Rot, Loc);
		ABLEnemyCharacter* E = GetWorld()->SpawnActorDeferred<ABLEnemyCharacter>(ABLEnemyCharacter::StaticClass(), At, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!E)
		{
			continue;
		}
		E->EnemyRole = i < W.Gunners ? EBLEnemyRole::Gunner : i < W.Gunners + W.Operators ? EBLEnemyRole::Operator : EBLEnemyRole::Rifleman;
		E->bFlashlight = W.bFlashlights;
		E->SquadId = O.SquadId;
		E->FinishSpawning(At);
		E->Tags.Add(FName("BLWaveEnemy"));
		if (!E->GetController())
		{
			E->SpawnDefaultController();
		}
		// Llegan sabiendo dónde está el jugador (vienen a por él)
		if (ABLAIController* AI = Cast<ABLAIController>(E->GetController()))
		{
			AI->OnSquadAlert(P, P->GetActorLocation());
		}
		WaveEnemies.Add(E);
		// El primero lanza humo hacia el bloque para cubrir el avance
		if (W.bSmoke && i == 0)
		{
			FVector Velocity;
			const FVector Start = Loc + FVector(0.f, 0.f, 60.f);
			const FVector Target = FMath::Lerp(Start, P->GetActorLocation(), 0.55f);
			if (UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, Velocity, Start, Target, 0.f, 0.6f))
			{
				FActorSpawnParameters GP;
				GP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				GP.Instigator = E;
				if (ABLGrenade* G = GetWorld()->SpawnActor<ABLGrenade>(ABLGrenade::StaticClass(), Start, FRotator::ZeroRotator, GP))
				{
					G->bSmoke = true;
					G->Launch(Velocity, E->GetController(), 2.2f);
				}
			}
		}
	}
	UE_LOG(LogBlackline, Log, TEXT("[Mision] Oleada %d: %d milicianos%s"), WavesSpawned + 1, W.Count, W.bSmoke ? TEXT(" con humo") : TEXT(""));
}
