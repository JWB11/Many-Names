#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GameplayTagsManager.h"
#include "Core/ManyNamesTypes.h"
#include "Engine/GameInstance.h"
#include "Systems/ManyNamesGameInstance.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManyNamesDefaultWorldStateTest, "ManyNames.Core.DefaultWorldState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManyNamesDefaultWorldStateTest::RunTest(const FString& Parameters)
{
	FManyNamesWorldState WorldState;
	TestTrue(TEXT("Default world state starts with no region visits"), WorldState.RegionVisitOrder.IsEmpty());
	TestTrue(TEXT("Default world state starts with no eligible endings"), WorldState.EligibleEndings.IsEmpty());
	TestFalse(TEXT("Default world state starts without a dominant antagonist"), WorldState.bHasDominantAntagonist);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManyNamesPrimaryContentLoadTest, "ManyNames.Core.PrimaryContentLoad", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManyNamesPrimaryContentLoadTest::RunTest(const FString& Parameters)
{
	UGameInstance* TestGameInstance = NewObject<UGameInstance>(GetTransientPackage(), UGameInstance::StaticClass());
	TestNotNull(TEXT("Test game instance can be instantiated"), TestGameInstance);
	if (!TestGameInstance)
	{
		return false;
	}

	UManyNamesContentSubsystem* ContentSubsystem = NewObject<UManyNamesContentSubsystem>(TestGameInstance);
	TestNotNull(TEXT("Content subsystem can be instantiated"), ContentSubsystem);
	if (!ContentSubsystem)
	{
		return false;
	}

	TestTrue(TEXT("Primary content loads from authored data"), ContentSubsystem->ReloadPrimaryContent());
	TestTrue(TEXT("Supplemental content loads from authored data"), ContentSubsystem->ReloadSupplementalContent());

	FManyNamesRegionRow EgyptRow;
	TestTrue(TEXT("Egypt region row exists"), ContentSubsystem->GetRegionRow(EManyNamesRegionId::Egypt, EgyptRow));
	TestEqual(TEXT("Egypt hub map resolves"), EgyptRow.HubMap.ToString(), FString(TEXT("/Game/Maps/L_EgyptHub")));

	FManyNamesRegionRow GreeceRow;
	TestTrue(TEXT("Greece region row exists"), ContentSubsystem->GetRegionRow(EManyNamesRegionId::Greece, GreeceRow));
	TestEqual(TEXT("Greece hub map resolves"), GreeceRow.HubMap.ToString(), FString(TEXT("/Game/Maps/L_GreeceHub")));

	FManyNamesRegionRow ItalicRow;
	TestTrue(TEXT("Italic region row exists"), ContentSubsystem->GetRegionRow(EManyNamesRegionId::ItalicWest, ItalicRow));
	TestEqual(TEXT("Italic hub map resolves"), ItalicRow.HubMap.ToString(), FString(TEXT("/Game/Maps/L_ItalicHub")));

	FManyNamesRegionRow ConvergenceRow;
	TestTrue(TEXT("Convergence region row exists"), ContentSubsystem->GetRegionRow(EManyNamesRegionId::Convergence, ConvergenceRow));
	TestEqual(TEXT("Convergence hub map resolves"), ConvergenceRow.HubMap.ToString(), FString(TEXT("/Game/Maps/L_Convergence")));

	FManyNamesQuestRow OpeningQuest;
	TestTrue(TEXT("Opening main quest exists"), ContentSubsystem->GetQuestRow(TEXT("opening_main_01"), OpeningQuest));
	TestEqual(TEXT("Opening main quest grants two reward domains"), OpeningQuest.RewardDomains.Num(), 2);

	const FGameplayTag LightTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Domain.Light"), false);
	const FGameplayTag HealingTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Domain.Healing"), false);
	TestTrue(TEXT("Opening main quest includes Domain.Light"), OpeningQuest.RewardDomains.Contains(LightTag));
	TestTrue(TEXT("Opening main quest includes Domain.Healing"), OpeningQuest.RewardDomains.Contains(HealingTag));

	const TArray<FManyNamesDialogueChoiceRow> WitnessChoices = ContentSubsystem->GetDialogueChoicesForQuest(TEXT("opening_side_01"));
	TestEqual(TEXT("Opening side quest has two dialogue choices"), WitnessChoices.Num(), 2);

	const TArray<FManyNamesDialogueChoiceRow> GreeceChoices = ContentSubsystem->GetDialogueChoicesForQuest(TEXT("greece_main_01"));
	TestTrue(TEXT("Greece main quest has dialogue choices"), GreeceChoices.Num() > 0);
	TestTrue(TEXT("Greece main quest has expanded branch count"), GreeceChoices.Num() >= 4);

	const TArray<FManyNamesDialogueChoiceRow> ItalicChoices = ContentSubsystem->GetDialogueChoicesForQuest(TEXT("italic_main_01"));
	TestTrue(TEXT("Italic main quest has dialogue choices"), ItalicChoices.Num() > 0);

	FManyNamesDialogueSceneRecord OpeningWitnessScene;
	TestTrue(TEXT("Opening witness scene exists"), ContentSubsystem->GetDialogueSceneForQuest(TEXT("opening_side_01"), OpeningWitnessScene));
	TestEqual(TEXT("Opening witness scene references two choices"), OpeningWitnessScene.ChoiceIds.Num(), 2);

	FManyNamesDialogueSceneRecord ConvergenceScene;
	TestTrue(TEXT("Convergence scene exists"), ContentSubsystem->GetDialogueSceneForQuest(TEXT("convergence_main_01"), ConvergenceScene));
	TestEqual(TEXT("Convergence scene exposes five ending choices"), ConvergenceScene.ChoiceIds.Num(), 5);

	FManyNamesCharacterCastRecord ArchiveKeeperCast;
	TestTrue(TEXT("Archive keeper cast exists"), ContentSubsystem->GetCharacterCastRecord(TEXT("ArchiveKeeper"), ArchiveKeeperCast));
	TestEqual(TEXT("Archive keeper is in Egypt"), ArchiveKeeperCast.RegionId, EManyNamesRegionId::Egypt);

	FManyNamesCharacterCastRecord TempleCantorCast;
	TestTrue(TEXT("Temple cantor cast exists"), ContentSubsystem->GetCharacterCastRecord(TEXT("TempleCantor"), TempleCantorCast));
	TestEqual(TEXT("Temple cantor is in Egypt"), TempleCantorCast.RegionId, EManyNamesRegionId::Egypt);

	FManyNamesCharacterCastRecord ForgeMatronCast;
	TestTrue(TEXT("Forge matron cast exists"), ContentSubsystem->GetCharacterCastRecord(TEXT("ForgeMatron"), ForgeMatronCast));
	TestEqual(TEXT("Forge matron is in Italic West"), ForgeMatronCast.RegionId, EManyNamesRegionId::ItalicWest);

	FManyNamesQuestRow EgyptSideTwoQuest;
	TestTrue(TEXT("Egypt side 02 quest exists"), ContentSubsystem->GetQuestRow(TEXT("egypt_side_02"), EgyptSideTwoQuest));
	TestEqual(TEXT("Egypt side 02 lives in Egypt"), EgyptSideTwoQuest.RegionId, EManyNamesRegionId::Egypt);

	FManyNamesQuestRow GreeceSideTwoQuest;
	TestTrue(TEXT("Greece side 02 quest exists"), ContentSubsystem->GetQuestRow(TEXT("greece_side_02"), GreeceSideTwoQuest));
	TestEqual(TEXT("Greece side 02 lives in Greece"), GreeceSideTwoQuest.RegionId, EManyNamesRegionId::Greece);

	FManyNamesQuestRow ItalicSideTwoQuest;
	TestTrue(TEXT("Italic side 02 quest exists"), ContentSubsystem->GetQuestRow(TEXT("italic_side_02"), ItalicSideTwoQuest));
	TestEqual(TEXT("Italic side 02 lives in Italic West"), ItalicSideTwoQuest.RegionId, EManyNamesRegionId::ItalicWest);

	FManyNamesQuestRow ConvergenceSideTwoQuest;
	TestTrue(TEXT("Convergence side 02 quest exists"), ContentSubsystem->GetQuestRow(TEXT("convergence_side_02"), ConvergenceSideTwoQuest));
	TestEqual(TEXT("Convergence side 02 lives in Convergence"), ConvergenceSideTwoQuest.RegionId, EManyNamesRegionId::Convergence);

	FManyNamesAmbientProfileRecord EgyptPriestsProfile;
	TestTrue(TEXT("Egypt priests ambient profile exists"), ContentSubsystem->GetAmbientProfile(TEXT("Egypt.Priests"), EgyptPriestsProfile));
	TestEqual(TEXT("Egypt priests ambient profile targets Egypt"), EgyptPriestsProfile.RegionId, EManyNamesRegionId::Egypt);
	TestTrue(TEXT("Egypt priests ambient profile has multiple cast sources"), EgyptPriestsProfile.CharacterIds.Num() >= 2);

	FManyNamesAmbientProfileRecord EgyptAdjudicatorsProfile;
	TestTrue(TEXT("Egypt adjudicators ambient profile exists"), ContentSubsystem->GetAmbientProfile(TEXT("Egypt.Adjudicators"), EgyptAdjudicatorsProfile));
	TestEqual(TEXT("Egypt adjudicators ambient profile targets Egypt"), EgyptAdjudicatorsProfile.RegionId, EManyNamesRegionId::Egypt);

	FManyNamesDialogueSceneRecord EgyptLedgerScene;
	TestTrue(TEXT("Egypt ledger scene exists"), ContentSubsystem->GetDialogueSceneForQuest(TEXT("egypt_side_02"), EgyptLedgerScene));
	TestEqual(TEXT("Egypt ledger scene exposes three choices"), EgyptLedgerScene.ChoiceIds.Num(), 3);

	FManyNamesCinematicSceneRecord EgyptArrivalCinematic;
	TestTrue(TEXT("Egypt arrival cinematic exists"), ContentSubsystem->GetCinematicScene(TEXT("cin_egypt_arrival"), EgyptArrivalCinematic));
	TestEqual(TEXT("Egypt arrival cinematic belongs to Egypt"), EgyptArrivalCinematic.RegionId, EManyNamesRegionId::Egypt);

	const TArray<FManyNamesCinematicSceneRecord> ConvergenceCinematics = ContentSubsystem->GetCinematicScenesForQuest(TEXT("convergence_main_01"));
	TestTrue(TEXT("Convergence has multiple cinematics"), ConvergenceCinematics.Num() >= 5);

	FManyNamesAudioProfileRecord OpeningTheme;
	TestTrue(TEXT("Opening theme audio profile exists"), ContentSubsystem->GetAudioProfile(TEXT("music_opening_theme"), OpeningTheme));
	TestTrue(TEXT("Opening theme loops"), OpeningTheme.bLooping);

	FManyNamesExternalAssetLicenseRecord BlenderLicense;
	TestTrue(TEXT("Blender license record exists"), ContentSubsystem->GetExternalAssetLicense(TEXT("tool_blender"), BlenderLicense));

	bool bFoundLightChoice = false;
	bool bFoundDeceptionChoice = false;
	const FGameplayTag DeceptionTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Domain.Deception"), false);
	for (const FManyNamesDialogueChoiceRow& Choice : WitnessChoices)
	{
		bFoundLightChoice |= Choice.RequiredDomains.Contains(LightTag);
		bFoundDeceptionChoice |= Choice.RequiredDomains.Contains(DeceptionTag);
	}

	TestTrue(TEXT("Witness choices include a light-gated option"), bFoundLightChoice);
	TestTrue(TEXT("Witness choices include a deception-gated option"), bFoundDeceptionChoice);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManyNamesWorldStateInitTest, "ManyNames.Core.WorldStateInit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManyNamesWorldStateInitTest::RunTest(const FString& Parameters)
{
	UManyNamesGameInstance* GameInstance = NewObject<UManyNamesGameInstance>(GetTransientPackage(), UManyNamesGameInstance::StaticClass());
	TestNotNull(TEXT("ManyNames game instance can be instantiated"), GameInstance);
	if (!GameInstance)
	{
		return false;
	}

	GameInstance->InitializeDefaultWorldState();
	const FManyNamesWorldState& WorldState = GameInstance->GetWorldState();

	const FManyNamesRegionState* EgyptState = WorldState.Regions.Find(EManyNamesRegionId::Egypt);
	TestNotNull(TEXT("Egypt region state exists after init"), EgyptState);
	if (EgyptState)
	{
		TestTrue(TEXT("Egypt AvailableQuestIds includes egypt_main_01"), EgyptState->AvailableQuestIds.Contains(TEXT("egypt_main_01")));
		TestTrue(TEXT("Egypt AvailableQuestIds includes egypt_side_01"), EgyptState->AvailableQuestIds.Contains(TEXT("egypt_side_01")));
		TestTrue(TEXT("Egypt AvailableQuestIds includes egypt_side_02"), EgyptState->AvailableQuestIds.Contains(TEXT("egypt_side_02")));
	}

	const FManyNamesRegionState* GreeceState = WorldState.Regions.Find(EManyNamesRegionId::Greece);
	TestNotNull(TEXT("Greece region state exists after init"), GreeceState);
	if (GreeceState)
	{
		TestTrue(TEXT("Greece AvailableQuestIds includes greece_main_01"), GreeceState->AvailableQuestIds.Contains(TEXT("greece_main_01")));
		TestTrue(TEXT("Greece AvailableQuestIds includes greece_side_01"), GreeceState->AvailableQuestIds.Contains(TEXT("greece_side_01")));
		TestTrue(TEXT("Greece AvailableQuestIds includes greece_side_02"), GreeceState->AvailableQuestIds.Contains(TEXT("greece_side_02")));
	}

	const FManyNamesRegionState* ItalicState = WorldState.Regions.Find(EManyNamesRegionId::ItalicWest);
	TestNotNull(TEXT("Italic West region state exists after init"), ItalicState);
	if (ItalicState)
	{
		TestTrue(TEXT("Italic West AvailableQuestIds includes italic_main_01"), ItalicState->AvailableQuestIds.Contains(TEXT("italic_main_01")));
		TestTrue(TEXT("Italic West AvailableQuestIds includes italic_side_01"), ItalicState->AvailableQuestIds.Contains(TEXT("italic_side_01")));
		TestTrue(TEXT("Italic West AvailableQuestIds includes italic_side_02"), ItalicState->AvailableQuestIds.Contains(TEXT("italic_side_02")));
	}

	const FManyNamesRegionState* ConvergenceState = WorldState.Regions.Find(EManyNamesRegionId::Convergence);
	TestNotNull(TEXT("Convergence region state exists after init"), ConvergenceState);
	if (ConvergenceState)
	{
		TestTrue(TEXT("Convergence AvailableQuestIds includes convergence_main_01"), ConvergenceState->AvailableQuestIds.Contains(TEXT("convergence_main_01")));
		TestTrue(TEXT("Convergence AvailableQuestIds includes convergence_side_02"), ConvergenceState->AvailableQuestIds.Contains(TEXT("convergence_side_02")));
	}

	TestTrue(TEXT("Opening region starts unlocked"), WorldState.Regions.FindRef(EManyNamesRegionId::Opening).bUnlocked);
	TestFalse(TEXT("Egypt region starts locked"), WorldState.Regions.FindRef(EManyNamesRegionId::Egypt).bUnlocked);
	TestFalse(TEXT("Greece region starts locked"), WorldState.Regions.FindRef(EManyNamesRegionId::Greece).bUnlocked);
	TestFalse(TEXT("Italic West region starts locked"), WorldState.Regions.FindRef(EManyNamesRegionId::ItalicWest).bUnlocked);

	TestTrue(TEXT("All three companions are initialized"), WorldState.Companions.Num() == 3);
	TestTrue(TEXT("TraumaRecovery power starts unlocked"), WorldState.UnlockedPowers.Contains(EManyNamesPowerId::TraumaRecovery));

	return true;
}

#endif
