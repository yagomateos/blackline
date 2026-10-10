#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Player/BLCharacter.h"
#include "Animation/BLFirstPersonAnimInstance.h"
#include "Animation/BLWeaponAnimInstance.h"
#include "AnimationRuntime.h"
#include "Engine/SkeletalMesh.h"
#include "Weapons/BLWeaponComponent.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

namespace
{
	FString TestDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("BLTest"));
	}
}

UBLAutoTestComponent::UBLAutoTestComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

ABLCharacter* UBLAutoTestComponent::Char() const
{
	return Cast<ABLCharacter>(GetOwner());
}

float UBLAutoTestComponent::GroundSpeed() const
{
	return Char() ? Char()->GetCharacterMovement()->Velocity.Size2D() : 0.f;
}

void UBLAutoTestComponent::StartTest(const FString& TestName)
{
	CurrentTest = TestName;
	if (TestName.Equals(TEXT("Movement"), ESearchCase::IgnoreCase))
	{
		BuildMovementTest();
	}
	else if (TestName.Equals(TEXT("Weapons"), ESearchCase::IgnoreCase))
	{
		BuildWeaponsTest();
	}
	else if (TestName.Equals(TEXT("PistolHands"), ESearchCase::IgnoreCase))
	{
		BuildPistolHandsTest();
	}
	else if (TestName.Equals(TEXT("Pistol"), ESearchCase::IgnoreCase))
	{
		BuildPistolTest();
	}
	else if (TestName.Equals(TEXT("Shotgun"), ESearchCase::IgnoreCase))
	{
		BuildShotgunTest();
	}
	else if (TestName.Equals(TEXT("Combat"), ESearchCase::IgnoreCase))
	{
		BuildCombatTest();
	}
	else if (TestName.Equals(TEXT("Level"), ESearchCase::IgnoreCase))
	{
		BuildLevelTest();
	}
	else if (TestName.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
	{
		BuildAITest();
	}
	else if (TestName.Equals(TEXT("Mission"), ESearchCase::IgnoreCase))
	{
		BuildMissionTest();
	}
	else if (TestName.Equals(TEXT("Audio"), ESearchCase::IgnoreCase))
	{
		BuildAudioTest();
	}
	else if (TestName.Equals(TEXT("Views"), ESearchCase::IgnoreCase))
	{
		BuildViewsTest();
	}
	else if (TestName.Equals(TEXT("Grenade"), ESearchCase::IgnoreCase))
	{
		BuildGrenadeTest();
	}
	else if (TestName.Equals(TEXT("Mission5"), ESearchCase::IgnoreCase))
	{
		BuildMission5Test();
	}
	else if (TestName.Equals(TEXT("Mission5Fallo"), ESearchCase::IgnoreCase))
	{
		BuildMission5FailTest();
	}
	else if (TestName.Equals(TEXT("Mission4"), ESearchCase::IgnoreCase))
	{
		BuildMission4Test();
	}
	else if (TestName.Equals(TEXT("MountedGun"), ESearchCase::IgnoreCase))
	{
		BuildMountedGunTest();
	}
	else if (TestName.Equals(TEXT("Distancias"), ESearchCase::IgnoreCase))
	{
		BuildRangeTest();
	}
	else if (TestName.Equals(TEXT("Aguante"), ESearchCase::IgnoreCase))
	{
		BuildSurvivalTest();
	}
	else if (TestName.Equals(TEXT("Rotor"), ESearchCase::IgnoreCase))
	{
		BuildRotorTest();
	}
	else if (TestName.Equals(TEXT("Mission3"), ESearchCase::IgnoreCase))
	{
		BuildMission3Test();
	}
	else if (TestName.Equals(TEXT("Mission2"), ESearchCase::IgnoreCase))
	{
		BuildMission2Test();
	}
	else if (TestName.Equals(TEXT("GrenadeAI"), ESearchCase::IgnoreCase))
	{
		BuildGrenadeAITest();
	}
	else if (TestName.Equals(TEXT("Disparo"), ESearchCase::IgnoreCase))
	{
		BuildFiringTest();
	}
	else
	{
		UE_LOG(LogBlackline, Error, TEXT("[BLTest] Prueba desconocida: %s"), *TestName);
		Finish();
		return;
	}
	if (AController* PC = Char() ? Char()->GetController() : nullptr)
	{
		PC->SetIgnoreLookInput(true); // el ratón del usuario no debe alterar la prueba
	}
	UE_LOG(LogBlackline, Display, TEXT("[BLTest] Inicio de '%s' (%d pasos). Resultados en %s"), *TestName, Steps.Num(), *TestDir());
	BeginStep(0);
}

bool UBLAutoTestComponent::TeleportToStart(const FString& StartName)
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(*(TEXT("BLTest_Start_") + StartName)), Found);
	ABLCharacter* C = Char();
	if (Found.Num() == 0 || !C)
	{
		return false;
	}
	const float HalfHeight = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	C->GetCharacterMovement()->StopMovementImmediately();
	C->SetActorLocation(Found[0]->GetActorLocation() + FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* PC = C->GetController())
	{
		PC->SetControlRotation(Found[0]->GetActorRotation());
	}
	return true;
}

