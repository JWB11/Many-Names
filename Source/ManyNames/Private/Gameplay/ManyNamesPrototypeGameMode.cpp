#include "Gameplay/ManyNamesPrototypeGameMode.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Gameplay/ManyNamesDialogueController.h"
#include "Gameplay/ManyNamesEnvironmentController.h"
#include "Gameplay/ManyNamesFirstPersonCharacter.h"
#include "GameplayTagsManager.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Misc/PackageName.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesMythSubsystem.h"
#include "Systems/ManyNamesQuestSubsystem.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"
#include "Sound/SoundBase.h"

namespace
{
	bool QuestIdHasToken(FName QuestId, const TCHAR* Token)
	{
		return QuestId.ToString().Contains(Token);
	}

	bool SceneIdHasToken(FName SceneId, const FString& Token)
	{
		return !Token.IsEmpty() && SceneId.ToString().Contains(Token);
	}

	void SortQuestRows(TArray<FManyNamesQuestRow>& QuestRows)
	{
		QuestRows.Sort([](const FManyNamesQuestRow& Left, const FManyNamesQuestRow& Right)
		{
			if (Left.RegionId != Right.RegionId)
			{
				return static_cast<uint8>(Left.RegionId) < static_cast<uint8>(Right.RegionId);
			}

			const FString LeftId = Left.QuestId.ToString();
			const FString RightId = Right.QuestId.ToString();
			const bool bLeftMain = LeftId.Contains(TEXT("_main_"));
			const bool bRightMain = RightId.Contains(TEXT("_main_"));
			if (bLeftMain != bRightMain)
			{
				return bLeftMain;
			}

			return LeftId < RightId;
		});
	}
}

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
	if (UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
	{
		WorldStateSubsystem->RefreshDominantAntagonist();
	}
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
	ActivateRegionQuestsIfAvailable(QuestRow.RegionId);

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
		WorldStateSubsystem->AddWorldStateOutput(GetRegionCompletionOutputId(EManyNamesRegionId::Opening));
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
	else if (IsRegionMainQuest(QuestRow) && QuestRow.RegionId != EManyNamesRegionId::Opening)
	{
		HandleRegionResolved(QuestRow.RegionId);
		ShowStatusMessage(TEXT("Regional main quest resolved. The local power structure now reflects your chosen myth."), FColor::Green);
	}
	else if (IsRegionSideQuest(QuestRow))
	{
		ShowStatusMessage(TEXT("Side quest resolved. Journal and rumor state updated."), FColor::Green);
	}

	BroadcastJournalUpdated();
}

void AManyNamesPrototypeGameMode::HandleRegionResolved(EManyNamesRegionId RegionId)
{
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (!WorldStateSubsystem)
	{
		return;
	}

	WorldStateSubsystem->SetRegionCompleted(RegionId, true);
	WorldStateSubsystem->AddWorldStateOutput(GetRegionCompletionOutputId(RegionId));

	WorldStateSubsystem->RefreshDominantAntagonist();
	TryEnterConvergence();
	BroadcastJournalUpdated();
}

void AManyNamesPrototypeGameMode::OpenRegionSelect()
{
	bAwaitingRegionSelection = true;
}

