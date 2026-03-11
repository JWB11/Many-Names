#include "Systems/ManyNamesWorldStateSubsystem.h"

#include "Systems/ManyNamesGameInstance.h"
#include "GameplayTagsManager.h"

namespace
{
TArray<FName> GetCompatibleOutputAliases(FName OutputId)
{
	TArray<FName> Outputs;
	if (OutputId.IsNone())
	{
		return Outputs;
	}

	if (OutputId == TEXT("Region.Egypt.MainResolved") || OutputId == TEXT("State.Region.Egypt.Complete"))
	{
		Outputs.Add(TEXT("Region.Egypt.MainResolved"));
		Outputs.Add(TEXT("State.Region.Egypt.Complete"));
	}
	else if (OutputId == TEXT("Region.Greece.MainResolved") || OutputId == TEXT("State.Region.Greece.Complete"))
	{
		Outputs.Add(TEXT("Region.Greece.MainResolved"));
		Outputs.Add(TEXT("State.Region.Greece.Complete"));
	}
	else if (OutputId == TEXT("Region.ItalicWest.MainResolved") || OutputId == TEXT("State.Region.ItalicWest.Complete"))
	{
		Outputs.Add(TEXT("Region.ItalicWest.MainResolved"));
		Outputs.Add(TEXT("State.Region.ItalicWest.Complete"));
	}
	else
	{
		Outputs.Add(OutputId);
	}

	return Outputs;
}

int32 GetDomainScore(const FManyNamesWorldState& WorldState, const TCHAR* TagName)
{
	const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(TagName), false);
	if (!Tag.IsValid())
	{
		return 0;
	}

	if (const int32* Score = WorldState.MythicDomainProfile.DomainScores.Find(Tag))
	{
		return *Score;
	}

	return 0;
}

int32 ComputeCompanionThreatScore(const FManyNamesWorldState& WorldState, EManyNamesCompanionId CompanionId, const FManyNamesCompanionState& CompanionState)
{
	int32 Score = FMath::Max(0, CompanionState.EscalationScore);
	switch (CompanionState.AllianceState)
	{
	case EManyNamesAllianceState::Allied:
		Score += 2;
		break;
	case EManyNamesAllianceState::Opposed:
		Score += 8;
		break;
	case EManyNamesAllianceState::Replaced:
		Score += 5;
		break;
	default:
		break;
	}

	if (CompanionState.Affinity >= 0)
	{
		Score += CompanionState.Affinity;
	}
	else
	{
		Score += FMath::Abs(CompanionState.Affinity) * 2;
	}

	if (CompanionState.bTruthRevealed)
	{
		Score += 2;
	}

	switch (CompanionId)
	{
	case EManyNamesCompanionId::OracleAI:
		Score += GetDomainScore(WorldState, TEXT("Domain.Deception"));
		Score += GetDomainScore(WorldState, TEXT("Domain.Judgment"));
		Score += WorldState.RumorProfile.ConcealmentScore;
		break;
	case EManyNamesCompanionId::SkyRuler:
		Score += GetDomainScore(WorldState, TEXT("Domain.Storm"));
		Score += GetDomainScore(WorldState, TEXT("Domain.Light"));
		Score += FMath::Max(0, WorldState.RumorProfile.PublicMiracleScore);
		Score += FMath::Max(0, WorldState.RumorProfile.CombatReputation);
		break;
	case EManyNamesCompanionId::BronzeLawgiver:
		Score += GetDomainScore(WorldState, TEXT("Domain.Order"));
		Score += GetDomainScore(WorldState, TEXT("Domain.Craft"));
		Score += GetDomainScore(WorldState, TEXT("Domain.Judgment"));
		Score += FMath::Max(0, WorldState.RumorProfile.CombatReputation / 2);
		break;
	default:
		break;
	}

	return Score;
}

EManyNamesCompanionThreatState ComputeThreatState(int32 Score)
{
	if (Score >= 18)
	{
		return EManyNamesCompanionThreatState::Dominant;
	}
	if (Score >= 12)
	{
		return EManyNamesCompanionThreatState::Ascendant;
	}
	if (Score >= 6)
	{
		return EManyNamesCompanionThreatState::Tempting;
	}
	return EManyNamesCompanionThreatState::Dormant;
}

FName DominantOutputForCompanion(EManyNamesCompanionId CompanionId)
{
	switch (CompanionId)
	{
	case EManyNamesCompanionId::OracleAI:
		return TEXT("Story.Antagonist.OracleAI");
	case EManyNamesCompanionId::SkyRuler:
		return TEXT("Story.Antagonist.SkyRuler");
	case EManyNamesCompanionId::BronzeLawgiver:
		return TEXT("Story.Antagonist.BronzeLawgiver");
	default:
		return NAME_None;
	}
}

FName ThreatOutputForCompanion(EManyNamesCompanionId CompanionId, bool bDominant)
{
	const TCHAR* StateSuffix = bDominant ? TEXT("Dominant") : TEXT("Ascendant");
	switch (CompanionId)
	{
	case EManyNamesCompanionId::OracleAI:
		return FName(FString::Printf(TEXT("Companion.OracleAI.%s"), StateSuffix));
	case EManyNamesCompanionId::SkyRuler:
		return FName(FString::Printf(TEXT("Companion.SkyRuler.%s"), StateSuffix));
	case EManyNamesCompanionId::BronzeLawgiver:
		return FName(FString::Printf(TEXT("Companion.BronzeLawgiver.%s"), StateSuffix));
	default:
		return NAME_None;
	}
}
}

