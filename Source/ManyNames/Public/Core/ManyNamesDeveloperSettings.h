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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString DialogueScenesJsonPath = TEXT("Data/dialogue_scenes.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString CharacterCastJsonPath = TEXT("Data/character_cast.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString AmbientProfilesJsonPath = TEXT("Data/ambient_profiles.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString CinematicScenesJsonPath = TEXT("Data/cinematic_scenes.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString AudioProfilesJsonPath = TEXT("Data/audio_profiles.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Data")
	FString ExternalAssetLicensesJsonPath = TEXT("Data/external_asset_licenses.json");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Save")
	FString DefaultSaveSlot = TEXT("ManyNames_Autosave");
};
