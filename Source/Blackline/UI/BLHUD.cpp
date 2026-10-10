#include "UI/BLHUD.h"

#include "UI/BLMenuData.h"
#include "Vehicles/BLBoat.h"

#include "Combat/BLHealthComponent.h"
#include "Mission/BLCheckpointSubsystem.h"
#include "Mission/BLInteractable.h"
#include "Weapons/BLMountedGun.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/CameraComponent.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor Amber(1.f, 0.72f, 0.26f, 1.f);
	const FLinearColor Ink(0.93f, 0.93f, 0.9f, 1.f);
	const FLinearColor Alert(0.95f, 0.25f, 0.15f, 1.f);

	FLinearColor WithAlpha(FLinearColor C, float A)
	{
		C.A *= FMath::Clamp(A, 0.f, 1.f);
		return C;
	}
}

ABLHUD::ABLHUD()
{
	static ConstructorHelpers::FObjectFinder<UFont> Mono(TEXT("/Engine/EngineFonts/DroidSansMono.DroidSansMono"));
	static ConstructorHelpers::FObjectFinder<UFont> Roboto(TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	MonoFont = Mono.Object;
	TextFont = Roboto.Object;
}

// ---------------------------------------------------------------------------
// Primitivas
// ---------------------------------------------------------------------------

float ABLHUD::S() const
{
	return Canvas ? Canvas->ClipY / 1080.f : 1.f;
}

FVector2D ABLHUD::Measure(const FString& Str, float Size, bool bMono) const
{
	const UFont* Font = bMono ? MonoFont : TextFont;
	if (!Font || !FSlateApplication::IsInitialized())
	{
		return FVector2D(Str.Len() * Size * 0.6f, Size);
	}
	return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str, FSlateFontInfo(Font, Size));
}

void ABLHUD::Text(const FString& Str, FVector2D Pos, float Size, const FLinearColor& Color, bool bMono, float AlignX, bool bShadow)
{
	const UFont* Font = bMono ? MonoFont : TextFont;
	if (!Font || Color.A <= 0.01f)
	{
		return;
	}
	if (AlignX > 0.f)
	{
		Pos.X -= Measure(Str, Size, bMono).X * AlignX;
	}
	FCanvasTextItem Item(Pos, FText::FromString(Str), FSlateFontInfo(Font, Size), Color);
	if (bShadow)
	{
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.65f * Color.A), FVector2D(1.5f, 1.5f));
	}
	Canvas->DrawItem(Item);
}

