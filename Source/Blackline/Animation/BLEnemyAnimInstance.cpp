#include "Animation/BLEnemyAnimInstance.h"

#include "AI/BLEnemyCharacter.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TwoBoneIK.h"

namespace
{
	/** Velocidades a las que las animaciones de Epic no patinan (cm/s, aprox.). */
	constexpr float WalkAnimSpeed = 165.f;
	constexpr float JogAnimSpeed = 375.f;
	/** Cuánto baja la pelvis al agacharse (cm). */
	constexpr float CrouchDrop = 42.f;

	FCompactPoseBoneIndex FindEnemyBone(const FBoneContainer& Bones, FName Name)
	{
		// Por el esqueleto (no por índice de malla): en la build cocinada el contenedor puede no incluir todos los huesos
		const USkeleton* Skel = Bones.GetSkeletonAsset();
		const int32 SkelIndex = Skel ? Skel->GetReferenceSkeleton().FindBoneIndex(Name) : INDEX_NONE;
		return SkelIndex == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.GetCompactPoseIndexFromSkeletonIndex(SkelIndex);
	}

	void Sample(const UAnimSequence* Anim, float Time, FPoseContext& Out)
	{
		FAnimationPoseData Data(Out);
		Anim->GetAnimationPose(Data, FAnimExtractContext(double(Time), false));
	}

	void BlendInto(FPoseContext& Out, const FPoseContext& Other, float W)
	{
		for (const FCompactPoseBoneIndex I : Out.Pose.ForEachBoneIndex())
		{
			FTransform T;
			T.Blend(Out.Pose[I], Other.Pose[I], W);
			Out.Pose[I] = T;
		}
	}
}

// ---------------------------------------------------------------------------
// Hilo de juego
// ---------------------------------------------------------------------------

void UBLEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (const USkeletalMeshComponent* Comp = GetSkelMeshComponent())
	{
		if (const USkeletalMesh* Mesh = Comp->GetSkeletalMeshAsset())
		{
			if (const USkeletalMeshSocket* R = Mesh->FindSocket(TEXT("HandGrip_R")))
			{
				State.HandGripRLocal = R->GetSocketLocalTransform();
			}
			if (const USkeletalMeshSocket* L = Mesh->FindSocket(TEXT("HandGrip_L")))
			{
				State.HandGripLLocal = L->GetSocketLocalTransform();
			}
		}
	}
}

void UBLEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const ABLEnemyCharacter* Enemy = Cast<ABLEnemyCharacter>(TryGetPawnOwner());
	if (!Enemy || Enemy->WalkAnims.Num() != 8 || Enemy->JogAnims.Num() != 8)
	{
		return;
	}
	State.Idle = Enemy->IdleAnim;
	State.IdleTime = FMath::Fmod(State.IdleTime + DeltaSeconds, State.Idle ? FMath::Max(State.Idle->GetPlayLength(), 0.1f) : 1.f);

	// Dirección de la marcha respecto a donde mira el cuerpo: 0 = delante, 90 = derecha
	const FVector Vel = Enemy->GetVelocity();
	const float Speed = Vel.Size2D();
	const FVector Local = Enemy->GetActorTransform().InverseTransformVectorNoScale(Vel);
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
	const float Slot = FMath::Fmod(Angle + 360.f, 360.f) / 45.f;   // 0..8
	const int32 A = FMath::FloorToInt(Slot) % 8;
	const int32 B = (A + 1) % 8;
	State.DirBlend = Slot - FMath::FloorToFloat(Slot);
	State.WalkA = Enemy->WalkAnims[A];
	State.WalkB = Enemy->WalkAnims[B];
	State.JogA = Enemy->JogAnims[A];
	State.JogB = Enemy->JogAnims[B];
	State.JogWeight = FMath::GetMappedRangeValueClamped(FVector2D(200.f, 330.f), FVector2D(0.f, 1.f), Speed);
	const float TargetMove = FMath::GetMappedRangeValueClamped(FVector2D(15.f, 90.f), FVector2D(0.f, 1.f), Speed);
	State.MoveWeight = FMath::FInterpTo(State.MoveWeight, TargetMove, DeltaSeconds, 10.f);
	// Fase común: avanza según la velocidad real respecto a la de la animación (sin patinar)
	if (State.WalkA)
	{
		const float RefSpeed = FMath::Lerp(WalkAnimSpeed, JogAnimSpeed, State.JogWeight);
		const float CycleLength = FMath::Lerp(State.WalkA->GetPlayLength(), State.JogA ? State.JogA->GetPlayLength() : 1.f, State.JogWeight);
		State.LocoPhase = FMath::Fmod(State.LocoPhase + DeltaSeconds * FMath::Max(Speed, 40.f) / RefSpeed / FMath::Max(CycleLength, 0.1f), 1.f);
	}

	// Apuntar en vertical
	const FVector To = Enemy->GetAimPoint() - Enemy->GetEyeLocation();
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(To.Z, To.Size2D()));
	State.AimPitch = FMath::FInterpTo(State.AimPitch, Enemy->IsAiming() ? FMath::Clamp(Pitch, -50.f, 50.f) : -8.f, DeltaSeconds, 8.f);
	State.CrouchAlpha = Enemy->GetCrouchAlpha();

	// Recarga (parte superior) y mano izquierda
	State.Reload = Enemy->GetReloadAnim();
	const float Dur = Enemy->GetReloadDuration();
	const float Left = Enemy->GetReloadTimeLeft();
	const bool bReloading = Left > 0.f && State.Reload;
	State.ReloadTime = bReloading ? (1.f - Left / FMath::Max(Dur, 0.01f)) * State.Reload->GetPlayLength() : 0.f;
	State.ReloadWeight = FMath::FInterpTo(State.ReloadWeight, bReloading ? 1.f : 0.f, DeltaSeconds, 8.f);
	State.LeftHandAlpha = 1.f - State.ReloadWeight;

	// Retroceso: muelle amortiguado
	KickVelocity += Enemy->GetWeapon() ? const_cast<ABLEnemyCharacter*>(Enemy)->ConsumeFireKick() * 40.f : 0.f;
	KickVelocity += (-180.f * KickValue - 18.f * KickVelocity) * DeltaSeconds;
	KickValue += KickVelocity * DeltaSeconds;
	State.Kick = KickValue;

	const UBLWeaponData* Data = Enemy->GetWeapon() ? Enemy->GetWeapon()->GetWeaponData() : nullptr;
	State.bHasWeapon = Data && Enemy->GetWeaponMesh() && Enemy->GetWeaponMesh()->GetSkeletalMeshAsset() && !Enemy->IsDead();
	State.bReady = true;
	if (State.bHasWeapon)
	{
		const USkeletalMesh* WM = Enemy->GetWeaponMesh()->GetSkeletalMeshAsset();
		const int32 Bone = WM->GetRefSkeleton().FindBoneIndex(Data->LeftHandSocket);
		if (Bone != INDEX_NONE)
		{
			State.GripInWeapon = FAnimationRuntime::GetComponentSpaceTransformRefPose(WM->GetRefSkeleton(), Bone).GetLocation();
		}
	}
}

FAnimInstanceProxy* UBLEnemyAnimInstance::CreateAnimInstanceProxy()
{
	return new FBLEnemyAnimProxy(this);
}

void UBLEnemyAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

// ---------------------------------------------------------------------------
// Hilo de animación
// ---------------------------------------------------------------------------

void FBLEnemyAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	State = CastChecked<UBLEnemyAnimInstance>(InAnimInstance)->GetState();
}

