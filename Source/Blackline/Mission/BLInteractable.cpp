#include "Mission/BLInteractable.h"

#include "Blackline.h"
#include "Player/BLCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

float ABLInteractable::LastPhotoTime = -100.f;
int32 ABLInteractable::PhotosTaken = 0;

ABLInteractable::ABLInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(FName("BlockAll"));
	Mesh->SetMobility(EComponentMobility::Movable);
	static ConstructorHelpers::FObjectFinder<USoundBase> Pickup(TEXT("/Game/Audio/UI/SW_UI_Pickup.SW_UI_Pickup"));
	if (Pickup.Succeeded())
	{
		UseSound = Pickup.Object;
	}
}

void ABLInteractable::BeginPlay()
{
	Super::BeginPlay();
	// Nivel nuevo: el reloj del mundo vuelve a 0
	LastPhotoTime = -100.f;
	PhotosTaken = 0;
}

void ABLInteractable::Use(ABLCharacter* User)
{
	if (!CanInteract(User))
	{
		return;
	}
	bUsed = true;
	if (bPhoto)
	{
		LastPhotoTime = GetWorld()->GetTimeSeconds();
		++PhotosTaken;
	}
	if (UseSound)
	{
		UGameplayStatics::PlaySound2D(this, UseSound);
	}
	if (bPickup)
	{
		Mesh->SetVisibility(false, true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	UE_LOG(LogBlackline, Log, TEXT("Interacción: %s (%s)"), *GetName(), *Prompt);
	OnUsed.Broadcast(this, User);
}
