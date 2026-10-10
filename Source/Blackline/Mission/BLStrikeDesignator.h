#pragma once

#include "CoreMinimal.h"
#include "Mission/BLInteractable.h"
#include "BLStrikeDesignator.generated.h"

class USoundBase;

/**
 * Designador láser en trípode (misión 4). Con el blindado (TargetTag) a la vista, F mantenido lo marca: TORRE pide
 * el apoyo, a los Delay segundos pasan los cazas (actores con JetTag: ABLScriptedMover) y StrikeDelay después cae
 * la pasada: explosiones encadenadas sobre el blindado y ABLBTR::DestroyVehicle.
 */
UCLASS()
class BLACKLINE_API ABLStrikeDesignator : public ABLInteractable
{
	GENERATED_BODY()

public:
	ABLStrikeDesignator();

	virtual void Tick(float DeltaTime) override;
	virtual void Use(ABLCharacter* User) override;
	virtual bool CanInteract(const ABLCharacter* User) const override;
	/** El visor del designador, no el pie del trípode. */
	virtual FVector GetInteractLocation() const override { return GetActorLocation() + FVector(0.f, 0.f, 130.f); }

	UPROPERTY(EditAnywhere, Category = "Strike") FName TargetTag = TEXT("BLBTR");
	UPROPERTY(EditAnywhere, Category = "Strike") FName JetTag = TEXT("BLJets");
	UPROPERTY(EditAnywhere, Category = "Strike") float Delay = 4.f;
	UPROPERTY(EditAnywhere, Category = "Strike") float StrikeDelay = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Strike") TArray<FBLRadioLine> MarkRadio;
	UPROPERTY(EditAnywhere, Category = "Strike") TArray<TObjectPtr<USoundBase>> BlastSounds;

	bool HasStruck() const { return bStruck; }

private:
	AActor* FindTarget() const;

	float Timer = -1.f;
	int32 Blasts = 0;
	float NextBlast = 0.f;
	bool bJets = false;
	bool bStruck = false;
};
