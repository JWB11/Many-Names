#include "Editor/ManyNamesWorldBuildCommandlet.h"

#include "Animation/AnimationAsset.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Brush.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "Gameplay/ManyNamesInteractableActor.h"
#include "Gameplay/ManyNamesScenicActor.h"
#include "GameplayTagsManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace
{
	struct FManyNamesBuildAssets
	{
		UStaticMesh* Plane = nullptr;
		UStaticMesh* Cube = nullptr;
		UStaticMesh* Cylinder = nullptr;
		UStaticMesh* Cone = nullptr;
		UStaticMesh* Sphere = nullptr;

		UStaticMesh* EgyptFloor = nullptr;
		UStaticMesh* EgyptWallA = nullptr;
		UStaticMesh* EgyptWallB = nullptr;
		UStaticMesh* EgyptColumn = nullptr;
		UStaticMesh* EgyptRoof = nullptr;
		UStaticMesh* EgyptStairs = nullptr;
		UStaticMesh* BentPyramid = nullptr;
		UStaticMesh* MastabaEntrance = nullptr;
		UStaticMesh* AncientCarvedStone = nullptr;
		UStaticMesh* PyramidStone = nullptr;
		UStaticMesh* CaveRock = nullptr;
		UStaticMesh* HighDetailRock = nullptr;
		UStaticMesh* RealisticRock = nullptr;
		UStaticMesh* Rock001 = nullptr;
		UStaticMesh* GreeceDolmen = nullptr;
		UStaticMesh* ItalicChurchRock = nullptr;
		UStaticMesh* ItalicBollard = nullptr;
		UStaticMesh* ConvergenceDestroyedWood = nullptr;

		USkeletalMesh* Manny = nullptr;
		USkeletalMesh* Quinn = nullptr;
		USkeletalMesh* HumanFryPose = nullptr;

		UAnimationAsset* IdleAnimation = nullptr;
		UAnimationAsset* DeathBack = nullptr;
		UAnimationAsset* DeathLeft = nullptr;
		UAnimationAsset* DeathRight = nullptr;

		UMaterialInterface* AshStone = nullptr;
		UMaterialInterface* WreckMetal = nullptr;
		UMaterialInterface* SandStone = nullptr;
		UMaterialInterface* Plaster = nullptr;
		UMaterialInterface* Basalt = nullptr;
		UMaterialInterface* Bronze = nullptr;
		UMaterialInterface* Linen = nullptr;
		UMaterialInterface* Water = nullptr;
		UMaterialInterface* Miracle = nullptr;
		UMaterialInterface* Oracle = nullptr;
	};

	struct FManyNamesRoleTags
	{
		FGameplayTag NamedInteractable;
		FGameplayTag NamedScenic;
		FGameplayTag AmbientGuard;
		FGameplayTag AmbientCivilian;
		FGameplayTag AmbientRitual;
	};

	template <typename AssetType>
	AssetType* LoadAsset(const TCHAR* AssetPath)
	{
		return LoadObject<AssetType>(nullptr, AssetPath);
	}

	template <typename AssetType>
	AssetType* FirstValid(std::initializer_list<AssetType*> Candidates)
	{
		for (AssetType* Candidate : Candidates)
		{
			if (Candidate)
			{
				return Candidate;
			}
		}

		return nullptr;
	}

	FManyNamesRoleTags LoadRoleTags()
	{
		FManyNamesRoleTags RoleTags;
		RoleTags.NamedInteractable = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("NPC.NamedInteractable"), false);
		RoleTags.NamedScenic = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("NPC.NamedScenic"), false);
		RoleTags.AmbientGuard = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("NPC.AmbientGuard"), false);
		RoleTags.AmbientCivilian = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("NPC.AmbientCivilian"), false);
		RoleTags.AmbientRitual = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("NPC.AmbientRitual"), false);
		return RoleTags;
	}

	void ApplyMaterialToAllSlots(UMeshComponent* MeshComponent, UMaterialInterface* Material)
	{
		if (!MeshComponent || !Material)
		{
			return;
		}

		const int32 SlotCount = FMath::Max(1, MeshComponent->GetNumMaterials());
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			MeshComponent->SetMaterial(Index, Material);
		}
	}

	void AddAssetPath(TArray<FSoftObjectPath>& Paths, const UObject* Asset)
	{
		if (Asset)
		{
			Paths.Add(FSoftObjectPath(Asset->GetPathName()));
		}
	}

	FManyNamesBuildAssets LoadBuildAssets()
	{
		FManyNamesBuildAssets Assets;
		Assets.Plane = LoadAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Plane.Plane"));
		Assets.Cube = LoadAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube"));
		Assets.Cylinder = LoadAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		Assets.Cone = LoadAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cone.Cone"));
		Assets.Sphere = LoadAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

		Assets.EgyptFloor = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPCIAN_FLOOR1.EGYPCIAN_FLOOR1"));
		Assets.EgyptWallA = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPTIAN_WALL_1.EGYPTIAN_WALL_1"));
		Assets.EgyptWallB = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPTIAN_WALL_2.EGYPTIAN_WALL_2"));
		Assets.EgyptColumn = FirstValid<UStaticMesh>({
			LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPTIAN_COLUMNS.EGYPTIAN_COLUMNS")),
			LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPTIAN_COLUMN_2.EGYPTIAN_COLUMN_2"))
		});
		Assets.EgyptRoof = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPTIAN_ROOF.EGYPTIAN_ROOF"));
		Assets.EgyptStairs = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPTIAN_STAIRS_ENTRANCE.EGYPTIAN_STAIRS_ENTRANCE"));
		Assets.BentPyramid = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/BentPyramidTemple/DAHSHUR2.DAHSHUR2"));
		Assets.MastabaEntrance = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentEgypt/PtahshepsesMastabaEntrance/ABUSIR2.ABUSIR2"));
		Assets.AncientCarvedStone = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/PropsShared/AncientCarvedStone/carvedstone011.carvedstone011"));
		Assets.PyramidStone = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/PropsShared/PyramidShapeStone/sm_pyramid_stone.sm_pyramid_stone"));
		Assets.CaveRock = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/PropsShared/CaveRock/sm_stone_ga312251958.sm_stone_ga312251958"));
		Assets.HighDetailRock = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/PropsShared/HighDetailRock/sm_stone_ga3120037.sm_stone_ga3120037"));
		Assets.RealisticRock = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/PropsShared/RealisticRock/Make_a_photorealistic_0426214013_texture.Make_a_photorealistic_0426214013_texture"));
		Assets.Rock001 = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/PropsShared/Rock001/Rock_001.Rock_001"));
		Assets.GreeceDolmen = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentGreece/StoneAgeDolmen/capeshj_lowpoly.capeshj_lowpoly"));
		Assets.ItalicChurchRock = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentItalic/ChurchRock/church_rock1.church_rock1"));
		Assets.ItalicBollard = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentItalic/RoundedCornerBollard/round-corner1.round-corner1"));
		Assets.ConvergenceDestroyedWood = LoadAsset<UStaticMesh>(TEXT("/Game/Marketplace/Fab/EnvironmentConvergence/CuttedDestroyedWood/shareModel.shareModel"));

		Assets.Manny = LoadAsset<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
		Assets.Quinn = LoadAsset<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
		Assets.HumanFryPose = LoadAsset<USkeletalMesh>(TEXT("/Game/Marketplace/Fab/ArtifactsHero/HumanFryPose/AS.AS"));

		Assets.IdleAnimation = LoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
		Assets.DeathBack = LoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
		Assets.DeathLeft = LoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01"));
		Assets.DeathRight = LoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01"));

		Assets.AshStone = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_AshStone.M_MN_AshStone"));
		Assets.WreckMetal = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_WreckMetal.M_MN_WreckMetal"));
		Assets.SandStone = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_SandStone.M_MN_SandStone"));
		Assets.Plaster = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Plaster.M_MN_Plaster"));
		Assets.Basalt = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Basalt.M_MN_Basalt"));
		Assets.Bronze = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Bronze.M_MN_Bronze"));
		Assets.Linen = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Linen.M_MN_Linen"));
		Assets.Water = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Water.M_MN_Water"));
		Assets.Miracle = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Miracle.M_MN_Miracle"));
		Assets.Oracle = LoadAsset<UMaterialInterface>(TEXT("/Game/Materials/M_MN_Oracle.M_MN_Oracle"));
		return Assets;
	}

	bool ValidateAssets(const FManyNamesBuildAssets& Assets)
	{
		return Assets.Plane && Assets.Cube && Assets.Cylinder && Assets.Cone && Assets.Sphere &&
			Assets.Manny && Assets.Quinn && Assets.IdleAnimation && Assets.AshStone && Assets.WreckMetal &&
			Assets.SandStone && Assets.Plaster && Assets.Basalt && Assets.Bronze && Assets.Linen &&
			Assets.Water && Assets.Miracle && Assets.Oracle;
	}

	UStaticMesh* PickMesh(UStaticMesh* PreferredMesh, UStaticMesh* FallbackMesh)
	{
		return PreferredMesh ? PreferredMesh : FallbackMesh;
	}

	FManyNamesNpcVisualProfile MakeNpcProfile(
		const FGameplayTag& RoleTag,
		USkeletalMesh* SkeletalMesh,
		UAnimationAsset* IdleAnimation,
		UStaticMesh* FallbackStaticMesh = nullptr,
		const FVector& RelativeScale = FVector(1.0f, 1.0f, 1.0f),
		const FVector& RelativeLocation = FVector(0.0f, 0.0f, -88.0f))
	{
		FManyNamesNpcVisualProfile Profile;
		Profile.RoleTag = RoleTag;
		Profile.SkeletalMesh = SkeletalMesh;
		Profile.IdleAnimation = IdleAnimation;
		Profile.FallbackStaticMesh = FallbackStaticMesh;
		Profile.RelativeScale = RelativeScale;
		Profile.RelativeLocation = RelativeLocation;
		return Profile;
	}

	FManyNamesRegionArtProfile BuildRegionArtProfile(EManyNamesRegionId RegionId, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		FManyNamesRegionArtProfile Profile;
		Profile.RegionId = RegionId;
		Profile.PopulationRoleTags = { RoleTags.NamedInteractable, RoleTags.NamedScenic, RoleTags.AmbientGuard, RoleTags.AmbientCivilian, RoleTags.AmbientRitual };

		switch (RegionId)
		{
		case EManyNamesRegionId::Opening:
			Profile.SunIntensity = 9.2f;
			Profile.SkyIntensity = 1.0f;
			Profile.FogDensity = 0.012f;
			Profile.KeyLightTint = FLinearColor(1.0f, 0.94f, 0.85f, 1.0f);
			Profile.FogTint = FLinearColor(0.75f, 0.68f, 0.60f, 1.0f);
			Profile.StyleNotes = TEXT("Crash ravine with dusty firelight, scorched metal, and a hard travel palette.");
			AddAssetPath(Profile.StructuralMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.StructuralMeshPaths, Assets.HighDetailRock);
			AddAssetPath(Profile.PropMeshPaths, Assets.Rock001);
			break;
		case EManyNamesRegionId::Egypt:
			Profile.SunIntensity = 10.5f;
			Profile.SkyIntensity = 1.35f;
			Profile.FogDensity = 0.009f;
			Profile.KeyLightTint = FLinearColor(1.0f, 0.93f, 0.80f, 1.0f);
			Profile.FogTint = FLinearColor(0.95f, 0.85f, 0.70f, 1.0f);
			Profile.StyleNotes = TEXT("Layered sacred masonry, warm dust, solar authority, and controlled ceremonial density.");
			AddAssetPath(Profile.StructuralMeshPaths, Assets.EgyptWallA);
			AddAssetPath(Profile.StructuralMeshPaths, Assets.EgyptColumn);
			AddAssetPath(Profile.StructuralMeshPaths, Assets.MastabaEntrance);
			AddAssetPath(Profile.PropMeshPaths, Assets.AncientCarvedStone);
			AddAssetPath(Profile.PropMeshPaths, Assets.PyramidStone);
			break;
		case EManyNamesRegionId::Greece:
			Profile.SunIntensity = 9.8f;
			Profile.SkyIntensity = 1.45f;
			Profile.FogDensity = 0.007f;
			Profile.KeyLightTint = FLinearColor(0.97f, 0.95f, 0.90f, 1.0f);
			Profile.FogTint = FLinearColor(0.78f, 0.83f, 0.88f, 1.0f);
			Profile.StyleNotes = TEXT("Bright sky exposure, cliff silhouettes, ritual stone, banners, and storm spectacle.");
			AddAssetPath(Profile.StructuralMeshPaths, Assets.GreeceDolmen);
			AddAssetPath(Profile.StructuralMeshPaths, Assets.HighDetailRock);
			AddAssetPath(Profile.PropMeshPaths, Assets.AncientCarvedStone);
			break;
		case EManyNamesRegionId::ItalicWest:
			Profile.SunIntensity = 8.7f;
			Profile.SkyIntensity = 1.1f;
			Profile.FogDensity = 0.011f;
			Profile.KeyLightTint = FLinearColor(0.98f, 0.91f, 0.82f, 1.0f);
			Profile.FogTint = FLinearColor(0.72f, 0.69f, 0.62f, 1.0f);
			Profile.StyleNotes = TEXT("Timber-stone pragmatism, boundary markers, civic repetition, road discipline, and forge fire.");
			AddAssetPath(Profile.StructuralMeshPaths, Assets.ItalicChurchRock);
			AddAssetPath(Profile.PropMeshPaths, Assets.ItalicBollard);
			AddAssetPath(Profile.PropMeshPaths, Assets.ConvergenceDestroyedWood);
			break;
		case EManyNamesRegionId::Convergence:
		default:
			Profile.SunIntensity = 6.2f;
			Profile.SkyIntensity = 0.9f;
			Profile.FogDensity = 0.016f;
			Profile.KeyLightTint = FLinearColor(0.80f, 0.88f, 0.95f, 1.0f);
			Profile.FogTint = FLinearColor(0.35f, 0.42f, 0.50f, 1.0f);
			Profile.StyleNotes = TEXT("Buried ruin shell, severe descent core, stripped archaeological sci-fi, and cold reflected light.");
			AddAssetPath(Profile.StructuralMeshPaths, Assets.ConvergenceDestroyedWood);
			AddAssetPath(Profile.StructuralMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.PropMeshPaths, Assets.PyramidStone);
			break;
		}

		return Profile;
	}

	void ClearWorld(UWorld* World)
	{
		TArray<AActor*> ActorsToDelete;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || Actor->IsA<AWorldSettings>() || Actor->IsA<ABrush>())
			{
				continue;
			}
			ActorsToDelete.Add(Actor);
		}

		for (AActor* Actor : ActorsToDelete)
		{
			World->EditorDestroyActor(Actor, true);
		}

		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->bForceNoPrecomputedLighting = true;
		}
	}

	UWorld* LoadOrCreateMap(const FString& MapPath)
	{
		if (FPackageName::DoesPackageExist(MapPath))
		{
			return UEditorLoadingAndSavingUtils::LoadMap(MapPath);
		}

		return UEditorLoadingAndSavingUtils::NewBlankMap(false);
	}

	AManyNamesScenicActor* SpawnScenic(
		UWorld* World,
		const FString& Label,
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		UMaterialInterface* Material = nullptr,
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		if (!World || !Mesh)
		{
			return nullptr;
		}

		AManyNamesScenicActor* Actor = World->SpawnActor<AManyNamesScenicActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Label);
		Actor->SetActorScale3D(Scale);
		if (UStaticMeshComponent* StaticMeshComponent = Actor->GetStaticMeshComponent())
		{
			StaticMeshComponent->SetStaticMesh(Mesh);
			ApplyMaterialToAllSlots(StaticMeshComponent, Material);
		}
		if (USkeletalMeshComponent* SkeletalMeshComponent = Actor->GetSkeletalMeshComponent())
		{
			SkeletalMeshComponent->SetVisibility(false);
		}
		return Actor;
	}

	AManyNamesScenicActor* SpawnNpc(UWorld* World, const FString& Label, const FManyNamesNpcVisualProfile& Profile, const FVector& Location, float Yaw)
	{
		if (!World)
		{
			return nullptr;
		}

		AManyNamesScenicActor* Actor = World->SpawnActor<AManyNamesScenicActor>(Location, FRotator(0.0f, Yaw, 0.0f));
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Label);
		Actor->SetNpcVisualProfile(Profile);
		return Actor;
	}

	AManyNamesInteractableActor* SpawnInteractableNpc(
		UWorld* World,
		const FString& Label,
		const FText& InteractionLabel,
		const FManyNamesNpcVisualProfile& Profile,
		const FVector& Location,
		float Yaw,
		EManyNamesInteractionActionType InteractionType,
		FName QuestId,
		const TArray<FName>& RequiredOutputs,
		bool bSingleUse)
	{
		if (!World)
		{
			return nullptr;
		}

		AManyNamesInteractableActor* Actor = World->SpawnActor<AManyNamesInteractableActor>(Location, FRotator(0.0f, Yaw, 0.0f));
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Label);
		Actor->SetInteractionLabel(InteractionLabel);
		Actor->SetInteractionType(InteractionType);
		Actor->SetQuestId(QuestId);
		Actor->SetRequiredOutputs(RequiredOutputs);
		Actor->SetSingleUse(bSingleUse);
		Actor->SetNpcVisualProfile(Profile);
		return Actor;
	}

	AManyNamesInteractableActor* SpawnTravelGate(
		UWorld* World,
		const FString& Label,
		const FText& InteractionLabel,
		const FVector& Location,
		const FVector& Scale,
		EManyNamesRegionId RegionId,
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		const TArray<FName>& RequiredOutputs)
	{
		if (!World || !Mesh)
		{
			return nullptr;
		}

		AManyNamesInteractableActor* Actor = World->SpawnActor<AManyNamesInteractableActor>(Location, FRotator::ZeroRotator);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Label);
		Actor->SetInteractionLabel(InteractionLabel);
		Actor->SetInteractionType(EManyNamesInteractionActionType::RegionTravel);
		Actor->SetTargetRegionId(RegionId);
		Actor->SetRequiredOutputs(RequiredOutputs);
		Actor->SetSingleUse(false);
		if (UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent())
		{
			MeshComponent->SetStaticMesh(Mesh);
			MeshComponent->SetRelativeScale3D(Scale);
			ApplyMaterialToAllSlots(MeshComponent, Material);
		}
		return Actor;
	}

	void SpawnPointLight(UWorld* World, const FString& Label, const FVector& Location, float Intensity, float Radius)
	{
		if (!World)
		{
			return;
		}

		APointLight* PointLight = World->SpawnActor<APointLight>(Location, FRotator::ZeroRotator);
		if (!PointLight)
		{
			return;
		}

		PointLight->SetActorLabel(Label);
		if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(PointLight->GetLightComponent()))
		{
			PointLightComponent->SetMobility(EComponentMobility::Movable);
			PointLightComponent->SetIntensity(Intensity);
			PointLightComponent->AttenuationRadius = Radius;
		}
	}

	void SpawnSky(UWorld* World, const FManyNamesRegionArtProfile& ArtProfile, const FRotator& SunRotation)
	{
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 1400.0f), SunRotation);
		if (Sun)
		{
			Sun->SetActorLabel(TEXT("SunLight"));
			if (UDirectionalLightComponent* DirectionalLightComponent = Sun->GetComponent())
			{
				DirectionalLightComponent->SetMobility(EComponentMobility::Movable);
				DirectionalLightComponent->SetIntensity(ArtProfile.SunIntensity);
				DirectionalLightComponent->SetLightColor(ArtProfile.KeyLightTint.ToFColor(true));
			}
		}

		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 240.0f), FRotator::ZeroRotator);
		if (SkyLight)
		{
			SkyLight->SetActorLabel(TEXT("SkyLight"));
			if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
			{
				SkyLightComponent->SetMobility(EComponentMobility::Movable);
				SkyLightComponent->SetIntensity(ArtProfile.SkyIntensity);
			}
		}

		AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (Fog)
		{
			Fog->SetActorLabel(TEXT("HeightFog"));
			if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
			{
				FogComponent->FogDensity = ArtProfile.FogDensity;
				FogComponent->FogHeightFalloff = 0.2f;
				FogComponent->SetFogInscatteringColor(ArtProfile.FogTint);
			}
		}

		ASkyAtmosphere* Atmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (Atmosphere)
		{
			Atmosphere->SetActorLabel(TEXT("SkyAtmosphere"));
		}
	}

	void SpawnPlayerStart(UWorld* World, const FVector& Location, float Yaw)
	{
		if (!World)
		{
			return;
		}

		APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(Location, FRotator(0.0f, Yaw, 0.0f));
		if (PlayerStart)
		{
			PlayerStart->SetActorLabel(TEXT("PlayerStart"));
		}
	}

	void BuildOpeningMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Opening, Assets, RoleTags);
		SpawnSky(World, ArtProfile, FRotator(-34.0f, -18.0f, 0.0f));

		SpawnScenic(World, TEXT("OpeningGround"), Assets.Plane, FVector(0.0f, 0.0f, -25.0f), FVector(72.0f, 52.0f, 1.0f), Assets.AshStone);
		SpawnScenic(World, TEXT("OpeningRidgeNorth"), PickMesh(Assets.ItalicChurchRock, Assets.CaveRock), FVector(-4400.0f, 2500.0f, 620.0f), FVector(5.5f, 5.5f, 5.5f), nullptr, FRotator(0.0f, -20.0f, 0.0f));
		SpawnScenic(World, TEXT("OpeningRidgeSouth"), PickMesh(Assets.HighDetailRock, Assets.Cube), FVector(-1700.0f, -2600.0f, 460.0f), FVector(7.0f, 7.0f, 7.0f), nullptr, FRotator(0.0f, 45.0f, 0.0f));
		SpawnScenic(World, TEXT("OpeningCliffEast"), PickMesh(Assets.CaveRock, Assets.Cube), FVector(4100.0f, 2100.0f, 540.0f), FVector(8.0f, 8.0f, 8.0f), nullptr, FRotator(0.0f, 105.0f, 0.0f));
		SpawnScenic(World, TEXT("OpeningCliffWest"), PickMesh(Assets.RealisticRock, Assets.Cube), FVector(-5200.0f, -2200.0f, 500.0f), FVector(9.0f, 9.0f, 9.0f), nullptr);
		SpawnScenic(World, TEXT("CrashTrench"), Assets.Cube, FVector(250.0f, 0.0f, -90.0f), FVector(10.0f, 2.5f, 0.9f), Assets.WreckMetal, FRotator(0.0f, 20.0f, -8.0f));
		SpawnScenic(World, TEXT("ImpactShard_A"), Assets.Cube, FVector(980.0f, -180.0f, 140.0f), FVector(2.6f, 1.1f, 4.5f), Assets.WreckMetal, FRotator(22.0f, 18.0f, 38.0f));
		SpawnScenic(World, TEXT("ImpactShard_B"), Assets.Cube, FVector(-180.0f, 320.0f, 110.0f), FVector(2.3f, 1.4f, 3.2f), Assets.WreckMetal, FRotator(-14.0f, -18.0f, -26.0f));
		SpawnScenic(World, TEXT("CampBase"), Assets.Cube, FVector(-2000.0f, 920.0f, 50.0f), FVector(2.4f, 1.6f, 0.25f), Assets.Plaster);
		SpawnScenic(World, TEXT("CampTarp"), Assets.Plane, FVector(-2000.0f, 920.0f, 300.0f), FVector(2.6f, 1.6f, 1.0f), Assets.Linen, FRotator(12.0f, 0.0f, 0.0f));
		SpawnScenic(World, TEXT("RouteOverlookMarker"), PickMesh(Assets.AncientCarvedStone, Assets.Cube), FVector(-3100.0f, 180.0f, 180.0f), FVector(2.2f, 2.2f, 2.2f), nullptr);
		SpawnScenic(World, TEXT("MiracleCore"), Assets.Sphere, FVector(250.0f, 0.0f, 130.0f), FVector(0.55f, 0.55f, 0.55f), Assets.Miracle);

		for (int32 Index = 0; Index < 7; ++Index)
		{
			const float X = -5600.0f + (Index * 1800.0f);
			const float Y = (Index % 2 == 0) ? -2900.0f : 2900.0f;
			UStaticMesh* RockMesh = (Index % 3 == 0) ? PickMesh(Assets.CaveRock, Assets.Cube) : ((Index % 3 == 1) ? PickMesh(Assets.HighDetailRock, Assets.Cube) : PickMesh(Assets.Rock001, Assets.Cube));
			SpawnScenic(World, FString::Printf(TEXT("OpeningRock_%d"), Index + 1), RockMesh, FVector(X, Y, 360.0f + (Index * 18.0f)), FVector(3.2f, 3.2f, 3.2f), nullptr, FRotator(0.0f, Index * 18.0f, 0.0f));
		}

		SpawnPointLight(World, TEXT("MiracleGlow"), FVector(250.0f, 0.0f, 160.0f), 9600.0f, 2600.0f);
		SpawnPointLight(World, TEXT("CampFire"), FVector(-2020.0f, 870.0f, 70.0f), 4200.0f, 1800.0f);
		SpawnPlayerStart(World, FVector(-5200.0f, -120.0f, 180.0f), 5.0f);

		AManyNamesInteractableActor* MiracleAnchor = World->SpawnActor<AManyNamesInteractableActor>(FVector(250.0f, 0.0f, 120.0f), FRotator::ZeroRotator);
		if (MiracleAnchor)
		{
			MiracleAnchor->SetActorLabel(TEXT("FirstMiracleAnchor"));
			MiracleAnchor->SetInteractionLabel(FText::FromString(TEXT("Trigger the first miracle")));
			MiracleAnchor->SetInteractionType(EManyNamesInteractionActionType::FirstMiracle);
			MiracleAnchor->SetSingleUse(true);
			if (UStaticMeshComponent* MeshComponent = MiracleAnchor->GetStaticMeshComponent())
			{
				MeshComponent->SetStaticMesh(Assets.Cube);
				MeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 2.0f));
				ApplyMaterialToAllSlots(MeshComponent, Assets.Miracle);
			}
		}

		SpawnInteractableNpc(
			World,
			TEXT("WitnessAnchor"),
			FText::FromString(TEXT("Speak to the witness")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Quinn, Assets.IdleAnimation),
			FVector(-1860.0f, 960.0f, 88.0f),
			-35.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("opening_side_01"),
			{ TEXT("Story.Prologue.Complete") },
			true);

		SpawnNpc(World, TEXT("WitnessAttendant"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Manny, Assets.IdleAnimation), FVector(-2300.0f, 1200.0f, 88.0f), 35.0f);
		SpawnNpc(World, TEXT("FallenCrew_Manny"), MakeNpcProfile(RoleTags.NamedScenic, Assets.Manny, Assets.DeathBack), FVector(-420.0f, -520.0f, 88.0f), 65.0f);
		SpawnNpc(World, TEXT("FallenCrew_Quinn"), MakeNpcProfile(RoleTags.NamedScenic, Assets.Quinn, Assets.DeathLeft), FVector(-760.0f, 680.0f, 88.0f), -55.0f);
		if (Assets.HumanFryPose)
		{
			SpawnNpc(World, TEXT("FallenCrew_Remnant"), MakeNpcProfile(RoleTags.NamedScenic, Assets.HumanFryPose, nullptr), FVector(420.0f, -760.0f, 88.0f), 20.0f);
		}

		SpawnTravelGate(
			World,
			TEXT("EgyptGate"),
			FText::FromString(TEXT("Travel to Egypt")),
			FVector(3200.0f, -220.0f, 150.0f),
			FVector(1.8f, 1.3f, 2.8f),
			EManyNamesRegionId::Egypt,
			PickMesh(Assets.MastabaEntrance, Assets.Cube),
			Assets.Bronze,
			{ TEXT("State.Region.Opening.Complete") });
		SpawnTravelGate(
			World,
			TEXT("GreeceGate"),
			FText::FromString(TEXT("Travel to Greece")),
			FVector(3500.0f, 820.0f, 160.0f),
			FVector(1.6f, 1.6f, 1.6f),
			EManyNamesRegionId::Greece,
			PickMesh(Assets.GreeceDolmen, Assets.Cube),
			nullptr,
			{ TEXT("State.Region.Opening.Complete") });
		SpawnTravelGate(
			World,
			TEXT("ItalicGate"),
			FText::FromString(TEXT("Travel to Italic West")),
			FVector(3300.0f, -1180.0f, 170.0f),
			FVector(2.0f, 2.0f, 2.0f),
			EManyNamesRegionId::ItalicWest,
			PickMesh(Assets.ItalicBollard, Assets.Cube),
			nullptr,
			{ TEXT("State.Region.Opening.Complete") });
	}

	void BuildEgyptMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Egypt, Assets, RoleTags);
		SpawnSky(World, ArtProfile, FRotator(-38.0f, -28.0f, 0.0f));

		SpawnScenic(World, TEXT("EgyptGround"), Assets.Plane, FVector(0.0f, 0.0f, -20.0f), FVector(120.0f, 82.0f, 1.0f), Assets.SandStone);
		SpawnScenic(World, TEXT("TempleDais"), PickMesh(Assets.EgyptFloor, Assets.Cube), FVector(0.0f, -220.0f, 110.0f), FVector(6.0f, 6.0f, 3.0f), nullptr);
		SpawnScenic(World, TEXT("TempleStairs"), PickMesh(Assets.EgyptStairs, Assets.Cube), FVector(0.0f, -820.0f, 90.0f), FVector(2.8f, 2.8f, 2.8f), nullptr);
		SpawnScenic(World, TEXT("ArchiveFacade"), PickMesh(Assets.MastabaEntrance, Assets.Cube), FVector(2800.0f, 0.0f, 320.0f), FVector(2.4f, 2.4f, 2.4f), nullptr, FRotator(0.0f, 90.0f, 0.0f));
		SpawnScenic(World, TEXT("BentPyramidVista"), PickMesh(Assets.BentPyramid, Assets.Cube), FVector(4200.0f, 2800.0f, 420.0f), FVector(2.6f, 2.6f, 2.6f), nullptr, FRotator(0.0f, -45.0f, 0.0f));
		SpawnScenic(World, TEXT("TemplePool"), Assets.Plane, FVector(0.0f, -360.0f, 30.0f), FVector(4.0f, 2.0f, 1.0f), Assets.Water);
		SpawnScenic(World, TEXT("NecropolisRoad"), Assets.Cube, FVector(0.0f, 3320.0f, 40.0f), FVector(5.4f, 13.0f, 0.2f), Assets.Basalt);
		SpawnScenic(World, TEXT("NecropolisGate"), PickMesh(Assets.MastabaEntrance, Assets.Cube), FVector(0.0f, 4400.0f, 200.0f), FVector(1.7f, 1.7f, 1.7f), nullptr);

		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float X = -1250.0f + (Index * 830.0f);
			SpawnScenic(World, FString::Printf(TEXT("TempleColumn_%d"), Index + 1), PickMesh(Assets.EgyptColumn, Assets.Cylinder), FVector(X, -280.0f, 350.0f), FVector(1.8f, 1.8f, 4.8f), nullptr);
		}

		for (int32 Index = 0; Index < 5; ++Index)
		{
			const float Y = -1650.0f + (Index * 820.0f);
			SpawnScenic(World, FString::Printf(TEXT("MarketCanopy_%d"), Index + 1), Assets.Plane, FVector(-2800.0f, Y, 260.0f), FVector(3.1f, 2.2f, 1.0f), Assets.Linen, FRotator(7.0f, 0.0f, 0.0f));
			SpawnScenic(World, FString::Printf(TEXT("MarketTable_%d"), Index + 1), Assets.Cube, FVector(-2800.0f, Y, 72.0f), FVector(1.5f, 0.8f, 0.35f), Assets.Plaster);
			SpawnScenic(World, FString::Printf(TEXT("MarketStone_%d"), Index + 1), PickMesh(Assets.AncientCarvedStone, Assets.Cube), FVector(-3200.0f, Y + 130.0f, 40.0f), FVector(1.1f, 1.1f, 1.1f), nullptr);
		}

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float X = -4600.0f + (Index * 1600.0f);
			const float Y = (Index % 2 == 0) ? -2600.0f : 2600.0f;
			UStaticMesh* RockMesh = (Index % 2 == 0) ? PickMesh(Assets.HighDetailRock, Assets.Cube) : PickMesh(Assets.CaveRock, Assets.Cube);
			SpawnScenic(World, FString::Printf(TEXT("EgyptCliff_%d"), Index + 1), RockMesh, FVector(X, Y, 480.0f), FVector(4.0f, 4.0f, 4.0f), nullptr, FRotator(0.0f, Index * 25.0f, 0.0f));
		}

		for (const FVector& Position : TArray<FVector>{
			FVector(-620.0f, 2880.0f, 230.0f),
			FVector(620.0f, 2880.0f, 230.0f),
			FVector(-620.0f, 3880.0f, 230.0f),
			FVector(620.0f, 3880.0f, 230.0f) })
		{
			SpawnScenic(World, TEXT("NecropolisMarker"), PickMesh(Assets.PyramidStone, Assets.Cylinder), Position, FVector(1.6f, 1.6f, 3.6f), nullptr);
		}

		for (const FVector& LightPosition : TArray<FVector>{
			FVector(-650.0f, -1150.0f, 100.0f),
			FVector(650.0f, -1150.0f, 100.0f),
			FVector(2100.0f, -700.0f, 88.0f),
			FVector(2100.0f, 700.0f, 88.0f),
			FVector(-3100.0f, -1700.0f, 88.0f),
			FVector(-3100.0f, 1700.0f, 88.0f) })
		{
			SpawnPointLight(World, TEXT("BrazierLight"), LightPosition + FVector(0.0f, 0.0f, 120.0f), 3600.0f, 1800.0f);
			SpawnScenic(World, TEXT("BrazierCore"), Assets.Sphere, LightPosition + FVector(0.0f, 0.0f, 80.0f), FVector(0.15f, 0.15f, 0.15f), Assets.Miracle);
		}

		SpawnPlayerStart(World, FVector(-4600.0f, -100.0f, 160.0f), 0.0f);

		SpawnInteractableNpc(
			World,
			TEXT("ArchiveKeeper"),
			FText::FromString(TEXT("Speak with the archive keeper")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Manny, Assets.IdleAnimation),
			FVector(1660.0f, 0.0f, 88.0f),
			90.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("egypt_main_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true);

		SpawnInteractableNpc(
			World,
			TEXT("FloodplainPetitioner"),
			FText::FromString(TEXT("Hear the floodplain petition")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Quinn, Assets.IdleAnimation),
			FVector(-2620.0f, -950.0f, 88.0f),
			20.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("egypt_side_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true);

		SpawnNpc(World, TEXT("TemplePriest_01"), MakeNpcProfile(RoleTags.AmbientRitual, Assets.Quinn, Assets.IdleAnimation), FVector(-360.0f, -520.0f, 88.0f), 20.0f);
		SpawnNpc(World, TEXT("TemplePriest_02"), MakeNpcProfile(RoleTags.AmbientRitual, Assets.Manny, Assets.IdleAnimation), FVector(420.0f, -640.0f, 88.0f), -25.0f);
		SpawnNpc(World, TEXT("ArchiveScribe_01"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Quinn, Assets.IdleAnimation), FVector(2220.0f, -280.0f, 88.0f), -90.0f);
		SpawnNpc(World, TEXT("ArchiveScribe_02"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Manny, Assets.IdleAnimation), FVector(2300.0f, 320.0f, 88.0f), -100.0f);
		SpawnNpc(World, TEXT("MarketVendor_01"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Quinn, Assets.IdleAnimation), FVector(-2500.0f, -1400.0f, 88.0f), 90.0f);
		SpawnNpc(World, TEXT("MarketVendor_02"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Manny, Assets.IdleAnimation), FVector(-2400.0f, 700.0f, 88.0f), -90.0f);
		SpawnNpc(World, TEXT("TempleGuard_01"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Manny, Assets.IdleAnimation), FVector(1050.0f, -860.0f, 88.0f), 180.0f);
		SpawnNpc(World, TEXT("TempleGuard_02"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Manny, Assets.IdleAnimation), FVector(1050.0f, 860.0f, 88.0f), 180.0f);
		SpawnNpc(World, TEXT("NecropolisWatcher"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Quinn, Assets.IdleAnimation), FVector(0.0f, 3650.0f, 88.0f), 180.0f);
		SpawnNpc(World, TEXT("Citizen_01"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Quinn, Assets.IdleAnimation), FVector(-1200.0f, 220.0f, 88.0f), 15.0f);
		SpawnNpc(World, TEXT("Citizen_02"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Manny, Assets.IdleAnimation), FVector(-900.0f, -40.0f, 88.0f), 120.0f);
	}

	void BuildGreeceMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Greece, Assets, RoleTags);
		SpawnSky(World, ArtProfile, FRotator(-32.0f, 42.0f, 0.0f));

		SpawnScenic(World, TEXT("GreeceGround"), Assets.Plane, FVector(0.0f, 0.0f, -20.0f), FVector(112.0f, 76.0f, 1.0f), Assets.Plaster);
		SpawnScenic(World, TEXT("SanctuaryCourt"), Assets.Cube, FVector(0.0f, -180.0f, 70.0f), FVector(10.0f, 7.0f, 0.8f), Assets.Plaster);
		SpawnScenic(World, TEXT("StormLandmark"), PickMesh(Assets.GreeceDolmen, Assets.Cone), FVector(0.0f, -1280.0f, 250.0f), FVector(3.0f, 3.0f, 3.0f), nullptr, FRotator(0.0f, 35.0f, 0.0f));
		SpawnScenic(World, TEXT("MountainRoute"), Assets.Cube, FVector(1800.0f, 1500.0f, 180.0f), FVector(16.0f, 2.4f, 0.3f), Assets.Basalt, FRotator(10.0f, 45.0f, 0.0f));
		SpawnScenic(World, TEXT("ShrineCourt"), Assets.Cube, FVector(2600.0f, 2200.0f, 260.0f), FVector(7.0f, 5.0f, 0.8f), Assets.Plaster);
		SpawnScenic(World, TEXT("BronzeBeacon"), Assets.Cylinder, FVector(320.0f, -1480.0f, 240.0f), FVector(0.4f, 0.4f, 4.5f), Assets.Bronze);

		for (int32 Index = 0; Index < 8; ++Index)
		{
			const float Angle = Index * 45.0f;
			const FVector Position = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * 900.0f, FMath::Sin(FMath::DegreesToRadians(Angle)) * 900.0f, 150.0f);
			SpawnScenic(World, FString::Printf(TEXT("SanctuaryStone_%d"), Index + 1), PickMesh(Assets.AncientCarvedStone, Assets.Cylinder), Position, FVector(1.3f, 1.3f, 2.6f), nullptr, FRotator(0.0f, Angle, 0.0f));
		}

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float X = -4200.0f + (Index * 1600.0f);
			const float Y = (Index % 2 == 0) ? -2600.0f : 2600.0f;
			UStaticMesh* RockMesh = (Index % 2 == 0) ? PickMesh(Assets.HighDetailRock, Assets.Cube) : PickMesh(Assets.RealisticRock, Assets.Cube);
			SpawnScenic(World, FString::Printf(TEXT("GreeceCliff_%d"), Index + 1), RockMesh, FVector(X, Y, 520.0f), FVector(4.6f, 4.6f, 4.6f), nullptr, FRotator(0.0f, 15.0f * Index, 0.0f));
		}

		for (int32 Index = 0; Index < 3; ++Index)
		{
			SpawnScenic(World, FString::Printf(TEXT("Banner_%d"), Index + 1), Assets.Plane, FVector(-600.0f + (Index * 600.0f), -980.0f, 320.0f), FVector(1.2f, 2.0f, 1.0f), Assets.Linen, FRotator(0.0f, 90.0f, 0.0f));
			SpawnPointLight(World, TEXT("SanctuaryFire"), FVector(-580.0f + (Index * 600.0f), -760.0f, 140.0f), 2800.0f, 1600.0f);
		}

		SpawnPlayerStart(World, FVector(-4400.0f, 0.0f, 190.0f), 15.0f);

		SpawnInteractableNpc(
			World,
			TEXT("StormHerald"),
			FText::FromString(TEXT("Confront the herald of the storm ruler")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Manny, Assets.IdleAnimation),
			FVector(180.0f, -420.0f, 88.0f),
			180.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("greece_main_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true);

		SpawnInteractableNpc(
			World,
			TEXT("WarbandEnvoy"),
			FText::FromString(TEXT("Answer the warband envoy")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Quinn, Assets.IdleAnimation),
			FVector(2700.0f, 2080.0f, 88.0f),
			-140.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("greece_side_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true);

		SpawnNpc(World, TEXT("RitualSinger_01"), MakeNpcProfile(RoleTags.AmbientRitual, Assets.Quinn, Assets.IdleAnimation), FVector(-420.0f, -360.0f, 88.0f), 110.0f);
		SpawnNpc(World, TEXT("RitualSinger_02"), MakeNpcProfile(RoleTags.AmbientRitual, Assets.Manny, Assets.IdleAnimation), FVector(420.0f, -350.0f, 88.0f), -110.0f);
		SpawnNpc(World, TEXT("Guard_Cliff"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Manny, Assets.IdleAnimation), FVector(2350.0f, 1860.0f, 88.0f), -90.0f);
		SpawnNpc(World, TEXT("Guard_Court"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Quinn, Assets.IdleAnimation), FVector(-1000.0f, 180.0f, 88.0f), 30.0f);
		SpawnNpc(World, TEXT("Pilgrim_01"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Manny, Assets.IdleAnimation), FVector(-1500.0f, -220.0f, 88.0f), 90.0f);
		SpawnNpc(World, TEXT("Pilgrim_02"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Quinn, Assets.IdleAnimation), FVector(1100.0f, 220.0f, 88.0f), -90.0f);
	}

	void BuildItalicMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::ItalicWest, Assets, RoleTags);
		SpawnSky(World, ArtProfile, FRotator(-30.0f, -12.0f, 0.0f));

		SpawnScenic(World, TEXT("ItalicGround"), Assets.Plane, FVector(0.0f, 0.0f, -20.0f), FVector(112.0f, 76.0f, 1.0f), Assets.AshStone);
		SpawnScenic(World, TEXT("HillSettlement"), Assets.Cube, FVector(-1200.0f, -600.0f, 120.0f), FVector(8.0f, 6.0f, 1.4f), Assets.Plaster);
		SpawnScenic(World, TEXT("ForgeLawChamber"), Assets.Cube, FVector(1800.0f, -200.0f, 220.0f), FVector(6.5f, 5.0f, 2.0f), Assets.Basalt);
		SpawnScenic(World, TEXT("RitualRoad"), Assets.Cube, FVector(250.0f, 1600.0f, 50.0f), FVector(17.0f, 2.0f, 0.2f), Assets.Basalt, FRotator(0.0f, 16.0f, 0.0f));
		SpawnScenic(World, TEXT("BoundaryFieldMarker"), PickMesh(Assets.ItalicBollard, Assets.Cylinder), FVector(2400.0f, 1500.0f, 120.0f), FVector(2.0f, 2.0f, 2.0f), nullptr);
		SpawnScenic(World, TEXT("PalisadeRidge"), PickMesh(Assets.ItalicChurchRock, Assets.Cube), FVector(-3800.0f, -2200.0f, 480.0f), FVector(5.4f, 5.4f, 5.4f), nullptr);
		SpawnScenic(World, TEXT("ForgeDebris"), PickMesh(Assets.ConvergenceDestroyedWood, Assets.Cube), FVector(1350.0f, 180.0f, 90.0f), FVector(2.6f, 2.6f, 2.6f), nullptr, FRotator(0.0f, 30.0f, 0.0f));

		for (int32 Index = 0; Index < 8; ++Index)
		{
			const float X = -2200.0f + (Index * 650.0f);
			SpawnScenic(World, FString::Printf(TEXT("RoadMarker_%d"), Index + 1), PickMesh(Assets.ItalicBollard, Assets.Cylinder), FVector(X, 1220.0f + ((Index % 2 == 0) ? 120.0f : -120.0f), 110.0f), FVector(1.6f, 1.6f, 1.6f), nullptr, FRotator(0.0f, Index * 12.0f, 0.0f));
		}

		for (int32 Index = 0; Index < 5; ++Index)
		{
			const float X = -3200.0f + (Index * 1550.0f);
			const float Y = (Index % 2 == 0) ? -2600.0f : 2400.0f;
			UStaticMesh* RockMesh = (Index % 2 == 0) ? PickMesh(Assets.ItalicChurchRock, Assets.Cube) : PickMesh(Assets.CaveRock, Assets.Cube);
			SpawnScenic(World, FString::Printf(TEXT("ItalicRock_%d"), Index + 1), RockMesh, FVector(X, Y, 480.0f), FVector(4.4f, 4.4f, 4.4f), nullptr, FRotator(0.0f, 10.0f * Index, 0.0f));
		}

		SpawnPointLight(World, TEXT("ForgeFire"), FVector(1600.0f, -40.0f, 160.0f), 4600.0f, 1800.0f);
		SpawnPointLight(World, TEXT("BoundaryFire"), FVector(2500.0f, 1580.0f, 160.0f), 3000.0f, 1400.0f);
		SpawnPlayerStart(World, FVector(-4600.0f, -100.0f, 190.0f), 0.0f);

		SpawnInteractableNpc(
			World,
			TEXT("MeasureKeeper"),
			FText::FromString(TEXT("Confront the keeper of measures")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Manny, Assets.IdleAnimation),
			FVector(1680.0f, -80.0f, 88.0f),
			90.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("italic_main_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true);

		SpawnInteractableNpc(
			World,
			TEXT("BoundaryElder"),
			FText::FromString(TEXT("Hear the boundary dispute")),
			MakeNpcProfile(RoleTags.NamedInteractable, Assets.Quinn, Assets.IdleAnimation),
			FVector(2420.0f, 1540.0f, 88.0f),
			-120.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("italic_side_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true);

		SpawnNpc(World, TEXT("ForgeGuard_01"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Manny, Assets.IdleAnimation), FVector(1080.0f, -420.0f, 88.0f), 45.0f);
		SpawnNpc(World, TEXT("ForgeGuard_02"), MakeNpcProfile(RoleTags.AmbientGuard, Assets.Quinn, Assets.IdleAnimation), FVector(1100.0f, 300.0f, 88.0f), -45.0f);
		SpawnNpc(World, TEXT("RoadWorker_01"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Manny, Assets.IdleAnimation), FVector(-640.0f, 1380.0f, 88.0f), 90.0f);
		SpawnNpc(World, TEXT("RoadWorker_02"), MakeNpcProfile(RoleTags.AmbientCivilian, Assets.Quinn, Assets.IdleAnimation), FVector(220.0f, 1760.0f, 88.0f), -90.0f);
		SpawnNpc(World, TEXT("RitualWitness"), MakeNpcProfile(RoleTags.AmbientRitual, Assets.Quinn, Assets.IdleAnimation), FVector(2080.0f, 1180.0f, 88.0f), 180.0f);
	}

	void BuildConvergenceMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Convergence, Assets, RoleTags);
		SpawnSky(World, ArtProfile, FRotator(-18.0f, 130.0f, 0.0f));

		SpawnScenic(World, TEXT("ConvergenceGround"), Assets.Plane, FVector(0.0f, 0.0f, -20.0f), FVector(90.0f, 64.0f, 1.0f), Assets.Basalt);
		SpawnScenic(World, TEXT("RuinShell"), PickMesh(Assets.MastabaEntrance, Assets.Cube), FVector(0.0f, -1200.0f, 280.0f), FVector(1.8f, 1.8f, 1.8f), nullptr, FRotator(0.0f, 180.0f, 0.0f));
		SpawnScenic(World, TEXT("DescentAccess"), Assets.Cube, FVector(0.0f, 0.0f, 70.0f), FVector(8.0f, 3.0f, 1.0f), Assets.WreckMetal);
		SpawnScenic(World, TEXT("BridgeChamber"), Assets.Cube, FVector(0.0f, 2200.0f, 210.0f), FVector(10.0f, 5.0f, 2.0f), Assets.WreckMetal);
		SpawnScenic(World, TEXT("BuriedCore"), Assets.Sphere, FVector(0.0f, 2400.0f, 240.0f), FVector(1.4f, 1.4f, 1.4f), Assets.Miracle);
		SpawnScenic(World, TEXT("CoreDebris_01"), PickMesh(Assets.ConvergenceDestroyedWood, Assets.Cube), FVector(-1200.0f, 820.0f, 120.0f), FVector(2.2f, 2.2f, 2.2f), nullptr, FRotator(0.0f, 35.0f, 0.0f));
		SpawnScenic(World, TEXT("CoreDebris_02"), PickMesh(Assets.ConvergenceDestroyedWood, Assets.Cube), FVector(1380.0f, 1180.0f, 120.0f), FVector(2.0f, 2.0f, 2.0f), nullptr, FRotator(0.0f, -20.0f, 0.0f));
		SpawnScenic(World, TEXT("BurialRock_A"), PickMesh(Assets.CaveRock, Assets.Cube), FVector(-2800.0f, -1800.0f, 540.0f), FVector(4.8f, 4.8f, 4.8f), nullptr);
		SpawnScenic(World, TEXT("BurialRock_B"), PickMesh(Assets.HighDetailRock, Assets.Cube), FVector(2800.0f, -1600.0f, 520.0f), FVector(4.6f, 4.6f, 4.6f), nullptr);
		SpawnScenic(World, TEXT("BurialRock_C"), PickMesh(Assets.Rock001, Assets.Cube), FVector(0.0f, 3600.0f, 480.0f), FVector(5.0f, 5.0f, 5.0f), nullptr);

		for (int32 Index = 0; Index < 4; ++Index)
		{
			SpawnPointLight(World, TEXT("CoreLight"), FVector(-900.0f + (Index * 600.0f), 1600.0f, 180.0f), 3200.0f, 1500.0f);
		}

		SpawnPlayerStart(World, FVector(-4200.0f, -120.0f, 200.0f), 0.0f);

		AManyNamesInteractableActor* CoreInterface = World->SpawnActor<AManyNamesInteractableActor>(FVector(0.0f, 2400.0f, 200.0f), FRotator::ZeroRotator);
		if (CoreInterface)
		{
			CoreInterface->SetActorLabel(TEXT("ConvergenceCoreInterface"));
			CoreInterface->SetInteractionLabel(FText::FromString(TEXT("Access the descent core")));
			CoreInterface->SetInteractionType(EManyNamesInteractionActionType::QuestDialogue);
			CoreInterface->SetQuestId(TEXT("convergence_main_01"));
			CoreInterface->SetRequiredOutputs({ TEXT("State.Region.Egypt.Complete"), TEXT("State.Region.Greece.Complete"), TEXT("State.Region.ItalicWest.Complete") });
			CoreInterface->SetSingleUse(true);
			if (UStaticMeshComponent* MeshComponent = CoreInterface->GetStaticMeshComponent())
			{
				MeshComponent->SetStaticMesh(Assets.Sphere);
				MeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 2.0f));
				ApplyMaterialToAllSlots(MeshComponent, Assets.Miracle);
			}
		}

		SpawnNpc(World, TEXT("BridgeWatcher"), MakeNpcProfile(RoleTags.NamedScenic, Assets.Manny, Assets.IdleAnimation), FVector(-460.0f, 2000.0f, 88.0f), 30.0f);
		SpawnNpc(World, TEXT("SystemsRemnant"), MakeNpcProfile(RoleTags.NamedScenic, Assets.Quinn, Assets.IdleAnimation), FVector(540.0f, 1950.0f, 88.0f), -30.0f);
	}

	bool SaveBuiltMap(UWorld* World, const FString& MapPath)
	{
		return World && UEditorLoadingAndSavingUtils::SaveMap(World, MapPath);
	}
}

