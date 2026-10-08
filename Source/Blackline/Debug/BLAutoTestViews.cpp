// Prueba automática "Views" (Bloque 8): vistas fijas de la misión 1 para revisar el arte y medir fps (rápida, sin recorrido).
// Mapa /Game/Maps/M01/L_M01_AmanecerRoto. Capturas: Saved/BLTest/Views_<Nombre>.png
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLEnemyCharacter.h"
#include "Combat/BLHealthComponent.h"
#include "Player/BLCharacter.h"

#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildViewsTest()
{
	Char()->GetHealth()->bInvulnerable = true;
	// Sin enemigos: que no disparen ni alteren las vistas
	// salvo uno, sin IA, para el retrato del miliciano (equipo, uniforme, arma)
	for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(FName("BLEnemy_Control_Caseta")))
		{
			if (AController* C = It->GetController()) { C->UnPossess(); }
			It->SetActorLocationAndRotation(FVector(9700.f, -2900.f, 96.f), FRotator(0.f, 200.f, 0.f));
			continue;
		}
		It->Destroy();
	}
	struct FView { const TCHAR* Name; FVector Loc; float Yaw; float Pitch; };
	const FView Views[] = {
		{ TEXT("Insercion"), FVector(600.f, -150.f, 0.f), 0.f, 2.f },          // calle trasera junto al furgón
		{ TEXT("Callejon"), FVector(3050.f, -500.f, 0.f), -90.f, 4.f },        // callejón A hacia el patio
		{ TEXT("Patio"), FVector(3050.f, -2300.f, 0.f), -60.f, -2.f },         // contenedores
		{ TEXT("Control"), FVector(9600.f, -4000.f, 0.f), 90.f, -1.f },        // control de carretera
		{ TEXT("Calle"), FVector(10800.f, 600.f, 0.f), 0.f, 0.f },             // calle principal
		{ TEXT("Fachadas"), FVector(14000.f, 300.f, 0.f), 35.f, 22.f },        // mirando arriba a las fachadas
		{ TEXT("Local"), FVector(19700.f, 600.f, 0.f), 0.f, -8.f },            // interior del objetivo
		{ TEXT("Miliciano"), FVector(9470.f, -2985.f, 0.f), 20.f, -6.f },      // retrato de un enemigo a 2,5 m
		{ TEXT("Fuego"), FVector(14250.f, 650.f, 0.f), 25.f, -4.f },           // coche ardiendo
	};
	for (const FView& V : Views)
	{
		TSharedRef<FVector2D> Acc = MakeShared<FVector2D>(0.f, 0.f);
		Steps.Add({ V.Name, 3.0f,
			[this, V, Acc]()
			{
				*Acc = FVector2D::ZeroVector;
				ABLCharacter* C = Char();
				C->GetCharacterMovement()->StopMovementImmediately();
				C->SetActorLocation(V.Loc + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
				if (AController* PC = C->GetController()) { PC->SetControlRotation(FRotator(V.Pitch, V.Yaw, 0.f)); }
			},
			[this, Acc, V](float T)
			{
				// Perfil de GPU de una vista (al log): -BLProfileView=<Nombre>
				FString ProfileView;
				if (FParse::Value(FCommandLine::Get(), TEXT("BLProfileView="), ProfileView) && ProfileView == V.Name && T > 2.0f && T - GetWorld()->GetDeltaSeconds() <= 2.0f)
				{
					GEngine->Exec(GetWorld(), TEXT("ProfileGPU"));
				}
				if (T > 1.0f)
				{
					Acc->X += GetWorld()->GetDeltaSeconds();
					Acc->Y += 1.f;
				}
			},
			[Acc](FString& D)
			{
				const float Ms = Acc->Y > 0.f ? Acc->X / Acc->Y * 1000.f : 999.f;
				D = FString::Printf(TEXT("%.2f ms (%.0f fps)"), Ms, 1000.f / Ms);
				return Ms < 1000.f / 45.f;
			}, nullptr, 2.6f });
	}
}
