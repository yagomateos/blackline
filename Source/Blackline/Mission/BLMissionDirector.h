#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLMissionTypes.h"
#include "Combat/BLDamageTypes.h"
#include "BLMissionDirector.generated.h"

class ABLCharacter;
class ABLInteractable;
class USoundBase;
class UAudioComponent;

/**
 * Director de la misión (uno por nivel): lleva la lista de objetivos, comprueba cuándo se cumplen,
 * reproduce la radio con subtítulos y termina la misión con un resumen (tiempo, bajas, precisión).
 * El HUD le pregunta qué mostrar (objetivo, marcador, subtítulo, pantalla final).
 * Los objetivos los configura el script del nivel (build_m01_greybox.py).
 */
UCLASS()
class BLACKLINE_API ABLMissionDirector : public AActor
{
	GENERATED_BODY()

public:
	ABLMissionDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static ABLMissionDirector* Get(const UObject* WorldContext);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission") FString MissionName = TEXT("AMANECER ROTO");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission") TArray<FBLObjective> Objectives;
	/** Radio de apertura (antes del primer objetivo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission") TArray<FBLRadioLine> Briefing;
	/** Radio al terminar (antes de la pantalla de misión completada). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission") TArray<FBLRadioLine> Debriefing;

	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<USoundBase> RadioInSound;
	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<USoundBase> RadioOutSound;
	UPROPERTY(EditAnywhere, Category = "Sound") TObjectPtr<USoundBase> ObjectiveSound;

	// ---- Consultas del HUD ----
	const FBLObjective* GetCurrentObjective() const { return Objectives.IsValidIndex(CurrentIndex) ? &Objectives[CurrentIndex] : nullptr; }
	int32 GetCurrentIndex() const { return CurrentIndex; }
	/** Segundos desde que cambió el objetivo (el HUD lo resalta al principio). */
	float GetObjectiveAge() const { return ObjectiveAge; }
	bool GetMarkerLocation(FVector& Out) const;
	/** Subtítulo actual; Alpha para el fundido. false si no hay. */
	bool GetSubtitle(FString& OutSpeaker, FString& OutText, float& OutAlpha) const;
	bool IsMissionComplete() const { return bComplete; }
	float GetCompleteAge() const { return CompleteAge; }
	/** Frases de radio con voz que han empezado a sonar (pruebas). */
	int32 GetVoiceLinesPlayed() const { return VoiceLinesPlayed; }
	bool IsVoicePlaying() const;

	// ---- Resumen ----
	float GetMissionTime() const { return bComplete ? FinalTime : ElapsedTime; }
	int32 GetKills() const { return Kills; }
	int32 GetShots() const;
	int32 GetHits() const { return Hits; }
	int32 GetHeadshots() const { return Headshots; }
	int32 GetDeaths() const { return Deaths; }

	/** Fuerza que se cumpla el objetivo actual (pruebas y depuración). */
	void CompleteCurrentObjective();
	void RestartMission();

private:
	void StartObjective(int32 Index);
	/** ForObjective: objetivo al que pertenecen las frases (INDEX_NONE = briefing/cierre/cumplido: no se descartan). */
	void QueueRadio(const TArray<FBLRadioLine>& Lines, int32 ForObjective = INDEX_NONE);
	void TickRadio(float DeltaTime);
	bool IsObjectiveDone(const FBLObjective& O) const;
	AActor* FindTarget(FName Tag) const;
	ABLCharacter* Player() const;

	UFUNCTION() void HandleInteract(ABLInteractable* Interactable, ABLCharacter* User);
	UFUNCTION() void HandleHit(EBLHitZone Zone, float Damage, bool bKilled);
	UFUNCTION() void HandlePlayerDeath(const struct FBLDamageInfo& Info);

	int32 CurrentIndex = INDEX_NONE;
	float ObjectiveAge = 0.f;
	float ElapsedTime = 0.f;
	float StartDelay = 1.5f;
	bool bStarted = false;
	bool bInteractDone = false;
	bool bComplete = false;
	bool bAllObjectivesDone = false;
	float CompleteAge = 0.f;
	float FinalTime = 0.f;

	TArray<FBLRadioLine> RadioQueue;
	TArray<int32> RadioOwners;
	FBLRadioLine CurrentLine;
	float LineTime = 0.f;
	float LineDuration = 0.f;
	bool bLineActive = false;
	float RadioGap = 0.f;
	/** La voz entra un poco después del clic de apertura. */
	float VoiceDelay = -1.f;
	int32 VoiceLinesPlayed = 0;
	UPROPERTY() TObjectPtr<UAudioComponent> VoiceComponent;

	int32 Kills = 0;
	int32 Hits = 0;
	int32 Headshots = 0;
	int32 Deaths = 0;
	int32 ShotsAtStart = 0;
	bool bBoundPlayer = false;
};
