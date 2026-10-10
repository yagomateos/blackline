#include "UI/SBLMenuWidgets.h"
#include "UI/BLMenuData.h"

#include "UI/BLMenuStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	bool IsAccept(const FKey& K) { return K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom || K == EKeys::Virtual_Gamepad_Accept.GetVirtualKey(); }
	int32 StepOf(const FKey& K)
	{
		if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftStick_Left) return -1;
		if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_LeftStick_Right) return 1;
		return 0;
	}
}

// ---------------------------------------------------------------------------
// Botón
// ---------------------------------------------------------------------------

void SBLMenuButton::Construct(const FArguments& InArgs)
{
	OnClicked = InArgs._OnClicked;
	OnHighlighted = InArgs._OnHighlighted;
	bLocked = InArgs._bLocked;
	const TAttribute<FText> Hint = InArgs._Hint;
	const int32 Size = InArgs._FontSize;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(BLMenu::White())
		.BorderBackgroundColor_Lambda([this]() { return IsActive() ? FLinearColor(1.f, 0.62f, 0.16f, 0.1f) : FLinearColor::Transparent; })
		.Padding(0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(4.f)
				[
					SNew(SImage).Image(BLMenu::White())
					.ColorAndOpacity_Lambda([this]() { return IsActive() && !bLocked ? BLMenu::Amber : FLinearColor::Transparent; })
				]
			]
			+ SHorizontalBox::Slot().Padding(16.f, 8.f, 24.f, 8.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(InArgs._Text).Font(BLMenu::Sans(Size, true))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(bLocked ? BLMenu::Locked : IsActive() ? BLMenu::Amber : BLMenu::Text); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Hint).Font(BLMenu::Mono(10))
					.ColorAndOpacity(FSlateColor(BLMenu::TextDim))
					.Visibility_Lambda([Hint]() { return Hint.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
				]
			]
		]
	];
}

void SBLMenuButton::Activate()
{
	if (bLocked)
	{
		BLMenu::PlayBack();
		return;
	}
	BLMenu::PlaySelect();
	OnClicked.ExecuteIfBound();
}

FReply SBLMenuButton::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsAccept(InKeyEvent.GetKey()))
	{
		Activate();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SBLMenuButton::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	if (InFocusEvent.GetCause() == EFocusCause::Navigation)
	{
		BLMenu::PlayMove();
	}
	OnHighlighted.ExecuteIfBound();
	return FReply::Handled();
}

void SBLMenuButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	BLMenu::PlayMove();
	OnHighlighted.ExecuteIfBound();
}

FReply SBLMenuButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Activate();
		return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
	}
	return FReply::Unhandled();
}

// ---------------------------------------------------------------------------
// Fila de opción
// ---------------------------------------------------------------------------

void SBLSelectorRow::Construct(const FArguments& InArgs)
{
	OnStep = InArgs._OnStep;
	Fraction = InArgs._Fraction;
	auto Arrow = [this](const TCHAR* Glyph, int32 Dir)
	{
		return SNew(STextBlock).Text(FText::FromString(Glyph)).Font(BLMenu::Mono(16))
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(IsActive() ? BLMenu::Amber : BLMenu::AmberDim); });
	};
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(BLMenu::White())
		.BorderBackgroundColor_Lambda([this]() { return IsActive() ? FLinearColor(1.f, 0.62f, 0.16f, 0.08f) : FLinearColor::Transparent; })
		.Padding(FMargin(14.f, 7.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(InArgs._Label).Font(BLMenu::Sans(15, true))
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(IsActive() ? BLMenu::Amber : BLMenu::Text); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
			[
				// Barra del valor (solo si hay fracción)
				SNew(SBox).WidthOverride(140.f).HeightOverride(6.f)
				.Visibility_Lambda([this]() { return Fraction.Get().IsSet() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SImage).Image(BLMenu::White()).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.08f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox).WidthOverride_Lambda([this]() { return FOptionalSize(140.f * Fraction.Get().Get(0.f)); })
						[SNew(SImage).Image(BLMenu::White()).ColorAndOpacity_Lambda([this]() { return FSlateColor(IsActive() ? BLMenu::Amber : BLMenu::AmberDim); })]
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT("<"), -1)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
			[
				SNew(SBox).MinDesiredWidth(150.f).HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(InArgs._Value).Font(BLMenu::Mono(15)).Justification(ETextJustify::Center)
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(IsActive() ? BLMenu::Amber : BLMenu::Text); })
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT(">"), 1)]
		]
	];
}

void SBLSelectorRow::Step(int32 Dir)
{
	BLMenu::PlayTick();
	OnStep.ExecuteIfBound(Dir);
}

