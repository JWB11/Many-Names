#include "Gameplay/ManyNamesPrototypeGameMode.h"

#include "AudioDevice.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/ManyNamesDialogueController.h"
#include "Gameplay/ManyNamesEnvironmentController.h"
#include "Gameplay/ManyNamesFirstPersonCharacter.h"
#include "GameplayTagsManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Misc/PackageName.h"
#include "MovieScene.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesMythSubsystem.h"
#include "Systems/ManyNamesQuestSubsystem.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/ManyNamesHUD.h"

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

	const TCHAR* CompanionLabel(EManyNamesCompanionId CompanionId)
	{
		switch (CompanionId)
		{
		case EManyNamesCompanionId::SkyRuler:
			return TEXT("SkyRuler");
		case EManyNamesCompanionId::BronzeLawgiver:
			return TEXT("BronzeLawgiver");
		case EManyNamesCompanionId::OracleAI:
		default:
			return TEXT("OracleAI");
		}
	}

	FString PowerLabel(EManyNamesPowerId PowerId)
	{
		switch (PowerId)
		{
		case EManyNamesPowerId::TraumaRecovery:
			return TEXT("Trauma Recovery");
		case EManyNamesPowerId::ThermalShielding:
			return TEXT("Thermal Shielding");
		case EManyNamesPowerId::FocusShift:
			return TEXT("Focus Shift");
		case EManyNamesPowerId::InsightPulse:
			return TEXT("Insight Pulse");
		case EManyNamesPowerId::ArtifactRecognition:
			return TEXT("Artifact Recognition");
		default:
			return TEXT("Unknown");
		}
	}
}

