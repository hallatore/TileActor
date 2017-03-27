// Fill out your copyright notice in the Description page of Project Settings.

#include "Landscape_1.h"
#include "FoliageTileActor.h"
#include "SimplexNoise.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
AFoliageTileActor::AFoliageTileActor()
{
	SpawnNoiseSize = 1000;
	SpawnNoiseMin = 0.0f;
	SpawnNoiseMax = 1.0f;
	OffsetFactor = 0.5f;
	ScaleMin = 1.0f;
	ScaleMax = 1.0f;
	ScaleNoiseSize = 1000;
	MinCullDistance = 100.0f;
	HeightMin = INT32_MIN;
	HeightMax = INT32_MAX;
	MeshesPrTile = 1;
	SpawnChance = 1.0f;
	MaxSlope = 30.0f;
	AlignWithSlope = true;
	Collision = false;
	AffectDistanceFieldLighting = false;
	CastShadow = 0;

	int32 arraySize = (int32)sqrt((double)MeshesPrTile);
	TotalMeshes = Super::Size * Super::Size * arraySize * arraySize * SpawnChance;
}

// Called when the game starts or when spawned
void AFoliageTileActor::BeginPlay()
{
	Super::BeginPlay();
	uint32 seed = Hash(InitialSeed);

	if (Mesh == NULL)
		return;

	FoliageTiles.AddUninitialized(Super::Size * Super::Size);

	int32 componentSize = 3;
	float FoliageTilesize = Super::Radius * 2 / Super::Size;
	float endCullDistance = Super::Radius - (FoliageTilesize * 1.1f);
	int32 arraySize = (int32)sqrt((double)MeshesPrTile);
	int32 tileX = 0;
	int32 tileY = 0;

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
			float cullDistance = (endCullDistance / componentSize * (componentIndex + 1)) * FMath::Max(0.0f, (1.0f - (1.0f / endCullDistance * MinCullDistance)));
			component->InstanceEndCullDistance = endCullDistance - cullDistance;
			component->InstanceStartCullDistance = endCullDistance - cullDistance - (endCullDistance / componentSize * (componentIndex + 1));
			component->CastShadow = CastShadow;

			if (Collision == false)
				component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			else
				component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			component->AttachTo(GetRootComponent());
			component->RegisterComponent();
			tile->MeshComponents[componentIndex] = component;
		}		

		int32 tmpX = 0;
		int32 tmpY = 0;
		int32 max = sqrt((double)MeshesPrTile);
		float split = FoliageTilesize / max;
		int32 compIndex = FMath::RandRange(0, componentSize - 1);

		for (int32 i = 0; i < (arraySize * arraySize); i++)
		{
			if (SpawnChance > (double)seed / UINT32_MAX) {
				seed = Hash(seed);
				float r1 = (double)seed / UINT32_MAX;
				seed = Hash(seed);
				float r2 = (double)seed / UINT32_MAX;

				FFoliageInstance foliageInstance = FFoliageInstance();
				foliageInstance.Location = FVector(split * tmpX + (split * r1 * OffsetFactor), split * tmpY + (split * r2 * OffsetFactor), -1000000.0f);;
				foliageInstance.InstanceId = tile->MeshComponents[compIndex]->AddInstance(FTransform(foliageInstance.Location));
				foliageInstance.ComponentIndex = compIndex;
				tile->Instances.Add(foliageInstance);
			}

			compIndex = FMath::RandRange(0, componentSize - 1);
			tmpX++;

			if (tmpX >= max) {
				tmpX = 0;
				tmpY++;
			}
		}

		tileX++;

		if (tileX >= Super::Size) {
			tileX = 0;
			tileY++;
		}
	}
}

// This should to be done on a different thread if possible
void AFoliageTileActor::UpdateTile(int32 x, int32 y, FVector location) {
	UFoliageTile* tile = FoliageTiles[GetIndex(x, y)];
	uint32 seed = Hash(InitialSeed + (x * Super::Size) + y);

	for (int32 i = 0; i < tile->Instances.Num(); i++)
	{
		float spawnNoise = (USimplexNoise::SimplexNoise2D((location.X + tile->Instances[i].Location.X) / SpawnNoiseSize, (location.Y + tile->Instances[i].Location.Y) / SpawnNoiseSize) + 1) / 2.0f;

		if (spawnNoise < SpawnNoiseMin || spawnNoise > SpawnNoiseMax)
			continue;

		float noise = (USimplexNoise::SimplexNoise2D((location.X + tile->Instances[i].Location.X) / ScaleNoiseSize, (location.Y + tile->Instances[i].Location.Y) / ScaleNoiseSize) + 1) / 2.0f;
		tile->Instances[i].Scale = ScaleMin + ((ScaleMax - ScaleMin) * noise);
		FTransform transform = GetTransform(location + tile->Instances[i].Location, seed);
		transform.SetScale3D(FVector(tile->Instances[i].Scale, tile->Instances[i].Scale, tile->Instances[i].Scale));
		tile->MeshComponents[tile->Instances[i].ComponentIndex]->UpdateInstanceTransform(tile->Instances[i].InstanceId, transform, true);		
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
			if (PhysicalMaterials.Num() > 0) {
				if (HitData.PhysMaterial == NULL) {
					return result;
				}

				auto name = HitData.PhysMaterial->GetName();
				bool found = false;

				for (int i = 0; i < PhysicalMaterials.Num(); i++)
				{
					if (PhysicalMaterials[i]->GetName() == name)
						found = true;
				}

				if (found == false)
					return result;
			}

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

void AFoliageTileActor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	int32 arraySize = (int32)sqrt((double)MeshesPrTile);
	TotalMeshes = Super::Size * Super::Size * arraySize * arraySize * SpawnChance;
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