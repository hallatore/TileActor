#include "Landscape_1.h"
#include "TileUtils.h"

uint32 TileUtils::Hash(uint32 a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

FVector TileUtils::GetCurrentCameraLocation(UWorld* world)
{
	if (world == nullptr)
		return emptyFVector;

	auto player = world->GetFirstLocalPlayerFromController();

	if (player != NULL)
		return player->LastViewLocation;

	if (world->ViewLocationsRenderedLastFrame.Num() > 0)
		return world->ViewLocationsRenderedLastFrame[0];

	return emptyFVector;
}

bool TileUtils::IsEmptyFVector(FVector location)
{
	return location.X == emptyFVector.X &&
		location.Y == emptyFVector.Y &&
		location.Z == emptyFVector.Z;
}