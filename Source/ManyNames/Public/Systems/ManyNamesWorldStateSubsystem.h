#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesWorldStateSubsystem.generated.h"

UCLASS()
class MANYNAMES_API UManyNamesWorldStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	FManyNamesWorldState GetWorldState() const;

	UFUNCTION(BlueprintPure, Category="ManyNames|WorldState")
	bool HasWorldStateOutput(FName OutputId) const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void AddWorldStateOutput(FName OutputId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void RemoveWorldStateOutput(FName OutputId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void RegisterRegionVisit(EManyNamesRegionId RegionId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void SetRegionUnlocked(EManyNamesRegionId RegionId, bool bUnlocked);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void SetRegionCompleted(EManyNamesRegionId RegionId, bool bCompleted);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void ApplyChoice(const FName ChoiceId, const FName SelectedOptionId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void SetEligibleEnding(EManyNamesEndingId EndingId, bool bEligible);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void UpdateCombatReputation(int32 Delta);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void ApplyRumorEffect(const FManyNamesRumorEffectRecord& RumorEffect);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void UnlockPower(EManyNamesPowerId PowerId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void UpdateCompanionAffinity(EManyNamesCompanionId CompanionId, int32 Delta);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void SetCompanionAllianceState(EManyNamesCompanionId CompanionId, EManyNamesAllianceState AllianceState);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void SetCompanionTruthRevealed(EManyNamesCompanionId CompanionId, bool bTruthRevealed);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void AddCompanionEscalation(EManyNamesCompanionId CompanionId, int32 Delta);

	UFUNCTION(BlueprintCallable, Category="ManyNames|WorldState")
	void RefreshDominantAntagonist();

private:
	class UManyNamesGameInstance* GetManyNamesGameInstance() const;
};
