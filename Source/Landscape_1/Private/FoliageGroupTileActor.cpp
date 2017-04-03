#include "Landscape_1.h"
#include "FoliageGroupTileActor.h"
#include "SimplexNoise.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Kismet/KismetSystemLibrary.h"

AFoliageGroupTileActor::AFoliageGroupTileActor()
{
}

void AFoliageGroupTileActor::Load()
{	
	uint32 seed = Hash(Seed);
	float FoliageTilesize = Radius * 2 / Size;
	float endCullDistance = Radius - (FoliageTilesize * 1.1f);
	USceneComponent* rootComponent = GetRootComponent();
	FoliageGroupTiles.Reserve(Size * Size);

	for (int groupTileIndex = 0; groupTileIndex < Size * Size; groupTileIndex++)
	{
		UFoliageGroupTile* tile = NewObject<UFoliageGroupTile>(this, NAME_None);
		FoliageGroupTiles.Add(tile);
		tile->Groups.Reserve(Groups.Num());

		for (int groupIndex = 0; groupIndex < Groups.Num(); groupIndex++)
		{
			FFoliageGroup group = Groups[groupIndex];
			UFoliageGroupTileGroup* tileGroup = NewObject<UFoliageGroupTileGroup>(this, NAME_None);
			tile->Groups.Add(tileGroup);
			tileGroup->Items.Reserve(group.Items.Num());

			for (int itemIndex = 0; itemIndex < group.Items.Num(); itemIndex++)
			{
				FFoliageGroupItem item = group.Items[itemIndex];
				UFoliageGroupTileItem* tileItem = NewObject<UFoliageGroupTileItem>(this, NAME_None);
				tileGroup->Items.Add(tileItem);
				tileItem->MeshComponents.Reserve(item.Meshes.Num());

				for (int meshIndex = 0; meshIndex < item.Meshes.Num(); meshIndex++)
				{
					seed = Hash(seed);
					UHierarchicalInstancedStaticMeshComponent* component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
					component->SetStaticMesh(item.Meshes[meshIndex]);
					component->bSelectable = false;
					component->bHasPerInstanceHitProxies = true;
					component->InstancingRandomSeed = seed % UINT32_MAX;
					component->bAffectDistanceFieldLighting = item.AffectDistanceFieldLighting;
					component->CastShadow = item.CastShadow;
					component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					component->InstanceEndCullDistance = endCullDistance;
					component->AttachToComponent(rootComponent, FAttachmentTransformRules::KeepRelativeTransform);
					component->RegisterComponent();
					tileItem->MeshComponents.Add(component);
				}
			}
		}
	}

	Super::Load();
}

void AFoliageGroupTileActor::Unload()
{
	TArray<UActorComponent*> components = GetComponentsByClass(UHierarchicalInstancedStaticMeshComponent::StaticClass());

	for (int32 i = 0; i < components.Num(); i++)
	{
		components[i]->DestroyComponent();
	}

	FoliageGroupTiles.Empty();
	Super::Unload();
}

