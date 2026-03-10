#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesSaveGame.generated.h"

UCLASS()
class MANYNAMES_API UManyNamesSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
	FManyNamesWorldState WorldState;
};