bool FBLEnemyAnimProxy::Evaluate(FPoseContext& Output)
{
	if (State.Idle)
	{
		Sample(State.Idle, State.IdleTime, Output);
	}
	else
	{
		Output.ResetToRefPose();
	}

	if (State.MoveWeight > 0.01f && State.WalkA && State.WalkB && State.JogA && State.JogB)
	{
		auto Directional = [this](UAnimSequence* A, UAnimSequence* B, FPoseContext& Out)
		{
			Sample(A, State.LocoPhase * A->GetPlayLength(), Out);
			if (State.DirBlend > 0.01f)
			{
				FPoseContext Second(Out);
				Sample(B, State.LocoPhase * B->GetPlayLength(), Second);
				BlendInto(Out, Second, State.DirBlend);
			}
		};
		FPoseContext Loco(Output);
		if (State.JogWeight < 0.99f)
		{
			Directional(State.WalkA, State.WalkB, Loco);
		}
		if (State.JogWeight > 0.01f)
		{
			FPoseContext Jog(Output);
			Directional(State.JogA, State.JogB, Jog);
			if (State.JogWeight >= 0.99f)
			{
				Loco.Pose.CopyBonesFrom(Jog.Pose);
			}
			else
			{
				BlendInto(Loco, Jog, State.JogWeight);
			}
		}
		BlendInto(Output, Loco, State.MoveWeight);
	}

	// Recarga: solo de la columna hacia arriba
	if (State.Reload && State.ReloadWeight > 0.01f)
	{
		FPoseContext ReloadPose(Output);
		Sample(State.Reload, State.ReloadTime, ReloadPose);
		const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
		const FCompactPoseBoneIndex Spine = FindEnemyBone(Bones, TEXT("spine_01"));
		for (const FCompactPoseBoneIndex I : Output.Pose.ForEachBoneIndex())
		{
			bool bUpper = false;
			for (FCompactPoseBoneIndex B = I; B != INDEX_NONE; B = Bones.GetParentBoneIndex(B))
			{
				if (B == Spine) { bUpper = true; break; }
			}
			if (bUpper)
			{
				FTransform T;
				T.Blend(Output.Pose[I], ReloadPose.Pose[I], State.ReloadWeight);
				Output.Pose[I] = T;
			}
		}
	}

	ApplyProcedural(Output);
	return true;
}

