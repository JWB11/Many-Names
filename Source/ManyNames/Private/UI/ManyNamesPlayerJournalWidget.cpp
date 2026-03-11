#include "UI/ManyNamesPlayerJournalWidget.h"

#include "Gameplay/ManyNamesPrototypeGameMode.h"
#include "GameplayTagsManager.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesQuestSubsystem.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"

namespace
{
FString QuestStateLabel(EManyNamesQuestState QuestState)
{
	switch (QuestState)
	{
	case EManyNamesQuestState::Available:
		return TEXT("Available");
	case EManyNamesQuestState::Active:
		return TEXT("Active");
	case EManyNamesQuestState::Completed:
		return TEXT("Completed");
	case EManyNamesQuestState::Failed:
		return TEXT("Failed");
	case EManyNamesQuestState::Escalated:
		return TEXT("Escalated");
	case EManyNamesQuestState::Locked:
	default:
		return TEXT("Locked");
	}
}

FString DomainShortName(const FGameplayTag& DomainTag)
{
	return DomainTag.GetTagName().ToString().Replace(TEXT("Domain."), TEXT(""));
}

EManyNamesRegionId ResolveRegionFromLevelName(const UWorld* World)
{
	if (!World)
	{
		return EManyNamesRegionId::Opening;
	}

	const FString LevelName = World->GetMapName();
	if (LevelName.Contains(TEXT("Egypt")))
	{
		return EManyNamesRegionId::Egypt;
	}
	if (LevelName.Contains(TEXT("Greece")))
	{
		return EManyNamesRegionId::Greece;
	}
	if (LevelName.Contains(TEXT("Italic")))
	{
		return EManyNamesRegionId::ItalicWest;
	}
	if (LevelName.Contains(TEXT("Convergence")))
	{
		return EManyNamesRegionId::Convergence;
	}
	return EManyNamesRegionId::Opening;
}
}

void UManyNamesPlayerJournalWidget::RefreshFromWorldState(const FManyNamesWorldState& WorldState)
{
	if (const AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		JournalSummary = GameMode->GetJournalSummaryText();
	}
	else
	{
		JournalSummary = FText::FromString(TEXT("Journal unavailable."));
	}

	if (const UManyNamesContentSubsystem* ContentSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesContentSubsystem>() : nullptr)
	{
		const EManyNamesRegionId CurrentRegionId = ResolveRegionFromLevelName(GetWorld());

		FManyNamesRegionBriefRecord RegionBrief;
		if (ContentSubsystem->GetRegionBrief(CurrentRegionId, RegionBrief))
		{
			FString RegionText = FString::Printf(
				TEXT("%s\n%s\nPublic belief: %s"),
				*RegionBrief.CourtDisplayName.ToString(),
				*RegionBrief.HubSummary.ToString(),
				*RegionBrief.SurfaceBeliefText.ToString());
			if (!RegionBrief.HiddenTruthText.IsEmpty())
			{
				RegionText += FString::Printf(TEXT("\nBuried truth: %s"), *RegionBrief.HiddenTruthText.ToString());
			}
			RegionSummary = FText::FromString(RegionText);
		}
		else
		{
			RegionSummary = FText::GetEmpty();
		}
	}

	RefreshQuestList();
	RefreshDomains();
	RefreshRumors();
	RefreshEndings();
}

void UManyNamesPlayerJournalWidget::RefreshQuestList()
{
	const UManyNamesContentSubsystem* ContentSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesContentSubsystem>() : nullptr;
	const UManyNamesQuestSubsystem* QuestSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UManyNamesQuestSubsystem>() : nullptr;
	if (!ContentSubsystem || !QuestSubsystem)
	{
		QuestSummary = FText::FromString(TEXT("Quest data unavailable."));
		return;
	}

	TArray<FManyNamesQuestRow> QuestRows = ContentSubsystem->GetAllQuestRows();
	QuestRows.Sort([](const FManyNamesQuestRow& Left, const FManyNamesQuestRow& Right)
	{
		if (Left.RegionId != Right.RegionId)
		{
			return static_cast<uint8>(Left.RegionId) < static_cast<uint8>(Right.RegionId);
		}
		return Left.QuestId.LexicalLess(Right.QuestId);
	});

	TStringBuilder<4096> Builder;
	EManyNamesRegionId CurrentRegionId = EManyNamesRegionId::Convergence;
	bool bHasRegionHeader = false;
	for (const FManyNamesQuestRow& QuestRow : QuestRows)
	{
		if (!bHasRegionHeader || CurrentRegionId != QuestRow.RegionId)
		{
			CurrentRegionId = QuestRow.RegionId;
			bHasRegionHeader = true;
			Builder.Appendf(TEXT("%s\n"), *UEnum::GetDisplayValueAsText(CurrentRegionId).ToString());
		}

		const FString QuestType = QuestRow.QuestId.ToString().Contains(TEXT("_main_")) ? TEXT("Main") : TEXT("Side");
		Builder.Appendf(TEXT("  [%s] %s - %s\n"),
			*QuestType,
			*QuestRow.Title.ToString(),
			*QuestStateLabel(QuestSubsystem->GetQuestState(QuestRow.QuestId)));
	}

	QuestSummary = FText::FromString(Builder.ToString());
}

void UManyNamesPlayerJournalWidget::RefreshDomains()
{
	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>() : nullptr;
	if (!WorldStateSubsystem)
	{
		DomainSummary = FText::FromString(TEXT("Domain profile unavailable."));
		return;
	}

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	TArray<FGameplayTag> DomainTags;
	WorldState.MythicDomainProfile.DomainScores.GetKeys(DomainTags);
	DomainTags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		return Left.ToString() < Right.ToString();
	});

	TStringBuilder<1024> Builder;
	for (const FGameplayTag& DomainTag : DomainTags)
	{
		if (const int32* Score = WorldState.MythicDomainProfile.DomainScores.Find(DomainTag))
		{
			Builder.Appendf(TEXT("%s: %d\n"), *DomainShortName(DomainTag), *Score);
		}
	}

	DomainSummary = FText::FromString(Builder.ToString());
}

void UManyNamesPlayerJournalWidget::RefreshRumors()
{
	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>() : nullptr;
	if (!WorldStateSubsystem)
	{
		RumorSummary = FText::FromString(TEXT("Rumor profile unavailable."));
		return;
	}

	const FManyNamesRumorState Rumors = WorldStateSubsystem->GetWorldState().RumorProfile;
	RumorSummary = FText::FromString(FString::Printf(
		TEXT("Public miracle: %d\nConcealment: %d\nCombat reputation: %d"),
		Rumors.PublicMiracleScore,
		Rumors.ConcealmentScore,
		Rumors.CombatReputation));
}

void UManyNamesPlayerJournalWidget::RefreshEndings()
{
	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>() : nullptr;
	if (!WorldStateSubsystem)
	{
		EndingSummary = FText::FromString(TEXT("Ending profile unavailable."));
		return;
	}

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	TStringBuilder<1024> Builder;
	Builder.Appendf(TEXT("Eligible endings: %d\n"), WorldState.EligibleEndings.Num());
	EManyNamesCompanionId DominantCompanionId = EManyNamesCompanionId::OracleAI;
	if (WorldStateSubsystem->TryGetDominantAntagonist(DominantCompanionId))
	{
		Builder.Appendf(TEXT("Dominant antagonist: %s\n"), *UEnum::GetDisplayValueAsText(DominantCompanionId).ToString());
	}
	else
	{
		Builder.Append(TEXT("Dominant antagonist: unresolved\n"));
	}

	EndingSummary = FText::FromString(Builder.ToString());
}
