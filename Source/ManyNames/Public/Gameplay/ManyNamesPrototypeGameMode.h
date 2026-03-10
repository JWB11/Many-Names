#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/GameModeBase.h"
#include "ManyNamesPrototypeGameMode.generated.h"

class AManyNamesDialogueController;

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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Flow")
	TObjectPtr<AManyNamesDialogueController> DialogueController;

private:
	void BootstrapCurrentMap();
	void BootstrapOpeningMap();
	void BootstrapEgyptMap();
	void BootstrapGreeceMap();
	void BootstrapItalicMap();
	void BootstrapConvergenceMap();
	void ReconcileQuestStates();
	void ActivateQuestIfLocked(FName QuestId);
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
};