void AManyNamesPrototypeGameMode::TryEnterConvergence()
{
	UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	if (HasWorldStateOutput(GetRegionCompletionOutputId(EManyNamesRegionId::Egypt)) &&
		HasWorldStateOutput(GetRegionCompletionOutputId(EManyNamesRegionId::Greece)) &&
		HasWorldStateOutput(GetRegionCompletionOutputId(EManyNamesRegionId::ItalicWest)))
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

	if (TryPlayQuestCinematic(QuestId))
	{
		PendingDialogueQuestId = QuestId;
		return;
	}

	OpenDialogueInternal(QuestId);
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

bool AManyNamesPrototypeGameMode::IsDialogueMovementLocked() const
{
	return DialogueController && DialogueController->IsMovementLocked();
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

void AManyNamesPrototypeGameMode::ApplyWeatherState(FName WeatherStateId)
{
	if (AManyNamesEnvironmentController* EnvironmentController = GetEnvironmentController())
	{
		EnvironmentController->ApplyWeatherState(WeatherStateId);
	}
}

void AManyNamesPrototypeGameMode::RestoreBaselineWeather()
{
	if (AManyNamesEnvironmentController* EnvironmentController = GetEnvironmentController())
	{
		EnvironmentController->ApplyBaselineState();
	}
}

FString AManyNamesPrototypeGameMode::GetJournalSummary() const
{
	const UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem();
	const UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	if (!QuestSubsystem || !WorldStateSubsystem || !ContentSubsystem)
	{
		return TEXT("Journal unavailable.");
	}

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	TStringBuilder<4096> Builder;
	Builder.Append(TEXT("Many Names Journal\n"));
	Builder.Appendf(TEXT("Visited regions: %d\n"), WorldState.RegionVisitOrder.Num());
	Builder.Appendf(TEXT("Known outputs: %d\n"), WorldState.WorldStateOutputs.Num());
	Builder.Appendf(TEXT("Public miracle: %d | Concealment: %d | Combat: %d\n"),
		WorldState.RumorProfile.PublicMiracleScore,
		WorldState.RumorProfile.ConcealmentScore,
		WorldState.RumorProfile.CombatReputation);
	EManyNamesCompanionId DominantCompanionId = EManyNamesCompanionId::OracleAI;
	if (WorldStateSubsystem->TryGetDominantAntagonist(DominantCompanionId))
	{
		const TCHAR* AntagonistName = TEXT("OracleAI");
		switch (DominantCompanionId)
		{
		case EManyNamesCompanionId::SkyRuler:
			AntagonistName = TEXT("SkyRuler");
			break;
		case EManyNamesCompanionId::BronzeLawgiver:
			AntagonistName = TEXT("BronzeLawgiver");
			break;
		default:
			break;
		}
		Builder.Appendf(TEXT("Dominant antagonist path: %s\n"), AntagonistName);
	}

	TArray<FManyNamesQuestRow> QuestRows = ContentSubsystem->GetAllQuestRows();
	SortQuestRows(QuestRows);
	EManyNamesRegionId CurrentRegionHeader = EManyNamesRegionId::Convergence;
	bool bHasRegionHeader = false;
	const auto AppendQuestSummary = [&Builder, QuestSubsystem, this](const FManyNamesQuestRow& QuestRow)
	{
		const EManyNamesQuestState QuestState = QuestSubsystem->GetQuestState(QuestRow.QuestId);
		const TCHAR* QuestKind = IsRegionMainQuest(QuestRow) ? TEXT("Main") : (IsRegionSideQuest(QuestRow) ? TEXT("Side") : TEXT("Quest"));
		const TCHAR* QuestStateLabel = TEXT("Locked");
		switch (QuestState)
		{
		case EManyNamesQuestState::Available:
			QuestStateLabel = TEXT("Available");
			break;
		case EManyNamesQuestState::Active:
			QuestStateLabel = TEXT("Active");
			break;
		case EManyNamesQuestState::Completed:
			QuestStateLabel = TEXT("Completed");
			break;
		case EManyNamesQuestState::Failed:
			QuestStateLabel = TEXT("Failed");
			break;
		case EManyNamesQuestState::Escalated:
			QuestStateLabel = TEXT("Escalated");
			break;
		default:
			break;
		}

		Builder.Appendf(TEXT("  [%s] %s - %s\n"), QuestKind, *QuestRow.Title.ToString(), QuestStateLabel);
	};

	for (const FManyNamesQuestRow& QuestRow : QuestRows)
	{
		if (!bHasRegionHeader || CurrentRegionHeader != QuestRow.RegionId)
		{
			CurrentRegionHeader = QuestRow.RegionId;
			bHasRegionHeader = true;
			Builder.Appendf(TEXT("\n%s\n"), *UEnum::GetDisplayValueAsText(QuestRow.RegionId).ToString());
		}
		AppendQuestSummary(QuestRow);
	}

	return Builder.ToString();
}

FText AManyNamesPrototypeGameMode::GetJournalSummaryText() const
{
	return FText::FromString(GetJournalSummary());
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

	TryPlaySceneById(TEXT("cin_opening_arrival"));
	ShowStatusMessage(TEXT("Opening objective: trigger the miracle, speak to the witness, then choose your first region."), FColor::Cyan);
}

void AManyNamesPrototypeGameMode::BootstrapEgyptMap()
{
	BootstrapRegionFromContent(EManyNamesRegionId::Egypt, TEXT("Egypt objective: resolve the archive dispute, the floodplain petition, and the river ledger without turning the district into a public panic."), false);
}

void AManyNamesPrototypeGameMode::BootstrapGreeceMap()
{
	BootstrapRegionFromContent(EManyNamesRegionId::Greece, TEXT("Greece objective: confront the storm cult, the warband spectacle, and the oath crisis on the ridge."), false);
}

void AManyNamesPrototypeGameMode::BootstrapItalicMap()
{
	BootstrapRegionFromContent(EManyNamesRegionId::ItalicWest, TEXT("Italic West objective: decide whether law, labor, and the road become common structure or measured domination."), false);
}

void AManyNamesPrototypeGameMode::BootstrapConvergenceMap()
{
	BootstrapRegionFromContent(EManyNamesRegionId::Convergence, TEXT("Convergence objective: descend into the buried wreck, confront the ascendant companion path, and decide which legacy survives."), true);
}

void AManyNamesPrototypeGameMode::BootstrapRegionFromContent(EManyNamesRegionId RegionId, const FString& ObjectiveMessage, bool bRequireConvergenceUnlock)
{
	if (!HasWorldStateOutput(GetRegionCompletionOutputId(EManyNamesRegionId::Opening)) && RegionId != EManyNamesRegionId::Opening)
	{
		ShowStatusMessage(FString::Printf(TEXT("%s should only be reachable after the opening witness scene."), *UEnum::GetDisplayValueAsText(RegionId).ToString()), FColor::Red);
		return;
	}

	if (bRequireConvergenceUnlock && !IsConvergenceUnlocked())
	{
		ShowStatusMessage(TEXT("Convergence remains sealed until Egypt, Greece, and Italic West are all resolved."), FColor::Red);
		return;
	}

	FManyNamesRegionRow RegionRow;
	if (const UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem(); ContentSubsystem && ContentSubsystem->GetRegionRow(RegionId, RegionRow))
	{
		if (!AreRegionEntryConditionsMet(RegionRow))
		{
			ShowStatusMessage(TEXT("This region's entry conditions are not yet satisfied in the current save."), FColor::Red);
			return;
		}
	}

	ActivateRegionQuestsIfAvailable(RegionId);
	FName ArrivalQuestId = NAME_None;
	switch (RegionId)
	{
	case EManyNamesRegionId::Egypt:
		ArrivalQuestId = TEXT("egypt_main_01");
		break;
	case EManyNamesRegionId::Greece:
		ArrivalQuestId = TEXT("greece_main_01");
		break;
	case EManyNamesRegionId::ItalicWest:
		ArrivalQuestId = TEXT("italic_main_01");
		break;
	case EManyNamesRegionId::Convergence:
		ArrivalQuestId = TEXT("convergence_main_01");
		break;
	default:
		break;
	}
	if (!ArrivalQuestId.IsNone())
	{
		TryPlayQuestCinematic(ArrivalQuestId, TEXT("arrival"));
	}
	ShowStatusMessage(ObjectiveMessage, FColor::Cyan);
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
			if (IsRegionMainQuest(QuestRow))
			{
				if (UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
				{
					WorldStateSubsystem->SetRegionCompleted(QuestRow.RegionId, true);
					WorldStateSubsystem->AddWorldStateOutput(GetRegionCompletionOutputId(QuestRow.RegionId));
				}
			}
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

void AManyNamesPrototypeGameMode::ActivateRegionQuestsIfAvailable(EManyNamesRegionId RegionId)
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	if (!ContentSubsystem || !QuestSubsystem)
	{
		return;
	}

	TArray<FManyNamesQuestRow> QuestRows = ContentSubsystem->GetAllQuestRows();
	SortQuestRows(QuestRows);
	for (const FManyNamesQuestRow& QuestRow : QuestRows)
	{
		if (QuestRow.RegionId != RegionId)
		{
			continue;
		}

		const EManyNamesQuestState CurrentState = QuestSubsystem->GetQuestState(QuestRow.QuestId);
		if (CurrentState == EManyNamesQuestState::Completed || CurrentState == EManyNamesQuestState::Active)
		{
			continue;
		}

		if (AreQuestPrerequisitesMet(QuestRow))
		{
			ActivateQuestIfLocked(QuestRow.QuestId);
		}
	}
}

bool AManyNamesPrototypeGameMode::AreQuestPrerequisitesMet(const FManyNamesQuestRow& QuestRow) const
{
	const UManyNamesQuestSubsystem* QuestSubsystem = GetQuestSubsystem();
	if (!QuestSubsystem)
	{
		return false;
	}

	return QuestSubsystem->IsQuestAvailable(QuestRow);
}

bool AManyNamesPrototypeGameMode::AreRegionEntryConditionsMet(const FManyNamesRegionRow& RegionRow) const
{
	for (const FName& RequiredOutput : RegionRow.EntryConditionOutputs)
	{
		if (!HasWorldStateOutput(RequiredOutput))
		{
			return false;
		}
	}

	return true;
}

bool AManyNamesPrototypeGameMode::IsRegionMainQuest(const FManyNamesQuestRow& QuestRow) const
{
	return QuestIdHasToken(QuestRow.QuestId, TEXT("_main_"));
}

bool AManyNamesPrototypeGameMode::IsRegionSideQuest(const FManyNamesQuestRow& QuestRow) const
{
	return QuestIdHasToken(QuestRow.QuestId, TEXT("_side_"));
}

bool AManyNamesPrototypeGameMode::HasWorldStateOutput(FName OutputId) const
{
	if (const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem())
	{
		return WorldStateSubsystem->HasWorldStateOutput(OutputId);
	}

	return false;
}

FName AManyNamesPrototypeGameMode::GetRegionCompletionOutputId(EManyNamesRegionId RegionId) const
{
	return UManyNamesWorldStateSubsystem::GetCanonicalRegionCompletionOutput(RegionId);
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

void AManyNamesPrototypeGameMode::BroadcastJournalUpdated() const
{
	OnJournalUpdated.Broadcast(GetJournalSummaryText());
}

void AManyNamesPrototypeGameMode::OpenDialogueInternal(FName QuestId)
{
	if (!DialogueController)
	{
		return;
	}

	DialogueController->OpenDialogue(QuestId);
	OnDialogueStateChanged.Broadcast(QuestId, DialogueController->IsDialogueOpen());
	BroadcastJournalUpdated();

	if (!DialogueController->IsDialogueOpen())
	{
		ShowStatusMessage(TEXT("No valid dialogue options are currently unlocked for that encounter."), FColor::Red);
	}
}

bool AManyNamesPrototypeGameMode::TryPlaySceneVoice(FName SceneId)
{
	if (SceneId.IsNone())
	{
		return false;
	}

	const FString AssetPath = FString::Printf(TEXT("/Game/Audio/Generated/voices/%s.%s"), *SceneId.ToString(), *SceneId.ToString());
	if (USoundBase* VoiceSound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *AssetPath)))
	{
		if (UAudioComponent* Component = UGameplayStatics::SpawnSound2D(this, VoiceSound))
		{
			ActiveSceneAudioComponents.Add(Component);
			return true;
		}
	}

	return false;
}

bool AManyNamesPrototypeGameMode::TryPlayQuestCinematic(FName QuestId, const FString& SceneToken)
{
	const UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	if (!ContentSubsystem)
	{
		return false;
	}

	for (const FManyNamesCinematicSceneRecord& SceneRecord : ContentSubsystem->GetCinematicScenesForQuest(QuestId))
	{
		if (SceneRecord.SceneId.IsNone() ||
			(!SceneToken.IsEmpty() && !SceneIdHasToken(SceneRecord.SceneId, SceneToken)) ||
			HasWorldStateOutput(FName(*FString::Printf(TEXT("State.Cinematic.%s.Played"), *SceneRecord.SceneId.ToString()))))
		{
			continue;
		}

		bool bAllRequiredOutputsPresent = true;
		for (const FName& RequiredOutput : SceneRecord.RequiredWorldStateOutputs)
		{
			if (!HasWorldStateOutput(RequiredOutput))
			{
				bAllRequiredOutputsPresent = false;
				break;
			}
		}

		if (bAllRequiredOutputsPresent && TryPlaySceneById(SceneRecord.SceneId, QuestId))
		{
			return true;
		}
	}

	return false;
}

bool AManyNamesPrototypeGameMode::TryPlaySceneById(FName SceneId, FName DialogueQuestId)
{
	if (IsCinematicPlaying())
	{
		return false;
	}

	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	if (!ContentSubsystem)
	{
		return false;
	}

	FManyNamesCinematicSceneRecord SceneRecord;
	if (!ContentSubsystem->GetCinematicScene(SceneId, SceneRecord))
	{
		return false;
	}

	ULevelSequence* Sequence = Cast<ULevelSequence>(SceneRecord.SequenceAsset.TryLoad());
	if (!Sequence)
	{
		return false;
	}

	if (!SceneRecord.WeatherStateId.IsNone())
	{
		ApplyWeatherState(SceneRecord.WeatherStateId);
	}

	PendingDialogueQuestId = DialogueQuestId;
	ActiveCinematicSceneId = SceneId;
	ALevelSequenceActor* SpawnedSequenceActor = nullptr;
	ActiveSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Sequence, FMovieSceneSequencePlaybackSettings(), SpawnedSequenceActor);
	ActiveSequenceActor = SpawnedSequenceActor;
	if (!ActiveSequencePlayer)
	{
		ActiveCinematicSceneId = NAME_None;
		PendingDialogueQuestId = NAME_None;
		return false;
	}

	for (const FName& AudioId : SceneRecord.AudioProfileIds)
	{
		FManyNamesAudioProfileRecord AudioRecord;
		if (ContentSubsystem->GetAudioProfile(AudioId, AudioRecord))
		{
			if (USoundBase* Sound = Cast<USoundBase>(AudioRecord.SoundAsset.TryLoad()))
			{
				if (UAudioComponent* Component = UGameplayStatics::SpawnSound2D(this, Sound, AudioRecord.VolumeMultiplier))
				{
					ActiveSceneAudioComponents.Add(Component);
				}
			}
		}
	}

	TryPlaySceneVoice(SceneRecord.SceneId);
	OnCinematicStateChanged.Broadcast(SceneId, true);
	ActiveSequencePlayer->OnFinished.AddWeakLambda(this, [this]()
	{
		FinishActiveCinematic(false);
	});
	ActiveSequencePlayer->Play();
	return true;
}

void AManyNamesPrototypeGameMode::FinishActiveCinematic(bool bWasSkipped)
{
	const FName FinishedSceneId = ActiveCinematicSceneId;
	if (UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetWorldStateSubsystem(); !FinishedSceneId.IsNone() && WorldStateSubsystem)
	{
		WorldStateSubsystem->AddWorldStateOutput(FName(*FString::Printf(TEXT("State.Cinematic.%s.Played"), *FinishedSceneId.ToString())));
	}

	for (UAudioComponent* AudioComponent : ActiveSceneAudioComponents)
	{
		if (AudioComponent)
		{
			AudioComponent->Stop();
		}
	}
	ActiveSceneAudioComponents.Reset();

	if (ActiveSequencePlayer)
	{
		ActiveSequencePlayer->Stop();
		ActiveSequencePlayer = nullptr;
	}
	if (ActiveSequenceActor)
	{
		ActiveSequenceActor->Destroy();
		ActiveSequenceActor = nullptr;
	}

	RestoreBaselineWeather();
	ActiveCinematicSceneId = NAME_None;
	OnCinematicStateChanged.Broadcast(FinishedSceneId, false);

	const FName DialogueQuestId = PendingDialogueQuestId;
	PendingDialogueQuestId = NAME_None;
	if (!bWasSkipped && !DialogueQuestId.IsNone())
	{
		OpenDialogueInternal(DialogueQuestId);
	}
}

bool AManyNamesPrototypeGameMode::SkipActiveCinematic()
{
	if (!IsCinematicPlaying())
	{
		return false;
	}

	FinishActiveCinematic(true);
	return true;
}

bool AManyNamesPrototypeGameMode::IsCinematicPlaying() const
{
	return ActiveSequencePlayer != nullptr && !ActiveCinematicSceneId.IsNone();
}

void AManyNamesPrototypeGameMode::ShowStatusMessage(const FString& Message, FColor Color) const
{
	OnStatusMessage.Broadcast(FText::FromString(Message), FLinearColor(Color));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, Color, Message);
	}
}

