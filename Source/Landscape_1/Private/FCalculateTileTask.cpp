#include "Landscape_1.h"
#include "FoliageGroup.h"
#include "SimplexNoise.h"
#include "TileUtils.h"
#include "FCalculateTileTask.h"

void FCalculateTileTask::DoWork()
{
	GridArrayType collisionGrid;
	collisionGrid.AddDefaulted(collisionGridSize * collisionGridSize);
	FTileTaskResult result = FTileTaskResult(TileIndex, true, Location);

	for (int groupIndex = 0; groupIndex < Groups.Num(); groupIndex++)
	{
		FFoliageGroup group = Groups[groupIndex];
		TArray<FSortItem> itemsSorted;

		for (int itemIndex = 0; itemIndex < group.Items.Num(); itemIndex++)
		{
			itemsSorted.Add(FSortItem(itemIndex, group.Items[itemIndex].Width));
		}

		itemsSorted.Sort([](const FSortItem& a, const FSortItem& b) {
			return a.Width > b.Width;
		});

		for (int tmpIndex = 0; tmpIndex < group.Items.Num(); tmpIndex++)
		{
			int itemIndex = itemsSorted[tmpIndex].Index;
			FFoliageGroupItem& item = group.Items[itemIndex];
			int itemRadius = FMath::Max<int>(1, item.Width / 2);
			float itemWorldWidth = TileSize / collisionGridSize * item.Width;
			GridArrayType itemCollisionGrid;
			itemCollisionGrid.AddDefaulted(collisionGridSize * collisionGridSize);

			for (int tileX = 0; tileX < collisionGridSize; tileX++)
			{
				for (int tileY = 0; tileY < collisionGridSize; tileY++)
				{
					Seed = TileUtils::Hash(Seed);

					if (collisionGrid[(tileX * collisionGridSize) + tileY] ||
						tileX < itemRadius ||
						tileY < itemRadius ||
						collisionGridSize - tileX < itemRadius ||
						collisionGridSize - tileY < itemRadius)
						continue;

					auto tileSeed = Seed;
					bool spawn = true;
					float fallOffSpawnChance = 0.0f;
					float extraWidth = 0.0f;
					tileSeed = TileUtils::Hash(tileSeed);
					float r1 = ((double)tileSeed / UINT32_MAX * 2) - 1.0f;
					tileSeed = TileUtils::Hash(tileSeed);
					float r2 = ((double)tileSeed / UINT32_MAX * 2) - 1.0f;

					FVector tileLocation = FVector(
						Location.X + (TileSize / collisionGridSize * tileX) + (itemWorldWidth * r1 * item.OffsetFactor),
						Location.Y + (TileSize / collisionGridSize * tileY) + (itemWorldWidth * r2 * item.OffsetFactor),
						-1000000.0f);

					float scaleNoise = FMath::Abs(USimplexNoise::SimplexNoise2D(tileLocation.X / item.Scale.NoiseSize, tileLocation.Y / item.Scale.NoiseSize));
					float scale = item.Scale.Min + ((item.Scale.Max - item.Scale.Min) * scaleNoise);

					// Parent Noise check
					for (int noiseIndex = 0; noiseIndex < group.Noise.Num(); noiseIndex++)
					{
						auto& noise = group.Noise[noiseIndex];
						float noiseSeed = 100000.0f * ((float)TileUtils::Hash(noise.Seed) / UINT32_MAX);
						float noiseValue = FMath::Abs(USimplexNoise::SimplexNoise2D((tileLocation.X + noiseSeed) / noise.NoiseSize, (tileLocation.Y + noiseSeed) / noise.NoiseSize));

						if (noiseValue < noise.Min || noiseValue > noise.Max)
							spawn = false;

						fallOffSpawnChance = GetFallOffSpawnChance(noise, noiseValue);
						tileSeed = TileUtils::Hash(tileSeed);
						if (fallOffSpawnChance < (float)tileSeed / UINT32_MAX)
						{
							extraWidth = (item.Spacing * 2) * (1.0f - fallOffSpawnChance);
						}
					}

					if (!spawn)
						continue;

					// Noise check
					for (int noiseIndex = 0; noiseIndex < item.Noise.Num(); noiseIndex++)
					{
						auto& noise = item.Noise[noiseIndex];
						float noiseSeed = 100000.0f * ((float)TileUtils::Hash(noise.Seed) / UINT32_MAX);
						float noiseValue = FMath::Abs(USimplexNoise::SimplexNoise2D((tileLocation.X + noiseSeed) / noise.NoiseSize, (tileLocation.Y + noiseSeed) / noise.NoiseSize));

						if (noiseValue < noise.Min || noiseValue > noise.Max)
							spawn = false;

						fallOffSpawnChance = GetFallOffSpawnChance(noise, noiseValue);
						tileSeed = TileUtils::Hash(tileSeed);
						if (fallOffSpawnChance < (float)tileSeed / UINT32_MAX)
						{
							extraWidth = (item.Spacing * 2) * (1.0f - fallOffSpawnChance);
						}
					}

					if (!spawn)
						continue;

					auto spawnSpace = CalculateSpawnSpace(collisionGrid, collisionGridSize, tileX, tileY, item.Width / 2 + item.Spacing);

					if (spawnSpace.IsSpace)
						spawnSpace = CalculateSpawnSpace(itemCollisionGrid, collisionGridSize, tileX, tileY, item.Width / 2 + item.Spacing);

					if (!spawnSpace.IsSpace)
					{
						tileY += FMath::Max<int>(0, (item.Width / 2 + item.Spacing) - spawnSpace.FailedDistanceY);
						continue;
					}

					tileSeed = TileUtils::Hash(tileSeed);
					if (extraWidth > 0 && (fallOffSpawnChance + 0.5f) / 1.5f < (float)tileSeed / UINT32_MAX)
					{
						Spawn(itemCollisionGrid, collisionGridSize, tileX, tileY, extraWidth);
						continue;
					}

					Spawn(itemCollisionGrid, collisionGridSize, tileX, tileY, item.Width);

					// Spawn chance check
					tileSeed = TileUtils::Hash(tileSeed);
					if (item.SpawnChance < (float)tileSeed / UINT32_MAX)
						continue;

					Spawn(collisionGrid, collisionGridSize, tileX, tileY, item.Width);
					result.Items.Add(FTileTaskResultItem(tileLocation, groupIndex, itemIndex, scale, tileSeed));
				}
			}

			itemCollisionGrid.Empty();
		}
	}

	collisionGrid.Empty();
	TaskResultDelegate.ExecuteIfBound(result);
}

