#include "Animation/BLFirstPersonAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TwoBoneIK.h"

namespace
{
	const FName CurveDisableLHandIK(TEXT("DisableLHandIK"));
	const FName BoneUpperArmL(TEXT("upperarm_l"));
	const FName BoneLowerArmL(TEXT("lowerarm_l"));
	const FName BoneHandL(TEXT("hand_l"));
	const FName BoneHandR(TEXT("hand_r"));

	FCompactPoseBoneIndex FindBone(const FBoneContainer& Bones, FName Name)
	{
		const int32 MeshIndex = Bones.GetPoseBoneIndexForBoneName(Name);
		return MeshIndex == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
	}
}

// ---------------------------------------------------------------------------
// Instancia (hilo de juego)
// ---------------------------------------------------------------------------

void UBLFirstPersonAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Sockets de agarre del Mannequin: relación mano -> arma
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

void UBLFirstPersonAnimInstance::SetBaseAnim(UAnimSequence* Anim)
{
	if (BaseAnim != Anim)
	{
		BaseAnim = Anim;
		BaseTime = 0.f;
	}
}

void UBLFirstPersonAnimInstance::PlayAction(UAnimSequence* Anim, float Duration, float BlendIn, float BlendOut)
{
	if (!Anim)
	{
		return;
	}
	ActionAnim = Anim;
	ActionTime = 0.f;
	ActionRate = Duration > 0.f ? Anim->GetPlayLength() / Duration : 1.f;
	ActionBlendIn = FMath::Max(BlendIn, 0.01f);
	ActionBlendOut = FMath::Max(BlendOut, 0.01f);
	bActionStopping = false;
}

void UBLFirstPersonAnimInstance::StopAction(float BlendOut)
{
	if (ActionAnim)
	{
		bActionStopping = true;
		ActionBlendOut = FMath::Max(BlendOut, 0.01f);
	}
}

void UBLFirstPersonAnimInstance::SetLeftHandGrip(const FTransform& GripInWeapon, bool bValid)
{
	State.LeftGripInWeapon = GripInWeapon;
	bHasGrip = bValid;
}

void UBLFirstPersonAnimInstance::SetWeaponTransform(const FTransform& WeaponInComponent, bool bValid)
{
	State.WeaponInComponent = WeaponInComponent;
	State.bHasWeapon = bValid;
}

void UBLFirstPersonAnimInstance::SetLeftHandOverride(float Alpha, const FVector& LocationInWeapon)
{
	State.LeftHandOverrideAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	State.LeftHandOverrideLocation = LocationInWeapon;
}

void UBLFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (BaseAnim)
	{
		BaseTime = FMath::Fmod(BaseTime + DeltaSeconds, FMath::Max(BaseAnim->GetPlayLength(), 0.01f));
	}

	float DisableIK = 0.f;
	if (ActionAnim)
	{
		const float Length = ActionAnim->GetPlayLength();
		ActionTime = FMath::Min(ActionTime + DeltaSeconds * ActionRate, Length);
		const float RealRemaining = (Length - ActionTime) / FMath::Max(ActionRate, 0.01f);
		if (!bActionStopping && RealRemaining <= ActionBlendOut)
		{
			bActionStopping = true; // empezar a salir antes del final para que no haya corte
		}

		if (bActionStopping)
		{
			ActionWeight -= DeltaSeconds / ActionBlendOut;
		}
		else
		{
			ActionWeight += DeltaSeconds / ActionBlendIn;
		}
		ActionWeight = FMath::Clamp(ActionWeight, 0.f, 1.f);

		DisableIK = ActionAnim->EvaluateCurveData(CurveDisableLHandIK, FAnimExtractContext(double(ActionTime))) * ActionWeight;

		if (bActionStopping && ActionWeight <= 0.f)
		{
			ActionAnim = nullptr;
			bActionStopping = false;
		}
	}
	else
	{
		ActionWeight = 0.f;
	}

	// La IK sigue a la curva con suavizado para que la mano no salte
	const float TargetIK = bHasGrip ? FMath::Clamp(1.f - DisableIK, 0.f, 1.f) : 0.f;
	LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, TargetIK, DeltaSeconds, 18.f);

	State.BaseAnim = BaseAnim;
	State.BaseTime = BaseTime;
	State.ActionAnim = ActionAnim;
	State.ActionTime = ActionTime;
	State.ActionWeight = ActionWeight;
	State.LeftHandIKAlpha = LeftHandIKAlpha;
}

FAnimInstanceProxy* UBLFirstPersonAnimInstance::CreateAnimInstanceProxy()
{
	return new FBLFirstPersonAnimProxy(this);
}

void UBLFirstPersonAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

// ---------------------------------------------------------------------------
// Proxy (hilo de animación)
// ---------------------------------------------------------------------------

void FBLFirstPersonAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	State = CastChecked<UBLFirstPersonAnimInstance>(InAnimInstance)->GetState();
}

