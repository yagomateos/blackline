#include "UI/BLHUD.h"

#include "Combat/BLHealthComponent.h"
#include "Mission/BLCheckpointSubsystem.h"
#include "Mission/BLInteractable.h"
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
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float Radius = Canvas->ClipY * 0.17f;
	const FRotator View = Char->GetControlRotation();
	for (const ABLCharacter::FDamageIndicator& I : Char->GetDamageIndicators())
	{
		const FVector Local = FRotator(0.f, View.Yaw, 0.f).UnrotateVector((I.Source - Char->GetActorLocation()).GetSafeNormal2D());
		const float Ang = FMath::Atan2(Local.Y, Local.X);
		const float Age = (Now - I.Time) / 1.8f;
		const FLinearColor Col = WithAlpha(FLinearColor(0.85f, 0.08f, 0.05f, 0.85f), I.Strength * (1.f - FMath::SmoothStep(0.4f, 1.f, Age)));
		const float Half = FMath::DegreesToRadians(18.f);
		for (int32 k = 0; k < 10; ++k)
		{
			const float A0 = Ang - Half + 2.f * Half * k / 10.f, A1 = Ang - Half + 2.f * Half * (k + 1) / 10.f;
			const float Mid = 1.f - FMath::Abs((k + 0.5f) / 10.f * 2.f - 1.f);
			Line(C + FVector2D(FMath::Sin(A0), -FMath::Cos(A0)) * Radius, C + FVector2D(FMath::Sin(A1), -FMath::Cos(A1)) * Radius, Col, (3.f + 5.f * Mid) * S());
		}
	}
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
	if (W->GetShotsFired() != LastShots || W->IsReloading() || W->GetMagazine() != LastMag || Char->IsAiming())
	{
		AmmoAttention = 0.f;
	}
	LastShots = W->GetShotsFired();
	LastMag = W->GetMagazine();
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
	Text(O->Text, P + FVector2D(0.f, 22.f * S()), 19.f * S(), WithAlpha(Ink, Alpha), false);
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

void ABLHUD::DrawDeath(const ABLCharacter* Char)
{
	const float T = Char->GetDeathTime();
	const float Alpha = FMath::Clamp((T - 0.6f) / 0.6f, 0.f, 1.f);
	Text(TEXT("HAS CAÍDO"), FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.44f), 30.f * S(), WithAlpha(Alert, Alpha), true, 0.5f);
	Text(TEXT("Volviendo al último punto de control"), FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.44f + 46.f * S()), 18.f * S(), WithAlpha(Ink, Alpha * 0.85f), false, 0.5f);
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
	Text(TEXT("MISIÓN COMPLETADA"), FVector2D(X, Y), 38.f * S(), WithAlpha(Ink, A), true);
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
		Text(TEXT("[F]  VOLVER A JUGAR"), FVector2D(X, Y + 20.f * S()), 15.f * S(), WithAlpha(Amber, Blink), true);
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
	DrawCheckpointNotice();
	DrawSubtitle(Director);
	if (Char->IsDead())
	{
		DrawDeath(Char);
		return;
	}
	const UBLWeaponComponent* W = Char->GetWeapon();
	DrawObjective(Director);
	DrawMarker(Char, Director);
	DrawDamageIndicators(Char);
	DrawCrosshair(Char, (1.f - Char->GetAimAlpha()) * (1.f - Char->GetSprintAlpha()) * (W && W->IsReloading() ? 0.4f : 1.f));
	DrawHitMarker(Char);
	DrawAmmo(Char);
	DrawInteraction(Char);
}
