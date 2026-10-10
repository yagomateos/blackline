#include "Animation/BLWeaponAnimInstance.h"

#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

// ---------------------------------------------------------------------------
// Instancia (hilo de juego)
// ---------------------------------------------------------------------------

namespace
{
	// Bombeo de la corredera (fracciones del ciclo); los sonidos del arma (CycleSounds) van a juego
	constexpr float PumpBackStart = 0.18f;
	constexpr float PumpBackEnd = 0.45f;
	constexpr float PumpForwardEnd = 0.75f;
}

UBLWeaponComponent* UBLWeaponAnimInstance::FindWeaponComponent() const
{
	const USkeletalMeshComponent* Comp = GetSkelMeshComponent();
	const AActor* Owner = Comp ? Comp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<UBLWeaponComponent>() : nullptr;
}

void UBLWeaponAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const UBLWeaponComponent* Weapon = FindWeaponComponent();
	const UBLWeaponData* Data = Weapon ? Weapon->GetWeaponData() : nullptr;
	if (!Data)
	{
		State = FBLWeaponMechState();
		return;
	}
	State.BoltBone = Data->BoltBone;
	State.TriggerBone = Data->TriggerBone;

	// Nuevo disparo: empieza un ciclo del cerrojo (más corto si la cadencia no da tiempo)
	const int32 Shots = Weapon->GetShotsFired();
	const bool bPump = Data->FireMode == EBLFireMode::Pump;
	if (LastShotsFired != INDEX_NONE && Shots != LastShotsFired)
	{
		CycleTime = 0.f;
		// Corredera: el bombeo ocupa casi todo el intervalo entre disparos (no hay muelle que lo acorte)
		CycleDuration = bPump ? Data->GetShotInterval() * 0.92f : FMath::Min(Data->BoltCycleTime, Data->GetShotInterval() * 0.95f);
	}
	LastShotsFired = Shots;
	CycleTime += DeltaSeconds;

	// Cerrojo abierto: cargador vacío y sin recámara; en la recarga en vacío se suelta al golpear la retenida
	// (corredera: la escopeta vacía se queda con el guardamanos atrás tras el último bombeo)
	const float OpenAt = bPump ? PumpBackEnd : 0.35f;
	if (Weapon->IsReloading())
	{
		bLockedOpen = Weapon->IsReloadActionOpen();
	}
	else
	{
		bLockedOpen = Weapon->GetMagazine() == 0 && CycleTime >= CycleDuration * OpenAt;
	}

	float Cycle = 0.f;
	if (CycleTime < CycleDuration)
	{
		const float T = CycleTime / CycleDuration;
		if (bPump)
		{
			// Bombeo a mano: espera a que pase el retroceso, atrás con decisión y adelante algo más rápido
			Cycle = T < PumpBackStart ? 0.f
				: T < PumpBackEnd ? FMath::SmoothStep(PumpBackStart, PumpBackEnd, T)
				: 1.f - FMath::SmoothStep(PumpBackEnd + 0.04f, PumpForwardEnd, T);
		}
		else
		{
			// Ciclo: retrocede rápido (35 %) y vuelve empujado por el muelle
			Cycle = T < 0.35f ? FMath::Sin(HALF_PI * T / 0.35f) : 1.f - FMath::Square((T - 0.35f) / 0.65f);
		}
	}
	if (bLockedOpen)
	{
		BoltAlpha = 1.f;
	}
	else if (BoltAlpha > Cycle && CycleTime >= CycleDuration)
	{
		BoltAlpha = FMath::Max(Cycle, BoltAlpha - DeltaSeconds / 0.05f);  // al soltarse, cierra de golpe
	}
	else
	{
		BoltAlpha = Cycle;
	}

	const float TriggerTarget = Weapon->IsTriggerHeld() && !Weapon->IsReloading() ? 1.f : 0.f;
	TriggerAlpha = FMath::FInterpTo(TriggerAlpha, TriggerTarget, DeltaSeconds, 30.f);

	const FVector Back = -Data->ForwardAxis.GetSafeNormal();
	State.BoltOffset = Back * Data->BoltTravel * BoltAlpha;
	State.TriggerOffset = Back * Data->TriggerTravel * TriggerAlpha;
}

FAnimInstanceProxy* UBLWeaponAnimInstance::CreateAnimInstanceProxy()
{
	return new FBLWeaponAnimProxy(this);
}

void UBLWeaponAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

// ---------------------------------------------------------------------------
// Proxy (hilo de animación)
// ---------------------------------------------------------------------------

void FBLWeaponAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	State = CastChecked<UBLWeaponAnimInstance>(InAnimInstance)->GetMechState();
}

bool FBLWeaponAnimProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	if (State.BoltOffset.IsNearlyZero() && State.TriggerOffset.IsNearlyZero())
	{
		return true;
	}

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(Output.Pose);

	// Los huesos exportados desde Blender traen rotación/escala propias: se desplazan en espacio de la malla
	TArray<FBoneTransform, TInlineAllocator<2>> Result;
	auto Move = [&](FName Bone, const FVector& Offset)
	{
		const int32 MeshIndex = Bone.IsNone() ? INDEX_NONE : Bones.GetPoseBoneIndexForBoneName(Bone);
		if (MeshIndex == INDEX_NONE || Offset.IsNearlyZero())
		{
			return;
		}
		const FCompactPoseBoneIndex Index = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
		if (Index == INDEX_NONE)
		{
			return;
		}
		FTransform CS = CSPose.GetComponentSpaceTransform(Index);
		CS.AddToTranslation(Offset);
		Result.Add(FBoneTransform(Index, CS));
	};
	Move(State.BoltBone, State.BoltOffset);
	Move(State.TriggerBone, State.TriggerOffset);
	if (Result.Num() > 0)
	{
		Result.Sort([](const FBoneTransform& A, const FBoneTransform& B) { return A.BoneIndex < B.BoneIndex; });
		CSPose.SafeSetCSBoneTransforms(Result);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CSPose), Output.Pose);
	}
	return true;
}
