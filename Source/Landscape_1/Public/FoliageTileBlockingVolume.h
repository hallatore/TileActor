#pragma once

#include "GameFramework/Volume.h"
#include "FoliageTileBlockingVolume.generated.h"

UCLASS()
class LANDSCAPE_1_API AFoliageTileBlockingVolume : public AVolume
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FName> FoliageLayers;
};