void ABLHUD::Rect(FVector2D Pos, FVector2D Size, const FLinearColor& Color)
{
	FCanvasTileItem Tile(Pos, Size, Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void ABLHUD::Line(FVector2D A, FVector2D B, const FLinearColor& Color, float Thickness)
{
	DrawLine(A.X, A.Y, B.X, B.Y, Color, Thickness);
}

// ---------------------------------------------------------------------------
// Combate
// ---------------------------------------------------------------------------

void ABLHUD::DrawCrosshair(const ABLCharacter* Char, float Alpha)
{
	const UBLWeaponComponent* W = Char->GetWeapon();
	if (Alpha <= 0.01f || !W)
	{
		return;
	}
	const float FOV = Char->GetCamera()->FieldOfView;
	const float HalfW = Canvas->ClipX * 0.5f;
	const float TargetGap = FMath::Tan(FMath::DegreesToRadians(W->GetCurrentSpread())) / FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f)) * HalfW;
	ShownGap = FMath::FInterpTo(ShownGap, FMath::Max(TargetGap, 4.f * S()), GetWorld()->GetDeltaSeconds(), 25.f);
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float Len = 9.f * S();
	for (const FVector2D D : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
	{
		const FVector2D A = C + D * ShownGap, B = C + D * (ShownGap + Len);
		Line(A + FVector2D(1, 1), B + FVector2D(1, 1), FLinearColor(0, 0, 0, 0.5f * Alpha), 2.f);
		Line(A, B, WithAlpha(Ink, 0.85f * Alpha), 2.f);
	}
}

void ABLHUD::DrawHitMarker(const ABLCharacter* Char)
{
	const float Age = Char->GetHitMarkerAge();
	const bool bKill = Char->IsHitMarkerKill();
	const float Life = bKill ? 0.45f : 0.28f;
	if (Age > Life)
	{
		return;
	}
	const float T = Age / Life;
	const bool bHead = Char->GetHitMarkerZone() == EBLHitZone::Head;
	const float Pop = 1.f + 0.35f * FMath::Exp(-Age * 30.f);
	const float Inner = (8.f + (bKill ? 3.f : 0.f)) * S() * Pop;
	const float Len = (bHead || bKill ? 11.f : 8.f) * S() * Pop;
	const FLinearColor Col = WithAlpha(bKill ? Alert : Ink, 1.f - T);
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	for (const FVector2D D : { FVector2D(1, 1), FVector2D(-1, 1), FVector2D(1, -1), FVector2D(-1, -1) })
	{
		const FVector2D N = D.GetSafeNormal();
		Line(C + N * Inner + FVector2D(1, 1), C + N * (Inner + Len) + FVector2D(1, 1), FLinearColor(0, 0, 0, 0.5f * (1.f - T)), 2.5f * S());
		Line(C + N * Inner, C + N * (Inner + Len), Col, 2.5f * S());
	}
}

void ABLHUD::DrawDamageIndicators(const ABLCharacter* Char)
{
	// Arcos rojos alrededor de la mira, en la dirección del tirador (girando con la vista): grandes y gruesos,
	// lejos de la mira para no tapar al enemigo; se desvanecen en los últimos 0,8 s
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float Radius = Canvas->ClipY * 0.26f;
	const FRotator View = Char->GetControlRotation();
	for (const ABLCharacter::FDamageIndicator& I : Char->GetDamageIndicators())
	{
		const FVector Local = FRotator(0.f, View.Yaw, 0.f).UnrotateVector((I.Source - Char->GetActorLocation()).GetSafeNormal2D());
		const float Ang = FMath::Atan2(Local.Y, Local.X);
		const float Age = Now - I.Time;
		const float Fade = 1.f - FMath::SmoothStep(1.8f, 2.6f, Age);
		const float Pop = 1.f + 0.12f * FMath::Max(0.f, 1.f - Age / 0.15f);   // aparece con un pequeño golpe
		const FLinearColor Col = WithAlpha(FLinearColor(0.9f, 0.08f, 0.04f, 0.9f), FMath::Lerp(0.55f, 1.f, I.Strength) * Fade);
		const float Half = FMath::DegreesToRadians(22.f);
		const int32 N = 14;
		for (int32 k = 0; k < N; ++k)
		{
			const float A0 = Ang - Half + 2.f * Half * k / N, A1 = Ang - Half + 2.f * Half * (k + 1) / N;
			const float Mid = 1.f - FMath::Abs((k + 0.5f) / N * 2.f - 1.f);
			Line(C + FVector2D(FMath::Sin(A0), -FMath::Cos(A0)) * Radius * Pop, C + FVector2D(FMath::Sin(A1), -FMath::Cos(A1)) * Radius * Pop, Col, (4.f + 10.f * Mid) * S());
		}
		// Punta hacia fuera: indica claramente "de ahí viene"
		const FVector2D Dir(FMath::Sin(Ang), -FMath::Cos(Ang));
		const FVector2D Side(-Dir.Y, Dir.X);
		const FVector2D Tip = C + Dir * (Radius * Pop + 26.f * S());
		const FVector2D Base = C + Dir * (Radius * Pop + 6.f * S());
		Line(Tip, Base + Side * 14.f * S(), Col, 4.f * S());
		Line(Tip, Base - Side * 14.f * S(), Col, 4.f * S());
	}
}

void ABLHUD::DrawDamageVignette(const ABLCharacter* Char)
{
	// Bordes de la pantalla en rojo: el golpe (destello, más fuerte en el lado del tirador) y la salud baja (constante,
	// late con el corazón). El centro queda limpio: no tapa al enemigo ni la mira
	const UBLHealthComponent* H = Char->GetHealth();
	const float Frac = H ? H->GetHealthFraction() : 1.f;
	const float Low = (1.f - FMath::SmoothStep(0.15f, 0.55f, Frac)) * (0.75f + 0.25f * FMath::Sin(GetWorld()->GetTimeSeconds() * 7.5f));
	const float Flash = Char->GetDamageFlash();
	if (Flash < 0.01f && Low < 0.01f)
	{
		return;
	}
	// Lado del último golpe (0 arriba, 1 dcha, 2 abajo, 3 izda)
	float SideBoost[4] = { 0.f, 0.f, 0.f, 0.f };
	if (Char->GetDamageIndicators().Num() > 0)
	{
		const ABLCharacter::FDamageIndicator& Last = Char->GetDamageIndicators().Last();
		const FVector Local = FRotator(0.f, Char->GetControlRotation().Yaw, 0.f).UnrotateVector((Last.Source - Char->GetActorLocation()).GetSafeNormal2D());
		SideBoost[0] = FMath::Max(0.f, (float)Local.X);
		SideBoost[2] = FMath::Max(0.f, (float)-Local.X);
		SideBoost[1] = FMath::Max(0.f, (float)Local.Y);
		SideBoost[3] = FMath::Max(0.f, (float)-Local.Y);
	}
	const float W = Canvas->ClipX, Hh = Canvas->ClipY;
	const float Depth = Hh * 0.16f;
	const int32 Bands = 18;
	for (int32 Side = 0; Side < 4; ++Side)
	{
		const float Strength = FMath::Clamp(0.55f * Low + Flash * (0.45f + 0.75f * SideBoost[Side]), 0.f, 1.f);
		if (Strength < 0.01f)
		{
			continue;
		}
		for (int32 b = 0; b < Bands; ++b)
		{
			const float T = float(b) / Bands;
			const float A = Strength * 0.55f * (1.f - T) * (1.f - T);
			const FLinearColor Col(0.45f, 0.01f, 0.01f, FMath::Min(A, 0.6f));
			const float D0 = Depth * T, D1 = Depth * (T + 1.f / Bands);
			switch (Side)
			{
			case 0: Rect(FVector2D(0.f, D0), FVector2D(W, D1 - D0), Col); break;
			case 2: Rect(FVector2D(0.f, Hh - D1), FVector2D(W, D1 - D0), Col); break;
			case 1: Rect(FVector2D(W - D1, 0.f), FVector2D(D1 - D0, Hh), Col); break;
			default: Rect(FVector2D(D0, 0.f), FVector2D(D1 - D0, Hh), Col); break;
			}
		}
	}
}

void ABLHUD::DrawHealth(const ABLCharacter* Char)
{
	// Salud en segmentos (abajo a la izquierda): aparece al recibir daño o con la salud por debajo del máximo y se
	// desvanece con la salud llena; el segmento que regenera parpadea
	const UBLHealthComponent* H = Char->GetHealth();
	if (!H)
	{
		return;
	}
	const float Frac = H->GetHealthFraction();
	const float Since = H->GetTimeSinceDamage();
	const float A = Frac < 0.999f ? 1.f : 1.f - FMath::SmoothStep(2.f, 3.f, Since);
	if (A < 0.01f)
	{
		return;
	}
	const int32 Segs = 4;
	const float SegW = 64.f * S(), SegH = 9.f * S(), Gap = 6.f * S();
	const FVector2D Pos(64.f * S(), Canvas->ClipY - 66.f * S());
	const float PerSeg = 1.f / Segs;
	for (int32 i = 0; i < Segs; ++i)
	{
		const FVector2D P(Pos.X + i * (SegW + Gap), Pos.Y);
		Rect(P, FVector2D(SegW, SegH), FLinearColor(0.f, 0.f, 0.f, 0.45f * A));
		const float Fill = FMath::Clamp((Frac - i * PerSeg) / PerSeg, 0.f, 1.f);
		const FLinearColor Col = Frac < 0.3f ? Alert : WithAlpha(Ink, 0.9f);
		Rect(P, FVector2D(SegW * Fill, SegH), WithAlpha(Col, A * (Fill < 1.f && Fill > 0.f && Since > 3.5f ? 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f) : 1.f)));
	}
	Text(TEXT("SALUD"), FVector2D(Pos.X, Pos.Y - 18.f * S()), 11.f * S(), WithAlpha(Frac < 0.3f ? Alert : Amber, 0.8f * A), true);
}

