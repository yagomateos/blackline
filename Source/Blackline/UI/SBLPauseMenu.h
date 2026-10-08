#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ABLPlayerController;
class SBLOptionsPanel;
class SWidgetSwitcher;

/** Menú de pausa (Esc en la partida): continuar, reiniciar desde el punto de control o la misión, opciones, salir al menú. */
class SBLPauseMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBLPauseMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ABLPlayerController>, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	void FocusFirst();
	bool IsInOptions() const { return bOptions; }

private:
	void ShowOptions(bool bShow);
	TWeakObjectPtr<ABLPlayerController> Owner;
	TSharedPtr<SWidgetSwitcher> Switcher;
	TSharedPtr<SBLOptionsPanel> Options;
	TSharedPtr<SWidget> First;
	bool bOptions = false;
};
