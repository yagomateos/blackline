#include "Player/BLCharacter.h"

#include "Blackline.h"
#include "Player/BLFirstPersonRigComponent.h"
#include "Player/BLSpring.h"
#include "Debug/BLAutoTestComponent.h"
#include "Animation/BLFirstPersonAnimInstance.h"
#include "Animation/BLWeaponAnimInstance.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLWeaponFXComponent.h"

#include "AnimationRuntime.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/StaticMeshComponent.h"
#include "FX/BLSurfaceEffects.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Core/BLAssetUtils.h"

namespace
{
	using BL::LoadDefault;

	/** Transform de un socket (o hueso) en espacio de componente, en la pose de referencia de la malla. */
	bool GetRefPoseSocketTransform(const USkeletalMesh* Mesh, FName Name, FTransform& Out)
	{
		if (!Mesh || Name.IsNone())
		{
			return false;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		FName Bone = Name;
		FTransform Local = FTransform::Identity;
		if (const USkeletalMeshSocket* Socket = Mesh->FindSocket(Name))
		{
			Bone = Socket->BoneName;
			Local = Socket->GetSocketLocalTransform();
		}
		const int32 Index = Ref.FindBoneIndex(Bone);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Out = Local * FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index);
		return true;
	}
}

ABLCharacter::ABLCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 92.f);

	// ---- Cámara ----
	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	CameraRoot->SetupAttachment(GetCapsuleComponent());
	CameraRoot->SetRelativeLocation(FVector(0.f, 0.f, EyeHeightStanding - 92.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRoot);
	Camera->bUsePawnControlRotation = false; // la rotación la gestiona CameraRoot
	Camera->SetFieldOfView(90.f);
	Camera->bEnableFirstPersonFieldOfView = true;
	Camera->FirstPersonFieldOfView = 50.f;
	Camera->bEnableFirstPersonScale = true;
	Camera->FirstPersonScale = 0.6f;

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(Camera);

	// ---- Malla en primera persona (solo la ve el jugador) ----
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(Camera); // el cuerpo sigue a la cámara; las manos siguen al arma (IK)
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetCastShadow(false);
	FirstPersonMesh->bSelfShadowOnly = false;
	// El Mannequin mira hacia +Y: se gira -90º y se baja para que la cabeza quede en la cámara
	FirstPersonMesh->SetRelativeLocationAndRotation(FVector(-8.f, 0.f, -160.f), FRotator(0.f, -90.f, 0.f));
	// Animación de brazos en C++ (base + acciones + IK de mano izquierda)
	FirstPersonMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	FirstPersonMesh->AnimClass = UBLFirstPersonAnimInstance::StaticClass();

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponRoot); // el rig coloca WeaponRoot delante de la cámara
	WeaponMesh->SetOnlyOwnerSee(true);
	WeaponMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	WeaponMesh->SetCollisionProfileName(FName("NoCollision"));
	WeaponMesh->SetCastShadow(false);
	// Mecánica del arma (cerrojo, gatillo) en C++
	WeaponMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	WeaponMesh->AnimClass = UBLWeaponAnimInstance::StaticClass();

	// ---- Cuerpo para el mundo: invisible para el jugador pero proyecta sombra ----
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -92.f), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	FirstPersonRig = CreateDefaultSubobject<UBLFirstPersonRigComponent>(TEXT("FirstPersonRig"));
	Weapon = CreateDefaultSubobject<UBLWeaponComponent>(TEXT("Weapon"));
	Health = CreateDefaultSubobject<UBLHealthComponent>(TEXT("Health")); // 100, 4 segmentos, regenera tras 4 s
	Health->DamageTakenMultiplier = 0.55f; // dificultad "normal" provisional (menú de dificultad en la Fase 4)

	// ---- Movimiento: rápido pero con inercia creíble ----
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->MaxAcceleration = 2400.f;
	Move->BrakingDecelerationWalking = 2200.f;
	Move->GroundFriction = 8.f;
	Move->JumpZVelocity = 390.f;      // ~78 cm de salto
	Move->AirControl = 0.25f;
	Move->BrakingDecelerationFalling = 600.f;
	Move->MaxStepHeight = 42.f;
	Move->SetWalkableFloorAngle(46.f);
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(58.f);
	Move->bCanWalkOffLedgesWhenCrouching = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;

	// ---- Assets por defecto (packs de Epic; se sustituirán por los propios) ----
	if (USkeletalMesh* Manny = LoadDefault<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")))
	{
		FirstPersonMesh->SetSkeletalMeshAsset(Manny);
		GetMesh()->SetSkeletalMeshAsset(Manny);
	}
	// Uniforme oscuro en vez del Mannequin blanco (provisional hasta tener brazos de soldado propios)
	UMaterialInterface* Body = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Player/Materials/MI_Soldier_Body.MI_Soldier_Body"));
	UMaterialInterface* Sleeves = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Player/Materials/MI_Soldier_Sleeves.MI_Soldier_Sleeves"));
	for (USkeletalMeshComponent* M : { FirstPersonMesh.Get(), GetMesh() })
	{
		if (Body) { M->SetMaterial(0, Body); }
		if (Sleeves) { M->SetMaterial(1, Sleeves); }
	}
	if (USkeletalMesh* Rifle = LoadDefault<USkeletalMesh>(TEXT("/Game/Weapons/Rifle/Meshes/SKM_Rifle.SKM_Rifle")))
	{
		WeaponMesh->SetSkeletalMeshAsset(Rifle);
	}
	BodyIdleAnim = LoadDefault<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
	SurfaceEffects = LoadDefault<UBLSurfaceEffectsData>(TEXT("/Game/FX/DA_SurfaceEffects.DA_SurfaceEffects"));
	for (int32 i = 1; i <= 4; ++i)
	{
		if (USoundBase* Gear = LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Foley/SW_Foley_Gear_%02d.SW_Foley_Gear_%02d"), i, i)))
		{
			GearSounds.Add(Gear);
		}
	}
	for (int32 i = 1; i <= 3; ++i)
	{
		if (USoundBase* Hurt = LoadDefault<USoundBase>(*FString::Printf(TEXT("/Game/Audio/Player/SW_Player_Hurt_%02d.SW_Player_Hurt_%02d"), i, i)))
		{
			HurtSounds.Add(Hurt);
		}
	}
	HitmarkerSound = LoadDefault<USoundBase>(TEXT("/Game/Audio/UI/SW_UI_Hitmarker.SW_UI_Hitmarker"));
	HitmarkerKillSound = LoadDefault<USoundBase>(TEXT("/Game/Audio/UI/SW_UI_Hitmarker_Kill.SW_UI_Hitmarker_Kill"));
	HeartbeatSound = LoadDefault<USoundBase>(TEXT("/Game/Audio/Player/SW_Player_Heartbeat_Loop.SW_Player_Heartbeat_Loop"));
	if (UBLWeaponData* AR7 = LoadDefault<UBLWeaponData>(TEXT("/Game/Weapons/AR7/DA_AR7.DA_AR7")))
	{
		Weapon->StartingWeapons.Add(AR7);
	}

	MoveAction      = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	LookAction      = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	MouseLookAction = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	JumpAction      = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	SprintAction    = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Sprint.IA_Sprint"));
	CrouchAction    = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Crouch.IA_Crouch"));
	LeanLeftAction  = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_LeanLeft.IA_LeanLeft"));
	LeanRightAction = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_LeanRight.IA_LeanRight"));
	AimAction       = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Aim.IA_Aim"));
	FireAction      = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Fire.IA_Fire"));
	ReloadAction    = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Reload.IA_Reload"));
	InteractAction  = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Interact.IA_Interact"));
}

void ABLCharacter::BeginPlay()
{
	Super::BeginPlay();

	SmoothedEyeHeight = EyeHeightStanding;

	// Ocultar cabeza y piernas de la malla FP: solo deben verse brazos y arma
	FirstPersonMesh->HideBoneByName(FName("neck_01"), PBO_None);
	FirstPersonMesh->HideBoneByName(FName("thigh_l"), PBO_None);
	FirstPersonMesh->HideBoneByName(FName("thigh_r"), PBO_None);

	if (BodyIdleAnim)
	{
		GetMesh()->PlayAnimation(BodyIdleAnim, true);
	}

	FirstPersonRig->Initialize(this, Camera, WeaponRoot, WeaponMesh);
	FirstPersonRig->OnFootstep.AddUniqueDynamic(this, &ABLCharacter::HandleFootstep);
	// La malla FP evalúa la animación después de que el rig haya colocado el arma (sin retraso de un frame)
	FirstPersonMesh->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);

	InitCombat();

	// Piloto automático de pruebas: -BLTest=Movement
	FString TestName;
	if (FParse::Value(FCommandLine::Get(), TEXT("BLTest="), TestName) && IsPlayerControlled())
	{
		UBLAutoTestComponent* AutoTest = NewObject<UBLAutoTestComponent>(this, TEXT("BLAutoTest"));
		AutoTest->RegisterComponent();
		AutoTest->StartTest(TestName);
	}
}

void ABLCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogBlackline, Error, TEXT("%s: se requiere EnhancedInputComponent"), *GetNameSafe(this));
		return;
	}

	auto BindIf = [Input](UInputAction* Action, ETriggerEvent Event, auto* Obj, auto Func)
	{
		if (Action)
		{
			Input->BindAction(Action, Event, Obj, Func);
		}
		else
		{
			UE_LOG(LogBlackline, Warning, TEXT("Input action no asignada (falta el asset)"));
		}
	};

	BindIf(MoveAction, ETriggerEvent::Triggered, this, &ABLCharacter::MoveInput);
	BindIf(LookAction, ETriggerEvent::Triggered, this, &ABLCharacter::LookInput);
	BindIf(MouseLookAction, ETriggerEvent::Triggered, this, &ABLCharacter::LookInput);
	BindIf(JumpAction, ETriggerEvent::Started, this, &ABLCharacter::DoJumpOrMantle);
	BindIf(JumpAction, ETriggerEvent::Completed, this, &ABLCharacter::DoStopJump);
	BindIf(CrouchAction, ETriggerEvent::Started, this, &ABLCharacter::ToggleCrouch);
	BindIf(ReloadAction, ETriggerEvent::Started, this, &ABLCharacter::DoReload);

	if (SprintAction)
	{
		Input->BindActionValueLambda(SprintAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetSprintHeld(true); });
		Input->BindActionValueLambda(SprintAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetSprintHeld(false); });
	}
	if (AimAction)
	{
		Input->BindActionValueLambda(AimAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetAimHeld(true); });
		Input->BindActionValueLambda(AimAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetAimHeld(false); });
	}
	if (FireAction)
	{
		Input->BindActionValueLambda(FireAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetFireHeld(true); });
		Input->BindActionValueLambda(FireAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetFireHeld(false); });
	}
	if (LeanLeftAction)
	{
		Input->BindActionValueLambda(LeanLeftAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetLeanLeftHeld(true); });
		Input->BindActionValueLambda(LeanLeftAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetLeanLeftHeld(false); });
	}
	if (InteractAction)
	{
		Input->BindActionValueLambda(InteractAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetInteractHeld(true); });
		Input->BindActionValueLambda(InteractAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetInteractHeld(false); });
	}
	if (LeanRightAction)
	{
		Input->BindActionValueLambda(LeanRightAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetLeanRightHeld(true); });
		Input->BindActionValueLambda(LeanRightAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetLeanRightHeld(false); });
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void ABLCharacter::MoveInput(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	DoMove(V.X, V.Y);
}

