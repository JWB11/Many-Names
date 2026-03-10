#include "Gameplay/ManyNamesDialogueController.h"

#include "Gameplay/ManyNamesPrototypeGameMode.h"
#include "GameplayTagsManager.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesMythSubsystem.h"
#include "Systems/ManyNamesQuestSubsystem.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"

AManyNamesDialogueController::AManyNamesDialogueController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AManyNamesDialogueController::OpenDialogue(FName QuestId, const FText& PromptOverride)
{
	CurrentQuestId = QuestId;
	CurrentChoices = GetAvailableChoices(QuestId);
	CurrentPrompt = PromptOverride;
	CurrentScene = FManyNamesDialogueSceneRecord();

	if (UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem())
	{
		ContentSubsystem->GetDialogueSceneForQuest(QuestId, CurrentScene);
	}

	if (CurrentPrompt.IsEmpty() && CurrentChoices.Num() > 0)
	{
		CurrentPrompt = !CurrentScene.BodyText.IsEmpty() ? CurrentScene.BodyText : CurrentChoices[0].Prompt;
	}

	if (CurrentPrompt.IsEmpty() && !CurrentScene.BodyText.IsEmpty())
	{
		CurrentPrompt = CurrentScene.BodyText;
	}

	if (QuestId == TEXT("convergence_main_01"))
	{
		if (const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
		{
			const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
			if (WorldState.bHasDominantAntagonist)
			{
				FString AntagonistLine;
				switch (WorldState.DominantAntagonist)
				{
				case EManyNamesCompanionId::SkyRuler:
					AntagonistLine = TEXT("\n\nThe loudest hostile signal in the chamber now comes from the storm-ruler branch: spectacle, punishment, and weather as obedience.");
					break;
				case EManyNamesCompanionId::BronzeLawgiver:
					AntagonistLine = TEXT("\n\nThe loudest hostile signal in the chamber now comes from the lawgiver branch: measured coercion, public order, and systems that never stop counting.");
					break;
				case EManyNamesCompanionId::OracleAI:
				default:
					AntagonistLine = TEXT("\n\nThe loudest hostile signal in the chamber now comes from the oracle branch: secrecy, curated truth, and memory controlled as infrastructure.");
					break;
				}

				CurrentPrompt = FText::FromString(CurrentPrompt.ToString() + AntagonistLine);
			}
		}
	}

	bDialogueOpen = CurrentChoices.Num() > 0;

	if (bDialogueOpen && !CurrentScene.WeatherStateId.IsNone())
	{
		if (AManyNamesPrototypeGameMode* GameMode = GetManyNamesGameMode())
		{
			GameMode->ApplyWeatherState(CurrentScene.WeatherStateId);
		}
	}
}

TArray<FManyNamesDialogueChoiceRow> AManyNamesDialogueController::GetAvailableChoices(FName QuestId) const
{
	TArray<FManyNamesDialogueChoiceRow> Results;
	if (const UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem())
	{
		for (const FManyNamesDialogueChoiceRow& Choice : ContentSubsystem->GetDialogueChoicesForQuest(QuestId))
		{
			if (HasRequiredDomains(Choice))
			{
				Results.Add(Choice);
			}
		}
	}

	return Results;
}

bool AManyNamesDialogueController::ResolveChoiceByIndex(int32 SelectionIndex)
{
	if (!bDialogueOpen || !CurrentChoices.IsValidIndex(SelectionIndex))
	{
		return false;
	}

	ApplyConsequenceRecord(CurrentChoices[SelectionIndex]);
	CloseDialogue();
	return true;
}

void AManyNamesDialogueController::CloseDialogue()
{
	CurrentChoices.Reset();
	CurrentQuestId = NAME_None;
	CurrentPrompt = FText::GetEmpty();
	CurrentScene = FManyNamesDialogueSceneRecord();
	bDialogueOpen = false;

	if (AManyNamesPrototypeGameMode* GameMode = GetManyNamesGameMode())
	{
		GameMode->RestoreBaselineWeather();
	}
}

FText AManyNamesDialogueController::GetCurrentPrompt() const
{
	return CurrentPrompt;
}

FText AManyNamesDialogueController::GetChoiceMenuText() const
{
	TStringBuilder<2048> Builder;
	if (!CurrentScene.SpeakerName.IsEmpty())
	{
		Builder.Append(CurrentScene.SpeakerName.ToString());
		if (!CurrentScene.SpeakerRole.IsEmpty())
		{
			Builder.Append(TEXT(" - "));
			Builder.Append(CurrentScene.SpeakerRole.ToString());
		}
		Builder.Append(TEXT("\n"));
	}

	if (!CurrentPrompt.IsEmpty())
	{
		Builder.Append(CurrentPrompt.ToString());
		Builder.Append(TEXT("\n"));
	}

	for (int32 Index = 0; Index < CurrentChoices.Num(); ++Index)
	{
		Builder.Appendf(TEXT("%d. %s\n"), Index + 1, *CurrentChoices[Index].OptionText.ToString());
	}

	return FText::FromString(Builder.ToString());
}

bool AManyNamesDialogueController::HasRequiredDomains(const FManyNamesDialogueChoiceRow& Choice) const
{
	const UManyNamesMythSubsystem* MythSubsystem = GetMythSubsystem();
	if (!MythSubsystem)
	{
		return false;
	}

	for (const FGameplayTag& RequiredTag : Choice.RequiredDomains)
	{
		if (!RequiredTag.IsValid() || MythSubsystem->GetDomainScore(RequiredTag) <= 0)
		{
			return false;
		}
	}

	return true;
}

void AManyNamesDialogueController::ApplyConsequenceRecord(const FManyNamesDialogueChoiceRow& ChoiceRow)
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	UManyNamesMythSubsystem* MythSubsystem = GetMythSubsystem();
	AManyNamesPrototypeGameMode* GameMode = GetManyNamesGameMode();
	if (!ContentSubsystem || !WorldStateSubsystem || !MythSubsystem || !GameMode)
	{
		return;
	}

	WorldStateSubsystem->ApplyChoice(ChoiceRow.ChoiceId, ChoiceRow.ChoiceId);

	FManyNamesChoiceConsequenceRecord Consequence;
	if (ContentSubsystem->GetChoiceConsequence(ChoiceRow.ChoiceId, Consequence))
	{
		for (const FName& OutputId : Consequence.WorldStateOutputs)
		{
			WorldStateSubsystem->AddWorldStateOutput(OutputId);
			ApplyCompanionOutput(OutputId);
		}

		const bool bPublicChoice = Consequence.RumorEffects.PublicMiracle > Consequence.RumorEffects.Concealment;
		for (const TPair<FName, int32>& DomainDelta : Consequence.DomainDeltas)
		{
			const FGameplayTag DomainTag = UGameplayTagsManager::Get().RequestGameplayTag(DomainDelta.Key, false);
			if (DomainTag.IsValid())
			{
				MythSubsystem->AddDomainResonance(DomainTag, DomainDelta.Value, bPublicChoice);
			}
		}

		WorldStateSubsystem->ApplyRumorEffect(Consequence.RumorEffects);

		for (const TPair<FName, int32>& CompanionDelta : Consequence.CompanionAffinityEffects)
		{
			EManyNamesCompanionId CompanionId;
			if (ContentSubsystem->TryConvertCompanionName(CompanionDelta.Key, CompanionId))
			{
				WorldStateSubsystem->UpdateCompanionAffinity(CompanionId, CompanionDelta.Value);
			}
		}

		for (const FName& EligibleEnding : Consequence.EligibleEndings)
		{
			EManyNamesEndingId EndingId;
			if (ContentSubsystem->TryConvertEndingName(EligibleEnding, EndingId))
			{
				WorldStateSubsystem->SetEligibleEnding(EndingId, true);
			}
		}
	}

	WorldStateSubsystem->RefreshDominantAntagonist();

	FManyNamesQuestRow QuestRow;
	if (ContentSubsystem->GetQuestRow(ChoiceRow.QuestId, QuestRow))
	{
		GameMode->HandleQuestCompleted(QuestRow.QuestId, QuestRow.WorldStateOutputId);
	}
}

