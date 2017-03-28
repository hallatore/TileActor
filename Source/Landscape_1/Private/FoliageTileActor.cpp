#include "Landscape_1.h"
#include "FoliageTileActor.h"
#include "SimplexNoise.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Kismet/KismetSystemLibrary.h"

AFoliageTileActor::AFoliageTileActor()
{
	DistanceBetween = 200.0f;
	OffsetFactor = 0.5f;
	Scale = FFoliageTileNoise();
	Scale.Min = 1.0f;
	MinCullDistance = 0.0f;
	SpawnChance = 1.0f;
	AlignWithSlope = true;
	Collision = false;
	AffectDistanceFieldLighting = false;
	CastShadow = 0;
}

void AFoliageTileActor::Load()
{
	Super::Load();
	uint32 seed = Hash(Seed);

	if (Mesh == NULL)
		return;

	FoliageTiles.AddUninitialized(Size * Size);

	int32 componentSize = 2;
	float FoliageTilesize = Radius * 2 / Size;
	float endCullDistance = Radius - (FoliageTilesize * 1.1f);
	float minCullDistance = MinCullDistance > 0 ? MinCullDistance : endCullDistance / 2;
	USceneComponent* rootComponent = GetRootComponent();

	for (int32 y = 0; y < FoliageTiles.Num(); y++)
	{
		seed = Hash(seed);
		UFoliageTile* tile = NewObject<UFoliageTile>(this, NAME_None);
		tile->MeshComponents.AddUninitialized(componentSize);
		FoliageTiles[y] = tile;

		for (int32 componentIndex = 0; componentIndex < componentSize; componentIndex++)
		{
			UHierarchicalInstancedStaticMeshComponent* component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			component->StaticMesh = Mesh;
			component->bSelectable = false;
			component->bHasPerInstanceHitProxies = true;
			component->InstancingRandomSeed = seed % INT32_MAX;
			component->bAffectDistanceFieldLighting = AffectDistanceFieldLighting;
			component->CastShadow = CastShadow;

			float cullDistance = (endCullDistance / componentSize * (componentIndex + 1)) * FMath::Max(0.0f, (1.0f - (1.0f / endCullDistance * minCullDistance)));
			component->InstanceEndCullDistance = endCullDistance - cullDistance;
			component->InstanceStartCullDistance = endCullDistance - cullDistance - (endCullDistance / componentSize * (componentIndex + 1));

			if (Collision == false)
				component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			else
				component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			component->AttachTo(rootComponent);
			component->RegisterComponent();
			tile->MeshComponents[componentIndex] = component;
		}	
	}

	IsLoaded = true;
}

void AFoliageTileActor::Unload() {
	Super::Unload();

	for (int32 i = 0; i < FoliageTiles.Num(); i++)
	{
		for (int32 y = 0; y < FoliageTiles[i]->MeshComponents.Num(); y++)
		{
			FoliageTiles[i]->MeshComponents[y]->DetachFromParent();
			FoliageTiles[i]->MeshComponents[y]->UnregisterComponent();
		}

		FoliageTiles[i]->MeshComponents.Empty();
	}

	FoliageTiles.Empty();
}

void AFoliageTileActor::UpdateTile(int32 x, int32 y, FVector location) {
	if (Mesh == NULL || DistanceBetween <= 0.0)
		return;

	UFoliageTile* tile = FoliageTiles[GetIndex(x, y)];
	uint32 seed = Hash(Seed + (x * Size) + y);

	for (int32 componentIndex = 0; componentIndex < tile->MeshComponents.Num(); componentIndex++)
	{
		tile->MeshComponents[componentIndex]->ClearInstances();
	}

	float tileSize = Radius * 2 / Size;
	int32 arraySize = (int32)(tileSize / DistanceBetween);
	float split = tileSize / arraySize;

	for (int32 tileX = 0; tileX < arraySize; tileX++)
	{
		for (int32 tileY = 0; tileY < arraySize; tileY++)
		{
			int32 compIndex = FMath::RandRange(0, tile->MeshComponents.Num() - 1);
			seed = Hash(seed);

			if (SpawnChance > (double)seed / UINT32_MAX) {
				float r1 = (double)Hash(seed) / UINT32_MAX;
				float r2 = (double)Hash(seed) / UINT32_MAX;
				bool spawn = true;
				FVector instanceLocation = FVector(location.X + split * tileX + (split * r1 * OffsetFactor), location.Y + split * tileY + (split * r2 * OffsetFactor), -1000000.0f);

				for (int32 spawnNoiseIndex = 0; spawnNoiseIndex < SpawnNoise.Num(); spawnNoiseIndex++)
				{
					float noiseSeed = 100000.0f * ((float)Hash(SpawnNoise[spawnNoiseIndex].Seed) / UINT32_MAX);
					float noise = (USimplexNoise::SimplexNoise2D((instanceLocation.X + noiseSeed) / SpawnNoise[spawnNoiseIndex].NoiseSize, (instanceLocation.Y + noiseSeed) / SpawnNoise[spawnNoiseIndex].NoiseSize) + 1) / 2.0f;

					if (noise < SpawnNoise[spawnNoiseIndex].Min || noise > SpawnNoise[spawnNoiseIndex].Max)
					{
						spawn = false;
						break;
					}
				}

				if (!spawn)
					continue;

				float noise = (USimplexNoise::SimplexNoise2D(instanceLocation.X / Scale.NoiseSize, instanceLocation.Y / Scale.NoiseSize) + 1) / 2.0f;
				float scale = Scale.Min + ((Scale.Max - Scale.Min) * noise);
				FTransform transform = GetTransform(instanceLocation, Hash(seed));
				transform.SetScale3D(FVector(scale, scale, scale));
				tile->MeshComponents[compIndex]->AddInstance(transform);
			}
		}
	}
}

FTransform AFoliageTileActor::GetTransform(FVector location, uint32 seed) {
	const FVector Start = FVector(location.X, location.Y, 100000.0f);
	const FVector End = FVector(location.X, location.Y, -100000.0f);
	float r = (double)seed / UINT32_MAX;

	FTransform result = FTransform();
	FQuat rotator = FQuat(FRotator(0.0f, 360.0f * r, 0.0f));
	result.SetTranslation(FVector(-1.0f, -1.0f, -100000.0f));
	result.SetRotation(rotator);

	FHitResult HitData(ForceInit);
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(NULL);

	TArray<TEnumAsByte<enum EObjectTypeQuery> > Objects;
	Objects.Add(EObjectTypeQuery::ObjectTypeQuery1);

	// Todo: Find a way to trace only landscapes for better performance
	if (UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), Start, End, Objects, false, ActorsToIgnore, EDrawDebugTrace::None, HitData, true))
	{
		if (HitData.GetActor() && HitData.Actor->IsA(ALandscape::StaticClass()))
		{
			result.SetTranslation(FVector(location.X, location.Y, HitData.ImpactPoint.Z));

			if (AlignWithSlope == true)
			{
				FRotator rotation = FRotationMatrix::MakeFromZ(HitData.Normal).Rotator();
				result.SetRotation(FQuat(rotation) * rotator);
			}
		}
	}

	return result;
}

uint32 AFoliageTileActor::Hash(uint32 a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}