void ABLCharacter::LookInput(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	DoLook(V.X, V.Y);
}

void ABLCharacter::DoMove(float Right, float Forward)
{
	if (bDead)
	{
		return;
	}
	PendingMoveInput = FVector2D(FMath::Clamp(PendingMoveInput.X + Right, -1.f, 1.f),
	                             FMath::Clamp(PendingMoveInput.Y + Forward, -1.f, 1.f));
	if (bIsMantling || !Controller)
	{
		return;
	}
	AddMovementInput(GetActorRightVector(), Right);
	AddMovementInput(GetActorForwardVector(), Forward);
}

void ABLCharacter::DoLook(float Yaw, float Pitch)
{
	if (bDead)
	{
		return;
	}
	if (!Controller)
	{
		return;
	}
	const float Sens = LookSensitivity * FMath::Lerp(1.f, 0.7f, AimAlpha); // algo más preciso en ADS
	AddControllerYawInput(Yaw * Sens);
	AddControllerPitchInput(Pitch * Sens);
	LookDeltaThisFrame += FVector2D(Yaw, Pitch) * Sens;
}

void ABLCharacter::DoJumpOrMantle()
{
	if (bDead)
	{
		return;
	}
	if (bIsMantling)
	{
		return;
	}
	if (TryStartMantle())
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch(); // saltar estando agachado = levantarse
		return;
	}
	Jump();
}

void ABLCharacter::DoStopJump()
{
	StopJumping();
}

void ABLCharacter::SetSprintHeld(bool bHeld)
{
	bSprintHeld = bHeld && !bDead;
	if (bHeld && bIsCrouched)
	{
		UnCrouch();
	}
}

void ABLCharacter::ToggleCrouch()
{
	if (bDead)
	{
		return;
	}
	if (bIsMantling)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		bSprintHeld = false;
		Crouch();
	}
}

void ABLCharacter::SetLeanLeftHeld(bool bHeld) { bLeanLeftHeld = bHeld && !bDead; }
void ABLCharacter::SetLeanRightHeld(bool bHeld) { bLeanRightHeld = bHeld && !bDead; }
void ABLCharacter::SetAimHeld(bool bHeld) { bAimHeld = bHeld && !bDead; }

void ABLCharacter::SetFireHeld(bool bHeld)
{
	if (Weapon)
	{
		Weapon->SetTriggerHeld(bHeld && !bDead);
	}
}

void ABLCharacter::DoReload()
{
	if (bDead)
	{
		return;
	}
	if (Weapon)
	{
		Weapon->StartReload();
	}
}

void ABLCharacter::PlayFootstep(float Volume)
{
	if (!SurfaceEffects)
	{
		return;
	}
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BLFootstep), false, this);
	Params.bReturnPhysicalMaterial = true;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}
	const TArray<TObjectPtr<USoundBase>>& Sounds = SurfaceEffects->Get(UBLSurfaceEffectsData::ResolveSurface(Hit)).FootstepSounds;
	if (Sounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sounds[FMath::RandHelper(Sounds.Num())], Hit.ImpactPoint, Volume, FMath::FRandRange(0.94f, 1.06f));
	}
}

