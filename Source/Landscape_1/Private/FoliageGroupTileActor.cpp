#include "Landscape_1.h"
#include "FoliageGroupTileActor.h"
#include "FCalculateTileTask.h"
#include "FoliageGroup.h"
#include "SimplexNoise.h"
#include "TileUtils.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Kismet/KismetSystemLibrary.h"

AFoliageGroupTileActor::AFoliageGroupTileActor()
{
}

void AFoliageGroupTileActor::Load()
{	
	uint32 seed = TileUtils::Hash(Seed);
	float FoliageTilesize = Radius * 2 / Size;
	float endCullDistance = Radius - (FoliageTilesize * 1.1f);
	USceneComponent* rootComponent = GetRootComponent();
	FoliageGroupTiles.Reserve(Size * Size);
	TileTaskResults.AddDefaulted(Size * Size);

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
					seed = TileUtils::Hash(seed);
					UHierarchicalInstancedStaticMeshComponent* component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
					component->SetStaticMesh(item.Meshes[meshIndex]);
					component->bSelectable = false;
					component->bHasPerInstanceHitProxies = true;
					component->InstancingRandomSeed = seed % UINT32_MAX;
					component->bAffectDistanceFieldLighting = item.AffectDistanceFieldLighting;
					component->CastShadow = item.CastShadow;
					component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					component->InstanceEndCullDistance = item.CullDistance > 0.0f ? item.CullDistance : endCullDistance;
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
	Super::Unload();
	FoliageGroupTiles.Empty();
	TileTaskResults.Empty();
	TArray<UActorComponent*> components = GetComponentsByClass(UHierarchicalInstancedStaticMeshComponent::StaticClass());

	for (int32 i = 0; i < components.Num(); i++)
	{
		components[i]->DestroyComponent();
	}
}

void AFoliageGroupTileActor::TaskResultCompleted(FTileTaskResult tileTaskResult)
{
	if (TileTaskResults.Num() > tileTaskResult.TileIndex)
		TileTaskResults[tileTaskResult.TileIndex] = tileTaskResult;
}

int AFoliageGroupTileActor::GetClosestTileToRender(FVector currentLocation)
{
	int32 index = -1;
	float distance = MAX_flt;

	for (int tileIndex = 0; tileIndex < TileTaskResults.Num(); tileIndex++)
	{
		if (TileTaskResults[tileIndex].ShouldRender)
		{
			float tmpDistance = FVector::DistSquaredXY(currentLocation, TileTaskResults[tileIndex].Location);

			if (tmpDistance > distance)
				continue;

			distance = tmpDistance;
			index = tileIndex;
		}
	}

	return index;
}

void AFoliageGroupTileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsLoaded)
		return;

	auto world = GetWorld();
	FVector currentCameraLocation = TileUtils::GetCurrentCameraLocation(world);

	if (TileUtils::IsEmptyFVector(currentCameraLocation))
		return;

	int tileIndex = GetClosestTileToRender(currentCameraLocation);

	if (tileIndex == -1)
		return;

	auto& result = TileTaskResults[tileIndex];

	if (result.ShouldRender && FoliageGroupTiles.Num() > tileIndex)
	{
		result.ShouldRender = false;
		UFoliageGroupTile* tile = FoliageGroupTiles[tileIndex];
		float tileSize = Radius * 2 / Size;
		ClearInstances(tile);

		for (int i = 0; i < result.Items.Num(); i++)
		{
			auto& item = result.Items[i];
			auto meshComponents = tile->Groups[item.GroupIndex]->Items[item.ItemIndex]->MeshComponents;
			auto tileSeed = TileUtils::Hash(item.Seed);
			int meshIndex = tileSeed % meshComponents.Num();

			TArray<AActor*> BlockingVolumes;
			tileSeed = TileUtils::Hash(tileSeed);
			FTransform transform = GetTransform(item.Location, tileSeed, BlockingVolumes);
			transform.SetScale3D(FVector(item.Scale, item.Scale, item.Scale));

			if (transform.GetLocation().Z == -100000.0f)
				continue;

			meshComponents[meshIndex]->AddInstance(transform);
		}

		result.Items.Empty();
	}
}

void AFoliageGroupTileActor::UpdateTile(int32 x, int32 y, FVector location)
{
	int tileIndex = GetIndex(x, y);
	uint32 seed = TileUtils::Hash(Seed + (x * Size) + y);
	UFoliageGroupTile* tile = FoliageGroupTiles[tileIndex];
	float tileSize = Radius * 2 / Size;
	FTileTaskResultDelegate taskResultDelegate;
	taskResultDelegate.BindUObject(this, &AFoliageGroupTileActor::TaskResultCompleted);
	(new FAutoDeleteAsyncTask<FCalculateTileTask>(location, tileIndex, Groups, tileSize, seed, taskResultDelegate))->StartBackgroundTask();
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

