#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FBLOnStep, int32 /*Dirección -1 / +1*/);

/** Botón de texto del menú: barra ámbar a la izquierda y fondo tenue al pasar por encima o tener el foco. */
class SBLMenuButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBLMenuButton) : _FontSize(22), _bLocked(false) {}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FText, Hint)
		SLATE_ARGUMENT(int32, FontSize)
		SLATE_ARGUMENT(bool, bLocked)
		SLATE_EVENT(FSimpleDelegate, OnClicked)
		/** Al recibir el foco o el ratón (para mostrar detalles de lo que se va a elegir). */
		SLATE_EVENT(FSimpleDelegate, OnHighlighted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	bool IsActive() const { return IsHovered() || HasKeyboardFocus(); }
	void Activate();

private:
	FSimpleDelegate OnClicked;
	FSimpleDelegate OnHighlighted;
	bool bLocked = false;
};

/** Fila de opción: "ETIQUETA ......  < VALOR >" con barra opcional (volúmenes, sensibilidad). Izq/der cambian el valor. */
class SBLSelectorRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBLSelectorRow) {}
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, Value)
		SLATE_ATTRIBUTE(TOptional<float>, Fraction)
		SLATE_EVENT(FBLOnStep, OnStep)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	void Step(int32 Dir);
	bool IsActive() const { return IsHovered() || HasKeyboardFocus(); }
	FBLOnStep OnStep;
	TAttribute<TOptional<float>> Fraction;
};

/**
 * Fondo del menú: mapa topográfico de Kessra dibujado por código (curvas de nivel de un relieve procedural, costa,
 * ría, cuadrícula con coordenadas), la línea negra que parte la ciudad y los puntos de las misiones parpadeando.
 */
class SBLTopoMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBLTopoMap) {}
		/** Misión resaltada (índice en Missions) o -1. */
		SLATE_ATTRIBUTE(int32, Highlight)
	SLATE_END_ARGS()

	struct FMarker { FVector2D Pos; FString Label; bool bAvailable; };

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920, 1080); }

	static const TArray<FMarker>& Missions();

private:
	/** Segmentos de curvas de nivel (coordenadas normalizadas 0..1) y si son "maestras" (más gruesas). */
	TArray<TArray<FVector2D>> Contours;
	TArray<bool> ContourMajor;
	TArray<TArray<FVector2D>> Coast;
	TArray<FVector2D> BlackLine;
	TAttribute<int32> Highlight;
};
