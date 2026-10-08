#pragma once

#include "CoreMinimal.h"
#include "UObject/ConstructorHelpers.h"

namespace BL
{
	/** Carga un asset por defecto en un constructor (null si no existe). */
	template <typename T>
	T* LoadDefault(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<T> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
}
