#include "Audio/BLAudioSubsystem.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "AI/BLSquadSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"

namespace
{
	const TCHAR* BarkFolder = TEXT("/Game/Audio/Voice/Barks/");
	const TCHAR* BulletFolder = TEXT("/Game/Audio/Weapons/Bullet/");
	const TCHAR* DistantFolder = TEXT("/Game/Audio/Ambience/Distant/");
	const TCHAR* MusicFolder = TEXT("/Game/Audio/Music/");

	/** Hueco mínimo entre dos frases de la misma categoría (cualquier enemigo). */
	float CategoryGap(EBLBark Bark)
	{
		switch (Bark)
		{
		case EBLBark::Hit: return 1.5f;
		case EBLBark::ManDown: return 3.f;
		case EBLBark::Reload: return 3.f;
		case EBLBark::Contact: return 6.f;
		default: return 4.f;
		}
	}
}

UBLAudioSubsystem* UBLAudioSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UBLAudioSubsystem>() : nullptr;
}

bool UBLAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

const TCHAR* UBLAudioSubsystem::BarkName(EBLBark Bark)
{
	static const TCHAR* Names[] = { TEXT("Suspicious"), TEXT("HeardShots"), TEXT("Contact"), TEXT("Hit"), TEXT("Reload"), TEXT("Flank"),
		TEXT("Reposition"), TEXT("Chase"), TEXT("GiveUp"), TEXT("Lost"), TEXT("ManDown") };
	static_assert(UE_ARRAY_COUNT(Names) == (int32)EBLBark::MAX, "Faltan nombres de barks");
	return Names[(int32)Bark];
}

USoundBase* UBLAudioSubsystem::LoadSound(const FString& Name, const TCHAR* Folder)
{
	USoundBase* S = LoadObject<USoundBase>(nullptr, *(FString(Folder) + Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (S)
	{
		Loaded.Add(S);
	}
	return S;
}

void UBLAudioSubsystem::LoadList(TArray<TObjectPtr<USoundBase>>& Out, const FString& Prefix, const TCHAR* Folder, int32 Max)
{
	for (int32 i = 1; i <= Max; ++i)
	{
		if (USoundBase* S = LoadSound(FString::Printf(TEXT("%s%02d"), *Prefix, i), Folder))
		{
			Out.Add(S);
		}
	}
}

void UBLAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (double& T : LastBarkByCategory)
	{
		T = -100.0;
	}
	LoadList(CrackSounds, TEXT("SW_Bullet_Crack_"), BulletFolder, 8);
	LoadList(WhizSounds, TEXT("SW_Bullet_Whiz_"), BulletFolder, 8);
	LoadList(DistantBursts, TEXT("SW_Dist_Burst_"), DistantFolder, 12);
	LoadList(DistantExplosions, TEXT("SW_Dist_Explosion_"), DistantFolder, 8);
	Siren = LoadSound(TEXT("SW_Dist_Siren_01"), DistantFolder);
	MusicCombat = LoadSound(TEXT("SW_Mus_Combat_Loop"), MusicFolder);
	StingerContact = LoadSound(TEXT("SW_Mus_Stinger_Contact"), MusicFolder);
	StingerComplete = LoadSound(TEXT("SW_Mus_Stinger_Complete"), MusicFolder);
	RadioDuckMix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Audio/Settings/SMix_RadioDuck.SMix_RadioDuck"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	DistantTimer = FMath::FRandRange(3.f, 7.f);
	SirenTimer = FMath::FRandRange(25.f, 45.f);
	UE_LOG(LogBlackline, Log, TEXT("[Audio] Balas %d/%d, lejanos %d+%d, musica %d, mezcla radio %d"),
		CrackSounds.Num(), WhizSounds.Num(), DistantBursts.Num(), DistantExplosions.Num(), MusicCombat != nullptr, RadioDuckMix != nullptr);
}

void UBLAudioSubsystem::Deinitialize()
{
	if (bDucking && RadioDuckMix)
	{
		UGameplayStatics::PopSoundMixModifier(this, RadioDuckMix);
	}
	Super::Deinitialize();
}

bool UBLAudioSubsystem::GetListener(FVector& OutLocation, APawn*& OutPawn) const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = PC->PlayerCameraManager->GetCameraLocation();
	OutPawn = PC->GetPawn();
	return true;
}

