#include "UI/SBLMainMenu.h"

#include "Mission/BLCampaignProgress.h"
#include "Weapons/BLWeaponData.h"
#include "UI/BLUserSettings.h"

#include "UI/BLMenuData.h"
#include "UI/BLMenuPlayerController.h"
#include "UI/BLMenuStyle.h"
#include "UI/SBLMenuWidgets.h"
#include "UI/SBLOptionsPanel.h"

#include "Framework/Application/SlateApplication.h"
#include "Misc/DateTime.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "BLMainMenu"

namespace
{
	FText T(const TCHAR* S) { return FText::FromString(S); }

	/** Columna izquierda con el fondo del panel. */
	TSharedRef<SWidget> Column(float Width, TSharedRef<SWidget> Content)
	{
		return SNew(SBox).WidthOverride(Width)
		[
			SNew(SBorder).BorderImage(BLMenu::White()).BorderBackgroundColor(BLMenu::Panel).Padding(FMargin(56.f, 44.f, 40.f, 44.f))
			[Content]
		];
	}

	/** Panel de detalle a la derecha (misión, arma, expediente). */
	TSharedRef<SWidget> DetailPanel(TSharedRef<SWidget> Content)
	{
		return SNew(SBox).WidthOverride(640.f).VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 64.f, 0.f))
		[
			SNew(SBorder).BorderImage(BLMenu::White()).BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.022f, 0.92f)).Padding(FMargin(32.f, 28.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[SNew(SBox).HeightOverride(2.f)[SNew(SImage).Image(BLMenu::White()).ColorAndOpacity(BLMenu::Amber)]]
				+ SVerticalBox::Slot().AutoHeight()[Content]
			]
		];
	}

	/** Fila de equipamiento fija (sin flechas: no se puede cambiar). */
	TSharedRef<SWidget> FixedRow(const FText& Label, const FText& Value)
	{
		return SNew(SBox).Padding(FMargin(10.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[SNew(STextBlock).Font(BLMenu::Sans(13, true)).ColorAndOpacity(FSlateColor(BLMenu::Text)).Text(Label)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[SNew(STextBlock).Font(BLMenu::Mono(13)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).Text(Value)]
		];
	}

	TSharedRef<SWidget> StatBar(const FText& Label, TFunction<float()> Value)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(Label).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))]
			+ SHorizontalBox::Slot().FillWidth(0.65f).VAlign(VAlign_Center).Padding(0.f, 5.f)
			[
				SNew(SBox).HeightOverride(6.f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SImage).Image(BLMenu::White()).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.07f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[SNew(SBox).WidthOverride_Lambda([Value]() { return FOptionalSize(330.f * Value()); })[SNew(SImage).Image(BLMenu::White()).ColorAndOpacity(BLMenu::Amber)]]
				]
			];
	}
}

