#include "Gameplay/ManyNamesPrototypeGameMode.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Gameplay/ManyNamesDialogueController.h"
#include "Gameplay/ManyNamesFirstPersonCharacter.h"
#include "GameplayTagsManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesMythSubsystem.h"
#include "Systems/ManyNamesQuestSubsystem.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"

AManyNamesPrototypeGameMode::AManyNamesPrototypeGameMode()
{
	DefaultPawnClass = AManyNamesFirstPersonCharacter::StaticClass();
}

void AManyNamesPrototypeGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!DialogueController && GetWorld())
	{
		DialogueController = GetWorld()->SpawnActor<AManyNamesDialogueController>();
	}

	InitializePrototypeRun();
}

void AManyNamesPrototypeGameMode::InitializePrototypeRun()
{
	if (UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem())
	{
		ContentSubsystem->ReloadPrimaryContent();
		ContentSubsystem->ReloadSupplementalContent();
	}

	ReconcileQuestStates();
	BootstrapCurrentMap();
}

void AManyNamesPrototypeGameMode::HandleQuestCompleted(FName QuestId, FName WorldStateOutputId)
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (!ContentSubsystem || !QuestSubsystem || !WorldStateSubsystem)
	{
		return;
	}

	FManyNamesQuestRow QuestRow;
	if (!ContentSubsystem->GetQuestRow(QuestId, QuestRow))
	{
		return;
	}

	if (QuestSubsystem->GetQuestState(QuestId) == EManyNamesQuestState::Completed ||
		(!QuestRow.WorldStateOutputId.IsNone() && WorldStateSubsystem->HasWorldStateOutput(QuestRow.WorldStateOutputId)))
	{
		ShowStatusMessage(TEXT("That quest has already been resolved in this save."), FColor::Yellow);
		return;
	}

	QuestSubsystem->SetQuestState(QuestId, EManyNamesQuestState::Completed);

	if (!WorldStateOutputId.IsNone())
	{
		WorldStateSubsystem->AddWorldStateOutput(WorldStateOutputId);
	}

	AddQuestRewards(QuestRow);

	if (QuestId == TEXT("opening_main_01"))
	{
		WorldStateSubsystem->UnlockPower(EManyNamesPowerId::InsightPulse);
		ActivateQuestIfLocked(TEXT("opening_side_01"));
		OpenRegionSelect();
		ShowStatusMessage(TEXT("First miracle triggered. The witness now defines whether your myth stays hidden."), FColor::Yellow);
	}
	else if (QuestId == TEXT("opening_side_01"))
	{
		WorldStateSubsystem->SetRegionCompleted(EManyNamesRegionId::Opening, true);
		WorldStateSubsystem->AddWorldStateOutput(TEXT("State.Region.Opening.Complete"));
		WorldStateSubsystem->SetRegionUnlocked(EManyNamesRegionId::Egypt, true);
		WorldStateSubsystem->SetRegionUnlocked(EManyNamesRegionId::Greece, true);
		WorldStateSubsystem->SetRegionUnlocked(EManyNamesRegionId::ItalicWest, true);
		OpenRegionSelect();
		ShowStatusMessage(TEXT("Region travel unlocked. Egypt, Greece, and Italic West are now staged in this build."), FColor::Green);
	}
	else if (QuestId == TEXT("egypt_main_01"))
	{
		HandleRegionResolved(EManyNamesRegionId::Egypt);
		ShowStatusMessage(TEXT("Egypt main quest resolved. The archive now reflects your choice."), FColor::Green);
	}
	else if (QuestId == TEXT("greece_main_01"))
	{
		HandleRegionResolved(EManyNamesRegionId::Greece);
		ShowStatusMessage(TEXT("Greece main quest resolved. The sanctuary now carries your version of the storm sign."), FColor::Green);
	}
	else if (QuestId == TEXT("italic_main_01"))
	{
		HandleRegionResolved(EManyNamesRegionId::ItalicWest);
		ShowStatusMessage(TEXT("Italic West main quest resolved. The law-road now reflects your judgment."), FColor::Green);
	}
	else if (QuestId == TEXT("convergence_main_01"))
	{
		HandleRegionResolved(EManyNamesRegionId::Convergence);
		ShowStatusMessage(TEXT("Convergence resolved. One of the final legacy paths is now recorded in this save."), FColor::Green);
	}
	else if (QuestId == TEXT("egypt_side_01") || QuestId == TEXT("greece_side_01") || QuestId == TEXT("italic_side_01"))
	{
		ShowStatusMessage(TEXT("Side quest resolved. Journal and rumor state updated."), FColor::Green);
	}
}