// ---------------------------------------------------------------------------
// Barks
// ---------------------------------------------------------------------------

const TArray<TObjectPtr<USoundBase>>& UBLAudioSubsystem::GetBarkSounds(EBLBark Bark, int32 VoiceIndex)
{
	const FString Key = FString::Printf(TEXT("%s_V%d"), BarkName(Bark), VoiceIndex + 1);
	if (TArray<TObjectPtr<USoundBase>>* Found = BarkSounds.Find(Key))
	{
		return *Found;
	}
	TArray<TObjectPtr<USoundBase>>& List = BarkSounds.Add(Key);
	for (int32 i = 1; i <= 6; ++i)
	{
		if (USoundBase* S = LoadSound(FString::Printf(TEXT("VO_Bark_%s_%d"), *Key, i), BarkFolder))
		{
			List.Add(S);
		}
	}
	return List;
}

int32 UBLAudioSubsystem::CountBarkSounds(EBLBark Bark, int32 VoiceIndex)
{
	return GetBarkSounds(Bark, VoiceIndex).Num();
}

UAudioComponent* UBLAudioSubsystem::PlayBark(AActor* Speaker, int32 VoiceIndex, EBLBark Bark, const TCHAR* LogText)
{
	if (!Speaker)
	{
		return nullptr;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const int32 Cat = (int32)Bark;
	FVector Listener;
	APawn* ListenerPawn = nullptr;
	const bool bAudible = GetListener(Listener, ListenerPawn) && FVector::Dist(Listener, Speaker->GetActorLocation()) < BarkAudibleDistance;
	// Una frase cada vez y sin repetir la misma categoría en seguida (seis milicianos gritando "¡Contacto!" a la vez suena falso).
	// Los gritos de dolor y las bajas no esperan turno
	const bool bUrgent = Bark == EBLBark::Hit || Bark == EBLBark::ManDown;
	if (!bAudible || (!bUrgent && Now - LastBarkTime < BarkGlobalGap) || Now - LastBarkByCategory[Cat] < CategoryGap(Bark))
	{
		++BarksSkipped;
		return nullptr;
	}
	const TArray<TObjectPtr<USoundBase>>& Sounds = GetBarkSounds(Bark, VoiceIndex);
	if (Sounds.Num() == 0)
	{
		++BarksSkipped;
		return nullptr;
	}
	LastBarkTime = Now;
	LastBarkByCategory[Cat] = Now;
	++BarksPlayed;
	++BarkCounts[Cat];
	UE_LOG(LogBlackline, Log, TEXT("[Bark] %s (voz %d): %s"), *GetNameSafe(Speaker), VoiceIndex + 1, LogText ? LogText : BarkName(Bark));

	USoundBase* Sound = Sounds[FMath::RandHelper(Sounds.Num())];
	USceneComponent* Attach = Speaker->GetRootComponent();
	FName Socket = NAME_None;
	if (const ACharacter* C = Cast<ACharacter>(Speaker))
	{
		Attach = C->GetMesh();
		Socket = FName("head");
	}
	return UGameplayStatics::SpawnSoundAttached(Sound, Attach, Socket, FVector::ZeroVector, EAttachLocation::SnapToTarget, true,
		1.f, FMath::FRandRange(0.97f, 1.03f));
}

void UBLAudioSubsystem::NotifyEnemyKilled(AActor* Victim)
{
	const UBLSquadSubsystem* Squad = GetWorld()->GetSubsystem<UBLSquadSubsystem>();
	if (!Victim || !Squad)
	{
		return;
	}
	ABLEnemyCharacter* Best = nullptr;
	float BestDist = 3000.f;
	for (const TWeakObjectPtr<ABLAIController>& AI : Squad->GetMembers())
	{
		ABLEnemyCharacter* E = AI.IsValid() ? Cast<ABLEnemyCharacter>(AI->GetPawn()) : nullptr;
		if (E && E != Victim && !E->IsDead())
		{
			const float D = FVector::Dist(E->GetActorLocation(), Victim->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = E;
			}
		}
	}
	if (Best)
	{
		Pending.Add({ Best, Best->GetVoiceIndex(), EBLBark::ManDown, FMath::FRandRange(0.5f, 0.9f) });
	}
}

// ---------------------------------------------------------------------------
// Balas cerca del jugador
// ---------------------------------------------------------------------------

void UBLAudioSubsystem::NotifyBulletPass(const FVector& Start, const FVector& End, const AActor* HitActor)
{
	FVector Listener;
	APawn* ListenerPawn = nullptr;
	if (!GetListener(Listener, ListenerPawn) || CrackSounds.Num() == 0 || (HitActor && HitActor == ListenerPawn))
	{
		return;
	}
	// Punto de la trayectoria más cercano al oyente; tiene que pasarlo de largo (no cuenta si se queda corta)
	const FVector Seg = End - Start;
	const float Len2 = Seg.SizeSquared();
	if (Len2 < 1.f)
	{
		return;
	}
	const float T = FVector::DotProduct(Listener - Start, Seg) / Len2;
	if (T <= 0.f || T >= 1.f)
	{
		return;
	}
	const FVector Closest = Start + Seg * T;
	const float Dist = FVector::Dist(Closest, Listener);
	const double Now = GetWorld()->GetTimeSeconds();
	if (Dist > 350.f || Now - LastCrackTime < 0.05)
	{
		return;
	}
	LastCrackTime = Now;
	++BulletPasses;
	const float Volume = FMath::GetMappedRangeValueClamped(FVector2D(40.f, 350.f), FVector2D(1.f, 0.35f), Dist);
	UGameplayStatics::PlaySoundAtLocation(this, CrackSounds[FMath::RandHelper(CrackSounds.Num())], Closest, Volume, FMath::FRandRange(0.92f, 1.08f));
	if (WhizSounds.Num() > 0 && Dist < 200.f && FMath::FRand() < 0.6f)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WhizSounds[FMath::RandHelper(WhizSounds.Num())], Closest + Seg.GetSafeNormal() * 150.f, Volume * 0.8f, FMath::FRandRange(0.9f, 1.1f));
	}
}

