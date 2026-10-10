#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mission/BLMissionTypes.h"
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

	virtual bool CanInteract(const ABLCharacter* User) const { return bEnabled && !bUsed; }
	/** Punto al que se mira para usarlo (las puertas: el centro de la hoja, no la bisagra). */
	virtual FVector GetInteractLocation() const { return GetActorLocation(); }
	/** Lo usa el jugador (las puertas lo redefinen: abrir, patada, carga de brecha). */
	virtual void Use(ABLCharacter* User);
	bool IsUsed() const { return bUsed; }

	/** Texto de la acción: "Recoger el disco duro". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") FString Prompt = TEXT("Usar");
	/** Segundos manteniendo el botón (0 = instantáneo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") float HoldTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") bool bEnabled = true;
	/** Si se recoge, la malla desaparece al usarlo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") bool bPickup = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") TObjectPtr<USoundBase> UseSound;
	/** Fotografiar (misión 2): obturador y destello en el HUD en vez del sonido de recoger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") bool bPhoto = false;
	/** Radio al usarlo (lo reproduce el director: "Cuatro cuatro siete uno. Cajas sin marcas."). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") TArray<FBLRadioLine> UseRadio;

	/** Momento (segundos de mundo) de la última foto, para el destello del HUD; < 0 si no hay. */
	static float GetLastPhotoTime() { return LastPhotoTime; }
	static int32 GetPhotosTaken() { return PhotosTaken; }

	UPROPERTY(BlueprintAssignable, Category = "Interact") FBLInteractSignature OnUsed;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool bUsed = false;
	static float LastPhotoTime;
	static int32 PhotosTaken;
};
