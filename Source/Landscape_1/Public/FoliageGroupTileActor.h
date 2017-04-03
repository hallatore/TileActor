#pragma once

#include "TileActor.h"
#include "FoliageGroup.h"
#include "FoliageTileBlockingVolume.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FoliageGroupTileActor.generated.h"


UCLASS()
class UFoliageGroupTileItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
		TArray<UHierarchicalInstancedStaticMeshComponent*> MeshComponents;
};

UCLASS()
class UFoliageGroupTileGroup : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
		TArray<UFoliageGroupTileItem*> Items;
};

UCLASS()
class UFoliageGroupTile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
		TArray<UFoliageGroupTileGroup*> Groups;
};

UCLASS()
class LANDSCAPE_1_API AFoliageGroupTileActor : public ATileActor
{
	GENERATED_BODY()
	
public:	
	AFoliageGroupTileActor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
		TArray<FFoliageGroup> Groups;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void UpdateTile(int32 x, int32 y, FVector location) override;
	virtual void Load() override;
	virtual void Unload() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced")
		int32 Seed;

private:
	void TaskResultCompleted(FTileTaskResult tileTaskResult);
	void ClearInstances(UFoliageGroupTile* tile);
	FTransform GetTransform(FVector location, uint32 seed, TArray<AActor*> actorsToIgnore);
	int GetClosestTileToRender(FVector currentLocation);

	UPROPERTY(transient)
		TArray<UFoliageGroupTile*> FoliageGroupTiles;

	UPROPERTY(transient)
		TArray<FTileTaskResult> TileTaskResults;
};