void AFoliageGroupTileActor::UpdateTile(int32 x, int32 y, FVector location)
{
	UFoliageGroupTile* tile = FoliageGroupTiles[GetIndex(x, y)];
	ClearInstances(tile);

	uint32 seed = Hash(Seed + (x * Size) + y);
	float tileSize = Radius * 2 / Size;
	GridArrayType collisionGrid;
	collisionGrid.AddDefaulted(collisionGridSize * collisionGridSize);

	for (int groupIndex = 0; groupIndex < Groups.Num(); groupIndex++)
	{
		FFoliageGroup& group = Groups[groupIndex];
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
			float itemWorldWidth = tileSize / collisionGridSize * item.Width;
			GridArrayType itemCollisionGrid;
			itemCollisionGrid.AddDefaulted(collisionGridSize * collisionGridSize);

			for (int tileX = 0; tileX < collisionGridSize; tileX++)
			{
				for (int tileY = 0; tileY < collisionGridSize; tileY++)
				{
					seed = Hash(seed);

					if (collisionGrid[(tileX * collisionGridSize) + tileY] || 
						tileX < itemRadius || 
						tileY < itemRadius || 
						collisionGridSize - tileX < itemRadius || 
						collisionGridSize - tileY < itemRadius)
						continue;

					auto tileSeed = seed;
					bool spawn = true;
					float fallOffSpawnChance = 0.0f;
					float extraWidth = 0.0f;
					tileSeed = Hash(tileSeed);
					float r1 = ((double)tileSeed / UINT32_MAX * 2) - 1.0f;
					tileSeed = Hash(tileSeed);
					float r2 = ((double)tileSeed / UINT32_MAX * 2) - 1.0f;
					FVector tileLocation = FVector(
						location.X + (tileSize / collisionGridSize * tileX) + (itemWorldWidth * r1 * item.OffsetFactor),
						location.Y + (tileSize / collisionGridSize * tileY) + (itemWorldWidth * r2 * item.OffsetFactor),
						-1000000.0f);

					// Parent Noise check
					for (int noiseIndex = 0; noiseIndex < group.Noise.Num(); noiseIndex++)
					{
						auto& noise = group.Noise[noiseIndex];
						float noiseSeed = 100000.0f * ((float)Hash(noise.Seed) / UINT32_MAX);
						float noiseValue = FMath::Abs(USimplexNoise::SimplexNoise2D((tileLocation.X + noiseSeed) / noise.NoiseSize, (tileLocation.Y + noiseSeed) / noise.NoiseSize));

						if (noiseValue < noise.Min || noiseValue > noise.Max)
							spawn = false;

						fallOffSpawnChance = GetFallOffSpawnChance(noise, noiseValue);
						tileSeed = Hash(tileSeed);
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
						float noiseSeed = 100000.0f * ((float)Hash(noise.Seed) / UINT32_MAX);
						float noiseValue = FMath::Abs(USimplexNoise::SimplexNoise2D((tileLocation.X + noiseSeed) / noise.NoiseSize, (tileLocation.Y + noiseSeed) / noise.NoiseSize));

						if (noiseValue < noise.Min || noiseValue > noise.Max)
							spawn = false;

						fallOffSpawnChance = GetFallOffSpawnChance(noise, noiseValue);
						tileSeed = Hash(tileSeed);
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

					tileSeed = Hash(tileSeed);
					if (extraWidth > 0 && (fallOffSpawnChance + 0.5f) / 1.5f < (float)tileSeed / UINT32_MAX)
					{
						Spawn(itemCollisionGrid, collisionGridSize, tileX, tileY, extraWidth);
						continue;
					}

					Spawn(itemCollisionGrid, collisionGridSize, tileX, tileY, item.Width);

					// Spawn chance check
					tileSeed = Hash(tileSeed);
					if (item.SpawnChance < (float)tileSeed / UINT32_MAX)
						continue;

					Spawn(collisionGrid, collisionGridSize, tileX, tileY, item.Width);

					tileSeed = Hash(tileSeed);
					TArray<AActor*> BlockingVolumes;
					FTransform transform = GetTransform(tileLocation, tileSeed, BlockingVolumes);
					float noise = FMath::Abs(USimplexNoise::SimplexNoise2D(tileLocation.X / item.Scale.NoiseSize, tileLocation.Y / item.Scale.NoiseSize));
					float scale = item.Scale.Min + ((item.Scale.Max - item.Scale.Min) * noise);
					transform.SetScale3D(FVector(scale, scale, scale));

					if (transform.GetLocation().Z == -100000.0f)
						continue;

					//for (int32 blockingVolumeIndex = 0; blockingVolumeIndex < BlockingVolumes.Num(); blockingVolumeIndex++)
					//{
					//	auto volume = (AFoliageTileBlockingVolume*)BlockingVolumes[blockingVolumeIndex];
					//	if (volume->GetBrushComponent()->OverlapComponent(transform.GetLocation(), transform.GetRotation(), FCollisionShape()))
					//	{
					//		if ((Layer == NAME_None && volume->FoliageLayers.Num() == 0) ||
					//			(Layer != NAME_None && volume->FoliageLayers.Contains(Layer)))
					//		{
					//			spawn = false;
					//			break;
					//		}
					//	}
					//}

					if (!spawn)
						continue;

					auto meshComponents = tile->Groups[groupIndex]->Items[itemIndex]->MeshComponents;
					tileSeed = Hash(tileSeed);
					int meshIndex = tileSeed % meshComponents.Num();
					meshComponents[meshIndex]->AddInstance(transform);
				}
			}
		}
	}

	collisionGrid.Empty();
}

