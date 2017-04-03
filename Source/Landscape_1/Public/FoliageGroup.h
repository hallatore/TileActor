#pragma once

#include "FoliageGroup.generated.h"

const int collisionGridSize = 400;
typedef TArray<bool> GridArrayType;

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
	
	/** A value of 0.0 causes this to be calculated automatically. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float CullDistance;

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

USTRUCT()
struct FTileTaskResultItem
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		FVector Location;

	UPROPERTY()
		int GroupIndex;

	UPROPERTY()
		int ItemIndex;

	UPROPERTY()
		float Scale;

	UPROPERTY()
		uint32 Seed;

	FTileTaskResultItem()
	{
	}

	FTileTaskResultItem(FVector location, int groupIndex, int itemIndex, float scale, uint32 seed)
	{
		Location = location;
		GroupIndex = groupIndex;
		ItemIndex = itemIndex;
		Scale = scale;
		Seed = seed;
	}
};



USTRUCT()
struct FTileTaskResult
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		int TileIndex;

	UPROPERTY()
		bool ShouldRender;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		TArray<FTileTaskResultItem> Items;

	FTileTaskResult()
	{
	}

	FTileTaskResult(int tileIndex, bool shouldRender, FVector location)
	{
		TileIndex = tileIndex;
		ShouldRender = shouldRender;
		Location = location;
	}
};