void SBLMainMenu::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBLTopoMap).Highlight_Lambda([this]() { return Page == EPage::Missions ? SelectedMission : -1; })
		]
		+ SOverlay::Slot()
		[
			SAssignNew(Switcher, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()[MainPage()]
			+ SWidgetSwitcher::Slot()[MissionsPage()]
			+ SWidgetSwitcher::Slot()[LoadoutPage()]
			+ SWidgetSwitcher::Slot()[IntelPage()]
			+ SWidgetSwitcher::Slot()[OptionsPage()]
			+ SWidgetSwitcher::Slot()[QuitPage()]
			+ SWidgetSwitcher::Slot()[EquipPage()]
		]
		// Estado del enlace y reloj (arriba a la derecha)
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 28.f, 40.f, 0.f)
		[
			SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::AmberDim)).Justification(ETextJustify::Right)
			.Text_Lambda([]()
			{
				const FDateTime Now = FDateTime::Now();
				return FText::FromString(FString::Printf(TEXT("ENLACE SEGURO  ·  TORRE\nKESSRA  %02d:%02d:%02d  ·  14 NOV 2031"), Now.GetHour(), Now.GetMinute(), Now.GetSecond()));
			})
		]
		// Ayuda de controles y versión (abajo)
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 40.f, 24.f)
		[
			SNew(STextBlock).Font(BLMenu::Mono(10)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))
			.Text(LOCTEXT("Hints", "[ENTER] SELECCIONAR    [ESC] VOLVER    ·    BLACKLINE 0.1 — VERTICAL SLICE"))
		]
		// Pantalla de carga al desplegar
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(BLMenu::White()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.92f))
			.Visibility_Lambda([this]() { return Owner.IsValid() && Owner->IsLoading() ? EVisibility::Visible : EVisibility::Collapsed; })
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[SNew(STextBlock).Text(LOCTEXT("Deploy", "DESPLEGANDO")).Font(BLMenu::Sans(34, true)).ColorAndOpacity(FSlateColor(BLMenu::Amber))]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f)
				[
					SNew(STextBlock).Font(BLMenu::Mono(13)).ColorAndOpacity(FSlateColor(BLMenu::Text))
					.Text_Lambda([this]()
					{
						const BLMenuData::FMission& Sel = BLMenuData::Missions[SelectedMission];
						return FText::FromString(FString::Printf(TEXT("%s · %s  //  %s"), Sel.Code, Sel.Name, BLMenuData::StartPhases[Sel.FirstPhase + StartPhase]));
					})
				]
			]
		]
	];
	ShowPage(EPage::Main);
}

TSharedRef<SWidget> SBLMainMenu::Header(const FText& Title, const FText& Sub)
{
	FSlateFontInfo Logo = BLMenu::Sans(15, true);
	Logo.LetterSpacing = 400;
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("LogoSmall", "BLACKLINE")).Font(Logo).ColorAndOpacity(FSlateColor(BLMenu::Amber))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 2.f)[SNew(STextBlock).Text(Title).Font(BLMenu::Sans(30, true)).ColorAndOpacity(FSlateColor(BLMenu::Text))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 26.f)[SNew(STextBlock).Text(Sub).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))];
}

TSharedRef<SWidget> SBLMainMenu::MainPage()
{
	FSlateFontInfo Logo = BLMenu::Sans(52, true);
	Logo.LetterSpacing = 150;
	// Con progreso guardado, el primer botón continúa la campaña por la primera misión sin completar
	const bool bContinue = BLCampaign::HasProgress() && !BLCampaign::IsCampaignComplete();
	const int32 PlayMission = bContinue ? BLCampaign::NextMissionToPlay() : 0;
	const FText PlayHint = T(*FString::Printf(TEXT("%sMISIÓN %s · %s"), BLCampaign::IsCampaignComplete() ? TEXT("CAMPAÑA COMPLETADA · ") : TEXT(""),
		BLMenuData::Missions[PlayMission].Code, BLMenuData::Missions[PlayMission].Name));
	TSharedRef<SWidget> Play = SNew(SBLMenuButton).Text(bContinue ? LOCTEXT("Continue", "CONTINUAR") : LOCTEXT("Play", "JUGAR")).Hint(PlayHint)
		.OnClicked_Lambda([this, PlayMission]() { SelectedMission = PlayMission; StartPhase = 0; OpenEquip(PlayMission, 0); });
	FirstFocus.Add(int32(EPage::Main), Play);
	auto Btn = [this](const FText& Text, const FText& Hint, EPage To)
	{
		return SNew(SBLMenuButton).Text(Text).Hint(Hint).OnClicked_Lambda([this, To]() { ShowPage(To); });
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(600.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("Logo", "BLACKLINE")).Font(Logo).ColorAndOpacity(FSlateColor(BLMenu::Amber))]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 0.f, 56.f)
				[SNew(STextBlock).Text(LOCTEXT("Tag", "GRUPO OPERATIVO  //  KESSRA  //  NOV 2031")).Font(BLMenu::Mono(12)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))]
				+ SVerticalBox::Slot().AutoHeight()[Play]
				+ SVerticalBox::Slot().AutoHeight()[Btn(LOCTEXT("Missions", "SELECCIÓN DE MISIÓN"), LOCTEXT("MissionsHint", "CAMPAÑA DE KESSRA · PUNTO DE INICIO"), EPage::Missions)]
				+ SVerticalBox::Slot().AutoHeight()[Btn(LOCTEXT("Loadout", "ARMAMENTO"), LOCTEXT("LoadoutHint", "EQUIPO ASIGNADO A SABLE 2-1"), EPage::Loadout)]
				+ SVerticalBox::Slot().AutoHeight()[Btn(LOCTEXT("Intel", "INTELIGENCIA"), LOCTEXT("IntelHint", "EXPEDIENTES: KESSRA, LA COLUMNA, CORVANE"), EPage::Intel)]
				+ SVerticalBox::Slot().AutoHeight()[Btn(LOCTEXT("Options", "OPCIONES"), LOCTEXT("OptionsHint", "GRÁFICOS · AUDIO · CONTROLES"), EPage::Options)]
				+ SVerticalBox::Slot().AutoHeight()[Btn(LOCTEXT("Quit", "SALIR"), FText::GetEmpty(), EPage::Quit)]
				+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SSpacer)]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(BLMenu::Mono(10)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).AutoWrapText(true)
					.Text(LOCTEXT("Situation", "SITUACIÓN: LA COLUMNA VESK CONTROLA EL ESTE DE LA CIUDAD. EL INFORMANTE VAREK NO RESPONDE DESDE LAS 04:00."))
				])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)];
}