void ABLHUD::DrawAmmo(const ABLCharacter* Char)
{
	const UBLWeaponComponent* W = Char->GetWeapon();
	const UBLWeaponData* Data = W ? W->GetWeaponData() : nullptr;
	if (!Data)
	{
		return;
	}
	// Contextual: aparece al disparar, recargar, apuntar o cambiar la munición; se atenúa a los 4 s
	const float Dt = GetWorld()->GetDeltaSeconds();
	AmmoAttention += Dt;
	if (W->GetShotsFired() != LastShots || W->IsReloading() || W->GetMagazine() != LastMag || Char->IsAiming() || Char->GetGrenades() != LastGrenades)
	{
		AmmoAttention = 0.f;
	}
	LastShots = W->GetShotsFired();
	LastMag = W->GetMagazine();
	LastGrenades = Char->GetGrenades();
	const bool bLow = W->GetMagazine() <= Data->MagazineSize / 4;
	const float Target = (AmmoAttention < 4.f || bLow) ? 1.f : 0.28f;
	AmmoAlpha = FMath::FInterpTo(AmmoAlpha, Target, Dt, AmmoAlpha < Target ? 12.f : 1.5f);

	const float X = Canvas->ClipX - 64.f * S();
	const float Y = Canvas->ClipY - 70.f * S();
	const FString Mag = FString::Printf(TEXT("%02d"), W->GetMagazine());
	const FString Res = FString::Printf(TEXT(" / %d"), W->GetReserve());
	const float ResW = Measure(Res, 18.f * S(), true).X;
	Text(Res, FVector2D(X - ResW, Y + 14.f * S()), 18.f * S(), WithAlpha(Ink, 0.8f * AmmoAlpha), true);
	Text(Mag, FVector2D(X - ResW, Y - 6.f * S()), 34.f * S(), WithAlpha(bLow ? Alert : Ink, AmmoAlpha), true, 1.f);
	Text(Data->DisplayName.ToUpper().ToString(), FVector2D(X, Y - 26.f * S()), 13.f * S(), WithAlpha(Amber, 0.9f * AmmoAlpha), true, 1.f);
	// Barra del cargador
	const float BarW = 150.f * S();
	const float Frac = float(W->GetMagazine()) / FMath::Max(Data->MagazineSize, 1);
	Rect(FVector2D(X - BarW, Y + 44.f * S()), FVector2D(BarW, 2.f * S()), WithAlpha(FLinearColor(1, 1, 1, 0.18f), AmmoAlpha));
	Rect(FVector2D(X - BarW, Y + 44.f * S()), FVector2D(BarW * FMath::Min(Frac, 1.f), 2.f * S()), WithAlpha(bLow ? Alert : Amber, AmmoAlpha));
	// Granadas: un rombo por granada (rellenos = disponibles)
	for (int32 i = 0; i < Char->GetMaxGrenades(); ++i)
	{
		const bool bHave = i < Char->GetGrenades();
		const FVector2D P(X - 8.f * S() - i * 14.f * S(), Y + 56.f * S());
		Rect(P, FVector2D(8.f * S(), 8.f * S()), WithAlpha(bHave ? Amber : FLinearColor(1, 1, 1, 0.15f), AmmoAlpha));
	}
	Text(TEXT("M-6"), FVector2D(X - 14.f * S() * Char->GetMaxGrenades() - 10.f * S(), Y + 52.f * S()), 11.f * S(), WithAlpha(Ink, 0.7f * AmmoAlpha), true, 1.f);

	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f + 64.f * S());
	if (W->IsReloading())
	{
		Text(TEXT("RECARGANDO"), Center, 15.f * S(), WithAlpha(Amber, 0.9f), true, 0.5f);
	}
	else if (W->GetMagazine() == 0 && W->GetReserve() == 0)
	{
		Text(TEXT("SIN MUNICIÓN"), Center, 15.f * S(), Alert, true, 0.5f);
	}
}

