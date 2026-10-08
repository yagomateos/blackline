#include "Mission/BLCheckpointVolume.h"

#include "Mission/BLCheckpointSubsystem.h"
#include "Player/BLCharacter.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"

ABLCheckpointVolume::ABLCheckpointVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(150.f, 150.f, 120.f));
	Box->SetCollisionProfileName(FName("Trigger"));
	RootComponent = Box;
	RespawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(Box);
	RespawnPoint->SetRelativeLocation(FVector(0.f, 0.f, -28.f)); // a la altura de la cápsula del jugador (caja de 120 de semialtura)
}

void ABLCheckpointVolume::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddDynamic(this, &ABLCheckpointVolume::HandleOverlap);
}

void ABLCheckpointVolume::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABLCharacter* Player = Cast<ABLCharacter>(OtherActor);
	if (!Player || Player->IsDead() || !Player->IsPlayerControlled())
	{
		return;
	}
	if (UBLCheckpointSubsystem* Checkpoints = GetWorld()->GetSubsystem<UBLCheckpointSubsystem>())
	{
		const FName Id = CheckpointId.IsNone() ? GetFName() : CheckpointId;
		const FTransform T(FRotator(0.f, RespawnPoint->GetComponentRotation().Yaw, 0.f), RespawnPoint->GetComponentLocation());
		Checkpoints->SaveCheckpoint(Id, T, Player);
	}
}