EManyNamesRegionId AManyNamesPrototypeGameMode::GetCurrentRegionId() const
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;
	if (ResolveRegionFromLevelName(LevelName, RegionId))
	{
		return RegionId;
	}

	UE_LOG(LogTemp, Warning, TEXT("Unknown map '%s' encountered by GetCurrentRegionId(); defaulting to Opening-safe behavior."), *LevelName);
	return EManyNamesRegionId::Opening;
}

bool AManyNamesPrototypeGameMode::ResolveRegionFromLevelName(const FString& LevelName, EManyNamesRegionId& OutRegionId) const
{
	if (LevelName.Contains(TEXT("Opening")))
	{
		OutRegionId = EManyNamesRegionId::Opening;
		return true;
	}
	if (LevelName.Contains(TEXT("Egypt")))
	{
		OutRegionId = EManyNamesRegionId::Egypt;
		return true;
	}
	if (LevelName.Contains(TEXT("Greece")))
	{
		OutRegionId = EManyNamesRegionId::Greece;
		return true;
	}
	if (LevelName.Contains(TEXT("Italic")))
	{
		OutRegionId = EManyNamesRegionId::ItalicWest;
		return true;
	}
	if (LevelName.Contains(TEXT("Convergence")))
	{
		OutRegionId = EManyNamesRegionId::Convergence;
		return true;
	}

	return false;
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

AManyNamesEnvironmentController* AManyNamesPrototypeGameMode::GetEnvironmentController() const
{
	return GetWorld() ? Cast<AManyNamesEnvironmentController>(UGameplayStatics::GetActorOfClass(GetWorld(), AManyNamesEnvironmentController::StaticClass())) : nullptr;
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