// ---------------------------------------------------------------------------
// Misión
// ---------------------------------------------------------------------------

void ABLHUD::DrawObjective(const ABLMissionDirector* Director)
{
	const FBLObjective* O = Director ? Director->GetCurrentObjective() : nullptr;
	if (!O)
	{
		return;
	}
	const float Age = Director->GetObjectiveAge();
	// Recién cambiado: entra deslizando y destaca; después queda discreto
	const float In = FMath::Clamp(Age / 0.35f, 0.f, 1.f);
	const float Fresh = 1.f - FMath::SmoothStep(5.f, 7.f, Age);
	const float Alpha = In * FMath::Lerp(0.6f, 1.f, Fresh);
	const FVector2D P(FMath::Lerp(20.f, 52.f, In) * S(), 52.f * S());
	Rect(P + FVector2D(-12.f * S(), 0.f), FVector2D(3.f * S(), 52.f * S()), WithAlpha(Amber, Alpha));
	Text(FString::Printf(TEXT("OBJETIVO %02d/%02d"), Director->GetCurrentIndex() + 1, Director->Objectives.Num()), P, 13.f * S(), WithAlpha(Amber, Alpha), true);
	Text(Director->GetObjectiveDisplayText(), P + FVector2D(0.f, 22.f * S()), 19.f * S(), WithAlpha(Ink, Alpha), false);
	if (Fresh > 0.01f && Age < 1.2f)
	{
		Text(TEXT("NUEVO"), P + FVector2D(Measure(FString::Printf(TEXT("OBJETIVO %02d/%02d"), 1, 1), 13.f * S(), true).X + 12.f * S(), 0.f), 13.f * S(),
			WithAlpha(Ink, (1.f - Age / 1.2f) * Alpha), true);
	}
}

