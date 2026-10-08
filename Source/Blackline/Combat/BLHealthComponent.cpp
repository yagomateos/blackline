#include "Combat/BLHealthComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UBLHealthComponent::UBLHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBLHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetHealth();
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakePointDamage.AddDynamic(this, &UBLHealthComponent::HandlePointDamage);
		Owner->OnTakeRadialDamage.AddDynamic(this, &UBLHealthComponent::HandleRadialDamage);
	}
}

void UBLHealthComponent::ResetHealth(float NewHealth)
{
	Health = NewHealth > 0.f ? FMath::Min(NewHealth, MaxHealth) : MaxHealth;
	RegenTarget = SegmentCeiling();
	TimeSinceDamage = 100.f;
	bDead = false;
}

float UBLHealthComponent::SegmentCeiling() const
{
	if (Segments <= 1)
	{
		return MaxHealth;
	}
	const float Size = MaxHealth / Segments;
	// Tope del segmento en el que está la salud (un segmento lleno exacto cuenta como su propio tope)
	return FMath::Min(MaxHealth, FMath::CeilToFloat(Health / Size - KINDA_SMALL_NUMBER) * Size);
}

float UBLHealthComponent::ApplyDamage(FBLDamageInfo Info)
{
	if (bDead || bInvulnerable || Info.Amount <= 0.f)
	{
		return 0.f;
	}
	const float Applied = FMath::Min(Info.Amount * DamageTakenMultiplier, Health);
	Health -= Applied;
	TimeSinceDamage = 0.f;
	RegenTarget = SegmentCeiling();
	Info.Amount = Applied;
	if (Health <= KINDA_SMALL_NUMBER)
	{
		Health = 0.f;
		bDead = true;
		Info.bKilled = true;
	}
	OnDamaged.Broadcast(Info);
	if (bDead)
	{
		OnDeath.Broadcast(Info);
	}
	return Applied;
}

void UBLHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TimeSinceDamage += DeltaTime;
	if (!bDead && RegenRate > 0.f && TimeSinceDamage >= RegenDelay && Health < RegenTarget)
	{
		Health = FMath::Min(RegenTarget, Health + RegenRate * DeltaTime);
	}
}

void UBLHealthComponent::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	FBLDamageInfo Info;
	Info.Amount = Damage;
	Info.Bone = BoneName;
	Info.Zone = BLDamage::ZoneFromBone(BoneName);
	Info.Location = HitLocation;
	Info.Direction = ShotFromDirection.GetSafeNormal();
	Info.Causer = DamageCauser;
	Info.Instigator = InstigatedBy;
	const APawn* InstigatorPawn = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	Info.SourceLocation = InstigatorPawn ? InstigatorPawn->GetActorLocation()
		: DamageCauser ? DamageCauser->GetActorLocation() : HitLocation - Info.Direction * 1000.f;
	ApplyDamage(Info);
}

void UBLHealthComponent::HandleRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin,
	const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser)
{
	FBLDamageInfo Info;
	Info.Amount = Damage;
	Info.Bone = HitInfo.BoneName;
	Info.Zone = EBLHitZone::Torso;
	Info.Location = HitInfo.bBlockingHit ? FVector(HitInfo.ImpactPoint) : GetOwner()->GetActorLocation();
	Info.Direction = (Info.Location - Origin).GetSafeNormal();
	Info.SourceLocation = Origin;
	Info.Causer = DamageCauser;
	Info.Instigator = InstigatedBy;
	ApplyDamage(Info);
}
