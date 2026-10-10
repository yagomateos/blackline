#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BLWeaponData.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UAnimSequence;
class USoundBase;
class UMaterialInterface;
class UNiagaraSystem;
class UBLSurfaceEffectsData;

UENUM(BlueprintType)
enum class EBLFireMode : uint8
{
	Semi,
	Auto,
	/** Corredera: un disparo por pulsación y después hay que bombear (CycleSounds, el guardamanos va y vuelve). */
	Pump
};

/** Cómo se recarga en primera persona (línea de tiempo procedural de la mano izquierda). */
UENUM(BlueprintType)
enum class EBLReloadStyle : uint8
{
	/** Saca el cargador, lo guarda, mete el nuevo; en vacío golpea la retenida. */
	Rifle,
	/** El cargador cae solo, la mano trae otro del cinturón; en vacío monta la corredera por encima. */
	Pistol,
	/** Cartucho a cartucho por la portilla de carga (escopeta). En vacío el primero entra por la ventana de expulsión
	 *  y se cierra la corredera; disparar interrumpe la recarga (los cartuchos ya metidos se quedan). */
	Shells
};

/**
 * Poses del arma en primera persona: desplazamientos en espacio de cámara respecto a la pose ADS calibrada
 * (las rotaciones pivotan sobre la mira). Valores por defecto = los del AR-7. Ver UBLFirstPersonRigComponent.
 */
