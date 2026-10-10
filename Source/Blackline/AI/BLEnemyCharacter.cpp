#include "AI/BLEnemyCharacter.h"

#include "AI/BLAIController.h"
#include "Audio/BLAudioSubsystem.h"
#include "Animation/BLEnemyAnimInstance.h"
#include "Animation/BLWeaponAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Combat/BLDamageTypes.h"
#include "Combat/BLHealthComponent.h"
#include "Combat/BLHitReactionComponent.h"
#include "FX/BLSurfaceEffects.h"
#include "Weapons/BLWeaponComponent.h"
#include "Weapons/BLWeaponData.h"
#include "Weapons/BLWeaponPickup.h"

#include "Components/AudioComponent.h"
#include "Components/SpotLightComponent.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Core/BLAssetUtils.h"

namespace
{
	using BL::LoadDefault;

	/** Ojos sobre el centro de la cápsula (de pie / agachado). */
	constexpr float EyeStand = 63.f;
	constexpr float EyeCrouch = 3.f;
}

ABLEnemyCharacter::ABLEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ABLAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	Health = CreateDefaultSubobject<UBLHealthComponent>(TEXT("Health"));
	Health->Segments = 1;
	Health->RegenRate = 0.f;
	HitReaction = CreateDefaultSubobject<UBLHitReactionComponent>(TEXT("HitReaction"));
	Weapon = CreateDefaultSubobject<UBLWeaponComponent>(TEXT("Weapon"));
	Weapon->bInfiniteReserve = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 92.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -92.f), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->AnimClass = UBLEnemyAnimInstance::StaticClass();
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName("HandGrip_R"));
	WeaponMesh->SetCollisionProfileName(FName("NoCollision"));
	WeaponMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	WeaponMesh->AnimClass = UBLWeaponAnimInstance::StaticClass();

	auto MakeGear = [this](const TCHAR* Name, const TCHAR* MeshPath, FName Bone)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(GetMesh(), Bone);
		C->SetStaticMesh(LoadDefault<UStaticMesh>(MeshPath));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		return C;
	};
	Vest = MakeGear(TEXT("Vest"), TEXT("/Game/Characters/Enemy/Gear/SM_Militia_Vest.SM_Militia_Vest"), FName("spine_05"));
	Helmet = MakeGear(TEXT("Helmet"), TEXT("/Game/Characters/Enemy/Gear/SM_Militia_Helmet.SM_Militia_Helmet"), FName("head"));
	Armband = MakeGear(TEXT("Armband"), TEXT("/Game/Characters/Enemy/Gear/SM_Militia_Armband.SM_Militia_Armband"), FName("upperarm_l"));

	// Linterna bajo el guardamanos (el arma mira a +Y): apagada salvo con bFlashlight
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(WeaponMesh);
	Flashlight->SetRelativeLocationAndRotation(FVector(0.f, 48.f, 2.f), FRotator(0.f, 90.f, 0.f));
	Flashlight->SetIntensityUnits(ELightUnits::Candelas);
	Flashlight->SetIntensity(320.f);
	Flashlight->SetAttenuationRadius(3200.f);
	Flashlight->SetInnerConeAngle(9.f);
	Flashlight->SetOuterConeAngle(24.f);
	Flashlight->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
	Flashlight->SetCastShadows(false);
	Flashlight->SetVolumetricScatteringIntensity(2.f);   // el haz se ve en la niebla
	Flashlight->SetVisibility(false);

	Laser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Laser"));
	Laser->SetupAttachment(RootComponent);
	Laser->SetStaticMesh(LoadDefault<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")));
	Laser->SetMaterial(0, LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Laser_Red.MI_Laser_Red")));
	Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Laser->SetCastShadow(false);
	Laser->SetCanEverAffectNavigation(false);
	Laser->SetUsingAbsoluteLocation(true);
	Laser->SetUsingAbsoluteRotation(true);
	Laser->SetUsingAbsoluteScale(true);
	Laser->SetVisibility(false);
	CorvaneBody = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Corvane_Body.MI_Corvane_Body"));
	CorvaneSleeves = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Corvane_Sleeves.MI_Corvane_Sleeves"));
	CorvaneHelmet = LoadDefault<UStaticMesh>(TEXT("/Game/Characters/Enemy/Gear/SM_Corvane_Helmet.SM_Corvane_Helmet"));
	CorvaneVest = LoadDefault<UStaticMesh>(TEXT("/Game/Characters/Enemy/Gear/SM_Corvane_Vest.SM_Corvane_Vest"));
	ArmyBody = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Army_Body.MI_Army_Body"));
	ArmySleeves = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Army_Sleeves.MI_Army_Sleeves"));

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->MaxAcceleration = 1400.f;
	Move->BrakingDecelerationWalking = 1600.f;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 200.f;
	bUseControllerRotationYaw = false;

	if (USkeletalMesh* Manny = LoadDefault<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")))
	{
		GetMesh()->SetSkeletalMeshAsset(Manny);
	}
	if (UMaterialInterface* Body = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Militia_Body.MI_Militia_Body")))
	{
		GetMesh()->SetMaterial(0, Body);
	}
	if (UMaterialInterface* Sleeves = LoadDefault<UMaterialInterface>(TEXT("/Game/Characters/Enemy/Materials/MI_Militia_Sleeves.MI_Militia_Sleeves")))
	{
		GetMesh()->SetMaterial(1, Sleeves);
	}
	if (UBLWeaponData* AR7 = LoadDefault<UBLWeaponData>(TEXT("/Game/Weapons/AR7/DA_AR7.DA_AR7")))
	{
		Weapon->StartingWeapons.Add(AR7);
	}
	SurfaceEffects = LoadDefault<UBLSurfaceEffectsData>(TEXT("/Game/FX/DA_SurfaceEffects.DA_SurfaceEffects"));
	const TCHAR* Base = TEXT("/Game/Characters/Mannequins/Anims/Rifle/");
	IdleAnim = LoadDefault<UAnimSequence>(*FString::Printf(TEXT("%sMF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"), Base));
	ReloadAnim = LoadDefault<UAnimSequence>(*FString::Printf(TEXT("%sMM_Rifle_Reload.MM_Rifle_Reload"), Base));
	for (const TCHAR* Dir : { TEXT("Fwd"), TEXT("Fwd_Right"), TEXT("Right"), TEXT("Bwd_Right"), TEXT("Bwd"), TEXT("Bwd_Left"), TEXT("Left"), TEXT("Fwd_Left") })
	{
		WalkAnims.Add(LoadDefault<UAnimSequence>(*FString::Printf(TEXT("%sWalk/MF_Rifle_Walk_%s.MF_Rifle_Walk_%s"), Base, Dir, Dir)));
		JogAnims.Add(LoadDefault<UAnimSequence>(*FString::Printf(TEXT("%sJog/MF_Rifle_Jog_%s.MF_Rifle_Jog_%s"), Base, Dir, Dir)));
	}
}

void ABLEnemyCharacter::BeginPlay()
{
	DropWeaponData = Weapon->StartingWeapons.Num() > 0 ? Weapon->StartingWeapons[0].Get() : nullptr;
	// Tirador de supresión: antes de Super (el arma y la salud se inicializan en el BeginPlay de sus componentes)
	if (EnemyRole == EBLEnemyRole::Gunner)
	{
		Health->MaxHealth = 190.f;
		WalkSpeed = 150.f;
		JogSpeed = 320.f;
		if (Weapon->StartingWeapons.Num() > 0 && Weapon->StartingWeapons[0])
		{
			// Ametralladora ligera a partir del AR-7: cinta de 100, algo más lenta y menos precisa
			UBLWeaponData* LMG = DuplicateObject<UBLWeaponData>(Weapon->StartingWeapons[0], this);
			LMG->RoundsPerMinute = 640.f;
			LMG->MagazineSize = 100;
			LMG->Damage = 24.f;
			LMG->HipSpread *= 1.6f;
			LMG->AimSpread = 0.7f;
			LMG->NoiseRange = 6500.f;
			Weapon->StartingWeapons[0] = LMG;
		}
	}
	else if (EnemyRole == EBLEnemyRole::Operator || EnemyRole == EBLEnemyRole::Sniper)
	{
		// Placa: el cuerpo recibe ~65 % (la cabeza sigue multiplicando x2,5 en el arma)
		Health->MaxHealth = EnemyRole == EBLEnemyRole::Operator ? 140.f : 110.f;
		Health->DamageTakenMultiplier = EnemyRole == EBLEnemyRole::Operator ? 0.65f : 0.8f;
		if (Weapon->StartingWeapons.Num() > 0 && Weapon->StartingWeapons[0])
		{
			UBLWeaponData* W = DuplicateObject<UBLWeaponData>(Weapon->StartingWeapons[0], this);
			if (EnemyRole == EBLEnemyRole::Sniper)
			{
				// Fusil de tirador: tiro a tiro, mucho daño, preciso
				W->RoundsPerMinute = 45.f;
				W->MagazineSize = 5;
				W->Damage = 80.f;
				W->HipSpread = 0.15f;
				W->AimSpread = 0.02f;
				W->NoiseRange = 9000.f;
				W->MaxRange = 20000.f;
			}
			else
			{
				W->Damage = 30.f;
				W->HipSpread *= 0.6f;
			}
			Weapon->StartingWeapons[0] = W;
		}
		JogSpeed = 430.f;
	}
	else if (EnemyRole == EBLEnemyRole::Ally)
	{
		Health->MaxHealth = 160.f;
	}
	Super::BeginPlay();
	Flashlight->SetVisibility(bFlashlight);
	if (IsCorvane())
	{
		// Negro sin insignias, casco con visor y chaleco de placas; sin brazalete de la Columna
		if (CorvaneBody) { GetMesh()->SetMaterial(0, CorvaneBody); }
		if (CorvaneSleeves) { GetMesh()->SetMaterial(1, CorvaneSleeves); }
		if (CorvaneHelmet) { Helmet->SetStaticMesh(CorvaneHelmet); }
		if (CorvaneVest) { Vest->SetStaticMesh(CorvaneVest); }
		Armband->SetVisibility(false);
	}
	else if (EnemyRole == EBLEnemyRole::Ally)
	{
		// Ejército de Varania: uniforme gris verdoso, sin el brazalete negro de la Columna
		if (ArmyBody) { GetMesh()->SetMaterial(0, ArmyBody); }
		if (ArmySleeves) { GetMesh()->SetMaterial(1, ArmySleeves); }
		Armband->SetVisibility(false);
	}
	BLDamage::SetupCharacterCollision(this);
	Health->OnDeath.AddDynamic(this, &ABLEnemyCharacter::HandleDeath);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	AimPoint = GetActorLocation() + GetActorForwardVector() * 1000.f;
	if (VoiceIndex < 0)
	{
		VoiceIndex = int32(GetTypeHash(GetName()) % 3u);
	}
	FitGearToBone(Vest, FName("spine_05"));
	FitGearToBone(Helmet, FName("head"));
	// Variedad: uno de cada tres va sin casco (pasamontañas); el tirador de supresión y Corvane siempre con casco
	Helmet->SetVisibility(VoiceIndex != 2 || EnemyRole != EBLEnemyRole::Rifleman);
}

void ABLEnemyCharacter::OnMissionActivate(FName Tag)
{
	if (IBLActivatable* C = Cast<IBLActivatable>(GetController()))
	{
		C->OnMissionActivate(Tag);
	}
}

bool ABLEnemyCharacter::IsLaserOn() const
{
	return Laser && Laser->IsVisible();
}

void ABLEnemyCharacter::TickLaser()
{
	const bool bOn = EnemyRole == EBLEnemyRole::Sniper && bAiming && !IsDead();
	Laser->SetVisibility(bOn);
	if (!bOn)
	{
		return;
	}
	// Del cañón hacia donde apunta, hasta lo primero que toque (el punto rojo sobre la pared o sobre ti)
	const FVector Start = WeaponMesh->DoesSocketExist(FName("Muzzle")) ? WeaponMesh->GetSocketLocation(FName("Muzzle")) : GetEyeLocation();
	const FVector Dir = (AimPoint - Start).GetSafeNormal();
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLLaser), false, this);
	const FVector End = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Dir * 12000.f, ECC_Visibility, Q) ? FVector(Hit.ImpactPoint) : Start + Dir * 12000.f;
	const float Len = FVector::Dist(Start, End);
	Laser->SetWorldLocationAndRotation((Start + End) * 0.5f, Dir.Rotation());
	Laser->SetWorldScale3D(FVector(Len / 100.f, 0.006f, 0.006f));
}

