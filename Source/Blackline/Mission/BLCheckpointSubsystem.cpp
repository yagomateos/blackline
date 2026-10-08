#include "Mission/BLCheckpointSubsystem.h"

#include "Blackline.h"
#include "Player/BLCharacter.h"

bool UBLCheckpointSubsystem::SaveCheckpoint(FName Id, const FTransform& InRespawnTransform, const ABLCharacter* Player)
{
	if (bHasCheckpoint && Id == CurrentId)
	{
		return false;
	}
	bHasCheckpoint = true;
	CurrentId = Id;
	RespawnTransform = InRespawnTransform;
	RespawnTransform.SetScale3D(FVector::OneVector);
	if (Player && Player->GetWeapon())
	{
		SavedInventory = Player->GetWeapon()->GetInventorySnapshot();
		SavedWeaponIndex = Player->GetWeapon()->GetCurrentIndex();
	}
	LastReachedTime = GetWorld()->GetTimeSeconds();
	UE_LOG(LogBlackline, Log, TEXT("Checkpoint '%s' en %s"), *Id.ToString(), *RespawnTransform.GetLocation().ToString());
	OnCheckpointReached.Broadcast(Id);
	return true;
}

void UBLCheckpointSubsystem::RegisterStart(const ABLCharacter* Player)
{
	if (!bHasCheckpoint && Player)
	{
		SaveCheckpoint(FName("Inicio"), Player->GetActorTransform(), Player);
		LastReachedTime = -100.f; // el inicio no muestra aviso
	}
}

void UBLCheckpointSubsystem::RespawnPlayer(ABLCharacter* Player)
{
	if (!Player)
	{
		return;
	}
	const FTransform T = bHasCheckpoint ? RespawnTransform : Player->GetActorTransform();
	Player->RespawnAt(T, SavedInventory, SavedWeaponIndex);
}