float FCalculateTileTask::GetFallOffSpawnChance(FFoliageGroupSpawnNoise& noise, float noiseValue)
{
	if (noise.FallOff == 0.0f)
		return 1.0f;

	float minFallOff = 1.0f / noise.FallOff * (noiseValue - noise.Min);
	float maxFallOff = 1.0f / noise.FallOff * (noise.Max - noiseValue);

	if (noise.Min > 0.0f && (noise.Max == 1.0f || minFallOff < maxFallOff))
		return minFallOff;

	if (noise.Max < 1.0f && (noise.Min == 0.0f || minFallOff > maxFallOff))
		return maxFallOff;

	return 1.0f;
}

FItemSpawnSpace FCalculateTileTask::CalculateSpawnSpace(GridArrayType& collisionGrid, int size, int x, int y, int spacing) {
	for (int tmpX = 0; tmpX <= spacing; tmpX++)
	{
		for (int tmpY = 0; tmpY <= spacing; tmpY++)
		{
			int gridPlusX = x + tmpX;
			int gridPlusY = y + tmpY;
			int gridMinusX = x - tmpX;
			int gridMinusY = y - tmpY;

			if ((gridMinusX >= 0 && gridMinusY >= 0 && collisionGrid[(gridMinusX * size) + gridMinusY]) ||
				(gridPlusX < size && gridMinusY >= 0 && collisionGrid[(gridPlusX * size) + gridMinusY]) ||
				(gridPlusX < size && gridPlusY < size && collisionGrid[(gridPlusX * size) + gridPlusY]) ||
				(gridMinusX >= 0 && gridPlusY < size && collisionGrid[(gridMinusX * size) + gridPlusY]))
				return FItemSpawnSpace(false, tmpY);
		}
	}

	return FItemSpawnSpace(true, 0);
}

void FCalculateTileTask::Spawn(GridArrayType& collisionGrid, int size, int x, int y, int width) {
	if (width == 1)
	{
		collisionGrid[(x * size) + y] = true;
		return;
	}

	int radius = width / 2;
	int startX = FMath::Max<int>(0, x - radius);
	int startY = FMath::Max<int>(0, y - radius);
	int endX = FMath::Min<int>(size, x + radius);
	int endY = FMath::Min<int>(size, y + radius);

	for (int tmpX = startX; tmpX < endX; tmpX++)
	{
		for (int tmpY = startY; tmpY < endY; tmpY++)
		{
			collisionGrid[(tmpX * size) + tmpY] = true;
		}
	}
}