USpotLightComponent* ABLEnemyCharacter::GetFlashlight() const
{
	return bFlashlight && Flashlight && Flashlight->IsVisible() && !IsDead() ? Flashlight.Get() : nullptr;
}

void ABLEnemyCharacter::SetAim(const FVector& Point, bool bInAiming)
{
	AimPoint = Point;
	bAiming = bInAiming;
	GetCharacterMovement()->bOrientRotationToMovement = !bInAiming;
}

void ABLEnemyCharacter::SetJog(bool bJog)
{
	GetCharacterMovement()->MaxWalkSpeed = bJog ? JogSpeed : WalkSpeed;
}

bool ABLEnemyCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

FVector ABLEnemyCharacter::GetEyeLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FMath::Lerp(EyeStand, EyeCrouch, CrouchAlpha));
}

void ABLEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickLaser();   // antes de salir si está muerto: así se apaga
	if (IsDead())
	{
		return;
	}
	// Agacharse (solo la pose: la cápsula no cambia; las balas dan en los huesos)
	CrouchAlpha = FMath::FInterpTo(CrouchAlpha, bWantsCrouch ? 1.f : 0.f, DeltaTime, 6.f);
	TickFootsteps(DeltaTime);
	ReloadTimeLeft = FMath::Max(0.f, ReloadTimeLeft - DeltaTime);

	// Apuntando: el cuerpo mira al objetivo (y camina en lateral/de espaldas con las animaciones de 8 direcciones)
	if (bAiming)
	{
		const FVector To = (AimPoint - GetActorLocation()).GetSafeNormal2D();
		if (!To.IsNearlyZero())
		{
			const FRotator Target(0.f, To.Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Target, DeltaTime, 300.f));
		}
	}
}