void AManyNamesPrototypeGameMode::HandleRegionResolved(EManyNamesRegionId RegionId)
{
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (!WorldStateSubsystem)
	{
		return;
	}

	WorldStateSubsystem->SetRegionCompleted(RegionId, true);
	switch (RegionId)
	{
	case EManyNamesRegionId::Egypt:
		WorldStateSubsystem->AddWorldStateOutput(TEXT("State.Region.Egypt.Complete"));
		break;
	case EManyNamesRegionId::Greece:
		WorldStateSubsystem->AddWorldStateOutput(TEXT("State.Region.Greece.Complete"));
		break;
	case EManyNamesRegionId::ItalicWest:
		WorldStateSubsystem->AddWorldStateOutput(TEXT("State.Region.ItalicWest.Complete"));
		break;
	case EManyNamesRegionId::Convergence:
		WorldStateSubsystem->AddWorldStateOutput(TEXT("State.Region.Convergence.Complete"));
		break;
	default:
		break;
	}

	TryEnterConvergence();
}

void AManyNamesPrototypeGameMode::OpenRegionSelect()
{
	bAwaitingRegionSelection = true;
}

void AManyNamesPrototypeGameMode::TryEnterConvergence()
{
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (HasWorldStateOutput(TEXT("State.Region.Egypt.Complete")) &&
		HasWorldStateOutput(TEXT("State.Region.Greece.Complete")) &&
		HasWorldStateOutput(TEXT("State.Region.ItalicWest.Complete")))
	{
		if (WorldStateSubsystem)
		{
			WorldStateSubsystem->SetRegionUnlocked(EManyNamesRegionId::Convergence, true);
		}
		OpenRegionSelect();
		ShowStatusMessage(TEXT("Convergence is now unlocked."), FColor::Green);
	}
}

void AManyNamesPrototypeGameMode::StartRegionTravel(EManyNamesRegionId RegionId)
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (!ContentSubsystem || !WorldStateSubsystem)
	{
		return;
	}

	if ((RegionId == EManyNamesRegionId::Egypt || RegionId == EManyNamesRegionId::Greece || RegionId == EManyNamesRegionId::ItalicWest) &&
		!HasWorldStateOutput(TEXT("State.Region.Opening.Complete")))
	{
		ShowStatusMessage(TEXT("Travel is locked until the witness scene is resolved."), FColor::Red);
		return;
	}

	FManyNamesRegionRow RegionRow;
	if (!ContentSubsystem->GetRegionRow(RegionId, RegionRow))
	{
		ShowStatusMessage(TEXT("Region data is missing."), FColor::Red);
		return;
	}

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	const FManyNamesRegionState* RegionState = WorldState.Regions.Find(RegionId);
	if (!RegionState || !RegionState->bUnlocked)
	{
		ShowStatusMessage(TEXT("That region is still locked in this save."), FColor::Red);
		return;
	}

	const FString PackageName = RegionRow.HubMap.GetLongPackageName();
	if (PackageName.IsEmpty() || !FPackageName::DoesPackageExist(PackageName))
	{
		ShowStatusMessage(TEXT("That region is not staged in this build yet."), FColor::Red);
		return;
	}

	bAwaitingRegionSelection = false;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TSoftObjectPtr<UWorld>(RegionRow.HubMap));
}

