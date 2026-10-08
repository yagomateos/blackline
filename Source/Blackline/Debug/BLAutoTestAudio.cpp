// Prueba automática "Audio" (Bloque 7): mapa /Game/Maps/M01/L_M01_AmanecerRoto.
// Se ejecuta con -NoSound: comprueba la lógica (qué suena, cuándo y dónde) y que los sonidos existen, no el mezclador.
#include "Debug/BLAutoTestComponent.h"

#include "Blackline.h"
#include "AI/BLAIController.h"
#include "AI/BLEnemyCharacter.h"
#include "Audio/BLAudioSubsystem.h"
#include "Audio/BLReverbZone.h"
#include "Combat/BLHealthComponent.h"
#include "Mission/BLMissionDirector.h"
#include "Player/BLCharacter.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"

namespace
{
	ABLEnemyCharacter* AudioTestEnemy(UWorld* World, const TCHAR* Name)
	{
		for (TActorIterator<ABLEnemyCharacter> It(World); It; ++It)
		{
			if (It->Tags.Contains(FName(*(FString(TEXT("BLEnemy_")) + Name))))
			{
				return *It;
			}
		}
		return nullptr;
	}

	void KillEnemy(ABLEnemyCharacter* E, AController* By, AActor* Causer)
	{
		if (E && !E->IsDead())
		{
			FPointDamageEvent Ev(500.f, FHitResult(), FVector(1.f, 0.f, 0.f), nullptr);
			E->TakeDamage(500.f, Ev, By, Causer);
		}
	}
}

