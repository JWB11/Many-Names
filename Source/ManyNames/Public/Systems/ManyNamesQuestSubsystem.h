#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesQuestSubsystem.generated.h"

UCLASS()
class MANYNAMES_API UManyNamesQuestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ManyNames|Quests")
	EManyNamesQuestState GetQuestState(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Quests")
	void SetQuestState(FName QuestId, EManyNamesQuestState NewState);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Quests")
	bool IsQuestAvailable(const FManyNamesQuestRow& QuestRow) const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Quests")
	TArray<FName> GetActiveQuestsForRegion(EManyNamesRegionId RegionId) const;

private:
	UPROPERTY(Transient)
	TMap<FName, EManyNamesQuestState> QuestStates;
};