void ABLCharacter::HandleFootstep(bool bLeftFoot, float Intensity)
{
	// Más fuerte al esprintar, muy suave agachado o apuntando
	float Volume = FootstepVolume * FMath::Lerp(0.55f, 1.15f, Intensity);
	Volume *= bIsCrouched ? 0.45f : 1.f;
	Volume *= FMath::Lerp(1.f, 0.75f, AimAlpha);
	PlayFootstep(Volume);
	if (bIsSprinting && FMath::RandBool())
	{
		PlayGear(0.35f);
	}
}

void ABLCharacter::PlayGear(float Volume)
{
	if (GearSounds.Num() > 0)
	{
		UGameplayStatics::PlaySound2D(this, GearSounds[FMath::RandHelper(GearSounds.Num())], Volume, FMath::FRandRange(0.9f, 1.1f));
	}
}

UBLFirstPersonAnimInstance* ABLCharacter::GetFirstPersonAnim() const
{
	return FirstPersonMesh ? Cast<UBLFirstPersonAnimInstance>(FirstPersonMesh->GetAnimInstance()) : nullptr;
}

// ---------------------------------------------------------------------------
// Arma (IBLWeaponOwner)
// ---------------------------------------------------------------------------

void ABLCharacter::GetWeaponAimView(FVector& OutOrigin, FVector& OutDirection) const
{
	// Desde CameraRoot (sin bob ni impulsos visuales): el retroceso visual no afecta a la punteria
	OutOrigin = CameraRoot->GetComponentLocation();
	OutDirection = Controller ? Controller->GetControlRotation().Vector() : GetActorForwardVector();
}

bool ABLCharacter::IsWeaponBlocked() const
{
	return bIsMantling || bIsSprinting || SprintAlpha > 0.35f;
}

void ABLCharacter::OnWeaponTriggerPressed()
{
	bSprintHeld = false; // disparar corta el sprint
}

void ABLCharacter::OnWeaponEquipped(const UBLWeaponData* Data)
{
	if (!Data)
	{
		return;
	}
	if (Data->Mesh)
	{
		WeaponMesh->SetSkeletalMeshAsset(Data->Mesh);
	}
	FirstPersonRig->ConfigureWeaponSight(Data->SightSocket, Data->SightLocalOffset, Data->ForwardAxis, Data->AimEyeDistance);

	if (UBLFirstPersonAnimInstance* Anim = GetFirstPersonAnim())
	{
		Anim->SetBaseAnim(Data->IdleAnim);
		FTransform Grip;
		const bool bGrip = GetRefPoseSocketTransform(Data->Mesh, Data->LeftHandSocket, Grip);
		// Solo la posición: los huesos exportados desde Blender traen rotación y escala (x100) propias
		Grip = FTransform(Grip.GetLocation());
		Anim->SetLeftHandGrip(Grip, bGrip);
		if (!bGrip)
		{
			UE_LOG(LogBlackline, Warning, TEXT("%s: el arma no tiene agarre izquierdo '%s' (sin IK)"), *GetNameSafe(Data), *Data->LeftHandSocket.ToString());
		}
	}
}

void ABLCharacter::OnWeaponFired(const UBLWeaponData* Data)
{
	const float Mult = FMath::Lerp(1.f, Data->VisualKickAimMultiplier, AimAlpha);
	const float R = WeaponKickRandomness;
	FRotator Cam = Data->CameraKick * Mult;
	Cam.Roll *= FMath::RandBool() ? 1.f : -1.f;
	FRotator WRot = Data->WeaponKickRotation * Mult;
	WRot.Yaw *= FMath::FRandRange(-R, R) + (1.f - R);
	WRot.Roll *= FMath::FRandRange(-1.f, 1.f);
	FirstPersonRig->AddCameraKick(Cam);
	FirstPersonRig->AddWeaponKick(Data->WeaponKickLocation * Mult, WRot);
	FirstPersonRig->NotifyShot(FMath::Lerp(1.f, 0.55f, AimAlpha));
}

void ABLCharacter::OnWeaponReloadStarted(const UBLWeaponData* Data, float Duration, bool bEmpty)
{
	// La recarga en primera persona es procedural (UpdateProceduralWeaponActions): las animaciones de
	// recarga de Epic son de tercera persona y sacan el arma de cámara. ReloadAnim queda para el cuerpo/IA.
	bSprintHeld = false;
	LastReloadProgress = 0.f;
}

void ABLCharacter::OnWeaponReloadEnded(bool bCompleted)
{
	SetMagazineInHand(false);
}

