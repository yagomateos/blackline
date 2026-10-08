#include "UI/SBLOptionsPanel.h"

#include "UI/BLMenuStyle.h"
#include "UI/BLUserSettings.h"
#include "UI/SBLMenuWidgets.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "BLOptions"

namespace
{
	UGameUserSettings* GUS() { return GEngine ? GEngine->GetGameUserSettings() : nullptr; }
	FText Pct(float V) { return FText::FromString(FString::Printf(TEXT("%d %%"), FMath::RoundToInt(V * 100.f))); }
	float StepClamp(float V, int32 Dir, float Step, float Min, float Max) { return FMath::Clamp(FMath::GridSnap(V + Dir * Step, Step), Min, Max); }
	FText YesNo(bool b) { return b ? LOCTEXT("Yes", "SÍ") : LOCTEXT("No", "NO"); }
}

void SBLOptionsPanel::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[MakeTab(LOCTEXT("TabG", "GRÁFICOS"), 0)]
			+ SHorizontalBox::Slot().AutoWidth()[MakeTab(LOCTEXT("TabA", "AUDIO"), 1)]
			+ SHorizontalBox::Slot().AutoWidth()[MakeTab(LOCTEXT("TabC", "CONTROLES"), 2)]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(Pages, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()[GraphicsPage()]
			+ SWidgetSwitcher::Slot()[AudioPage()]
			+ SWidgetSwitcher::Slot()[ControlsPage()]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14.f, 16.f, 0.f, 0.f)
		[
			SNew(STextBlock).Font(BLMenu::Mono(10)).ColorAndOpacity(FSlateColor(BLMenu::TextDim))
			.Text(LOCTEXT("Help", "IZQ./DER. CAMBIAR  ·  LOS CAMBIOS SE APLICAN AL MOMENTO Y SE GUARDAN AL VOLVER"))
		]
	];
}

TSharedRef<SWidget> SBLOptionsPanel::MakeTab(const FText& Text, int32 Index)
{
	TSharedRef<SBLMenuButton> B = SNew(SBLMenuButton).Text(Text).FontSize(15)
		.OnClicked_Lambda([this, Index]() { Tab = Index; Pages->SetActiveWidgetIndex(Index); });
	if (Index == 0)
	{
		FirstTab = B;
	}
	return SNew(SBox).Padding(0.f, 0.f, 8.f, 0.f)
	[
		SNew(SBorder).BorderImage(BLMenu::White()).Padding(0.f)
		.BorderBackgroundColor_Lambda([this, Index]() { return Tab == Index ? FLinearColor(1.f, 0.62f, 0.16f, 0.16f) : FLinearColor(1.f, 1.f, 1.f, 0.03f); })
		[B]
	];
}

TSharedPtr<SWidget> SBLOptionsPanel::GetFirstFocus() const
{
	return FirstTab;
}

void SBLOptionsPanel::ApplyLive()
{
	UBLUserSettings* S = UBLUserSettings::Get();
	if (UWorld* W = World.Get())
	{
		S->ApplyAudio(W);
		if (APlayerController* PC = W->GetFirstPlayerController())
		{
			S->ApplyToPlayer(PC);
		}
	}
}

void SBLOptionsPanel::Save()
{
	UBLUserSettings::Get()->Save();
	if (UGameUserSettings* G = GUS())
	{
		G->SaveSettings();
	}
}