bool FBLFirstPersonAnimProxy::Evaluate(FPoseContext& Output)
{
	if (State.BaseAnim)
	{
		FAnimationPoseData BaseData(Output);
		State.BaseAnim->GetAnimationPose(BaseData, FAnimExtractContext(double(State.BaseTime), false, {}, true));
	}
	else
	{
		Output.ResetToRefPose();
	}

	if (State.ActionAnim && State.ActionWeight > ZERO_ANIMWEIGHT_THRESH)
	{
		FPoseContext ActionPose(Output);
		FAnimationPoseData ActionData(ActionPose);
		State.ActionAnim->GetAnimationPose(ActionData, FAnimExtractContext(double(State.ActionTime)));

		// Solo brazos (clavículas e hijos): el torso se queda en la pose base. Las animaciones de
		// acción son de tercera persona y doblan la espalda; en primera persona eso saca el arma de la vista.
		const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
		const FCompactPoseBoneIndex ClavL = FindBone(Bones, TEXT("clavicle_l"));
		const FCompactPoseBoneIndex ClavR = FindBone(Bones, TEXT("clavicle_r"));
		const float W = State.ActionWeight;
		for (const FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
		{
			bool bArm = false;
			for (FCompactPoseBoneIndex B = BoneIndex; B != INDEX_NONE; B = Bones.GetParentBoneIndex(B))
			{
				if (B == ClavL || B == ClavR)
				{
					bArm = true;
					break;
				}
			}
			if (bArm)
			{
				FTransform Blended;
				Blended.Blend(Output.Pose[BoneIndex], ActionPose.Pose[BoneIndex], W);
				Output.Pose[BoneIndex] = Blended;
			}
		}
	}

	if (State.bHasWeapon || State.LeftHandIKAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		ApplyHandIK(Output);
	}
	return true;
}

namespace
{
	/** IK de dos huesos de un brazo hacia DesiredHand (posición y rotación), mezclado por Alpha. */
	void SolveArm(FCSPose<FCompactPose>& CSPose, FCompactPoseBoneIndex Upper, FCompactPoseBoneIndex Lower, FCompactPoseBoneIndex Hand,
		const FTransform& DesiredHand, float Alpha, TArray<FBoneTransform, TInlineAllocator<6>>& Out)
	{
		const FTransform UpperIn = CSPose.GetComponentSpaceTransform(Upper);
		const FTransform LowerIn = CSPose.GetComponentSpaceTransform(Lower);
		const FTransform HandIn = CSPose.GetComponentSpaceTransform(Hand);

		FTransform UpperOut = UpperIn, LowerOut = LowerIn, HandOut = HandIn;
		// Polo del codo: se conserva la dirección del codo de la animación
		AnimationCore::SolveTwoBoneIK(UpperOut, LowerOut, HandOut, LowerIn.GetLocation(), DesiredHand.GetLocation(), false, 1.0, 1.0);
		HandOut.SetRotation(DesiredHand.GetRotation());

		FTransform UpperB, LowerB, HandB;
		UpperB.Blend(UpperIn, UpperOut, Alpha);
		LowerB.Blend(LowerIn, LowerOut, Alpha);
		HandB.Blend(HandIn, HandOut, Alpha);
		Out.Add(FBoneTransform(Upper, UpperB));
		Out.Add(FBoneTransform(Lower, LowerB));
		Out.Add(FBoneTransform(Hand, HandB));
	}
}

void FBLFirstPersonAnimProxy::ApplyHandIK(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex UpperL = FindBone(Bones, BoneUpperArmL);
	const FCompactPoseBoneIndex LowerL = FindBone(Bones, BoneLowerArmL);
	const FCompactPoseBoneIndex HandL = FindBone(Bones, BoneHandL);
	const FCompactPoseBoneIndex UpperR = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerR = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex HandR = FindBone(Bones, BoneHandR);
	if (UpperL == INDEX_NONE || LowerL == INDEX_NONE || HandL == INDEX_NONE
		|| UpperR == INDEX_NONE || LowerR == INDEX_NONE || HandR == INDEX_NONE)
	{
		return;
	}

	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(Output.Pose);

	// Arma: la coloca el rig (si no, cuelga del socket HandGrip_R de la animación)
	const FTransform WeaponCS = State.bHasWeapon
		? State.WeaponInComponent
		: State.HandGripRLocal * CSPose.GetComponentSpaceTransform(HandR);

	TArray<FBoneTransform, TInlineAllocator<6>> Result;
	if (State.bHasWeapon)
	{
		// Mano derecha: su socket HandGrip_R coincide con el origen del arma (pistolete)
		SolveArm(CSPose, UpperR, LowerR, HandR, State.HandGripRLocal.Inverse() * WeaponCS, 1.f, Result);
	}
	if (State.LeftHandIKAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		FTransform GripInWeapon = State.LeftGripInWeapon;
		GripInWeapon.SetLocation(FMath::Lerp(GripInWeapon.GetLocation(), State.LeftHandOverrideLocation, State.LeftHandOverrideAlpha));
		// La mano se coloca para que su socket HandGrip_L coincida con el agarre
		SolveArm(CSPose, UpperL, LowerL, HandL, State.HandGripLLocal.Inverse() * (GripInWeapon * WeaponCS), State.LeftHandIKAlpha, Result);
	}
	if (Result.Num() == 0)
	{
		return;
	}
	Result.Sort([](const FBoneTransform& A, const FBoneTransform& B) { return A.BoneIndex < B.BoneIndex; });
	CSPose.SafeSetCSBoneTransforms(Result);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CSPose), Output.Pose);
}
