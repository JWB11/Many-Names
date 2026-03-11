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
	TArray<FManyNamesDialogueSceneRecord> GetDialogueScenesForQuest(FName QuestId) const;

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
	bool GetRegionBrief(EManyNamesRegionId RegionId, FManyNamesRegionBriefRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesRegionBriefRecord> GetAllRegionBriefs() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetCourtFaction(FName FactionId, FManyNamesCourtFactionRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesCourtFactionRecord> GetCourtFactionsForRegion(EManyNamesRegionId RegionId) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesCourtFactionRecord> GetAllCourtFactions() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetCinematicScene(FName SceneId, FManyNamesCinematicSceneRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesCinematicSceneRecord> GetAllCinematicScenes() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesCinematicSceneRecord> GetCinematicScenesForQuest(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetAudioProfile(FName AudioId, FManyNamesAudioProfileRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesAudioProfileRecord> GetAllAudioProfiles() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	bool GetExternalAssetLicense(FName AssetId, FManyNamesExternalAssetLicenseRecord& OutRecord) const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Content")
	TArray<FManyNamesExternalAssetLicenseRecord> GetAllExternalAssetLicenses() const;

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
	bool LoadRegionBriefs();
	bool LoadCourtFactions();
	bool LoadCinematicScenes();
	bool LoadAudioProfiles();
	bool LoadExternalAssetLicenses();
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

	TMap<FName, TArray<FManyNamesDialogueSceneRecord>> DialogueScenesByQuestId;

	UPROPERTY(Transient)
	TArray<FManyNamesCharacterCastRecord> CharacterCast;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesCharacterCastRecord> CharacterCastById;

	UPROPERTY(Transient)
	TArray<FManyNamesAmbientProfileRecord> AmbientProfiles;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesAmbientProfileRecord> AmbientProfilesById;

	UPROPERTY(Transient)
	TArray<FManyNamesRegionBriefRecord> RegionBriefs;

	UPROPERTY(Transient)
	TMap<EManyNamesRegionId, FManyNamesRegionBriefRecord> RegionBriefsById;

	UPROPERTY(Transient)
	TArray<FManyNamesCourtFactionRecord> CourtFactions;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesCourtFactionRecord> CourtFactionsById;

	TMap<EManyNamesRegionId, TArray<FManyNamesCourtFactionRecord>> CourtFactionsByRegionId;

	UPROPERTY(Transient)
	TArray<FManyNamesCinematicSceneRecord> CinematicScenes;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesCinematicSceneRecord> CinematicScenesById;

	TMap<FName, TArray<FManyNamesCinematicSceneRecord>> CinematicScenesByQuestId;

	UPROPERTY(Transient)
	TArray<FManyNamesAudioProfileRecord> AudioProfiles;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesAudioProfileRecord> AudioProfilesById;

	UPROPERTY(Transient)
	TArray<FManyNamesExternalAssetLicenseRecord> ExternalAssetLicenses;

	UPROPERTY(Transient)
	TMap<FName, FManyNamesExternalAssetLicenseRecord> ExternalAssetLicensesById;
};
