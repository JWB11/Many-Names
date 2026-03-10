#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/Actor.h"
#include "ManyNamesDialogueController.generated.h"

UCLASS(Blueprintable)
class MANYNAMES_API AManyNamesDialogueController : public AActor
{
	GENERATED_BODY()

public:
	AManyNamesDialogueController();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Dialogue")
	void OpenDialogue(FName QuestId, const FText& PromptOverride = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category="ManyNames|Dialogue")
	TArray<FManyNamesDialogueChoiceRow> GetAvailableChoices(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Dialogue")
	bool ResolveChoiceByIndex(int32 SelectionIndex);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Dialogue")
	void CloseDialogue();

	UFUNCTION(BlueprintPure, Category="ManyNames|Dialogue")
	bool IsDialogueOpen() const { return bDialogueOpen; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Dialogue")
	FText GetCurrentPrompt() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|Dialogue")
	FText GetChoiceMenuText() const;

private:
	bool HasRequiredDomains(const FManyNamesDialogueChoiceRow& Choice) const;
	void ApplyConsequenceRecord(const FManyNamesDialogueChoiceRow& ChoiceRow);
	void ApplyCompanionOutput(FName OutputId) const;

	class UManyNamesContentSubsystem* GetContentSubsystem() const;
	class UManyNamesWorldStateSubsystem* GetWorldStateSubsystem() const;
	class UManyNamesMythSubsystem* GetMythSubsystem() const;
	class UManyNamesQuestSubsystem* GetQuestSubsystem() const;
	class AManyNamesPrototypeGameMode* GetManyNamesGameMode() const;

	UPROPERTY(Transient)
	TArray<FManyNamesDialogueChoiceRow> CurrentChoices;

	UPROPERTY(Transient)
	FName CurrentQuestId = NAME_None;

	UPROPERTY(Transient)
	FText CurrentPrompt;

	UPROPERTY(Transient)
	bool bDialogueOpen = false;
};