void UBLAutoTestComponent::Screenshot(const FString& Name)
{
	const FString Path = TestDir() / FString::Printf(TEXT("%s_%s.png"), *CurrentTest, *Name);
	FScreenshotRequest::RequestScreenshot(Path, false, false);
	LastScreenshotFrame = GFrameCounter;
	UE_LOG(LogBlackline, Display, TEXT("[BLTest] Captura: %s"), *Path);
}

void UBLAutoTestComponent::BeginStep(int32 Index)
{
	StepIndex = Index;
	StepElapsed = 0.f;
	bScreenshotTaken = false;
	if (!Steps.IsValidIndex(Index))
	{
		Finish();
		return;
	}
	UE_LOG(LogBlackline, Display, TEXT("[BLTest] -> %s"), *Steps[Index].Name);
	if (Steps[Index].Begin)
	{
		Steps[Index].Begin();
	}
}

void UBLAutoTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFinished)
	{
		QuitTimer -= DeltaTime;
		if (QuitTimer <= 0.f)
		{
			FPlatformMisc::RequestExit(false, TEXT("BLAutoTest"));
		}
		return;
	}
	if (!Steps.IsValidIndex(StepIndex))
	{
		return;
	}

	// Rendimiento (se ignoran los primeros frames de carga)
	const bool bCaptureFrame = LastScreenshotFrame > 0 && GFrameCounter - LastScreenshotFrame <= 3;
	if (StepIndex > 0 && !bCaptureFrame)
	{
		FrameTimeAccum += DeltaTime;
		++FrameCount;
		WorstFrame = FMath::Max(WorstFrame, DeltaTime);
		if (DeltaTime > 0.1f && Spikes.Num() < 12)
		{
			Spikes.Add(FString::Printf(TEXT("%.0f ms en %s (+%.1f s)"), DeltaTime * 1000.f, *Steps[StepIndex].Name, StepElapsed));
			UE_LOG(LogBlackline, Display, TEXT("[BLTest] Pico: %s"), *Spikes.Last());
		}
	}

	FStep& Step = Steps[StepIndex];
	StepElapsed += DeltaTime;
	if (Step.Tick)
	{
		Step.Tick(StepElapsed);
	}
	if (Step.ScreenshotAt >= 0.f && !bScreenshotTaken && StepElapsed >= Step.ScreenshotAt)
	{
		bScreenshotTaken = true;
		Screenshot(Step.Name);
	}

	if (StepElapsed >= Step.Duration || (Step.Done && Step.Done()))
	{
		FString Detail;
		const bool bOk = Step.Verify ? Step.Verify(Detail) : true;
		const FString Line = FString::Printf(TEXT("%s  %-16s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *Step.Name, *Detail);
		Results.Add(Line);
		bOk ? ++Passed : ++Failed;
		UE_LOG(LogBlackline, Display, TEXT("[BLTest] %s"), *Line);
		if (Step.End)
		{
			Step.End();
		}
		BeginStep(StepIndex + 1);
	}
}

void UBLAutoTestComponent::Finish()
{
	bFinished = true;
	QuitTimer = 1.5f; // da tiempo a escribir la última captura

	const float AvgMs = FrameCount > 0 ? float(FrameTimeAccum / FrameCount) * 1000.f : 0.f;
	Results.Add(FString::Printf(TEXT("Rendimiento: %.2f ms medio (%.0f fps), peor frame %.1f ms, %d frames"),
		AvgMs, AvgMs > 0.f ? 1000.f / AvgMs : 0.f, WorstFrame * 1000.f, FrameCount));
	if (Spikes.Num() > 0)
	{
		Results.Add(TEXT("Picos (> 100 ms): ") + FString::Join(Spikes, TEXT("; ")));
	}
	Results.Add(FString::Printf(TEXT("RESUMEN %s: %d PASS, %d FAIL"), *CurrentTest, Passed, Failed));

	UE_LOG(LogBlackline, Display, TEXT("[BLTest] %s"), *Results[Results.Num() - 2]);
	UE_LOG(LogBlackline, Display, TEXT("[BLTest] %s"), *Results.Last());
	FFileHelper::SaveStringArrayToFile(Results, *(TestDir() / CurrentTest + TEXT("_results.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	// -BLPressContinue: tras una prueba de misión, pulsa F en el resumen (pasa a la siguiente misión; el log dice qué mapa carga)
	if (FParse::Param(FCommandLine::Get(), TEXT("BLPressContinue")) && Char())
	{
		UE_LOG(LogBlackline, Display, TEXT("[BLTest] Pulsando continuar en el resumen"));
		Char()->SetInteractHeld(true);
	}
}

// ---------------------------------------------------------------------------
// Prueba de movimiento
// ---------------------------------------------------------------------------

void UBLAutoTestComponent::BuildMovementTest()
{
	auto Near = [](float Value, float Expected, float Tol) { return FMath::Abs(Value - Expected) <= Tol; };

	Steps.Add({ TEXT("Asentar"), 2.0f, nullptr, nullptr,
		[this](FString& D) { D = TEXT("en el suelo"); return Char()->GetCharacterMovement()->IsMovingOnGround(); },
		nullptr, 1.8f });

	Steps.Add({ TEXT("Andar"), 2.0f, nullptr,
		[this](float) { Char()->DoMove(0.f, 1.f); },
		[this, Near](FString& D) { const float S = GroundSpeed(); D = FString::Printf(TEXT("velocidad %.0f (esperado 420)"), S); return Near(S, 420.f, 30.f); },
		nullptr, 1.5f });

	Steps.Add({ TEXT("Sprint"), 2.0f,
		[this]() { Char()->SetSprintHeld(true); },
		[this](float) { Char()->DoMove(0.f, 1.f); },
		[this, Near](FString& D) { const float S = GroundSpeed(); D = FString::Printf(TEXT("velocidad %.0f (esperado 660), sprint=%d"), S, Char()->IsSprinting()); return Char()->IsSprinting() && Near(S, 660.f, 40.f); },
		[this]() { Char()->SetSprintHeld(false); }, 1.6f });

	Steps.Add({ TEXT("Frenar"), 1.0f, nullptr, nullptr,
		[this](FString& D) { const float S = GroundSpeed(); D = FString::Printf(TEXT("velocidad %.0f"), S); return S < 10.f; } });

	Steps.Add({ TEXT("Agacharse"), 1.0f,
		[this]() { Char()->ToggleCrouch(); }, nullptr,
		[this](FString& D) { const float E = Char()->GetEyeHeightFromFloor(); D = FString::Printf(TEXT("agachado=%d ojos %.0f cm"), Char()->bIsCrouched, E); return Char()->bIsCrouched && E < 110.f; } });

	Steps.Add({ TEXT("AndarAgachado"), 1.5f, nullptr,
		[this](float) { Char()->DoMove(0.f, 1.f); },
		[this, Near](FString& D) { const float S = GroundSpeed(); D = FString::Printf(TEXT("velocidad %.0f (esperado 210)"), S); return Near(S, 210.f, 25.f); },
		nullptr, 1.2f });

	Steps.Add({ TEXT("Levantarse"), 1.0f,
		[this]() { Char()->ToggleCrouch(); }, nullptr,
		[this](FString& D) { const float E = Char()->GetEyeHeightFromFloor(); D = FString::Printf(TEXT("agachado=%d ojos %.0f cm"), Char()->bIsCrouched, E); return !Char()->bIsCrouched && E > 158.f; } });

	Steps.Add({ TEXT("ADS"), 1.0f,
		[this]() { Char()->SetAimHeld(true); }, nullptr,
		[this, Near](FString& D) { const float F = Char()->GetCamera()->FieldOfView; D = FString::Printf(TEXT("aim %.2f FOV %.1f"), Char()->GetAimAlpha(), F); return Char()->GetAimAlpha() > 0.98f && Near(F, 68.f, 1.f); },
		[this]() { Char()->SetAimHeld(false); }, 0.8f });

	Steps.Add({ TEXT("SoltarADS"), 0.7f, nullptr, nullptr,
		[this](FString& D) { D = FString::Printf(TEXT("aim %.2f"), Char()->GetAimAlpha()); return Char()->GetAimAlpha() < 0.05f; },
		nullptr, 0.0f });

	Steps.Add({ TEXT("LeanIzq"), 0.9f,
		[this]() { Char()->SetLeanLeftHeld(true); }, nullptr,
		[this](FString& D) { D = FString::Printf(TEXT("lean %.2f"), Char()->GetLeanAlpha()); return Char()->GetLeanAlpha() < -0.9f; },
		[this]() { Char()->SetLeanLeftHeld(false); }, 0.7f });

	Steps.Add({ TEXT("LeanDer"), 1.2f,
		[this]() { Char()->SetLeanRightHeld(true); }, nullptr,
		[this](FString& D) { D = FString::Printf(TEXT("lean %.2f"), Char()->GetLeanAlpha()); return Char()->GetLeanAlpha() > 0.9f; },
		[this]() { Char()->SetLeanRightHeld(false); } });

	Steps.Add({ TEXT("Salto"), 1.4f,
		[this]() { StartZ = MaxZ = Char()->GetActorLocation().Z; Char()->DoJumpOrMantle(); },
		[this](float) { MaxZ = FMath::Max(MaxZ, Char()->GetActorLocation().Z); },
		[this](FString& D) { const float H = MaxZ - StartZ; D = FString::Printf(TEXT("altura %.0f cm (esperado 60-90), en suelo=%d"), H, Char()->GetCharacterMovement()->IsMovingOnGround()); return H > 60.f && H < 90.f && Char()->GetCharacterMovement()->IsMovingOnGround(); },
		[this]() { Char()->DoStopJump(); } });

	// Mantle: se sitúa delante del obstáculo, espera, pulsa saltar y comprueba la subida
	auto AddMantleStep = [this](const FString& Name, float ExpectedRise, bool bExpectMantle, float Tol)
	{
		Steps.Add({ Name, 1.8f,
			[this, Name]()
			{
				bSawMantle = false;
				bJumpPressed = false;
				if (!TeleportToStart(Name))
				{
					UE_LOG(LogBlackline, Warning, TEXT("[BLTest] No existe BLTest_Start_%s en el mapa"), *Name);
				}
			},
			[this](float T)
			{
				if (!bJumpPressed && T > 0.4f)
				{
					bJumpPressed = true;
					StartZ = Char()->GetActorLocation().Z;
					Char()->DoJumpOrMantle();
				}
				bSawMantle |= Char()->IsMantling();
			},
			[this, ExpectedRise, bExpectMantle, Tol](FString& D)
			{
				const float Rise = Char()->GetActorLocation().Z - StartZ;
				const bool bGround = Char()->GetCharacterMovement()->IsMovingOnGround();
				D = FString::Printf(TEXT("mantle=%d subida %.0f cm (esperado %s%.0f), en suelo=%d"),
					bSawMantle, Rise, bExpectMantle ? TEXT("") : TEXT("sin mantle, "), ExpectedRise, bGround);
				return bSawMantle == bExpectMantle && FMath::Abs(Rise - ExpectedRise) <= Tol && bGround;
			},
			[this]() { Char()->DoStopJump(); }, 0.75f });
	};
	AddMantleStep(TEXT("Mantle100"), 100.f, true, 12.f);
	AddMantleStep(TEXT("Mantle150"), 150.f, true, 12.f);
	AddMantleStep(TEXT("Muro250"), 0.f, false, 5.f);

	Steps.Add({ TEXT("Escaleras"), 3.5f,
		[this]() { TeleportToStart(TEXT("Escaleras")); StartZ = Char()->GetActorLocation().Z; },
		[this](float T) { if (T > 0.3f) { Char()->DoMove(0.f, 1.f); } },
		[this](FString& D) { const float Rise = Char()->GetActorLocation().Z - StartZ; D = FString::Printf(TEXT("subida %.0f cm (esperado 120)"), Rise); return FMath::Abs(Rise - 120.f) < 15.f; },
		nullptr, 2.0f });

	Steps.Add({ TEXT("LeanPared"), 1.2f,
		[this]() { TeleportToStart(TEXT("LeanPared")); Char()->SetLeanRightHeld(true); }, nullptr,
		[this](FString& D) { const float L = Char()->GetLeanAlpha(); D = FString::Printf(TEXT("lean %.2f (limitado por la pared, esperado < 0.7)"), L); return L > 0.05f && L < 0.7f; },
		[this]() { Char()->SetLeanRightHeld(false); }, 1.0f });
}

// ---------------------------------------------------------------------------
// Prueba de armas (mapa L_Dev_Movement, estación "Tiro")
// ---------------------------------------------------------------------------

void UBLAutoTestComponent::HandleShot(const FHitResult& Hit)
{
	HitsCount += Hit.bBlockingHit ? 1 : 0;
	if (!Hit.bBlockingHit)
	{
		const FRotator Dir = (Hit.TraceEnd - Hit.TraceStart).Rotation();
		MissInfo += FString::Printf(TEXT(" [fallo p%.1f y%.1f desde %s]"), Dir.Pitch, Dir.Yaw, *Hit.TraceStart.ToCompactString());
	}
	if (Hit.bBlockingHit)
	{
		LastSurface = int32(UBLSurfaceEffectsData::ResolveSurface(Hit));
		LastHitInfo = FString::Printf(TEXT("%s/%s"), *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.PhysMaterial.Get()));
	}
	// Captura en el instante de un disparo (con el fogonazo visible)
	if (!PendingShotScreenshot.IsEmpty() && Char()->GetWeapon()->GetShotsFired() - ShotsAtStart == ShotScreenshotAt)
	{
		Screenshot(PendingShotScreenshot);
		PendingShotScreenshot.Empty();
	}
}

void UBLAutoTestComponent::BuildWeaponsTest()
{
	UBLWeaponComponent* W = Char()->GetWeapon();
	W->OnShot.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleShot);

	auto Pitch = [this]() { return Char()->GetController() ? Char()->GetController()->GetControlRotation().GetNormalized().Pitch : 0.f; };
	auto BeginShots = [this, W, Pitch]()
	{
		ShotsAtStart = W->GetShotsFired();
		HitsCount = 0;
		MissInfo.Empty();
		MagAtStart = W->GetMagazine();
		ReserveAtStart = W->GetReserve();
		PitchAtStart = PeakPitch = Pitch();
	};

	Steps.Add({ TEXT("Equipar"), 1.5f,
		[this]() { TeleportToStart(TEXT("Tiro")); },
		nullptr,
		[this, W](FString& D)
		{
			const UBLWeaponData* Data = W->GetWeaponData();
			D = FString::Printf(TEXT("arma=%s cargador %d reserva %d equipando=%d pos=%s rot=%s"), Data ? *Data->GetName() : TEXT("ninguna"), W->GetMagazine(), W->GetReserve(), W->IsEquipping(),
				*Char()->GetActorLocation().ToCompactString(), *Char()->GetControlRotation().ToCompactString());
			return Data && W->GetMagazine() == Data->MagazineSize && !W->IsEquipping();
		},
		nullptr, 1.4f });

	Steps.Add({ TEXT("ManoIzq"), 0.3f, nullptr, nullptr,
		[this, W](FString& D)
		{
			const FVector Hand = Char()->GetFirstPersonMesh()->GetSocketLocation(TEXT("HandGrip_L"));
			const FVector Grip = Char()->GetWeaponMesh()->GetSocketLocation(W->GetWeaponData()->LeftHandSocket);
			const float Dist = FVector::Dist(Hand, Grip);
			const FVector RefGrip = Char()->GetFirstPersonAnim() ? Char()->GetFirstPersonAnim()->GetState().LeftGripInWeapon.GetLocation() : FVector::ZeroVector;
			const FVector LiveGrip = Char()->GetWeaponMesh()->GetSocketTransform(W->GetWeaponData()->LeftHandSocket, RTS_Component).GetLocation();
			D = FString::Printf(TEXT("mano izquierda a %.1f cm del agarre (esperado < 3) [agarre ref %s, real %s, ik %.2f arma %d, mano CS %s, agarre CS %s]"), Dist, *RefGrip.ToCompactString(), *LiveGrip.ToCompactString(),
				Char()->GetFirstPersonAnim()->GetState().LeftHandIKAlpha, Char()->GetFirstPersonAnim()->GetState().bHasWeapon,
				*Char()->GetFirstPersonMesh()->GetComponentTransform().InverseTransformPosition(Hand).ToCompactString(),
				*Char()->GetFirstPersonMesh()->GetComponentTransform().InverseTransformPosition(Grip).ToCompactString());
			return Dist < 3.f;
		} });

	Steps.Add({ TEXT("Rafaga"), 1.0f,
		[this, BeginShots]() { BeginShots(); PendingShotScreenshot = TEXT("Disparo"); Char()->SetFireHeld(true); },
		[this, Pitch](float) { PeakPitch = FMath::Max(PeakPitch, Pitch()); },
		[this, W](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - ShotsAtStart;
			const float Rise = PeakPitch - PitchAtStart;
			D = FString::Printf(TEXT("%d disparos en 1 s (esperado 12-14 a 750 RPM), impactos %d, cargador %d->%d, retroceso +%.1f grados%s"),
				Shots, HitsCount, MagAtStart, W->GetMagazine(), Rise, *MissInfo);
			return Shots >= 12 && Shots <= 14 && HitsCount == Shots && W->GetMagazine() == MagAtStart - Shots && Rise > 2.f && Rise < 12.f;
		},
		[this]() { Char()->SetFireHeld(false); } });

	Steps.Add({ TEXT("Recuperacion"), 1.0f, nullptr, nullptr,
		[this, Pitch](FString& D)
		{
			const float Now = Pitch() - PitchAtStart;
			const float Peak = PeakPitch - PitchAtStart;
			D = FString::Printf(TEXT("mira +%.1f -> +%.1f grados (recupera parte, sin pasarse)"), Peak, Now);
			return Now < Peak - 1.f && Now > -0.5f;
		},
		nullptr, 0.9f });

	Steps.Add({ TEXT("Recarga"), 2.6f,
		[this, W]() { MagAtStart = W->GetMagazine(); ReserveAtStart = W->GetReserve(); bJumpPressed = false; bSawMantle = false; Char()->DoReload(); },
		[this](float T)
		{
			// Secuencia de la recarga (2,0 s): 0,4 s tira del cargador, 1,0 s paso, 1,2 s lo mete
			if (T > 0.4f && !bJumpPressed) { bJumpPressed = true; Screenshot(TEXT("Recarga_04")); }
			if (T > 1.2f && !bSawMantle) { bSawMantle = true; Screenshot(TEXT("Recarga_12")); }
		},
		[W](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d (30+1 en recamara) reserva %d recargando=%d"), W->GetMagazine(), W->GetReserve(), W->IsReloading());
			return W->GetMagazine() == W->GetWeaponData()->MagazineSize + 1 && !W->IsReloading();
		},
		nullptr, 1.0f });

	Steps.Add({ TEXT("SprintCorta"), 1.6f,
		[this, W]() { TeleportToStart(TEXT("Tiro")); ShotsAtStart = W->GetShotsFired(); bJumpPressed = false; Char()->SetSprintHeld(true); },
		[this](float T)
		{
			if (T < 0.8f) { Char()->DoMove(0.f, 1.f); }
			else if (!bJumpPressed) { bJumpPressed = true; Char()->SetFireHeld(true); }
			if (T > 1.4f) { Char()->SetFireHeld(false); }
		},
		[this, W](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - ShotsAtStart;
			D = FString::Printf(TEXT("sprint=%d disparos tras soltar sprint %d"), Char()->IsSprinting(), Shots);
			return !Char()->IsSprinting() && Shots >= 3;
		},
		[this]() { Char()->SetFireHeld(false); Char()->SetSprintHeld(false); } });

	// Cerrojo del arma (UBLWeaponAnimInstance): abierto con el cargador vacío, cerrado tras la recarga en vacío
	auto BoltAlpha = [this]()
	{
		const UBLWeaponAnimInstance* WA = Cast<UBLWeaponAnimInstance>(Char()->GetWeaponMesh()->GetAnimInstance());
		return WA ? WA->GetBoltAlpha() : -1.f;
	};
	TSharedRef<float> BoltOpenSeen = MakeShared<float>(0.f);

	Steps.Add({ TEXT("VaciarCargador"), 3.6f,
		[this, BoltOpenSeen]() { *BoltOpenSeen = 0.f; TeleportToStart(TEXT("Tiro")); Char()->SetFireHeld(true); },
		[this, W, BoltAlpha, BoltOpenSeen](float T)
		{
			if (W->GetMagazine() == 0 && T < 3.0f) { Char()->SetFireHeld(false); }
			if (W->GetMagazine() == 0 && T > 2.0f && T < 3.0f)
			{
				// Desplazamiento real del hueso respecto a la pose de referencia (cm)
				const USkeletalMeshComponent* WM = Char()->GetWeaponMesh();
				const FName Bolt = W->GetWeaponData()->BoltBone;
				const int32 Bi = WM->GetBoneIndex(Bolt);
				const float Moved = Bi == INDEX_NONE ? 0.f : FVector::Dist(WM->GetBoneTransform(Bi, FTransform::Identity).GetLocation(),
					FAnimationRuntime::GetComponentSpaceTransformRefPose(WM->GetSkeletalMeshAsset()->GetRefSkeleton(), Bi).GetLocation());
				*BoltOpenSeen = Moved >= W->GetWeaponData()->BoltTravel * 0.9f ? BoltAlpha() : 0.f;
			}
			if (T >= 3.0f && !W->IsReloading()) { Char()->SetFireHeld(true); } // gatillo en vacío -> recarga automática
		},
		[W, BoltOpenSeen](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d, recarga automatica=%d, cerrojo abierto %.2f"), W->GetMagazine(), W->IsReloading(), *BoltOpenSeen);
			return W->IsReloading() && *BoltOpenSeen > 0.99f;
		},
		[this]() { Char()->SetFireHeld(false); }, 3.4f });

	Steps.Add({ TEXT("RecargaVacio"), 2.6f, nullptr, nullptr,
		[W, BoltAlpha](FString& D)
		{
			D = FString::Printf(TEXT("cargador %d reserva %d cerrojo %.2f"), W->GetMagazine(), W->GetReserve(), BoltAlpha());
			return W->GetMagazine() == W->GetWeaponData()->MagazineSize && !W->IsReloading() && BoltAlpha() < 0.01f;
		},
		nullptr, 1.2f });

	Steps.Add({ TEXT("ADS"), 1.2f,
		[this]() { TeleportToStart(TEXT("Tiro")); Char()->SetAimHeld(true); },
		nullptr,
		[this, W](FString& D)
		{
			D = FString::Printf(TEXT("aim %.2f dispersion %.2f grados (cadera %.2f)"), Char()->GetAimAlpha(), W->GetCurrentSpread(), W->GetWeaponData()->HipSpread);
			return Char()->GetAimAlpha() > 0.98f && W->GetCurrentSpread() < 0.5f;
		},
		nullptr, 1.0f });

	Steps.Add({ TEXT("RafagaADS"), 1.0f,
		[this, BeginShots]() { BeginShots(); PendingShotScreenshot = TEXT("DisparoADS"); Char()->SetFireHeld(true); },
		[this, Pitch](float) { PeakPitch = FMath::Max(PeakPitch, Pitch()); },
		[this, W](FString& D)
		{
			const int32 Shots = W->GetShotsFired() - ShotsAtStart;
			D = FString::Printf(TEXT("%d disparos en ADS, retroceso +%.1f grados, impactos %d (mira al empezar a %.1f°)%s"), Shots, PeakPitch - PitchAtStart, HitsCount, PitchAtStart, *MissInfo);
			return Shots >= 12 && HitsCount == Shots;
		},
		[this]() { Char()->SetFireHeld(false); Char()->SetAimHeld(false); } });

	Steps.Add({ TEXT("Impactos"), 1.0f, nullptr, nullptr,
		[](FString& D) { D = TEXT("captura del muro con impactos"); return true; },
		nullptr, 0.5f });

	// Impactos por material: apuntar a cada diana, ráfaga corta, comprobar superficie y partículas
	AddSurfaceStep(TEXT("Concrete"), int32(BLSurface::Concrete), false);
	AddSurfaceStep(TEXT("Metal"), int32(BLSurface::Metal), true);
	AddSurfaceStep(TEXT("Wood"), int32(BLSurface::Wood), false);
	AddSurfaceStep(TEXT("Glass"), int32(BLSurface::Glass), false);
	AddSurfaceStep(TEXT("Dirt"), int32(BLSurface::Dirt), false);
}

void UBLAutoTestComponent::AddSurfaceStep(const FString& Name, int32 ExpectedSurface, bool bExpectSparks)
{
	Steps.Add({ TEXT("Impacto") + Name, 1.4f,
		[this, Name]()
		{
			TeleportToStart(TEXT("Materiales"));
			LastSurface = -1;
			MaxSprites = MaxSparks = 0;
			TArray<AActor*> Found;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(*(TEXT("BLTarget_") + Name)), Found);
			if (Found.Num() > 0 && Char()->GetController())
			{
				FVector Origin, Extent;
				Found[0]->GetActorBounds(false, Origin, Extent);
				if (Extent.Z > 60.f)  // los blancos bajos (montículo de tierra) se apuntan al centro para no dar en el suelo
				{
					Origin.Z = FMath::Min(Origin.Z, Found[0]->GetActorLocation().Z + Extent.Z * 0.6f);
				}
				const FVector Eye = Char()->GetCamera()->GetComponentLocation();
				Char()->GetController()->SetControlRotation((Origin - Eye).Rotation());
			}
		},
		[this](float T)
		{
			Char()->SetFireHeld(T > 0.15f && T < 0.38f);
			if (const UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>())
			{
				MaxSprites = FMath::Max(MaxSprites, FX->GetActiveSprites());
				MaxSparks = FMath::Max(MaxSparks, FX->GetActiveSparks());
			}
		},
		[this, ExpectedSurface, bExpectSparks](FString& D)
		{
			D = FString::Printf(TEXT("[%d %s] superficie %s (esperada %s), sprites %d, chispas %d"), LastSurface, *LastHitInfo,
				UBLSurfaceEffectsData::Name(EPhysicalSurface(FMath::Max(LastSurface, 0))), UBLSurfaceEffectsData::Name(EPhysicalSurface(ExpectedSurface)), MaxSprites, MaxSparks);
			return LastSurface == ExpectedSurface && (MaxSprites > 0 || MaxSparks > 0) && (!bExpectSparks || MaxSparks > 0);
		},
		[this]() { Char()->SetFireHeld(false); }, 0.45f });
}
