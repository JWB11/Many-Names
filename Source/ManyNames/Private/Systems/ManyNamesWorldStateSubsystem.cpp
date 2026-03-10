#include "Systems/ManyNamesWorldStateSubsystem.h"

#include "Systems/ManyNamesGameInstance.h"

FManyNamesWorldState UManyNamesWorldStateSubsystem::GetWorldState() const
{
	if (const UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance())
	{
		return GameInstance->GetWorldState();
	}

	return FManyNamesWorldState();
}

bool UManyNamesWorldStateSubsystem::HasWorldStateOutput(FName OutputId) const
{
	if (const UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance())
	{
		return GameInstance->GetWorldState().WorldStateOutputs.Contains(OutputId);
	}

	return false;
}

void UManyNamesWorldStateSubsystem::AddWorldStateOutput(FName OutputId)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance || OutputId.IsNone())
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	WorldState.WorldStateOutputs.Add(OutputId);
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::RegisterRegionVisit(EManyNamesRegionId RegionId)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	if (!WorldState.RegionVisitOrder.Contains(RegionId))
	{
		WorldState.RegionVisitOrder.Add(RegionId);
	}

	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::SetRegionUnlocked(EManyNamesRegionId RegionId, bool bUnlocked)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	if (FManyNamesRegionState* RegionState = WorldState.Regions.Find(RegionId))
	{
		RegionState->bUnlocked = bUnlocked;
	}

	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::SetRegionCompleted(EManyNamesRegionId RegionId, bool bCompleted)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	if (FManyNamesRegionState* RegionState = WorldState.Regions.Find(RegionId))
	{
		RegionState->bCompleted = bCompleted;
	}

	if (RegionId == EManyNamesRegionId::Egypt || RegionId == EManyNamesRegionId::Greece || RegionId == EManyNamesRegionId::ItalicWest)
	{
		const bool bAllPrimaryRegionsComplete =
			WorldState.Regions.FindRef(EManyNamesRegionId::Egypt).bCompleted &&
			WorldState.Regions.FindRef(EManyNamesRegionId::Greece).bCompleted &&
			WorldState.Regions.FindRef(EManyNamesRegionId::ItalicWest).bCompleted;
		if (FManyNamesRegionState* ConvergenceState = WorldState.Regions.Find(EManyNamesRegionId::Convergence))
		{
			ConvergenceState->bUnlocked = bAllPrimaryRegionsComplete;
		}
	}

	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::ApplyChoice(const FName ChoiceId, const FName SelectedOptionId)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	FManyNamesChoiceRecord Record;
	Record.ChoiceId = ChoiceId;
	Record.SelectedOptionId = SelectedOptionId;
	WorldState.MajorChoices.Add(Record);
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::SetEligibleEnding(EManyNamesEndingId EndingId, bool bEligible)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	if (bEligible)
	{
		WorldState.EligibleEndings.Add(EndingId);
	}
	else
	{
		WorldState.EligibleEndings.Remove(EndingId);
	}

	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::UpdateCombatReputation(int32 Delta)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	WorldState.RumorProfile.CombatReputation += Delta;
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::ApplyRumorEffect(const FManyNamesRumorEffectRecord& RumorEffect)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	WorldState.RumorProfile.PublicMiracleScore += RumorEffect.PublicMiracle;
	WorldState.RumorProfile.ConcealmentScore += RumorEffect.Concealment;
	WorldState.RumorProfile.CombatReputation += RumorEffect.CombatReputation;
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::UnlockPower(EManyNamesPowerId PowerId)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	WorldState.UnlockedPowers.Add(PowerId);
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::UpdateCompanionAffinity(EManyNamesCompanionId CompanionId, int32 Delta)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	FManyNamesCompanionState& CompanionState = WorldState.Companions.FindOrAdd(CompanionId);
	CompanionState.CompanionId = CompanionId;
	CompanionState.Affinity += Delta;
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::SetCompanionAllianceState(EManyNamesCompanionId CompanionId, EManyNamesAllianceState AllianceState)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	FManyNamesCompanionState& CompanionState = WorldState.Companions.FindOrAdd(CompanionId);
	CompanionState.CompanionId = CompanionId;
	CompanionState.AllianceState = AllianceState;
	GameInstance->SetWorldState(WorldState);
}

UManyNamesGameInstance* UManyNamesWorldStateSubsystem::GetManyNamesGameInstance() const
{
	return Cast<UManyNamesGameInstance>(this->GetGameInstance());
}