FReply SBLSelectorRow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (const int32 Dir = StepOf(InKeyEvent.GetKey()))
	{
		Step(Dir);
		return FReply::Handled();
	}
	if (IsAccept(InKeyEvent.GetKey()))
	{
		Step(1);
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SBLSelectorRow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Mitad derecha / izquierda del valor = subir / bajar; clic derecho = bajar
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const bool bLeftHalf = Local.X < MyGeometry.GetLocalSize().X - 100.f;
	Step(MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || bLeftHalf ? -1 : 1);
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

// ---------------------------------------------------------------------------
// Mapa topográfico
// ---------------------------------------------------------------------------

namespace
{
	float Hash2(int32 X, int32 Y)
	{
		uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return float((H ^ (H >> 16)) & 0xFFFF) / 65535.f;
	}

	float ValueNoise(float X, float Y)
	{
		const int32 IX = FMath::FloorToInt(X), IY = FMath::FloorToInt(Y);
		float FX = X - IX, FY = Y - IY;
		FX = FX * FX * (3.f - 2.f * FX);
		FY = FY * FY * (3.f - 2.f * FY);
		return FMath::Lerp(FMath::Lerp(Hash2(IX, IY), Hash2(IX + 1, IY), FX), FMath::Lerp(Hash2(IX, IY + 1), Hash2(IX + 1, IY + 1), FX), FY);
	}

	/** Relieve de Kessra: mar al oeste y al sur, colinas al este, la ría entrando desde el oeste. */
	float Height(float X, float Y)
	{
		float H = 0.f, A = 0.5f, F = 3.f;
		for (int32 O = 0; O < 4; ++O)
		{
			H += ValueNoise(X * F + 11.f, Y * F * 0.56f + 7.f) * A;
			A *= 0.5f;
			F *= 2.1f;
		}
		H += (X - 0.32f) * 1.1f + (0.82f - Y) * 0.35f;                          // sube hacia el este y el norte
		const float RiaY = 0.46f + 0.06f * FMath::Sin(X * 9.f);
		H -= FMath::Exp(-FMath::Square((Y - RiaY) / 0.025f)) * FMath::Clamp(0.75f - X, 0.f, 1.f) * 1.2f;   // ría
		return H;
	}
}

const TArray<SBLTopoMap::FMarker>& SBLTopoMap::Missions()
{
	static const TArray<FMarker> M = {
		{ FVector2D(0.585, 0.53), TEXT("01 AMANECER ROTO"), true },
		{ FVector2D(0.66, 0.66), TEXT("02 MANIFIESTO"), false },
		{ FVector2D(0.47, 0.44), TEXT("03 RÍA"), false },
		{ FVector2D(0.53, 0.30), TEXT("04 FUEGO CRUZADO"), false },
		{ FVector2D(0.71, 0.48), TEXT("05 LÍNEA NEGRA"), false },
	};
	return M;
}

void SBLTopoMap::Construct(const FArguments& InArgs)
{
	Highlight = InArgs._Highlight;
	// Marching squares sobre una rejilla; los segmentos se encadenan por la arista de la celda que comparten
	const int32 GX = 160, GY = 90;
	TArray<float> Hs;
	Hs.SetNum((GX + 1) * (GY + 1));
	for (int32 y = 0; y <= GY; ++y)
	{
		for (int32 x = 0; x <= GX; ++x)
		{
			Hs[y * (GX + 1) + x] = Height(float(x) / GX, float(y) / GY);
		}
	}
	auto H = [&](int32 x, int32 y) { return Hs[y * (GX + 1) + x]; };
	for (int32 Level = 0; Level < 22; ++Level)
	{
		const float Iso = 0.05f + Level * 0.065f;
		// arista -> (punto, segmentos que la usan)
		TMap<int64, FVector2D> EdgePoint;
		TMultiMap<int64, int64> Links;
		auto EdgeId = [&](int32 x, int32 y, bool bHorizontal) { return (int64(y) * (GX + 2) + x) * 2 + (bHorizontal ? 0 : 1); };
		auto Interp = [&](int32 x0, int32 y0, int32 x1, int32 y1)
		{
			const float a = H(x0, y0), b = H(x1, y1);
			const float t = FMath::Clamp((Iso - a) / (b - a), 0.f, 1.f);
			return FVector2D(FMath::Lerp(float(x0), float(x1), t) / GX, FMath::Lerp(float(y0), float(y1), t) / GY);
		};
		for (int32 y = 0; y < GY; ++y)
		{
			for (int32 x = 0; x < GX; ++x)
			{
				// Aristas de la celda: 0 arriba, 1 derecha, 2 abajo, 3 izquierda
				const bool c[4] = { H(x, y) > Iso, H(x + 1, y) > Iso, H(x + 1, y + 1) > Iso, H(x, y + 1) > Iso };
				int64 E[4] = { EdgeId(x, y, true), EdgeId(x + 1, y, false), EdgeId(x, y + 1, true), EdgeId(x, y, false) };
				TArray<int32, TInlineAllocator<4>> Cross;
				if (c[0] != c[1]) { Cross.Add(0); EdgePoint.FindOrAdd(E[0]) = Interp(x, y, x + 1, y); }
				if (c[1] != c[2]) { Cross.Add(1); EdgePoint.FindOrAdd(E[1]) = Interp(x + 1, y, x + 1, y + 1); }
				if (c[3] != c[2]) { Cross.Add(2); EdgePoint.FindOrAdd(E[2]) = Interp(x, y + 1, x + 1, y + 1); }
				if (c[0] != c[3]) { Cross.Add(3); EdgePoint.FindOrAdd(E[3]) = Interp(x, y, x, y + 1); }
				for (int32 i = 0; i + 1 < Cross.Num(); i += 2)
				{
					Links.Add(E[Cross[i]], E[Cross[i + 1]]);
					Links.Add(E[Cross[i + 1]], E[Cross[i]]);
				}
			}
		}
		// Encadenar en polilíneas
		TSet<int64> Used;
		for (const TPair<int64, FVector2D>& Start : EdgePoint)
		{
			if (Used.Contains(Start.Key))
			{
				continue;
			}
			TArray<FVector2D> Poly;
			int64 Cur = Start.Key;
			while (!Used.Contains(Cur))
			{
				Used.Add(Cur);
				Poly.Add(EdgePoint[Cur]);
				TArray<int64> Next;
				Links.MultiFind(Cur, Next);
				int64 N = -1;
				for (int64 Cand : Next) { if (!Used.Contains(Cand)) { N = Cand; break; } }
				if (N < 0) { break; }
				Cur = N;
			}
			if (Poly.Num() >= 3)
			{
				if (Level == 0) { Coast.Add(Poly); }
				else { Contours.Add(Poly); ContourMajor.Add(Level % 5 == 0); }
			}
		}
	}
	// La línea negra: sigue el antiguo ferrocarril de norte a sur, al este del casco viejo
	for (int32 i = 0; i <= 40; ++i)
	{
		const float T = float(i) / 40.f;
		BlackLine.Add(FVector2D(0.555f + 0.035f * FMath::Sin(T * 7.f) + 0.012f * FMath::Sin(T * 23.f), 0.08f + T * 0.84f));
	}
}

int32 SBLTopoMap::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = G.GetLocalSize();
	const double Time = FSlateApplication::Get().GetCurrentTime();
	// El mapa cubre el área manteniendo la proporción 16:9
	const float Scale = FMath::Max(Size.X / 16.f, Size.Y / 9.f);
	const FVector2D MapSize(16.f * Scale, 9.f * Scale);
	const FVector2D Offset = (Size - MapSize) * 0.5f;
	auto P = [&](const FVector2D& N) { return Offset + N * MapSize; };
	auto Lines = [&](const TArray<FVector2D>& Norm, const FLinearColor& C, float Thick, int32 Layer)
	{
		TArray<FVector2D> Pts;
		Pts.Reserve(Norm.Num());
		for (const FVector2D& N : Norm) { Pts.Add(P(N)); }
		FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, C, true, Thick);
	};

	FSlateDrawElement::MakeBox(Out, LayerId, G.ToPaintGeometry(), BLMenu::White(), ESlateDrawEffect::None, BLMenu::Bg);
	// Cuadrícula con coordenadas
	for (int32 i = 1; i < 16; ++i)
	{
		Lines({ FVector2D(i / 16.f, 0.f), FVector2D(i / 16.f, 1.f) }, FLinearColor(1.f, 1.f, 1.f, 0.035f), 1.f, LayerId + 1);
		FSlateDrawElement::MakeText(Out, LayerId + 1, G.ToPaintGeometry(FVector2f(60.f, 14.f), FSlateLayoutTransform(FVector2f(P(FVector2D(i / 16.f, 0.f)) + FVector2D(4.f, 4.f)))),
			FString::Printf(TEXT("%02d"), 40 + i), BLMenu::Mono(8), ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.18f));
	}
	for (int32 i = 1; i < 9; ++i)
	{
		Lines({ FVector2D(0.f, i / 9.f), FVector2D(1.f, i / 9.f) }, FLinearColor(1.f, 1.f, 1.f, 0.035f), 1.f, LayerId + 1);
	}
	for (int32 i = 0; i < Contours.Num(); ++i)
	{
		Lines(Contours[i], FLinearColor(0.55f, 0.6f, 0.55f, ContourMajor[i] ? 0.22f : 0.09f), ContourMajor[i] ? 1.4f : 1.f, LayerId + 2);
	}
	for (const TArray<FVector2D>& C : Coast)
	{
		Lines(C, FLinearColor(BLMenu::AmberDim.R, BLMenu::AmberDim.G, BLMenu::AmberDim.B, 0.7f), 1.8f, LayerId + 3);
	}
	// La línea negra: banda oscura ancha + núcleo rojizo que respira
	const float Pulse = 0.75f + 0.25f * FMath::Sin(float(Time) * 1.3f);
	Lines(BlackLine, FLinearColor(0.f, 0.f, 0.f, 0.85f), 9.f, LayerId + 4);
	Lines(BlackLine, FLinearColor(BLMenu::Line.R, BLMenu::Line.G, BLMenu::Line.B, 0.75f * Pulse), 2.f, LayerId + 5);
	// Rótulos de zona
	const TPair<FVector2D, const TCHAR*> Zones[] = { { FVector2D(0.27, 0.62), TEXT("KESSRA-OESTE") }, { FVector2D(0.69, 0.40), TEXT("KESSRA-ESTE  //  COLUMNA VESK") },
		{ FVector2D(0.63, 0.74), TEXT("PUERTO") }, { FVector2D(0.78, 0.62), TEXT("REFINERÍA") }, { FVector2D(0.42, 0.50), TEXT("CASCO VIEJO") },
		{ FVector2D(0.12, 0.84), TEXT("MAR") }, { FVector2D(0.565, 0.06), TEXT("LÍNEA NEGRA") } };
	for (const auto& Z : Zones)
	{
		FSlateDrawElement::MakeText(Out, LayerId + 6, G.ToPaintGeometry(FVector2f(400.f, 16.f), FSlateLayoutTransform(FVector2f(P(Z.Key)))),
			FString(Z.Value), BLMenu::Mono(10), ESlateDrawEffect::None, FLinearColor(0.7f, 0.72f, 0.68f, 0.38f));
	}
	// Misiones
	const int32 Hi = Highlight.Get(-1);
	for (int32 i = 0; i < Missions().Num(); ++i)
	{
		const FMarker& M = Missions()[i];
		// Disponible según los datos del menú (BLMenuData), no según la lista del mapa
		const bool bAvail = i < int32(UE_ARRAY_COUNT(BLMenuData::Missions)) ? BLMenuData::Missions[i].bAvailable : M.bAvailable;
		const FVector2D C = P(M.Pos);
		const FLinearColor Col = bAvail ? BLMenu::Amber : BLMenu::Locked;
		const float R = (i == Hi ? 13.f : 8.f) + (bAvail ? 3.f * FMath::Sin(float(Time) * 3.f + i) : 0.f);
		TArray<FVector2D> Ring;
		for (int32 k = 0; k <= 24; ++k)
		{
			const float A = k / 24.f * 2.f * PI;
			Ring.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
		}
		FSlateDrawElement::MakeLines(Out, LayerId + 7, G.ToPaintGeometry(), Ring, ESlateDrawEffect::None, Col, true, i == Hi ? 2.f : 1.4f);
		FSlateDrawElement::MakeBox(Out, LayerId + 7, G.ToPaintGeometry(FVector2f(6.f, 6.f), FSlateLayoutTransform(FVector2f(C - FVector2D(3.f, 3.f)))), BLMenu::White(), ESlateDrawEffect::None, Col);
		FSlateDrawElement::MakeText(Out, LayerId + 7, G.ToPaintGeometry(FVector2f(300.f, 16.f), FSlateLayoutTransform(FVector2f(C + FVector2D(R + 6.f, -8.f)))),
			M.Label + (bAvail ? TEXT("") : TEXT("  [BLOQUEADA]")), BLMenu::Mono(i == Hi ? 12 : 10), ESlateDrawEffect::None, Col);
	}
	// Barrido de radar lento y viñeta
	const float Sweep = float(FMath::Fmod(Time * 0.06, 1.0));
	FSlateDrawElement::MakeBox(Out, LayerId + 8, G.ToPaintGeometry(FVector2f(Size.X, 2.f), FSlateLayoutTransform(FVector2f(0.f, Sweep * Size.Y))),
		BLMenu::White(), ESlateDrawEffect::None, FLinearColor(1.f, 0.62f, 0.16f, 0.06f));
	return LayerId + 9;
}
