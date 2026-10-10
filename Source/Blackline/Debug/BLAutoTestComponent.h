#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/BLDamageTypes.h"
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
	/** Pistola P-17 y cambio de arma (BLAutoTestPistol.cpp). */
	void BuildPistolTest();
	void BuildPistolHandsTest();
	/** Escopeta SG-12 y armas del suelo (BLAutoTestShotgun.cpp). */
	void BuildShotgunTest();
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
	/** Granada M-6 (BLAutoTestGrenade.cpp): lanzamiento, explosión, daño; y la reacción de la IA. */
	void BuildGrenadeTest();
	void BuildGrenadeAITest();
	/** Misión 2 "Manifiesto" de principio a fin: sigilo nocturno, fotos, alarma, dron, gas, lancha (BLAutoTestMission2.cpp). */
	void BuildMission2Test();
	/** Misión 3 "Ría": brecha, tirador con láser, operadores, baliza, el barco zarpa (BLAutoTestMission3.cpp). */
	void BuildMission3Test();
	/** Misión 4 "Fuego cruzado": aliados, ametralladora montada, cazas, voladura del puente (BLAutoTestMission4.cpp). */
	void BuildMission4Test();
	/** Misión 5 "Línea negra" de principio a fin y su condición de fallo (BLAutoTestMission5.cpp). */
	void BuildMission5Test();
	void BuildMission5FailTest();
	/** Balas reales del jugador contra los enemigos de cualquier mapa (BLAutoTestFiring.cpp). */
	void BuildFiringTest();
	void BuildMountedGunTest();
	void BuildRangeTest();
	void BuildSurvivalTest();
	void BuildRotorTest();

	UFUNCTION()
	void HandleShot(const FHitResult& Hit);
	UFUNCTION()
	void HandleCheckpoint(FName Id);
	UFUNCTION()
	void HandleHitConfirmed(EBLHitZone Zone, float Damage, bool bKilled) { ++HitConfirms; HitConfirmDamage += Damage; }
	int32 HitConfirms = 0;
	float HitConfirmDamage = 0.f;
	// Prueba "Distancias": impactos confirmados por zona y el que mató
	UFUNCTION()
	void HandleRangeHit(EBLHitZone Zone, float Damage, bool bKilled)
	{
		++RangeHits;
		RangeHeadHits += Zone == EBLHitZone::Head ? 1 : 0;
		if (bKilled && RangeKillHit < 0) { RangeKillHit = RangeHits; RangeKillZone = Zone; }
	}
	int32 RangeHits = 0;
	int32 RangeHeadHits = 0;
	int32 RangeKillHit = -1;
	EBLHitZone RangeKillZone = EBLHitZone::Torso;
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
	/** Disparo (desde ShotsAtStart) en el que se hace PendingShotScreenshot. */
	int32 ShotScreenshotAt = 5;
	int32 LastSurface = 0;
	FString LastHitInfo;
	// Disparos sin impacto en la ráfaga actual: dirección (pitch/yaw) y origen, para diagnosticar
	FString MissInfo;
	// Frames de más de 100 ms: paso y segundo en que ocurrieron (para localizar tirones)
	TArray<FString> Spikes;
	// Frame de la última captura: la lectura de la GPU congela 1-2 frames y no cuenta como tirón del juego
	uint64 LastScreenshotFrame = 0;
	int32 MaxSprites = 0;
	int32 MaxSparks = 0;
	void AddSurfaceStep(const FString& Name, int32 ExpectedSurface, bool bExpectSparks);
};
