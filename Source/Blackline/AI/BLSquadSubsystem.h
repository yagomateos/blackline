#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BLSquadSubsystem.generated.h"

class ABLAIController;
class ABLCoverPoint;

/**
 * Coordinador de la IA (Bloque 5). Evita que todos actúen igual a la vez:
 *  - Turnos de ataque: como mucho MaxAttackTokens enemigos disparan al jugador a la vez; el resto se cubre o se mueve.
 *  - Alerta compartida: quien detecta al jugador avisa a su escuadra y a los cercanos (AlertRadius).
 *  - Reservas de cobertura: dos enemigos no eligen el mismo punto.
 *  - Flanqueo: con 3 o más en combate, cada cierto tiempo uno pasa a flanquear.
 */
UCLASS()
class BLACKLINE_API UBLSquadSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBLSquadSubsystem, STATGROUP_Tickables); }

	void Register(ABLAIController* AI);
	void Unregister(ABLAIController* AI);

	/** Pide permiso para disparar. true si lo tiene (o ya lo tenía). */
	bool RequestAttackToken(ABLAIController* AI);
	void ReleaseAttackToken(ABLAIController* AI);
	bool HasAttackToken(const ABLAIController* AI) const { return TokenHolders.Contains(AI); }

	bool IsCoverFree(const ABLCoverPoint* Cover, const ABLAIController* For) const;
	void ReserveCover(ABLCoverPoint* Cover, ABLAIController* AI);
	void ReleaseCover(ABLAIController* AI);

	/** Un enemigo ha visto/oído al jugador: avisa a los suyos. */
	void ReportTarget(ABLAIController* From, AActor* Target, const FVector& Location);

	/** Dirección media desde la que la escuadra ataca al objetivo (para elegir flancos). */
	FVector GetMainAttackDirection(const AActor* Target) const;

	const TArray<TWeakObjectPtr<ABLAIController>>& GetMembers() const { return Members; }
	int32 GetActiveTokens() const { return TokenHolders.Num(); }
	int32 GetMaxTokensSeen() const { return MaxTokensSeen; }
	int32 GetFlankAssignments() const { return FlankAssignments; }

	UPROPERTY(EditAnywhere, Category = "Squad") int32 MaxAttackTokens = 3;
	UPROPERTY(EditAnywhere, Category = "Squad") float AlertRadius = 3500.f;
	UPROPERTY(EditAnywhere, Category = "Squad") float FlankInterval = 14.f;

private:
	TArray<TWeakObjectPtr<ABLAIController>> Members;
	TArray<TObjectPtr<ABLAIController>> TokenHolders;
	TMap<TWeakObjectPtr<ABLCoverPoint>, TWeakObjectPtr<ABLAIController>> Reservations;
	float FlankTimer = 6.f;
	int32 MaxTokensSeen = 0;
	int32 FlankAssignments = 0;
};
