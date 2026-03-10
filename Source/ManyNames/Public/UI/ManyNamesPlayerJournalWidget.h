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

protected:
	UPROPERTY(BlueprintReadOnly, Category="ManyNames|Journal")
	FText JournalSummary;
};
