#pragma once

#include "CoreMinimal.h"
#include "Mission/BLInteractable.h"
#include "BLDoor.generated.h"

class USoundBase;
class UStaticMeshComponent;

/**
 * Puerta con bisagra (misión 3, CQB casa a casa). El actor está en la bisagra y la hoja (Mesh) se extiende hacia +Y.
 *  - F: se abre empujando hacia el lado contrario al jugador. Si se llega corriendo, de una patada: se abre de golpe,
 *    hace ruido y tira (aturde 1,2 s) a quien esté justo detrás.
 *  - bBarred: atrancada. F coloca una carga de brecha; a los 3 s revienta: la hoja sale volando, polvo, y los de
 *    la habitación quedan aturdidos (StunTime) sin poder disparar. Quien esté pegado a la puerta, herido.
 *  - La IA abre las puertas normales al llegar a ellas (el NavMesh no las ve: la hoja no afecta a la navegación).
 */
UCLASS()
class BLACKLINE_API ABLDoor : public ABLInteractable
{
	GENERATED_BODY()

public:
	ABLDoor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Use(ABLCharacter* User) override;
	virtual bool CanInteract(const ABLCharacter* User) const override;
	virtual FVector GetInteractLocation() const override { return GetCenter(); }

	UPROPERTY(EditAnywhere, Category = "Door") bool bBarred = false;
	UPROPERTY(EditAnywhere, Category = "Door") float OpenAngle = 100.f;
	UPROPERTY(EditAnywhere, Category = "Door") float StunRadius = 750.f;
	UPROPERTY(EditAnywhere, Category = "Door") float StunTime = 3.5f;
	UPROPERTY(EditAnywhere, Category = "Door") float ChargeDelay = 3.f;
	UPROPERTY(EditAnywhere, Category = "Door") TObjectPtr<USoundBase> OpenSound;
	UPROPERTY(EditAnywhere, Category = "Door") TObjectPtr<USoundBase> KickSound;
	UPROPERTY(EditAnywhere, Category = "Door") TObjectPtr<USoundBase> BeepSound;
	UPROPERTY(EditAnywhere, Category = "Door") TObjectPtr<USoundBase> BlastSound;

	bool IsOpen() const { return bOpen; }
	bool IsBreached() const { return bBreached; }
	bool IsChargeArmed() const { return ChargeTime >= 0.f; }

private:
	void Open(const FVector& From, bool bKick);
	void Detonate();
	FVector GetCenter() const;

	UPROPERTY(VisibleAnywhere, Category = "Door") TObjectPtr<UStaticMeshComponent> Charge;

	FRotator ClosedRotation = FRotator::ZeroRotator;
	float Angle = 0.f;
	float TargetAngle = 0.f;
	float Speed = 120.f;
	float ChargeTime = -1.f;
	float NextBeep = 0.f;
	float AITimer = 0.f;
	bool bOpen = false;
	bool bBreached = false;
	TWeakObjectPtr<ABLCharacter> Breacher;
};