void ABLHUD::DrawMarker(const ABLCharacter* Char, const ABLMissionDirector* Director)
{
	FVector Target;
	if (!Director || !Director->GetMarkerLocation(Target))
	{
		return;
	}
	const FVector Eye = Char->GetCamera()->GetComponentLocation();
	const float Dist = FVector::Dist(Eye, Target) / 100.f;
	const float Alpha = FMath::Clamp((Dist - 2.f) / 3.f, 0.f, 1.f) * 0.9f;
	if (Alpha <= 0.01f)
	{
		return;
	}
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const bool bBehind = FVector::DotProduct(Target - Eye, Char->GetCamera()->GetForwardVector()) < 0.f;
	FVector Proj = Canvas->Project(Target);
	FVector2D P(Proj.X, Proj.Y);
	if (bBehind)
	{
		P = C + (C - P);   // detrás: el punto proyectado sale invertido
	}
	// Pegado al borde (elipse) si sale de pantalla
	const FVector2D Margin(Canvas->ClipX * 0.44f, Canvas->ClipY * 0.40f);
	FVector2D Off = P - C;
	const float K = FMath::Square(Off.X / Margin.X) + FMath::Square(Off.Y / Margin.Y);
	const bool bOff = bBehind || K > 1.f;
	if (bOff)
	{
		Off /= FMath::Sqrt(FMath::Max(K, 1e-4f));
		if (bBehind && FMath::Abs(Off.Y) < Margin.Y * 0.2f)
		{
			Off.Y = Margin.Y * 0.6f;   // detrás: abajo, hacia el lado por el que girar
		}
		P = C + Off;
	}
	const float R = 9.f * S();
	const FLinearColor Col = WithAlpha(Amber, Alpha);
	const FVector2D Up(0, -R), Right(R, 0), Down(0, R), Left(-R, 0);
	for (const auto& Seg : { TPair<FVector2D, FVector2D>(Up, Right), TPair<FVector2D, FVector2D>(Right, Down), TPair<FVector2D, FVector2D>(Down, Left), TPair<FVector2D, FVector2D>(Left, Up) })
	{
		Line(P + Seg.Key + FVector2D(1, 1), P + Seg.Value + FVector2D(1, 1), FLinearColor(0, 0, 0, 0.5f * Alpha), 2.f * S());
		Line(P + Seg.Key, P + Seg.Value, Col, 2.f * S());
	}
	Rect(P - FVector2D(2.f, 2.f) * S(), FVector2D(4.f, 4.f) * S(), Col);
	if (bOff)
	{
		// Flecha hacia el objetivo
		const FVector2D Dir = Off.GetSafeNormal();
		const FVector2D Tip = P + Dir * 20.f * S();
		const FVector2D Side(-Dir.Y, Dir.X);
		Line(Tip, Tip - Dir * 8.f * S() + Side * 6.f * S(), Col, 2.f * S());
		Line(Tip, Tip - Dir * 8.f * S() - Side * 6.f * S(), Col, 2.f * S());
	}
	Text(FString::Printf(TEXT("%d m"), FMath::RoundToInt(Dist)), P + FVector2D(0.f, 14.f * S()), 13.f * S(), WithAlpha(Ink, Alpha), true, 0.5f);
}

void ABLHUD::DrawInteraction(const ABLCharacter* Char)
{
	const ABLInteractable* I = Char->GetFocusedInteractable();
	if (!I)
	{
		return;
	}
	const FVector2D P(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f + 92.f * S());
	const FString Key = TEXT("[F]");
	const FString Label = I->Prompt;
	const float KeyW = Measure(Key, 16.f * S(), true).X;
	const float LabelW = Measure(Label, 18.f * S(), false).X;
	const float Total = KeyW + 10.f * S() + LabelW;
	const float X0 = P.X - Total * 0.5f;
	Text(Key, FVector2D(X0, P.Y + 1.f * S()), 16.f * S(), Amber, true);
	Text(Label, FVector2D(X0 + KeyW + 10.f * S(), P.Y), 18.f * S(), Ink, false);
	const float Progress = Char->GetInteractProgress();
	if (Progress > 0.f)
	{
		const float BarW = FMath::Max(Total, 200.f * S());
		Rect(FVector2D(P.X - BarW * 0.5f, P.Y + 32.f * S()), FVector2D(BarW, 3.f * S()), FLinearColor(1, 1, 1, 0.2f));
		Rect(FVector2D(P.X - BarW * 0.5f, P.Y + 32.f * S()), FVector2D(BarW * Progress, 3.f * S()), Amber);
	}
}

void ABLHUD::DrawSubtitle(const ABLMissionDirector* Director)
{
	FString Speaker, Body;
	float Alpha = 0.f;
	if (!Director || !Director->GetSubtitle(Speaker, Body, Alpha))
	{
		return;
	}
	// Ajuste de línea al 56 % del ancho
	const float Size = 21.f * S();
	const float MaxW = Canvas->ClipX * 0.56f;
	const FString Head = Speaker.ToUpper() + TEXT("  ");
	const float HeadW = Measure(Head, 16.f * S(), true).X;
	TArray<FString> Words, Lines;
	Body.ParseIntoArray(Words, TEXT(" "));
	FString Cur;
	for (const FString& Wd : Words)
	{
		const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
		if (Measure(Try, Size, false).X + (Lines.Num() == 0 ? HeadW : 0.f) > MaxW && !Cur.IsEmpty())
		{
			Lines.Add(Cur);
			Cur = Wd;
		}
		else
		{
			Cur = Try;
		}
	}
	if (!Cur.IsEmpty())
	{
		Lines.Add(Cur);
	}
	const float LineH = Size * 1.45f;
	float Y = Canvas->ClipY * 0.80f - (Lines.Num() - 1) * LineH;
	float Widest = 0.f;
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		Widest = FMath::Max(Widest, Measure(Lines[i], Size, false).X + (i == 0 ? HeadW : 0.f));
	}
	const float X0 = Canvas->ClipX * 0.5f - Widest * 0.5f;
	Rect(FVector2D(X0 - 16.f * S(), Y - 8.f * S()), FVector2D(Widest + 32.f * S(), LineH * Lines.Num() + 10.f * S()), FLinearColor(0.f, 0.f, 0.f, 0.38f * Alpha));
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		float X = X0;
		if (i == 0)
		{
			Text(Head, FVector2D(X, Y + 4.f * S()), 16.f * S(), WithAlpha(Amber, Alpha), true);
			X += HeadW;
		}
		Text(Lines[i], FVector2D(X, Y), Size, WithAlpha(Ink, Alpha), false);
		Y += LineH;
	}
}