// ---------------------------------------------------------------------------
// Mezcla y música
// ---------------------------------------------------------------------------

void UBLAudioSubsystem::SetRadioTalking(bool bTalking)
{
	if (bTalking == bDucking || !RadioDuckMix)
	{
		return;
	}
	bDucking = bTalking;
	if (bTalking)
	{
		UGameplayStatics::PushSoundMixModifier(this, RadioDuckMix);
	}
	else
	{
		UGameplayStatics::PopSoundMixModifier(this, RadioDuckMix);
	}
}

void UBLAudioSubsystem::OnMissionComplete()
{
	if (bMissionDone)
	{
		return;
	}
	bMissionDone = true;
	if (MusicComponent)
	{
		MusicComponent->FadeOut(2.5f, 0.f);
	}
	bCombatMusic = false;
	if (StingerComplete)
	{
		UGameplayStatics::PlaySound2D(this, StingerComplete, MusicVolume * 1.4f);
		++StingersPlayed;
	}
}

void UBLAudioSubsystem::TickMusic(float DeltaTime)
{
	if (bMissionDone || !MusicCombat)
	{
		return;
	}
	// Combate = algún enemigo vivo en estado de combate
	bool bAnyCombat = false;
	if (const UBLSquadSubsystem* Squad = GetWorld()->GetSubsystem<UBLSquadSubsystem>())
	{
		for (const TWeakObjectPtr<ABLAIController>& AI : Squad->GetMembers())
		{
			bAnyCombat |= AI.IsValid() && AI->IsInCombat();
		}
	}
	NoCombatTime = bAnyCombat ? 0.f : NoCombatTime + DeltaTime;
	if (!bCombatMusic)
	{
		SinceCombatEnd += DeltaTime;
	}

	if (bAnyCombat && !bCombatMusic)
	{
		bCombatMusic = true;
		// Golpe de entrada solo si hace rato del último combate (no en cada escaramuza encadenada)
		if (StingerContact && SinceCombatEnd > 25.f)
		{
			UGameplayStatics::PlaySound2D(this, StingerContact, MusicVolume * 1.5f);
			++StingersPlayed;
		}
		if (!MusicComponent)
		{
			MusicComponent = UGameplayStatics::SpawnSound2D(this, MusicCombat, MusicVolume, 1.f, 0.f, nullptr, false, false);
		}
		if (MusicComponent)
		{
			MusicComponent->FadeIn(1.5f, MusicVolume);
		}
		UE_LOG(LogBlackline, Log, TEXT("[Audio] Musica de combate: entra"));
	}
	else if (bCombatMusic && NoCombatTime > 5.f)
	{
		bCombatMusic = false;
		SinceCombatEnd = 0.f;
		if (MusicComponent)
		{
			MusicComponent->FadeOut(7.f, 0.f);
		}
		UE_LOG(LogBlackline, Log, TEXT("[Audio] Musica de combate: sale"));
	}
}

