#include "Audio/BLReverbZone.h"

#include "Blackline.h"
#include "Core/BLGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/ReverbEffect.h"

ABLReverbZone::ABLReverbZone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetBoxExtent(FVector(300.f));
	Box->SetHiddenInGame(true);
	RootComponent = Box;
}

void ABLReverbZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	// Punto dentro de la caja (con rotación y escala del actor)
	const FVector Local = Box->GetComponentTransform().InverseTransformPosition(PC->PlayerCameraManager->GetCameraLocation());
	const FVector E = Box->GetUnscaledBoxExtent();
	SetInside(FMath::Abs(Local.X) <= E.X && FMath::Abs(Local.Y) <= E.Y && FMath::Abs(Local.Z) <= E.Z);
}

void ABLReverbZone::SetInside(bool bNewInside)
{
	if (bNewInside == bInside)
	{
		return;
	}
	bInside = bNewInside;
	const FName Tag(*FString::Printf(TEXT("Zona_%s"), *GetName()));
	if (bInside)
	{
		if (Reverb)
		{
			UGameplayStatics::ActivateReverbEffect(this, Reverb, Tag, Priority, ReverbVolume, FadeTime);
		}
	}
	else
	{
		UGameplayStatics::DeactivateReverbEffect(this, Tag);
	}
	if (!FMath::IsNearlyEqual(AmbienceScale, 1.f))
	{
		if (ABLGameMode* GM = Cast<ABLGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->SetAmbienceScale(bInside ? AmbienceScale : 1.f, FadeTime * 2.f);
		}
	}
	UE_LOG(LogBlackline, Verbose, TEXT("[Audio] %s zona %s"), bInside ? TEXT("Entra en") : TEXT("Sale de"), *GetName());
}

void ABLReverbZone::EndPlay(const EEndPlayReason::Type Reason)
{
	SetInside(false);
	Super::EndPlay(Reason);
}

ABLReverbZone* ABLReverbZone::GetActiveZone(const UObject* WorldContext)
{
	ABLReverbZone* Best = nullptr;
	for (TActorIterator<ABLReverbZone> It(WorldContext->GetWorld()); It; ++It)
	{
		if (It->IsListenerInside() && (!Best || It->Priority > Best->Priority))
		{
			Best = *It;
		}
	}
	return Best;
}