TSharedRef<SWidget> SBLMainMenu::MissionsPage()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 i = 0; i < UE_ARRAY_COUNT(BLMenuData::Missions); ++i)
	{
		const BLMenuData::FMission& M = BLMenuData::Missions[i];
		TSharedRef<SBLMenuButton> B = SNew(SBLMenuButton).FontSize(18).bLocked(!M.bAvailable)
			.Text(T(*FString::Printf(TEXT("%s  %s"), M.Code, M.Name)))
			.Hint(M.bAvailable ? T(*FString::Printf(TEXT("%s%s"), BLCampaign::IsCompleted(i) ? TEXT("COMPLETADA · ") : TEXT(""), M.Place))
				: LOCTEXT("Locked", "NO DISPONIBLE EN EL VERTICAL SLICE"))
			.OnHighlighted_Lambda([this, i]() { if (SelectedMission != i) { SelectedMission = i; StartPhase = 0; } })
			.OnClicked_Lambda([this, i]() { SelectedMission = i; OpenEquip(i, StartPhase); });
		if (i == 0)
		{
			FirstFocus.Add(int32(EPage::Missions), B);
		}
		List->AddSlot().AutoHeight()[B];
	}
	List->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
	[
		SNew(SBLSelectorRow).Label(LOCTEXT("Start", "PUNTO DE INICIO"))
		.Value_Lambda([this]()
		{
			const BLMenuData::FMission& Sel = BLMenuData::Missions[SelectedMission];
			return Sel.NumPhases > 0 ? T(BLMenuData::StartPhases[Sel.FirstPhase + StartPhase]) : LOCTEXT("NoStart", "—");
		})
		.OnStep_Lambda([this](int32 Dir)
		{
			const int32 N = FMath::Max(BLMenuData::Missions[SelectedMission].NumPhases, 1);
			StartPhase = (StartPhase + Dir + N) % N;
		})
	];
	List->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
	[
		SNew(SBLMenuButton).Text(LOCTEXT("Deploy", "DESPLEGAR")).Hint_Lambda([this]() { return T(*FString::Printf(TEXT("MISIÓN %s"), BLMenuData::Missions[SelectedMission].Code)); })
		.OnClicked_Lambda([this]() { OpenEquip(SelectedMission, StartPhase); })
	];
	List->AddSlot().AutoHeight()[SNew(SBLMenuButton).Text(LOCTEXT("Back", "VOLVER")).FontSize(16).OnClicked_Lambda([this]() { Back(); })];

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(600.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Header(LOCTEXT("MTitle", "SELECCIÓN DE MISIÓN"), LOCTEXT("MSub", "CAMPAÑA DE KESSRA · 5 OPERACIONES"))]
				+ SVerticalBox::Slot().AutoHeight()[List])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			DetailPanel(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Sans(26, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(BLMenuData::Missions[SelectedMission].bAvailable ? BLMenu::Amber : BLMenu::Locked); })
					.Text_Lambda([this]() { return T(*FString::Printf(TEXT("%s · %s"), BLMenuData::Missions[SelectedMission].Code, BLMenuData::Missions[SelectedMission].Name)); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 16.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).Text_Lambda([this]() { return T(BLMenuData::Missions[SelectedMission].Place); })]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Sans(15)).ColorAndOpacity(FSlateColor(BLMenu::Text)).AutoWrapText(true).LineHeightPercentage(1.15f)
					.Text_Lambda([this]() { return T(BLMenuData::Missions[SelectedMission].Briefing); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity_Lambda([this]() { return FSlateColor(BLMenuData::Missions[SelectedMission].bAvailable ? BLMenu::Amber : BLMenu::Locked); })
					.Text_Lambda([this]() { return BLMenuData::Missions[SelectedMission].bAvailable ? LOCTEXT("Ready", "ESTADO: LISTA PARA DESPLIEGUE") : LOCTEXT("Planned", "ESTADO: EN PLANIFICACIÓN (FASE 4)"); })])
		];
}

