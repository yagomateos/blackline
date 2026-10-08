#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"

class USoundBase;

/** Estilo "terminal táctico de mando" del menú (Bloque 10): fondo casi negro, ámbar, monoespaciada. */
namespace BLMenu
{
	inline const FLinearColor Bg(0.012f, 0.016f, 0.018f, 1.f);
	inline const FLinearColor Panel(0.008f, 0.011f, 0.013f, 0.94f);
	inline const FLinearColor Amber(1.f, 0.62f, 0.16f, 1.f);
	inline const FLinearColor AmberDim(0.45f, 0.29f, 0.09f, 1.f);
	inline const FLinearColor Text(0.8f, 0.82f, 0.78f, 1.f);
	inline const FLinearColor TextDim(0.42f, 0.45f, 0.43f, 1.f);
	inline const FLinearColor Line(0.6f, 0.09f, 0.05f, 1.f);       // la línea negra (con brillo rojizo)
	inline const FLinearColor Locked(0.3f, 0.32f, 0.3f, 1.f);

	FSlateFontInfo Mono(int32 Size);
	FSlateFontInfo Sans(int32 Size, bool bBold = false);
	const FSlateBrush* White();

	/** Sonidos de navegación (UI/SW_UI_Menu*), en 2D. */
	void PlayMove();
	void PlaySelect();
	void PlayBack();
	void PlayTick();
}