FName UManyNamesWorldStateSubsystem::GetCanonicalRegionCompletionOutput(EManyNamesRegionId RegionId)
{
	switch (RegionId)
	{
	case EManyNamesRegionId::Opening:
		return TEXT("State.Region.Opening.Complete");
	case EManyNamesRegionId::Egypt:
		return TEXT("State.Region.Egypt.Complete");
	case EManyNamesRegionId::Greece:
		return TEXT("State.Region.Greece.Complete");
	case EManyNamesRegionId::ItalicWest:
		return TEXT("State.Region.ItalicWest.Complete");
	case EManyNamesRegionId::Convergence:
		return TEXT("State.Region.Convergence.Complete");
	default:
		return NAME_None;
	}
}

bool UManyNamesWorldStateSubsystem::TryGetDominantAntagonist(EManyNamesCompanionId& OutCompanionId) const
{
	if (const UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance())
	{
		const FManyNamesWorldState& WorldState = GameInstance->GetWorldState();
		if (WorldState.bHasDominantAntagonist)
		{
			OutCompanionId = WorldState.DominantAntagonist;
			return true;
		}
	}

	return false;
}

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
		const TSet<FName>& WorldOutputs = GameInstance->GetWorldState().WorldStateOutputs;
		for (const FName& Alias : GetCompatibleOutputAliases(OutputId))
		{
			if (WorldOutputs.Contains(Alias))
			{
				return true;
			}
		}
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
	for (const FName& Alias : GetCompatibleOutputAliases(OutputId))
	{
		WorldState.WorldStateOutputs.Add(Alias);
	}
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::RemoveWorldStateOutput(FName OutputId)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance || OutputId.IsNone())
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	for (const FName& Alias : GetCompatibleOutputAliases(OutputId))
	{
		WorldState.WorldStateOutputs.Remove(Alias);
	}
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

void UManyNamesWorldStateSubsystem::SetCompanionTruthRevealed(EManyNamesCompanionId CompanionId, bool bTruthRevealed)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	FManyNamesCompanionState& CompanionState = WorldState.Companions.FindOrAdd(CompanionId);
	CompanionState.CompanionId = CompanionId;
	CompanionState.bTruthRevealed = bTruthRevealed;
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::AddCompanionEscalation(EManyNamesCompanionId CompanionId, int32 Delta)
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	FManyNamesCompanionState& CompanionState = WorldState.Companions.FindOrAdd(CompanionId);
	CompanionState.CompanionId = CompanionId;
	CompanionState.EscalationScore = FMath::Max(0, CompanionState.EscalationScore + Delta);
	GameInstance->SetWorldState(WorldState);
}

void UManyNamesWorldStateSubsystem::RefreshDominantAntagonist()
{
	UManyNamesGameInstance* GameInstance = GetManyNamesGameInstance();
	if (!GameInstance)
	{
		return;
	}

	FManyNamesWorldState WorldState = GameInstance->GetWorldState();
	for (const EManyNamesCompanionId CompanionId : {EManyNamesCompanionId::OracleAI, EManyNamesCompanionId::SkyRuler, EManyNamesCompanionId::BronzeLawgiver})
	{
		WorldState.WorldStateOutputs.Remove(ThreatOutputForCompanion(CompanionId, false));
		WorldState.WorldStateOutputs.Remove(ThreatOutputForCompanion(CompanionId, true));
		WorldState.WorldStateOutputs.Remove(DominantOutputForCompanion(CompanionId));
	}

	int32 BestScore = 0;
	EManyNamesCompanionId BestCompanion = EManyNamesCompanionId::OracleAI;
	for (TPair<EManyNamesCompanionId, FManyNamesCompanionState>& Pair : WorldState.Companions)
	{
		const int32 Score = ComputeCompanionThreatScore(WorldState, Pair.Key, Pair.Value);
		Pair.Value.EscalationScore = FMath::Max(Pair.Value.EscalationScore, Score);
		Pair.Value.ThreatState = ComputeThreatState(Score);
		Pair.Value.bDominantAntagonist = false;
		if (Pair.Value.ThreatState == EManyNamesCompanionThreatState::Ascendant || Pair.Value.ThreatState == EManyNamesCompanionThreatState::Dominant)
		{
			WorldState.WorldStateOutputs.Add(ThreatOutputForCompanion(Pair.Key, Pair.Value.ThreatState == EManyNamesCompanionThreatState::Dominant));
		}
		if (Score > BestScore)
		{
			BestScore = Score;
			BestCompanion = Pair.Key;
		}
	}

	WorldState.bHasDominantAntagonist = BestScore >= 12;
	WorldState.DominantAntagonist = BestCompanion;
	if (WorldState.bHasDominantAntagonist)
	{
		if (FManyNamesCompanionState* CompanionState = WorldState.Companions.Find(BestCompanion))
		{
			CompanionState->bDominantAntagonist = true;
			CompanionState->ThreatState = EManyNamesCompanionThreatState::Dominant;
		}
		WorldState.WorldStateOutputs.Add(ThreatOutputForCompanion(BestCompanion, true));
		WorldState.WorldStateOutputs.Add(DominantOutputForCompanion(BestCompanion));
	}

	GameInstance->SetWorldState(WorldState);
}

UManyNamesGameInstance* UManyNamesWorldStateSubsystem::GetManyNamesGameInstance() const
{
	return Cast<UManyNamesGameInstance>(this->GetGameInstance());
}
