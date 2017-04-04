#include "Landscape_1.h"
#include "TileUtils.h"
#include "TileActor.h"

ATileActor::ATileActor()
{
	RenderInEditor = false;
	Size = 11;
	PrimaryActorTick.bCanEverTick = true;
	Radius = 10000.0f;

	CurrentTileX = 1;
	CurrentTileY = 1;
	CurrentTileLocation = FVector(-1000000.0f, -1000000.0f, -1000000.0f);
	IsLoaded = false;
}

void ATileActor::BeginPlay()
{
	Super::BeginPlay();
	IsLoadedFromBeginPlay = true;

	if (!IsLoaded)
		Reload();
}

void ATileActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	IsLoadedFromBeginPlay = false;

	if (!RenderInEditor)
		Unload();
}

void ATileActor::Load()
{
	Tiles.AddUninitialized(Size * Size);

	for (int32 i = 0; i < Size * Size; i++)
	{
		Tiles[i] = FTile();
		Tiles[i].ShouldUpdate = true;
	}

	IsLoaded = true;
}

void ATileActor::Reload()
{
	Unload();
	Load();
}

void ATileActor::Unload()
{
	IsLoaded = false;
	Tiles.Empty();
	CurrentTileX = 1;
	CurrentTileY = 1;
	CurrentTileLocation = FVector(-1000000.0f, -1000000.0f, -1000000.0f);
}

void ATileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!RenderInEditor && !IsLoadedFromBeginPlay)
		return;

	if (IsLoaded == false)
	{
		this->Load();
		return;
	}

	auto world = GetWorld();
	auto currentCameraLocation = TileUtils::GetCurrentCameraLocation(world);

	if (TileUtils::IsEmptyFVector(currentCameraLocation))
		return;

	if (Tiles.Num() == Size * Size) {
		FTileInfo tileInfo = GetClosestTileToUpdate(currentCameraLocation);

		if (tileInfo.X >= 0)
		{
			FTile & tile = Tiles[GetIndex(tileInfo.X, tileInfo.Y)];
			tile.ShouldUpdate = false;
			UpdateTile(tileInfo.X, tileInfo.Y, tile.Location);
		}
	}

	float tileSize = Radius * 2 / Size;
	float disX = currentCameraLocation.X - CurrentTileLocation.X;
	float disY = currentCameraLocation.Y - CurrentTileLocation.Y;
	bool updated = false;

	if (disX > tileSize || disX * -1 > tileSize / 2) {
		int32 currentX = (int32)floor(currentCameraLocation.X / tileSize);
		CurrentTileLocation.X = currentX * (int32)tileSize;
		CurrentTileX = currentX % Size;
		updated = true;

		if (CurrentTileX < 0)
			CurrentTileX += Size;
	}

	if (disY > tileSize || disY * -1 > tileSize / 2) {
		int32 currentY = (int32)floor(currentCameraLocation.Y / tileSize);
		CurrentTileLocation.Y = currentY * (int32)tileSize;
		CurrentTileY = currentY % Size;
		updated = true;

		if (CurrentTileY < 0)
			CurrentTileY += Size;
	}

	if (updated == false || Tiles.Num() != Size * Size)
		return;

	for (int32 x = 0; x < Size; x++)
	{
		for (int32 y = 0; y < Size; y++)
		{
			FTile & tile = Tiles[GetIndex(x, y)];
			tile.NewLocation = GetTileLocation(x, y);

			if (FVector::Dist(tile.Location, tile.NewLocation) > tileSize / 3) {
				tile.Location = tile.NewLocation;
				tile.ShouldUpdate = true;
			}
		}
	}
}

FTileInfo ATileActor::GetClosestTileToUpdate(FVector location)
{
	int32 closestX = -1;
	int32 closestY = -1;
	float distance = MAX_flt;

	for (int32 x = 0; x < Size; x++)
	{
		for (int32 y = 0; y < Size; y++)
		{
			FTile & tile = Tiles[GetIndex(x, y)];
			if (tile.ShouldUpdate) {
				float tmpDistance = FVector::DistSquaredXY(location, tile.Location);

				if (tmpDistance > distance)
					continue;

				distance = tmpDistance;
				closestX = x;
				closestY = y;
			}
		}
	}

	return FTileInfo(closestX, closestY);
}

int32 ATileActor::ConvertTileIndex(int32 index)
{
	int32 tmpIndex = index;
	if (index < 0)
		tmpIndex *= -1;

	if (index < 0 && tmpIndex > (float) Size / 2)
		return (Size * -1 + tmpIndex) * -1;
	else if (index > 0 && tmpIndex > (float)Size / 2)
		return (Size * -1 + tmpIndex);

	return index;
}

FVector ATileActor::GetTileLocation(int32 x, int32 y) {
	int32 tileSize = Radius * 2 / Size;
	int32 tmpX = ConvertTileIndex(x - CurrentTileX);
	int32 tmpY = ConvertTileIndex(y - CurrentTileY);
	float xOffset = tmpX * tileSize;
	float yOffset = tmpY * tileSize;
	FVector offset = FVector(xOffset, yOffset, 0.0f);
	return CurrentTileLocation + offset;
}

int32 ATileActor::GetIndex(int32 x, int32 y) {
	int32 result = 0;

	for (int32 x2 = 0; x2 < Size; x2++)
	{
		for (int32 y2 = 0; y2 < Size; y2++)
		{
			if (x2 == x && y2 == y)
				return result;

			result++;

			if (x2 == x && y2 == y)
				return result;
		}
	}

	return result;
}

void ATileActor::UpdateTile(int32 x, int32 y, FVector location) {}
void ATileActor::PostUpdateTiles() {}
bool ATileActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ATileActor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);

	if (RenderInEditor)
		Reload();
	else
		Unload();
}