#include "Player/BLFirstPersonRigComponent.h"

#include "Player/BLCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	FVector RotToVec(const FRotator& R) { return FVector(R.Pitch, R.Yaw, R.Roll); }
	FRotator VecToRot(const FVector& V) { return FRotator(V.X, V.Y, V.Z); }
}

UBLFirstPersonRigComponent::UBLFirstPersonRigComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // lo actualiza el personaje
}

void UBLFirstPersonRigComponent::Initialize(ABLCharacter* InCharacter, UCameraComponent* InCamera, USceneComponent* InWeaponRoot, USceneComponent* InWeaponMesh)
{
	Character = InCharacter;
	Camera = InCamera;
	WeaponRoot = InWeaponRoot;
	WeaponMesh = InWeaponMesh;
	bHasCalibration = false;
}

void UBLFirstPersonRigComponent::UpdateAimCalibration(float DeltaTime)
{
	USceneComponent* Root = WeaponRoot.Get();
	USceneComponent* Weapon = WeaponMesh.Get();
	if (!Root || !Weapon)
	{
		return;
	}

	if (bCalibrationFrozen && bHasCalibration)
	{
		return;
	}

	// Marco de la mira en espacio de la malla del arma: X = adelante del arma, Z = arriba.
	// Del socket solo se usa la posición: la orientación de los huesos exportados no es fiable.
	const FQuat Axes = FRotationMatrix::MakeFromXZ(WeaponForwardAxis, FVector::UpVector).ToQuat();
	const FVector SightLoc = Weapon->DoesSocketExist(SightSocket)
		? Weapon->GetSocketTransform(SightSocket, RTS_Component).GetLocation()
		: SightLocalOffset;
	const FTransform SightInWeapon(Axes, SightLoc);

	// Mira relativa a WeaponRoot (depende solo de la animación de la malla FP, no de WeaponRoot)
	const FTransform WeaponToRoot = Weapon->GetComponentTransform().GetRelativeTransform(Root->GetComponentTransform());
	SightInWeaponRoot = SightInWeapon * WeaponToRoot;

	// Queremos: SightInWeaponRoot * W = Objetivo (mira en el eje de la cámara a AimDistance)
	const FTransform Target(FQuat::Identity, FVector(AimDistance, 0.f, 0.f));
	const FTransform Desired = SightInWeaponRoot.Inverse() * Target;

	if (!bHasCalibration)
	{
		CalibratedAimTransform = Desired;
		bHasCalibration = true;
	}
	else
	{
		// Suavizado: absorbe el movimiento de respiración del idle para que el ADS sea estable
		const float Alpha = 1.f - FMath::Exp(-12.f * DeltaTime);
		CalibratedAimTransform.SetLocation(FMath::Lerp(CalibratedAimTransform.GetLocation(), Desired.GetLocation(), Alpha));
		CalibratedAimTransform.SetRotation(FQuat::Slerp(CalibratedAimTransform.GetRotation(), Desired.GetRotation(), Alpha));
	}
}

void UBLFirstPersonRigComponent::ConfigureWeaponSight(FName InSightSocket, const FVector& InSightLocalOffset, const FVector& InForwardAxis, float InAimDistance)
{
	AimDistance = InAimDistance;
	SightSocket = InSightSocket;
	SightLocalOffset = InSightLocalOffset;
	WeaponForwardAxis = InForwardAxis.GetSafeNormal();
	bHasCalibration = false;
}

void UBLFirstPersonRigComponent::NotifyLanded(float VerticalSpeed)
{
	const float Speed = -VerticalSpeed;
	if (Speed < LandingMinSpeed)
	{
		return;
	}
	const float Impulse = FMath::Min(Speed * LandingDipScale, 14.f);
	CameraOffsetSpring.AddImpulse(FVector(0.f, 0.f, -Impulse * 60.f));
	WeaponKickLocSpring.AddImpulse(FVector(0.f, 0.f, -Impulse * 80.f));
	WeaponKickRotSpring.AddImpulse(FVector(-Impulse * 25.f, 0.f, 0.f));
	CameraKickSpring.AddImpulse(FVector(-Impulse * 6.f, 0.f, 0.f));
}