void AManyNamesDialogueController::ApplyCompanionOutput(FName OutputId) const
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (!ContentSubsystem || !WorldStateSubsystem)
	{
		return;
	}

	const FString OutputString = OutputId.ToString();
	if (!OutputString.StartsWith(TEXT("Companion.")))
	{
		return;
	}

	TArray<FString> Parts;
	OutputString.ParseIntoArray(Parts, TEXT("."));
	if (Parts.Num() < 3)
	{
		return;
	}

	EManyNamesCompanionId CompanionId;
	if (!ContentSubsystem->TryConvertCompanionName(FName(*Parts[1]), CompanionId))
	{
		return;
	}

	if (Parts[2] == TEXT("Allied"))
	{
		WorldStateSubsystem->SetCompanionAllianceState(CompanionId, EManyNamesAllianceState::Allied);
	}
	else if (Parts[2] == TEXT("Opposed"))
	{
		WorldStateSubsystem->SetCompanionAllianceState(CompanionId, EManyNamesAllianceState::Opposed);
		WorldStateSubsystem->AddCompanionEscalation(CompanionId, 4);
	}
	else if (Parts[2] == TEXT("Replaced"))
	{
		WorldStateSubsystem->SetCompanionAllianceState(CompanionId, EManyNamesAllianceState::Replaced);
		WorldStateSubsystem->AddCompanionEscalation(CompanionId, 3);
	}
	else if (Parts[2] == TEXT("TruthRevealed"))
	{
		WorldStateSubsystem->SetCompanionTruthRevealed(CompanionId, true);
		WorldStateSubsystem->AddCompanionEscalation(CompanionId, 2);
	}
	else if (Parts[2] == TEXT("Ascendant"))
	{
		WorldStateSubsystem->AddCompanionEscalation(CompanionId, 5);
	}
	else if (Parts[2] == TEXT("Dominant"))
	{
		WorldStateSubsystem->AddCompanionEscalation(CompanionId, 8);
	}
}

UManyNamesContentSubsystem* AManyNamesDialogueController::GetContentSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesContentSubsystem>() : nullptr;
}

UManyNamesWorldStateSubsystem* AManyNamesDialogueController::GetWorldStateSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>() : nullptr;
}

UManyNamesMythSubsystem* AManyNamesDialogueController::GetMythSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesMythSubsystem>() : nullptr;
}

UManyNamesQuestSubsystem* AManyNamesDialogueController::GetQuestSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UManyNamesQuestSubsystem>() : nullptr;
}

AManyNamesPrototypeGameMode* AManyNamesDialogueController::GetManyNamesGameMode() const
{
	return GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
}
