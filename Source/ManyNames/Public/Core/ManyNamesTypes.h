#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ManyNamesTypes.generated.h"

class UAnimInstance;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;

UENUM(BlueprintType)
enum class EManyNamesRegionId : uint8
{
	Opening UMETA(DisplayName = "Opening"),
	Egypt UMETA(DisplayName = "Egypt"),
	Greece UMETA(DisplayName = "Greece"),
	ItalicWest UMETA(DisplayName = "Italic West"),
	Convergence UMETA(DisplayName = "Convergence")
};

UENUM(BlueprintType)
enum class EManyNamesQuestState : uint8
{
	Locked,
	Available,
	Active,
	Completed,
	Failed,
	Escalated
};

UENUM(BlueprintType)
enum class EManyNamesCompanionId : uint8
{
	OracleAI,
	SkyRuler,
	BronzeLawgiver
};

UENUM(BlueprintType)
enum class EManyNamesAllianceState : uint8
{
	Unknown,
	Allied,
	Opposed,
	Replaced
};

UENUM(BlueprintType)
enum class EManyNamesPowerId : uint8
{
	TraumaRecovery,
	ThermalShielding,
	FocusShift,
	InsightPulse,
	ArtifactRecognition
};

UENUM(BlueprintType)
enum class EManyNamesEndingId : uint8
{
	ReturnToFuture,
	RemainAsMyth,
	DismantleDivinity,
	ReplaceCompanion,
	FragmentLegacy
};

UENUM(BlueprintType)
enum class EManyNamesWeatherPrecipitation : uint8
{
	None,
	Dust,
	Rain,
	Snow,
	Ash
};

UENUM(BlueprintType)
enum class EManyNamesRenderPath : uint8
{
	Auto,
	WindowsHighEnd,
	MacFallback
};

UENUM(BlueprintType)
enum class EManyNamesCrowdBehaviorTier : uint8
{
	StaticScenic,
	HubAmbient,
	StoryFacing
};

UENUM(BlueprintType)
enum class EManyNamesCompanionThreatState : uint8
{
	Dormant,
	Tempting,
	Ascendant,
	Dominant
};

USTRUCT(BlueprintType)
struct FManyNamesNpcVisualProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RoleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPreferMetaHuman = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UStaticMesh> FallbackStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UAnimInstance> AnimationBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector RelativeLocation = FVector(0.0f, 0.0f, -88.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector RelativeScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PoseVariant = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CameraAnchorTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClothingVariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClothTierId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName VoiceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLockFacingToPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableClothSimulation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FootPlacementWeight = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesCrowdBehaviorTier CrowdBehaviorTier = EManyNamesCrowdBehaviorTier::StaticScenic;
};

USTRUCT(BlueprintType)
struct FManyNamesRegionArtProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SunIntensity = 8.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SkyIntensity = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FogDensity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor KeyLightTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FogTint = FLinearColor(0.92f, 0.82f, 0.72f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoftObjectPath> StructuralMeshPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoftObjectPath> PropMeshPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> PopulationRoleTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString StyleNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BaselineWeatherStateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName HeroWeatherStateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HubPopulationDensity = 1.0f;
};

USTRUCT(BlueprintType)
struct FManyNamesRouteSplineDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RouteId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RouteWidth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RouteNotes;
};

USTRUCT(BlueprintType)
struct FManyNamesTerrainProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<class UMaterialInterface> LandscapeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TerrainOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TerrainExtent = FVector(12000.0f, 9000.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LandscapeScale = FVector(100.0f, 100.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LandscapeQuadsPerSection = 63;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LandscapeSectionsPerComponent = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint LandscapeComponentCount = FIntPoint(8, 8);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HeightAmplitude = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HeightBias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PrimaryHubLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PrimarySpawnLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FManyNamesRouteSplineDefinition> RouteSplines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoftObjectPath> StructuralNaniteMeshPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoftObjectPath> FoliageScatterMeshPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath SharedPcgGraphAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath RegionPcgGraphAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SharedPcgGraphId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RegionPcgGraphId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnTraceHeight = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnTraceDepth = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnSafetyLift = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableNaniteForStructuralMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableNaniteFoliage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bGenerateBackdropLandmarks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TerrainNotes;
};

USTRUCT(BlueprintType)
struct FManyNamesWeatherState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SunPitch = -32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SunYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SunIntensity = 8.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SkyIntensity = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FogDensity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor KeyLightTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FogTint = FLinearColor(0.92f, 0.82f, 0.72f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WindIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesWeatherPrecipitation Precipitation = EManyNamesWeatherPrecipitation::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PrecipitationIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundWetness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundSnow = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TraversalSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAffectsTraversal = false;
};

USTRUCT(BlueprintType)
struct FManyNamesEnvironmentProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FManyNamesWeatherState BaselineState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FManyNamesWeatherState HeroState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRenderPath PreferredRenderPath = EManyNamesRenderPath::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowMegaLights = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowHeterogeneousVolumes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowNaniteAssemblies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowNaniteSkinnedMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowNaniteVoxels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPreferMegaLightLocalLights = false;
};

