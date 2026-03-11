#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/HUD.h"
#include "ManyNamesHUD.generated.h"

USTRUCT()
struct FManyNamesHudMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FText Message;

	UPROPERTY()
	FLinearColor Color = FLinearColor::White;

	UPROPERTY()
	float ExpireAt = 0.0f;
};

UCLASS()
class MANYNAMES_API AManyNamesHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	void SetInteractionPrompt(const FText& InPrompt);
	void SetMenuPrompt(const FText& InPrompt);
	void PushStatusMessage(const FText& Message, const FLinearColor& Color);
	bool ToggleJournal();
	void SetJournalVisible(bool bVisible);

	UFUNCTION()
	void HandleStatusMessage(const FText& Message, FLinearColor Color);

	UFUNCTION()
	void HandleJournalUpdated(const FText& Summary);

	UFUNCTION()
	void HandleCinematicStateChanged(FName SceneId, bool bIsPlaying);

	UFUNCTION()
	void HandleWorldStateChanged(const FManyNamesWorldState& WorldState);

private:
	void RefreshCachedJournal();
	void DrawTextBlock(const FText& Text, const FVector2D& Position, const FLinearColor& Color, float Scale = 1.0f) const;
	void DrawSection(const FString& Header, const FText& Body, float& InOutY, float Left) const;

	FText InteractionPrompt;
	FText MenuPrompt;
	bool bJournalVisible = false;
	bool bCinematicPlaying = false;

	UPROPERTY()
	TArray<FManyNamesHudMessage> StatusMessages;

	FText JournalSummary;
	FText RegionSummary;
	FText QuestSummary;
	FText DomainSummary;
	FText RumorSummary;
	FText EndingSummary;
};