AManyNamesPrototypeGameMode::AManyNamesPrototypeGameMode()
{
	DefaultPawnClass = AManyNamesFirstPersonCharacter::StaticClass();
	HUDClass = AManyNamesHUD::StaticClass();
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
		else
		{
			if (!DialogueController->IsDialogueOpen())
			{
				StopDialogueSceneAudio();
				OnDialogueStateChanged.Broadcast(NAME_None, false);
			}
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
	const UManyNamesMythSubsystem* MythSubsystem = GetMythSubsystem();
	if (!QuestSubsystem || !WorldStateSubsystem || !ContentSubsystem)
	{
		return TEXT("Journal unavailable.");
	}

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	TStringBuilder<4096> Builder;
	Builder.Append(TEXT("Many Names Journal\n"));
	const EManyNamesRegionId CurrentRegionId = GetCurrentRegionId();
	FManyNamesRegionBriefRecord RegionBrief;
	if (ContentSubsystem->GetRegionBrief(CurrentRegionId, RegionBrief))
	{
		Builder.Appendf(TEXT("%s\n"), *UEnum::GetDisplayValueAsText(CurrentRegionId).ToString());
		Builder.Appendf(TEXT("Dominant court: %s\n"), *RegionBrief.CourtDisplayName.ToString());
		if (!RegionBrief.HubSummary.IsEmpty())
		{
			Builder.Appendf(TEXT("%s\n"), *RegionBrief.HubSummary.ToString());
		}
		if (!RegionBrief.SurfaceBeliefText.IsEmpty())
		{
			Builder.Appendf(TEXT("Public belief: %s\n"), *RegionBrief.SurfaceBeliefText.ToString());
		}
		if (!RegionBrief.HiddenTruthText.IsEmpty())
		{
			Builder.Appendf(TEXT("Buried truth: %s\n"), *RegionBrief.HiddenTruthText.ToString());
		}
		if (!RegionBrief.TravelAdvisoryText.IsEmpty())
		{
			Builder.Appendf(TEXT("Traveler's note: %s\n"), *RegionBrief.TravelAdvisoryText.ToString());
		}

		const TArray<FManyNamesCourtFactionRecord> Factions = ContentSubsystem->GetCourtFactionsForRegion(CurrentRegionId);
		if (Factions.Num() > 0)
		{
			Builder.Append(TEXT("Factions:\n"));
			for (const FManyNamesCourtFactionRecord& Faction : Factions)
			{
				Builder.Appendf(TEXT("  - %s: %s\n"), *Faction.FactionName.ToString(), *Faction.JournalLabel.ToString());
			}
		}
		Builder.Append(TEXT("\n"));
	}

	Builder.Appendf(TEXT("Visited regions: %d | Known outputs: %d\n"), WorldState.RegionVisitOrder.Num(), WorldState.WorldStateOutputs.Num());
	Builder.Appendf(TEXT("Public miracle: %d | Concealment: %d | Combat: %d\n"),
		WorldState.RumorProfile.PublicMiracleScore,
		WorldState.RumorProfile.ConcealmentScore,
		WorldState.RumorProfile.CombatReputation);
	EManyNamesCompanionId DominantCompanionId = EManyNamesCompanionId::OracleAI;
	if (WorldStateSubsystem->TryGetDominantAntagonist(DominantCompanionId))
	{
		Builder.Appendf(TEXT("Dominant antagonist path: %s\n"), CompanionLabel(DominantCompanionId));
	}

	if (WorldState.UnlockedPowers.Num() > 0)
	{
		Builder.Append(TEXT("Unlocked powers: "));
		bool bFirstPower = true;
		for (EManyNamesPowerId PowerId : WorldState.UnlockedPowers)
		{
			if (!bFirstPower)
			{
				Builder.Append(TEXT(", "));
			}
			Builder.Append(PowerLabel(PowerId));
			bFirstPower = false;
		}
		Builder.Append(TEXT("\n"));
	}

	if (MythSubsystem)
	{
		TArray<FGameplayTag> DomainTags;
		WorldState.MythicDomainProfile.DomainScores.GetKeys(DomainTags);
		DomainTags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});
		if (DomainTags.Num() > 0)
		{
			Builder.Append(TEXT("Domain resonance:\n"));
			for (const FGameplayTag& DomainTag : DomainTags)
			{
				if (const int32* Score = WorldState.MythicDomainProfile.DomainScores.Find(DomainTag))
				{
					Builder.Appendf(TEXT("  - %s: %d\n"), *DomainTag.GetTagName().ToString().Replace(TEXT("Domain."), TEXT("")), *Score);
				}
			}
		}
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

void AManyNamesPrototypeGameMode::BroadcastJournalUpdated()
{
	OnJournalUpdated.Broadcast(GetJournalSummaryText());
}

void AManyNamesPrototypeGameMode::OpenDialogueInternal(FName QuestId)
{
	if (!DialogueController)
	{
		return;
	}

	StopDialogueSceneAudio();
	DialogueController->OpenDialogue(QuestId);
	OnDialogueStateChanged.Broadcast(QuestId, DialogueController->IsDialogueOpen());
	BroadcastJournalUpdated();

	if (!DialogueController->IsDialogueOpen())
	{
		ShowStatusMessage(TEXT("No valid dialogue options are currently unlocked for that encounter."), FColor::Red);
	}
	else
	{
	TryPlayDialogueScenePresentation(QuestId);
	}
}

bool AManyNamesPrototypeGameMode::TryPlaySceneVoice(FName SceneId, TArray<TObjectPtr<UAudioComponent>>& TargetComponents)
{
	if (SceneId.IsNone())
	{
		return false;
	}

	TArray<FString> CandidatePaths;
	CandidatePaths.Add(FString::Printf(TEXT("/Game/Audio/Generated/voices/%s.%s"), *SceneId.ToString(), *SceneId.ToString()));
	CandidatePaths.Add(FString::Printf(TEXT("/Game/Audio/Generated/Voices/%s.%s"), *SceneId.ToString(), *SceneId.ToString()));

	for (const FString& AssetPath : CandidatePaths)
	{
		if (USoundBase* VoiceSound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *AssetPath)))
		{
			if (UAudioComponent* Component = UGameplayStatics::SpawnSound2D(this, VoiceSound))
			{
				TargetComponents.Add(Component);
				return true;
			}
		}
	}

	return false;
}