void ABLEnemyCharacter::GetWeaponAimView(FVector& OutOrigin, FVector& OutDirection) const
{
	OutOrigin = GetEyeLocation();
	OutDirection = (AimPoint - OutOrigin).GetSafeNormal();
}

bool ABLEnemyCharacter::IsWeaponBlocked() const
{
	if (!bAiming || IsDead() || (CrouchAlpha > 0.15f && CrouchAlpha < 0.85f))
	{
		return true;
	}
	// No dispara hasta tener el cuerpo girado hacia el objetivo
	const FVector To = (AimPoint - GetActorLocation()).GetSafeNormal2D();
	return FVector::DotProduct(To, GetActorForwardVector()) < 0.9f;
}

void ABLEnemyCharacter::OnWeaponEquipped(const UBLWeaponData* Data)
{
	if (Data && Data->Mesh)
	{
		WeaponMesh->SetSkeletalMeshAsset(Data->Mesh);
	}
}

void ABLEnemyCharacter::OnWeaponFired(const UBLWeaponData* Data)
{
	FireKick = 1.f;
}

void ABLEnemyCharacter::OnWeaponReloadStarted(const UBLWeaponData* Data, float Duration, bool bEmpty)
{
	ReloadDuration = ReloadTimeLeft = Duration;
}

