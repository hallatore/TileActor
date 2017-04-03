#pragma once

#include "TileActor.h"
#include "FoliageTileBlockingVolume.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FoliageGroupTileActor.generated.h"

USTRUCT()
struct FFoliageGroupSpawnNoise
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
		int32 NoiseSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float Min;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float Max;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float FallOff;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
		int32 Seed;

	FFoliageGroupSpawnNoise()
	{
		NoiseSize = 1000;
		Min = 0.0f;
		Max = 1.0f;
		FallOff = 0.0f;
		Seed = 0;
	}
};

USTRUCT()
struct FFoliageGroupScaleNoise
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
		int32 NoiseSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float Min;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float Max;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
		int32 Seed;

	FFoliageGroupScaleNoise()
	{
		NoiseSize = 1000;
		Min = 1.0f;
		Max = 1.0f;
		Seed = 0;
	}
};

USTRUCT()
struct FFoliageGroupItem
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<UStaticMesh*> Meshes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		FFoliageGroupScaleNoise Scale;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
		int Width;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		float CalculatedWidth;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
		int Spacing;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float OffsetFactor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float SpawnChance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FFoliageGroupSpawnNoise> Noise;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
		uint32 CastShadow : 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool AffectDistanceFieldLighting;

	FFoliageGroupItem()
	{
		Scale = FFoliageGroupScaleNoise();
		Width = 10;
		Spacing = 2;
		OffsetFactor = 0.3f;
		SpawnChance = 1.0f;
	}
};

USTRUCT()
struct FFoliageGroup
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FFoliageGroupItem> Items;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FFoliageGroupSpawnNoise> Noise;

	FFoliageGroup()
	{
	}
};

USTRUCT()
struct FSortItem
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		int Index;

	UPROPERTY()
		int Width;

	FSortItem()
	{
	}

	FSortItem(int index, int width)
	{
		Index = index;
		Width = width;
	}
};

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

USTRUCT()
struct FItemSpawnSpace
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		bool IsSpace;

	UPROPERTY()
		int FailedDistanceY;

	FItemSpawnSpace()
	{
	}

	FItemSpawnSpace(bool isSpace, int failedDistanceY)
	{
		IsSpace = isSpace;
		FailedDistanceY = failedDistanceY;
	}
};

const int collisionGridSize = 400;
typedef TArray<bool, TInlineAllocator<80000>> GridArrayType;

UCLASS()
class LANDSCAPE_1_API AFoliageGroupTileActor : public ATileActor
{
	GENERATED_BODY()
	
public:	
	AFoliageGroupTileActor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
		TArray<FFoliageGroup> Groups;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

protected:
	virtual void UpdateTile(int32 x, int32 y, FVector location) override;
	virtual void Load() override;
	virtual void Unload() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced")
		int32 Seed;

private:
	uint32 Hash(uint32 a);
	void ClearInstances(UFoliageGroupTile* tile);
	float GetItemMinWidth();
	FTransform GetTransform(FVector location, uint32 seed, TArray<AActor*> actorsToIgnore);
	FItemSpawnSpace CalculateSpawnSpace(GridArrayType& collisionGrid, int size, int x, int y, int spacing);
	float GetFallOffSpawnChance(FFoliageGroupSpawnNoise& noise, float noiseValue);
	void Spawn(GridArrayType& collisionGrid, int size, int x, int y, int width);

	UPROPERTY(transient)
		TArray<UFoliageGroupTile*> FoliageGroupTiles;
};
