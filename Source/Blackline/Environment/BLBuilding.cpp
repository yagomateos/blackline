#include "Environment/BLBuilding.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Medidas del kit (gen_facade_kit.py). */
	constexpr float KitWidth = 400.f;
	constexpr float KitFloor = 320.f;
	constexpr float WallDepth = 25.f;

	struct FPieceDef
	{
		const TCHAR* Mesh;
		bool bCollision;
	};
	const FPieceDef PieceDefs[] = {
		{ TEXT("SM_Fac_Window"), true }, { TEXT("SM_Fac_WindowWide"), true }, { TEXT("SM_Fac_BalconyDoor"), true },
		{ TEXT("SM_Fac_Blank"), true }, { TEXT("SM_Fac_Balcony"), false }, { TEXT("SM_Fac_Shutter"), false },
		{ TEXT("SM_Fac_ShopClosed"), true }, { TEXT("SM_Fac_ShopFront"), true }, { TEXT("SM_Fac_Door"), true },
		{ TEXT("SM_Fac_GroundBlank"), true }, { TEXT("SM_Fac_Band"), false }, { TEXT("SM_Fac_Cornice"), true },
		{ TEXT("SM_Fac_Pipe"), false }, { TEXT("SM_Fac_AC"), false }, { TEXT("SM_Fac_SignBlade"), false },
	};
	static_assert(UE_ARRAY_COUNT(PieceDefs) == (int32)EBLFacadePiece::MAX, "Faltan piezas del kit");

	/** Piezas cuya ranura 0 es el muro del edificio. */
	bool UsesWall(EBLFacadePiece P)
	{
		return P <= EBLFacadePiece::Blank || (P >= EBLFacadePiece::ShopClosed && P <= EBLFacadePiece::GroundBlank) || P == EBLFacadePiece::Cornice;
	}

	enum class EColumn : uint8 { Window, Wide, Balcony, Blank };
}

ABLBuilding::ABLBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	Core->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Core->SetStaticMesh(Cube.Object);
	Core->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	for (int32 i = 0; i < (int32)EBLFacadePiece::MAX; ++i)
	{
		const FPieceDef& Def = PieceDefs[i];
		UInstancedStaticMeshComponent* ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Def.Mesh);
		ISM->SetupAttachment(RootComponent);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(*FString::Printf(TEXT("/Game/Environment/Facade/%s.%s"), Def.Mesh, Def.Mesh));
		if (Mesh.Succeeded())
		{
			ISM->SetStaticMesh(Mesh.Object);
		}
		ISM->SetCollisionProfileName(Def.bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		ISM->SetCanEverAffectNavigation(false);    // la navegación la da el núcleo
		Pieces.Add(ISM);
	}
}

void ABLBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

int32 ABLBuilding::GetInstanceCount() const
{
	int32 N = 0;
	for (const UInstancedStaticMeshComponent* ISM : Pieces)
	{
		N += ISM ? ISM->GetInstanceCount() : 0;
	}
	return N;
}

void ABLBuilding::AddPiece(EBLFacadePiece Piece, const FTransform& Local)
{
	if (UInstancedStaticMeshComponent* ISM = Pieces[(int32)Piece])
	{
		ISM->AddInstance(Local, false);
	}
}

