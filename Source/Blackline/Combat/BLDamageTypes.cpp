#include "Combat/BLDamageTypes.h"

#include "Components/CapsuleComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

EBLHitZone BLDamage::ZoneFromBone(FName Bone)
{
	if (Bone.IsNone())
	{
		return EBLHitZone::Torso;
	}
	const FString Name = Bone.ToString().ToLower();
	if (Name.StartsWith(TEXT("head")) || Name.StartsWith(TEXT("neck")))
	{
		return EBLHitZone::Head;
	}
	if (Name.StartsWith(TEXT("spine")) || Name.StartsWith(TEXT("pelvis")) || Name.StartsWith(TEXT("clavicle")) || Name == TEXT("root"))
	{
		return EBLHitZone::Torso;
	}
	return EBLHitZone::Limb;
}

void BLDamage::SetupCharacterCollision(ACharacter* Character)
{
	if (!Character)
	{
		return;
	}
	Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_BLWeapon, ECR_Ignore);
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		Mesh->SetCollisionProfileName(FName("CharacterMesh"));
		Mesh->SetCollisionResponseToChannel(ECC_BLWeapon, ECR_Block);
		Mesh->SetGenerateOverlapEvents(false);
	}
}

bool BLDamage::WeaponTrace(const UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, FCollisionQueryParams Params)
{
	if (!World)
	{
		return false;
	}
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		if (!World->LineTraceSingleByChannel(OutHit, Start, End, ECC_BLWeapon, Params))
		{
			return false;
		}
		const UPrimitiveComponent* Comp = OutHit.GetComponent();
		const bool bTriggerShape = Comp && Comp->IsA<UShapeComponent>() && !Cast<ACharacter>(Comp->GetOwner());
		const bool bInvisible = Comp && !Comp->IsA<USkeletalMeshComponent>() && (!Comp->IsVisible() || Comp->bHiddenInGame);
		if (!bTriggerShape && !bInvisible)
		{
			return true;
		}
		Params.AddIgnoredComponent(Comp);
	}
	return OutHit.bBlockingHit;
}
