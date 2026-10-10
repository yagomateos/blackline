#include "Mission/BLCampaignProgress.h"

#include "Blackline.h"
#include "UI/BLMenuData.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

namespace
{
	const TCHAR* SlotName = TEXT("BLCampaign");

	UBLCampaignSave* LoadSave()
	{
		if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		{
			if (UBLCampaignSave* Save = Cast<UBLCampaignSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
			{
				return Save;
			}
		}
		return Cast<UBLCampaignSave>(UGameplayStatics::CreateSaveGameObject(UBLCampaignSave::StaticClass()));
	}
}

int32 BLCampaign::NumMissions()
{
	return UE_ARRAY_COUNT(BLMenuData::Missions);
}

int32 BLCampaign::FindMission(const FString& LevelName)
{
	const FString Short = FPackageName::GetShortName(LevelName);
	for (int32 i = 0; i < NumMissions(); ++i)
	{
		const TCHAR* Map = BLMenuData::Missions[i].Map;
		if (Map && FPackageName::GetShortName(FString(Map)) == Short)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FString BLCampaign::MissionMap(int32 Mission)
{
	return Mission >= 0 && Mission < NumMissions() && BLMenuData::Missions[Mission].Map ? FString(BLMenuData::Missions[Mission].Map) : FString();
}

bool BLCampaign::IsCompleted(int32 Mission)
{
	const UBLCampaignSave* Save = LoadSave();
	return Save && Mission >= 0 && (Save->CompletedMask & (1 << Mission)) != 0;
}

bool BLCampaign::HasProgress()
{
	const UBLCampaignSave* Save = LoadSave();
	return Save && Save->CompletedMask != 0;
}

bool BLCampaign::IsCampaignComplete()
{
	const UBLCampaignSave* Save = LoadSave();
	const int32 All = (1 << NumMissions()) - 1;
	return Save && (Save->CompletedMask & All) == All;
}

int32 BLCampaign::NextMissionToPlay()
{
	const UBLCampaignSave* Save = LoadSave();
	if (!Save)
	{
		return 0;
	}
	auto Done = [Save](int32 i) { return (Save->CompletedMask & (1 << i)) != 0; };
	// La siguiente a la más alta completada; si ya está hecha (o no hay), la primera sin completar
	int32 Highest = INDEX_NONE;
	for (int32 i = 0; i < NumMissions(); ++i)
	{
		Highest = Done(i) ? i : Highest;
	}
	if (Highest + 1 < NumMissions() && !Done(Highest + 1))
	{
		return Highest + 1;
	}
	for (int32 i = 0; i < NumMissions(); ++i)
	{
		if (!Done(i))
		{
			return i;
		}
	}
	return 0;
}

void BLCampaign::MarkCompleted(int32 Mission, float Seconds)
{
	if (Mission < 0 || Mission >= NumMissions())
	{
		return;
	}
	// Las pruebas automáticas no tocan el progreso del jugador (salvo -BLSaveProgress, para probar el guardado)
	FString Test;
	if (FParse::Value(FCommandLine::Get(), TEXT("BLTest="), Test) && !FParse::Param(FCommandLine::Get(), TEXT("BLSaveProgress")))
	{
		UE_LOG(LogBlackline, Log, TEXT("[Campaña] misión %d completada (prueba automática: no se guarda)"), Mission + 1);
		return;
	}
	UBLCampaignSave* Save = LoadSave();
	if (!Save)
	{
		return;
	}
	Save->CompletedMask |= 1 << Mission;
	if (Save->BestTimes.Num() < NumMissions())
	{
		Save->BestTimes.SetNumZeroed(NumMissions());
	}
	float& Best = Save->BestTimes[Mission];
	Best = Best > 0.f ? FMath::Min(Best, Seconds) : Seconds;
	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	UE_LOG(LogBlackline, Log, TEXT("[Campaña] misión %d completada (%.0f s), progreso 0x%x, guardado=%d"), Mission + 1, Seconds, Save->CompletedMask, bOk);
}