void ABLHUD::DrawCheckpointNotice()
{
	const UBLCheckpointSubsystem* Checkpoints = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>();
	if (!Checkpoints)
	{
		return;
	}
	const float Age = GetWorld()->GetTimeSeconds() - Checkpoints->GetLastReachedTime();
	if (Age < 0.f || Age > 3.f)
	{
		return;
	}
	const float Alpha = FMath::Min(Age / 0.3f, 1.f) * (1.f - FMath::SmoothStep(2.3f, 3.f, Age));
	Text(TEXT("PUNTO DE CONTROL"), FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.2f), 16.f * S(), WithAlpha(Amber, Alpha), true, 0.5f);
}

void ABLHUD::DrawPhotoFlash()
{
	const float Age = GetWorld()->GetTimeSeconds() - ABLInteractable::GetLastPhotoTime();
	if (Age < 0.f || Age > 1.2f)
	{
		return;
	}
	// Destello blanco muy corto y, encima, el visor (esquinas y "FOTO n") que se desvanece
	const float Flash = FMath::Clamp(1.f - Age / 0.18f, 0.f, 1.f);
	if (Flash > 0.f)
	{
		Rect(FVector2D::ZeroVector, FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(1.f, 1.f, 1.f, 0.55f * Flash));
	}
	const float A = 1.f - FMath::SmoothStep(0.7f, 1.2f, Age);
	const FLinearColor C = WithAlpha(Ink, A * 0.9f);
	const float W = Canvas->ClipX * 0.3f, H = Canvas->ClipY * 0.28f, L = 34.f * S();
	const FVector2D Ctr(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	for (int32 i = 0; i < 4; ++i)
	{
		const float SX = (i & 1) ? 1.f : -1.f, SY = (i & 2) ? 1.f : -1.f;
		const FVector2D P(Ctr.X + SX * W, Ctr.Y + SY * H);
		Line(P, P - FVector2D(SX * L, 0.f), C, 2.f * S());
		Line(P, P - FVector2D(0.f, SY * L), C, 2.f * S());
	}
	Text(FString::Printf(TEXT("FOTO %d  ·  ENVIADA A TORRE"), ABLInteractable::GetPhotosTaken()), FVector2D(Ctr.X, Ctr.Y + H + 14.f * S()),
		13.f * S(), WithAlpha(Amber, A), true, 0.5f);
}

void ABLHUD::DrawBoat(const ABLCharacter* Char)
{
	const ABLBoat* Boat = Char->GetDrivenBoat();
	if (!Boat)
	{
		return;
	}
	// Velocidad en nudos y controles, abajo en el centro
	const float CX = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY - 92.f * S();
	const float Knots = FMath::Abs(Boat->GetCurrentSpeed()) * 0.036f / 1.852f;
	Text(FString::Printf(TEXT("%02d"), FMath::RoundToInt(Knots)), FVector2D(CX, Y), 30.f * S(), WithAlpha(Ink, 0.9f), true, 0.5f);
	Text(Boat->GetCurrentSpeed() < -10.f ? TEXT("NUDOS · ATRÁS") : TEXT("NUDOS"), FVector2D(CX, Y + 36.f * S()), 11.f * S(), WithAlpha(Amber, 0.8f), true, 0.5f);
	Text(TEXT("[W/S] ACELERAR / FRENAR    [A/D] TIMÓN"), FVector2D(CX, Y + 58.f * S()), 11.f * S(), WithAlpha(Ink, 0.55f), true, 0.5f);
}

void ABLHUD::DrawMountedGun(const ABLCharacter* Char)
{
	const ABLMountedGun* Gun = Char->GetMountedGun();
	if (!Gun)
	{
		return;
	}
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float R = 22.f * S();
	const FLinearColor Col = Gun->IsOverheated() ? Alert : WithAlpha(Ink, 0.85f);
	// Retícula de ametralladora: anillo partido + punto
	for (int32 i = 0; i < 4; ++i)
	{
		const float A0 = HALF_PI * i + 0.35f, A1 = HALF_PI * (i + 1) - 0.35f;
		for (int32 k = 0; k < 6; ++k)
		{
			const float T0 = FMath::Lerp(A0, A1, k / 6.f), T1 = FMath::Lerp(A0, A1, (k + 1) / 6.f);
			Line(C + FVector2D(FMath::Cos(T0), FMath::Sin(T0)) * R, C + FVector2D(FMath::Cos(T1), FMath::Sin(T1)) * R, Col, 1.6f * S());
		}
	}
	Rect(C - FVector2D(1.5f, 1.5f) * S(), FVector2D(3.f, 3.f) * S(), Col);
	// Calor: barra abajo en el centro
	const FVector2D BarPos(C.X - 110.f * S(), Canvas->ClipY - 70.f * S());
	const FVector2D BarSize(220.f * S(), 6.f * S());
	Rect(BarPos, BarSize, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	const float H = FMath::Clamp(Gun->GetHeat(), 0.f, 1.f);
	Rect(BarPos, FVector2D(BarSize.X * H, BarSize.Y), FMath::Lerp(Amber, Alert, FMath::Clamp((H - 0.6f) / 0.4f, 0.f, 1.f)));
	Text(Gun->IsOverheated() ? TEXT("SOBRECALENTADA") : TEXT("CALOR"), FVector2D(C.X, BarPos.Y - 20.f * S()), 11.f * S(),
		Gun->IsOverheated() ? Alert : WithAlpha(Ink, 0.7f), true, 0.5f);
	Text(TEXT("[F] Bajarse"), FVector2D(C.X, BarPos.Y + 16.f * S()), 11.f * S(), WithAlpha(Ink, 0.55f), true, 0.5f);
}

void ABLHUD::DrawAlarm(const ABLMissionDirector* Director)
{
	const float Age = Director ? Director->GetAlarmAge() : -1.f;
	if (Age < 0.f || Age > 8.f)
	{
		return;
	}
	// Parpadea al saltar y se queda unos segundos
	const float Blink = Age < 3.f ? (FMath::Fmod(Age, 0.5f) < 0.3f ? 1.f : 0.25f) : 1.f - FMath::SmoothStep(6.f, 8.f, Age);
	Text(TEXT("ALARMA"), FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.14f), 17.f * S(), WithAlpha(Alert, Blink), true, 0.5f);
}

void ABLHUD::DrawDeath(const ABLCharacter* Char)
{
	const float T = Char->GetDeathTime();
	const float Alpha = FMath::Clamp((T - 0.6f) / 0.6f, 0.f, 1.f);
	Text(TEXT("HAS CAÍDO"), FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.44f), 30.f * S(), WithAlpha(Alert, Alpha), true, 0.5f);
	Text(TEXT("Volviendo al último punto de control"), FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.44f + 46.f * S()), 18.f * S(), WithAlpha(Ink, Alpha * 0.85f), false, 0.5f);
}