void ABLCharacter::SetMagazineInHand(bool bInHand)
{
	const UBLWeaponData* Data = Weapon ? Weapon->GetWeaponData() : nullptr;
	if (!Data || !Data->MagazineMesh || WeaponMesh->GetBoneIndex(Data->MagazineBone) == INDEX_NONE)
	{
		return;
	}
	if (bInHand)
	{
		if (!HandMagazine)
		{
			HandMagazine = NewObject<UStaticMeshComponent>(this, TEXT("HandMagazine"));
			HandMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HandMagazine->SetCastShadow(false);
			HandMagazine->SetOnlyOwnerSee(true);
			HandMagazine->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
			HandMagazine->RegisterComponent();
		}
		HandMagazine->SetStaticMesh(Data->MagazineMesh);
		// Donde está ahora el cargador en el arma, pero colgando de la mano izquierda
		const FTransform MagWorld(WeaponMesh->GetComponentQuat(), WeaponMesh->GetSocketLocation(Data->MagazineBone));
		HandMagazine->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::KeepRelativeTransform, TEXT("hand_l"));
		HandMagazine->SetWorldTransform(MagWorld);
		HandMagazine->SetVisibility(true);
		WeaponMesh->HideBoneByName(Data->MagazineBone, PBO_None);
	}
	else
	{
		if (HandMagazine)
		{
			HandMagazine->SetVisibility(false);
		}
		WeaponMesh->UnHideBoneByName(Data->MagazineBone);
	}
}