TSharedRef<SWidget> SBLMainMenu::LoadoutPage()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 i = 0; i < UE_ARRAY_COUNT(BLMenuData::Weapons); ++i)
	{
		const BLMenuData::FWeapon& W = BLMenuData::Weapons[i];
		TSharedRef<SBLMenuButton> B = SNew(SBLMenuButton).FontSize(18).bLocked(!W.bAvailable).Text(T(W.Name)).Hint(T(W.Class))
			.OnHighlighted_Lambda([this, i]() { SelectedWeapon = i; })
			.OnClicked_Lambda([this, i]() { SelectedWeapon = i; });
		if (i == 0)
		{
			FirstFocus.Add(int32(EPage::Loadout), B);
		}
		List->AddSlot().AutoHeight()[B];
	}
	List->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[SNew(SBLMenuButton).Text(LOCTEXT("Back", "VOLVER")).FontSize(16).OnClicked_Lambda([this]() { Back(); })];

	static const TCHAR* StatNames[] = { TEXT("DAÑO"), TEXT("CADENCIA"), TEXT("ALCANCE"), TEXT("CONTROL"), TEXT("MOVILIDAD") };
	TSharedRef<SVerticalBox> Stats = SNew(SVerticalBox);
	for (int32 s = 0; s < 5; ++s)
	{
		Stats->AddSlot().AutoHeight()[StatBar(T(StatNames[s]), [this, s]() { return BLMenuData::Weapons[SelectedWeapon].Stats[s]; })];
	}
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(600.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Header(LOCTEXT("LTitle", "ARMAMENTO"), LOCTEXT("LSub", "EQUIPO DE SABLE 2-1 · EL RESTO LLEGA EN LA FASE 4"))]
				+ SVerticalBox::Slot().AutoHeight()[List])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			DetailPanel(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Sans(28, true)).Text_Lambda([this]() { return T(BLMenuData::Weapons[SelectedWeapon].Name); })
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(BLMenuData::Weapons[SelectedWeapon].bAvailable ? BLMenu::Amber : BLMenu::Locked); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 4.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).Text_Lambda([this]() { return T(BLMenuData::Weapons[SelectedWeapon].Class); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::Text)).AutoWrapText(true).Text_Lambda([this]() { return T(BLMenuData::Weapons[SelectedWeapon].Specs); })]
				+ SVerticalBox::Slot().AutoHeight()[Stats]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
				[SNew(STextBlock).Font(BLMenu::Sans(14)).ColorAndOpacity(FSlateColor(BLMenu::Text)).AutoWrapText(true).Text_Lambda([this]() { return T(BLMenuData::Weapons[SelectedWeapon].Text); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(BLMenuData::Weapons[SelectedWeapon].bAvailable ? BLMenu::Amber : BLMenu::Locked); })
					.Text_Lambda([this]() { return BLMenuData::Weapons[SelectedWeapon].bAvailable ? LOCTEXT("Equipped", "EQUIPADA") : LOCTEXT("Waiting", "EN ESPERA DE SUMINISTRO"); })])
		];
}

