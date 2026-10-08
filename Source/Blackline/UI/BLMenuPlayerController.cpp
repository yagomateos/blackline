#include "UI/BLMenuPlayerController.h"

#include "Blackline.h"
#include "UI/BLMenuData.h"
#include "UI/BLUserSettings.h"
#include "UI/SBLMainMenu.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundBase.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SWeakWidget.h"

ABLMenuGameMode::ABLMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ABLMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}

void ABLMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	UBLUserSettings::Get()->ApplyAudio(this);
}

ABLMenuPlayerController::ABLMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	static ConstructorHelpers::FObjectFinder<USoundBase> Theme(TEXT("/Game/Audio/Music/SW_Mus_Menu_Loop.SW_Mus_Menu_Loop"));
	MenuMusic = Theme.Object;
}

void ABLMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalPlayerController() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	SAssignNew(Menu, SBLMainMenu).Owner(this);
	MenuContainer = SNew(SWeakWidget).PossiblyNullContent(Menu.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(MenuContainer.ToSharedRef(), 10);

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;
	Menu->FocusFirst();

	if (MenuMusic)
	{
		Music = UGameplayStatics::SpawnSound2D(this, MenuMusic, 1.f, 1.f, 0.f, nullptr, false, false);
		if (Music)
		{
			Music->FadeIn(2.5f, 1.f);
		}
	}

	FString Test;
	if (FParse::Value(FCommandLine::Get(), TEXT("BLTest="), Test) && Test.Equals(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		BuildTest();
	}
}

void ABLMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (MenuContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuContainer.ToSharedRef());
	}
	Super::EndPlay(Reason);
}

void ABLMenuPlayerController::StartMission(int32 StartPhase)
{
	if (IsLoading())
	{
		return;
	}
	StartPhase = FMath::Clamp(StartPhase, 0, 4);
	// En la partida, la fase llega como opción de la URL (?BLStart=Fase3) y la lee ABLGameMode / ABLMissionDirector
	PendingLevelOptions = FString::Printf(TEXT("BLStart=%s"), BLMenuData::StartPhaseIds[StartPhase]);
	if (TestIndex != INDEX_NONE)
	{
		PendingLevelOptions += TEXT("?BLMenuTest=1");
	}
	LoadTimer = 1.2f;
	if (Music)
	{
		Music->FadeOut(1.f, 0.f);
	}
	UE_LOG(LogBlackline, Log, TEXT("[Menu] Desplegando: %s"), *PendingLevelOptions);
}

void ABLMenuPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ABLMenuPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// El foco inicial no se aplica hasta que el menú se ha pintado al menos una vez
	if (Menu.IsValid() && ++FramesShown == 2)
	{
		Menu->FocusFirst();
	}
	if (LoadTimer >= 0.f)
	{
		LoadTimer -= DeltaTime;
		if (LoadTimer < 0.f)
		{
			LoadTimer = 0.f;   // sigue "cargando" hasta que cambie el nivel
			UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/M01/L_M01_AmanecerRoto")), true, PendingLevelOptions);
		}
	}

	// ---- Prueba automática del menú ----
	if (TestIndex == INDEX_NONE || !TestSteps.IsValidIndex(TestIndex))
	{
		return;
	}
	if (TestTime == 0.f && TestSteps[TestIndex].Begin)
	{
		TestSteps[TestIndex].Begin();
	}
	TestTime += DeltaTime;
	const FMenuStep& S = TestSteps[TestIndex];
	if (TestTime >= S.Duration * 0.8f && TestTime - DeltaTime < S.Duration * 0.8f)
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("BLTest") / FString::Printf(TEXT("Menu_%s.png"), *S.Name);
		FScreenshotRequest::RequestScreenshot(Path, true, false);
	}
	if (TestTime >= S.Duration)
	{
		FString Detail;
		const bool bOk = S.Verify ? S.Verify(Detail) : true;
		(bOk ? TestPassed : TestFailed)++;
		const FString Line = FString::Printf(TEXT("%s  %-14s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *S.Name, *Detail);
		TestResults.Add(Line);
		UE_LOG(LogBlackline, Display, TEXT("[BLTest] %s"), *Line);
		TestTime = 0.f;
		++TestIndex;
		if (!TestSteps.IsValidIndex(TestIndex))
		{
			// El último paso (despliegue) lo verifica ABLGameMode en el nivel de la misión y añade su línea al fichero
			TestResults.Add(FString::Printf(TEXT("RESUMEN Menu (parcial): %d PASS, %d FAIL"), TestPassed, TestFailed));
			FFileHelper::SaveStringArrayToFile(TestResults, *(FPaths::ProjectSavedDir() / TEXT("BLTest/Menu_results.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
			StartMission(1);
		}
	}
}

void ABLMenuPlayerController::BuildTest()
{
	auto PageIs = [this](SBLMainMenu::EPage P, FString& D, const TCHAR* Name)
	{
		D = FString::Printf(TEXT("página %s"), Name);
		return Menu.IsValid() && Menu->GetPage() == P;
	};
	TestSteps.Add({ TEXT("Principal"), 2.5f, nullptr, [this, PageIs](FString& D)
	{
		const bool bOk = PageIs(SBLMainMenu::EPage::Main, D, TEXT("principal"));
		D += FString::Printf(TEXT(", música %s"), MenuMusic ? *MenuMusic->GetName() : TEXT("(falta)"));
		return bOk && MenuMusic != nullptr;
	} });
	TestSteps.Add({ TEXT("Misiones"), 1.5f, [this]() { Menu->ShowPage(SBLMainMenu::EPage::Missions); },
		[PageIs](FString& D) { return PageIs(SBLMainMenu::EPage::Missions, D, TEXT("misiones")); } });
	TestSteps.Add({ TEXT("Armamento"), 1.5f, [this]() { Menu->ShowPage(SBLMainMenu::EPage::Loadout); Menu->SetSelectedWeapon(0); },
		[PageIs](FString& D) { return PageIs(SBLMainMenu::EPage::Loadout, D, TEXT("armamento")); } });
	TestSteps.Add({ TEXT("Inteligencia"), 1.5f, [this]() { Menu->ShowPage(SBLMainMenu::EPage::Intel); Menu->SetSelectedIntel(2); },
		[PageIs](FString& D) { return PageIs(SBLMainMenu::EPage::Intel, D, TEXT("inteligencia")); } });
	TestSteps.Add({ TEXT("Opciones"), 1.5f, [this]()
	{
		Menu->ShowPage(SBLMainMenu::EPage::Options);
		UBLUserSettings* S = UBLUserSettings::Get();
		S->MouseSensitivity = 1.7f;
		S->MusicVolume = 0.55f;
	}, [PageIs](FString& D) { return PageIs(SBLMainMenu::EPage::Options, D, TEXT("opciones")); } });
	// Al volver se guardan las opciones: se comprueba leyendo el .ini y se restauran los valores
	TestSteps.Add({ TEXT("Guardado"), 1.0f, [this]() { Menu->ShowPage(SBLMainMenu::EPage::Main); }, [](FString& D)
	{
		float Sens = 0.f, Music = 0.f;
		GConfig->Flush(true, GGameUserSettingsIni);
		GConfig->LoadFile(GGameUserSettingsIni);
		const bool bSens = GConfig->GetFloat(TEXT("/Script/Blackline.BLUserSettings"), TEXT("MouseSensitivity"), Sens, GGameUserSettingsIni);
		GConfig->GetFloat(TEXT("/Script/Blackline.BLUserSettings"), TEXT("MusicVolume"), Music, GGameUserSettingsIni);
		D = FString::Printf(TEXT("guardado en GameUserSettings.ini: sensibilidad %.1f, música %.2f"), Sens, Music);
		UBLUserSettings* S = UBLUserSettings::Get();
		S->MouseSensitivity = 1.f;
		S->MusicVolume = 0.8f;
		S->Save();
		return bSens && FMath::IsNearlyEqual(Sens, 1.7f, 0.01f) && FMath::IsNearlyEqual(Music, 0.55f, 0.01f);
	} });
	TestSteps.Add({ TEXT("Salir"), 1.0f, [this]() { Menu->ShowPage(SBLMainMenu::EPage::Quit); },
		[PageIs](FString& D) { return PageIs(SBLMainMenu::EPage::Quit, D, TEXT("confirmar salida")); } });
	TestSteps.Add({ TEXT("Volver"), 0.6f, [this]() { Menu->ShowPage(SBLMainMenu::EPage::Main); },
		[PageIs](FString& D) { return PageIs(SBLMainMenu::EPage::Main, D, TEXT("principal")); } });
	TestIndex = 0;
	TestTime = 0.f;
}
