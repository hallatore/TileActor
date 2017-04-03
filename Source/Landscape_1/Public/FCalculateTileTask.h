#pragma once

#include "FoliageGroup.h"

DECLARE_DELEGATE_OneParam(FTileTaskResultDelegate, FTileTaskResult);

class LANDSCAPE_1_API FCalculateTileTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FCalculateTileTask>;

public:
	FCalculateTileTask(FVector location, int tileIndex, TArray<FFoliageGroup> groups, float tileSize, uint32 seed, FTileTaskResultDelegate taskResultDelegate)
	{
		Location = location;
		TileIndex = tileIndex;
		Groups = groups;
		TileSize = tileSize;
		Seed = seed;
		TaskResultDelegate = taskResultDelegate;
	}

protected:
	FVector Location;
	int TileIndex;
	TArray<FFoliageGroup> Groups;
	float TileSize;
	uint32 Seed;
	FTileTaskResultDelegate TaskResultDelegate;

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCalculateTileTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	float GetFallOffSpawnChance(FFoliageGroupSpawnNoise& noise, float noiseValue);
	FItemSpawnSpace CalculateSpawnSpace(GridArrayType& collisionGrid, int size, int x, int y, int spacing);
	void Spawn(GridArrayType& collisionGrid, int size, int x, int y, int width);
};
