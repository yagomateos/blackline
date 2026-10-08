// Prueba automática "Combat" (Bloque 3): mapa L_Dev_Movement, estaciones "Dianas" y "Checkpoint".
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "Combat/BLHealthComponent.h"
#include "Combat/BLHitReactionComponent.h"
#include "Combat/BLTargetDummy.h"
#include "FX/BLFXSubsystem.h"
#include "FX/BLSurfaceEffects.h"
#include "Mission/BLCheckpointSubsystem.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectIterator.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	ABLTargetDummy* FindDummy(UWorld* World, int32 Index)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(World, FName(*FString::Printf(TEXT("BLDummy_%d"), Index)), Found);
		return Found.Num() > 0 ? Cast<ABLTargetDummy>(Found[0]) : nullptr;
	}

	/** Punto al que apuntar en una zona del maniquí (centro aproximado del cuerpo físico). */
	FVector ZonePoint(const ABLTargetDummy* Dummy, EBLHitZone Zone)
	{
		const USkeletalMeshComponent* M = Dummy->GetMesh();
		switch (Zone)
		{
		case EBLHitZone::Head: return M->GetBoneLocation(TEXT("head")) + FVector(0.f, 0.f, 8.f);
		case EBLHitZone::Limb: return (M->GetBoneLocation(TEXT("thigh_l")) + M->GetBoneLocation(TEXT("calf_l"))) * 0.5f;
		default: return M->GetBoneLocation(TEXT("spine_03"));
		}
	}
}

