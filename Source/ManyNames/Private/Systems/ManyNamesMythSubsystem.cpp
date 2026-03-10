#include "Systems/ManyNamesMythSubsystem.h"

#include "Systems/ManyNamesGameInstance.h"
#include "Core/ManyNamesTypes.h"

int32 UManyNamesMythSubsystem::GetDomainScore(FGameplayTag DomainTag) const
{
	if (const UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance())
	{
		const FManyNamesWorldState& WorldState = GameInstance->GetWorldState();
		if (const int32* Score = WorldState.MythicDomainProfile.DomainScores.Find(DomainTag))
		{
			return *Score;
		}
	}

	return 0;
}

void UManyNamesMythSubsystem::AddDomainResonance(FGameplayTag DomainTag, int32 Delta, bool bPubliclyVisible)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	int32& Score = WorldState.MythicDomainProfile.DomainScores.FindOrAdd(DomainTag);
	Score += Delta;
	if (bPubliclyVisible)
	{
		WorldState.MythicDomainProfile.PublicDomains.AddTag(DomainTag);
		WorldState.RumorProfile.PublicMiracleScore += Delta;
	}
	else
	{
		WorldState.RumorProfile.ConcealmentScore += Delta;
	}

	GameInstance->SetWorldState(WorldState);
}

void UManyNamesMythSubsystem::AddRegionalInterpretation(EManyNamesRegionId RegionId, FGameplayTag InterpretationTag)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	WorldState.MythicDomainProfile.RegionalInterpretations.FindOrAdd(RegionId).AddTag(InterpretationTag);
	WorldState.RumorProfile.RegionalInterpretations.FindOrAdd(RegionId).AddTag(InterpretationTag);
	GameInstance->SetWorldState(WorldState);
}

UManyNamesGameInstance* UManyNamesMythSubsystem::GetManyNamesGameInstance() const
{
	return Cast<UManyNamesGameInstance>(this->GetGameInstance());
}
