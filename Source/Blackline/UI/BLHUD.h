#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BLHUD.generated.h"

class ABLCharacter;
class ABLMissionDirector;
class UFont;

/**
 * HUD de BLACKLINE (Bloque 6). Mínimo y contextual, identidad de "terminal táctico": acento ámbar,
 * tipografía monoespaciada para datos y Roboto para el texto. Dibujado con Canvas (sin assets de UMG):
 *  - Mira dinámica, hitmarker, indicadores direccionales de daño.
 *  - Objetivo (arriba a la izquierda, resaltado al cambiar) y marcador con distancia (pegado al borde si sale de pantalla).
 *  - Munición abajo a la derecha: aparece al disparar/recargar/apuntar y se atenúa después (siempre visible si queda poca).
 *  - Aviso de interacción "[F] ..." con barra de mantener.
 *  - Subtítulos de radio, aviso de punto de control, pantalla de caída y resumen de misión completada.
 * Sin minimapa ni barra de vida (la salud se lee en la pantalla).
 */
UCLASS()
class BLACKLINE_API ABLHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABLHUD();
	virtual void DrawHUD() override;

private:
	// Primitivas
	float S() const;
	FVector2D Measure(const FString& Text, float Size, bool bMono) const;
	void Text(const FString& Str, FVector2D Pos, float Size, const FLinearColor& Color, bool bMono, float AlignX = 0.f, bool bShadow = true);
	void Rect(FVector2D Pos, FVector2D Size, const FLinearColor& Color);
	void Line(FVector2D A, FVector2D B, const FLinearColor& Color, float Thickness);

	// Elementos
	void DrawCrosshair(const ABLCharacter* Char, float Alpha);
	void DrawHitMarker(const ABLCharacter* Char);
	void DrawDamageIndicators(const ABLCharacter* Char);
	void DrawAmmo(const ABLCharacter* Char);
	void DrawObjective(const ABLMissionDirector* Director);
	void DrawMarker(const ABLCharacter* Char, const ABLMissionDirector* Director);
	void DrawInteraction(const ABLCharacter* Char);
	void DrawSubtitle(const ABLMissionDirector* Director);
	void DrawCheckpointNotice();
	/** Misión 2: destello y visor de la cámara al fotografiar; aviso de alarma. */
	void DrawPhotoFlash();
	void DrawAlarm(const ABLMissionDirector* Director);
	/** Misión 4: ametralladora montada (retícula, calor, cómo bajarse). */
	void DrawMountedGun(const ABLCharacter* Char);
	void DrawDeath(const ABLCharacter* Char);
	void DrawMissionComplete(const ABLMissionDirector* Director);
	void DrawMissionFailed(const ABLMissionDirector* Director);

	UPROPERTY() TObjectPtr<UFont> MonoFont;
	UPROPERTY() TObjectPtr<UFont> TextFont;

	float ShownGap = 0.f;
	float AmmoAlpha = 1.f;
	float AmmoAttention = 100.f;   // segundos desde la última acción con el arma
	int32 LastShots = 0;
	int32 LastMag = -1;
	int32 LastGrenades = -1;
	bool bWasReloading = false;
};
