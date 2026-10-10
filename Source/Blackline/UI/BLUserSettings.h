#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BLUserSettings.generated.h"

class APlayerController;

/** Preajustes de gráficos del menú (Bajo / Medio / Alto / Épico). "Alto" es el que mide 60 fps a 1080p en una GTX 1660 Super. */
UENUM()
enum class EBLGraphicsPreset : uint8
{
	Low, Medium, High, Epic, Custom
};

/**
 * Opciones del jugador que no son de UGameUserSettings (Bloque 10): controles, accesibilidad y volúmenes por tipo
 * de sonido. Se guardan en GameUserSettings.ini (sección /Script/Blackline.BLUserSettings).
 * Los gráficos (calidad, resolución, ventana, VSync) van por UGameUserSettings; aquí solo hay ayudas para aplicarlos.
 */
UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class BLACKLINE_API UBLUserSettings : public UObject
{
	GENERATED_BODY()

public:
	static UBLUserSettings* Get();

	// ---- Controles ----
	UPROPERTY(config) float MouseSensitivity = 1.f;
	UPROPERTY(config) bool bInvertY = false;
	UPROPERTY(config) float FieldOfView = 90.f;
	/** Movimiento de cámara y arma (bob, sway, sacudidas): 1 = completo, 0,25 = mínimo (accesibilidad). */
	UPROPERTY(config) float MotionScale = 1.f;

	// ---- Juego ----
	/** 0 Recluta, 1 Veterano (por defecto), 2 Élite: daño que recibe el jugador. */
	UPROPERTY(config) int32 Difficulty = 1;
	static constexpr int32 NumDifficulties = 3;
	static float PlayerDamageTaken(int32 Level);
	static const TCHAR* DifficultyName(int32 Level);

	// ---- Equipamiento (pantalla EQUIPAMIENTO antes de cada misión) ----
	UPROPERTY(config) FString LoadoutPrimary = TEXT("AR7");
	UPROPERTY(config) FString LoadoutSecondary = TEXT("P17");

	// ---- Audio (0..1) ----
	UPROPERTY(config) float MasterVolume = 1.f;
	UPROPERTY(config) float MusicVolume = 0.8f;
	UPROPERTY(config) float EffectsVolume = 1.f;
	UPROPERTY(config) float VoiceVolume = 1.f;
	UPROPERTY(config) float AmbienceVolume = 0.9f;

	void Save();

	/** Volúmenes: volumen general del dispositivo + SMix_User (clases SC_Music/SFX/Voice/Ambience). */
	void ApplyAudio(const UObject* WorldContext) const;
	/** Sensibilidad, inversión, FOV, movimiento de cámara y dificultad al personaje del jugador (si hay). */
	void ApplyToPlayer(APlayerController* PC) const;

	// ---- Gráficos (UGameUserSettings) ----
	static EBLGraphicsPreset GetGraphicsPreset();
	static void SetGraphicsPreset(EBLGraphicsPreset Preset);
	static void ApplyAndSaveGraphics();
	/** Escala de resolución en % (la de TSR). Corrige valores inválidos (0 = el editor guarda "por defecto"). */
	static float GetResolutionScalePercent();
	static void SetResolutionScalePercent(float Percent);
};
