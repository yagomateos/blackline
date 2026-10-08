#include "UI/BLUserSettings.h"

#include "Player/BLCharacter.h"
#include "Player/BLFirstPersonRigComponent.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UBLUserSettings* UBLUserSettings::Get()
{
	return GetMutableDefault<UBLUserSettings>();
}

void UBLUserSettings::Save()
{
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.1f, 4.f);
	FieldOfView = FMath::Clamp(FieldOfView, 70.f, 110.f);
	MotionScale = FMath::Clamp(MotionScale, 0.f, 1.f);
	SaveConfig();
}

void UBLUserSettings::ApplyAudio(const UObject* WorldContext) const
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	if (FAudioDeviceHandle Device = World->GetAudioDevice())
	{
		Device->SetTransientPrimaryVolume(MasterVolume);
	}
	USoundMix* Mix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Audio/Settings/SMix_User.SMix_User"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mix)
	{
		return;
	}
	const TPair<const TCHAR*, float> Classes[] = {
		{ TEXT("SC_Music"), MusicVolume }, { TEXT("SC_SFX"), EffectsVolume }, { TEXT("SC_Voice"), VoiceVolume }, { TEXT("SC_Ambience"), AmbienceVolume } };
	for (const TPair<const TCHAR*, float>& C : Classes)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Settings/%s.%s"), C.Key, C.Key);
		if (USoundClass* SC = LoadObject<USoundClass>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			UGameplayStatics::SetSoundMixClassOverride(World, Mix, SC, C.Value, 1.f, 0.2f, true);
		}
	}
	UGameplayStatics::PushSoundMixModifier(World, Mix);
}

void UBLUserSettings::ApplyToPlayer(APlayerController* PC) const
{
	if (ABLCharacter* C = PC ? Cast<ABLCharacter>(PC->GetPawn()) : nullptr)
	{
		C->SetLookOptions(MouseSensitivity, bInvertY);
		if (UBLFirstPersonRigComponent* Rig = C->GetFirstPersonRig())
		{
			// El FOV de ADS mantiene la misma proporción respecto al de cadera
			const float Ratio = Rig->AimFOV / FMath::Max(Rig->BaseFOV, 1.f);
			Rig->BaseFOV = FieldOfView;
			Rig->AimFOV = FieldOfView * Ratio;
			Rig->CameraMotionScale = MotionScale;
			Rig->WeaponMotionScale = FMath::Lerp(0.5f, 1.f, MotionScale);
		}
	}
}

EBLGraphicsPreset UBLUserSettings::GetGraphicsPreset()
{
	const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S)
	{
		return EBLGraphicsPreset::High;
	}
	const int32 AA = S->GetAntiAliasingQuality(), GI = S->GetGlobalIlluminationQuality(), Sh = S->GetShadowQuality();
	const int32 Tex = S->GetTextureQuality(), PP = S->GetPostProcessingQuality();
	if (AA == 2 && GI == 2 && S->GetReflectionQuality() == 2 && Sh == 3 && Tex == 3 && PP == 3) return EBLGraphicsPreset::High;
	if (AA == 3 && GI == 3 && Sh == 3) return EBLGraphicsPreset::Epic;
	if (AA == 1 && GI == 1 && Sh == 1) return EBLGraphicsPreset::Medium;
	if (AA == 0 && Sh == 0) return EBLGraphicsPreset::Low;
	return EBLGraphicsPreset::Custom;
}

void UBLUserSettings::SetGraphicsPreset(EBLGraphicsPreset Preset)
{
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S || Preset == EBLGraphicsPreset::Custom)
	{
		return;
	}
	switch (Preset)
	{
	case EBLGraphicsPreset::Low:
		S->SetOverallScalabilityLevel(0);
		S->SetGlobalIlluminationQuality(1);   // en 0 no hay Lumen: la escena queda negra en las sombras
		S->SetResolutionScaleValueEx(66.f);
		break;
	case EBLGraphicsPreset::Medium:
		S->SetOverallScalabilityLevel(1);
		S->SetResolutionScaleValueEx(70.f);
		break;
	case EBLGraphicsPreset::High:
		// El preajuste medido para la GTX 1660 Super (ver DefaultGameUserSettings.ini)
		S->SetOverallScalabilityLevel(3);
		S->SetAntiAliasingQuality(2);
		S->SetGlobalIlluminationQuality(2);
		S->SetReflectionQuality(2);
		S->SetResolutionScaleValueEx(75.f);
		break;
	case EBLGraphicsPreset::Epic:
		S->SetOverallScalabilityLevel(3);
		S->SetResolutionScaleValueEx(75.f);
		break;
	default:
		break;
	}
}

float UBLUserSettings::GetResolutionScalePercent()
{
	const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	float Normalized = 0.f, Value = 0.f, Min = 0.f, Max = 0.f;
	if (S)
	{
		S->GetResolutionScaleInformationEx(Normalized, Value, Min, Max);
	}
	return Value >= 25.f ? Value : 75.f;
}

void UBLUserSettings::SetResolutionScalePercent(float Percent)
{
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->SetResolutionScaleValueEx(FMath::Clamp(Percent, 50.f, 100.f));
	}
}

void UBLUserSettings::ApplyAndSaveGraphics()
{
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		// Nunca aplicar una escala inválida (con 0 el juego se renderizaría a la resolución mínima)
		float Normalized = 0.f, Value = 0.f, Min = 0.f, Max = 0.f;
		S->GetResolutionScaleInformationEx(Normalized, Value, Min, Max);
		if (Value < 25.f)
		{
			S->SetResolutionScaleValueEx(75.f);
		}
		S->ApplySettings(false);
		S->SaveSettings();
	}
}
