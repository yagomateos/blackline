#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "BLCampaignProgress.generated.h"

/** Progreso de la campaña guardado en disco (Saved/SaveGames/BLCampaign.sav). */
UCLASS()
class BLACKLINE_API UBLCampaignSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bit i = misión i (orden de BLMenuData::Missions) completada al menos una vez. */
	UPROPERTY() int32 CompletedMask = 0;
	/** Mejor tiempo por misión en segundos (0 = sin completar). */
	UPROPERTY() TArray<float> BestTimes;
};

/** Consultas y escritura del progreso de campaña (menú y director de misión). */
namespace BLCampaign
{
	/** Índice de la misión cuyo mapa es LevelName (nombre corto o ruta), o INDEX_NONE. */
	BLACKLINE_API int32 FindMission(const FString& LevelName);
	BLACKLINE_API int32 NumMissions();
	BLACKLINE_API bool IsCompleted(int32 Mission);
	/** Misión que propone "CONTINUAR": la siguiente a la más alta completada (o la primera sin completar; 0 si no hay). */
	BLACKLINE_API int32 NextMissionToPlay();
	BLACKLINE_API bool HasProgress();
	BLACKLINE_API bool IsCampaignComplete();
	BLACKLINE_API void MarkCompleted(int32 Mission, float Seconds);
	/** Mapa de una misión (ruta /Game/...), o vacío. */
	BLACKLINE_API FString MissionMap(int32 Mission);
}