void UBLFirstPersonRigComponent::NotifyJumped()
{
	WeaponKickLocSpring.AddImpulse(FVector(0.f, 0.f, -35.f));
	WeaponKickRotSpring.AddImpulse(FVector(12.f, 0.f, 0.f));
}

void UBLFirstPersonRigComponent::NotifyMantleStarted(float HeightAlpha)
{
	CameraKickSpring.AddImpulse(FVector(-20.f - 20.f * HeightAlpha, 0.f, 10.f));
}

void UBLFirstPersonRigComponent::AddCameraKick(const FRotator& Kick)
{
	CameraKickSpring.AddImpulse(RotToVec(Kick));
}

void UBLFirstPersonRigComponent::AddWeaponKick(const FVector& Location, const FRotator& Rotation)
{
	WeaponKickLocSpring.AddImpulse(Location);
	WeaponKickRotSpring.AddImpulse(RotToVec(Rotation));
}

void UBLFirstPersonRigComponent::NotifyShot(float Strength)
{
	const float S = ShotCameraShake * Strength;
	CameraShakeSpring.AddImpulse(FVector(FMath::FRandRange(0.3f, 1.f) * S, FMath::FRandRange(-1.f, 1.f) * S * 0.6f, FMath::FRandRange(-1.f, 1.f) * S * 1.2f));
	FovSpring.AddImpulse(FVector(-ShotFovPunch * 25.f * Strength, 0.f, 0.f));
}

void UBLFirstPersonRigComponent::NotifyCrouchChanged(bool bCrouched)
{
	WeaponKickLocSpring.AddImpulse(FVector(0.f, 0.f, bCrouched ? -28.f : 18.f));
	WeaponKickRotSpring.AddImpulse(FVector(bCrouched ? -10.f : 6.f, 0.f, bCrouched ? 6.f : -4.f));
}

