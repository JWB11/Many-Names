#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesContentSubsystem.generated.h"

UCLASS()
class MANYNAMES_API UManyNamesContentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Content")
	bool ReloadSupplementalContent();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Content")
	bool ReloadPrimaryContent();

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetRegionRow(EManyNamesRegionId RegionId, FManyNamesRegionRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetQuestRow(FName QuestId, FManyNamesQuestRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesQuestRow> GetAllQuestRows() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesDialogueChoiceRow> GetDialogueChoicesForQuest(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetDialogueSceneForQuest(FName QuestId, FManyNamesDialogueSceneRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesDialogueSceneRecord> GetAllDialogueScenes() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetCharacterCastRecord(FName CharacterId, FManyNamesCharacterCastRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesCharacterCastRecord> GetAllCharacterCastRecords() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetAmbientProfile(FName ProfileId, FManyNamesAmbientProfileRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesAmbientProfileRecord> GetAllAmbientProfiles() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesQuestStepRecord> GetQuestStepsForQuest(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesQuestStepRecord> GetAllQuestSteps() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetChoiceConsequence(FName ChoiceId, FManyNamesChoiceConsequenceRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesChoiceConsequenceRecord> GetAllChoiceConsequences() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetEndingGate(FName EndingId, FManyNamesEndingGateRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesEndingGateRecord> GetAllEndingGates() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool TryConvertEndingName(FName EndingName, EManyNamesEndingId& OutEndingId) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool TryConvertCompanionName(FName CompanionName, EManyNamesCompanionId& OutCompanionId) const;

private:
	bool LoadPrimaryDataTables();
	bool LoadPrimaryRegions();
	bool LoadPrimaryQuests();
	bool LoadPrimaryDialogueChoices();
	bool LoadQuestSteps();
	bool LoadChoiceConsequences();
	bool LoadEndingGates();
	bool LoadDialogueScenes();
	bool LoadCharacterCast();
	bool LoadAmbientProfiles();
	FString ResolveDataPath(const FString& RelativePath) const;

	UPROPERTY(Transient)
	TObjectPtr<class UDataTable> RegionDataTable;

	UPROPERTY(Transient)
	TObjectPtr<class UDataTable> QuestDataTable;

	UPROPERTY(Transient)
	TObjectPtr<class UDataTable> DialogueChoiceDataTable;

	UPROPERTY(Transient)
	TMap<EManyNamesRegionId, FManyNamesRegionRow> RegionsById;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesQuestRow> QuestsById;

	TMap<FName, TArray<FManyNamesDialogueChoiceRow>> DialogueChoicesByQuestId;

	UPROPERTY(Transient)
	TArray<FManyNamesQuestStepRecord> QuestSteps;

	TMap<FName, TArray<FManyNamesQuestStepRecord>> QuestStepsByQuestId;

	UPROPERTY(Transient)
	TArray<FManyNamesChoiceConsequenceRecord> ChoiceConsequences;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesChoiceConsequenceRecord> ChoiceConsequencesByChoiceId;

	UPROPERTY(Transient)
	TArray<FManyNamesEndingGateRecord> EndingGates;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesEndingGateRecord> EndingGatesById;

	UPROPERTY(Transient)
	TArray<FManyNamesDialogueSceneRecord> DialogueScenes;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesDialogueSceneRecord> DialogueScenesByQuestId;

	UPROPERTY(Transient)
	TArray<FManyNamesCharacterCastRecord> CharacterCast;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesCharacterCastRecord> CharacterCastById;

	UPROPERTY(Transient)
	TArray<FManyNamesAmbientProfileRecord> AmbientProfiles;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesAmbientProfileRecord> AmbientProfilesById;
};
