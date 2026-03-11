#include "UI/ManyNamesHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Gameplay/ManyNamesPrototypeGameMode.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesGameInstance.h"
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

void AManyNamesHUD::BeginPlay()
{
	Super::BeginPlay();

	if (AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		GameMode->OnStatusMessage.AddDynamic(this, &AManyNamesHUD::HandleStatusMessage);
		GameMode->OnJournalUpdated.AddDynamic(this, &AManyNamesHUD::HandleJournalUpdated);
		GameMode->OnCinematicStateChanged.AddDynamic(this, &AManyNamesHUD::HandleCinematicStateChanged);
	}

	if (UManyNamesGameInstance* GameInstance = GetGameInstance<UManyNamesGameInstance>())
	{
		GameInstance->OnWorldStateChanged.AddDynamic(this, &AManyNamesHUD::HandleWorldStateChanged);
	}

	RefreshCachedJournal();
}

void AManyNamesHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	StatusMessages.RemoveAll([Now](const FManyNamesHudMessage& Entry)
	{
		return Entry.ExpireAt > 0.0f && Entry.ExpireAt <= Now;
	});

	float StatusY = 48.0f;
	for (int32 Index = FMath::Max(0, StatusMessages.Num() - 3); Index < StatusMessages.Num(); ++Index)
	{
		DrawTextBlock(StatusMessages[Index].Message, FVector2D(42.0f, StatusY), StatusMessages[Index].Color, 1.0f);
		StatusY += 28.0f;
	}

	const float BottomY = Canvas->ClipY - 110.0f;
	if (!InteractionPrompt.IsEmpty())
	{
		DrawTextBlock(InteractionPrompt, FVector2D(42.0f, BottomY), FLinearColor::White, 1.0f);
	}

	if (!MenuPrompt.IsEmpty())
	{
		DrawTextBlock(MenuPrompt, FVector2D(42.0f, BottomY + 28.0f), FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), 0.95f);
	}

	if (bCinematicPlaying)
	{
		DrawTextBlock(FText::FromString(TEXT("Cinematic playing")), FVector2D(Canvas->ClipX - 260.0f, 42.0f), FLinearColor(0.95f, 0.8f, 0.45f, 1.0f), 0.95f);
	}

	if (!bJournalVisible)
	{
		return;
	}

	const float PanelX = 36.0f;
	const float PanelY = 36.0f;
	const float PanelWidth = FMath::Min(760.0f, Canvas->ClipX - 72.0f);
	const float PanelHeight = FMath::Min(Canvas->ClipY - 72.0f, 860.0f);
	DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.88f), PanelX, PanelY, PanelWidth, PanelHeight);
	DrawRect(FLinearColor(0.55f, 0.44f, 0.18f, 0.75f), PanelX, PanelY, PanelWidth, 4.0f);

	float CursorY = PanelY + 18.0f;
	DrawSection(TEXT("Journal"), JournalSummary, CursorY, PanelX + 22.0f);
	DrawSection(TEXT("Region"), RegionSummary, CursorY, PanelX + 22.0f);
	DrawSection(TEXT("Quests"), QuestSummary, CursorY, PanelX + 22.0f);
	DrawSection(TEXT("Domains"), DomainSummary, CursorY, PanelX + 22.0f);
	DrawSection(TEXT("Rumors"), RumorSummary, CursorY, PanelX + 22.0f);
	DrawSection(TEXT("Endings"), EndingSummary, CursorY, PanelX + 22.0f);
}

void AManyNamesHUD::SetInteractionPrompt(const FText& InPrompt)
{
	InteractionPrompt = InPrompt;
}

void AManyNamesHUD::SetMenuPrompt(const FText& InPrompt)
{
	MenuPrompt = InPrompt;
}

void AManyNamesHUD::PushStatusMessage(const FText& Message, const FLinearColor& Color)
{
	FManyNamesHudMessage& Entry = StatusMessages.AddDefaulted_GetRef();
	Entry.Message = Message;
	Entry.Color = Color;
	Entry.ExpireAt = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + 6.0f;
}

bool AManyNamesHUD::ToggleJournal()
{
	SetJournalVisible(!bJournalVisible);
	return bJournalVisible;
}

void AManyNamesHUD::SetJournalVisible(bool bVisible)
{
	bJournalVisible = bVisible;
	if (bJournalVisible)
	{
		RefreshCachedJournal();
	}
}

void AManyNamesHUD::HandleStatusMessage(const FText& Message, FLinearColor Color)
{
	PushStatusMessage(Message, Color);
}

void AManyNamesHUD::HandleJournalUpdated(const FText& Summary)
{
	JournalSummary = Summary;
	if (bJournalVisible)
	{
		RefreshCachedJournal();
	}
}

void AManyNamesHUD::HandleCinematicStateChanged(FName SceneId, bool bIsPlaying)
{
	bCinematicPlaying = bIsPlaying;
	if (bIsPlaying)
	{
		PushStatusMessage(FText::FromString(FString::Printf(TEXT("Scene: %s"), *SceneId.ToString())), FLinearColor(0.85f, 0.7f, 0.35f, 1.0f));
	}
}

void AManyNamesHUD::HandleWorldStateChanged(const FManyNamesWorldState& WorldState)
{
	if (bJournalVisible)
	{
		RefreshCachedJournal();
	}

	if (WorldState.bHasDominantAntagonist)
	{
		PushStatusMessage(FText::FromString(TEXT("A dominant antagonist path is now taking shape.")), FLinearColor(0.85f, 0.5f, 0.45f, 1.0f));
	}
}