TSharedRef<SWidget> SBLOptionsPanel::GraphicsPage()
{
	auto Row = [](const FText& Label, TFunction<FText()> Value, TFunction<void(int32)> Step, TFunction<TOptional<float>()> Frac = nullptr)
	{
		return SNew(SBLSelectorRow).Label(Label)
			.Value_Lambda([Value]() { return Value(); })
			.Fraction_Lambda([Frac]() { return Frac ? Frac() : TOptional<float>(); })
			.OnStep_Lambda([Step](int32 Dir) { Step(Dir); });
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Preset", "CALIDAD"),
			[]() {
				switch (UBLUserSettings::GetGraphicsPreset())
				{
				case EBLGraphicsPreset::Low: return LOCTEXT("Low", "BAJA");
				case EBLGraphicsPreset::Medium: return LOCTEXT("Med", "MEDIA");
				case EBLGraphicsPreset::High: return LOCTEXT("High", "ALTA (RECOMENDADA)");
				case EBLGraphicsPreset::Epic: return LOCTEXT("Epic", "ÉPICA");
				default: return LOCTEXT("Custom", "PERSONALIZADA");
				}
			},
			[](int32 Dir) {
				int32 P = int32(UBLUserSettings::GetGraphicsPreset());
				P = P == int32(EBLGraphicsPreset::Custom) ? int32(EBLGraphicsPreset::High) : FMath::Clamp(P + Dir, 0, 3);
				UBLUserSettings::SetGraphicsPreset(EBLGraphicsPreset(P));
				UBLUserSettings::ApplyAndSaveGraphics();
			})]
		+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Window", "PANTALLA"),
			[]() {
				const EWindowMode::Type M = GUS() ? GUS()->GetFullscreenMode() : EWindowMode::Windowed;
				return M == EWindowMode::Fullscreen ? LOCTEXT("Full", "COMPLETA") : M == EWindowMode::WindowedFullscreen ? LOCTEXT("Borderless", "SIN BORDES") : LOCTEXT("Windowed", "VENTANA");
			},
			[](int32 Dir) {
				if (UGameUserSettings* G = GUS())
				{
					G->SetFullscreenMode(EWindowMode::Type((int32(G->GetFullscreenMode()) + Dir + 3) % 3));
					UBLUserSettings::ApplyAndSaveGraphics();
				}
			})]
		+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Res", "RESOLUCIÓN"),
			[]() {
				const FIntPoint R = GUS() ? GUS()->GetScreenResolution() : FIntPoint(1920, 1080);
				return FText::FromString(FString::Printf(TEXT("%d x %d"), R.X, R.Y));
			},
			[](int32 Dir) {
				TArray<FIntPoint> Res;
				UKismetSystemLibrary::GetSupportedFullscreenResolutions(Res);
				UGameUserSettings* G = GUS();
				if (!G || Res.Num() == 0) { return; }
				int32 I = Res.IndexOfByKey(G->GetScreenResolution());
				I = I == INDEX_NONE ? Res.Num() - 1 : FMath::Clamp(I + Dir, 0, Res.Num() - 1);
				G->SetScreenResolution(Res[I]);
				UBLUserSettings::ApplyAndSaveGraphics();
			})]
		+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Scale", "ESCALA DE RESOLUCIÓN (TSR)"),
			[]() { return FText::FromString(FString::Printf(TEXT("%d %%"), FMath::RoundToInt(UBLUserSettings::GetResolutionScalePercent()))); },
			[](int32 Dir) {
				UBLUserSettings::SetResolutionScalePercent(StepClamp(UBLUserSettings::GetResolutionScalePercent(), Dir, 5.f, 50.f, 100.f));
				UBLUserSettings::ApplyAndSaveGraphics();
			},
			[]() { return TOptional<float>((UBLUserSettings::GetResolutionScalePercent() - 50.f) / 50.f); })]
		+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("VSync", "SINCRONIZACIÓN VERTICAL"),
			[]() { return YesNo(GUS() && GUS()->IsVSyncEnabled()); },
			[](int32) { if (UGameUserSettings* G = GUS()) { G->SetVSyncEnabled(!G->IsVSyncEnabled()); UBLUserSettings::ApplyAndSaveGraphics(); } })]
		+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Fps", "LÍMITE DE FPS"),
			[]() {
				const float L = GUS() ? GUS()->GetFrameRateLimit() : 0.f;
				return L <= 0.f ? LOCTEXT("NoLimit", "SIN LÍMITE") : FText::AsNumber(FMath::RoundToInt(L));
			},
			[](int32 Dir) {
				static const float Limits[] = { 0.f, 30.f, 60.f, 90.f, 120.f, 144.f };
				UGameUserSettings* G = GUS();
				if (!G) { return; }
				int32 I = 0;
				for (int32 k = 0; k < 6; ++k) { if (FMath::IsNearlyEqual(Limits[k], G->GetFrameRateLimit())) { I = k; } }
				G->SetFrameRateLimit(Limits[(I + Dir + 6) % 6]);
				UBLUserSettings::ApplyAndSaveGraphics();
			})];
}

