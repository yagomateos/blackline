#include "UI/SBLPauseMenu.h"

#include "Mission/BLMissionDirector.h"
#include "Player/BLPlayerController.h"
#include "UI/BLMenuStyle.h"
#include "UI/SBLMenuWidgets.h"
#include "UI/SBLOptionsPanel.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "BLPause"

void SBLPauseMenu::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	FSlateFontInfo Logo = BLMenu::Sans(15, true);
	Logo.LetterSpacing = 400;
	auto Act = [this](TFunction<void(ABLPlayerController*)> F) { return [this, F]() { if (Owner.IsValid()) { F(Owner.Get()); } }; };

	SAssignNew(Options, SBLOptionsPanel).World(Owner.IsValid() ? Owner->GetWorld() : nullptr);
	TSharedRef<SWidget> Resume = SNew(SBLMenuButton).Text(LOCTEXT("Resume", "CONTINUAR"))
		.OnClicked_Lambda(Act([](ABLPlayerController* PC) { PC->SetPauseMenu(false); }));
	First = Resume;

	ChildSlot
	[
		SNew(SBorder).BorderImage(BLMenu::White()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.62f)).Padding(0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride_Lambda([this]() { return FOptionalSize(bOptions ? 880.f : 600.f); })
				[
					SNew(SBorder).BorderImage(BLMenu::White()).BorderBackgroundColor(BLMenu::Panel).Padding(FMargin(56.f, 44.f, 40.f, 44.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("Logo", "BLACKLINE")).Font(Logo).ColorAndOpacity(FSlateColor(BLMenu::Amber))]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 2.f)
						[SNew(STextBlock).Font(BLMenu::Sans(30, true)).ColorAndOpacity(FSlateColor(BLMenu::Text))
							.Text_Lambda([this]() { return bOptions ? LOCTEXT("OptTitle", "OPCIONES") : LOCTEXT("Title", "PAUSA"); })]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 26.f)
						[
							SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).AutoWrapText(true)
							.Text_Lambda([this]()
							{
								const ABLMissionDirector* M = Owner.IsValid() ? ABLMissionDirector::Get(Owner.Get()) : nullptr;
								const FBLObjective* O = M ? M->GetCurrentObjective() : nullptr;
								return FText::FromString(FString::Printf(TEXT("01 · AMANECER ROTO  //  %s"), O ? *O->Text.ToUpper() : TEXT("SIN OBJETIVO")));
							})
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(Switcher, SWidgetSwitcher)
							+ SWidgetSwitcher::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[Resume]
								+ SVerticalBox::Slot().AutoHeight()[SNew(SBLMenuButton).Text(LOCTEXT("Checkpoint", "VOLVER AL PUNTO DE CONTROL"))
									.OnClicked_Lambda(Act([](ABLPlayerController* PC) { PC->RestartFromCheckpoint(); }))]
								+ SVerticalBox::Slot().AutoHeight()[SNew(SBLMenuButton).Text(LOCTEXT("Restart", "REINICIAR MISIÓN"))
									.OnClicked_Lambda(Act([](ABLPlayerController* PC) { PC->RestartMission(); }))]
								+ SVerticalBox::Slot().AutoHeight()[SNew(SBLMenuButton).Text(LOCTEXT("Options", "OPCIONES"))
									.OnClicked_Lambda([this]() { ShowOptions(true); })]
								+ SVerticalBox::Slot().AutoHeight()[SNew(SBLMenuButton).Text(LOCTEXT("Menu", "SALIR AL MENÚ PRINCIPAL"))
									.OnClicked_Lambda(Act([](ABLPlayerController* PC) { PC->QuitToMenu(); }))]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[Options.ToSharedRef()]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
								[SNew(SBLMenuButton).Text(LOCTEXT("SaveBack", "GUARDAR Y VOLVER")).FontSize(16).OnClicked_Lambda([this]() { ShowOptions(false); })]
							]
						]
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SBox)]
		]
	];
}

void SBLPauseMenu::FocusFirst()
{
	TSharedPtr<SWidget> Target = bOptions ? Options->GetFirstFocus() : First;
	if (Target.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocus(Target, EFocusCause::SetDirectly);
	}
}

void SBLPauseMenu::ShowOptions(bool bShow)
{
	if (bOptions && !bShow)
	{
		Options->Save();
		BLMenu::PlayBack();
	}
	bOptions = bShow;
	Switcher->SetActiveWidgetIndex(bShow ? 1 : 0);
	FocusFirst();
}

FReply SBLPauseMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Right || K == EKeys::Virtual_Gamepad_Back.GetVirtualKey())
	{
		if (bOptions)
		{
			ShowOptions(false);
		}
		else if (Owner.IsValid())
		{
			Owner->SetPauseMenu(false);
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
