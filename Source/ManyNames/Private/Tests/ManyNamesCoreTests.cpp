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

	FManyNamesAmbientProfileRecord EgyptPriestsProfile;
	TestTrue(TEXT("Egypt priests ambient profile exists"), ContentSubsystem->GetAmbientProfile(TEXT("Egypt.Priests"), EgyptPriestsProfile));
	TestEqual(TEXT("Egypt priests ambient profile targets Egypt"), EgyptPriestsProfile.RegionId, EManyNamesRegionId::Egypt);

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

#endif