void ABLEnemyCharacter::OnWeaponReloadEnded(bool bCompleted)
{
	ReloadTimeLeft = 0.f;
}

void ABLEnemyCharacter::FitGearToBone(UStaticMeshComponent* Piece, FName Bone)
{
	const USkeletalMesh* SkelMesh = GetMesh()->GetSkeletalMeshAsset();
	if (!Piece || !SkelMesh)
	{
		return;
	}
	const FReferenceSkeleton& Ref = SkelMesh->GetRefSkeleton();
	const int32 Index = Ref.FindBoneIndex(Bone);
	if (Index != INDEX_NONE)
	{
		// La pieza está modelada en el espacio de la malla: relativa al hueso = inversa de su pose de referencia
		Piece->SetRelativeTransform(FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index).Inverse());
	}
}

void ABLEnemyCharacter::TickFootsteps(float DeltaTime)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();
	if (!Move->IsMovingOnGround() || Speed < 40.f)
	{
		StepDistance = 0.f;
		return;
	}
	// Un paso cada media zancada (más larga al trotar). No hace falta seguir la animación: se oye, no se ve
	const float Jog = FMath::GetMappedRangeValueClamped(FVector2D(WalkSpeed, JogSpeed), FVector2D(0.f, 1.f), Speed);
	const float Stride = FMath::Lerp(72.f, 118.f, Jog);
	StepDistance += Speed * DeltaTime;
	if (StepDistance >= Stride)
	{
		StepDistance -= Stride;
		const float Volume = FootstepVolume * FMath::Lerp(0.6f, 1.1f, Jog) * FMath::Lerp(1.f, 0.6f, CrouchAlpha);
		PlaySurfaceSound(Volume, FMath::FRandRange(0.93f, 1.05f));
		++FootstepsPlayed;
	}
}

