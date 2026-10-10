#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWidgetSwitcher;
class SBLOptionsPanel;
class ABLMenuPlayerController;

/**
 * Menú principal (Bloque 10), estilo "terminal táctico de mando": mapa topográfico de Kessra con la línea negra
 * de fondo y un panel a la izquierda. Páginas: principal, selección de misión (con punto de inicio), armamento,
 * inteligencia (lore), opciones y confirmación de salida. Teclado, ratón y mando (Slate).
 */
class SBLMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBLMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ABLMenuPlayerController>, Owner)
	SLATE_END_ARGS()

	enum class EPage : int32 { Main, Missions, Loadout, Intel, Options, Quit, Equip };

	void Construct(const FArguments& InArgs);
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	void ShowPage(EPage Page);
	EPage GetPage() const { return Page; }
	void FocusFirst();
	/** Pruebas: cambia de página / selección sin input. */
	void SetSelectedIntel(int32 Index) { SelectedIntel = Index; }
	void SetSelectedWeapon(int32 Index) { SelectedWeapon = Index; }
	int32 GetStartPhase() const { return StartPhase; }
	/** Pantalla de equipamiento previa a una misión (JUGAR/CONTINUAR o DESPLEGAR). */
	void OpenEquip(int32 Mission, int32 Phase);
	int32 GetEquipPrimary() const { return EquipPrimary; }
	void SetEquipPrimary(int32 Index);
	void ConfirmEquipAndDeploy();

private:
	TSharedRef<SWidget> MainPage();
	TSharedRef<SWidget> MissionsPage();
	TSharedRef<SWidget> LoadoutPage();
	TSharedRef<SWidget> IntelPage();
	TSharedRef<SWidget> OptionsPage();
	TSharedRef<SWidget> QuitPage();
	TSharedRef<SWidget> EquipPage();
	TSharedRef<SWidget> Header(const FText& Title, const FText& Sub);
	void Back();

	TWeakObjectPtr<ABLMenuPlayerController> Owner;
	TSharedPtr<SWidgetSwitcher> Switcher;
	TSharedPtr<SBLOptionsPanel> Options;
	TMap<int32, TSharedPtr<SWidget>> FirstFocus;
	EPage Page = EPage::Main;
	int32 SelectedMission = 0;
	int32 SelectedWeapon = 0;
	int32 SelectedIntel = 0;
	int32 StartPhase = 0;
	int32 EquipMission = 0;
	int32 EquipPhase = 0;
	int32 EquipPrimary = 0;
	EPage EquipFrom = EPage::Main;
};
