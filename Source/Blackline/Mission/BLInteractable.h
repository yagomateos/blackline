#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLInteractable.generated.h"

class UStaticMeshComponent;
class USoundBase;
class ABLCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBLInteractSignature, class ABLInteractable*, Interactable, ABLCharacter*, User);

/**
 * Objeto con el que se interactúa con un botón (F): se mira, aparece "[F] <Prompt>" y se mantiene HoldTime.
 * El de la misión 1 es el disco duro de Varek (se recoge: la malla desaparece).
 */
UCLASS()
class BLACKLINE_API ABLInteractable : public AActor
{
	GENERATED_BODY()

public:
	ABLInteractable();

	bool CanInteract(const ABLCharacter* User) const { return bEnabled && !bUsed; }
	void Use(ABLCharacter* User);
	bool IsUsed() const { return bUsed; }

	/** Texto de la acción: "Recoger el disco duro". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") FString Prompt = TEXT("Usar");
	/** Segundos manteniendo el botón (0 = instantáneo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") float HoldTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") bool bEnabled = true;
	/** Si se recoge, la malla desaparece al usarlo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") bool bPickup = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") TObjectPtr<USoundBase> UseSound;

	UPROPERTY(BlueprintAssignable, Category = "Interact") FBLInteractSignature OnUsed;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool bUsed = false;
};