void ABLEnemyCharacter::PlaySurfaceSound(float Volume, float Pitch)
{
	if (!SurfaceEffects)
	{
		return;
	}
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 40.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BLEnemyStep), false, this);
	Params.bReturnPhysicalMaterial = true;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}
	const TArray<TObjectPtr<USoundBase>>& Sounds = SurfaceEffects->Get(UBLSurfaceEffectsData::ResolveSurface(Hit)).FootstepSounds;
	if (Sounds.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sounds[FMath::RandHelper(Sounds.Num())], Hit.ImpactPoint, Volume, Pitch);
	}
}

void ABLEnemyCharacter::HandleDeath(const FBLDamageInfo& Info)
{
	// Se calla en seco; un compañero cercano avisa de la baja
	if (UAudioComponent* Voice = VoiceComponent.Get())
	{
		Voice->FadeOut(0.08f, 0.f);
	}
	if (UBLAudioSubsystem* Audio = UBLAudioSubsystem::Get(this))
	{
		Audio->NotifyEnemyKilled(this);
	}
	// Golpe del cuerpo contra el suelo (paso grave y fuerte en la superficie)
	FTimerHandle FallTimer;
	GetWorldTimerManager().SetTimer(FallTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		PlaySurfaceSound(1.6f, 0.55f);
	}), 0.55f, false);
	Weapon->SetTriggerHeld(false);
	Weapon->CancelReload();
	// El arma cae al suelo con físicas (separada del ragdoll)
	WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	WeaponMesh->SetCollisionProfileName(FName("PhysicsActor"));
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->AddImpulse(Info.Direction * 150.f + FVector(0.f, 0.f, 100.f), NAME_None, true);
	// Cuando el arma ya ha caído, pasa a ser un arma del suelo que se puede recoger (o coger su munición)
	if (EnemyRole != EBLEnemyRole::Ally && DropWeaponData)
	{
		const int32 Magazine = Weapon->GetMagazine();
		FTimerHandle DropTimer;
		GetWorldTimerManager().SetTimer(DropTimer, FTimerDelegate::CreateWeakLambda(this, [this, Magazine]() { SpawnWeaponPickup(Magazine); }), 1.2f, false);
	}
	if (AController* C = GetController())
	{
		C->UnPossess();
	}
	SetLifeSpan(90.f);
}

void ABLEnemyCharacter::SpawnWeaponPickup(int32 Magazine)
{
	FBLWeaponSlot Slot;
	Slot.Data = DropWeaponData;
	Slot.Magazine = FMath::Clamp(Magazine, 0, DropWeaponData->MagazineSize);
	// Lo que llevaba en las cartucheras: uno o dos cargadores
	Slot.Reserve = DropWeaponData->MagazineSize * FMath::RandRange(1, 2);
	if (ABLWeaponPickup::SpawnDrop(this, Slot, WeaponMesh->Bounds.Origin))
	{
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetVisibility(false);
	}
}

void ABLEnemyCharacter::SetVoiceComponent(UAudioComponent* Component)
{
	VoiceComponent = Component;
}
