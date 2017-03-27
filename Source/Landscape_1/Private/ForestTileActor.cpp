// Fill out your copyright notice in the Description page of Project Settings.

#include "Landscape_1.h"
#include "ForestTileActor.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Kismet/KismetSystemLibrary.h"
#include "math.h"

AForestTileActor::AForestTileActor() {
	SpawnChance = 1;
	FoliageSpawnChance = 1;
	TreesPerTile = 1;
	FoliageStartCullDistance = 30000;
	FoliageEndCullDistance = 40000;
}

AForestTileActor::~AForestTileActor() {
	ForestTiles.Empty();
}

void AForestTileActor::BeginPlay()
{
	Super::BeginPlay();

	ForestTiles.AddUninitialized(Super::Size * Super::Size);
	uint32 seed = Hash(InitialSeed);

	float tilesize = Radius * 2 / Super::Size;
	float startCullDistance = Radius - (tilesize * 2.5f);
	float endCullDistance = Radius - (tilesize * 1.5f);

	for (int32 y = 0; y < Super::Size * Super::Size; y++)
	{
		UForestTile* tile = NewObject<UForestTile>(this, NAME_None);
		TArray<UHierarchicalInstancedStaticMeshComponent*> components;

		for (int32 i = 0; i < Trees.Num(); i++)
		{
			seed = Hash(seed);
			UHierarchicalInstancedStaticMeshComponent* component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			component->StaticMesh = Trees[i].Mesh;
			component->bSelectable = false;
			component->bHasPerInstanceHitProxies = true;
			component->InstancingRandomSeed = seed % INT32_MAX;

			//@todo - take the settings from a UFoliageType object.  For now, disable distance field lighting on grass so we don't hitch.
			//component->bAffectDistanceFieldLighting = false;

			component->InstanceStartCullDistance = startCullDistance;
			component->InstanceEndCullDistance = endCullDistance;

			if (Trees[i].Collision == false)
				component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			else
				component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			component->AttachTo(GetRootComponent());
			component->RegisterComponent();
			components.Add(component);
		}

		for (int32 i = 0; i < FoliageAroundTrees.Num(); i++)
		{
			seed = Hash(seed);
			UHierarchicalInstancedStaticMeshComponent* component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			component->StaticMesh = FoliageAroundTrees[i].Mesh;
			component->bSelectable = false;
			component->bHasPerInstanceHitProxies = true;
			component->InstancingRandomSeed = seed % INT32_MAX;

			//@todo - take the settings from a UFoliageType object.  For now, disable distance field lighting on grass so we don't hitch.
			//component->bAffectDistanceFieldLighting = false;

			component->InstanceStartCullDistance = FoliageStartCullDistance;
			component->InstanceEndCullDistance = FoliageEndCullDistance;

			if (FoliageAroundTrees[i].Collision == false)
				component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			else
				component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			component->AttachTo(GetRootComponent());
			component->RegisterComponent();
			components.Add(component);
		}

		tile->MeshComponents = components;
		ForestTiles[y] = tile;
	}

	Generate();
}