void UBLFirstPersonRigComponent::UpdateRig(float DeltaTime)
{
	ABLCharacter* Char = Character.Get();
	UCameraComponent* Cam = Camera.Get();
	USceneComponent* Weapon = WeaponRoot.Get();
	if (!Char || !Cam || !Weapon || DeltaTime <= 0.f)
	{
		return;
	}

	const UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	const FVector Velocity = Move->Velocity;
	const float GroundSpeed = Velocity.Size2D();
	const bool bGrounded = Move->IsMovingOnGround();
	const float Aim = Char->GetAimAlpha();
	const float Sprint = Char->GetSprintAlpha();
	const float Mantle = Char->GetMantleAlpha();

	// ------------------------------------------------------------------ Bob
	const float TargetBobWeight = (bGrounded && !Char->IsMantling()) ? FMath::Clamp(GroundSpeed / 420.f, 0.f, 1.6f) : 0.f;
	BobWeight = BLExpInterp(BobWeight, TargetBobWeight, DeltaTime, 10.f);

	const float Stride = FMath::Lerp(WalkStride, SprintStride, Sprint);
	if (bGrounded && GroundSpeed > 10.f)
	{
		// Una vuelta completa de fase (2π) = dos pasos
		const float PrevPhase = BobPhase;
		BobPhase += (GroundSpeed / Stride) * PI * DeltaTime;
		// Paso detectado cada vez que la fase cruza un múltiplo de π
		if (FMath::FloorToInt(BobPhase / PI) != FMath::FloorToInt(PrevPhase / PI))
		{
			bLastStepLeft = !bLastStepLeft;
			OnFootstep.Broadcast(bLastStepLeft, FMath::Clamp(GroundSpeed / 660.f, 0.2f, 1.f));
		}
		BobPhase = FMath::Fmod(BobPhase, 2.f * PI * 1000.f);
	}

	const float BobMult = BobWeight * FMath::Lerp(1.f, SprintBobMultiplier, Sprint) * FMath::Lerp(1.f, AimBobMultiplier, Aim);
	// Vertical: dos picos por ciclo (uno por paso). Horizontal: uno por ciclo.
	const float BobV = -FMath::Abs(FMath::Sin(BobPhase)) + 0.5f;
	const float BobH = FMath::Sin(BobPhase);

	// ------------------------------------------------------------------ Strafe
	const FVector LocalVel = Char->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	const float TargetStrafe = bGrounded ? FMath::Clamp(LocalVel.Y / 420.f, -1.f, 1.f) : 0.f;
	StrafeAlpha = BLExpInterp(StrafeAlpha, TargetStrafe, DeltaTime, 6.f);

	// ------------------------------------------------------------------ Cámara
	CameraOffsetSpring.Update(FVector::ZeroVector, DeltaTime, 3.5f, 0.55f);
	CameraKickSpring.Update(FVector::ZeroVector, DeltaTime, 5.f, 0.7f);
	CameraShakeSpring.Update(FVector::ZeroVector, DeltaTime, 13.f, 0.45f);
	FovSpring.Update(FVector::ZeroVector, DeltaTime, 7.f, 0.6f);

	const float CamScale = CameraMotionScale;
	FVector CamLoc = FVector(0.f, 0.f, BobV * CameraBobVertical * BobMult) + CameraOffsetSpring.Value;
	FRotator CamRot = VecToRot(CameraKickSpring.Value + CameraShakeSpring.Value);
	CamRot.Roll += BobH * CameraBobRoll * BobMult + StrafeAlpha * CameraStrafeRoll * (1.f - Aim * 0.7f);
	CamRot += ReloadCameraTilt * ReloadAlpha;
	Cam->SetRelativeLocationAndRotation(CamLoc * CamScale, CamRot * CamScale);

	const float TargetFOV = FMath::Lerp(BaseFOV + SprintFOVBonus * Sprint, AimFOV, Aim) + FovSpring.Value.X * CamScale;
	Cam->SetFieldOfView(TargetFOV);
	Cam->FirstPersonFieldOfView = FMath::Lerp(WeaponFOV, WeaponAimFOV, Aim);

	// ------------------------------------------------------------------ Arma: sway
	const FVector2D Look = Char->GetLookDeltaThisFrame();
	const float SwayMult = FMath::Lerp(1.f, AimSwayMultiplier, Aim);
	FVector SwayTarget(
		FMath::Clamp(Look.Y * SwayRotationAmount, -SwayMaxRotation, SwayMaxRotation),   // pitch
		FMath::Clamp(-Look.X * SwayRotationAmount, -SwayMaxRotation, SwayMaxRotation),  // yaw (el arma se queda atrás)
		FMath::Clamp(-Look.X * SwayRotationAmount * 0.6f, -SwayMaxRotation, SwayMaxRotation)); // roll
	SwayTarget *= SwayMult;
	WeaponSwaySpring.Update(SwayTarget, DeltaTime, SwayFrequency, SwayDamping);

	const FVector SwayLocTarget = FVector(0.f, -Look.X * SwayLocationAmount, Look.Y * SwayLocationAmount) * SwayMult;
	WeaponSwayLocSpring.Update(SwayLocTarget.BoundToCube(2.5f), DeltaTime, SwayFrequency, SwayDamping);

	WeaponKickLocSpring.Update(FVector::ZeroVector, DeltaTime, 6.f, 0.6f);
	WeaponKickRotSpring.Update(FVector::ZeroVector, DeltaTime, 6.f, 0.6f);

	// Inercia: el arma se queda atrás al acelerar y se adelanta al frenar (espacio de cámara: X adelante, Y derecha)
	const FVector LocalAccel = (LocalVel - LastLocalVelocity) / DeltaTime;
	LastLocalVelocity = LocalVel;
	const float InertiaMult = FMath::Lerp(1.f, 0.35f, Aim);
	const FVector InertiaTarget = FVector(-LocalAccel.X * WeaponInertia, -LocalAccel.Y * WeaponInertia, -FMath::Abs(LocalAccel.X) * WeaponInertia * 0.3f).BoundToCube(3.f) * InertiaMult;
	WeaponInertiaSpring.Update(InertiaTarget, DeltaTime, 4.5f, 0.5f);
	const FVector InertiaRotTarget = FVector(LocalAccel.X * WeaponInertiaRoll * 0.6f, 0.f, -LocalAccel.Y * WeaponInertiaRoll).BoundToCube(4.f) * InertiaMult;
	WeaponInertiaRotSpring.Update(InertiaRotTarget, DeltaTime, 4.5f, 0.5f);

	// Respiración: figura de ocho lenta, menor en ADS y nula al esprintar
	BreathTime += DeltaTime;
	const float Breath = FMath::Lerp(1.f, AimBreathMultiplier, Aim) * (1.f - Sprint);
	const FVector BreathLoc = FVector(0.f, FMath::Sin(BreathTime * 0.9f) * 0.6f, FMath::Sin(BreathTime * 1.8f)) * BreathLocation * Breath;
	const FVector BreathRot = FVector(FMath::Sin(BreathTime * 1.8f + 0.7f), FMath::Sin(BreathTime * 0.9f + 0.3f) * 0.7f, 0.f) * BreathRotation * Breath;

	// ------------------------------------------------------------------ Arma: poses
	UpdateAimCalibration(DeltaTime);

	// Desplazamientos en espacio de cámara respecto a la pose ADS calibrada
	FVector WLoc = FMath::Lerp(Poses.HipLocation, FVector::ZeroVector, Aim);
	FRotator WRot = FMath::Lerp(Poses.HipRotation, FRotator::ZeroRotator, Aim);

	WLoc = FMath::Lerp(WLoc, Poses.SprintLocation, Sprint);
	WRot = FMath::Lerp(WRot, Poses.SprintRotation, Sprint);
	WLoc = FMath::Lerp(WLoc, Poses.MantleLocation, Mantle);
	WRot = FMath::Lerp(WRot, Poses.MantleRotation, Mantle);
	WLoc = FMath::Lerp(WLoc, Poses.EquipLocation, EquipAlpha);
	WRot = FMath::Lerp(WRot, Poses.EquipRotation, EquipAlpha);
	// Las poses se ajustaron con la mira a PoseReferenceAimDistance: fuera de ADS no dependen de la distancia de cada arma
	WLoc.X += (Poses.PoseReferenceAimDistance - AimDistance) * (1.f - Aim);
	// Transición de ADS: el arma gira y baja un poco a mitad de camino (se "lleva" a la cara)
	const float AimBell = FMath::Sin(PI * Aim);
	WRot.Roll += AimBell * AimTransitionRoll;
	WLoc.Z += AimBell * AimTransitionDip;

	// Las rotaciones pivotan sobre la mira (en ADS está en (AimDistance,0,0) de la cámara)
	const FVector Pivot(AimDistance, 0.f, 0.f);
	auto PivotOffset = [&Pivot](const FVector& Loc, const FRotator& Rot)
	{
		const FQuat Q = Rot.Quaternion();
		return FTransform(Q, Pivot + Loc - Q.RotateVector(Pivot));
	};
	FTransform Pose = CalibratedAimTransform * PivotOffset(WLoc, WRot);

	// Recarga: pose absoluta del arma (pistolete en ReloadGripLocation, ejes según ReloadRotation)
	if (ReloadAlpha > 0.f)
	{
		const FQuat Axes = FRotationMatrix::MakeFromXZ(WeaponForwardAxis, FVector::UpVector).ToQuat();
		const FTransform ReloadPose = FTransform(Axes).Inverse() * FTransform(Poses.ReloadRotation.Quaternion(), Poses.ReloadGripLocation);
		FTransform Blended;
		Blended.Blend(Pose, ReloadPose, ReloadAlpha);
		Pose = Blended;
	}

	// Movimiento encima de la pose: bob, sway, inclinación al strafe e impulsos
	const float WScale = WeaponMotionScale;
	const FVector MotionLoc = (FVector(0.f, BobH * WeaponBobHorizontal, BobV * WeaponBobVertical) * BobMult
		+ WeaponSwayLocSpring.Value + WeaponKickLocSpring.Value + WeaponInertiaSpring.Value + BreathLoc + ReloadDynLoc) * WScale;
	FVector MotionRot = WeaponSwaySpring.Value + WeaponKickRotSpring.Value + WeaponInertiaRotSpring.Value + BreathRot + RotToVec(ReloadDynRot);
	MotionRot.Z += BobH * WeaponBobRoll * BobMult + StrafeAlpha * WeaponStrafeRoll * (1.f - Aim * 0.8f);

	Weapon->SetRelativeTransform(Pose * PivotOffset(MotionLoc, VecToRot(MotionRot * WScale)));
}