void ABLCharacter::UpdateProceduralWeaponActions(float DeltaTime)
{
	UBLFirstPersonAnimInstance* Anim = GetFirstPersonAnim();
	const UBLWeaponData* Data = Weapon ? Weapon->GetWeaponData() : nullptr;
	if (!Anim || !Data)
	{
		return;
	}

	const float Equip = FMath::SmoothStep(0.f, 1.f, Weapon->GetEquipFraction());
	float ReloadPose = 0.f;
	float HandAlpha = 0.f;
	FVector HandTarget = FVector::ZeroVector;

	FVector DynLoc = FVector::ZeroVector;
	FRotator DynRot = FRotator::ZeroRotator;
	if (Weapon->IsReloading())
	{
		const float P = Weapon->GetReloadProgress();
		const bool bEmpty = Weapon->IsReloadEmpty();
		const float In = Data->ReloadAmmoInsertTime;
		// El arma entra rápido en la pose de recarga y vuelve al final
		ReloadPose = FMath::SmoothStep(0.f, 0.12f, P) * (1.f - FMath::SmoothStep(bEmpty ? 0.8f : 0.76f, 1.f, P));

		// Puntos de la mano en espacio del arma
		const USkeletalMeshComponent* WMesh = WeaponMesh;
		auto SocketOr = [WMesh](FName Socket, const FVector& Fallback)
		{
			return WMesh->DoesSocketExist(Socket) ? WMesh->GetSocketTransform(Socket, RTS_Component).GetLocation() : Fallback;
		};
		const FVector Grip = Anim->GetState().LeftGripInWeapon.GetLocation();
		const FVector Mag = SocketOr(Data->MagSocket, Data->MagLocalOffset);
		const FVector MagTop = SocketOr(Data->MagazineBone, Mag + FVector(0.f, -3.f, 18.f));
		const FVector MagAxis = (Mag - MagTop).GetSafeNormal();            // hacia fuera del brocal
		const FVector MagBelow = Mag + MagAxis * 4.f + FVector(1.5f, 0.f, 0.f);
		const FVector MagPulled = Mag + MagAxis * 15.f;
		const FVector Pouch = Mag + FVector(9.f, -12.f, -38.f);              // portacargadores (fuera de cámara)
		const FVector Handle = SocketOr(Data->ChargingHandleSocket, Data->ChargingHandleLocalOffset);
		const FVector BoltCatch = FVector(2.4f, MagTop.Y - 6.f, MagTop.Z + 2.f); // retenida, lado izquierdo del cajón

		// Línea de tiempo (fracción). Interpolación Catmull-Rom: trayectoria continua, sin paradas
		TArray<TPair<float, FVector>, TInlineAllocator<16>> Keys;
		Keys.Add({ 0.00f, Grip });
		Keys.Add({ 0.09f, MagBelow });
		Keys.Add({ 0.15f, Mag });               // agarra el cargador (se separa del arma)
		Keys.Add({ 0.24f, MagPulled });         // tira a lo largo de su eje
		Keys.Add({ 0.34f, Pouch });
		Keys.Add({ 0.41f, Pouch + FVector(0.f, 3.f, 2.f) });  // coge el nuevo
		Keys.Add({ In - 0.08f, MagPulled });    // lo alinea bajo el brocal
		Keys.Add({ In, Mag });                   // lo mete
		Keys.Add({ In + 0.04f, Mag - MagAxis * 1.2f });  // golpe en la base para asentarlo
		if (bEmpty)
		{
			Keys.Add({ In + 0.08f, Mag + FVector(2.f, -2.f, 3.f) });
			Keys.Add({ In + 0.12f, BoltCatch });  // golpe a la retenida: el cerrojo cierra
			Keys.Add({ In + 0.17f, BoltCatch + FVector(1.f, 4.f, -2.f) });
			Keys.Add({ 0.88f, Grip + FVector(0.f, -3.f, -4.f) });
			Keys.Add({ 0.97f, Grip });
		}
		else
		{
			Keys.Add({ 0.72f, Grip + FVector(0.f, -4.f, -5.f) });
			Keys.Add({ 0.84f, Grip });
		}
		Keys.Add({ 1.01f, Grip });

		HandTarget = Grip;
		for (int32 k = 0; k + 1 < Keys.Num(); ++k)
		{
			if (P >= Keys[k].Key && P < Keys[k + 1].Key)
			{
				const float A = (P - Keys[k].Key) / (Keys[k + 1].Key - Keys[k].Key);
				const FVector& P0 = Keys[FMath::Max(k - 1, 0)].Value;
				const FVector& P1 = Keys[k].Value;
				const FVector& P2 = Keys[k + 1].Value;
				const FVector& P3 = Keys[FMath::Min(k + 2, Keys.Num() - 1)].Value;
				const float A2 = A * A, A3 = A2 * A;
				HandTarget = 0.5f * ((2.f * P1) + (-P0 + P2) * A + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * A2 + (-P0 + 3.f * P1 - 3.f * P2 + P3) * A3);
				break;
			}
		}
		HandAlpha = 1.f;

		// Movimiento del arma ligado a las acciones de la mano (espacio de cámara)
		auto Bell = [P](float A, float B) { return P > A && P < B ? FMath::Sin(PI * (P - A) / (B - A)) : 0.f; };
		DynRot.Roll += 9.f * Bell(0.13f, 0.30f);                 // el arma gira al tirar del cargador
		DynLoc.Z -= 1.5f * Bell(0.13f, 0.28f);
		DynRot.Pitch += 6.f * Bell(In - 0.12f, In + 0.04f);      // se inclina hacia la mano al meter el nuevo
		DynRot.Roll -= 5.f * Bell(In - 0.1f, In + 0.05f);
		if (bEmpty)
		{
			DynRot.Yaw += 5.f * Bell(In + 0.06f, In + 0.2f);    // gira hacia la retenida
		}

		// Golpes: sacar, meter, asentar, cerrojo
		auto Crossed = [this, P](float T) { return LastReloadProgress < T && P >= T; };
		if (Crossed(0.15f))
		{
			SetMagazineInHand(true);
			FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, -18.f), FRotator(-6.f, 0.f, 8.f));
		}
		if (Crossed(0.30f) || Crossed(0.80f))
		{
			if (UBLWeaponFXComponent* WFX = FindComponentByClass<UBLWeaponFXComponent>())
			{
				WFX->PlayHandling(Data, 0.8f);  // roce de la mano con el equipo
			}
		}
		if (Crossed(In))
		{
			SetMagazineInHand(false);
			FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, 55.f), FRotator(18.f, 0.f, -7.f));
			FirstPersonRig->AddCameraKick(FRotator(6.f, 0.f, 2.f));
		}
		if (Crossed(In + 0.04f))
		{
			FirstPersonRig->AddWeaponKick(FVector(0.f, 0.f, 25.f), FRotator(8.f, 0.f, 0.f));
		}
		if (bEmpty && Crossed(In + 0.12f))
		{
			FirstPersonRig->AddWeaponKick(FVector(-35.f, -10.f, 6.f), FRotator(6.f, -6.f, 4.f));
			FirstPersonRig->AddCameraKick(FRotator(4.f, 0.f, -3.f));
		}
		LastReloadProgress = P;
	}
	FirstPersonRig->SetReloadDynamics(DynLoc, DynRot);

	FirstPersonRig->SetActionPoses(ReloadPose, Equip);
	Anim->SetLeftHandOverride(HandAlpha, HandTarget);
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void ABLCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	LastMoveInput = PendingMoveInput;
	UpdateCombat(DeltaTime);
	UpdateInteraction(DeltaTime);

	UpdateMantle(DeltaTime);
	UpdateAim(DeltaTime);
	UpdateSprint(DeltaTime);
	UpdateStance(DeltaTime);
	UpdateLean(DeltaTime);
	UpdateCameraRoot();

	UpdateProceduralWeaponActions(DeltaTime);

	// Foley: cambios de postura del arma
	if (bIsSprinting != bWasSprinting || bIsAiming != bWasAiming)
	{
		PlayGear(bIsAiming != bWasAiming ? 0.3f : 0.45f);
		bWasSprinting = bIsSprinting;
		bWasAiming = bIsAiming;
	}
	const UBLFirstPersonAnimInstance* Anim = GetFirstPersonAnim();
	FirstPersonRig->SetCalibrationFrozen(Anim && Anim->GetActionWeight() > 0.01f);
	FirstPersonRig->UpdateRig(DeltaTime);
	ApplyDeathPose(DeltaTime);
	if (UBLFirstPersonAnimInstance* FPAnim = GetFirstPersonAnim())
	{
		FPAnim->SetWeaponTransform(WeaponMesh->GetComponentTransform().GetRelativeTransform(FirstPersonMesh->GetComponentTransform()),
			WeaponMesh->GetSkeletalMeshAsset() != nullptr);
	}

	PendingMoveInput = FVector2D::ZeroVector;
	LookDeltaThisFrame = FVector2D::ZeroVector;
}