UManyNamesWorldBuildCommandlet::UManyNamesWorldBuildCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UManyNamesWorldBuildCommandlet::Main(const FString& Params)
{
	const FManyNamesBuildAssets Assets = LoadBuildAssets();
	if (!ValidateAssets(Assets))
	{
		UE_LOG(LogTemp, Error, TEXT("ManyNames world build aborted: required base assets are missing."));
		return 1;
	}

	const FManyNamesRoleTags RoleTags = LoadRoleTags();

	UWorld* OpeningWorld = LoadOrCreateMap(TEXT("/Game/Maps/L_OpeningCatastrophe"));
	if (!OpeningWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load or create opening map."));
		return 1;
	}
	BuildOpeningMap(OpeningWorld, Assets, RoleTags);
	if (!SaveBuiltMap(OpeningWorld, TEXT("/Game/Maps/L_OpeningCatastrophe")))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save opening map."));
		return 1;
	}

	UWorld* EgyptWorld = LoadOrCreateMap(TEXT("/Game/Maps/L_EgyptHub"));
	if (!EgyptWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load or create Egypt map."));
		return 1;
	}
	BuildEgyptMap(EgyptWorld, Assets, RoleTags);
	if (!SaveBuiltMap(EgyptWorld, TEXT("/Game/Maps/L_EgyptHub")))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Egypt map."));
		return 1;
	}

	UWorld* GreeceWorld = LoadOrCreateMap(TEXT("/Game/Maps/L_GreeceHub"));
	if (!GreeceWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load or create Greece map."));
		return 1;
	}
	BuildGreeceMap(GreeceWorld, Assets, RoleTags);
	if (!SaveBuiltMap(GreeceWorld, TEXT("/Game/Maps/L_GreeceHub")))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Greece map."));
		return 1;
	}

	UWorld* ItalicWorld = LoadOrCreateMap(TEXT("/Game/Maps/L_ItalicHub"));
	if (!ItalicWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load or create Italic map."));
		return 1;
	}
	BuildItalicMap(ItalicWorld, Assets, RoleTags);
	if (!SaveBuiltMap(ItalicWorld, TEXT("/Game/Maps/L_ItalicHub")))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Italic map."));
		return 1;
	}

	UWorld* ConvergenceWorld = LoadOrCreateMap(TEXT("/Game/Maps/L_Convergence"));
	if (!ConvergenceWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load or create Convergence map."));
		return 1;
	}
	BuildConvergenceMap(ConvergenceWorld, Assets, RoleTags);
	if (!SaveBuiltMap(ConvergenceWorld, TEXT("/Game/Maps/L_Convergence")))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Convergence map."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("ManyNames world build completed for Opening, Egypt, Greece, Italic West, and Convergence."));
	return 0;
}
