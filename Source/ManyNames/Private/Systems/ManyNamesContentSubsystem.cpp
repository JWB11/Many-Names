#include "Systems/ManyNamesContentSubsystem.h"

#include "Core/ManyNamesDeveloperSettings.h"

#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	template <typename StructType>
	bool LoadJsonArrayFile(const FString& AbsolutePath, TArray<StructType>& OutArray)
	{
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
		{
			return false;
		}

		return FJsonObjectConverter::JsonArrayStringToUStruct(JsonText, &OutArray, 0, 0);
	}

	bool LoadJsonObjectArray(const FString& AbsolutePath, TArray<TSharedPtr<FJsonValue>>& OutArray)
	{
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
		{
			return false;
		}

		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		return FJsonSerializer::Deserialize(Reader, OutArray);
	}

	bool TryParseRegionId(const FString& RegionName, EManyNamesRegionId& OutRegionId)
	{
		if (RegionName == TEXT("Opening"))
		{
			OutRegionId = EManyNamesRegionId::Opening;
			return true;
		}
		if (RegionName == TEXT("Egypt"))
		{
			OutRegionId = EManyNamesRegionId::Egypt;
			return true;
		}
		if (RegionName == TEXT("Greece"))
		{
			OutRegionId = EManyNamesRegionId::Greece;
			return true;
		}
		if (RegionName == TEXT("ItalicWest"))
		{
			OutRegionId = EManyNamesRegionId::ItalicWest;
			return true;
		}
		if (RegionName == TEXT("Convergence"))
		{
			OutRegionId = EManyNamesRegionId::Convergence;
			return true;
		}

		return false;
	}

	TArray<FString> GetStringArrayField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName)
	{
		TArray<FString> Results;
		const TArray<TSharedPtr<FJsonValue>>* JsonValues = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, JsonValues))
		{
			return Results;
		}

		for (const TSharedPtr<FJsonValue>& Value : *JsonValues)
		{
			FString StringValue;
			if (Value.IsValid() && Value->TryGetString(StringValue))
			{
				Results.Add(StringValue);
			}
		}

		return Results;
	}

	void PopulateNameArray(const TArray<FString>& SourceValues, TArray<FName>& OutValues)
	{
		for (const FString& Value : SourceValues)
		{
			OutValues.Add(FName(*Value));
		}
	}

	void PopulateGameplayTagArray(const TArray<FString>& SourceValues, TArray<FGameplayTag>& OutValues)
	{
		for (const FString& Value : SourceValues)
		{
			const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Value), false);
			if (Tag.IsValid())
			{
				OutValues.Add(Tag);
			}
		}
	}

	void PopulateGameplayTagContainer(const TArray<FString>& SourceValues, FGameplayTagContainer& OutValues)
	{
		for (const FString& Value : SourceValues)
		{
			const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Value), false);
			if (Tag.IsValid())
			{
				OutValues.AddTag(Tag);
			}
		}
	}
}

void UManyNamesContentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadPrimaryContent();
	ReloadSupplementalContent();
}

bool UManyNamesContentSubsystem::ReloadPrimaryContent()
{
	RegionDataTable = nullptr;
	QuestDataTable = nullptr;
	DialogueChoiceDataTable = nullptr;
	RegionsById.Reset();
	QuestsById.Reset();
	DialogueChoicesByQuestId.Reset();

	return LoadPrimaryDataTables();
}

bool UManyNamesContentSubsystem::ReloadSupplementalContent()
{
	QuestSteps.Reset();
	QuestStepsByQuestId.Reset();
	ChoiceConsequences.Reset();
	ChoiceConsequencesByChoiceId.Reset();
	EndingGates.Reset();
	EndingGatesById.Reset();
	DialogueScenes.Reset();
	DialogueScenesByQuestId.Reset();
	CharacterCast.Reset();
	CharacterCastById.Reset();
	AmbientProfiles.Reset();
	AmbientProfilesById.Reset();
	CinematicScenes.Reset();
	CinematicScenesById.Reset();
	CinematicScenesByQuestId.Reset();
	AudioProfiles.Reset();
	AudioProfilesById.Reset();
	ExternalAssetLicenses.Reset();
	ExternalAssetLicensesById.Reset();

	const bool bQuestStepsLoaded = LoadQuestSteps();
	const bool bConsequencesLoaded = LoadChoiceConsequences();
	const bool bEndingGatesLoaded = LoadEndingGates();
	const bool bDialogueScenesLoaded = LoadDialogueScenes();
	const bool bCharacterCastLoaded = LoadCharacterCast();
	const bool bAmbientProfilesLoaded = LoadAmbientProfiles();
	const bool bCinematicScenesLoaded = LoadCinematicScenes();
	const bool bAudioProfilesLoaded = LoadAudioProfiles();
	const bool bExternalLicensesLoaded = LoadExternalAssetLicenses();
	return bQuestStepsLoaded && bConsequencesLoaded && bEndingGatesLoaded && bDialogueScenesLoaded &&
		bCharacterCastLoaded && bAmbientProfilesLoaded && bCinematicScenesLoaded &&
		bAudioProfilesLoaded && bExternalLicensesLoaded;
}

