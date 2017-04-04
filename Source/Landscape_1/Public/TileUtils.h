#pragma once

const FVector emptyFVector = FVector(0.0f, 0.0f, -100000.0f);

class LANDSCAPE_1_API TileUtils
{
public:
	static uint32 Hash(uint32 a);
	static FVector GetCurrentCameraLocation(UWorld* world);
	static bool IsEmptyFVector(FVector location);
};
