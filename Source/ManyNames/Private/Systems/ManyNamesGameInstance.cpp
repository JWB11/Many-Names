#include "Systems/ManyNamesGameInstance.h"

#include "Core/ManyNamesDeveloperSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Save/ManyNamesSaveGame.h"

void UManyNamesGameInstance::Init()
{
	Super::Init();
	InitializeDefaultWorldState();
	LoadWorldState();
}

bool UManyNamesGameInstance::SaveWorldState()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	UManyNamesSaveGame* SaveGame = Cast<UManyNamesSaveGame>(UGameplayStatics::CreateSaveGameObject(UManyNamesSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	SaveGame->WorldState = WorldState;
	return UGameplayStatics::SaveGameToSlot(SaveGame, Settings->DefaultSaveSlot, 0);
}

bool UManyNamesGameInstance::LoadWorldState()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!UGameplayStatics::DoesSaveGameExist(Settings->DefaultSaveSlot, 0))
	{
		OnWorldStateChanged.Broadcast(WorldState);
		return false;
	}

	UManyNamesSaveGame* SaveGame = Cast<UManyNamesSaveGame>(UGameplayStatics::LoadGameFromSlot(Settings->DefaultSaveSlot, 0));
	if (!SaveGame)
	{
		return false;
	}

	WorldState = SaveGame->WorldState;
	OnWorldStateChanged.Broadcast(WorldState);
	return true;
}

const FManyNamesWorldState& UManyNamesGameInstance::GetWorldState() const
{
	return WorldState;
}

void UManyNamesGameInstance::SetWorldState(const FManyNamesWorldState& InWorldState)
{
	WorldState = InWorldState;
	OnWorldStateChanged.Broadcast(WorldState);
	SaveWorldState();
}

void UManyNamesGameInstance::InitializeDefaultWorldState()
{
	WorldState = FManyNamesWorldState();

	FManyNamesRegionState OpeningState;
	OpeningState.RegionId = EManyNamesRegionId::Opening;
	OpeningState.bUnlocked = true;
	OpeningState.ActiveDeityId = TEXT("the_falling_star");
	WorldState.Regions.Add(EManyNamesRegionId::Opening, OpeningState);

	FManyNamesRegionState EgyptState;
	EgyptState.RegionId = EManyNamesRegionId::Egypt;
	EgyptState.bUnlocked = false;
	EgyptState.ActiveDeityId = TEXT("radiant_voice");
	EgyptState.AvailableQuestIds = {TEXT("egypt_main_01"), TEXT("egypt_side_01")};
	WorldState.Regions.Add(EManyNamesRegionId::Egypt, EgyptState);

	FManyNamesRegionState GreeceState;
	GreeceState.RegionId = EManyNamesRegionId::Greece;
	GreeceState.bUnlocked = false;
	GreeceState.ActiveDeityId = TEXT("high_one_of_storm");
	GreeceState.AvailableQuestIds = {TEXT("greece_main_01"), TEXT("greece_side_01")};
	WorldState.Regions.Add(EManyNamesRegionId::Greece, GreeceState);

	FManyNamesRegionState ItalicState;
	ItalicState.RegionId = EManyNamesRegionId::ItalicWest;
	ItalicState.bUnlocked = false;
	ItalicState.ActiveDeityId = TEXT("keeper_of_measures");
	ItalicState.AvailableQuestIds = {TEXT("italic_main_01"), TEXT("italic_side_01")};
	WorldState.Regions.Add(EManyNamesRegionId::ItalicWest, ItalicState);

	FManyNamesRegionState ConvergenceState;
	ConvergenceState.RegionId = EManyNamesRegionId::Convergence;
	ConvergenceState.ActiveDeityId = TEXT("many_names");
	ConvergenceState.AvailableQuestIds = {TEXT("convergence_main_01")};
	WorldState.Regions.Add(EManyNamesRegionId::Convergence, ConvergenceState);

	FManyNamesCompanionState OracleState;
	OracleState.CompanionId = EManyNamesCompanionId::OracleAI;
	WorldState.Companions.Add(EManyNamesCompanionId::OracleAI, OracleState);

	FManyNamesCompanionState SkyState;
	SkyState.CompanionId = EManyNamesCompanionId::SkyRuler;
	WorldState.Companions.Add(EManyNamesCompanionId::SkyRuler, SkyState);

	FManyNamesCompanionState BronzeState;
	BronzeState.CompanionId = EManyNamesCompanionId::BronzeLawgiver;
	WorldState.Companions.Add(EManyNamesCompanionId::BronzeLawgiver, BronzeState);

	WorldState.UnlockedPowers.Add(EManyNamesPowerId::TraumaRecovery);
}