void AForestTileActor::Generate() {
	if (Trees.Num() == 0)
		return;

	double StartTime = FPlatformTime::Seconds();
	uint32 seed = Hash(InitialSeed);
	TArray<FTreeInstance> instances;

	float initialForestTilesize = Radius * 2 / Super::Size;

	for (int32 i = 0; i < Super::Size; i++)
	{
		for (int32 y = 0; y < Super::Size; y++)
		{
			for (int32 prTile = 0; prTile < TreesPerTile; prTile++)
			{
				seed = Hash(seed);

				if ((double)seed / UINT32_MAX > SpawnChance)
					continue;

				seed = Hash(seed);
				float r1 = (double)seed / UINT32_MAX;
				seed = Hash(seed);
				float r2 = (double)seed / UINT32_MAX;

				FTreeInstance treeInstance = FTreeInstance();
				treeInstance.Location = FVector(initialForestTilesize * i + initialForestTilesize * r1, initialForestTilesize * y + initialForestTilesize * r2, 0.0f);

				treeInstance.MeshIndex = seed % Trees.Num();
				treeInstance.Bounds = Trees[treeInstance.MeshIndex].Bounds;
				treeInstance.Scale = Trees[treeInstance.MeshIndex].ScaleMax;

				if (TestLocation(treeInstance.Location, treeInstance.Bounds, instances))
					instances.Add(treeInstance);
			}
		}
	}

	int32 mainInstancesCount = instances.Num();

	for (int32 i = 0; i < mainInstancesCount; i++)
	{
		seed = Hash(seed);
		AddChildren(instances[i], 0, instances, seed);
	}

	float ForestTilesize = Radius * 2 / Super::Size;

	for (int32 i = 0; i < instances.Num(); i++)
	{
		FTreeInstance & instance = instances[i];
		int32 x = (int32)floor(instance.Location.X / ForestTilesize);
		int32 y = (int32)floor(instance.Location.Y / ForestTilesize);

		if (x > Super::Size - 1)
			x = Super::Size - 1;

		if (y > Super::Size - 1)
			y = Super::Size - 1;

		int32 index = GetIndex(x, y);
		instance.Location -= FVector(x * ForestTilesize, y * ForestTilesize, 0.0f); // Make location relative to tile cell
		UForestTile* tile = ForestTiles[index];

		if (tile)
		{
			instance.InstanceId = tile->MeshComponents[instance.MeshIndex]->AddInstance(FTransform(FVector(0.0f, 0.0f, -1000000.0f)));
			tile->Instances.Add(instance);

			for (int32 y = 0; y < FoliageAroundTrees.Num(); y++)
			{
				FForestFoliageMesh & mesh = FoliageAroundTrees[y];

				for (int32 x = 0; x < mesh.MaxPerTree; x++)
				{
					seed = Hash(seed);

					if ((double)seed / UINT32_MAX > FoliageSpawnChance)
						continue;

					FTreeInstance foliageInstance = FTreeInstance();
					foliageInstance.MeshIndex = y;
					foliageInstance.IsFoliage = true;

					seed = Hash(seed);
					float r1 = ((double)seed / UINT32_MAX) - 0.5f;

					seed = Hash(seed);
					float r2 = ((double)seed / UINT32_MAX);

					seed = Hash(seed);
					float rad = seed % 57;
					float bounds = mesh.MinBounds + (mesh.MaxBounds - mesh.MinBounds) * r2;
					float locationX = bounds * cos(rad);
					float locationY = bounds * sin(rad);

					foliageInstance.Scale = mesh.ScaleMin + (mesh.ScaleMax - mesh.ScaleMin) * r1;
					foliageInstance.Location = instance.Location + FVector(locationX, locationY, 0.0f);

					foliageInstance.InstanceId = tile->MeshComponents[Trees.Num() + foliageInstance.MeshIndex]->AddInstance(FTransform(FVector(0.0f, 0.0f, -1000000.0f)));
					tile->Instances.Add(foliageInstance);
				}

			}
		}
	}

	instances.Empty();

	UE_LOG(LogStaticMesh, Display, TEXT("Generated %d trees in %.3fs."), instances.Num(), float(FPlatformTime::Seconds() - StartTime));
}

void AForestTileActor::AddChildren(FTreeInstance parent, int32 step, TArray<FTreeInstance> & instances, uint32 seed) {
	FForestMesh forestMesh = Trees[parent.MeshIndex];

	step = step++;

	if (step > forestMesh.NumberOfSteps)
		return;

	for (int i = 0; i < forestMesh.ChildrenPerTree; i++)
	{
		seed = Hash(seed);
		FVector childLocation = FVector();
		int32 childMeshIndex = seed % Trees.Num();
		FForestMesh childMesh = Trees[childMeshIndex];

		float r = ((double)seed / UINT32_MAX) * 0.2f;
		float r2 = 1.0f;

		if (forestMesh.NumberOfSteps > 0) // Divide by zero check
			r2 = 1.0f - ((double)step / forestMesh.NumberOfSteps);

		float childScale = childMesh.ScaleMin + ((childMesh.ScaleMax - childMesh.ScaleMin) * r2) + ((childMesh.ScaleMax - childMesh.ScaleMin) * r);
		float childBounds = childMesh.Bounds * childScale;
		float r3 = 1.0f + (0.5f * ((double)seed / UINT32_MAX));
		float bounds = ((childBounds + parent.Bounds) / 2) * r3;

		if (FindLocation(seed, parent.Location, bounds, instances, childLocation)) {
			FTreeInstance treeInstance = FTreeInstance();
			treeInstance.MeshIndex = childMeshIndex;
			treeInstance.Location = childLocation;
			treeInstance.Bounds = childBounds;
			treeInstance.Scale = childScale;
			instances.Add(treeInstance);

			AddChildren(treeInstance, step, instances, seed);
			//UE_LOG(LogTemp, Display, TEXT("Child: %f, %f"), childLocation.X, childLocation.Y);
		}
	}
}