TSharedRef<SWidget> SBLOptionsPanel::AudioPage()
{
	auto Vol = [this](const FText& Label, float UBLUserSettings::* Field)
	{
		return SNew(SBLSelectorRow).Label(Label)
			.Value_Lambda([Field]() { return Pct(UBLUserSettings::Get()->*Field); })
			.Fraction_Lambda([Field]() { return TOptional<float>(UBLUserSettings::Get()->*Field); })
			.OnStep_Lambda([this, Field](int32 Dir)
			{
				float& V = UBLUserSettings::Get()->*Field;
				V = StepClamp(V, Dir, 0.05f, 0.f, 1.f);
				ApplyLive();
			});
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Vol(LOCTEXT("Master", "VOLUMEN GENERAL"), &UBLUserSettings::MasterVolume)]
		+ SVerticalBox::Slot().AutoHeight()[Vol(LOCTEXT("Music", "MÚSICA"), &UBLUserSettings::MusicVolume)]
		+ SVerticalBox::Slot().AutoHeight()[Vol(LOCTEXT("Fx", "EFECTOS"), &UBLUserSettings::EffectsVolume)]
		+ SVerticalBox::Slot().AutoHeight()[Vol(LOCTEXT("Voice", "VOCES Y RADIO"), &UBLUserSettings::VoiceVolume)]
		+ SVerticalBox::Slot().AutoHeight()[Vol(LOCTEXT("Amb", "AMBIENTE"), &UBLUserSettings::AmbienceVolume)];
}

TSharedRef<SWidget> SBLOptionsPanel::ControlsPage()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBLSelectorRow).Label(LOCTEXT("Sens", "SENSIBILIDAD DEL RATÓN"))
			.Value_Lambda([]() { return FText::FromString(FString::Printf(TEXT("%.1f"), UBLUserSettings::Get()->MouseSensitivity)); })
			.Fraction_Lambda([]() { return TOptional<float>((UBLUserSettings::Get()->MouseSensitivity - 0.2f) / 2.8f); })
			.OnStep_Lambda([this](int32 Dir) { float& V = UBLUserSettings::Get()->MouseSensitivity; V = StepClamp(V, Dir, 0.1f, 0.2f, 3.f); ApplyLive(); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBLSelectorRow).Label(LOCTEXT("Invert", "INVERTIR EJE VERTICAL"))
			.Value_Lambda([]() { return YesNo(UBLUserSettings::Get()->bInvertY); })
			.OnStep_Lambda([this](int32) { UBLUserSettings::Get()->bInvertY = !UBLUserSettings::Get()->bInvertY; ApplyLive(); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBLSelectorRow).Label(LOCTEXT("Fov", "CAMPO DE VISIÓN"))
			.Value_Lambda([]() { return FText::FromString(FString::Printf(TEXT("%d°"), FMath::RoundToInt(UBLUserSettings::Get()->FieldOfView))); })
			.Fraction_Lambda([]() { return TOptional<float>((UBLUserSettings::Get()->FieldOfView - 70.f) / 40.f); })
			.OnStep_Lambda([this](int32 Dir) { float& V = UBLUserSettings::Get()->FieldOfView; V = StepClamp(V, Dir, 5.f, 70.f, 110.f); ApplyLive(); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBLSelectorRow).Label(LOCTEXT("Motion", "MOVIMIENTO DE CÁMARA"))
			.Value_Lambda([]() { return Pct(UBLUserSettings::Get()->MotionScale); })
			.Fraction_Lambda([]() { return TOptional<float>(UBLUserSettings::Get()->MotionScale); })
			.OnStep_Lambda([this](int32 Dir) { float& V = UBLUserSettings::Get()->MotionScale; V = StepClamp(V, Dir, 0.25f, 0.25f, 1.f); ApplyLive(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14.f, 18.f, 0.f, 0.f)
		[
			SNew(STextBlock).Font(BLMenu::Mono(11)).ColorAndOpacity(FSlateColor(BLMenu::TextDim)).AutoWrapText(true)
			.Text(LOCTEXT("Keys", "WASD MOVERSE · RATÓN APUNTAR · SHIFT CORRER · C AGACHARSE · ESPACIO SALTAR/ENCARAMARSE\nQ/E INCLINARSE · CLIC DCHO. APUNTAR · CLIC IZQ. DISPARAR · R RECARGAR · F INTERACTUAR · ESC PAUSA"))
		];
}

#undef LOCTEXT_NAMESPACE
