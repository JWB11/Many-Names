#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesDeveloperSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Many Names Prototype"))
class MANYNAMES_API UManyNamesDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString RegionsJsonPath = TEXT("Data/regions.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString QuestsJsonPath = TEXT("Data/quests.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString DialogueChoicesJsonPath = TEXT("Data/dialogue_choices.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FSoftObjectPath RegionDataTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FSoftObjectPath QuestDataTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FSoftObjectPath DialogueChoiceDataTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString QuestStepsJsonPath = TEXT("Data/quest_steps.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString ChoiceConsequencesJsonPath = TEXT("Data/choice_consequences.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString EndingGatesJsonPath = TEXT("Data/ending_gates.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Save")
	FString DefaultSaveSlot = TEXT("ManyNames_Autosave");
};
