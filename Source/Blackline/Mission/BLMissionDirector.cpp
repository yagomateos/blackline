#include "Mission/BLMissionDirector.h"

#include "Blackline.h"
#include "Audio/BLAudioSubsystem.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLInteractable.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"

#include "Components/AudioComponent.h"
#include "EngineUtils.h"
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
			if (RadioOutSound)
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
	if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this))
	{
		Audio->SetRadioTalking(true);
	}
	if (RadioInSound)
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
	if (const FBLObjective* O = GetCurrentObjective())
	{
		QueueRadio(O->RadioOnStart, Index);
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
		return P && T && FVector::Dist2D(P->GetActorLocation(), T->GetActorLocation()) < O.Radius;
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
			FString StartPhase;
			static const TMap<FString, int32> PhaseObjective = { { TEXT("Fase2"), 1 }, { TEXT("Fase3"), 2 }, { TEXT("Fase4"), 3 }, { TEXT("Objetivo"), 4 } };
			const int32* Skip = FParse::Value(FCommandLine::Get(), TEXT("BLStart="), StartPhase) ? PhaseObjective.Find(StartPhase) : nullptr;
			if (Skip && Objectives.IsValidIndex(*Skip))
			{
				StartObjective(*Skip);
			}
			else
			{
				QueueRadio(Briefing);
				StartObjective(0);
			}
		}
		return;
	}
	ObjectiveAge += DeltaTime;
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
		UE_LOG(LogBlackline, Log, TEXT("[Mision] %s completada en %.0f s: %d bajas, %d/%d impactos"), *MissionName, FinalTime, Kills, Hits, GetShots());
	}
}

bool ABLMissionDirector::IsVoicePlaying() const
{
	return VoiceComponent && VoiceComponent->IsPlaying();
}
