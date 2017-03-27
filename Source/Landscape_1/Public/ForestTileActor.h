#pragma once

#include "TileActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ForestTileActor.generated.h"

USTRUCT()
struct FForestMesh
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		class UStaticMesh* Mesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float ScaleMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float ScaleMax;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float Bounds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float ZOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float HeightMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float HeightMax;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxSlope;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool AlignWithSlope;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool Collision;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float SpawnChanceVsOtherTrees;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 NumberOfSteps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 ChildrenPerTree;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float DeadTreePercentage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 GroupId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<UPhysicalMaterial*> PhysicalMaterials;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 InitialSeed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		int32 TotalMeshes;

	FForestMesh()
	{
		ScaleMin = 1.0f;
		ScaleMax = 1.0f;
		Bounds = 300.0f;
		HeightMin = INT32_MIN;
		HeightMax = INT32_MAX;
		MaxSlope = 30.0f;
		GroupId = 0;
		SpawnChanceVsOtherTrees = 1.0f;
		NumberOfSteps = 0;
		ChildrenPerTree = 0;
		DeadTreePercentage = 0.0f;
		AlignWithSlope = false;
		Collision = false;
	}
};

USTRUCT()
struct FForestFoliageMesh
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		class UStaticMesh* Mesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 MaxPerTree;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float ScaleMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float ScaleMax;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MinBounds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxBounds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float ZOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float HeightMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float HeightMax;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxSlope;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool AlignWithSlope;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool Collision;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<UPhysicalMaterial*> PhysicalMaterials;

	FForestFoliageMesh()
	{
		MaxPerTree = 1;
		ScaleMin = 1.0f;
		ScaleMax = 1.0f;
		MinBounds = 100.0f;
		MaxBounds = 300.0f;
		HeightMin = INT32_MIN;
		HeightMax = INT32_MAX;
		MaxSlope = 30.0f;
		AlignWithSlope = true;
		Collision = false;
	}
};

USTRUCT()
struct FTreeInstance
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		int32 MeshIndex;

	UPROPERTY()
		int32 InstanceId;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		float Bounds;

	UPROPERTY()
		float Scale;

	UPROPERTY()
		bool IsFoliage;

	FTreeInstance()
	{
		IsFoliage = false;
	}
};

//USTRUCT()
//struct FTile
//{
//	GENERATED_USTRUCT_BODY()
//
//public:
//
//	UPROPERTY()
//		FVector Location;
//
//	UPROPERTY()
//		TArray<FTreeInstance> Instances;
//
//	UPROPERTY(transient)
//		TArray<UHierarchicalInstancedStaticMeshComponent*> MeshComponents;
//
//	FTile()
//	{
//	}
//};

UCLASS()
class UForestTile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
		TArray<UHierarchicalInstancedStaticMeshComponent*> MeshComponents;

	UPROPERTY()
		TArray<FTreeInstance> Instances;
};

UCLASS()
class LANDSCAPE_1_API AForestTileActor : public ATileActor
{
	GENERATED_BODY()

public:
	AForestTileActor();
	~AForestTileActor();
	virtual void BeginPlay() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FForestMesh> Trees;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FForestFoliageMesh> FoliageAroundTrees;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float SpawnChance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float FoliageSpawnChance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 TreesPerTile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 FoliageStartCullDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 FoliageEndCullDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 InitialSeed;

protected:
	virtual void UpdateTile(int32 x, int32 y, FVector location) override;

private:
	UPROPERTY(transient, duplicatetransient)
		TArray<UForestTile*> ForestTiles;

	uint32 Hash(uint32 a);
	FTransform GetTransform(FVector location, uint32 seed, bool AlignWithSlope, float HeightMin, float HeightMax, float MaxSlope, TArray<UPhysicalMaterial*> PhysicalMaterials);
	void Generate();
	void AddChildren(FTreeInstance parent, int32 step, TArray<FTreeInstance> & instances, uint32 seed);
	bool FindLocation(uint32 seed, FVector parentLocation, float bounds, TArray<FTreeInstance> & instances, FVector & childLocation);
	bool TestLocation(FVector location, float bounds, TArray<FTreeInstance> & instances);
	float GetAngle(FVector start, FHitResult HitData);
	int32 GetIndex(int32 x, int32 y);
};