void AManyNamesPrototypeGameMode::OpenQuestDialogue(FName QuestId)
{
	if (!DialogueController)
	{
		return;
	}

	if (UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem())
	{
		FManyNamesQuestRow QuestRow;
		if (ContentSubsystem->GetQuestRow(QuestId, QuestRow))
		{
			if (HasWorldStateOutput(QuestRow.WorldStateOutputId))
			{
				ShowStatusMessage(TEXT("That encounter has already been resolved."), FColor::Yellow);
				return;
			}
		}
	}

	DialogueController->OpenDialogue(QuestId);
	if (!DialogueController->IsDialogueOpen())
	{
		ShowStatusMessage(TEXT("No valid dialogue options are currently unlocked for that encounter."), FColor::Red);
	}
}

void AManyNamesPrototypeGameMode::HandleMenuSelection(int32 SelectionIndex)
{
	if (DialogueController && DialogueController->IsDialogueOpen())
	{
		if (!DialogueController->ResolveChoiceByIndex(SelectionIndex))
		{
			ShowStatusMessage(TEXT("That dialogue choice is not available."), FColor::Red);
		}
		return;
	}

	if (!bAwaitingRegionSelection)
	{
		return;
	}

	switch (SelectionIndex)
	{
	case 0:
		StartRegionTravel(EManyNamesRegionId::Egypt);
		break;
	case 1:
		StartRegionTravel(EManyNamesRegionId::Greece);
		break;
	case 2:
		StartRegionTravel(EManyNamesRegionId::ItalicWest);
		break;
	case 3:
		StartRegionTravel(EManyNamesRegionId::Convergence);
		break;
	default:
		ShowStatusMessage(TEXT("Select 1, 2, 3, or 4."), FColor::Red);
		break;
	}
}

void AManyNamesPrototypeGameMode::TriggerFirstMiracle()
{
	if (HasWorldStateOutput(TEXT("Story.Prologue.Complete")))
	{
		ShowStatusMessage(TEXT("The opening miracle has already been triggered."), FColor::Yellow);
		return;
	}

	HandleQuestCompleted(TEXT("opening_main_01"), TEXT("Story.Prologue.Complete"));
}

bool AManyNamesPrototypeGameMode::IsDialogueOpen() const
{
	return DialogueController && DialogueController->IsDialogueOpen();
}

FText AManyNamesPrototypeGameMode::GetMenuPromptText() const
{
	if (DialogueController && DialogueController->IsDialogueOpen())
	{
		return DialogueController->GetChoiceMenuText();
	}

	if (bAwaitingRegionSelection)
	{
		if (IsConvergenceUnlocked())
		{
			return FText::FromString(TEXT("Choose a destination:\n1. Egypt\n2. Greece\n3. Italic West\n4. Convergence"));
		}

		return FText::FromString(TEXT("Choose your first region:\n1. Egypt\n2. Greece\n3. Italic West"));
	}

	return FText::GetEmpty();
}