bool ABLCharacter::CanSprint() const
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	return bSprintHeld
		&& !bIsAiming
		&& !(Weapon && (Weapon->IsReloading() || Weapon->IsTriggerHeld()))
		&& !bIsMantling
		&& !bIsCrouched
		&& Move->IsMovingOnGround()
		&& LastMoveInput.Y >= SprintForwardThreshold
		&& FMath::Abs(LastMoveInput.X) < 0.8f;
}

void ABLCharacter::UpdateAim(float DeltaTime)
{
	// Apuntar cancela el sprint (como en un shooter moderno: ADS tiene prioridad)
	// Recargar tambien saca del ADS
	bIsAiming = bAimHeld && !bIsMantling && !(Weapon && Weapon->IsReloading());
	AimAlpha = BLExpInterp(AimAlpha, bIsAiming ? 1.f : 0.f, DeltaTime, AimInterpSpeed);
	if (FMath::IsNearlyEqual(AimAlpha, bIsAiming ? 1.f : 0.f, 0.001f))
	{
		AimAlpha = bIsAiming ? 1.f : 0.f;
	}
}

void ABLCharacter::UpdateSprint(float DeltaTime)
{
	bIsSprinting = CanSprint();
	SprintAlpha = BLExpInterp(SprintAlpha, bIsSprinting ? 1.f : 0.f, DeltaTime, 8.f);

	float Speed = bIsSprinting ? SprintSpeed : WalkSpeed;
	Speed *= FMath::Lerp(1.f, AimSpeedMultiplier, AimAlpha);
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed * FMath::Lerp(1.f, 0.8f, AimAlpha);
}

void ABLCharacter::UpdateStance(float DeltaTime)
{
	const float Target = bIsCrouched ? EyeHeightCrouched : EyeHeightStanding;
	SmoothedEyeHeight = BLExpInterp(SmoothedEyeHeight, Target, DeltaTime, EyeHeightInterpSpeed);
}

void ABLCharacter::UpdateLean(float DeltaTime)
{
	float Target = 0.f;
	if (!bIsSprinting && !bIsMantling)
	{
		Target = (bLeanRightHeld ? 1.f : 0.f) - (bLeanLeftHeld ? 1.f : 0.f);
	}

	// Limitar la inclinación si hay una pared al lado (evita meter la cámara en la geometría)
	if (!FMath::IsNearlyZero(Target))
	{
		const FVector Base = CameraRoot->GetComponentLocation() - GetActorRightVector() * (LeanAlpha * LeanOffset);
		const FVector End = Base + GetActorRightVector() * (Target * LeanOffset);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BLLean), false, this);
		if (GetWorld()->SweepSingleByChannel(Hit, Base, End, FQuat::Identity, ECC_Camera,
		                                     FCollisionShape::MakeSphere(LeanProbeRadius), Params))
		{
			Target *= Hit.Time;
		}
	}
	LeanAlpha = BLExpInterp(LeanAlpha, Target, DeltaTime, LeanInterpSpeed);
}

void ABLCharacter::UpdateCameraRoot()
{
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Pitch = Controller ? Controller->GetControlRotation().GetNormalized().Pitch : 0.f;

	// Altura medida desde el suelo: al agacharse la cápsula cambia de golpe, pero la cámara no salta
	FVector Loc(0.f, LeanAlpha * LeanOffset, SmoothedEyeHeight - HalfHeight);
	// Inclinarse baja un poco la cabeza (pivotando en la cadera)
	Loc.Z -= FMath::Abs(LeanAlpha) * 4.f;
	FRotator Rot(Pitch, 0.f, LeanAlpha * LeanRoll);
	if (bDead)
	{
		// Caída: la cabeza baja con aceleración hasta el suelo y la vista rueda hacia un lado
		const float A = FMath::Clamp(DeathTime / 0.85f, 0.f, 1.f);
		Loc.Z = FMath::Lerp(Loc.Z, 22.f - HalfHeight, A * A);
		Loc.Y += DeathRollSign * 14.f * A;
		Rot.Roll = FMath::Lerp(Rot.Roll, DeathRollSign * 75.f, FMath::SmoothStep(0.f, 1.f, A));
		Rot.Pitch = FMath::Lerp(Rot.Pitch, 6.f, A);
	}
	CameraRoot->SetRelativeLocationAndRotation(Loc, Rot);
}

void ABLCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	FirstPersonRig->NotifyLanded(GetCharacterMovement()->Velocity.Z);
	const float Impact = FMath::GetMappedRangeValueClamped(FVector2D(200.f, 900.f), FVector2D(0.6f, 1.3f), -GetCharacterMovement()->Velocity.Z);
	PlayFootstep(FootstepVolume * Impact);
	PlayGear(0.5f * Impact);
}

void ABLCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonRig->NotifyCrouchChanged(true);
	PlayGear(0.4f);
}

void ABLCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonRig->NotifyCrouchChanged(false);
	PlayGear(0.35f);
}

void ABLCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	FirstPersonRig->NotifyJumped();
}

// ---------------------------------------------------------------------------
// Mantle
// ---------------------------------------------------------------------------

bool ABLCharacter::TryStartMantle()
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Loc = GetActorLocation();
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const float FeetZ = Loc.Z - HalfHeight;
	UWorld* World = GetWorld();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BLMantle), false, this);

	if (bIsCrouched)
	{
		return false; // primero levantarse
	}

	// 1) Pared delante: se prueba a varias alturas (útil también en el aire junto a un borde)
	FHitResult WallHit;
	bool bWall = false;
	for (const float H : { 40.f, 80.f, 125.f })
	{
		const FVector Start(Loc.X, Loc.Y, FeetZ + H);
		const FVector End = Start + Forward * (Radius + MantleReach);
		if (World->LineTraceSingleByChannel(WallHit, Start, End, ECC_Visibility, Params))
		{
			bWall = true;
			break;
		}
	}
	if (!bWall)
	{
		return false;
	}
	const FVector WallNormal = WallHit.ImpactNormal.GetSafeNormal2D();
	if (FVector::DotProduct(-WallNormal, Forward) < 0.7f) // hay que mirar la pared de frente (±45º)
	{
		return false;
	}

	// 2) Borde superior: trazar hacia abajo justo detrás de la cara de la pared
	const FVector Probe = WallHit.ImpactPoint - WallNormal * 22.f;
	FHitResult LedgeHit;
	const FVector DownStart(Probe.X, Probe.Y, FeetZ + MantleMaxHeight + 10.f);
	const FVector DownEnd(Probe.X, Probe.Y, FeetZ + MantleMinHeight - 5.f);
	if (!World->SweepSingleByChannel(LedgeHit, DownStart, DownEnd, FQuat::Identity, ECC_Visibility,
	                                 FCollisionShape::MakeSphere(8.f), Params)
		|| LedgeHit.bStartPenetrating || LedgeHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}
	const float LedgeZ = LedgeHit.ImpactPoint.Z;
	const float Height = LedgeZ - FeetZ;
	if (Height < MantleMinHeight || Height > MantleMaxHeight)
	{
		return false;
	}

	// 3) Espacio libre: subiendo junto a la pared y en el destino encima del borde
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	const FVector Mid(Loc.X, Loc.Y, LedgeZ + HalfHeight + 2.f);
	const FVector Dest = FVector(WallHit.ImpactPoint.X, WallHit.ImpactPoint.Y, LedgeZ + HalfHeight + 2.f) - WallNormal * (Radius + 12.f);
	if (World->OverlapBlockingTestByChannel(Mid, FQuat::Identity, ECC_Pawn, CapsuleShape, Params)
		|| World->OverlapBlockingTestByChannel(Dest, FQuat::Identity, ECC_Pawn, CapsuleShape, Params))
	{
		return false;
	}

	bIsMantling = true;
	MantleTime = 0.f;
	MantleStart = Loc;
	MantleMid = Mid;
	MantleEnd = Dest;
	const float HeightAlpha = (Height - MantleMinHeight) / (MantleMaxHeight - MantleMinHeight);
	MantleDuration = FMath::Lerp(MantleDurationLow, MantleDurationHigh, HeightAlpha);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	FirstPersonRig->NotifyMantleStarted(HeightAlpha);

	UE_LOG(LogBlackline, Verbose, TEXT("Mantle: altura %.0f cm, duración %.2f s"), Height, MantleDuration);
	return true;
}

void ABLCharacter::UpdateMantle(float DeltaTime)
{
	MantleAlpha = BLExpInterp(MantleAlpha, bIsMantling ? 1.f : 0.f, DeltaTime, 14.f);
	if (!bIsMantling)
	{
		return;
	}

	MantleTime += DeltaTime;
	const float T = FMath::Clamp(MantleTime / MantleDuration, 0.f, 1.f);
	// 60% subir, 40% avanzar, con algo de solape para que no se note la esquina del recorrido
	const float Rise = FMath::SmoothStep(0.f, 0.65f, T);
	const float Advance = FMath::SmoothStep(0.45f, 1.f, T);
	FVector NewLoc = FMath::Lerp(MantleStart, MantleMid, Rise);
	NewLoc.X = FMath::Lerp(MantleStart.X, MantleEnd.X, Advance);
	NewLoc.Y = FMath::Lerp(MantleStart.Y, MantleEnd.Y, Advance);
	SetActorLocation(NewLoc, false, nullptr, ETeleportType::None);

	if (T >= 1.f)
	{
		bIsMantling = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}