TSharedRef<SWidget> SBLMainMenu::IntelPage()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 i = 0; i < UE_ARRAY_COUNT(BLMenuData::Intel); ++i)
	{
		TSharedRef<SBLMenuButton> B = SNew(SBLMenuButton).FontSize(16).Text(T(BLMenuData::Intel[i].Title))
			.OnHighlighted_Lambda([this, i]() { SelectedIntel = i; })
			.OnClicked_Lambda([this, i]() { SelectedIntel = i; });
		if (i == 0)
		{
			FirstFocus.Add(int32(EPage::Intel), B);
		}
		List->AddSlot().AutoHeight()[B];
	}
	List->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[SNew(SBLMenuButton).Text(LOCTEXT("Back", "VOLVER")).FontSize(16).OnClicked_Lambda([this]() { Back(); })];
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(600.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Header(LOCTEXT("ITitle", "INTELIGENCIA"), LOCTEXT("ISub", "EXPEDIENTES · CLASIFICACIÓN: RESERVADO"))]
				+ SVerticalBox::Slot().AutoHeight()[List])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			DetailPanel(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::AmberDim))
					.Text_Lambda([this]() { return T(*FString::Printf(TEXT("EXPEDIENTE %02d / %02d  ·  FUENTE: TORRE"), SelectedIntel + 1, int32(UE_ARRAY_COUNT(BLMenuData::Intel)))); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 16.f)
				[SNew(STextBlock).Font(BLMenu::Sans(26, true)).ColorAndOpacity(FSlateColor(BLMenu::Amber)).AutoWrapText(true).Text_Lambda([this]() { return T(BLMenuData::Intel[SelectedIntel].Title); })]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Sans(15)).ColorAndOpacity(FSlateColor(BLMenu::Text)).AutoWrapText(true).LineHeightPercentage(1.15f)
					.Text_Lambda([this]() { return T(BLMenuData::Intel[SelectedIntel].Body); })])
		];
}

TSharedRef<SWidget> SBLMainMenu::OptionsPage()
{
	SAssignNew(Options, SBLOptionsPanel).World(Owner.IsValid() ? Owner->GetWorld() : nullptr);
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(860.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Header(LOCTEXT("OTitle", "OPCIONES"), LOCTEXT("OSub", "PREAJUSTE RECOMENDADO PARA GTX 1660 SUPER: ALTA"))]
				+ SVerticalBox::Slot().AutoHeight()[Options.ToSharedRef()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
				[SNew(SBLMenuButton).Text(LOCTEXT("SaveBack", "GUARDAR Y VOLVER")).FontSize(16).OnClicked_Lambda([this]() { Back(); })])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)];
}

TSharedRef<SWidget> SBLMainMenu::QuitPage()
{
	TSharedRef<SWidget> No = SNew(SBLMenuButton).Text(LOCTEXT("No", "NO, VOLVER")).OnClicked_Lambda([this]() { Back(); });
	FirstFocus.Add(int32(EPage::Quit), No);
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(600.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Header(LOCTEXT("QTitle", "¿CERRAR EL ENLACE?"), LOCTEXT("QSub", "SE CERRARÁ EL JUEGO"))]
				+ SVerticalBox::Slot().AutoHeight()[No]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(SBLMenuButton).Text(LOCTEXT("Yes", "SÍ, SALIR")).OnClicked_Lambda([this]() { if (Owner.IsValid()) { Owner->QuitGame(); } })])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)];
}