void AFoliageGroupTileActor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	float tileSize = Radius * 2 / Size;

	for (int groupIndex = 0; groupIndex < Groups.Num(); groupIndex++)
	{
		FFoliageGroup& group = Groups[groupIndex];
		for (int itemIndex = 0; itemIndex < group.Items.Num(); itemIndex++)
		{
			FFoliageGroupItem& item = group.Items[itemIndex];
			item.CalculatedWidth = tileSize / collisionGridSize * item.Width;
		}
	}
}

float AFoliageGroupTileActor::GetFallOffSpawnChance(FFoliageGroupSpawnNoise& noise, float noiseValue) 
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

FTransform AFoliageGroupTileActor::GetTransform(FVector location, uint32 seed, TArray<AActor*> actorsToIgnore) 
{
	const FVector Start = FVector(location.X, location.Y, 100000.0f);
	const FVector End = FVector(location.X, location.Y, -100000.0f);
	float r = (double)seed / UINT32_MAX;

	FTransform result = FTransform();
	FQuat rotator = FQuat(FRotator(0.0f, 360.0f * r, 0.0f));
	result.SetTranslation(FVector(-1.0f, -1.0f, -100000.0f));
	result.SetRotation(rotator);
	FHitResult HitData(ForceInit);
	TArray<TEnumAsByte<enum EObjectTypeQuery> > Objects;
	Objects.Add(EObjectTypeQuery::ObjectTypeQuery1);

	// Todo: Find a way to trace only landscapes for better performance
	if (UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), Start, End, Objects, false, actorsToIgnore, EDrawDebugTrace::None, HitData, true) &&
		HitData.GetActor() && 
		HitData.Actor->IsA(ALandscape::StaticClass()))
	{
		result.SetTranslation(FVector(location.X, location.Y, HitData.ImpactPoint.Z));
	}

	return result;
}

FItemSpawnSpace AFoliageGroupTileActor::CalculateSpawnSpace(GridArrayType& collisionGrid, int size, int x, int y, int spacing) {
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

void AFoliageGroupTileActor::Spawn(GridArrayType& collisionGrid, int size, int x, int y, int width) {
	//UE_LOG(LogStaticMesh, Display, TEXT("AFoliageGroupTileActor::Spawn: x: %d, y: %d, width: %d"), x, y, width);

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

float AFoliageGroupTileActor::GetItemMinWidth() {
	float width = MAX_flt;

	for (int groupIndex = 0; groupIndex < Groups.Num(); groupIndex++)
	{
		auto group = Groups[groupIndex];

		for (int itemIndex = 0; itemIndex < group.Items.Num(); itemIndex++)
		{
			auto item = group.Items[itemIndex];
			
			if (item.Width < width)
				width = item.Width;
		}
	}

	return width;
}

void AFoliageGroupTileActor::ClearInstances(UFoliageGroupTile* tile)
{
	for (int groupIndex = 0; groupIndex < tile->Groups.Num(); groupIndex++)
	{
		auto group = tile->Groups[groupIndex];

		for (int itemIndex = 0; itemIndex < group->Items.Num(); itemIndex++)
		{
			auto item = group->Items[itemIndex];

			for (int meshIndex = 0; meshIndex < item->MeshComponents.Num(); meshIndex++)
			{
				item->MeshComponents[meshIndex]->ClearInstances();
			}
		}
	}
}

uint32 AFoliageGroupTileActor::Hash(uint32 a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

