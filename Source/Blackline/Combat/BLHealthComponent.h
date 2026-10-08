#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/BLDamageTypes.h"
#include "BLHealthComponent.generated.h"

class AController;

/** Un golpe recibido (lo usan la reacción al impacto, el HUD y la IA). */
USTRUCT(BlueprintType)
struct FBLDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float Amount = 0.f;
	UPROPERTY(BlueprintReadOnly) EBLHitZone Zone = EBLHitZone::None;
	UPROPERTY(BlueprintReadOnly) FName Bone = NAME_None;
	/** Punto de impacto (o la posición del dueño si el daño no es puntual). */
	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	/** Dirección de la bala / del empuje (normalizada). */
	UPROPERTY(BlueprintReadOnly) FVector Direction = FVector::ZeroVector;
	/** Posición de quien causó el daño (indicador direccional). */
	UPROPERTY(BlueprintReadOnly) FVector SourceLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Causer;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AController> Instigator;
	UPROPERTY(BlueprintReadOnly) bool bKilled = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBLDamagedSignature, const FBLDamageInfo&, Info);

/**
 * Salud por segmentos con regeneración parcial: tras RegenDelay segundos sin daño se recupera
 * hasta el tope del segmento actual (no se recupera un segmento perdido entero).
 * Recibe el daño estándar de Unreal (ApplyPointDamage / ApplyDamage) del actor dueño.
 * El multiplicador por zona lo aplica el arma (BLDamage::ZoneFromBone); aquí se aplica DamageTakenMultiplier.
 */
UCLASS(ClassGroup = (Blackline), meta = (BlueprintSpawnableComponent))
class BLACKLINE_API UBLHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBLHealthComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Health") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintCallable, Category = "Health") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintCallable, Category = "Health") float GetHealthFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	UFUNCTION(BlueprintCallable, Category = "Health") bool IsDead() const { return bDead; }
	float GetTimeSinceDamage() const { return TimeSinceDamage; }

	/** Aplica daño directamente (lo usan los manejadores de daño de Unreal y las pruebas). Devuelve el daño aplicado. */
	float ApplyDamage(FBLDamageInfo Info);

	/** Restaura la salud (reaparición en checkpoint). */
	UFUNCTION(BlueprintCallable, Category = "Health") void ResetHealth(float NewHealth = -1.f);

	UPROPERTY(BlueprintAssignable, Category = "Health") FBLDamagedSignature OnDamaged;
	UPROPERTY(BlueprintAssignable, Category = "Health") FBLDamagedSignature OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1")) float MaxHealth = 100.f;
	/** Número de segmentos (1 = sin segmentos: regenera hasta el máximo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1")) int32 Segments = 4;
	/** 0 = sin regeneración. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health") float RegenRate = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health") float RegenDelay = 4.f;
	/** Dificultad / blindaje. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health") float DamageTakenMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health") bool bInvulnerable = false;

private:
	UFUNCTION() void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);
	UFUNCTION() void HandleRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin,
		const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser);

	float SegmentCeiling() const;

	float Health = 100.f;
	float RegenTarget = 100.f;
	float TimeSinceDamage = 100.f;
	bool bDead = false;
};