bool AForestTileActor::FindLocation(uint32 seed, FVector parentLocation, float bounds, TArray<FTreeInstance> & instances, FVector & childLocation) {

	seed = Hash(seed);
	float scaler = seed / UINT32_MAX;
	seed = Hash(seed);
	float r1 = ((double)seed / UINT32_MAX) + 1.0f;
	seed = Hash(seed);
	float r2 = ((double)seed / UINT32_MAX) + 1.0f;
	seed = Hash(seed);
	float rad = seed % 57;
	float locationX = bounds * cos(rad) * r1;
	float locationY = bounds * sin(rad) * r2;

	childLocation = parentLocation + FVector(locationX, locationY, 0.0f);

	if (childLocation.X < 1.0f)
		childLocation.X = Radius * 2 + childLocation.X;

	if (childLocation.X > Radius * 2 - 1)
		childLocation.X = 0.0f + childLocation.X;

	if (childLocation.Y < 1.0f)
		childLocation.Y = Radius * 2 + childLocation.Y;

	if (childLocation.Y > Radius * 2 - 1)
		childLocation.Y = 0.0f + childLocation.Y;

	return TestLocation(childLocation, bounds, instances);
}

bool AForestTileActor::TestLocation(FVector location, float bounds, TArray<FTreeInstance> & instances) {
	for (int32 i = 0; i < instances.Num(); i++)
	{
		float x = location.X - instances[i].Location.X;

		if (x < 0.0f)
			x *= -1;

		if (x > bounds * 2)
			continue;

		float y = location.Y - instances[i].Location.Y;

		if (y < 0.0f)
			y *= -1;

		if (y > bounds * 2)
			continue;

		if (FVector::Dist(location, instances[i].Location) < bounds)
			return false;
	}

	return true;
}

void AForestTileActor::UpdateTile(int32 x, int32 y, FVector location) {
	if (Trees.Num() == 0)
		return;

	UForestTile* tile = ForestTiles[GetIndex(x, y)];
	uint32 seed = Hash(InitialSeed + (x * Super::Size) + y);

	for (int32 i = 0; i < tile->Instances.Num(); i++)
	{
		seed = Hash(seed);
		FTreeInstance instance = tile->Instances[i];

		if (instance.IsFoliage == false) {
			FForestMesh mesh = Trees[instance.MeshIndex];
			FVector newLocation = location + instance.Location;
			FTransform transform = GetTransform(newLocation, seed, mesh.AlignWithSlope, mesh.HeightMin, mesh.HeightMax, mesh.MaxSlope, mesh.PhysicalMaterials);
			transform.SetScale3D(FVector(instance.Scale, instance.Scale, instance.Scale));
			tile->MeshComponents[instance.MeshIndex]->UpdateInstanceTransform(instance.InstanceId, transform, true);
		}
		else {
			FForestFoliageMesh mesh = FoliageAroundTrees[instance.MeshIndex];
			FVector newLocation = location + instance.Location;
			FTransform transform = GetTransform(newLocation, seed, mesh.AlignWithSlope, mesh.HeightMin, mesh.HeightMax, mesh.MaxSlope, mesh.PhysicalMaterials);
			transform.SetScale3D(FVector(instance.Scale, instance.Scale, instance.Scale));
			tile->MeshComponents[Trees.Num() + instance.MeshIndex]->UpdateInstanceTransform(instance.InstanceId, transform, true);
		}
	}
}

FTransform AForestTileActor::GetTransform(FVector location, uint32 seed, bool AlignWithSlope, float HeightMin, float HeightMax, float MaxSlope, TArray<UPhysicalMaterial*> PhysicalMaterials) {
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

	if (UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), Start, End, Objects, false, ActorsToIgnore, EDrawDebugTrace::None, HitData, true))
	{
		if (HitData.GetActor() && HitData.Actor->IsA(ALandscape::StaticClass()))
		{
			float angle = GetAngle(Start, HitData);

			if (HitData.ImpactPoint.Z < HeightMin || HitData.ImpactPoint.Z > HeightMax || angle > MaxSlope)
				return result;

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

void AForestTileActor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}

float AForestTileActor::GetAngle(FVector start, FHitResult HitData) {
	float slope = HitData.ImpactPoint.Size() / HitData.ImpactNormal.Size();
	FVector impact = start - HitData.ImpactPoint;
	impact.Normalize();
	float DotProduct = FVector::DotProduct(impact, HitData.ImpactNormal);
	float radians = acosf(DotProduct);
	return FMath::RadiansToDegrees(radians);
}

int32 AForestTileActor::GetIndex(int32 x, int32 y) {
	int32 result = 0;

	for (int32 x2 = 0; x2 < Super::Size; x2++)
	{
		for (int32 y2 = 0; y2 < Super::Size; y2++)
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

uint32 AForestTileActor::Hash(uint32 a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}


