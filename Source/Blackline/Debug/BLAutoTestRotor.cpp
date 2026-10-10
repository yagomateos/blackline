// Prueba visual "Rotor" (mapa L_Dev_Movement): el helicóptero frente al jugador con el rotor a régimen de vuelo (disco
// de desenfoque) y frenando (palas nítidas), visto de lado y desde abajo. Capturas Rotor_*.png.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Player/BLCharacter.h"
#include "Vehicles/BLHelicopter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBLAutoTestComponent::BuildRotorTest()
{
	TSharedRef<TWeakObjectPtr<ABLHelicopter>> Heli = MakeShared<TWeakObjectPtr<ABLHelicopter>>();
	const FVector HeliAt(3500.f, -2600.f, 0.f);
	auto Shot = [this, Heli, HeliAt](const FString& Name, float Rate, const FVector& Eye, float ZAim)
	{
		Steps.Add({ Name, 2.5f,
			[this, Heli, HeliAt, Rate, Eye, ZAim]()
			{
				if (!Heli->IsValid())
				{
					FActorSpawnParameters P;
					P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					*Heli = GetWorld()->SpawnActor<ABLHelicopter>(ABLHelicopter::StaticClass(), HeliAt + FVector(0.f, 0.f, 600.f), FRotator(0.f, 90.f, 0.f), P);
				}
				(*Heli)->ShowForTest(Rate, false);
				ABLCharacter* C = Char();
				C->GetCharacterMovement()->StopMovementImmediately();
				C->SetActorLocation(Eye + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
			},
			[this, Heli, ZAim, Name](float T)
			{
				if (!Heli->IsValid()) { return; }
				const FVector Target = (*Heli)->GetActorLocation() + FVector(0.f, 0.f, ZAim);
				Char()->GetController()->SetControlRotation((Target - Char()->GetCamera()->GetComponentLocation()).Rotation());
				if (T > 2.0f && !bScreenshotTaken)
				{
					bScreenshotTaken = true;
					Screenshot(Name);
				}
			},
			[Heli, Rate](FString& D)
			{
				const bool bDisc = Heli->IsValid() && (*Heli)->IsRotorDiscVisible();
				D = FString::Printf(TEXT("rotor a %.0f °/s (%.0f rpm), disco de desenfoque visible=%d"), Rate, Rate / 6.f, bDisc);
				return Heli->IsValid() && bDisc == (Rate > 600.f);
			} });
	};
	Shot(TEXT("Vuelo_Lado"), 1800.f, FVector(3500.f, -4700.f, 0.f), 380.f);
	Shot(TEXT("Vuelo_Abajo"), 1800.f, FVector(3700.f, -2900.f, 0.f), 300.f);
	Shot(TEXT("Frenando_Lado"), 300.f, FVector(3500.f, -4700.f, 0.f), 380.f);
	Shot(TEXT("Frenando_Abajo"), 300.f, FVector(3700.f, -2900.f, 0.f), 300.f);
}
