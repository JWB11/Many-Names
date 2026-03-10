#pragma once

#include "Commandlets/Commandlet.h"
#include "ManyNamesWorldBuildCommandlet.generated.h"

UCLASS()
class MANYNAMES_API UManyNamesWorldBuildCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UManyNamesWorldBuildCommandlet();

	virtual int32 Main(const FString& Params) override;
};
