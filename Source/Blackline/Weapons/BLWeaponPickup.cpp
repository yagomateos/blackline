#include "Weapons/BLWeaponPickup.h"

#include "Blackline.h"
#include "Player/BLCharacter.h"
#include "Weapons/BLWeaponData.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABLWeaponPickup::ABLWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;   // solo actualiza el texto de la acción
	Prompt = TEXT("Coger");
	HoldTime = 0.35f;
	bPickup = false;
	// La raíz (malla estática de ABLInteractable) queda vacía; el arma es la malla esquelética, tumbada sobre el
	// lado izquierdo y apoyada en el suelo (ApplyMesh)
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(Mesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));   // giro sobre su eje adelante (Y): +X al suelo
}

void ABLWeaponPickup::ApplyMesh()
{
	USkeletalMesh* Asset = WeaponData ? WeaponData->Mesh.Get() : nullptr;
	WeaponMesh->SetSkeletalMeshAsset(Asset);
	if (Asset)
	{
		// Tumbada, el lado izquierdo (+X) es lo más bajo: se levanta lo justo para apoyarla en el suelo
		const FBoxSphereBounds B = Asset->GetBounds();
		WeaponMesh->SetRelativeLocation(FVector(0.f, 0.f, B.Origin.X + B.BoxExtent.X));
	}
}

void ABLWeaponPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyMesh();
}

void ABLWeaponPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyMesh();
	if (WeaponData)
	{
		Magazine = Magazine < 0 ? WeaponData->MagazineSize : Magazine;
		Reserve = Reserve < 0 ? WeaponData->StartReserveAmmo : Reserve;
	}
	UpdatePrompt();
}

void ABLWeaponPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatePrompt();
	// La munición de un arma que ya se lleva se coge sola al pasar por encima (no hace falta mantener F)
	ABLCharacter* Player = Cast<ABLCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UBLWeaponComponent* Weapons = Player && !Player->IsDead() ? Player->GetWeapon() : nullptr;
	if (Weapons && WeaponData && Weapons->FindWeapon(WeaponData) != INDEX_NONE && CanInteract(Player)
		&& FVector::DistSquared(Player->GetActorLocation(), GetInteractLocation()) < FMath::Square(AutoAmmoRadius))
	{
		Use(Player);
	}
}

void ABLWeaponPickup::UpdatePrompt()
{
	const ABLCharacter* Player = Cast<ABLCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UBLWeaponComponent* Weapons = Player ? Player->GetWeapon() : nullptr;
	if (!WeaponData || !Weapons)
	{
		return;
	}
	const FString Name = WeaponData->DisplayName.ToString();
	if (Weapons->FindWeapon(WeaponData) != INDEX_NONE)
	{
		Prompt = FString::Printf(TEXT("Coger munición (%s)"), *Name);
	}
	else if (Weapons->GetWeaponCount() < MaxCarried)
	{
		Prompt = FString::Printf(TEXT("Coger %s"), *Name);
	}
	else
	{
		const UBLWeaponData* Current = Weapons->GetWeaponData();
		Prompt = FString::Printf(TEXT("Cambiar %s por %s"), Current ? *Current->DisplayName.ToString() : TEXT("el arma"), *Name);
	}
}

bool ABLWeaponPickup::CanInteract(const ABLCharacter* User) const
{
	const UBLWeaponComponent* Weapons = User ? User->GetWeapon() : nullptr;
	if (!bEnabled || !WeaponData || !Weapons || Weapons->IsSwitching())
	{
		return false;
	}
	const int32 Own = Weapons->FindWeapon(WeaponData);
	if (Own != INDEX_NONE)
	{
		// Ya la lleva: solo si le cabe munición y el arma del suelo tiene
		const FBLWeaponSlot* Slot = Weapons->GetSlot(Own);
		return Magazine + Reserve > 0 && Slot && Slot->Reserve < WeaponData->MaxReserveAmmo;
	}
	return true;
}

FVector ABLWeaponPickup::GetInteractLocation() const
{
	return WeaponMesh->GetSkeletalMeshAsset() ? WeaponMesh->Bounds.Origin : GetActorLocation();
}

void ABLWeaponPickup::Use(ABLCharacter* User)
{
	UBLWeaponComponent* Weapons = User ? User->GetWeapon() : nullptr;
	if (!CanInteract(User) || !Weapons)
	{
		return;
	}
	if (UseSound)
	{
		UGameplayStatics::PlaySound2D(this, UseSound);
	}
	const int32 Own = Weapons->FindWeapon(WeaponData);
	if (Own != INDEX_NONE)
	{
		const int32 Taken = Weapons->AddReserveAmmo(Own, Magazine + Reserve);
		// Primero se vacía la reserva del arma del suelo y luego su cargador
		const int32 FromReserve = FMath::Min(Taken, Reserve);
		Reserve -= FromReserve;
		Magazine -= Taken - FromReserve;
		UE_LOG(LogBlackline, Log, TEXT("Munición de %s: +%d"), *GetNameSafe(WeaponData), Taken);
		if (Magazine + Reserve <= 0)
		{
			Destroy();
		}
		OnUsed.Broadcast(this, User);
		return;
	}

	FBLWeaponSlot NewSlot;
	NewSlot.Data = WeaponData;
	NewSlot.Magazine = Magazine;
	NewSlot.Reserve = Reserve;
	if (Weapons->GetWeaponCount() < MaxCarried)
	{
		Weapons->PickUpWeapon(NewSlot, INDEX_NONE);
	}
	else
	{
		// Cambio: la que lleva en la mano queda en el suelo donde estaba esta
		const int32 Current = Weapons->GetCurrentIndex();
		if (const FBLWeaponSlot* Old = Weapons->GetSlot(Current))
		{
			SpawnDrop(User, *Old, GetActorLocation());
		}
		Weapons->PickUpWeapon(NewSlot, Current);
	}
	UE_LOG(LogBlackline, Log, TEXT("Arma recogida: %s (%d+%d)"), *GetNameSafe(WeaponData), Magazine, Reserve);
	OnUsed.Broadcast(this, User);
	Destroy();
}

ABLWeaponPickup* ABLWeaponPickup::SpawnDrop(AActor* Dropper, const FBLWeaponSlot& Slot, const FVector& Near)
{
	UWorld* World = Dropper ? Dropper->GetWorld() : nullptr;
	if (!World || !Slot.Data)
	{
		return nullptr;
	}
	// En el suelo, girada al azar
	FVector Location = Near;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLWeaponDrop), false, Dropper);
	if (World->LineTraceSingleByChannel(Hit, Near + FVector(0.f, 0.f, 60.f), Near - FVector(0.f, 0.f, 300.f), ECC_Visibility, Q))
	{
		Location = Hit.ImpactPoint;
	}
	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Location);
	ABLWeaponPickup* Drop = World->SpawnActorDeferred<ABLWeaponPickup>(ABLWeaponPickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Drop)
	{
		Drop->WeaponData = Slot.Data;
		Drop->Magazine = Slot.Magazine;
		Drop->Reserve = Slot.Reserve;
		Drop->FinishSpawning(Transform);
	}
	return Drop;
}
