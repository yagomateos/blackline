#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWidgetSwitcher;
class SBLMenuButton;

/** Opciones > Gráficos / Audio / Controles. Los cambios se aplican al momento; se guardan al salir (Save). */
class SBLOptionsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBLOptionsPanel) {}
		/** Mundo en el que aplicar audio y controles (menú o partida). */
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Save();
	/** Foco al primer control (al abrir el panel). */
	TSharedPtr<SWidget> GetFirstFocus() const;

private:
	TSharedRef<SWidget> MakeTab(const FText& Text, int32 Index);
	TSharedRef<SWidget> GraphicsPage();
	TSharedRef<SWidget> AudioPage();
	TSharedRef<SWidget> ControlsPage();
	void ApplyLive();

	TWeakObjectPtr<UWorld> World;
	TSharedPtr<SWidgetSwitcher> Pages;
	TSharedPtr<SWidget> FirstTab;
	int32 Tab = 0;
};
