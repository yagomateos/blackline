#include "AI/BLCoverPoint.h"

#include "Components/ArrowComponent.h"
#include "Engine/World.h"

ABLCoverPoint::ABLCoverPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(RootComponent);
	Arrow->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Arrow->ArrowColor = FColor(255, 160, 40);
	Arrow->bIsScreenSizeScaled = true;
	SetHidden(true);
}

bool ABLCoverPoint::HasLineOfSight(const FVector& From, const FVector& To) const
{
	// Canal Visibility: lo bloquea la geometría; los personajes y los volúmenes (checkpoints) no tapan
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BLCover), false);
	Q.AddIgnoredActor(this);
	return !GetWorld()->LineTraceTestByChannel(From, To, ECC_Visibility, Q);
}

bool ABLCoverPoint::IsProtectedFrom(const FVector& ThreatEye) const
{
	const FVector Base = GetActorLocation();
	const float Eye = bLowCover ? CrouchEye : StandEye;
	// Cabeza y hombros ocultos (dos rayos a la altura de los ojos y del pecho)
	return !HasLineOfSight(ThreatEye, Base + FVector(0.f, 0.f, Eye)) && !HasLineOfSight(ThreatEye, Base + FVector(0.f, 0.f, Eye - 35.f));
}

bool ABLCoverPoint::FindFirePosition(const FVector& ThreatPoint, FVector& OutLocation, bool& bOutStand) const
{
	const FVector Base = GetActorLocation();
	if (bLowCover)
	{
		// De pie, por encima de la cobertura
		if (HasLineOfSight(Base + FVector(0.f, 0.f, StandEye), ThreatPoint))
		{
			OutLocation = Base;
			bOutStand = true;
			return true;
		}
	}
	// Asomarse a un lado (derecha o izquierda respecto a la flecha), primero el que esté más cerca del objetivo
	const FVector Right = GetActorRightVector();
	const float Side = FVector::DotProduct(ThreatPoint - Base, Right) >= 0.f ? 1.f : -1.f;
	for (const float S : { Side, -Side })
	{
		for (const float Dist : { 80.f, 130.f, 190.f })
		{
			const FVector P = Base + Right * S * Dist;
			// se ve el objetivo desde ahí y no hay una pared entre el punto y la posición de asomarse
			if (HasLineOfSight(P + FVector(0.f, 0.f, StandEye), ThreatPoint)
				&& HasLineOfSight(Base + FVector(0.f, 0.f, 60.f), P + FVector(0.f, 0.f, 60.f)))
			{
				OutLocation = P;
				bOutStand = !bLowCover;
				return true;
			}
		}
	}
	return false;
}
