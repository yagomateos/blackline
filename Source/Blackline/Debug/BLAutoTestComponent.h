#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BLAutoTestComponent.generated.h"

class ABLCharacter;

/**
 * Piloto automático de pruebas: conduce al personaje por una secuencia de pasos, verifica
 * el resultado de cada uno, hace capturas desde el render y cierra el juego.
 *
 * Uso: UnrealEditor.exe Blackline.uproject /Game/Maps/Dev/L_Dev_Movement -game -BLTest=Movement
 * Resultado: líneas "[BLTest]" en el log + Saved/BLTest/<Test>_results.txt + capturas en Saved/BLTest/.
 *
 * Los puntos de partida se buscan como actores con tag (TargetPoint "BLTest_Start_<Nombre>").
 */
UCLASS(ClassGroup = (Blackline))
class BLACKLINE_API UBLAutoTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLAutoTestComponent();

	void StartTest(const FString& TestName);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FStep
	{
		FString Name;
		float Duration = 1.f;
		TFunction<void()> Begin;
		TFunction<void(float /*Elapsed*/)> Tick;
		TFunction<bool(FString& /*OutDetail*/)> Verify;
		TFunction<void()> End;
		/** Si es > 0, se hace una captura en ese instante del paso. */
		float ScreenshotAt = -1.f;
		/** Opcional: termina el paso antes de Duration cuando devuelve true. */
		TFunction<bool()> Done;
	};

	void BuildMovementTest();
	void BuildWeaponsTest();
	/** Salud, daño por zonas, reacción, ragdoll, checkpoint, muerte y reaparición (BLAutoTestCombat.cpp). */
	void BuildCombatTest();
	/** Nivel de la misión 1: vistas por fase con fps, navegación, checkpoints y recorrido a pie completo (BLAutoTestLevel.cpp). */
	void BuildLevelTest();
	/** IA de la misión 1: patrulla, oído, detección, alerta, coberturas, disparo, recarga, flanqueo, persecución, bajas (BLAutoTestAI.cpp). */
	void BuildAITest();
	/** Flujo de la misión 1: briefing, objetivos, marcador, interacción y pantalla final (BLAutoTestMission.cpp). */
	void BuildMissionTest();
	/** Audio del Bloque 7: voces de radio, mezcla, ambiente por zona, acústica, pasos y barks de la IA, balas, música (BLAutoTestAudio.cpp). */
	void BuildAudioTest();
	/** Vistas fijas de la misión 1 para revisar el arte y medir fps (BLAutoTestViews.cpp). */
	void BuildViewsTest();

	UFUNCTION()
	void HandleShot(const FHitResult& Hit);
	UFUNCTION()
	void HandleCheckpoint(FName Id);
	TArray<FName> ReachedCheckpoints;
	void BeginStep(int32 Index);
	void Finish();
	bool TeleportToStart(const FString& StartName);
	void Screenshot(const FString& Name);
	float GroundSpeed() const;

	ABLCharacter* Char() const;

	TArray<FStep> Steps;
	FString CurrentTest;
	int32 StepIndex = INDEX_NONE;
	float StepElapsed = 0.f;
	bool bScreenshotTaken = false;
	bool bFinished = false;
	float QuitTimer = -1.f;

	TArray<FString> Results;
	int32 Passed = 0;
	int32 Failed = 0;

	// Medición de rendimiento durante la prueba
	double FrameTimeAccum = 0.0;
	int32 FrameCount = 0;
	float WorstFrame = 0.f;

	// Estado auxiliar compartido por los pasos
	float StartZ = 0.f;
	float MaxZ = 0.f;
	bool bSawMantle = false;
	bool bJumpPressed = false;

	// Prueba de armas
	int32 ShotsAtStart = 0;
	int32 HitsCount = 0;
	int32 MagAtStart = 0;
	int32 ReserveAtStart = 0;
	float PitchAtStart = 0.f;
	float PeakPitch = 0.f;
	FString PendingShotScreenshot;
	int32 LastSurface = 0;
	FString LastHitInfo;
	int32 MaxSprites = 0;
	int32 MaxSparks = 0;
	void AddSurfaceStep(const FString& Name, int32 ExpectedSurface, bool bExpectSparks);
};
