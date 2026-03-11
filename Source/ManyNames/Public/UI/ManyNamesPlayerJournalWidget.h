#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "Blueprint/UserWidget.h"
#include "ManyNamesPlayerJournalWidget.generated.h"

UCLASS(Blueprintable)
class MANYNAMES_API UManyNamesPlayerJournalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ManyNames|Journal")
	void RefreshFromWorldState(const FManyNamesWorldState& WorldState);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Journal")
	void RefreshQuestList();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Journal")
	void RefreshDomains();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Journal")
	void RefreshRumors();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Journal")
	void RefreshEndings();

	UFUNCTION(BlueprintPure, Category="ManyNames|Journal")
	FText GetJournalSummary() const { return JournalSummary; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Journal")
	FText GetRegionSummary() const { return RegionSummary; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Journal")
	FText GetQuestSummary() const { return QuestSummary; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Journal")
	FText GetDomainSummary() const { return DomainSummary; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Journal")
	FText GetRumorSummary() const { return RumorSummary; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Journal")
	FText GetEndingSummary() const { return EndingSummary; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText JournalSummary;

	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText RegionSummary;

	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText QuestSummary;

	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText DomainSummary;

	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText RumorSummary;

	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText EndingSummary;
};