USTRUCT(BlueprintType)
struct FManyNamesRegionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActiveDeityId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> AvailableQuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, int32> FactionStates;
};

USTRUCT(BlueprintType)
struct FManyNamesCompanionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesCompanionId CompanionId = EManyNamesCompanionId::OracleAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Affinity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTruthRevealed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesAllianceState AllianceState = EManyNamesAllianceState::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OutcomeTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesCompanionThreatState ThreatState = EManyNamesCompanionThreatState::Dormant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EscalationScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDominantAntagonist = false;
};

USTRUCT(BlueprintType)
struct FManyNamesRumorState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PublicMiracleScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ConcealmentScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CombatReputation = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EManyNamesRegionId, FGameplayTagContainer> RegionalInterpretations;
};

USTRUCT(BlueprintType)
struct FManyNamesDomainProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, int32> DomainScores;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer PublicDomains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EManyNamesRegionId, FGameplayTagContainer> RegionalInterpretations;
};

USTRUCT(BlueprintType)
struct FManyNamesChoiceRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ChoiceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SelectedOptionId = NAME_None;
};

USTRUCT(BlueprintType)
struct FManyNamesWorldState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<FName> WorldStateOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EManyNamesRegionId> RegionVisitOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EManyNamesRegionId, FManyNamesRegionState> Regions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EManyNamesCompanionId, FManyNamesCompanionState> Companions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FManyNamesDomainProfile MythicDomainProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FManyNamesRumorState RumorProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FManyNamesChoiceRecord> MajorChoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<EManyNamesPowerId> UnlockedPowers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<EManyNamesEndingId> EligibleEndings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesCompanionId DominantAntagonist = EManyNamesCompanionId::OracleAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasDominantAntagonist = false;
};

USTRUCT(BlueprintType)
struct FManyNamesJournalEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMainQuest = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesQuestState QuestState = EManyNamesQuestState::Locked;
};

USTRUCT(BlueprintType)
struct FManyNamesRegionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RegionKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ShortDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActiveDeityId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath HubMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> QuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer EntryConditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> EntryConditionOutputs;
};

USTRUCT(BlueprintType)
struct FManyNamesQuestRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Summary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> PrerequisiteQuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> RequiredDomains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> RewardDomains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanEscalateToCombat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FailureStateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WorldStateOutputId = NAME_None;
};

USTRUCT(BlueprintType)
struct FManyNamesDialogueChoiceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ChoiceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText OptionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> RequiredDomains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> GrantedDomains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CombatDelta = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ResultTags;
};

USTRUCT(BlueprintType)
struct FManyNamesDialogueSceneRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SceneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SpeakerRole;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText BodyText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LocationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CameraAnchorTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeatherStateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> ChoiceIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLockMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseRoamingCamera = true;
};

USTRUCT(BlueprintType)
struct FManyNamesCharacterCastRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNamedCharacter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Occupation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Origin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Presentation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText AgeRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText PhysicalBuild;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SkinToneNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText HairNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText GroomingNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText WardrobeNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DemeanorNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OccupationTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RoleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClothingVariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClothTierId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName VoiceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FootIkProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableClothSimulation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USkeletalMesh> PreferredSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UAnimationAsset> IdleAnimation;
};

USTRUCT(BlueprintType)
struct FManyNamesAmbientProfileRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OccupationTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RoleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> CharacterIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClothingVariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CrowdCountHint = 6;
};

USTRUCT(BlueprintType)
struct FManyNamesCinematicSceneRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SceneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DialogueSceneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Summary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LocationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CameraAnchorTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeatherStateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath SequenceAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> AudioProfileIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> RequiredWorldStateOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EstimatedDurationSeconds = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSkippable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEndingScene = false;
};

USTRUCT(BlueprintType)
struct FManyNamesAudioProfileRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AudioId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EManyNamesRegionId RegionId = EManyNamesRegionId::Opening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CategoryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath SoundAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceFile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> MoodTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EstimatedDurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLooping = false;
};

USTRUCT(BlueprintType)
struct FManyNamesExternalAssetLicenseRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AssetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CategoryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SourceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText LicenseName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText UsageNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bGeneratedInProject = false;
};

USTRUCT(BlueprintType)
struct FManyNamesRumorEffectRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PublicMiracle = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Concealment = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CombatReputation = 0;
};

USTRUCT(BlueprintType)
struct FManyNamesQuestStepRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StepId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StepIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Objective;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LocationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> RequiredWorldStateOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> CompletionOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> UnlockChoiceIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName NonCombatResolutionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CombatEscalationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> DomainFocus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> CompanionFocus;
};

USTRUCT(BlueprintType)
struct FManyNamesChoiceConsequenceRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ChoiceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OutcomeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> WorldStateOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, int32> DomainDeltas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FManyNamesRumorEffectRecord RumorEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, int32> CompanionAffinityEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> EligibleEndings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanAvoidCombat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanTriggerCombat = false;
};

USTRUCT(BlueprintType)
struct FManyNamesEndingGateRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EndingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Summary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> RequiredWorldStateOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> RequiredAnyWorldStateOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> RequiredDomains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> BlockedByOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> PreferredChoiceIds;
};