void AManyNamesPrototypeGameMode::PlayAudioProfiles(const TArray<FName>& AudioProfileIds, TArray<TObjectPtr<UAudioComponent>>& TargetComponents)
{
	UManyNamesContentSubsystem* ContentSubsystem = GetContentSubsystem();
	if (!ContentSubsystem)
	{
		return;
	}

	for (const FName& AudioId : AudioProfileIds)
	{
		FManyNamesAudioProfileRecord AudioRecord;
		if (ContentSubsystem->GetAudioProfile(AudioId, AudioRecord))
		{
			if (USoundBase* Sound = Cast<USoundBase>(AudioRecord.SoundAsset.TryLoad()))
			{
				if (UAudioComponent* Component = UGameplayStatics::SpawnSound2D(this, Sound, AudioRecord.VolumeMultiplier))
				{
					TargetComponents.Add(Component);
				}
			}
		}
	}
}

void AManyNamesPrototypeGameMode::StopDialogueSceneAudio()
{
	for (UAudioComponent* AudioComponent : ActiveDialogueAudioComponents)
	{
		if (AudioComponent)
		{
			AudioComponent->Stop();
		}
	}
	ActiveDialogueAudioComponents.Reset();
}

void AManyNamesPrototypeGameMode::TryPlayDialogueScenePresentation(FName QuestId)
{
	if (!DialogueController)
	{
		return;
	}

	const FManyNamesDialogueSceneRecord& SceneRecord = DialogueController->GetCurrentScene();
	if (SceneRecord.SceneId.IsNone() || SceneRecord.QuestId != QuestId)
	{
		return;
	}

	PlayAudioProfiles(SceneRecord.AudioProfileIds, ActiveDialogueAudioComponents);
	TryPlaySceneVoice(SceneRecord.SceneId, ActiveDialogueAudioComponents);
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
	const UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;
	const bool bUseProceduralFallback = !Sequence || !MovieScene || MovieScene->GetBindings().Num() == 0;
	if (bUseProceduralFallback)
	{
		return TryPlayProceduralCinematic(SceneRecord, DialogueQuestId);
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

	PlayAudioProfiles(SceneRecord.AudioProfileIds, ActiveSceneAudioComponents);
	TryPlaySceneVoice(SceneRecord.SceneId, ActiveSceneAudioComponents);
	OnCinematicStateChanged.Broadcast(SceneId, true);
	ActiveSequencePlayer->OnFinished.AddUniqueDynamic(this, &AManyNamesPrototypeGameMode::HandleActiveCinematicFinished);
	ActiveSequencePlayer->Play();
	return true;
}

bool AManyNamesPrototypeGameMode::TryPlayProceduralCinematic(const FManyNamesCinematicSceneRecord& SceneRecord, FName DialogueQuestId)
{
	if (!GetWorld())
	{
		return false;
	}

	if (!SceneRecord.WeatherStateId.IsNone())
	{
		ApplyWeatherState(SceneRecord.WeatherStateId);
	}

	PlayAudioProfiles(SceneRecord.AudioProfileIds, ActiveSceneAudioComponents);
	TryPlaySceneVoice(SceneRecord.SceneId, ActiveSceneAudioComponents);

	AActor* FocusActor = FindSceneFocusActor(SceneRecord);
	const FTransform CameraTransform = BuildProceduralCameraTransform(FocusActor, SceneRecord.BlockingPresetId);
	ActiveFallbackCineCamera = GetWorld()->SpawnActor<ACameraActor>(CameraTransform.GetLocation(), CameraTransform.Rotator());
	if (!ActiveFallbackCineCamera)
	{
		RestoreBaselineWeather();
		for (UAudioComponent* AudioComponent : ActiveSceneAudioComponents)
		{
			if (AudioComponent)
			{
				AudioComponent->Stop();
			}
		}
		ActiveSceneAudioComponents.Reset();
		return false;
	}

	if (UCameraComponent* CameraComponent = ActiveFallbackCineCamera->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(SceneRecord.RegionId == EManyNamesRegionId::Convergence ? 40.0f : 48.0f);
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		SavedViewTarget = PlayerController->GetViewTarget();
		PlayerController->SetViewTargetWithBlend(ActiveFallbackCineCamera, 0.45f);
	}

	PendingDialogueQuestId = DialogueQuestId;
	ActiveCinematicSceneId = SceneRecord.SceneId;
	OnCinematicStateChanged.Broadcast(SceneRecord.SceneId, true);
	GetWorldTimerManager().SetTimer(
		ActiveCinematicTimerHandle,
		this,
		&AManyNamesPrototypeGameMode::HandleActiveCinematicFinished,
		FMath::Max(1.0f, SceneRecord.EstimatedDurationSeconds),
		false);
	return true;
}

AActor* AManyNamesPrototypeGameMode::FindSceneFocusActor(const FManyNamesCinematicSceneRecord& SceneRecord) const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TArray<FName> CandidateTags;
	if (!SceneRecord.FallbackFocusTag.IsNone())
	{
		CandidateTags.Add(SceneRecord.FallbackFocusTag);
	}
	if (!SceneRecord.CameraAnchorTag.IsNone())
	{
		CandidateTags.Add(SceneRecord.CameraAnchorTag);
	}
	if (!SceneRecord.CharacterId.IsNone())
	{
		CandidateTags.Add(FName(*FString::Printf(TEXT("Character.%s"), *SceneRecord.CharacterId.ToString())));
	}

	for (const FName& CandidateTag : CandidateTags)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(CandidateTag))
			{
				return *It;
			}
		}
	}

	return UGameplayStatics::GetPlayerPawn(this, 0);
}

