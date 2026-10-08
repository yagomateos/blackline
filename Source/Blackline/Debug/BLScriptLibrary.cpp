#include "Debug/BLScriptLibrary.h"

#include "PhysicalMaterials/PhysicalMaterial.h"

void UBLScriptLibrary::SetPhysicalSurface(UPhysicalMaterial* PhysMat, int32 Index)
{
	if (PhysMat && Index >= 0 && Index < SurfaceType_Max)
	{
		PhysMat->SurfaceType = static_cast<EPhysicalSurface>(Index);
		PhysMat->MarkPackageDirty();
	}
}