void UBLAudioSubsystem::TickDistant(float DeltaTime)
{
	FVector Listener;
	APawn* ListenerPawn = nullptr;
	if (!bDistantBattle || !GetListener(Listener, ListenerPawn))
	{
		return;
	}
	auto FarPoint = [&Listener]()
	{
		const float Yaw = FMath::FRandRange(0.f, 360.f);
		return Listener + FRotator(0.f, Yaw, 0.f).Vector() * FMath::FRandRange(15000.f, 25000.f) + FVector(0.f, 0.f, 1500.f);
	};
	DistantTimer -= DeltaTime;
	if (DistantTimer <= 0.f)
	{
		DistantTimer = FMath::FRandRange(DistantMinInterval, DistantMaxInterval);
		const bool bExplosion = DistantExplosions.Num() > 0 && FMath::FRand() < 0.22f;
		const TArray<TObjectPtr<USoundBase>>& List = bExplosion ? DistantExplosions : DistantBursts;
		if (List.Num() > 0)
		{
			UGameplayStatics::PlaySoundAtLocation(this, List[FMath::RandHelper(List.Num())], FarPoint(), FMath::FRandRange(0.6f, 1.f), FMath::FRandRange(0.92f, 1.06f));
			++DistantEvents;
		}
	}
	SirenTimer -= DeltaTime;
	if (Siren && SirenTimer <= 0.f)
	{
		SirenTimer = FMath::FRandRange(150.f, 240.f);
		UGameplayStatics::PlaySoundAtLocation(this, Siren, FarPoint(), 0.8f);
		++DistantEvents;
	}
}

void UBLAudioSubsystem::Tick(float DeltaTime)
{
	for (int32 i = Pending.Num() - 1; i >= 0; --i)
	{
		Pending[i].Delay -= DeltaTime;
		if (Pending[i].Delay <= 0.f)
		{
			const FPendingBark P = Pending[i];
			Pending.RemoveAt(i);
			const ABLEnemyCharacter* E = Cast<ABLEnemyCharacter>(P.Speaker.Get());
			if (E && !E->IsDead())
			{
				PlayBark(P.Speaker.Get(), P.Voice, P.Bark, nullptr);
			}
		}
	}
	TickMusic(DeltaTime);
	TickDistant(DeltaTime);
}