FTransform AManyNamesPrototypeGameMode::BuildProceduralCameraTransform(const AActor* FocusActor, const FName& BlockingPresetId) const
{
	const FVector FocusLocation = FocusActor ? FocusActor->GetActorLocation() : FVector::ZeroVector;
	const FVector Offset =
		BlockingPresetId == TEXT("close_authority") ? FVector(-185.0f, 110.0f, 82.0f) :
		BlockingPresetId == TEXT("wide_arrival") ? FVector(-720.0f, 260.0f, 220.0f) :
		BlockingPresetId == TEXT("ritual_side") ? FVector(-320.0f, -210.0f, 120.0f) :
		BlockingPresetId == TEXT("convergence_reveal") ? FVector(-560.0f, 120.0f, 180.0f) :
		FVector(-420.0f, 170.0f, 128.0f);
	const FVector CameraLocation = FocusLocation + Offset;
	const FRotator CameraRotation = (FocusLocation - CameraLocation).Rotation();
	return FTransform(CameraRotation, CameraLocation, FVector::OneVector);
}

void AManyNamesPrototypeGameMode::HandleActiveCinematicFinished()
{
	FinishActiveCinematic(false);
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
	GetWorldTimerManager().ClearTimer(ActiveCinematicTimerHandle);

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
	if (ActiveFallbackCineCamera)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			PlayerController->SetViewTargetWithBlend(SavedViewTarget ? SavedViewTarget.Get() : UGameplayStatics::GetPlayerPawn(this, 0), 0.25f);
		}
		ActiveFallbackCineCamera->Destroy();
		ActiveFallbackCineCamera = nullptr;
		SavedViewTarget = nullptr;
	}

	RestoreBaselineWeather();
	ActiveCinematicSceneId = NAME_None;
	OnCinematicStateChanged.Broadcast(FinishedSceneId, false);

	const FName DialogueQuestId = PendingDialogueQuestId;
	PendingDialogueQuestId = NAME_None;
	if (!DialogueQuestId.IsNone())
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
	return !ActiveCinematicSceneId.IsNone() && (ActiveSequencePlayer != nullptr || ActiveFallbackCineCamera != nullptr);
}

void AManyNamesPrototypeGameMode::ShowStatusMessage(const FString& Message, FColor Color)
{
	OnStatusMessage.Broadcast(FText::FromString(Message), FLinearColor(Color));
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
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
