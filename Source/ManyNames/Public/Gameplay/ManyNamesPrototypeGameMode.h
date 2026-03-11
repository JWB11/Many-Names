#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/GameModeBase.h"
#include "ManyNamesPrototypeGameMode.generated.h"

class AManyNamesDialogueController;
class ALevelSequenceActor;
class UAudioComponent;
class ULevelSequencePlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FManyNamesStatusMessageEvent, const FText&, Message, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FManyNamesJournalUpdatedEvent, const FText&, Summary);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FManyNamesDialogueStateEvent, FName, QuestId, bool, bIsOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FManyNamesCinematicStateEvent, FName, SceneId, bool, bIsPlaying);

UCLASS(Blueprintable)
class MANYNAMES_API AManyNamesPrototypeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AManyNamesPrototypeGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void InitializePrototypeRun();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void HandleQuestCompleted(FName QuestId, FName WorldStateOutputId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void HandleRegionResolved(EManyNamesRegionId RegionId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void OpenRegionSelect();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void TryEnterConvergence();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void StartRegionTravel(EManyNamesRegionId RegionId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void OpenQuestDialogue(FName QuestId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void HandleMenuSelection(int32 SelectionIndex);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void TriggerFirstMiracle();

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	bool IsAwaitingRegionSelection() const { return bAwaitingRegionSelection; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	bool IsDialogueOpen() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	bool IsDialogueMovementLocked() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	FText GetMenuPromptText() const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void ApplyWeatherState(FName WeatherStateId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	void RestoreBaselineWeather();

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	FString GetJournalSummary() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	FText GetJournalSummaryText() const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Flow")
	bool SkipActiveCinematic();

	UFUNCTION(BlueprintPure, Category="ManyNames|Flow")
	bool IsCinematicPlaying() const;

	UPROPERTY(BlueprintAssignable, Category="ManyNames|UI")
	FManyNamesStatusMessageEvent OnStatusMessage;

	UPROPERTY(BlueprintAssignable, Category="ManyNames|UI")
	FManyNamesJournalUpdatedEvent OnJournalUpdated;

	UPROPERTY(BlueprintAssignable, Category="ManyNames|UI")
	FManyNamesDialogueStateEvent OnDialogueStateChanged;

	UPROPERTY(BlueprintAssignable, Category="ManyNames|UI")
	FManyNamesCinematicStateEvent OnCinematicStateChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Flow")
	TObjectPtr<AManyNamesDialogueController> DialogueController;

private:
	bool ResolveRegionFromLevelName(const FString& LevelName, EManyNamesRegionId& OutRegionId) const;
	void BootstrapCurrentMap();
	void BootstrapOpeningMap();
	void BootstrapEgyptMap();
	void BootstrapGreeceMap();
	void BootstrapItalicMap();
	void BootstrapConvergenceMap();
	void BootstrapRegionFromContent(EManyNamesRegionId RegionId, const FString& ObjectiveMessage, bool bRequireConvergenceUnlock = false);
	void ReconcileQuestStates();
	void ActivateQuestIfLocked(FName QuestId);
	void ActivateRegionQuestsIfAvailable(EManyNamesRegionId RegionId);
	bool AreQuestPrerequisitesMet(const FManyNamesQuestRow& QuestRow) const;
	bool AreRegionEntryConditionsMet(const FManyNamesRegionRow& RegionRow) const;
	bool IsRegionMainQuest(const FManyNamesQuestRow& QuestRow) const;
	bool IsRegionSideQuest(const FManyNamesQuestRow& QuestRow) const;
	FName GetRegionCompletionOutputId(EManyNamesRegionId RegionId) const;
	void BroadcastJournalUpdated() const;
	void OpenDialogueInternal(FName QuestId);
	bool TryPlaySceneVoice(FName SceneId);
	bool TryPlayQuestCinematic(FName QuestId, const FString& SceneToken = FString());
	bool TryPlaySceneById(FName SceneId, FName DialogueQuestId = NAME_None);
	void FinishActiveCinematic(bool bWasSkipped);
	bool HasWorldStateOutput(FName OutputId) const;
	void AddQuestRewards(const FManyNamesQuestRow& QuestRow);
	void ShowStatusMessage(const FString& Message, FColor Color = FColor::Cyan) const;
	EManyNamesRegionId GetCurrentRegionId() const;
	bool IsConvergenceUnlocked() const;

	class UManyNamesContentSubsystem* GetContentSubsystem() const;
	class UManyNamesQuestSubsystem* GetQuestSubsystem() const;
	class UManyNamesWorldStateSubsystem* GetWorldStateSubsystem() const;
	class UManyNamesMythSubsystem* GetMythSubsystem() const;
	class AManyNamesEnvironmentController* GetEnvironmentController() const;

	bool bAwaitingRegionSelection = false;
	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveSceneAudioComponents;

	UPROPERTY(Transient)
	FName ActiveCinematicSceneId = NAME_None;

	UPROPERTY(Transient)
	FName PendingDialogueQuestId = NAME_None;
};
