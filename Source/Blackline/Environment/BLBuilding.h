#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BLBuilding.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPhysicalMaterial;
class UStaticMesh;
class UStaticMeshComponent;

/** Piezas del kit de fachadas (gen_facade_kit.py). */
UENUM()
enum class EBLFacadePiece : uint8
{
	Window, WindowWide, BalconyDoor, Blank, Balcony, Shutter,
	ShopClosed, ShopFront, Door, GroundBlank,
	Band, Cornice, Pipe, AC, SignBlade,
	MAX UMETA(Hidden),
};

/**
 * Edificio de fachadas modulares (Bloque 8). Un núcleo macizo (colisión y navegación) y, por fuera, las fachadas
 * con instancias del kit: plantas de 320 cm, módulos de ~400 cm que se estiran para cubrir cada cara exacta.
 * Las columnas de ventanas mantienen el tipo de abajo arriba (como un edificio real); persianas a distintas
 * alturas, balcones, aires acondicionados, bajantes, imposta por planta, cornisa con peto, y en la planta baja
 * locales con persiana o escaparate, portales y paños ciegos (más locales en las caras a la calle).
 * Todo sale de Seed: el mismo edificio se genera igual siempre. Nanite hace barato el número de instancias.
 *
 * Coordenadas: el actor está en el centro de la base; Size = medidas exteriores (cm). Caras: bit 0 = +X, 1 = -X, 2 = +Y, 3 = -Y.
 */
UCLASS()
class BLACKLINE_API ABLBuilding : public AActor
{
	GENERATED_BODY()

public:
	ABLBuilding();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Building") FVector Size = FVector(1600.f, 1200.f, 960.f);
	UPROPERTY(EditAnywhere, Category = "Building") int32 Seed = 1;
	/** Caras con fachada (las que tocan otro edificio no se ven). */
	UPROPERTY(EditAnywhere, Category = "Building") int32 FaceMask = 15;
	/** Caras que dan a una calle (más locales y portales en la planta baja). */
	UPROPERTY(EditAnywhere, Category = "Building") int32 StreetMask = 15;
	/** Caras con peto de cubierta (las que no lo llevan dejan la azotea abierta: pasarelas, escaleras de incendios). */
	UPROPERTY(EditAnywhere, Category = "Building") int32 CorniceMask = 15;
	UPROPERTY(EditAnywhere, Category = "Building") float FloorHeight = 320.f;
	/** Todas las plantas son de pisos (para edificios montados encima de una planta baja hecha aparte). */
	UPROPERTY(EditAnywhere, Category = "Building") bool bNoGroundFloor = false;
	UPROPERTY(EditAnywhere, Category = "Building") float ModuleWidth = 400.f;
	/** Material del muro (ranura 0 de los módulos y núcleo). */
	UPROPERTY(EditAnywhere, Category = "Building") TObjectPtr<UMaterialInterface> WallMaterial;
	UPROPERTY(EditAnywhere, Category = "Building") TObjectPtr<UPhysicalMaterial> PhysMaterial;

	UFUNCTION(BlueprintCallable, Category = "Building") int32 GetInstanceCount() const;

private:
	void Rebuild();
	void AddPiece(EBLFacadePiece Piece, const FTransform& Local);

	UPROPERTY(VisibleAnywhere, Category = "Building") TObjectPtr<UStaticMeshComponent> Core;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Pieces;
};