void AManyNamesHUD::RefreshCachedJournal()
{
	if (!GetWorld())
	{
		return;
	}

	if (const AManyNamesPrototypeGameMode* GameMode = Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()))
	{
		JournalSummary = GameMode->GetJournalSummaryText();
	}

	const UManyNamesContentSubsystem* ContentSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesContentSubsystem>() : nullptr;
	const UManyNamesQuestSubsystem* QuestSubsystem = GetWorld()->GetSubsystem<UManyNamesQuestSubsystem>();
	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>() : nullptr;
	if (!ContentSubsystem || !QuestSubsystem || !WorldStateSubsystem)
	{
		return;
	}

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

	TArray<FManyNamesQuestRow> QuestRows = ContentSubsystem->GetAllQuestRows();
	QuestRows.Sort([](const FManyNamesQuestRow& Left, const FManyNamesQuestRow& Right)
	{
		if (Left.RegionId != Right.RegionId)
		{
			return static_cast<uint8>(Left.RegionId) < static_cast<uint8>(Right.RegionId);
		}
		return Left.QuestId.LexicalLess(Right.QuestId);
	});

	TStringBuilder<4096> QuestBuilder;
	EManyNamesRegionId SectionRegionId = EManyNamesRegionId::Convergence;
	bool bHasRegionHeader = false;
	for (const FManyNamesQuestRow& QuestRow : QuestRows)
	{
		if (!bHasRegionHeader || SectionRegionId != QuestRow.RegionId)
		{
			SectionRegionId = QuestRow.RegionId;
			bHasRegionHeader = true;
			QuestBuilder.Appendf(TEXT("%s\n"), *UEnum::GetDisplayValueAsText(SectionRegionId).ToString());
		}

		const FString QuestType = QuestRow.QuestId.ToString().Contains(TEXT("_main_")) ? TEXT("Main") : TEXT("Side");
		QuestBuilder.Appendf(TEXT("  [%s] %s - %s\n"),
			*QuestType,
			*QuestRow.Title.ToString(),
			*QuestStateLabel(QuestSubsystem->GetQuestState(QuestRow.QuestId)));
	}
	QuestSummary = FText::FromString(QuestBuilder.ToString());

	const FManyNamesWorldState WorldState = WorldStateSubsystem->GetWorldState();
	TArray<FGameplayTag> DomainTags;
	WorldState.MythicDomainProfile.DomainScores.GetKeys(DomainTags);
	DomainTags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		return Left.ToString() < Right.ToString();
	});

	TStringBuilder<1024> DomainBuilder;
	for (const FGameplayTag& DomainTag : DomainTags)
	{
		if (const int32* Score = WorldState.MythicDomainProfile.DomainScores.Find(DomainTag))
		{
			DomainBuilder.Appendf(TEXT("%s: %d\n"), *DomainShortName(DomainTag), *Score);
		}
	}
	DomainSummary = FText::FromString(DomainBuilder.ToString());

	RumorSummary = FText::FromString(FString::Printf(
		TEXT("Public miracle: %d\nConcealment: %d\nCombat reputation: %d"),
		WorldState.RumorProfile.PublicMiracleScore,
		WorldState.RumorProfile.ConcealmentScore,
		WorldState.RumorProfile.CombatReputation));

	TStringBuilder<1024> EndingBuilder;
	EndingBuilder.Appendf(TEXT("Eligible endings: %d\n"), WorldState.EligibleEndings.Num());
	EManyNamesCompanionId DominantCompanionId = EManyNamesCompanionId::OracleAI;
	if (WorldStateSubsystem->TryGetDominantAntagonist(DominantCompanionId))
	{
		EndingBuilder.Appendf(TEXT("Dominant antagonist: %s\n"), *UEnum::GetDisplayValueAsText(DominantCompanionId).ToString());
	}
	else
	{
		EndingBuilder.Append(TEXT("Dominant antagonist: unresolved\n"));
	}
	EndingSummary = FText::FromString(EndingBuilder.ToString());
}

void AManyNamesHUD::DrawTextBlock(const FText& Text, const FVector2D& Position, const FLinearColor& Color, float Scale) const
{
	if (!Canvas || Text.IsEmpty())
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	FCanvasTextItem TextItem(Position, Text, Font, Color);
	TextItem.EnableShadow(FLinearColor::Black);
	TextItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(TextItem);
}

void AManyNamesHUD::DrawSection(const FString& Header, const FText& Body, float& InOutY, float Left) const
{
	if (Body.IsEmpty())
	{
		return;
	}

	DrawTextBlock(FText::FromString(Header), FVector2D(Left, InOutY), FLinearColor(0.92f, 0.78f, 0.42f, 1.0f), 1.0f);
	InOutY += 24.0f;

	TArray<FString> Lines;
	Body.ToString().ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		FString OutputLine = Line;
		if (OutputLine.Len() > 140)
		{
			OutputLine = OutputLine.Left(137) + TEXT("...");
		}
		DrawTextBlock(FText::FromString(OutputLine), FVector2D(Left, InOutY), FLinearColor::White, 0.88f);
		InOutY += 20.0f;
		if (InOutY > Canvas->ClipY - 54.0f)
		{
			break;
		}
	}

	InOutY += 12.0f;
}