bool UManyNamesContentSubsystem::GetRegionRow(EManyNamesRegionId RegionId, FManyNamesRegionRow& OutRow) const
{
	if (const FManyNamesRegionRow* Row = RegionsById.Find(RegionId))
	{
		OutRow = *Row;
		return true;
	}

	return false;
}

bool UManyNamesContentSubsystem::GetQuestRow(FName QuestId, FManyNamesQuestRow& OutRow) const
{
	if (const FManyNamesQuestRow* Row = QuestsById.Find(QuestId))
	{
		OutRow = *Row;
		return true;
	}

	return false;
}

TArray<FManyNamesQuestRow> UManyNamesContentSubsystem::GetAllQuestRows() const
{
	TArray<FManyNamesQuestRow> Rows;
	QuestsById.GenerateValueArray(Rows);
	return Rows;
}

TArray<FManyNamesDialogueChoiceRow> UManyNamesContentSubsystem::GetDialogueChoicesForQuest(FName QuestId) const
{
	if (const TArray<FManyNamesDialogueChoiceRow>* Records = DialogueChoicesByQuestId.Find(QuestId))
	{
		return *Records;
	}

	return {};
}

bool UManyNamesContentSubsystem::GetDialogueSceneForQuest(FName QuestId, FManyNamesDialogueSceneRecord& OutRecord) const
{
	if (const TArray<FManyNamesDialogueSceneRecord>* Records = DialogueScenesByQuestId.Find(QuestId))
	{
		if (Records->Num() > 0)
		{
			OutRecord = (*Records)[0];
			return true;
		}
	}

	return false;
}

TArray<FManyNamesDialogueSceneRecord> UManyNamesContentSubsystem::GetDialogueScenesForQuest(FName QuestId) const
{
	if (const TArray<FManyNamesDialogueSceneRecord>* Records = DialogueScenesByQuestId.Find(QuestId))
	{
		return *Records;
	}

	return {};
}

TArray<FManyNamesDialogueSceneRecord> UManyNamesContentSubsystem::GetAllDialogueScenes() const
{
	return DialogueScenes;
}

