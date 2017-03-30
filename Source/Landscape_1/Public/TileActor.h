#pragma once

#include "GameFramework/Actor.h"
#include "TileActor.generated.h"

USTRUCT()
struct FTile
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		FVector Location;

	UPROPERTY()
		FVector NewLocation;

	UPROPERTY()
		bool ShouldUpdate;

	FTile()
	{
	}
};

USTRUCT()
struct FTileInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		int32 X;

	UPROPERTY()
		int32 Y;

	FTileInfo()
	{
		X = 0;
		Y = 0;
	}

	FTileInfo(int32 x, int32 y)
	{
		X = x;
		Y = y;
	}
};

UCLASS()
class LANDSCAPE_1_API ATileActor : public AActor
{
	GENERATED_BODY()

public:
	ATileActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced")
		bool RenderInEditor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawning")
		float Radius;

protected:
	UPROPERTY(transient)
		int32 Size;

	UPROPERTY(transient)
		bool IsLoaded;

	UPROPERTY(transient)
		bool IsLoadedFromBeginPlay;

	int32 GetIndex(int32 x, int32 y);
	virtual void UpdateTile(int32 x, int32 y, FVector location);
	virtual void PostUpdateTiles();
	virtual void Load();
	virtual void Reload();
	virtual void Unload();

private:
	UPROPERTY(transient)
		int32 CurrentTileX;

	UPROPERTY(transient)
		int32 CurrentTileY;

	UPROPERTY(transient)
		FVector CurrentTileLocation;

	UPROPERTY(transient)
		FVector CurrentCameraLocation;

	UPROPERTY(transient)
		TArray<FTile> Tiles;

	UPROPERTY(transient)
		int32 CurrentUpdateIndex;

	FVector GetTileLocation(int32 x, int32 y);
	int32 ConvertTileIndex(int32 index);
	FTileInfo GetClosestTileToUpdate();
};