FString AManyNamesPrototypeGameMode::GetJournalSummary() const
{
	const UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (!QuestSubsystem || !WorldStateSubsystem)
	{
		return TEXT("Journal unavailable.");
	}

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	TStringBuilder<2048> Builder;
	Builder.Append(TEXT("Many Names Journal\n"));
	Builder.Appendf(TEXT("Visited regions: %d\n"), WorldState.RegionVisitOrder.Num());
	Builder.Appendf(TEXT("Known outputs: %d\n"), WorldState.WorldStateOutputs.Num());
	Builder.Appendf(TEXT("Public miracle: %d | Concealment: %d | Combat: %d\n"),
		WorldState.RumorProfile.PublicMiracleScore,
		WorldState.RumorProfile.ConcealmentScore,
		WorldState.RumorProfile.CombatReputation);

	const auto AppendQuestSummary = [&Builder, QuestSubsystem](const TCHAR* Label, FName QuestId)
	{
		Builder.Appendf(TEXT("%s: %d\n"), Label, static_cast<int32>(QuestSubsystem->GetQuestState(QuestId)));
	};

	AppendQuestSummary(TEXT("opening_main_01"), TEXT("opening_main_01"));
	AppendQuestSummary(TEXT("opening_side_01"), TEXT("opening_side_01"));
	AppendQuestSummary(TEXT("egypt_main_01"), TEXT("egypt_main_01"));
	AppendQuestSummary(TEXT("egypt_side_01"), TEXT("egypt_side_01"));
	AppendQuestSummary(TEXT("greece_main_01"), TEXT("greece_main_01"));
	AppendQuestSummary(TEXT("greece_side_01"), TEXT("greece_side_01"));
	AppendQuestSummary(TEXT("italic_main_01"), TEXT("italic_main_01"));
	AppendQuestSummary(TEXT("italic_side_01"), TEXT("italic_side_01"));
	AppendQuestSummary(TEXT("convergence_main_01"), TEXT("convergence_main_01"));

	return Builder.ToString();
}

void AManyNamesPrototypeGameMode::BootstrapCurrentMap()
{
	if (UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
	{
		WorldStateSubsystem->RegisterRegionVisit(GetCurrentRegionId());
	}

	switch (GetCurrentRegionId())
	{
	case EManyNamesRegionId::Opening:
		BootstrapOpeningMap();
		break;
	case EManyNamesRegionId::Egypt:
		BootstrapEgyptMap();
		break;
	case EManyNamesRegionId::Greece:
		BootstrapGreeceMap();
		break;
	case EManyNamesRegionId::ItalicWest:
		BootstrapItalicMap();
		break;
	case EManyNamesRegionId::Convergence:
		BootstrapConvergenceMap();
		break;
	default:
		break;
	}
}

void AManyNamesPrototypeGameMode::BootstrapOpeningMap()
{
	ActivateQuestIfLocked(TEXT("opening_main_01"));

	if (HasWorldStateOutput(TEXT("Story.Prologue.Complete")))
	{
		ActivateQuestIfLocked(TEXT("opening_side_01"));
	}

	if (HasWorldStateOutput(TEXT("State.Region.Opening.Complete")))
	{
		OpenRegionSelect();
	}

	ShowStatusMessage(TEXT("Opening objective: trigger the miracle, speak to the witness, then choose your first region."), FColor::Cyan);
}

void AManyNamesPrototypeGameMode::BootstrapEgyptMap()
{
	if (HasWorldStateOutput(TEXT("State.Region.Opening.Complete")))
	{
		ActivateQuestIfLocked(TEXT("egypt_main_01"));
		ActivateQuestIfLocked(TEXT("egypt_side_01"));
		ShowStatusMessage(TEXT("Egypt objective: enter the archive and resolve the fate of the Radiant Voice."), FColor::Cyan);
	}
	else
	{
		ShowStatusMessage(TEXT("Egypt should only be reachable after the opening witness scene."), FColor::Red);
	}
}

void AManyNamesPrototypeGameMode::BootstrapGreeceMap()
{
	if (HasWorldStateOutput(TEXT("State.Region.Opening.Complete")))
	{
		ActivateQuestIfLocked(TEXT("greece_main_01"));
		ActivateQuestIfLocked(TEXT("greece_side_01"));
		ShowStatusMessage(TEXT("Greece objective: confront the High One of Storm and decide whether spectacle becomes law or fraud."), FColor::Cyan);
	}
	else
	{
		ShowStatusMessage(TEXT("Greece should only be reachable after the opening witness scene."), FColor::Red);
	}
}

void AManyNamesPrototypeGameMode::BootstrapItalicMap()
{
	if (HasWorldStateOutput(TEXT("State.Region.Opening.Complete")))
	{
		ActivateQuestIfLocked(TEXT("italic_main_01"));
		ActivateQuestIfLocked(TEXT("italic_side_01"));
		ShowStatusMessage(TEXT("Italic West objective: test whether measure and boundary become service or domination."), FColor::Cyan);
	}
	else
	{
		ShowStatusMessage(TEXT("Italic West should only be reachable after the opening witness scene."), FColor::Red);
	}
}

void AManyNamesPrototypeGameMode::BootstrapConvergenceMap()
{
	if (IsConvergenceUnlocked())
	{
		ActivateQuestIfLocked(TEXT("convergence_main_01"));
		ShowStatusMessage(TEXT("Convergence objective: descend into the wreck below history and decide which legacy survives."), FColor::Cyan);
	}
	else
	{
		ShowStatusMessage(TEXT("Convergence remains sealed until Egypt, Greece, and Italic West are all resolved."), FColor::Red);
	}
}

void AManyNamesPrototypeGameMode::ReconcileQuestStates()
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	if (!ContentSubsystem || !QuestSubsystem)
	{
		return;
	}

	for (const FManyNamesQuestRow& QuestRow : ContentSubsystem->GetAllQuestRows())
	{
		if (!QuestRow.WorldStateOutputId.IsNone() && HasWorldStateOutput(QuestRow.WorldStateOutputId))
		{
			QuestSubsystem->SetQuestState(QuestRow.QuestId, EManyNamesQuestState::Completed);
		}
	}
}