void FBLEnemyAnimProxy::ApplyProcedural(FPoseContext& Output) const
{
	if (!State.bReady)
	{
		return;
	}
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex Pelvis = FindEnemyBone(Bones, TEXT("pelvis"));
	if (Pelvis == INDEX_NONE)
	{
		return;
	}
	FCSPose<FCompactPose> CS;
	CS.InitPose(Output.Pose);
	auto SetCS = [&CS](FCompactPoseBoneIndex I, const FTransform& T)
	{
		TArray<FBoneTransform> One;
		One.Add(FBoneTransform(I, T));
		CS.SafeSetCSBoneTransforms(One);
	};
	// En la malla del Mannequin: +Y = delante del personaje, +X = su izquierda, +Z arriba
	const FVector Forward(0.f, 1.f, 0.f);

	// ---- Agacharse: la pelvis baja y las piernas se doblan por IK manteniendo los pies ----
	if (State.CrouchAlpha > 0.01f)
	{
		struct FLeg { FName Thigh, Calf, Foot; };
		const FLeg Legs[2] = { { "thigh_l", "calf_l", "foot_l" }, { "thigh_r", "calf_r", "foot_r" } };
		FTransform FeetBefore[2];
		for (int32 k = 0; k < 2; ++k)
		{
			const FCompactPoseBoneIndex Foot = FindEnemyBone(Bones, Legs[k].Foot);
			FeetBefore[k] = Foot != INDEX_NONE ? CS.GetComponentSpaceTransform(Foot) : FTransform::Identity;
		}
		FTransform P = CS.GetComponentSpaceTransform(Pelvis);
		P.AddToTranslation(FVector(0.f, -6.f, -CrouchDrop) * State.CrouchAlpha);
		SetCS(Pelvis, P);
		for (int32 k = 0; k < 2; ++k)
		{
			const FCompactPoseBoneIndex T = FindEnemyBone(Bones, Legs[k].Thigh), C = FindEnemyBone(Bones, Legs[k].Calf), F = FindEnemyBone(Bones, Legs[k].Foot);
			if (T == INDEX_NONE || C == INDEX_NONE || F == INDEX_NONE)
			{
				continue;
			}
			FTransform TT = CS.GetComponentSpaceTransform(T), CC = CS.GetComponentSpaceTransform(C), FF = CS.GetComponentSpaceTransform(F);
			const FVector Knee = CC.GetLocation() + Forward * 60.f;   // las rodillas se doblan hacia delante
			AnimationCore::SolveTwoBoneIK(TT, CC, FF, Knee, FeetBefore[k].GetLocation(), false, 1.0, 1.0);
			FF.SetRotation(FeetBefore[k].GetRotation());
			TArray<FBoneTransform> Leg;
			Leg.Add(FBoneTransform(T, TT));
			Leg.Add(FBoneTransform(C, CC));
			Leg.Add(FBoneTransform(F, FF));
			CS.SafeSetCSBoneTransforms(Leg);
		}
	}

	// ---- Torso: apuntar en vertical + inclinarse al agacharse + retroceso ----
	const float TorsoPitch = State.AimPitch - 10.f * State.CrouchAlpha + State.Kick * 6.f;
	for (const TCHAR* Name : { TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03") })
	{
		const FCompactPoseBoneIndex S = FindEnemyBone(Bones, Name);
		if (S == INDEX_NONE)
		{
			continue;
		}
		FTransform T = CS.GetComponentSpaceTransform(S);
		T.SetRotation(FQuat(FVector(1.f, 0.f, 0.f), FMath::DegreesToRadians(TorsoPitch / 3.f)) * T.GetRotation());
		SetCS(S, T);
	}

	// ---- Mano izquierda al guardamanos del arma (el arma va en el socket HandGrip_R de la mano derecha) ----
	const FCompactPoseBoneIndex HandR = FindEnemyBone(Bones, TEXT("hand_r"));
	const FCompactPoseBoneIndex UpperL = FindEnemyBone(Bones, TEXT("upperarm_l")), LowerL = FindEnemyBone(Bones, TEXT("lowerarm_l")), HandL = FindEnemyBone(Bones, TEXT("hand_l"));
	if (State.bHasWeapon && State.LeftHandAlpha > 0.01f && HandR != INDEX_NONE && UpperL != INDEX_NONE && LowerL != INDEX_NONE && HandL != INDEX_NONE)
	{
		const FTransform WeaponCS = State.HandGripRLocal * CS.GetComponentSpaceTransform(HandR);
		const FTransform Desired = State.HandGripLLocal.Inverse() * (FTransform(State.GripInWeapon) * WeaponCS);
		FTransform U = CS.GetComponentSpaceTransform(UpperL), L = CS.GetComponentSpaceTransform(LowerL), H = CS.GetComponentSpaceTransform(HandL);
		const FTransform U0 = U, L0 = L, H0 = H;
		AnimationCore::SolveTwoBoneIK(U, L, H, L0.GetLocation(), Desired.GetLocation(), false, 1.0, 1.0);
		H.SetRotation(Desired.GetRotation());
		FTransform UB, LB, HB;
		UB.Blend(U0, U, State.LeftHandAlpha);
		LB.Blend(L0, L, State.LeftHandAlpha);
		HB.Blend(H0, H, State.LeftHandAlpha);
		TArray<FBoneTransform> Arm;
		Arm.Add(FBoneTransform(UpperL, UB));
		Arm.Add(FBoneTransform(LowerL, LB));
		Arm.Add(FBoneTransform(HandL, HB));
		CS.SafeSetCSBoneTransforms(Arm);
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS), Output.Pose);
}
