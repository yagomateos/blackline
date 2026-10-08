#include "FX/BLSurfaceEffects.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstance.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

EPhysicalSurface UBLSurfaceEffectsData::ResolveSurface(const FHitResult& Hit)
{
	// 0) Personajes: su malla esquelética es carne (los Physics Asset no llevan material físico propio)
	if (const USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Hit.GetComponent()))
	{
		if (Cast<APawn>(Skel->GetOwner()))
		{
			return BLSurface::Flesh;
		}
	}
	// 1) Material físico devuelto por la colisión
	if (const UPhysicalMaterial* PM = Hit.PhysMaterial.Get())
	{
		if (PM->SurfaceType != SurfaceType_Default)
		{
			return PM->SurfaceType;
		}
	}
	const UPrimitiveComponent* Comp = Hit.GetComponent();
	if (!Comp)
	{
		return SurfaceType_Default;
	}
	// 2) Override del componente
	if (const UPhysicalMaterial* Override = Comp->BodyInstance.GetPhysMaterialOverride())
	{
		if (Override->SurfaceType != SurfaceType_Default)
		{
			return Override->SurfaceType;
		}
	}
	// 3) Material visible: las instancias guardan su PhysMaterial pero UE no lo usa en colisión simple
	const UMaterialInterface* Mat = Comp->GetMaterial(FMath::Max(int32(Hit.ElementIndex), 0));
	if (!Mat)
	{
		Mat = Comp->GetMaterial(0);
	}
	while (const UMaterialInstance* MI = Cast<UMaterialInstance>(Mat))
	{
		if (MI->PhysMaterial && MI->PhysMaterial->SurfaceType != SurfaceType_Default)
		{
			return MI->PhysMaterial->SurfaceType;
		}
		Mat = MI->Parent;
	}
	if (Mat)
	{
		if (const UPhysicalMaterial* PM = const_cast<UMaterialInterface*>(Mat)->GetPhysicalMaterial())
		{
			return PM->SurfaceType;
		}
	}
	return SurfaceType_Default;
}
