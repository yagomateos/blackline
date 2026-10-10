#include "Environment/BLAlarmLight.h"

#include "Blackline.h"

#include "Components/AudioComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"

ABLAlarmLight::ABLAlarmLight()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Light = CreateDefaultSubobject<USpotLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetLightColor(FLinearColor(1.f, 0.93f, 0.8f));
	Light->SetVolumetricScatteringIntensity(1.5f);

	Siren = CreateDefaultSubobject<UAudioComponent>(TEXT("Siren"));
	Siren->SetupAttachment(Root);
	Siren->bAutoActivate = false;
}

void ABLAlarmLight::BeginPlay()
{
	Super::BeginPlay();
	Light->SetRelativeLocationAndRotation(LightOffset, FRotator(LightPitch, 0.f, 0.f));
	Siren->SetRelativeLocation(LightOffset);
	Light->SetAttenuationRadius(Radius);
	Light->SetInnerConeAngle(ConeAngle * 0.55f);
	Light->SetOuterConeAngle(ConeAngle);
	Light->SetCastShadows(bShadows);
	if (SirenSound)
	{
		Siren->SetSound(SirenSound);
	}
	if (bStartOn)
	{
		bOn = true;
		OnTime = 10.f;   // sin parpadeo de arranque
		Light->SetIntensity(Intensity);
	}
	SetActorTickEnabled(false);
}

void ABLAlarmLight::OnMissionActivate(FName Tag)
{
	if (bOn)
	{
		return;
	}
	bOn = true;
	OnTime = 0.f;
	SetActorTickEnabled(true);
	if (SirenSound)
	{
		Siren->Play(FMath::FRand() * 2.f);
	}
}

void ABLAlarmLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (OnTime < 0.f)
	{
		return;
	}
	OnTime += DeltaTime;
	// Arranque de lámpara de descarga: dos destellos, oscuridad y luego sube hasta el máximo
	float K = 1.f;
	if (OnTime < 0.08f || (OnTime > 0.22f && OnTime < 0.3f))
	{
		K = 0.8f;
	}
	else if (OnTime < 0.6f)
	{
		K = 0.f;
	}
	else
	{
		K = FMath::Clamp((OnTime - 0.6f) / 1.4f, 0.15f, 1.f);
	}
	Light->SetIntensity(Intensity * K);
	if (OnTime > 2.1f)
	{
		Light->SetIntensity(Intensity);
		SetActorTickEnabled(false);
	}
}