void SBLMainMenu::ShowPage(EPage NewPage)
{
	if (Page == EPage::Options && NewPage != EPage::Options && Options.IsValid())
	{
		Options->Save();
	}
	Page = NewPage;
	Switcher->SetActiveWidgetIndex(int32(NewPage));
	FocusFirst();
}

void SBLMainMenu::FocusFirst()
{
	TSharedPtr<SWidget> Target = Page == EPage::Options && Options.IsValid() ? Options->GetFirstFocus() : FirstFocus.FindRef(int32(Page));
	if (Target.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocus(Target, EFocusCause::SetDirectly);
	}
}

void SBLMainMenu::Back()
{
	BLMenu::PlayBack();
	ShowPage(Page == EPage::Main ? EPage::Quit : Page == EPage::Equip ? EquipFrom : EPage::Main);
}

void SBLMainMenu::OpenEquip(int32 Mission, int32 Phase)
{
	EquipMission = FMath::Clamp(Mission, 0, int32(UE_ARRAY_COUNT(BLMenuData::Missions)) - 1);
	EquipPhase = Phase;
	EquipFrom = Page == EPage::Equip ? EquipFrom : Page;
	const TCHAR* Forced = BLMenuData::Missions[EquipMission].ForcedPrimary;
	EquipPrimary = BLMenuData::FindPrimary(Forced ? FString(Forced) : UBLUserSettings::Get()->LoadoutPrimary);
	ShowPage(EPage::Equip);
}

void SBLMainMenu::SetEquipPrimary(int32 Index)
{
	if (BLMenuData::Missions[EquipMission].ForcedPrimary)
	{
		return;   // la misión impone el arma
	}
	const int32 N = UE_ARRAY_COUNT(BLMenuData::LoadoutPrimaries);
	EquipPrimary = (Index % N + N) % N;
}

void SBLMainMenu::ConfirmEquipAndDeploy()
{
	UBLUserSettings* S = UBLUserSettings::Get();
	S->LoadoutPrimary = BLMenuData::LoadoutPrimaries[EquipPrimary].Id;
	S->LoadoutSecondary = BLMenuData::LoadoutSecondaries[0].Id;
	S->Save();
	if (Owner.IsValid())
	{
		Owner->StartMission(EquipMission, EquipPhase);
	}
}