void AManyNamesPrototypeGameMode::ActivateQuestIfLocked(FName QuestId)
{
	UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	if (!QuestSubsystem)
	{
		return;
	}

	if (QuestSubsystem->GetQuestState(QuestId) == EManyNamesQuestState::Locked)
	{
		QuestSubsystem->SetQuestState(QuestId, EManyNamesQuestState::Active);
	}
}

bool AManyNamesPrototypeGameMode::HasWorldStateOutput(FName OutputId) const
{
	if (const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
	{
		return WorldStateSubsystem->HasWorldStateOutput(OutputId);
	}

	return false;
}

void AManyNamesPrototypeGameMode::AddQuestRewards(const FManyNamesQuestRow& QuestRow)
{
	UManyNamesMythSubsystem* MythSubsystem = GetMythSubsystem();
	if (!MythSubsystem)
	{
		return;
	}

	for (const FGameplayTag& RewardTag : QuestRow.RewardDomains)
	{
		if (RewardTag.IsValid())
		{
			MythSubsystem->AddDomainResonance(RewardTag, 1, false);
		}
	}
}

void AManyNamesPrototypeGameMode::ShowStatusMessage(const FString& Message, FColor Color) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, Color, Message);
	}
}

EManyNamesRegionId AManyNamesPrototypeGameMode::GetCurrentRegionId() const
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (LevelName.Contains(TEXT("Opening")))
	{
		return EManyNamesRegionId::Opening;
	}
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

	return EManyNamesRegionId::Convergence;
}

UManyNamesContentSubsystem* AManyNamesPrototypeGameMode::GetContentSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesContentSubsystem>() : nullptr;
}

UManyNamesQuestSubsystem* AManyNamesPrototypeGameMode::GetQuestSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UManyNamesQuestSubsystem>() : nullptr;
}

UManyNamesWorldStateSubsystem* AManyNamesPrototypeGameMode::GetWorldStateSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>() : nullptr;
}

UManyNamesMythSubsystem* AManyNamesPrototypeGameMode::GetMythSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesMythSubsystem>() : nullptr;
}

bool AManyNamesPrototypeGameMode::IsConvergenceUnlocked() const
{
	if (const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
	{
		const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
		if (const FManyNamesRegionState* RegionState = WorldState.Regions.Find(EManyNamesRegionId::Convergence))
		{
			return RegionState->bUnlocked;
		}
	}

	return false;
}
