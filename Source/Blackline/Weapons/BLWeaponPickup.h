#pragma once

#include "CoreMinimal.h"
#include "Mission/BLInteractable.h"
#include "Weapons/BLWeaponComponent.h"
#include "BLWeaponPickup.generated.h"

class USkeletalMeshComponent;
class UBLWeaponData;

/**
 * Arma en el suelo que se recoge con F. Si el jugador ya la lleva, coge su munición; si lleva menos de dos armas,
 * la añade; si no, la cambia por la que tiene en la mano, que queda en el suelo en su lugar (con su munición).
 */
UCLASS()
class BLACKLINE_API ABLWeaponPickup : public ABLInteractable
{
	GENERATED_BODY()

public:
	ABLWeaponPickup();

	/** Armas que se pueden llevar a la vez. */
	static constexpr int32 MaxCarried = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon") TObjectPtr<UBLWeaponData> WeaponData;
	/** Munición del arma tirada (-1 = cargador lleno / reserva inicial del arma). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon") int32 Magazine = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon") int32 Reserve = -1;

	/** Distancia (cm, al centro del jugador) a la que se coge sola la munición de un arma que ya se lleva. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon") float AutoAmmoRadius = 140.f;

	/** Deja un arma en el suelo delante de Dropper (cambio de arma). */
	static ABLWeaponPickup* SpawnDrop(AActor* Dropper, const FBLWeaponSlot& Slot, const FVector& Near);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool CanInteract(const ABLCharacter* User) const override;
	virtual FVector GetInteractLocation() const override;
	virtual void Use(ABLCharacter* User) override;

	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

private:
	void ApplyMesh();
	void UpdatePrompt();

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USkeletalMeshComponent> WeaponMesh;
};
