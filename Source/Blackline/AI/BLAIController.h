#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Audio/BLAudioSubsystem.h"
#include "BLAIController.generated.h"

class ABLEnemyCharacter;
class ABLCoverPoint;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBLSquadSubsystem;

UENUM(BlueprintType)
enum class EBLAIState : uint8
{
	Patrol,       // ruta de patrulla o centinela
	Suspicious,   // ha visto algo: se para y mira mientras la consciencia sube o baja
	Investigate,  // va a ver un ruido o lo último que vio
	Combat,       // conoce al jugador: cobertura, disparo, flanqueo
	Search,       // lo perdió: busca alrededor de la última posición conocida
	Dead,
};

UENUM(BlueprintType)
enum class EBLCombatAction : uint8
{
	None,
	MoveToCover,
	InCover,      // escondido / asomándose a disparar
	Hold,         // sin cobertura útil: dispara desde donde está
	Flank,        // se mueve a una cobertura por el lateral sin disparar
	Chase,        // va a la última posición conocida
};

/**
 * IA del miliciano (Bloque 5). Máquina de estados en C++ (en vez de StateTree: así se genera y se prueba
 * por script, como la animación) con AI Perception (vista, oído, daño), coberturas puntuadas en C++ sobre
 * ABLCoverPoint (en vez de EQS) y coordinación por UBLSquadSubsystem (turnos de disparo, alertas, flanqueo).
 */
UCLASS()
class BLACKLINE_API ABLAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABLAIController(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;

	/** Aviso de la escuadra: alguien ha localizado al objetivo. */
	void OnSquadAlert(AActor* InTarget, const FVector& Location);
	bool IsInCombat() const { return State == EBLAIState::Combat; }
	bool IsFlanker() const { return Action == EBLCombatAction::Flank; }
	bool CanStartFlank() const { return State == EBLAIState::Combat && Action == EBLCombatAction::InCover && !bReloading; }
	/** Intenta flanquear: busca una cobertura lateral respecto a la escuadra. false si no hay ninguna válida. */
	bool StartFlank();

	// ---- Estado (pruebas, depuración) ----
	EBLAIState GetState() const { return State; }
	EBLCombatAction GetAction() const { return Action; }
	float GetAwareness() const { return Awareness; }
	AActor* GetTarget() const { return Target.Get(); }
	bool IsTargetVisible() const { return bTargetVisible; }
	FVector GetLastKnownLocation() const { return LastKnownLocation; }
	ABLCoverPoint* GetCover() const { return Cover.Get(); }
	bool IsAtCover() const { return bAtCover; }
	bool IsPeeking() const { return bPeeking; }
	int32 GetShotsFired() const { return ShotsFired; }
	int32 GetReloads() const { return Reloads; }
	int32 GetFlanksDone() const { return FlanksDone; }
	static const TCHAR* StateName(EBLAIState S);
	static const TCHAR* ActionName(EBLCombatAction A);

	// ---- Ajustes ----
	UPROPERTY(EditAnywhere, Category = "AI|Perception") float SightRadius = 5000.f;
	UPROPERTY(EditAnywhere, Category = "AI|Perception") float PeripheralAngle = 70.f;
	UPROPERTY(EditAnywhere, Category = "AI|Perception") float HearingRange = 6000.f;
	/** Consciencia por segundo viendo al jugador a 0 m; baja con la distancia (y agachado). */
	UPROPERTY(EditAnywhere, Category = "AI|Perception") float AwarenessRate = 2.6f;
	/** Error de puntería (cm) al empezar a ver al objetivo y mínimo tras apuntar un rato. */
	UPROPERTY(EditAnywhere, Category = "AI|Combat") float AimErrorMax = 170.f;
	UPROPERTY(EditAnywhere, Category = "AI|Combat") float AimErrorMin = 28.f;
	/** Segundos sin ver al objetivo antes de ir a buscarlo. */
	UPROPERTY(EditAnywhere, Category = "AI|Combat") float ChaseDelay = 9.f;
	UPROPERTY(EditAnywhere, Category = "AI|Combat") float SearchTime = 14.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, Category = "AI") TObjectPtr<UAIPerceptionComponent> Perception;

private:
	UFUNCTION() void HandlePerception(AActor* Actor, FAIStimulus Stimulus);
	UFUNCTION() void HandleDamaged(const struct FBLDamageInfo& Info);

	void SetState(EBLAIState NewState);
	void SetAction(EBLCombatAction NewAction);
	void EnterCombat(AActor* InTarget, const FVector& Location, bool bAlertSquad);

	void TickPerception(float DeltaTime);
	void TickPatrol(float DeltaTime);
	void TickSuspicious(float DeltaTime);
	void TickInvestigate(float DeltaTime);
	void TickCombat(float DeltaTime);
	void TickSearch(float DeltaTime);
	void TickShooting(float DeltaTime, bool bAllowed);

	ABLCoverPoint* FindCover(bool bFlank) const;
	bool MoveToPoint(const FVector& Location, bool bJog);
	bool HasArrived(float Radius = 60.f) const;
	FVector TargetEye() const;
	FVector TargetChest() const;
	bool CheckLineOfSight() const;
	void Bark(EBLBark Type, const TCHAR* Line);
	void LookAround(float DeltaTime, float Range);

	ABLEnemyCharacter* Enemy() const;
	UBLSquadSubsystem* Squad() const;

	EBLAIState State = EBLAIState::Patrol;
	EBLCombatAction Action = EBLCombatAction::None;
	float StateTime = 0.f;
	float ActionTime = 0.f;

	TWeakObjectPtr<AActor> Target;
	FVector LastKnownLocation = FVector::ZeroVector;
	FVector InvestigateLocation = FVector::ZeroVector;
	bool bTargetVisible = false;
	bool bSeenByPerception = false;
	float TimeSinceSeen = 100.f;
	float LosTimer = 0.f;
	float Awareness = 0.f;

	// Patrulla y búsqueda
	int32 PatrolIndex = 0;
	float WaitTimer = 0.f;
	FRotator HomeRotation = FRotator::ZeroRotator;
	FVector HomeLocation = FVector::ZeroVector;
	float LookYaw = 0.f;
	float LookTimer = 0.f;
	TArray<FVector> SearchPoints;

	// Combate
	TWeakObjectPtr<ABLCoverPoint> Cover;
	bool bAtCover = false;
	bool bPeeking = false;
	FVector FirePosition = FVector::ZeroVector;
	bool bFireStand = true;
	float CoverTime = 0.f;
	float CoverMaxTime = 15.f;
	float PhaseTimer = 0.f;
	float AimError = 170.f;
	FVector AimOffset = FVector::ZeroVector;
	int32 BurstLeft = 0;
	float BurstPause = 0.f;
	int32 LastWeaponShots = 0;
	bool bReloading = false;
	float RepathTimer = 0.f;

	// Estadísticas
	int32 ShotsFired = 0;
	int32 Reloads = 0;
	int32 FlanksDone = 0;
};