void ABLHUD::DrawMissionFailed(const ABLMissionDirector* Director)
{
	const float Age = Director->GetFailAge();
	const float Fade = FMath::Clamp(Age / 0.8f, 0.f, 1.f);
	Rect(FVector2D::ZeroVector, FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(0.02f, 0.005f, 0.005f, 0.8f * Fade));
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.42f);
	Text(TEXT("MISIÓN FALLIDA"), C, 34.f * S(), WithAlpha(Alert, Fade), true, 0.5f);
	Text(Director->GetFailReason(), C + FVector2D(0.f, 52.f * S()), 18.f * S(), WithAlpha(Ink, Fade * 0.9f), false, 0.5f);
	Text(TEXT("Repitiendo desde la última fase..."), C + FVector2D(0.f, 90.f * S()), 12.f * S(), WithAlpha(Ink, Fade * 0.55f), true, 0.5f);
}

void ABLHUD::DrawMissionComplete(const ABLMissionDirector* Director)
{
	const float Age = Director->GetCompleteAge();
	const float Fade = FMath::Clamp(Age / 1.2f, 0.f, 1.f);
	Rect(FVector2D::ZeroVector, FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(0.01f, 0.012f, 0.015f, 0.78f * Fade));
	if (Fade < 0.3f)
	{
		return;
	}
	const float A = FMath::Clamp((Age - 0.4f) / 0.8f, 0.f, 1.f);
	const float X = Canvas->ClipX * 0.5f - 300.f * S();
	float Y = Canvas->ClipY * 0.3f;
	Rect(FVector2D(X - 24.f * S(), Y - 6.f * S()), FVector2D(3.f * S(), 330.f * S()), WithAlpha(Amber, A));
	Text(TEXT("OPERACIÓN BLACKLINE  ·  KESSRA, 2031"), FVector2D(X, Y), 13.f * S(), WithAlpha(Amber, A), true);
	Y += 26.f * S();
	Text(Director->bCampaignFinale ? TEXT("FIN DE LA CAMPAÑA") : TEXT("MISIÓN COMPLETADA"), FVector2D(X, Y), 38.f * S(), WithAlpha(Ink, A), true);
	Y += 58.f * S();
	Text(Director->MissionName, FVector2D(X, Y), 20.f * S(), WithAlpha(Amber, A), false);
	Y += 46.f * S();
	const int32 Secs = FMath::RoundToInt(Director->GetMissionTime());
	const int32 Shots = Director->GetShots();
	const FString Acc = Shots > 0 ? FString::Printf(TEXT("%d %%"), FMath::RoundToInt(100.f * Director->GetHits() / Shots)) : TEXT("-");
	const TPair<FString, FString> Rows[] = {
		{ TEXT("TIEMPO"), FString::Printf(TEXT("%02d:%02d"), Secs / 60, Secs % 60) },
		{ TEXT("BAJAS"), FString::FromInt(Director->GetKills()) },
		{ TEXT("PRECISIÓN"), Acc },
		{ TEXT("A LA CABEZA"), FString::FromInt(Director->GetHeadshots()) },
		{ TEXT("CAÍDAS"), FString::FromInt(Director->GetDeaths()) },
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Rows); ++i)
	{
		const float RowA = FMath::Clamp((Age - 0.8f - i * 0.12f) / 0.3f, 0.f, 1.f);
		Text(Rows[i].Key, FVector2D(X, Y), 15.f * S(), WithAlpha(FLinearColor(0.6f, 0.62f, 0.6f, 1.f), RowA), true);
		Text(Rows[i].Value, FVector2D(X + 600.f * S(), Y - 2.f * S()), 18.f * S(), WithAlpha(Ink, RowA), true, 1.f);
		Rect(FVector2D(X, Y + 26.f * S()), FVector2D(600.f * S(), 1.f), WithAlpha(FLinearColor(1, 1, 1, 0.12f), RowA));
		Y += 36.f * S();
	}
	if (Age > 2.f)
	{
		const float Blink = 0.6f + 0.4f * FMath::Sin(Age * 3.f);
		const int32 Next = Director->GetNextMissionIndex();
		const FString Prompt = Next != INDEX_NONE
			? FString::Printf(TEXT("[F]  SIGUIENTE MISIÓN · %s %s"), BLMenuData::Missions[Next].Code, BLMenuData::Missions[Next].Name)
			: FString(TEXT("[F]  VOLVER AL MENÚ"));
		Text(Prompt, FVector2D(X, Y + 20.f * S()), 15.f * S(), WithAlpha(Amber, Blink), true);
		Text(TEXT("[ESC]  REINICIAR O SALIR"), FVector2D(X, Y + 48.f * S()), 12.f * S(), WithAlpha(FLinearColor(0.6f, 0.62f, 0.6f, 1.f), Blink), true);
	}
}

