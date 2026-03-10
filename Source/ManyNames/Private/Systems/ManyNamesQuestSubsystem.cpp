#include "Systems/ManyNamesQuestSubsystem.h"

namespace
{
	FString GetRegionQuestPrefix(const EManyNamesRegionId RegionId)
	{
		switch (RegionId)
		{
		case EManyNamesRegionId::Opening:
			return TEXT("opening_");
		case EManyNamesRegionId::Egypt:
			return TEXT("egypt_");
		case EManyNamesRegionId::Greece:
			return TEXT("greece_");
		case EManyNamesRegionId::ItalicWest:
			return TEXT("italic_");
		case EManyNamesRegionId::Convergence:
			return TEXT("convergence_");
		default:
			return FString();
		}
	}
}

EManyNamesQuestState UManyNamesQuestSubsystem::GetQuestState(FName QuestId) const
{
	if (const EManyNamesQuestState* State = QuestStates.Find(QuestId))
	{
		return *State;
	}

	return EManyNamesQuestState::Locked;
}

void UManyNamesQuestSubsystem::SetQuestState(FName QuestId, EManyNamesQuestState NewState)
{
	QuestStates.FindOrAdd(QuestId) = NewState;
}

bool UManyNamesQuestSubsystem::IsQuestAvailable(const FManyNamesQuestRow& QuestRow) const
{
	for (const FName& Prerequisite : QuestRow.PrerequisiteQuestIds)
	{
		if (GetQuestState(Prerequisite) != EManyNamesQuestState::Completed)
		{
			return false;
		}
	}

	return true;
}

TArray<FName> UManyNamesQuestSubsystem::GetActiveQuestsForRegion(EManyNamesRegionId RegionId) const
{
	TArray<FName> Results;
	const FString Prefix = GetRegionQuestPrefix(RegionId);
	for (const TPair<FName, EManyNamesQuestState>& Pair : QuestStates)
	{
		if (Pair.Value == EManyNamesQuestState::Active && Pair.Key.ToString().StartsWith(Prefix))
		{
			Results.Add(Pair.Key);
		}
	}

	return Results;
}
