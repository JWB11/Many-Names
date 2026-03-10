#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ManyNamesMythSubsystem.generated.h"

UCLASS()
class MANYNAMES_API UManyNamesMythSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ManyNames|Myth")
	int32 GetDomainScore(FGameplayTag DomainTag) const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Myth")
	void AddDomainResonance(FGameplayTag DomainTag, int32 Delta, bool bPubliclyVisible);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Myth")
	void AddRegionalInterpretation(EManyNamesRegionId RegionId, FGameplayTag InterpretationTag);

private:
	class UManyNamesGameInstance* GetManyNamesGameInstance() const;
};
