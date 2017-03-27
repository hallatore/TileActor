// Fill out your copyright notice in the Description page of Project Settings.

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

UCLASS()
class LANDSCAPE_1_API ATileActor : public AActor
{
	GENERATED_BODY()

public:
	ATileActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float Radius;

protected:
	UPROPERTY()
		int32 Size;

	int32 GetIndex(int32 x, int32 y);
	virtual void UpdateTile(int32 x, int32 y, FVector location);
	virtual void PostUpdateTiles();

private:
	UPROPERTY()
		int32 CurrentTileX;

	UPROPERTY()
		int32 CurrentTileY;

	UPROPERTY()
		FVector CurrentTileLocation;

	UPROPERTY()
		FVector CurrentCameraLocation;

	UPROPERTY(transient, duplicatetransient)
		TArray<FTile> Tiles;

	UPROPERTY()
		int32 CurrentUpdateIndex;

	FVector GetTileLocation(int32 x, int32 y);
	int32 ConvertTileIndex(int32 index);
};
