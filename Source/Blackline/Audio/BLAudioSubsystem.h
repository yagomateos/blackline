#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BLAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundMix;

/** Categorías de frases de la IA. Los sonidos se buscan por nombre: /Game/Audio/Voice/Barks/VO_Bark_<Categoría>_V<voz>_<n>. */
UENUM(BlueprintType)
enum class EBLBark : uint8
{
	Suspicious,
	HeardShots,
	Contact,
	Hit,
	Reload,
	Flank,
	Reposition,
	Chase,
	GiveUp,
	Lost,
	ManDown,
	Grenade,
	MAX UMETA(Hidden),
};

/**
 * Director de audio del mundo (Bloque 7). Junta lo que no pertenece a un actor concreto:
 *  - Barks de la IA: elige variación y voz, evita que se pisen (hueco global y por categoría) y los
 *    reproduce en 3D pegados a la cabeza del que habla. "¡Hombre abajo!" lo dice el compañero más cercano.
 *  - Música dinámica: solo en combate (golpe de entrada + bucle que se funde al acabar) y cierre de misión.
 *  - Combate lejano: ráfagas, explosiones y una sirena en direcciones aleatorias a 150-250 m (la ciudad está en guerra).
 *  - Balas que pasan cerca del jugador: chasquido supersónico + silbido en el punto de paso.
 *  - Mezcla: baja ambiente y música mientras habla la radio (SoundMix SMix_RadioDuck).
 */
UCLASS()
class BLACKLINE_API UBLAudioSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UBLAudioSubsystem* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBLAudioSubsystem, STATGROUP_Tickables); }

	/** Intenta decir una frase. Devuelve el componente de audio (o null si se descarta para no saturar). */
	UAudioComponent* PlayBark(AActor* Speaker, int32 VoiceIndex, EBLBark Bark, const TCHAR* LogText);
	/** Un enemigo ha caído: el compañero vivo más cercano (< 30 m) lo avisa. */
	void NotifyEnemyKilled(AActor* Victim);
	/** Disparo de la IA: si la trayectoria pasa cerca del oyente, suena el chasquido de la bala. */
	void NotifyBulletPass(const FVector& Start, const FVector& End, const AActor* HitActor);
	/** La radio empieza/termina de hablar (atenúa ambiente y música). */
	void SetRadioTalking(bool bTalking);
	void OnMissionComplete();

	static const TCHAR* BarkName(EBLBark Bark);

	// ---- Estado para pruebas ----
	int32 GetBarksPlayed() const { return BarksPlayed; }
	int32 GetBarksPlayed(EBLBark Bark) const { return BarkCounts[(int32)Bark]; }
	int32 GetBarksSkipped() const { return BarksSkipped; }
	int32 GetBulletPasses() const { return BulletPasses; }
	int32 GetDistantEvents() const { return DistantEvents; }
	int32 GetStingersPlayed() const { return StingersPlayed; }
	bool IsCombatMusicOn() const { return bCombatMusic; }
	bool IsRadioDucking() const { return bDucking; }
	/** Número de variaciones cargadas de una categoría/voz (0 = faltan los sonidos). */
	int32 CountBarkSounds(EBLBark Bark, int32 VoiceIndex);

	/** Ajustes. */
	float MusicVolume = 0.42f;
	float BarkGlobalGap = 0.6f;
	float BarkAudibleDistance = 6000.f;
	float DistantMinInterval = 5.f;
	float DistantMaxInterval = 14.f;
	bool bDistantBattle = true;

private:
	const TArray<TObjectPtr<USoundBase>>& GetBarkSounds(EBLBark Bark, int32 VoiceIndex);
	USoundBase* LoadSound(const FString& Name, const TCHAR* Folder);
	void LoadList(TArray<TObjectPtr<USoundBase>>& Out, const FString& Prefix, const TCHAR* Folder, int32 Max);
	void TickMusic(float DeltaTime);
	void TickDistant(float DeltaTime);
	bool GetListener(FVector& OutLocation, APawn*& OutPawn) const;

	/** Clave "Categoría_V<voz>" -> variaciones. */
	TMap<FString, TArray<TObjectPtr<USoundBase>>> BarkSounds;
	/** Mantiene vivos los sonidos cargados por nombre (no los referencia ningún asset). */
	UPROPERTY() TArray<TObjectPtr<UObject>> Loaded;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> CrackSounds;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> WhizSounds;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> DistantBursts;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> DistantExplosions;
	UPROPERTY() TObjectPtr<USoundBase> Siren;
	UPROPERTY() TObjectPtr<USoundBase> MusicCombat;
	UPROPERTY() TObjectPtr<USoundBase> StingerContact;
	UPROPERTY() TObjectPtr<USoundBase> StingerComplete;
	UPROPERTY() TObjectPtr<USoundMix> RadioDuckMix;
	UPROPERTY() TObjectPtr<UAudioComponent> MusicComponent;

	struct FPendingBark
	{
		TWeakObjectPtr<AActor> Speaker;
		int32 Voice = 0;
		EBLBark Bark = EBLBark::ManDown;
		float Delay = 0.f;
	};
	TArray<FPendingBark> Pending;

	double LastBarkTime = -100.0;
	double LastBarkByCategory[(int32)EBLBark::MAX];
	int32 BarkCounts[(int32)EBLBark::MAX] = {};
	int32 BarksPlayed = 0;
	int32 BarksSkipped = 0;

	bool bCombatMusic = false;
	bool bMissionDone = false;
	float NoCombatTime = 100.f;
	float SinceCombatEnd = 1000.f;
	int32 StingersPlayed = 0;

	float DistantTimer = 6.f;
	float SirenTimer = 30.f;
	int32 DistantEvents = 0;

	double LastCrackTime = -100.0;
	int32 BulletPasses = 0;
	bool bDucking = false;
};
