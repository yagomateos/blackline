#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Chaos/ChaosEngineInterface.h"
#include "BLSurfaceEffects.generated.h"

class USoundBase;
class UMaterialInterface;
class UStaticMesh;

/**
 * Superficies físicas del proyecto (DefaultEngine.ini, PhysicalSurfaces):
 *   SurfaceType1 Concrete · 2 Metal · 3 Wood · 4 Glass · 5 Dirt · 6 Flesh. Default = Concrete.
 */
namespace BLSurface
{
	constexpr EPhysicalSurface Concrete = SurfaceType1;
	constexpr EPhysicalSurface Metal = SurfaceType2;
	constexpr EPhysicalSurface Wood = SurfaceType3;
	constexpr EPhysicalSurface Glass = SurfaceType4;
	constexpr EPhysicalSurface Dirt = SurfaceType5;
	constexpr EPhysicalSurface Flesh = SurfaceType6;
}

/** Cómo reacciona una superficie a un impacto de bala, a un casquillo y a una pisada. */
USTRUCT(BlueprintType)
struct FBLSurfaceEffect
{
	GENERATED_BODY()

	// ---- Sonido ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> ImpactSounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> CasingSounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> FootstepSounds;

	// ---- Marca ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal") TObjectPtr<UMaterialInterface> Decal;
	/** Radio de la marca (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal") float DecalSize = 5.f;
	/** Salpicadura en la superficie que hay detrás (sangre en la pared): se busca a lo largo del disparo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal") TObjectPtr<UMaterialInterface> SplatterDecal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal") float SplatterDistance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal") float SplatterSize = 25.f;

	// ---- Polvo / humo (sprites iluminados) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") int32 DustCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") FLinearColor DustColor = FLinearColor(0.6f, 0.58f, 0.55f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") float DustOpacity = 0.6f;
	/** Tamaño inicial y final (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") FVector2D DustSize = FVector2D(10.f, 70.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") FVector2D DustLife = FVector2D(0.8f, 1.8f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") float DustSpeed = 150.f;
	/** Chorro rápido y fino a lo largo de la normal (la "nube" inicial del impacto). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") int32 JetCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dust") float JetSpeed = 600.f;

	// ---- Chispas (aditivas) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sparks") int32 SparkCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sparks") float SparkSpeed = 1200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sparks") FLinearColor SparkColor = FLinearColor(1.f, 0.55f, 0.18f);

	// ---- Escombros (mallas con física simple) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris") TObjectPtr<UStaticMesh> DebrisMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris") int32 DebrisCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris") float DebrisSpeed = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris") FVector2D DebrisScale = FVector2D(0.6f, 1.4f);
};

/** Tabla de efectos por superficie (DA_SurfaceEffects). */
UCLASS(BlueprintType)
class BLACKLINE_API UBLSurfaceEffectsData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") FBLSurfaceEffect Concrete;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") FBLSurfaceEffect Metal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") FBLSurfaceEffect Wood;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") FBLSurfaceEffect Glass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") FBLSurfaceEffect Dirt;
	/** Personajes (cualquier malla esquelética de un Pawn). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") FBLSurfaceEffect Flesh;

	const FBLSurfaceEffect& Get(EPhysicalSurface Surface) const
	{
		switch (Surface)
		{
		case BLSurface::Metal: return Metal;
		case BLSurface::Wood: return Wood;
		case BLSurface::Glass: return Glass;
		case BLSurface::Dirt: return Dirt;
		case BLSurface::Flesh: return Flesh;
		default: return Concrete; // hormigón por defecto (y materiales sin superficie asignada)
		}
	}

	/** Superficie de un impacto: material físico, override del componente o material (y sus padres). */
	static EPhysicalSurface ResolveSurface(const FHitResult& Hit);

	/** Nombre legible (pruebas / depuración). */
	static const TCHAR* Name(EPhysicalSurface Surface)
	{
		switch (Surface)
		{
		case BLSurface::Metal: return TEXT("Metal");
		case BLSurface::Wood: return TEXT("Madera");
		case BLSurface::Glass: return TEXT("Cristal");
		case BLSurface::Dirt: return TEXT("Tierra");
		case BLSurface::Flesh: return TEXT("Carne");
		default: return TEXT("Hormigon");
		}
	}
};