// ---------------------------------------------------------------------------

void ABLHUD::DrawHUD()
{
	Super::DrawHUD();
	const ABLCharacter* Char = Cast<ABLCharacter>(GetOwningPawn());
	if (!Char)
	{
		return;
	}
	const ABLMissionDirector* Director = ABLMissionDirector::Get(this);
	if (Director && Director->IsMissionComplete())
	{
		DrawSubtitle(Director);
		DrawMissionComplete(Director);
		return;
	}
	if (Director && Director->IsMissionFailed())
	{
		DrawMissionFailed(Director);
		return;
	}
	DrawCheckpointNotice();
	DrawPhotoFlash();
	DrawAlarm(Director);
	DrawSubtitle(Director);
	if (Char->IsDead())
	{
		DrawDeath(Char);
		return;
	}
	const UBLWeaponComponent* W = Char->GetWeapon();
	DrawObjective(Director);
	DrawMarker(Char, Director);
	DrawDamageVignette(Char);
	DrawDamageIndicators(Char);
	DrawHealth(Char);
	if (Char->IsDrivingBoat())
	{
		DrawBoat(Char);
		return;
	}
	if (Char->IsMounted())
	{
		DrawMountedGun(Char);
		DrawHitMarker(Char);
		return;
	}
	DrawCrosshair(Char, (1.f - Char->GetAimAlpha()) * (1.f - Char->GetSprintAlpha()) * (W && W->IsReloading() ? 0.4f : 1.f));
	DrawHitMarker(Char);
	DrawAmmo(Char);
	DrawInteraction(Char);
}