void ABLBuilding::Rebuild()
{
	for (UInstancedStaticMeshComponent* ISM : Pieces)
	{
		ISM->ClearInstances();
		if (WallMaterial && UsesWall((EBLFacadePiece)Pieces.IndexOfByKey(ISM)))
		{
			ISM->SetMaterial(0, WallMaterial);
		}
	}
	// Núcleo: 25 cm por dentro de la fachada (allí van las ventanas y sus derrames)
	const FVector CoreSize(FMath::Max(Size.X - 2.f * WallDepth, 10.f), FMath::Max(Size.Y - 2.f * WallDepth, 10.f), Size.Z);
	Core->SetRelativeLocation(FVector(0.f, 0.f, Size.Z * 0.5f));
	Core->SetRelativeScale3D(CoreSize / 100.f);
	if (WallMaterial)
	{
		Core->SetMaterial(0, WallMaterial);
	}
	Core->SetPhysMaterialOverride(PhysMaterial);

	FRandomStream Rng(Seed);
	const int32 Floors = FMath::Max(1, FMath::RoundToInt(Size.Z / FloorHeight));
	const float FH = Size.Z / Floors;
	const float SZ = FH / KitFloor;

	// Caras: (normal hacia fuera, longitud, mitad de la profundidad)
	struct FFace { FVector N; float Length; float Half; };
	const FFace Faces[4] = {
		{ FVector(1, 0, 0), Size.Y, Size.X * 0.5f }, { FVector(-1, 0, 0), Size.Y, Size.X * 0.5f },
		{ FVector(0, 1, 0), Size.X, Size.Y * 0.5f }, { FVector(0, -1, 0), Size.X, Size.Y * 0.5f },
	};
	for (int32 F = 0; F < 4; ++F)
	{
		if (!(FaceMask & (1 << F)))
		{
			continue;
		}
		const FFace& Face = Faces[F];
		const bool bStreet = (StreetMask & (1 << F)) != 0;
		const FRotator Rot = Face.N.Rotation();
		const FVector Along = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);   // +Y local del módulo
		const FVector Origin = Face.N * Face.Half;
		const int32 Mods = FMath::Max(1, FMath::RoundToInt(Face.Length / ModuleWidth));
		const float MW = Face.Length / Mods;
		const float SY = MW / KitWidth;
		auto At = [&](float T, float Z, float X = 0.f) { return Origin + Along * T + Face.N * X + FVector(0.f, 0.f, Z); };

		// Remates que recorren toda la cara (un poco más largos para cerrar la esquina)
		if (CorniceMask & (1 << F))
		{
			AddPiece(EBLFacadePiece::Cornice, FTransform(Rot, At(0.f, Size.Z), FVector(1.f, (Face.Length + 30.f) / KitWidth, 1.f)));
		}
		for (int32 K = 1; K < Floors; ++K)
		{
			AddPiece(EBLFacadePiece::Band, FTransform(Rot, At(0.f, K * FH), FVector(1.f, (Face.Length + 16.f) / KitWidth, 1.f)));
		}

		// Planta baja: un portal como mínimo en cada cara a la calle
		const int32 DoorAt = bStreet ? Rng.RandRange(0, Mods - 1) : -1;
		for (int32 M = 0; M < Mods; ++M)
		{
			const float T = -Face.Length * 0.5f + MW * (M + 0.5f);
			const FVector Scale(1.f, SY, SZ);

			EBLFacadePiece Ground = EBLFacadePiece::GroundBlank;
			const float R = Rng.FRand();
			if (M == DoorAt)
			{
				Ground = EBLFacadePiece::Door;
			}
			else if (bStreet)
			{
				Ground = R < 0.45f ? EBLFacadePiece::ShopClosed : R < 0.62f ? EBLFacadePiece::ShopFront : R < 0.75f ? EBLFacadePiece::Door : EBLFacadePiece::GroundBlank;
			}
			else if (R < 0.2f)
			{
				Ground = EBLFacadePiece::Door;
			}
			if (!bNoGroundFloor)
			{
				AddPiece(Ground, FTransform(Rot, At(T, 0.f), Scale));
			}
			if (!bNoGroundFloor && (Ground == EBLFacadePiece::ShopClosed || Ground == EBLFacadePiece::ShopFront) && Rng.FRand() < 0.3f)
			{
				AddPiece(EBLFacadePiece::SignBlade, FTransform(Rot, At(T + MW * 0.5f - 15.f, FH * 1.02f), FVector(1.f)));
			}

			// Pisos: el tipo de columna se mantiene de abajo arriba
			const float C = Rng.FRand();
			const EColumn Column = C < 0.55f ? EColumn::Window : C < 0.7f ? EColumn::Wide : C < 0.9f ? EColumn::Balcony : EColumn::Blank;
			for (int32 K = bNoGroundFloor ? 0 : 1; K < Floors; ++K)
			{
				const float Z = K * FH;
				switch (Column)
				{
				case EColumn::Window: AddPiece(EBLFacadePiece::Window, FTransform(Rot, At(T, Z), Scale)); break;
				case EColumn::Wide: AddPiece(EBLFacadePiece::WindowWide, FTransform(Rot, At(T, Z), Scale)); break;
				case EColumn::Balcony:
					AddPiece(EBLFacadePiece::BalconyDoor, FTransform(Rot, At(T, Z), Scale));
					AddPiece(EBLFacadePiece::Balcony, FTransform(Rot, At(T, Z), FVector(1.f, SY, 1.f)));
					break;
				default: AddPiece(EBLFacadePiece::Blank, FTransform(Rot, At(T, Z), Scale)); break;
				}
				if (Column != EColumn::Blank)
				{
					// Persiana: la mayoría a medio bajar, algunas cerradas o subidas del todo
					const float Drop = Rng.FRand();
					const float Frac = Drop < 0.25f ? 0.f : Drop < 0.55f ? Rng.FRandRange(0.1f, 0.35f) : Drop < 0.85f ? Rng.FRandRange(0.4f, 0.75f) : 1.f;
					const float OpeningH = Column == EColumn::Balcony ? 240.f : 150.f;
					const float OpeningW = Column == EColumn::Wide ? 200.f : Column == EColumn::Balcony ? 120.f : 140.f;
					if (Frac > 0.02f)
					{
						AddPiece(EBLFacadePiece::Shutter, FTransform(Rot, At(T, Z + 222.f * SZ),
							FVector(1.f, OpeningW / 140.f * SY, (OpeningH - 18.f) * Frac / 100.f * SZ)));
					}
					if (Column != EColumn::Balcony && Rng.FRand() < 0.14f)
					{
						const float Side = Rng.FRand() < 0.5f ? -1.f : 1.f;
						AddPiece(EBLFacadePiece::AC, FTransform(Rot, At(T + Side * (OpeningW * 0.5f + 60.f) * SY, Z + 40.f), FVector(1.f)));
					}
				}
			}
			// Bajantes cada tres módulos
			if (M % 3 == 2 && M < Mods - 1)
			{
				AddPiece(EBLFacadePiece::Pipe, FTransform(Rot, At(T + MW * 0.5f, 0.f), FVector(1.f, 1.f, (Size.Z - 10.f) / 100.f)));
			}
		}
	}
}