void UBLAutoTestComponent::BuildAudioTest()
{
	Char()->GetHealth()->bInvulnerable = true;
	auto Audio = [this]() { return UBLAudioSubsystem::Get(this); };
	auto Place = [this](const FVector& Loc, float Yaw)
	{
		ABLCharacter* C = Char();
		C->GetCharacterMovement()->StopMovementImmediately();
		C->SetActorLocation(Loc + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PC = C->GetController()) { PC->SetControlRotation(FRotator(0.f, Yaw, 0.f)); }
	};
	struct FAudioState { int32 Steps = 0; int32 Barks = 0; int32 Passes = 0; };
	TSharedRef<FAudioState> St = MakeShared<FAudioState>();

	// Todos los sonidos que se cargan por nombre existen (3 voces x 11 categorías, balas, música) y la radio tiene voz
	Steps.Add({ TEXT("Recursos"), 0.5f, nullptr, nullptr,
		[this, Audio](FString& D)
		{
			UBLAudioSubsystem* A = Audio();
			if (!A) { D = TEXT("sin subsistema de audio"); return false; }
			int32 MissingBarks = 0, BarkSounds = 0;
			for (int32 B = 0; B < (int32)EBLBark::MAX; ++B)
			{
				for (int32 V = 0; V < 3; ++V)
				{
					const int32 N = A->CountBarkSounds((EBLBark)B, V);
					BarkSounds += N;
					MissingBarks += N < 2 ? 1 : 0;
				}
			}
			int32 Lines = 0, Voiced = 0;
			if (const ABLMissionDirector* M = ABLMissionDirector::Get(this))
			{
				auto Count = [&Lines, &Voiced](const TArray<FBLRadioLine>& L) { for (const FBLRadioLine& R : L) { ++Lines; Voiced += R.Voice ? 1 : 0; } };
				Count(M->Briefing);
				Count(M->Debriefing);
				for (const FBLObjective& O : M->Objectives) { Count(O.RadioOnStart); Count(O.RadioOnComplete); }
			}
			// Mezclas que se cargan por nombre (deben estar cocinadas en la build)
			const bool bMixes = LoadObject<USoundMix>(nullptr, TEXT("/Game/Audio/Settings/SMix_User.SMix_User")) != nullptr
				&& LoadObject<USoundMix>(nullptr, TEXT("/Game/Audio/Settings/SMix_RadioDuck.SMix_RadioDuck")) != nullptr;
			const bool bMenuSounds = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/UI/SW_UI_MenuSelect.SW_UI_MenuSelect")) != nullptr;
			D = FString::Printf(TEXT("barks %d sonidos (%d grupos sin variaciones), radio %d/%d con voz, mezclas=%d, sonidos de menú=%d"),
				BarkSounds, MissingBarks, Voiced, Lines, bMixes, bMenuSounds);
			return MissingBarks == 0 && Lines > 0 && Voiced == Lines && bMixes && bMenuSounds;
		} });

	// Briefing: la voz suena tras el clic, el subtítulo dura lo que la voz y la mezcla baja ambiente/música
	Steps.Add({ TEXT("RadioVoz"), 3.0f, nullptr, nullptr,
		[this, Audio](FString& D)
		{
			const ABLMissionDirector* M = ABLMissionDirector::Get(this);
			FString Speaker, Text;
			float Alpha;
			const bool bSub = M && M->GetSubtitle(Speaker, Text, Alpha);
			D = FString::Printf(TEXT("voces %d, subtitulo=%d (%s), atenuacion por radio=%d"), M ? M->GetVoiceLinesPlayed() : 0, bSub, *Speaker, Audio() && Audio()->IsRadioDucking());
			return M && M->GetVoiceLinesPlayed() >= 1 && bSub && Audio() && Audio()->IsRadioDucking();
		}, nullptr, 2.5f });

	// Emisores de ambiente colocados con su sonido
	Steps.Add({ TEXT("Ambiente"), 0.3f, nullptr, nullptr,
		[this](FString& D)
		{
			int32 N = 0, WithSound = 0, Zones = 0;
			for (TActorIterator<AAmbientSound> It(GetWorld()); It; ++It)
			{
				++N;
				WithSound += It->GetAudioComponent() && It->GetAudioComponent()->Sound ? 1 : 0;
			}
			for (TActorIterator<ABLReverbZone> It(GetWorld()); It; ++It) { Zones += It->Reverb ? 1 : 0; }
			D = FString::Printf(TEXT("%d emisores (%d con sonido), %d zonas acusticas"), N, WithSound, Zones);
			return N >= 15 && WithSound == N && Zones >= 8;
		} });

	// Acústica: dentro del local manda la reverb de interior; en el callejón A la de callejón; en la calle, ninguna
	auto ZoneStep = [this, Place](const TCHAR* Name, FVector Loc, const TCHAR* Expected)
	{
		Steps.Add({ Name, 0.6f, [Place, Loc]() { Place(Loc, 0.f); }, nullptr,
			[this, Expected](FString& D)
			{
				const ABLReverbZone* Z = ABLReverbZone::GetActiveZone(this);
				const FString Got = Z ? (Z->Tags.Num() > 0 ? Z->Tags[0].ToString() : Z->GetName()) : TEXT("(exterior)");   // etiqueta = nombre (las etiquetas de actor solo existen en el editor)
				D = FString::Printf(TEXT("zona activa: %s"), *Got);
				return FString(Expected).IsEmpty() ? Z == nullptr : Got == Expected;
			} });
	};
	ZoneStep(TEXT("AcusticaLocal"), FVector(20500.f, 600.f, 0.f), TEXT("Acustica_Local"));
	ZoneStep(TEXT("AcusticaCallejon"), FVector(3050.f, -1000.f, 0.f), TEXT("Acustica_Callejon_A"));
	ZoneStep(TEXT("AcusticaCalle"), FVector(11000.f, 600.f, 0.f), TEXT(""));

	// Pasos de la IA: la patrulla del patio suena al andar
	Steps.Add({ TEXT("PasosIA"), 3.0f,
		[this, St]() { St->Steps = AudioTestEnemy(GetWorld(), TEXT("Patio_Patrulla1")) ? AudioTestEnemy(GetWorld(), TEXT("Patio_Patrulla1"))->GetFootstepsPlayed() : 0; }, nullptr,
		[this, St](FString& D)
		{
			const ABLEnemyCharacter* E = AudioTestEnemy(GetWorld(), TEXT("Patio_Patrulla1"));
			const int32 N = E ? E->GetFootstepsPlayed() - St->Steps : 0;
			D = FString::Printf(TEXT("%d pasos en 3 s (velocidad %.0f)"), N, E ? E->GetVelocity().Size2D() : 0.f);
			return N >= 4;
		} });

	// Contacto: el guardia del local lo ve de frente -> grito de contacto, golpe musical y música de combate
	Steps.Add({ TEXT("Contacto"), 4.0f, [Place]() { Place(FVector(19750.f, 420.f, 0.f), 0.f); }, nullptr,
		[Audio](FString& D)
		{
			const UBLAudioSubsystem* A = Audio();
			D = A ? FString::Printf(TEXT("barks %d (contacto %d, descartados %d), musica de combate=%d, golpes %d"),
				A->GetBarksPlayed(), A->GetBarksPlayed(EBLBark::Contact), A->GetBarksSkipped(), A->IsCombatMusicOn(), A->GetStingersPlayed()) : TEXT("-");
			return A && A->GetBarksPlayed(EBLBark::Contact) >= 1 && A->IsCombatMusicOn() && A->GetStingersPlayed() >= 1;
		}, nullptr, 3.5f });

	// Tiroteo: a 20 m en la calle; las balas que fallan pasan rozando -> chasquidos
	Steps.Add({ TEXT("BalasCerca"), 10.0f,
		[this, Place, Audio, St]() { Place(FVector(16400.f, 300.f, 0.f), 0.f); St->Passes = Audio() ? Audio()->GetBulletPasses() : 0; }, nullptr,
		[Audio, St](FString& D)
		{
			const UBLAudioSubsystem* A = Audio();
			const int32 N = A ? A->GetBulletPasses() - St->Passes : 0;
			D = FString::Printf(TEXT("%d balas pasan a < 3,5 m; barks %d"), N, A ? A->GetBarksPlayed() : 0);
			return N >= 2;
		}, nullptr, 6.f });
	Steps.Last().Done = [Audio, St]() { return Audio() && Audio()->GetBulletPasses() - St->Passes >= 4; };

	// Baja: el compañero de sacos avisa
	Steps.Add({ TEXT("HombreAbajo"), 2.5f,
		[this, Audio, St]()
		{
			St->Barks = Audio() ? Audio()->GetBarksPlayed(EBLBark::ManDown) : 0;
			KillEnemy(AudioTestEnemy(GetWorld(), TEXT("Calle_Sacos1")), Char()->GetController(), Char());
		}, nullptr,
		[Audio, St](FString& D)
		{
			const int32 N = Audio() ? Audio()->GetBarksPlayed(EBLBark::ManDown) - St->Barks : 0;
			D = FString::Printf(TEXT("avisos de baja: %d"), N);
			return N >= 1;
		} });

	// Sin enemigos en combate, la música se va (5 s de margen + fundido)
	Steps.Add({ TEXT("FinCombate"), 12.0f,
		[this]()
		{
			for (TActorIterator<ABLEnemyCharacter> It(GetWorld()); It; ++It) { KillEnemy(*It, Char()->GetController(), Char()); }
		}, nullptr,
		[Audio](FString& D)
		{
			D = FString::Printf(TEXT("musica de combate=%d"), Audio() && Audio()->IsCombatMusicOn());
			return Audio() && !Audio()->IsCombatMusicOn();
		} });
	Steps.Last().Done = [Audio]() { return Audio() && !Audio()->IsCombatMusicOn(); };

	// La ciudad en guerra: ráfagas/explosiones lejanas durante la prueba
	Steps.Add({ TEXT("CombateLejano"), 0.3f, nullptr, nullptr,
		[Audio](FString& D)
		{
			D = FString::Printf(TEXT("%d sonidos lejanos"), Audio() ? Audio()->GetDistantEvents() : 0);
			return Audio() && Audio()->GetDistantEvents() >= 2;
		} });
}
