// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TileActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FoliageTileActor.generated.h"

USTRUCT()
struct FFoliageInstance
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		int32 InstanceId;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		float Scale;

	UPROPERTY()
		int32 ComponentIndex;

	FFoliageInstance()
	{
	}
};

UCLASS()
class UFoliageTile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
		TArray<UHierarchicalInstancedStaticMeshComponent*> MeshComponents;

	UPROPERTY()
		TArray<FFoliageInstance> Instances;
};

UCLASS()
class LANDSCAPE_1_API AFoliageTileActor : public ATileActor
{
	GENERATED_BODY()

public:
	AFoliageTileActor();
	virtual void BeginPlay() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UStaticMesh* Mesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 MeshesPrTile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float SpawnChance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
		int32 SpawnNoiseSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float SpawnNoiseMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float SpawnNoiseMax;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float OffsetFactor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float ScaleMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float ScaleMax;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "1", UIMin = "1"))
		int32 ScaleNoiseSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
		float MinCullDistance;

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
		bool AffectDistanceFieldLighting;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
		uint32 CastShadow : 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<UPhysicalMaterial*> PhysicalMaterials;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 InitialSeed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		int32 TotalMeshes;

protected:
	virtual void UpdateTile(int32 x, int32 y, FVector location) override;

private:
	UPROPERTY(transient, duplicatetransient)
		TArray<UFoliageTile*> FoliageTiles;

	uint32 Hash(uint32 a);
	FTransform GetTransform(FVector location, uint32 seed);
};