USTRUCT(BlueprintType)
struct FBLWeaponPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector HipLocation = FVector(8.f, 4.5f, -2.5f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator HipRotation = FRotator(0.5f, -5.5f, -4.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector SprintLocation = FVector(7.f, 5.f, -4.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator SprintRotation = FRotator(1.f, -20.f, -28.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector MantleLocation = FVector(-3.f, 2.f, -4.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator MantleRotation = FRotator(-10.f, -10.f, -20.f);
	/** Recarga (pose absoluta en espacio de cámara): posición del pistolete y orientación del arma
	 *  (Yaw<0 cañón a la izquierda, Pitch>0 cañón arriba, Roll<0 parte de arriba hacia la izquierda). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector ReloadGripLocation = FVector(46.f, 9.f, -12.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator ReloadRotation = FRotator(20.f, -20.f, 40.f);
	/** Equipar/guardar: el arma sube desde abajo (y baja hasta aquí al cambiar de arma). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector EquipLocation = FVector(0.f, 4.f, -22.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator EquipRotation = FRotator(-35.f, -10.f, 20.f);
	/** Distancia ojo-mira con la que se ajustaron estas poses (fuera de ADS no dependen de AimEyeDistance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PoseReferenceAimDistance = 9.f;
};

/** Sonido que se reproduce en un instante concreto de una acción (p. ej. sacar cargador a 0,3 s). */
USTRUCT(BlueprintType)
struct FBLTimedSound
{
	GENERATED_BODY()

	/** Fracción de la acción (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float Time = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Sound;
};

/**
 * Definición de un arma: características, retroceso, recarga, animaciones y efectos.
 * Cada arma del juego es un asset de este tipo (DA_AR7, DA_P17...).
 */
UCLASS(BlueprintType)
class BLACKLINE_API UBLWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General") TObjectPtr<USkeletalMesh> Mesh;

	// ---- Sockets / huesos de la malla ----
	UPROPERTY(EditAnywhere, Category = "Sockets") FName MuzzleSocket = FName("Muzzle");
	UPROPERTY(EditAnywhere, Category = "Sockets") FName EjectSocket = FName("Eject");
	/** Punto de agarre de la mano izquierda (IK). */
	UPROPERTY(EditAnywhere, Category = "Sockets") FName LeftHandSocket = FName("HandGrip_L");
	UPROPERTY(EditAnywhere, Category = "Sockets") FName SightSocket = FName("Sight");
	/** Mira en espacio local de la malla si no existe SightSocket. */
	UPROPERTY(EditAnywhere, Category = "Sockets") FVector SightLocalOffset = FVector(0.f, 0.f, 17.f);
	/** Base del cargador (donde la mano lo agarra en la recarga). */
	UPROPERTY(EditAnywhere, Category = "Sockets") FName MagSocket = FName("Mag");
	UPROPERTY(EditAnywhere, Category = "Sockets") FVector MagLocalOffset = FVector(0.f, 9.f, -14.f);
	/** Hueso del cargador (se oculta mientras el cargador está en la mano) y malla suelta del cargador. */
	UPROPERTY(EditAnywhere, Category = "Sockets") FName MagazineBone = FName("magazine");
	UPROPERTY(EditAnywhere, Category = "Reload") TObjectPtr<UStaticMesh> MagazineMesh;
	/** Palanca de carga / cerrojo (recarga en vacío). */
	UPROPERTY(EditAnywhere, Category = "Sockets") FName ChargingHandleSocket = FName("ChargingHandle");
	UPROPERTY(EditAnywhere, Category = "Sockets") FVector ChargingHandleLocalOffset = FVector(-3.f, 6.f, 9.f);
	/** Ventana de expulsión si no existe EjectSocket. */
	UPROPERTY(EditAnywhere, Category = "Sockets") FVector EjectLocalOffset = FVector(1.5f, 8.f, 8.f);
	/** Distancia ojo-mira en ADS (cm). Miras de apertura: cerca del ojo; ópticas: según su relieve ocular. */
	UPROPERTY(EditAnywhere, Category = "Sockets") float AimEyeDistance = 14.f;
	/** Eje "adelante" de la malla (las armas de Epic y las nuestras apuntan a +Y). */
	UPROPERTY(EditAnywhere, Category = "Sockets") FVector ForwardAxis = FVector(0.f, 1.f, 0.f);
	/** Giro de la mano izquierda en el agarre, en espacio de la malla (pistola: la palma abraza el puño por la izquierda). */
	UPROPERTY(EditAnywhere, Category = "Sockets") FRotator LeftHandGripRotation = FRotator::ZeroRotator;
	/** Mano derecha respecto al origen del arma (espacio de la malla): el Mannequin agarra todas las armas como el fusil;
	 *  en la pistola la mano baja por el puño (si no, el dorso tapa la corredera) y se inclina con él. */
	UPROPERTY(EditAnywhere, Category = "Sockets") FVector RightHandOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Sockets") FRotator RightHandRotation = FRotator::ZeroRotator;

	// ---- Primera persona ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FirstPerson") FBLWeaponPoses Poses;

	// ---- Disparo ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") EBLFireMode FireMode = EBLFireMode::Auto;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "1")) float RoundsPerMinute = 750.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float Damage = 28.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float HeadshotMultiplier = 2.5f;
	/** Brazos y piernas. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float LimbMultiplier = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float MaxRange = 15000.f;
	/** Distancia a la que la IA oye el disparo (cm) y a la que nota los impactos cercanos. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float NoiseRange = 4000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float ImpactNoiseRange = 900.f;
	/** Caída de daño con la distancia (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float FalloffStart = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float FalloffEnd = 8000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float FalloffMinMultiplier = 0.65f;
	/** Perdigones por disparo (escopeta). Cada uno hace Damage; el hitmarker suma los de un mismo blanco. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "1")) int32 PelletCount = 1;
	/** Semiángulo del cono de los perdigones (grados) desde la cadera y apuntando; se suma a la dispersión normal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float PelletSpread = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float PelletAimSpread = 0.f;
	/** Tiempo desde que se deja de esprintar hasta poder disparar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire") float SprintToFireTime = 0.15f;

	// ---- Munición ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1")) int32 MagazineSize = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo") int32 MaxReserveAmmo = 210;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo") int32 StartReserveAmmo = 120;

	// ---- Dispersión (grados, semiángulo del cono) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float HipSpread = 2.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float AimSpread = 0.06f;
	/** Extra a velocidad de andar (escala con la velocidad). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float MoveSpread = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float AirSpread = 4.f;
	/** Aumento por disparo y máximo acumulado. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float SpreadPerShot = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float MaxBloom = 3.f;
	/** Recuperación de la dispersión acumulada (grados/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float BloomRecovery = 8.f;
	/** En ADS la dispersión acumulada se multiplica por esto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread") float AimBloomMultiplier = 0.15f;

	// ---- Retroceso: mueve la mira real del jugador ----
	/** Subida vertical por disparo (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilVertical = 0.55f;
	/** Desvío horizontal aleatorio máximo por disparo (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilHorizontal = 0.22f;
	/** Tendencia horizontal (-1 izquierda .. 1 derecha): hace el patrón aprendible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilHorizontalBias = 0.3f;
	/** El primer disparo de una ráfaga sube algo más. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilFirstShotMultiplier = 1.4f;
	/** Disparos hasta que el retroceso vertical llega al 100% (empieza en el 70%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") int32 RecoilRampShots = 5;
	/** Retroceso en ADS respecto a cadera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilAimMultiplier = 0.75f;
	/** Velocidad a la que se aplica el retroceso (1/s): da "peso" en vez de un salto instantáneo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilApplySpeed = 30.f;
	/** Parte del retroceso vertical que se recupera sola al dejar de disparar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0", ClampMax = "1")) float RecoilRecoveryFraction = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilRecoverySpeed = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil") float RecoilRecoveryDelay = 0.08f;

	// ---- Retroceso visual (no cambia la mira, solo cámara/arma) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Visual") FRotator CameraKick = FRotator(14.f, 0.f, 6.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Visual") FVector WeaponKickLocation = FVector(-55.f, 0.f, 8.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Visual") FRotator WeaponKickRotation = FRotator(40.f, 8.f, 14.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Visual") float VisualKickAimMultiplier = 0.45f;

	// ---- Recarga ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload") float ReloadTime = 2.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload") float EmptyReloadTime = 2.6f;
	/** Recarga táctica (con bala en la recámara): el arma queda con cargador + 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload") bool bChamberRound = true;
	/** Momento (fracción) en que la munición entra en el arma. Cancelar antes no recarga. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0", ClampMax = "1")) float ReloadAmmoInsertTime = 0.62f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload") EBLReloadStyle ReloadStyle = EBLReloadStyle::Rifle;
	/** Recarga cartucho a cartucho (ReloadStyle Shells): llevar el arma a la pose, cada cartucho, meter el primero por la
	 *  ventana y cerrar (en vacío) y volver. ReloadAmmoInsertTime = fracción de cada cartucho en que entra. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload|Shells") float ShellReloadStartTime = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload|Shells") float ShellInsertTime = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload|Shells") float ShellPortLoadTime = 0.95f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload|Shells") float ShellReloadEndTime = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload|Shells") TArray<TObjectPtr<USoundBase>> ShellInsertSounds;
	/** Sonido del cargador vacío al caer al suelo (recarga de pistola). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reload") TObjectPtr<USoundBase> MagazineDropSound;

	// ---- Mecánica del arma (huesos de la propia malla, UBLWeaponAnimInstance) ----
	/** Cerrojo: retrocede en cada disparo y queda abierto con el cargador vacío. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") FName BoltBone = FName("bolt_carrier");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") float BoltTravel = 7.f;
	/** Duración del ciclo del cerrojo (s); se acorta si la cadencia no da tiempo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") float BoltCycleTime = 0.07f;
	/** Fracción de la recarga en vacío en que se suelta el cerrojo (golpe a la retenida). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics", meta = (ClampMin = "0", ClampMax = "1")) float BoltReleaseTime = 0.71f;
	/** Corredera (FireMode Pump): sonidos del bombeo tras cada disparo (Time = fracción del intervalo entre disparos)
	 *  y momento en que sale la vaina (0 = al disparar, como en las armas automáticas). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") TArray<FBLTimedSound> CycleSounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics", meta = (ClampMin = "0", ClampMax = "1")) float CasingEjectTime = 0.f;
	/** La mano izquierda sigue al hueso del cerrojo (guardamanos de la escopeta). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") bool bLeftHandOnBolt = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") FName TriggerBone = FName("trigger");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mechanics") float TriggerTravel = 0.3f;

	// ---- Animaciones (sobre el esqueleto del Mannequin) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TObjectPtr<UAnimSequence> ReloadAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TObjectPtr<UAnimSequence> EquipAnim;
	/** Equipar al aparecer dura esto (la animación se acelera para encajar). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") float EquipTime = 0.6f;
	/** Bajar el arma al cambiar a otra. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") float HolsterTime = 0.35f;

	// ---- Sonido ----
	/** Disparo cercano del jugador (2D, estéreo): golpe + primeras reflexiones. Una al azar por tiro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> FireSounds;
	/** Cola (eco del entorno): se reproduce al terminar cada disparo suelto o ráfaga. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> FireTailSounds;
	/** Disparo de otros (IA) espacializado: cercano y lejano, mezclados por distancia. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> FireSounds3D;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> FireDistantSounds;
	/** Distancia (cm) a la que el disparo lejano sustituye al cercano. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") float DistantFireDistance = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TObjectPtr<USoundBase> DryFireSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<FBLTimedSound> ReloadSounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<FBLTimedSound> EmptyReloadSounds;
	/** Casquillo si el suelo no tiene superficie conocida (normalmente se usan los de SurfaceEffects). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> CasingSounds;
	/** Ruido de manipulación (foley) al recargar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound") TArray<TObjectPtr<USoundBase>> HandlingSounds;

	// ---- Efectos ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") TObjectPtr<UStaticMesh> MuzzleFlashMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") float MuzzleFlashDuration = 0.03f;
	/** Tamaño del fogonazo (pistola < 1: menos gas y cañón corto). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") float MuzzleFlashScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") FLinearColor MuzzleLightColor = FLinearColor(1.f, 0.62f, 0.3f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") float MuzzleLightIntensity = 7000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") TObjectPtr<UStaticMesh> CasingMesh;
	/** Impactos, marcas y sonidos por material (hormigón, metal, madera, cristal, tierra). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") TObjectPtr<UBLSurfaceEffectsData> SurfaceEffects;
	/** Multiplicador de la cantidad de partículas de impacto (escopeta < 1 por perdigón, francotirador > 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX") float ImpactScale = 1.f;

	/** Elemento aleatorio de una lista de variaciones (nullptr si está vacía). */
	static USoundBase* PickRandom(const TArray<TObjectPtr<USoundBase>>& Sounds)
	{
		return Sounds.Num() > 0 ? Sounds[FMath::RandHelper(Sounds.Num())].Get() : nullptr;
	}

	float GetShotInterval() const { return 60.f / FMath::Max(RoundsPerMinute, 1.f); }
};
