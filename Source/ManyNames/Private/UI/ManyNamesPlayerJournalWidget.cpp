#include "UI/ManyNamesPlayerJournalWidget.h"

void UManyNamesPlayerJournalWidget::RefreshFromWorldState(const FManyNamesWorldState& WorldState)
{
	JournalSummary = FText::FromString(FString::Printf(
		TEXT("Outputs: %d | Powers: %d | Endings: %d"),
		WorldState.WorldStateOutputs.Num(),
		WorldState.UnlockedPowers.Num(),
		WorldState.EligibleEndings.Num()));
}

void UManyNamesPlayerJournalWidget::RefreshQuestList()
{
}

void UManyNamesPlayerJournalWidget::RefreshDomains()
{
}

void UManyNamesPlayerJournalWidget::RefreshRumors()
{
}

void UManyNamesPlayerJournalWidget::RefreshEndings()
{
}
