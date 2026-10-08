#include "Combat/BLDamageTypes.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