TSharedRef<SWidget> SBLMainMenu::EquipPage()
{
	// Ficha del arma (de BLMenuData) + munición real del asset que se va a equipar
	auto Primary = [this]() -> const BLMenuData::FWeapon& { return BLMenuData::Weapons[BLMenuData::LoadoutPrimaries[EquipPrimary].MenuIndex]; };
	auto Ammo = [this]() -> FText
	{
		const UBLWeaponData* D = LoadObject<UBLWeaponData>(nullptr, BLMenuData::LoadoutPrimaries[EquipPrimary].Asset);
		if (!D)
		{
			return FText::GetEmpty();
		}
		const int32 Chamber = D->bChamberRound ? 1 : 0;
		return T(*FString::Printf(TEXT("CARGADOR %d+%d  ·  RESERVA %d  ·  %s"), D->MagazineSize, Chamber, D->StartReserveAmmo,
			D->ReloadStyle == EBLReloadStyle::Shells ? TEXT("RECARGA CARTUCHO A CARTUCHO") : TEXT("RECARGA POR CARGADOR")));
	};
	auto Forced = [this]() { return BLMenuData::Missions[EquipMission].ForcedPrimary != nullptr; };

	TSharedRef<SWidget> PrimaryRow = SNew(SBLSelectorRow).Label(LOCTEXT("EqPrimary", "ARMA PRINCIPAL"))
		.Value_Lambda([this, Primary, Forced]() { return T(*FString::Printf(TEXT("%s%s"), Primary().Name, Forced() ? TEXT("  (IMPUESTA)") : TEXT(""))); })
		.OnStep_Lambda([this](int32 Dir) { SetEquipPrimary(EquipPrimary + Dir); });
	FirstFocus.Add(int32(EPage::Equip), PrimaryRow);

	static const TCHAR* StatNames[] = { TEXT("DAÑO"), TEXT("CADENCIA"), TEXT("ALCANCE"), TEXT("CONTROL"), TEXT("MOVILIDAD") };
	TSharedRef<SVerticalBox> Stats = SNew(SVerticalBox);
	for (int32 s = 0; s < 5; ++s)
	{
		Stats->AddSlot().AutoHeight()[StatBar(T(StatNames[s]), [Primary, s]() { return Primary().Stats[s]; })];
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Column(600.f, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::AmberDim)).Text(LOCTEXT("EqKicker", "BLACKLINE"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[SNew(STextBlock).Font(BLMenu::Sans(30, true)).ColorAndOpacity(FSlateColor(BLMenu::Text)).Text(LOCTEXT("EqTitle", "EQUIPAMIENTO"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 26.f)
					[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))
						.Text_Lambda([this]() { return T(*FString::Printf(TEXT("MISIÓN %s · %s · SABLE 2-1"), BLMenuData::Missions[EquipMission].Code, BLMenuData::Missions[EquipMission].Name)); })]
				]
				+ SVerticalBox::Slot().AutoHeight()[PrimaryRow]
				+ SVerticalBox::Slot().AutoHeight()[FixedRow(LOCTEXT("EqSecondary", "ARMA SECUNDARIA"), T(BLMenuData::Weapons[BLMenuData::LoadoutSecondaries[0].MenuIndex].Name))]
				+ SVerticalBox::Slot().AutoHeight()[FixedRow(LOCTEXT("EqGrenades", "GRANADAS"), LOCTEXT("EqGrenadesV", "2 × M-6 FRAGMENTACIÓN"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(14.f, 10.f, 0.f, 18.f)
				[SNew(STextBlock).Font(BLMenu::Mono(10)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).AutoWrapText(true)
					.Text(LOCTEXT("EqHelp", "← →  CAMBIAR ARMA  ·  ENTER  CONFIRMAR  ·  ESC  VOLVER\nLA SECUNDARIA SE LLEVA SIEMPRE (TECLA 2). EN LA MISIÓN SE PUEDEN RECOGER ARMAS DEL SUELO."))]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(SBLMenuButton).Text(LOCTEXT("EqStart", "COMENZAR MISIÓN"))
					.Hint_Lambda([this, Primary]() { return T(*FString::Printf(TEXT("%s + P-17"), Primary().Name)); })
					.OnClicked_Lambda([this]() { ConfirmEquipAndDeploy(); })]
				+ SVerticalBox::Slot().AutoHeight()[SNew(SBLMenuButton).Text(LOCTEXT("Back", "VOLVER")).FontSize(16).OnClicked_Lambda([this]() { Back(); })])
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			DetailPanel(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::AmberDim)).Text(LOCTEXT("EqSel", "PRINCIPAL SELECCIONADA"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[SNew(STextBlock).Font(BLMenu::Sans(28, true)).ColorAndOpacity(FSlateColor(BLMenu::Amber)).Text_Lambda([Primary]() { return T(Primary().Name); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 4.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).Text_Lambda([Primary]() { return T(Primary().Class); })]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::Text)).AutoWrapText(true).Text_Lambda([Primary]() { return T(Primary().Specs); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 14.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::Amber)).AutoWrapText(true).Text_Lambda(Ammo)]
				+ SVerticalBox::Slot().AutoHeight()[Stats]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
				[SNew(STextBlock).Font(BLMenu::Sans(14)).ColorAndOpacity(FSlateColor(BLMenu::Text)).AutoWrapText(true).Text_Lambda([Primary]() { return T(Primary().Text); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
				[SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))
					.Text_Lambda([]() { const BLMenuData::FWeapon& W = BLMenuData::Weapons[BLMenuData::LoadoutSecondaries[0].MenuIndex];
						return T(*FString::Printf(TEXT("SECUNDARIA  %s  ·  %s"), W.Name, W.Specs)); })])
		];
}

FReply SBLMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::BackSpace || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Virtual_Gamepad_Back.GetVirtualKey())
	{
		Back();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