void UBLAutoTestComponent::BuildCombatTest()
{
	UBLWeaponComponent* W = Char()->GetWeapon();
	W->OnShot.AddUniqueDynamic(this, &UBLAutoTestComponent::HandleShot);
	const float Damage = W->GetWeaponData()->Damage;
	Char()->GetHealth()->DamageTakenMultiplier = 1.f;   // se prueba la mecánica, sin la dificultad

	// Estado compartido entre pasos
	struct FState
	{
		float HealthBefore = 0.f;
		float MaxReaction = 0.f;
		bool bFired = false;
		bool bReleased = false;
		float HitMarkerAge = 100.f;
		bool bKill = false;
		FVector DeathSpot = FVector::ZeroVector;
		float CameraDropZ = 0.f;
	};
	TSharedRef<FState> St = MakeShared<FState>();

	// Un disparo en ADS a una zona de un maniquí: apunta cada frame hasta disparar, suelta el gatillo al frame siguiente
	auto ShotStep = [this, St, W](const FString& Name, int32 DummyIndex, EBLHitZone Zone, TFunction<bool(FString&, ABLTargetDummy*)> Verify, float Shot = -1.f)
	{
		Steps.Add({ Name, 1.3f,
			[this, St, DummyIndex]()
			{
				TeleportToStart(TEXT("Dianas"));
				Char()->SetAimHeld(true);
				St->bFired = St->bReleased = false;
				St->MaxReaction = 0.f;
				St->HitMarkerAge = 100.f;
				St->bKill = false;
				LastSurface = -1;
				if (ABLTargetDummy* D = FindDummy(GetWorld(), DummyIndex))
				{
					St->HealthBefore = D->GetHealth()->GetHealth();
				}
			},
			[this, St, DummyIndex, Zone, W](float T)
			{
				ABLTargetDummy* D = FindDummy(GetWorld(), DummyIndex);
				if (!D)
				{
					return;
				}
				if (!St->bFired)
				{
					const FVector Eye = Char()->GetCamera()->GetComponentLocation();
					Char()->GetController()->SetControlRotation((ZonePoint(D, Zone) - Eye).Rotation());
				}
				if (!St->bFired && T > 0.55f && !W->IsEquipping())
				{
					Char()->SetFireHeld(true);
					St->bFired = true;
				}
				else if (St->bFired && !St->bReleased)
				{
					Char()->SetFireHeld(false);
					St->bReleased = true;
				}
				St->MaxReaction = FMath::Max(St->MaxReaction, D->GetHitReaction()->GetReactionWeight());
				St->HitMarkerAge = FMath::Min(St->HitMarkerAge, Char()->GetHitMarkerAge());
				St->bKill |= Char()->IsHitMarkerKill() && Char()->GetHitMarkerAge() < 0.5f;
			},
			[this, St, DummyIndex, Verify](FString& Detail)
			{
				ABLTargetDummy* D = FindDummy(GetWorld(), DummyIndex);
				return D && Verify(Detail, D);
			},
			[this]() { Char()->SetFireHeld(false); Char()->SetAimHeld(false); }, Shot });
	};

	Steps.Add({ TEXT("Dianas"), 0.8f, [this]() { TeleportToStart(TEXT("Dianas")); }, nullptr,
		[this](FString& D)
		{
			int32 N = 0;
			for (int32 i = 1; i <= 3; ++i) { N += FindDummy(GetWorld(), i) ? 1 : 0; }
			D = FString::Printf(TEXT("%d maniquies en el mapa"), N);
			return N == 3;
		} });

	ShotStep(TEXT("Torso"), 2, EBLHitZone::Torso, [St, Damage, this](FString& D, ABLTargetDummy* Dummy)
	{
		const float Lost = St->HealthBefore - Dummy->GetHealth()->GetHealth();
		D = FString::Printf(TEXT("dano %.1f (esperado %.1f), reaccion fisica %.2f, hitmarker %.2f s, superficie %s"), Lost, Damage, St->MaxReaction,
			St->HitMarkerAge, UBLSurfaceEffectsData::Name(EPhysicalSurface(FMath::Max(LastSurface, 0))));
		return FMath::IsNearlyEqual(Lost, Damage, 0.5f) && St->MaxReaction > 0.5f && St->HitMarkerAge < 1.5f && LastSurface == int32(BLSurface::Flesh);
	}, 0.62f);

	ShotStep(TEXT("Cabeza"), 1, EBLHitZone::Head, [St, Damage, W](FString& D, ABLTargetDummy* Dummy)
	{
		const float Lost = St->HealthBefore - Dummy->GetHealth()->GetHealth();
		const float Expected = Damage * W->GetWeaponData()->HeadshotMultiplier;
		D = FString::Printf(TEXT("dano %.1f (esperado %.1f)"), Lost, Expected);
		return FMath::IsNearlyEqual(Lost, Expected, 0.5f);
	});

	ShotStep(TEXT("Pierna"), 3, EBLHitZone::Limb, [St, Damage, W](FString& D, ABLTargetDummy* Dummy)
	{
		const float Lost = St->HealthBefore - Dummy->GetHealth()->GetHealth();
		const float Expected = Damage * W->GetWeaponData()->LimbMultiplier;
		D = FString::Printf(TEXT("dano %.1f (esperado %.1f)"), Lost, Expected);
		return FMath::IsNearlyEqual(Lost, Expected, 0.5f);
	});

	// Mirar la pared detrás de la diana central (salpicaduras de los disparos anteriores); solo captura
	Steps.Add({ TEXT("Salpicadura"), 0.7f,
		[this]()
		{
			TeleportToStart(TEXT("Dianas"));
			if (ABLTargetDummy* D = FindDummy(GetWorld(), 1))
			{
				// De cerca, a un lado de la diana del disparo a la cabeza, mirando a la pared de detrás
				Char()->SetActorLocation(FVector(820.f, D->GetActorLocation().Y - 170.f, Char()->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
				Char()->GetController()->SetControlRotation(FRotator::ZeroRotator);
			}
		},
		[this](float T)
		{
			if (ABLTargetDummy* D = FindDummy(GetWorld(), 1))
			{
				const FVector Eye = Char()->GetCamera()->GetComponentLocation();
				const FVector Wall(1050.f, D->GetActorLocation().Y - 40.f, 125.f);
				Char()->GetController()->SetControlRotation((Wall - Eye).Rotation());
			}
		},
		[this](FString& D)
		{
			const UBLFXSubsystem* FX = GetWorld()->GetSubsystem<UBLFXSubsystem>();
			const int32 N = FX ? FX->GetActiveDecals() : 0;
			FString Where;
			for (TObjectIterator<UDecalComponent> It; It; ++It)
			{
				if (It->GetWorld() == GetWorld() && It->GetDecalMaterial() && It->GetDecalMaterial()->GetName().Contains(TEXT("Blood")))
				{
					Where += It->GetComponentLocation().ToCompactString() + TEXT(" ");
				}
			}
			D = FString::Printf(TEXT("salpicaduras de sangre en la pared: %d (3 disparos a carne) %s"), N, *Where);
			return N >= 2;
		}, nullptr, 0.5f });

	ShotStep(TEXT("Baja"), 1, EBLHitZone::Head, [St](FString& D, ABLTargetDummy* Dummy)
	{
		D = FString::Printf(TEXT("muerto=%d ragdoll=%d hitmarker de baja=%d"), Dummy->GetHealth()->IsDead(), Dummy->GetHitReaction()->IsRagdoll(), St->bKill);
		return Dummy->GetHealth()->IsDead() && Dummy->GetHitReaction()->IsRagdoll() && St->bKill;
	}, 1.2f);

	Steps.Add({ TEXT("Reaparece"), 5.0f, nullptr, nullptr,
		[this](FString& D)
		{
			ABLTargetDummy* Dummy = FindDummy(GetWorld(), 1);
			D = Dummy ? FString::Printf(TEXT("salud %.0f muerto=%d ragdoll=%d"), Dummy->GetHealth()->GetHealth(), Dummy->GetHealth()->IsDead(), Dummy->GetHitReaction()->IsRagdoll()) : TEXT("sin maniqui");
			return Dummy && !Dummy->GetHealth()->IsDead() && !Dummy->GetHitReaction()->IsRagdoll() && Dummy->GetHealth()->GetHealth() >= 99.f;
		}, nullptr, 4.9f });

	Steps.Add({ TEXT("Checkpoint"), 0.8f, [this]() { TeleportToStart(TEXT("Checkpoint")); }, nullptr,
		[this](FString& D)
		{
			const UBLCheckpointSubsystem* C = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>();
			D = C ? FString::Printf(TEXT("checkpoint '%s' en %s"), *C->GetCurrentId().ToString(), *C->GetRespawnTransform().GetLocation().ToCompactString()) : TEXT("sin subsistema");
			return C && C->GetCurrentId() == FName("Dianas");
		}, nullptr, 0.4f });

	// Daño al jugador desde la derecha: salud, indicador, destello y sacudida
	Steps.Add({ TEXT("DanoJugador"), 0.6f,
		[this]()
		{
			TeleportToStart(TEXT("Dianas"));
			FBLDamageInfo Info;
			Info.Amount = 30.f;
			Info.Zone = EBLHitZone::Torso;
			Info.SourceLocation = Char()->GetActorLocation() + Char()->GetActorRightVector() * 800.f;
			Info.Direction = -Char()->GetActorRightVector();
			Info.Location = Char()->GetActorLocation();
			Char()->GetHealth()->ApplyDamage(Info);
		}, nullptr,
		[this](FString& D)
		{
			D = FString::Printf(TEXT("salud %.0f, indicadores %d, destello %.2f"), Char()->GetHealth()->GetHealth(), Char()->GetDamageIndicators().Num(), Char()->GetDamageFlash());
			return FMath::IsNearlyEqual(Char()->GetHealth()->GetHealth(), 70.f, 0.5f) && Char()->GetDamageIndicators().Num() == 1;
		}, nullptr, 0.15f });

	Steps.Add({ TEXT("Regeneracion"), 6.0f, nullptr, nullptr,
		[this](FString& D)
		{
			D = FString::Printf(TEXT("salud %.1f (esperado 75: tope del segmento)"), Char()->GetHealth()->GetHealth());
			return FMath::IsNearlyEqual(Char()->GetHealth()->GetHealth(), 75.f, 0.5f);
		} });

	Steps.Add({ TEXT("Muerte"), 1.4f,
		[this, St]()
		{
			St->DeathSpot = Char()->GetActorLocation();
			FBLDamageInfo Info;
			Info.Amount = 500.f;
			Info.SourceLocation = Char()->GetActorLocation() + Char()->GetActorForwardVector() * 800.f;
			Info.Direction = -Char()->GetActorForwardVector();
			Char()->GetHealth()->ApplyDamage(Info);
		},
		[this, St](float T) { St->CameraDropZ = Char()->GetActorLocation().Z - Char()->GetCamera()->GetComponentLocation().Z; },
		[this, St](FString& D)
		{
			D = FString::Printf(TEXT("muerto=%d, camara %.0f cm bajo el centro de la capsula"), Char()->IsDead(), St->CameraDropZ);
			return Char()->IsDead() && St->CameraDropZ > 40.f;
		}, nullptr, 1.2f });

	Steps.Add({ TEXT("Reaparicion"), 3.4f, nullptr, nullptr,
		[this](FString& D)
		{
			const UBLCheckpointSubsystem* C = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>();
			const float Dist = C ? FVector::Dist2D(Char()->GetActorLocation(), C->GetRespawnTransform().GetLocation()) : 1e6f;
			D = FString::Printf(TEXT("muerto=%d salud %.0f, a %.0f cm del checkpoint, municion %d/%d"), Char()->IsDead(), Char()->GetHealth()->GetHealth(), Dist,
				Char()->GetWeapon()->GetMagazine(), Char()->GetWeapon()->GetReserve());
			return !Char()->IsDead() && Char()->GetHealth()->GetHealth() >= 99.f && Dist < 50.f;
		}, nullptr, 3.3f });
}