bool UManyNamesContentSubsystem::GetCharacterCastRecord(FName CharacterId, FManyNamesCharacterCastRecord& OutRecord) const
{
	if (const FManyNamesCharacterCastRecord* Record = CharacterCastById.Find(CharacterId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesCharacterCastRecord> UManyNamesContentSubsystem::GetAllCharacterCastRecords() const
{
	return CharacterCast;
}

bool UManyNamesContentSubsystem::GetAmbientProfile(FName ProfileId, FManyNamesAmbientProfileRecord& OutRecord) const
{
	if (const FManyNamesAmbientProfileRecord* Record = AmbientProfilesById.Find(ProfileId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesAmbientProfileRecord> UManyNamesContentSubsystem::GetAllAmbientProfiles() const
{
	return AmbientProfiles;
}

bool UManyNamesContentSubsystem::GetCinematicScene(FName SceneId, FManyNamesCinematicSceneRecord& OutRecord) const
{
	if (const FManyNamesCinematicSceneRecord* Record = CinematicScenesById.Find(SceneId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesCinematicSceneRecord> UManyNamesContentSubsystem::GetAllCinematicScenes() const
{
	return CinematicScenes;
}

TArray<FManyNamesCinematicSceneRecord> UManyNamesContentSubsystem::GetCinematicScenesForQuest(FName QuestId) const
{
	if (const TArray<FManyNamesCinematicSceneRecord>* Records = CinematicScenesByQuestId.Find(QuestId))
	{
		return *Records;
	}

	return {};
}

bool UManyNamesContentSubsystem::GetAudioProfile(FName AudioId, FManyNamesAudioProfileRecord& OutRecord) const
{
	if (const FManyNamesAudioProfileRecord* Record = AudioProfilesById.Find(AudioId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesAudioProfileRecord> UManyNamesContentSubsystem::GetAllAudioProfiles() const
{
	return AudioProfiles;
}

bool UManyNamesContentSubsystem::GetExternalAssetLicense(FName AssetId, FManyNamesExternalAssetLicenseRecord& OutRecord) const
{
	if (const FManyNamesExternalAssetLicenseRecord* Record = ExternalAssetLicensesById.Find(AssetId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesExternalAssetLicenseRecord> UManyNamesContentSubsystem::GetAllExternalAssetLicenses() const
{
	return ExternalAssetLicenses;
}

TArray<FManyNamesQuestStepRecord> UManyNamesContentSubsystem::GetQuestStepsForQuest(FName QuestId) const
{
	if (const TArray<FManyNamesQuestStepRecord>* Records = QuestStepsByQuestId.Find(QuestId))
	{
		return *Records;
	}

	return {};
}

TArray<FManyNamesQuestStepRecord> UManyNamesContentSubsystem::GetAllQuestSteps() const
{
	return QuestSteps;
}

bool UManyNamesContentSubsystem::GetChoiceConsequence(FName ChoiceId, FManyNamesChoiceConsequenceRecord& OutRecord) const
{
	if (const FManyNamesChoiceConsequenceRecord* Record = ChoiceConsequencesByChoiceId.Find(ChoiceId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesChoiceConsequenceRecord> UManyNamesContentSubsystem::GetAllChoiceConsequences() const
{
	return ChoiceConsequences;
}

bool UManyNamesContentSubsystem::GetEndingGate(FName EndingId, FManyNamesEndingGateRecord& OutRecord) const
{
	if (const FManyNamesEndingGateRecord* Record = EndingGatesById.Find(EndingId))
	{
		OutRecord = *Record;
		return true;
	}

	return false;
}

TArray<FManyNamesEndingGateRecord> UManyNamesContentSubsystem::GetAllEndingGates() const
{
	return EndingGates;
}

bool UManyNamesContentSubsystem::TryConvertEndingName(FName EndingName, EManyNamesEndingId& OutEndingId) const
{
	static const TMap<FName, EManyNamesEndingId> EndingMap = {
		{TEXT("ReturnToFuture"), EManyNamesEndingId::ReturnToFuture},
		{TEXT("RemainAsMyth"), EManyNamesEndingId::RemainAsMyth},
		{TEXT("DismantleDivinity"), EManyNamesEndingId::DismantleDivinity},
		{TEXT("ReplaceCompanion"), EManyNamesEndingId::ReplaceCompanion},
		{TEXT("FragmentLegacy"), EManyNamesEndingId::FragmentLegacy}
	};

	if (const EManyNamesEndingId* Found = EndingMap.Find(EndingName))
	{
		OutEndingId = *Found;
		return true;
	}

	return false;
}

bool UManyNamesContentSubsystem::TryConvertCompanionName(FName CompanionName, EManyNamesCompanionId& OutCompanionId) const
{
	static const TMap<FName, EManyNamesCompanionId> CompanionMap = {
		{TEXT("OracleAI"), EManyNamesCompanionId::OracleAI},
		{TEXT("SkyRuler"), EManyNamesCompanionId::SkyRuler},
		{TEXT("BronzeLawgiver"), EManyNamesCompanionId::BronzeLawgiver}
	};

	if (const EManyNamesCompanionId* Found = CompanionMap.Find(CompanionName))
	{
		OutCompanionId = *Found;
		return true;
	}

	return false;
}

bool UManyNamesContentSubsystem::LoadPrimaryDataTables()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	RegionDataTable = Cast<UDataTable>(Settings->RegionDataTable.TryLoad());
	QuestDataTable = Cast<UDataTable>(Settings->QuestDataTable.TryLoad());
	DialogueChoiceDataTable = Cast<UDataTable>(Settings->DialogueChoiceDataTable.TryLoad());

	const bool bRegionsLoaded = LoadPrimaryRegions();
	const bool bQuestsLoaded = LoadPrimaryQuests();
	const bool bDialogueLoaded = LoadPrimaryDialogueChoices();
	return bRegionsLoaded && bQuestsLoaded && bDialogueLoaded;
}

bool UManyNamesContentSubsystem::LoadPrimaryRegions()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	TArray<TSharedPtr<FJsonValue>> JsonRows;
	if (!LoadJsonObjectArray(ResolveDataPath(Settings->RegionsJsonPath), JsonRows))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonRows)
	{
		const TSharedPtr<FJsonObject> Object = JsonValue.IsValid() ? JsonValue->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			continue;
		}

		FManyNamesRegionRow Row;
		Row.RegionKey = FName(*Object->GetStringField(TEXT("RegionKey")));
		Row.ActiveDeityId = FName(*Object->GetStringField(TEXT("ActiveDeityId")));
		Row.DisplayName = FText::FromString(Object->GetStringField(TEXT("DisplayName")));
		Row.ShortDescription = FText::FromString(Object->GetStringField(TEXT("ShortDescription")));
		Row.HubMap = FSoftObjectPath(Object->GetStringField(TEXT("HubMap")));

		PopulateNameArray(GetStringArrayField(Object, TEXT("QuestIds")), Row.QuestIds);
		PopulateNameArray(GetStringArrayField(Object, TEXT("EntryConditions")), Row.EntryConditionOutputs);
		PopulateGameplayTagContainer(GetStringArrayField(Object, TEXT("EntryConditions")), Row.EntryConditions);

		EManyNamesRegionId RegionId;
		if (!TryParseRegionId(Object->GetStringField(TEXT("RegionId")), RegionId))
		{
			continue;
		}

		Row.RegionId = RegionId;
		RegionsById.Add(Row.RegionId, Row);
	}

	return RegionsById.Num() > 0;
}

bool UManyNamesContentSubsystem::LoadPrimaryQuests()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	TArray<TSharedPtr<FJsonValue>> JsonRows;
	if (!LoadJsonObjectArray(ResolveDataPath(Settings->QuestsJsonPath), JsonRows))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonRows)
	{
		const TSharedPtr<FJsonObject> Object = JsonValue.IsValid() ? JsonValue->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			continue;
		}

		FManyNamesQuestRow Row;
		Row.QuestId = FName(*Object->GetStringField(TEXT("QuestId")));
		Row.Title = FText::FromString(Object->GetStringField(TEXT("Title")));
		Row.Summary = FText::FromString(Object->GetStringField(TEXT("Summary")));
		Row.bCanEscalateToCombat = Object->GetBoolField(TEXT("bCanEscalateToCombat"));
		Row.FailureStateId = FName(*Object->GetStringField(TEXT("FailureStateId")));
		Row.WorldStateOutputId = FName(*Object->GetStringField(TEXT("WorldStateOutputId")));

		PopulateNameArray(GetStringArrayField(Object, TEXT("PrerequisiteQuestIds")), Row.PrerequisiteQuestIds);
		PopulateGameplayTagArray(GetStringArrayField(Object, TEXT("RequiredDomains")), Row.RequiredDomains);
		PopulateGameplayTagArray(GetStringArrayField(Object, TEXT("RewardDomains")), Row.RewardDomains);

		EManyNamesRegionId RegionId;
		if (!TryParseRegionId(Object->GetStringField(TEXT("RegionId")), RegionId))
		{
			continue;
		}

		Row.RegionId = RegionId;
		QuestsById.Add(Row.QuestId, Row);
	}

	return QuestsById.Num() > 0;
}

bool UManyNamesContentSubsystem::LoadPrimaryDialogueChoices()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	TArray<TSharedPtr<FJsonValue>> JsonRows;
	if (!LoadJsonObjectArray(ResolveDataPath(Settings->DialogueChoicesJsonPath), JsonRows))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonRows)
	{
		const TSharedPtr<FJsonObject> Object = JsonValue.IsValid() ? JsonValue->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			continue;
		}

		FManyNamesDialogueChoiceRow Row;
		Row.ChoiceId = FName(*Object->GetStringField(TEXT("ChoiceId")));
		Row.QuestId = FName(*Object->GetStringField(TEXT("QuestId")));
		Row.Prompt = FText::FromString(Object->GetStringField(TEXT("Prompt")));
		Row.OptionText = FText::FromString(Object->GetStringField(TEXT("OptionText")));
		Row.CombatDelta = static_cast<int32>(Object->GetIntegerField(TEXT("CombatDelta")));

		PopulateGameplayTagArray(GetStringArrayField(Object, TEXT("RequiredDomains")), Row.RequiredDomains);
		PopulateGameplayTagArray(GetStringArrayField(Object, TEXT("GrantedDomains")), Row.GrantedDomains);
		PopulateGameplayTagContainer(GetStringArrayField(Object, TEXT("ResultTags")), Row.ResultTags);

		DialogueChoicesByQuestId.FindOrAdd(Row.QuestId).Add(Row);
	}

	return DialogueChoicesByQuestId.Num() > 0;
}

bool UManyNamesContentSubsystem::LoadQuestSteps()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->QuestStepsJsonPath), QuestSteps))
	{
		return false;
	}

	QuestSteps.Sort([](const FManyNamesQuestStepRecord& A, const FManyNamesQuestStepRecord& B)
	{
		if (A.QuestId == B.QuestId)
		{
			return A.StepIndex < B.StepIndex;
		}

		return A.QuestId.LexicalLess(B.QuestId);
	});

	for (const FManyNamesQuestStepRecord& Record : QuestSteps)
	{
		QuestStepsByQuestId.FindOrAdd(Record.QuestId).Add(Record);
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadChoiceConsequences()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->ChoiceConsequencesJsonPath), ChoiceConsequences))
	{
		return false;
	}

	for (const FManyNamesChoiceConsequenceRecord& Record : ChoiceConsequences)
	{
		ChoiceConsequencesByChoiceId.Add(Record.ChoiceId, Record);
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadEndingGates()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->EndingGatesJsonPath), EndingGates))
	{
		return false;
	}

	for (const FManyNamesEndingGateRecord& Record : EndingGates)
	{
		EndingGatesById.Add(Record.EndingId, Record);
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadDialogueScenes()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->DialogueScenesJsonPath), DialogueScenes))
	{
		return false;
	}

	for (const FManyNamesDialogueSceneRecord& Record : DialogueScenes)
	{
		if (!Record.QuestId.IsNone())
		{
			DialogueScenesByQuestId.FindOrAdd(Record.QuestId).Add(Record);
		}
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadCharacterCast()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->CharacterCastJsonPath), CharacterCast))
	{
		return false;
	}

	for (const FManyNamesCharacterCastRecord& Record : CharacterCast)
	{
		CharacterCastById.Add(Record.CharacterId, Record);
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadAmbientProfiles()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->AmbientProfilesJsonPath), AmbientProfiles))
	{
		return false;
	}

	for (const FManyNamesAmbientProfileRecord& Record : AmbientProfiles)
	{
		AmbientProfilesById.Add(Record.ProfileId, Record);
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadCinematicScenes()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->CinematicScenesJsonPath), CinematicScenes))
	{
		return false;
	}

	for (const FManyNamesCinematicSceneRecord& Record : CinematicScenes)
	{
		CinematicScenesById.Add(Record.SceneId, Record);
		if (!Record.QuestId.IsNone())
		{
			CinematicScenesByQuestId.FindOrAdd(Record.QuestId).Add(Record);
		}
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadAudioProfiles()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->AudioProfilesJsonPath), AudioProfiles))
	{
		return false;
	}

	for (const FManyNamesAudioProfileRecord& Record : AudioProfiles)
	{
		AudioProfilesById.Add(Record.AudioId, Record);
	}

	return true;
}

bool UManyNamesContentSubsystem::LoadExternalAssetLicenses()
{
	const UManyNamesDeveloperSettings* Settings = GetDefault<UManyNamesDeveloperSettings>();
	if (!LoadJsonArrayFile(ResolveDataPath(Settings->ExternalAssetLicensesJsonPath), ExternalAssetLicenses))
	{
		return false;
	}

	for (const FManyNamesExternalAssetLicenseRecord& Record : ExternalAssetLicenses)
	{
		ExternalAssetLicensesById.Add(Record.AssetId, Record);
	}

	return true;
}

FString UManyNamesContentSubsystem::ResolveDataPath(const FString& RelativePath) const
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
}
