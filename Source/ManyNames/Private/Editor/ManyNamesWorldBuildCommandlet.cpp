#include "Editor/ManyNamesWorldBuildCommandlet.h"

#include "Animation/AnimationAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Brush.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/HitResult.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "Gameplay/ManyNamesEnvironmentController.h"
#include "Gameplay/ManyNamesInteractableActor.h"
#include "Gameplay/ManyNamesScenicActor.h"
#include "GameplayTagsManager.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeConfigHelper.h"
#include "LandscapeDataAccess.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGVolume.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshEditorSubsystem.h"
#include "UObject/SavePackage.h"
#include "AssetToolsModule.h"

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

		USkeletalMesh* HumanFryPose = nullptr;
		USkeletalMesh* MetaHumanA = nullptr;
		USkeletalMesh* MetaHumanB = nullptr;
		USkeletalMesh* MetaHumanC = nullptr;
		TMap<FName, USkeletalMesh*> CharacterMeshesById;
		TMap<FName, USkeletalMesh*> AmbientMeshesByProfileId;

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

	struct FManyNamesSpawnValidationResult
	{
		bool bHasGround = false;
		FVector GroundLocation = FVector::ZeroVector;
	};

	template <typename AssetType>
	AssetType* LoadAsset(const TCHAR* AssetPath)
	{
		return LoadObject<AssetType>(nullptr, AssetPath);
	}

	template <typename AssetType>
	AssetType* TryLoadAsset(const TCHAR* AssetPath)
	{
		const FString FullPath(AssetPath);
		FString PackageName = FullPath;
		int32 DotIndex = INDEX_NONE;
		if (FullPath.FindChar(TEXT('.'), DotIndex))
		{
			PackageName = FullPath.Left(DotIndex);
		}

		if (PackageName.IsEmpty() || !FPackageName::DoesPackageExist(PackageName))
		{
			return nullptr;
		}

		return LoadAsset<AssetType>(AssetPath);
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

	void EnableNaniteIfSupported(UStaticMesh* Mesh)
	{
		if (!Mesh || Mesh->GetNaniteSettings().bEnabled)
		{
			return;
		}

		if (UStaticMeshEditorSubsystem* StaticMeshEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>() : nullptr)
		{
			FMeshNaniteSettings NaniteSettings = StaticMeshEditorSubsystem->GetNaniteSettings(Mesh);
			NaniteSettings.bEnabled = true;
			StaticMeshEditorSubsystem->SetNaniteSettings(Mesh, NaniteSettings, true);
		}
	}

	void AddAssetPath(TArray<FSoftObjectPath>& Paths, const UObject* Asset)
	{
		if (Asset)
		{
			Paths.Add(FSoftObjectPath(Asset->GetPathName()));
		}
	}

	void EnableNaniteForPaths(const TArray<FSoftObjectPath>& Paths)
	{
		for (const FSoftObjectPath& Path : Paths)
		{
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(Path.TryLoad()))
			{
				EnableNaniteIfSupported(Mesh);
			}
		}
	}

	void AppendSkeletalMeshesFromPath(const FString& PackagePath, TArray<USkeletalMesh*>& OutMeshes)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FARFilter Filter;
		Filter.PackagePaths.Add(*PackagePath);
		Filter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssets(Filter, AssetData);
		for (const FAssetData& Data : AssetData)
		{
			if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Data.GetAsset()))
			{
				OutMeshes.Add(Mesh);
			}
		}
	}

	void LoadMetaHumanManifest(FManyNamesBuildAssets& Assets)
	{
		const FString ManifestPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data/metahuman_manifest.json"));
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *ManifestPath))
		{
			return;
		}

		TSharedPtr<FJsonObject> RootObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
		{
			return;
		}

		const auto LoadMap = [&RootObject](const TCHAR* FieldName, TMap<FName, USkeletalMesh*>& TargetMap)
		{
			const TSharedPtr<FJsonObject>* ObjectField = nullptr;
			if (!RootObject->TryGetObjectField(FieldName, ObjectField) || !ObjectField || !ObjectField->IsValid())
			{
				return;
			}

			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*ObjectField)->Values)
			{
				FString AssetPath;
				if (!Pair.Value.IsValid() || !Pair.Value->TryGetString(AssetPath))
				{
					continue;
				}

				FString PackageName = AssetPath;
				int32 DotIndex = INDEX_NONE;
				if (AssetPath.FindChar(TEXT('.'), DotIndex))
				{
					PackageName = AssetPath.Left(DotIndex);
				}

				if (!PackageName.IsEmpty() && !FPackageName::DoesPackageExist(PackageName))
				{
					continue;
				}

				if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *AssetPath))
				{
					TargetMap.Add(FName(*Pair.Key), Mesh);
				}
			}
		};

		LoadMap(TEXT("named"), Assets.CharacterMeshesById);
		LoadMap(TEXT("ambient"), Assets.AmbientMeshesByProfileId);
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
		Assets.HumanFryPose = LoadAsset<USkeletalMesh>(TEXT("/Game/Marketplace/Fab/ArtifactsHero/HumanFryPose/AS.AS"));

		TArray<USkeletalMesh*> MetaHumans;
		AppendSkeletalMeshesFromPath(TEXT("/Game/MetaHumans"), MetaHumans);
		if (MetaHumans.Num() > 0)
		{
			Assets.MetaHumanA = MetaHumans[0];
			Assets.MetaHumanB = MetaHumans.Num() > 1 ? MetaHumans[1] : MetaHumans[0];
			Assets.MetaHumanC = MetaHumans.Num() > 2 ? MetaHumans[2] : MetaHumans.Last();
		}
		Assets.IdleAnimation = TryLoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
		Assets.DeathBack = TryLoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
		Assets.DeathLeft = TryLoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01"));
		Assets.DeathRight = TryLoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01"));

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
		LoadMetaHumanManifest(Assets);
		return Assets;
	}

	bool ValidateAssets(const FManyNamesBuildAssets& Assets)
	{
		return Assets.Plane && Assets.Cube && Assets.Cylinder && Assets.Cone && Assets.Sphere &&
			(Assets.MetaHumanA || Assets.HumanFryPose || Assets.CharacterMeshesById.Num() > 0) &&
			Assets.AshStone && Assets.WreckMetal &&
			Assets.SandStone && Assets.Plaster && Assets.Basalt && Assets.Bronze && Assets.Linen &&
			Assets.Water && Assets.Miracle && Assets.Oracle;
	}

	UStaticMesh* PickMesh(UStaticMesh* PreferredMesh, UStaticMesh* FallbackMesh)
	{
		return PreferredMesh ? PreferredMesh : FallbackMesh;
	}

	USkeletalMesh* ResolveNamedCharacter(const FManyNamesBuildAssets& Assets, FName CharacterId, int32 FallbackSlot)
	{
		if (const USkeletalMesh* const* Found = Assets.CharacterMeshesById.Find(CharacterId))
		{
			return const_cast<USkeletalMesh*>(*Found);
		}

		switch (FallbackSlot)
		{
		case 0:
			return FirstValid<USkeletalMesh>({Assets.MetaHumanA, Assets.MetaHumanB, Assets.HumanFryPose});
		case 1:
			return FirstValid<USkeletalMesh>({Assets.MetaHumanB, Assets.MetaHumanA, Assets.MetaHumanC, Assets.HumanFryPose});
		default:
			return FirstValid<USkeletalMesh>({Assets.MetaHumanC, Assets.MetaHumanA, Assets.MetaHumanB, Assets.HumanFryPose});
		}
	}

	USkeletalMesh* ResolveAmbientCharacter(const FManyNamesBuildAssets& Assets, FName ProfileId, int32 FallbackSlot)
	{
		if (const USkeletalMesh* const* Found = Assets.AmbientMeshesByProfileId.Find(ProfileId))
		{
			return const_cast<USkeletalMesh*>(*Found);
		}

		return (FallbackSlot % 2 == 0)
			? FirstValid<USkeletalMesh>({Assets.MetaHumanB, Assets.MetaHumanA, Assets.HumanFryPose, Assets.MetaHumanC})
			: FirstValid<USkeletalMesh>({Assets.MetaHumanA, Assets.MetaHumanB, Assets.HumanFryPose, Assets.MetaHumanC});
	}

	FManyNamesNpcVisualProfile MakeNpcProfile(
		const FGameplayTag& RoleTag,
		USkeletalMesh* SkeletalMesh,
		UAnimationAsset* IdleAnimation,
		UStaticMesh* FallbackStaticMesh = nullptr,
		const FVector& RelativeScale = FVector(1.0f, 1.0f, 1.0f),
		const FVector& RelativeLocation = FVector(0.0f, 0.0f, -88.0f),
		bool bPreferMetaHuman = false,
		FName ProfileId = NAME_None,
		FName CameraAnchorTag = NAME_None,
		FName CharacterId = NAME_None,
		FName ClothingVariantId = NAME_None,
		FName StanceId = NAME_None,
		EManyNamesCrowdBehaviorTier CrowdBehaviorTier = EManyNamesCrowdBehaviorTier::StaticScenic,
		bool bEnableClothSimulation = false)
	{
		FManyNamesNpcVisualProfile Profile;
		Profile.ProfileId = ProfileId;
		Profile.RoleTag = RoleTag;
		Profile.CharacterId = CharacterId.IsNone() ? ProfileId : CharacterId;
		Profile.bPreferMetaHuman = bPreferMetaHuman;
		Profile.SkeletalMesh = SkeletalMesh;
		Profile.IdleAnimation = IdleAnimation;
		Profile.FallbackStaticMesh = FallbackStaticMesh;
		Profile.RelativeScale = RelativeScale;
		Profile.RelativeLocation = RelativeLocation;
		Profile.CameraAnchorTag = CameraAnchorTag;
		Profile.ClothingVariantId = ClothingVariantId;
		Profile.StanceId = StanceId;
		Profile.CrowdBehaviorTier = CrowdBehaviorTier;
		Profile.bEnableClothSimulation = bEnableClothSimulation;
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
			Profile.BaselineWeatherStateId = TEXT("Opening.Baseline");
			Profile.HeroWeatherStateId = TEXT("Opening.Hero");
			Profile.HubPopulationDensity = 0.8f;
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
			Profile.BaselineWeatherStateId = TEXT("Egypt.Baseline");
			Profile.HeroWeatherStateId = TEXT("Egypt.Hero");
			Profile.HubPopulationDensity = 1.35f;
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
			Profile.BaselineWeatherStateId = TEXT("Greece.Baseline");
			Profile.HeroWeatherStateId = TEXT("Greece.Hero");
			Profile.HubPopulationDensity = 1.15f;
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
			Profile.BaselineWeatherStateId = TEXT("ItalicWest.Baseline");
			Profile.HeroWeatherStateId = TEXT("ItalicWest.Hero");
			Profile.HubPopulationDensity = 1.1f;
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
			Profile.BaselineWeatherStateId = TEXT("Convergence.Baseline");
			Profile.HeroWeatherStateId = TEXT("Convergence.Hero");
			Profile.HubPopulationDensity = 0.55f;
			AddAssetPath(Profile.StructuralMeshPaths, Assets.ConvergenceDestroyedWood);
			AddAssetPath(Profile.StructuralMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.PropMeshPaths, Assets.PyramidStone);
			break;
		}

		return Profile;
	}

	FManyNamesEnvironmentProfile BuildEnvironmentProfile(EManyNamesRegionId RegionId)
	{
		auto MakeWeatherState = [](
			FName StateId,
			float SunPitch,
			float SunYaw,
			float SunIntensity,
			float SkyIntensity,
			float FogDensity,
			const FLinearColor& KeyTint,
			const FLinearColor& FogTint,
			float WindIntensity,
			EManyNamesWeatherPrecipitation Precipitation,
			float PrecipitationIntensity,
			float GroundWetness,
			float GroundSnow,
			float TraversalSpeedMultiplier,
			bool bAffectsTraversal)
		{
			FManyNamesWeatherState State;
			State.StateId = StateId;
			State.SunPitch = SunPitch;
			State.SunYaw = SunYaw;
			State.SunIntensity = SunIntensity;
			State.SkyIntensity = SkyIntensity;
			State.FogDensity = FogDensity;
			State.KeyLightTint = KeyTint;
			State.FogTint = FogTint;
			State.WindIntensity = WindIntensity;
			State.Precipitation = Precipitation;
			State.PrecipitationIntensity = PrecipitationIntensity;
			State.GroundWetness = GroundWetness;
			State.GroundSnow = GroundSnow;
			State.TraversalSpeedMultiplier = TraversalSpeedMultiplier;
			State.bAffectsTraversal = bAffectsTraversal;
			return State;
		};

		FManyNamesEnvironmentProfile Profile;
		Profile.RegionId = RegionId;
		Profile.PreferredRenderPath = EManyNamesRenderPath::Auto;
		Profile.bAllowHeterogeneousVolumes = true;
		Profile.bAllowNaniteAssemblies = true;
		Profile.bAllowNaniteSkinnedMeshes = true;
		Profile.bAllowNaniteVoxels = true;

		switch (RegionId)
		{
		case EManyNamesRegionId::Opening:
			Profile.bAllowMegaLights = true;
			Profile.bPreferMegaLightLocalLights = true;
			Profile.BaselineState = MakeWeatherState(TEXT("Opening.Baseline"), -34.0f, -18.0f, 9.2f, 1.0f, 0.012f, FLinearColor(1.0f, 0.94f, 0.85f, 1.0f), FLinearColor(0.75f, 0.68f, 0.60f, 1.0f), 0.35f, EManyNamesWeatherPrecipitation::Dust, 0.2f, 0.0f, 0.0f, 1.0f, false);
			Profile.HeroState = MakeWeatherState(TEXT("Opening.Hero"), -22.0f, -10.0f, 6.8f, 0.8f, 0.02f, FLinearColor(0.90f, 0.88f, 0.82f, 1.0f), FLinearColor(0.52f, 0.49f, 0.44f, 1.0f), 0.9f, EManyNamesWeatherPrecipitation::Ash, 0.55f, 0.0f, 0.0f, 0.9f, true);
			break;
		case EManyNamesRegionId::Egypt:
			Profile.bAllowMegaLights = true;
			Profile.bPreferMegaLightLocalLights = true;
			Profile.BaselineState = MakeWeatherState(TEXT("Egypt.Baseline"), -38.0f, -28.0f, 10.5f, 1.35f, 0.009f, FLinearColor(1.0f, 0.93f, 0.80f, 1.0f), FLinearColor(0.95f, 0.85f, 0.70f, 1.0f), 0.25f, EManyNamesWeatherPrecipitation::Dust, 0.15f, 0.0f, 0.0f, 1.0f, false);
			Profile.HeroState = MakeWeatherState(TEXT("Egypt.Hero"), -31.0f, -12.0f, 8.0f, 1.0f, 0.018f, FLinearColor(0.96f, 0.82f, 0.62f, 1.0f), FLinearColor(0.84f, 0.70f, 0.54f, 1.0f), 0.8f, EManyNamesWeatherPrecipitation::Dust, 0.5f, 0.0f, 0.0f, 0.92f, true);
			break;
		case EManyNamesRegionId::Greece:
			Profile.bAllowMegaLights = true;
			Profile.bPreferMegaLightLocalLights = true;
			Profile.BaselineState = MakeWeatherState(TEXT("Greece.Baseline"), -32.0f, 42.0f, 9.8f, 1.45f, 0.007f, FLinearColor(0.97f, 0.95f, 0.90f, 1.0f), FLinearColor(0.78f, 0.83f, 0.88f, 1.0f), 0.4f, EManyNamesWeatherPrecipitation::None, 0.0f, 0.0f, 0.0f, 1.0f, false);
			Profile.HeroState = MakeWeatherState(TEXT("Greece.Hero"), -18.0f, 60.0f, 5.8f, 0.82f, 0.018f, FLinearColor(0.72f, 0.78f, 0.90f, 1.0f), FLinearColor(0.32f, 0.38f, 0.48f, 1.0f), 1.0f, EManyNamesWeatherPrecipitation::Rain, 0.8f, 0.5f, 0.0f, 0.84f, true);
			break;
		case EManyNamesRegionId::ItalicWest:
			Profile.bAllowMegaLights = true;
			Profile.bPreferMegaLightLocalLights = true;
			Profile.BaselineState = MakeWeatherState(TEXT("ItalicWest.Baseline"), -30.0f, -12.0f, 8.7f, 1.1f, 0.011f, FLinearColor(0.98f, 0.91f, 0.82f, 1.0f), FLinearColor(0.72f, 0.69f, 0.62f, 1.0f), 0.35f, EManyNamesWeatherPrecipitation::None, 0.0f, 0.0f, 0.0f, 1.0f, false);
			Profile.HeroState = MakeWeatherState(TEXT("ItalicWest.Hero"), -16.0f, -6.0f, 5.4f, 0.76f, 0.019f, FLinearColor(0.82f, 0.86f, 0.92f, 1.0f), FLinearColor(0.48f, 0.50f, 0.56f, 1.0f), 0.85f, EManyNamesWeatherPrecipitation::Snow, 0.45f, 0.15f, 0.35f, 0.82f, true);
			break;
		case EManyNamesRegionId::Convergence:
		default:
			Profile.bAllowMegaLights = true;
			Profile.bPreferMegaLightLocalLights = true;
			Profile.BaselineState = MakeWeatherState(TEXT("Convergence.Baseline"), -18.0f, 130.0f, 6.2f, 0.9f, 0.016f, FLinearColor(0.80f, 0.88f, 0.95f, 1.0f), FLinearColor(0.35f, 0.42f, 0.50f, 1.0f), 0.55f, EManyNamesWeatherPrecipitation::Ash, 0.15f, 0.0f, 0.0f, 1.0f, false);
			Profile.HeroState = MakeWeatherState(TEXT("Convergence.Hero"), -8.0f, 148.0f, 4.8f, 0.65f, 0.026f, FLinearColor(0.70f, 0.82f, 0.92f, 1.0f), FLinearColor(0.22f, 0.29f, 0.36f, 1.0f), 1.0f, EManyNamesWeatherPrecipitation::Snow, 0.3f, 0.05f, 0.25f, 0.88f, true);
			break;
		}

		return Profile;
	}

	FManyNamesTerrainProfile BuildTerrainProfile(EManyNamesRegionId RegionId, const FManyNamesBuildAssets& Assets)
	{
		FManyNamesTerrainProfile Profile;
		Profile.RegionId = RegionId;
		Profile.SharedPcgGraphId = TEXT("PCG.Shared.RouteBreakup");
		Profile.SharedPcgGraphAsset = FSoftObjectPath(TEXT("/Game/PCG/Shared/PCG_Shared_RouteBreakup.PCG_Shared_RouteBreakup"));
		Profile.SpawnTraceHeight = 1400.0f;
		Profile.SpawnTraceDepth = 2600.0f;
		Profile.SpawnSafetyLift = 120.0f;
		Profile.LandscapeQuadsPerSection = 63;
		Profile.LandscapeSectionsPerComponent = 1;
		Profile.LandscapeComponentCount = FIntPoint(12, 10);
		Profile.LandscapeScale = FVector(100.0f, 100.0f, 100.0f);

		switch (RegionId)
		{
		case EManyNamesRegionId::Opening:
			Profile.LandscapeMaterial = Assets.AshStone;
			Profile.RegionPcgGraphId = TEXT("PCG.Opening.AshScatter");
			Profile.RegionPcgGraphAsset = FSoftObjectPath(TEXT("/Game/PCG/Regions/PCG_Opening_AshScatter.PCG_Opening_AshScatter"));
			Profile.TerrainOrigin = FVector(-600.0f, 0.0f, -260.0f);
			Profile.TerrainExtent = FVector(120000.0f, 120000.0f, 0.0f);
			Profile.LandscapeComponentCount = FIntPoint(18, 18);
			Profile.HeightAmplitude = 860.0f;
			Profile.HeightBias = -30.0f;
			Profile.PrimaryHubLocation = FVector(-2800.0f, 1200.0f, 0.0f);
			Profile.PrimarySpawnLocation = FVector(-6200.0f, -180.0f, 220.0f);
			Profile.bEnableNaniteFoliage = true;
			Profile.RouteSplines = {
				{ TEXT("Opening.SpawnToWitness"), { FVector(-6200.0f, -180.0f, 0.0f), FVector(-5200.0f, -40.0f, 0.0f), FVector(-4300.0f, 220.0f, 0.0f), FVector(-2800.0f, 1200.0f, 0.0f) }, 700.0f, TEXT("Primary survivor route from spawn to witness camp.") },
				{ TEXT("Opening.WitnessToCrash"), { FVector(-2800.0f, 1200.0f, 0.0f), FVector(-1400.0f, 760.0f, 0.0f), FVector(-120.0f, 180.0f, 0.0f), FVector(980.0f, -60.0f, 0.0f) }, 720.0f, TEXT("Descending route from the witness camp into the impact trench.") },
				{ TEXT("Opening.WitnessToGates"), { FVector(-2800.0f, 1200.0f, 0.0f), FVector(-800.0f, 640.0f, 0.0f), FVector(1800.0f, 160.0f, 0.0f), FVector(5200.0f, -260.0f, 0.0f) }, 800.0f, TEXT("Exit route from camp to regional gates.") }
			};
			Profile.TerrainNotes = TEXT("Impact ravine, witness rise, and crash trench shelves.");
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.HighDetailRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.RealisticRock);
			AddAssetPath(Profile.FoliageScatterMeshPaths, Assets.Rock001);
			break;
		case EManyNamesRegionId::Egypt:
			Profile.LandscapeMaterial = Assets.SandStone;
			Profile.RegionPcgGraphId = TEXT("PCG.Egypt.DryScatter");
			Profile.RegionPcgGraphAsset = FSoftObjectPath(TEXT("/Game/PCG/Regions/PCG_Egypt_DryScatter.PCG_Egypt_DryScatter"));
			Profile.TerrainOrigin = FVector(0.0f, 0.0f, -260.0f);
			Profile.TerrainExtent = FVector(220000.0f, 200000.0f, 0.0f);
			Profile.LandscapeComponentCount = FIntPoint(24, 22);
			Profile.HeightAmplitude = 620.0f;
			Profile.PrimaryHubLocation = FVector(0.0f, -220.0f, 0.0f);
			Profile.PrimarySpawnLocation = FVector(-9800.0f, -600.0f, 180.0f);
			Profile.bEnableNaniteFoliage = true;
			Profile.RouteSplines = {
				{ TEXT("Egypt.SpawnToTemple"), { FVector(-9800.0f, -600.0f, 0.0f), FVector(-7200.0f, -520.0f, 0.0f), FVector(-3600.0f, -280.0f, 0.0f), FVector(0.0f, -220.0f, 0.0f) }, 760.0f, TEXT("Approach from arrival point to temple district.") },
				{ TEXT("Egypt.TempleToArchive"), { FVector(0.0f, -220.0f, 0.0f), FVector(900.0f, -160.0f, 0.0f), FVector(1800.0f, -80.0f, 0.0f), FVector(2800.0f, 0.0f, 0.0f) }, 650.0f, TEXT("Ceremonial road connecting temple court and archive.") },
				{ TEXT("Egypt.TempleToNecropolis"), { FVector(0.0f, -220.0f, 0.0f), FVector(120.0f, 1100.0f, 0.0f), FVector(0.0f, 2600.0f, 0.0f), FVector(0.0f, 6200.0f, 0.0f) }, 760.0f, TEXT("Necropolis ascent from the civic core.") },
				{ TEXT("Egypt.TempleToCanalEdge"), { FVector(0.0f, -220.0f, 0.0f), FVector(-1800.0f, -1200.0f, 0.0f), FVector(-4200.0f, -2200.0f, 0.0f), FVector(-7000.0f, -2600.0f, 0.0f) }, 740.0f, TEXT("Managed floodplain route and canal-facing approach.") }
			};
			Profile.TerrainNotes = TEXT("Temple plateau, archive road, market terraces, and necropolis ridge.");
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.EgyptWallA);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.EgyptColumn);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.MastabaEntrance);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.BentPyramid);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.FoliageScatterMeshPaths, Assets.PyramidStone);
			break;
		case EManyNamesRegionId::Greece:
			Profile.LandscapeMaterial = Assets.Plaster;
			Profile.RegionPcgGraphId = TEXT("PCG.Greece.MediterraneanScatter");
			Profile.RegionPcgGraphAsset = FSoftObjectPath(TEXT("/Game/PCG/Regions/PCG_Greece_MediterraneanScatter.PCG_Greece_MediterraneanScatter"));
			Profile.TerrainOrigin = FVector(0.0f, 0.0f, -320.0f);
			Profile.TerrainExtent = FVector(240000.0f, 220000.0f, 0.0f);
			Profile.LandscapeComponentCount = FIntPoint(24, 22);
			Profile.HeightAmplitude = 980.0f;
			Profile.PrimaryHubLocation = FVector(0.0f, -180.0f, 0.0f);
			Profile.PrimarySpawnLocation = FVector(-6200.0f, -120.0f, 220.0f);
			Profile.bEnableNaniteFoliage = true;
			Profile.RouteSplines = {
				{ TEXT("Greece.SpawnToSanctuary"), { FVector(-6200.0f, -120.0f, 0.0f), FVector(-4200.0f, -20.0f, 0.0f), FVector(-2400.0f, 140.0f, 0.0f), FVector(0.0f, -180.0f, 0.0f) }, 760.0f, TEXT("Main ascent into the sanctuary court.") },
				{ TEXT("Greece.SanctuaryToShrine"), { FVector(0.0f, -180.0f, 0.0f), FVector(1200.0f, 620.0f, 0.0f), FVector(2100.0f, 1400.0f, 0.0f), FVector(3600.0f, 3200.0f, 0.0f) }, 700.0f, TEXT("Cliff route from sanctuary to the upper shrine court.") },
				{ TEXT("Greece.SanctuaryToStormRidge"), { FVector(0.0f, -180.0f, 0.0f), FVector(-600.0f, -1200.0f, 0.0f), FVector(-200.0f, -2800.0f, 0.0f), FVector(800.0f, -4600.0f, 0.0f) }, 700.0f, TEXT("Exposed storm-ridge route beneath the divine landmark.") }
			};
			Profile.TerrainNotes = TEXT("Sanctuary terrace, mountain route, and cliff shelf rise.");
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.GreeceDolmen);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.HighDetailRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.RealisticRock);
			AddAssetPath(Profile.FoliageScatterMeshPaths, Assets.AncientCarvedStone);
			break;
		case EManyNamesRegionId::ItalicWest:
			Profile.LandscapeMaterial = Assets.AshStone;
			Profile.RegionPcgGraphId = TEXT("PCG.Italic.RoadsideScatter");
			Profile.RegionPcgGraphAsset = FSoftObjectPath(TEXT("/Game/PCG/Regions/PCG_Italic_RoadsideScatter.PCG_Italic_RoadsideScatter"));
			Profile.TerrainOrigin = FVector(0.0f, 0.0f, -280.0f);
			Profile.TerrainExtent = FVector(200000.0f, 200000.0f, 0.0f);
			Profile.LandscapeComponentCount = FIntPoint(22, 22);
			Profile.HeightAmplitude = 700.0f;
			Profile.PrimaryHubLocation = FVector(-1200.0f, -600.0f, 0.0f);
			Profile.PrimarySpawnLocation = FVector(-6200.0f, -360.0f, 220.0f);
			Profile.bEnableNaniteFoliage = true;
			Profile.RouteSplines = {
				{ TEXT("Italic.SpawnToSettlement"), { FVector(-6200.0f, -360.0f, 0.0f), FVector(-4600.0f, -420.0f, 0.0f), FVector(-2800.0f, -520.0f, 0.0f), FVector(-1200.0f, -600.0f, 0.0f) }, 700.0f, TEXT("Primary road into the hill settlement.") },
				{ TEXT("Italic.SettlementToBoundary"), { FVector(-1200.0f, -600.0f, 0.0f), FVector(-400.0f, 620.0f, 0.0f), FVector(800.0f, 1180.0f, 0.0f), FVector(3800.0f, 2400.0f, 0.0f) }, 700.0f, TEXT("Ritual road and boundary field approach.") },
				{ TEXT("Italic.SettlementToForge"), { FVector(-1200.0f, -600.0f, 0.0f), FVector(-200.0f, -420.0f, 0.0f), FVector(1000.0f, -260.0f, 0.0f), FVector(1800.0f, -200.0f, 0.0f) }, 620.0f, TEXT("Civic route linking the settlement and forge-law chamber.") }
			};
			Profile.TerrainNotes = TEXT("Hill settlement, civic road embankment, forge shelf, and boundary field.");
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.ItalicChurchRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.ConvergenceDestroyedWood);
			AddAssetPath(Profile.FoliageScatterMeshPaths, Assets.ItalicBollard);
			break;
		case EManyNamesRegionId::Convergence:
		default:
			Profile.LandscapeMaterial = Assets.Basalt;
			Profile.RegionPcgGraphId = TEXT("PCG.Convergence.SparseScatter");
			Profile.RegionPcgGraphAsset = FSoftObjectPath(TEXT("/Game/PCG/Regions/PCG_Convergence_SparseScatter.PCG_Convergence_SparseScatter"));
			Profile.TerrainOrigin = FVector(0.0f, 0.0f, -340.0f);
			Profile.TerrainExtent = FVector(180000.0f, 160000.0f, 0.0f);
			Profile.LandscapeComponentCount = FIntPoint(20, 18);
			Profile.HeightAmplitude = 900.0f;
			Profile.PrimaryHubLocation = FVector(0.0f, 2200.0f, 0.0f);
			Profile.PrimarySpawnLocation = FVector(-8600.0f, -420.0f, 220.0f);
			Profile.bEnableNaniteFoliage = false;
			Profile.RouteSplines = {
				{ TEXT("Convergence.SpawnToCore"), { FVector(-8600.0f, -420.0f, 0.0f), FVector(-5200.0f, -980.0f, 0.0f), FVector(-2200.0f, 120.0f, 0.0f), FVector(0.0f, 2200.0f, 0.0f) }, 760.0f, TEXT("Descent route from burial rim to bridge plateau.") },
				{ TEXT("Convergence.CoreToBridge"), { FVector(0.0f, 2200.0f, 0.0f), FVector(1000.0f, 2600.0f, 0.0f), FVector(2400.0f, 3400.0f, 0.0f), FVector(4200.0f, 4200.0f, 0.0f) }, 680.0f, TEXT("Peripheral route along the broken bridge approach.") }
			};
			Profile.TerrainNotes = TEXT("Burial basin, ruin shell, descent shelf, and bridge plateau.");
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.CaveRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.HighDetailRock);
			AddAssetPath(Profile.StructuralNaniteMeshPaths, Assets.ConvergenceDestroyedWood);
			AddAssetPath(Profile.FoliageScatterMeshPaths, Assets.PyramidStone);
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

	FManyNamesSpawnValidationResult ProjectLocationToGround(UWorld* World, const FVector& CandidateLocation, const FManyNamesTerrainProfile& TerrainProfile)
	{
		FManyNamesSpawnValidationResult Result;
		if (!World)
		{
			return Result;
		}

		const FVector TraceStart = CandidateLocation + FVector(0.0f, 0.0f, TerrainProfile.SpawnTraceHeight);
		const FVector TraceEnd = CandidateLocation - FVector(0.0f, 0.0f, TerrainProfile.SpawnTraceDepth);
		FHitResult HitResult;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ManyNamesSpawnProjection), false);
		Result.bHasGround = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, Params) && HitResult.bBlockingHit;
		Result.GroundLocation = Result.bHasGround ? HitResult.ImpactPoint : CandidateLocation;
		return Result;
	}

	void ValidateNaniteProfile(const FManyNamesTerrainProfile& TerrainProfile)
	{
		if (TerrainProfile.bEnableNaniteForStructuralMeshes)
		{
			EnableNaniteForPaths(TerrainProfile.StructuralNaniteMeshPaths);
		}
	}

	FString MakeSafeAssetName(const FString& RawName)
	{
		FString SafeName = RawName;
		SafeName.ReplaceInline(TEXT("."), TEXT("_"));
		SafeName.ReplaceInline(TEXT(" "), TEXT("_"));
		return SafeName;
	}

	UPCGGraph* EnsurePcgGraphAsset(const FString& PackagePath, const FString& AssetName)
	{
		const FString SafeAssetName = MakeSafeAssetName(AssetName);
		const FString ObjectPath = PackagePath / SafeAssetName + TEXT(".") + SafeAssetName;
		const FString LongPackageName = PackagePath / SafeAssetName;
		if (FPackageName::DoesPackageExist(LongPackageName))
		{
			if (UPCGGraph* ExistingGraph = LoadObject<UPCGGraph>(nullptr, *ObjectPath))
			{
				return ExistingGraph;
			}
		}

		UPackage* Package = CreatePackage(*LongPackageName);
		if (!Package)
		{
			return nullptr;
		}

		UPCGGraph* Graph = NewObject<UPCGGraph>(Package, *SafeAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Graph)
		{
			return nullptr;
		}

		Graph->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Graph);

		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		UPackage::SavePackage(Package, Graph, *Filename, SaveArgs);
		return Graph;
	}

	UPCGGraph* EnsurePcgGraphAssetFromPath(const FSoftObjectPath& AssetPath, const FString& FallbackPackagePath, const FString& FallbackAssetName)
	{
		if (!AssetPath.IsNull())
		{
			const FString LongPackageName = AssetPath.GetLongPackageName();
			const FString AssetName = AssetPath.GetAssetName();
			if (!LongPackageName.IsEmpty() && !AssetName.IsEmpty())
			{
				return EnsurePcgGraphAsset(FPaths::GetPath(LongPackageName), AssetName);
			}
		}

		return EnsurePcgGraphAsset(FallbackPackagePath, FallbackAssetName);
	}

	TArray<uint16> BuildLandscapeHeightData(const FManyNamesTerrainProfile& TerrainProfile)
	{
		const int32 ComponentSizeQuads = TerrainProfile.LandscapeQuadsPerSection * TerrainProfile.LandscapeSectionsPerComponent;
		const int32 QuadsX = TerrainProfile.LandscapeComponentCount.X * ComponentSizeQuads;
		const int32 QuadsY = TerrainProfile.LandscapeComponentCount.Y * ComponentSizeQuads;
		const int32 VertsX = QuadsX + 1;
		const int32 VertsY = QuadsY + 1;
		TArray<uint16> HeightData;
		HeightData.SetNumUninitialized(VertsX * VertsY);

		const float GeomExtentX = static_cast<float>(QuadsX) * TerrainProfile.LandscapeScale.X;
		const float GeomExtentY = static_cast<float>(QuadsY) * TerrainProfile.LandscapeScale.Y;
		const FVector2D HalfGeomExtent(GeomExtentX * 0.5f, GeomExtentY * 0.5f);
		const float ZScale = FMath::Max(1.0f, TerrainProfile.LandscapeScale.Z);

		auto AddGaussian = [](double X, double Y, const FVector2D& Center, double RadiusX, double RadiusY, double Strength)
		{
			const double DX = (X - Center.X) / FMath::Max(1.0, RadiusX);
			const double DY = (Y - Center.Y) / FMath::Max(1.0, RadiusY);
			return Strength * FMath::Exp(-(DX * DX + DY * DY));
		};

		// Named constants for the separable continuous-relief term.
		constexpr double ReliefFreqX = 0.00083;
		constexpr double ReliefFreqY = 0.00077;
		constexpr double ReliefAmpX  = 1.15;
		constexpr double ReliefAmpY  = 1.05;

		// Precompute per-column Sin and per-row Cos values so the inner heightmap
		// loop performs O(VertsX + VertsY) trig calls instead of O(VertsX * VertsY).
		TArray<double> SinX;
		SinX.SetNumUninitialized(VertsX);
		for (int32 X = 0; X < VertsX; ++X)
		{
			const double AlphaX = VertsX > 1 ? static_cast<double>(X) / static_cast<double>(VertsX - 1) : 0.0;
			const double WorldX = FMath::Lerp(-HalfGeomExtent.X, HalfGeomExtent.X, AlphaX);
			SinX[X] = FMath::Sin(WorldX * ReliefFreqX) * ReliefAmpX;
		}

		TArray<double> CosY;
		CosY.SetNumUninitialized(VertsY);
		for (int32 Y = 0; Y < VertsY; ++Y)
		{
			const double AlphaY = VertsY > 1 ? static_cast<double>(Y) / static_cast<double>(VertsY - 1) : 0.0;
			const double WorldY = FMath::Lerp(-HalfGeomExtent.Y, HalfGeomExtent.Y, AlphaY);
			CosY[Y] = FMath::Cos(WorldY * ReliefFreqY) * ReliefAmpY;
		}

		for (int32 Y = 0; Y < VertsY; ++Y)
		{
			for (int32 X = 0; X < VertsX; ++X)
			{
				const double AlphaX = VertsX > 1 ? static_cast<double>(X) / static_cast<double>(VertsX - 1) : 0.0;
				const double AlphaY = VertsY > 1 ? static_cast<double>(Y) / static_cast<double>(VertsY - 1) : 0.0;
				const double WorldX = FMath::Lerp(-HalfGeomExtent.X, HalfGeomExtent.X, AlphaX);
				const double WorldY = FMath::Lerp(-HalfGeomExtent.Y, HalfGeomExtent.Y, AlphaY);

				double HeightWorld = TerrainProfile.HeightBias;

				switch (TerrainProfile.RegionId)
				{
				case EManyNamesRegionId::Opening:
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(-2500.0, 600.0), 2800.0, 1800.0, 320.0);
					HeightWorld -= AddGaussian(WorldX, WorldY, FVector2D(250.0, 0.0), 2200.0, 900.0, 410.0);
					HeightWorld += 0.18 * WorldX + 0.05 * WorldY;
					break;
				case EManyNamesRegionId::Egypt:
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(-800.0, -900.0), 4200.0, 2800.0, 260.0);
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(1200.0, 3200.0), 2600.0, 1800.0, 280.0);
					HeightWorld -= AddGaussian(WorldX, WorldY, FVector2D(-3400.0, 0.0), 2200.0, 3400.0, 120.0);
					HeightWorld += 0.03 * WorldY;
					break;
				case EManyNamesRegionId::Greece:
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(2000.0, 1700.0), 2400.0, 2000.0, 520.0);
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(-600.0, -900.0), 2600.0, 1800.0, 220.0);
					HeightWorld -= AddGaussian(WorldX, WorldY, FVector2D(-2600.0, 400.0), 2000.0, 3000.0, 180.0);
					HeightWorld += 0.10 * WorldY;
					break;
				case EManyNamesRegionId::ItalicWest:
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(-1200.0, -600.0), 3000.0, 2200.0, 300.0);
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(2100.0, 1300.0), 2400.0, 1800.0, 180.0);
					HeightWorld += 0.04 * WorldX + 0.02 * WorldY;
					break;
				case EManyNamesRegionId::Convergence:
				default:
					HeightWorld -= AddGaussian(WorldX, WorldY, FVector2D(0.0, 0.0), 3600.0, 2800.0, 340.0);
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(0.0, 2400.0), 2600.0, 1600.0, 220.0);
					HeightWorld += AddGaussian(WorldX, WorldY, FVector2D(-2400.0, -1600.0), 1800.0, 1400.0, 260.0);
					break;
				}

				// Add subtle continuous relief so generated components do not collapse to perfectly
				// flat cached bounds, while keeping traversal unchanged at gameplay scale.
				HeightWorld += SinX[X] + CosY[Y];

				const float LocalHeight = static_cast<float>(HeightWorld / ZScale);
				HeightData[Y * VertsX + X] = LandscapeDataAccess::GetTexHeight(LocalHeight);
			}
		}

		return HeightData;
	}

	ALandscape* SpawnLandscapeTerrain(UWorld* World, const FManyNamesTerrainProfile& TerrainProfile)
	{
		if (!World)
		{
			return nullptr;
		}

		const int32 ComponentSizeQuads = TerrainProfile.LandscapeQuadsPerSection * TerrainProfile.LandscapeSectionsPerComponent;
		const int32 QuadsX = TerrainProfile.LandscapeComponentCount.X * ComponentSizeQuads;
		const int32 QuadsY = TerrainProfile.LandscapeComponentCount.Y * ComponentSizeQuads;

		FActorSpawnParameters SpawnParams;
		const float LandscapeGeomExtentX = static_cast<float>(QuadsX) * TerrainProfile.LandscapeScale.X;
		const float LandscapeGeomExtentY = static_cast<float>(QuadsY) * TerrainProfile.LandscapeScale.Y;
		const FVector LandscapeLocation = TerrainProfile.TerrainOrigin - FVector(LandscapeGeomExtentX * 0.5f, LandscapeGeomExtentY * 0.5f, 0.0f);
		ALandscape* Landscape = World->SpawnActor<ALandscape>(LandscapeLocation, FRotator::ZeroRotator, SpawnParams);
		if (!Landscape)
		{
			return nullptr;
		}

		Landscape->SetActorLabel(TEXT("TerrainLandscape"));
		Landscape->SetActorScale3D(TerrainProfile.LandscapeScale);
		Landscape->LandscapeMaterial = TerrainProfile.LandscapeMaterial.Get();

		TMap<FGuid, TArray<uint16>> HeightDataPerLayer;
		HeightDataPerLayer.Add(FGuid(), BuildLandscapeHeightData(TerrainProfile));
		TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
		MaterialLayerDataPerLayer.Add(FGuid(), {});

		Landscape->Import(
			FGuid::NewGuid(),
			0,
			0,
			QuadsX,
			QuadsY,
			TerrainProfile.LandscapeSectionsPerComponent,
			TerrainProfile.LandscapeQuadsPerSection,
			HeightDataPerLayer,
			nullptr,
			MaterialLayerDataPerLayer,
			ELandscapeImportAlphamapType::Additive,
			TArrayView<const FLandscapeLayer>());

		Landscape->CreateLandscapeInfo();
		for (ULandscapeComponent* Component : Landscape->LandscapeComponents)
		{
			if (!Component)
			{
				continue;
			}

			Component->UpdateCachedBounds(false);
			Component->UpdateMaterialInstances();
		}
		Landscape->RecreateCollisionComponents();
		Landscape->MarkComponentsRenderStateDirty();
		Landscape->ReregisterAllComponents();
		Landscape->PostEditChange();
		return Landscape;
	}

	AActor* SpawnRouteSplineActor(UWorld* World, const FManyNamesRouteSplineDefinition& RouteDefinition)
	{
		if (!World || RouteDefinition.ControlPoints.Num() < 2)
		{
			return nullptr;
		}

		AActor* RouteActor = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!RouteActor)
		{
			return nullptr;
		}

		RouteActor->SetActorLabel(RouteDefinition.RouteId.IsNone() ? TEXT("RouteSpline") : RouteDefinition.RouteId.ToString());
		USplineComponent* SplineComponent = NewObject<USplineComponent>(RouteActor, TEXT("RouteSpline"));
		RouteActor->AddInstanceComponent(SplineComponent);
		RouteActor->SetRootComponent(SplineComponent);
		SplineComponent->RegisterComponent();
		SplineComponent->SetClosedLoop(false);
		SplineComponent->ClearSplinePoints();
		for (const FVector& ControlPoint : RouteDefinition.ControlPoints)
		{
			SplineComponent->AddSplinePoint(ControlPoint, ESplineCoordinateSpace::World, false);
		}
		SplineComponent->UpdateSpline();
		return RouteActor;
	}

	void SpawnPcgVolume(UWorld* World, const FString& Label, const FVector& Center, const FVector& Extent, UPCGGraph* Graph)
	{
		if (!World || !Graph)
		{
			return;
		}

		APCGVolume* Volume = World->SpawnActor<APCGVolume>(Center, FRotator::ZeroRotator);
		if (!Volume)
		{
			return;
		}

		Volume->SetActorLabel(Label);
		Volume->SetActorScale3D(FVector(FMath::Max(1.0f, Extent.X / 100.0f), FMath::Max(1.0f, Extent.Y / 100.0f), FMath::Max(1.0f, Extent.Z / 100.0f)));
		if (UPCGComponent* PCGComponent = Volume->GetComponentByClass<UPCGComponent>())
		{
			PCGComponent->SetGraphLocal(Graph);
			PCGComponent->Seed = 42;
			PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
		}
	}

	void BuildTerrainSystems(UWorld* World, const FManyNamesTerrainProfile& TerrainProfile)
	{
		SpawnLandscapeTerrain(World, TerrainProfile);

		const FString SharedGraphPackage = TEXT("/Game/PCG/Shared");
		const FString RegionGraphPackage = TEXT("/Game/PCG/Regions");
		UPCGGraph* SharedGraph = EnsurePcgGraphAssetFromPath(TerrainProfile.SharedPcgGraphAsset, SharedGraphPackage, TerrainProfile.SharedPcgGraphId.ToString());
		UPCGGraph* RegionGraph = EnsurePcgGraphAssetFromPath(TerrainProfile.RegionPcgGraphAsset, RegionGraphPackage, TerrainProfile.RegionPcgGraphId.ToString());
		if (SharedGraph)
		{
			SpawnPcgVolume(World, TEXT("PCG_SharedScatter"), TerrainProfile.TerrainOrigin + FVector(0.0f, 0.0f, 300.0f), FVector(TerrainProfile.TerrainExtent.X * 0.5f, TerrainProfile.TerrainExtent.Y * 0.5f, 800.0f), SharedGraph);
		}
		if (RegionGraph)
		{
			SpawnPcgVolume(
				World,
				FString::Printf(TEXT("PCG_%s"), *UEnum::GetValueAsString(TerrainProfile.RegionId)),
				TerrainProfile.PrimaryHubLocation + FVector(0.0f, 0.0f, 220.0f),
				FVector(TerrainProfile.TerrainExtent.X * 0.22f, TerrainProfile.TerrainExtent.Y * 0.18f, 1200.0f),
				RegionGraph);
		}

		for (const FManyNamesRouteSplineDefinition& RouteDefinition : TerrainProfile.RouteSplines)
		{
			SpawnRouteSplineActor(World, RouteDefinition);
			if (SharedGraph && RouteDefinition.ControlPoints.Num() > 0)
			{
				FVector RouteCenter = FVector::ZeroVector;
				for (const FVector& ControlPoint : RouteDefinition.ControlPoints)
				{
					RouteCenter += ControlPoint;
				}
				RouteCenter /= RouteDefinition.ControlPoints.Num();
				SpawnPcgVolume(
					World,
					FString::Printf(TEXT("PCG_%s_Scatter"), *RouteDefinition.RouteId.ToString()),
					RouteCenter + FVector(0.0f, 0.0f, 180.0f),
					FVector(RouteDefinition.RouteWidth * 1.5f, RouteDefinition.RouteWidth * 1.5f, 900.0f),
					SharedGraph);
			}
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

	AManyNamesScenicActor* SpawnTerrainShelf(
		UWorld* World,
		const FString& Label,
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		UMaterialInterface* Material,
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		AManyNamesScenicActor* Actor = SpawnScenic(World, Label, Mesh, Location, Scale, Material, Rotation);
		if (Actor)
		{
			if (UStaticMeshComponent* StaticMeshComponent = Actor->GetStaticMeshComponent())
			{
				StaticMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
				StaticMeshComponent->SetMobility(EComponentMobility::Movable);
			}
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

	AManyNamesScenicActor* SpawnNpcOnTerrain(
		UWorld* World,
		const FString& Label,
		const FManyNamesNpcVisualProfile& Profile,
		const FVector& Location,
		float Yaw,
		const FManyNamesTerrainProfile& TerrainProfile,
		float StandingLift = 88.0f)
	{
		const FManyNamesSpawnValidationResult Projection = ProjectLocationToGround(World, Location, TerrainProfile);
		const FVector SafeLocation = Projection.bHasGround
			? FVector(Location.X, Location.Y, Projection.GroundLocation.Z + StandingLift)
			: Location;
		return SpawnNpc(World, Label, Profile, SafeLocation, Yaw);
	}

	void SpawnAmbientCluster(
		UWorld* World,
		const FString& BaseLabel,
		const FGameplayTag& RoleTag,
		const FManyNamesBuildAssets& Assets,
		FName ProfileId,
		const FVector& Center,
		const TArray<FVector>& Offsets,
		float BaseYaw,
		bool bEnableClothSimulation = false)
	{
		for (int32 Index = 0; Index < Offsets.Num(); ++Index)
		{
			const FVector SpawnLocation = Center + Offsets[Index];
			const float Yaw = BaseYaw + (Index * 17.0f);
			SpawnNpc(
				World,
				FString::Printf(TEXT("%s_%02d"), *BaseLabel, Index + 1),
				MakeNpcProfile(
					RoleTag,
					ResolveAmbientCharacter(Assets, ProfileId, Index),
					Assets.IdleAnimation,
					nullptr,
					FVector(1.0f),
					FVector(0.0f, 0.0f, -88.0f),
					false,
					ProfileId,
					NAME_None,
					NAME_None,
					ProfileId,
					TEXT("ambient_hub"),
					EManyNamesCrowdBehaviorTier::HubAmbient,
					bEnableClothSimulation),
				SpawnLocation,
				Yaw);
		}
	}

	void SpawnAmbientClusterOnTerrain(
		UWorld* World,
		const FString& BaseLabel,
		const FGameplayTag& RoleTag,
		const FManyNamesBuildAssets& Assets,
		FName ProfileId,
		const FVector& Center,
		const TArray<FVector>& Offsets,
		float BaseYaw,
		const FManyNamesTerrainProfile& TerrainProfile,
		float StandingLift = 88.0f,
		bool bEnableClothSimulation = false)
	{
		for (int32 Index = 0; Index < Offsets.Num(); ++Index)
		{
			const FVector RawLocation = Center + Offsets[Index];
			const FManyNamesSpawnValidationResult Projection = ProjectLocationToGround(World, RawLocation, TerrainProfile);
			const FVector SpawnLocation = Projection.bHasGround
				? FVector(RawLocation.X, RawLocation.Y, Projection.GroundLocation.Z + StandingLift)
				: RawLocation;
			const float Yaw = BaseYaw + (Index * 17.0f);
			SpawnNpc(
				World,
				FString::Printf(TEXT("%s_%02d"), *BaseLabel, Index + 1),
				MakeNpcProfile(
					RoleTag,
					ResolveAmbientCharacter(Assets, ProfileId, Index),
					Assets.IdleAnimation,
					nullptr,
					FVector(1.0f),
					FVector(0.0f, 0.0f, -88.0f),
					false,
					ProfileId,
					NAME_None,
					NAME_None,
					ProfileId,
					TEXT("ambient_hub"),
					EManyNamesCrowdBehaviorTier::HubAmbient,
					bEnableClothSimulation),
				SpawnLocation,
				Yaw);
		}
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

	AManyNamesInteractableActor* SpawnInteractableNpcOnTerrain(
		UWorld* World,
		const FString& Label,
		const FText& InteractionLabel,
		const FManyNamesNpcVisualProfile& Profile,
		const FVector& Location,
		float Yaw,
		EManyNamesInteractionActionType InteractionType,
		FName QuestId,
		const TArray<FName>& RequiredOutputs,
		bool bSingleUse,
		const FManyNamesTerrainProfile& TerrainProfile,
		float StandingLift = 88.0f)
	{
		const FManyNamesSpawnValidationResult Projection = ProjectLocationToGround(World, Location, TerrainProfile);
		const FVector SafeLocation = Projection.bHasGround
			? FVector(Location.X, Location.Y, Projection.GroundLocation.Z + StandingLift)
			: Location;
		return SpawnInteractableNpc(World, Label, InteractionLabel, Profile, SafeLocation, Yaw,
			InteractionType, QuestId, RequiredOutputs, bSingleUse);
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

	AManyNamesInteractableActor* SpawnTravelGateOnTerrain(
		UWorld* World,
		const FString& Label,
		const FText& InteractionLabel,
		const FVector& Location,
		const FVector& Scale,
		EManyNamesRegionId RegionId,
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		const TArray<FName>& RequiredOutputs,
		const FManyNamesTerrainProfile& TerrainProfile,
		float GateFloorLift = 80.0f)
	{
		const FManyNamesSpawnValidationResult Projection = ProjectLocationToGround(World, Location, TerrainProfile);
		const FVector SafeLocation = Projection.bHasGround
			? FVector(Location.X, Location.Y, Projection.GroundLocation.Z + GateFloorLift)
			: Location;
		return SpawnTravelGate(World, Label, InteractionLabel, SafeLocation, Scale, RegionId, Mesh, Material, RequiredOutputs);
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

	void SpawnSky(UWorld* World, const FManyNamesWeatherState& WeatherState)
	{
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 1400.0f), FRotator(WeatherState.SunPitch, WeatherState.SunYaw, 0.0f));
		if (Sun)
		{
			Sun->SetActorLabel(TEXT("SunLight"));
			if (UDirectionalLightComponent* DirectionalLightComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
			{
				DirectionalLightComponent->SetMobility(EComponentMobility::Movable);
				DirectionalLightComponent->SetIntensity(WeatherState.SunIntensity);
				DirectionalLightComponent->SetLightColor(WeatherState.KeyLightTint.ToFColor(true));
				DirectionalLightComponent->SetAtmosphereSunLight(true);
				DirectionalLightComponent->SetVolumetricScatteringIntensity(1.4f);
				DirectionalLightComponent->DynamicShadowDistanceMovableLight = 40000.0f;
			}
		}

		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 240.0f), FRotator::ZeroRotator);
		if (SkyLight)
		{
			SkyLight->SetActorLabel(TEXT("SkyLight"));
			if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
			{
				SkyLightComponent->SetMobility(EComponentMobility::Movable);
				SkyLightComponent->SetIntensity(WeatherState.SkyIntensity);
				SkyLightComponent->SetRealTimeCapture(true);
			}
		}

		AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (Fog)
		{
			Fog->SetActorLabel(TEXT("HeightFog"));
			if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
			{
				FogComponent->FogDensity = WeatherState.FogDensity;
				FogComponent->FogHeightFalloff = 0.2f;
				FogComponent->SetFogInscatteringColor(WeatherState.FogTint);
				FogComponent->bEnableVolumetricFog = true;
				FogComponent->VolumetricFogScatteringDistribution = 0.55f;
				FogComponent->VolumetricFogExtinctionScale = 1.2f;
			}
		}

		ASkyAtmosphere* Atmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (Atmosphere)
		{
			Atmosphere->SetActorLabel(TEXT("SkyAtmosphere"));
		}
	}

	void SpawnCinematicPostProcess(UWorld* World, EManyNamesRegionId RegionId)
	{
		if (!World)
		{
			return;
		}

		APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Volume)
		{
			return;
		}

		Volume->SetActorLabel(TEXT("CinematicPostProcess"));
		Volume->bUnbound = true;
		Volume->BlendWeight = 1.0f;
		FPostProcessSettings& Settings = Volume->Settings;
		Settings.bOverride_AutoExposureMethod = true;
		Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = 0.15f;
		Settings.bOverride_Contrast = true;
		Settings.Contrast = FVector4(1.03f, 1.03f, 1.03f, 1.0f);

		switch (RegionId)
		{
		case EManyNamesRegionId::Opening:
			Settings.bOverride_ColorSaturation = true;
			Settings.ColorSaturation = FVector4(0.92f, 0.91f, 0.88f, 1.0f);
			Settings.bOverride_ColorGain = true;
			Settings.ColorGain = FVector4(1.02f, 0.98f, 0.95f, 1.0f);
			break;
		case EManyNamesRegionId::Egypt:
			Settings.bOverride_ColorSaturation = true;
			Settings.ColorSaturation = FVector4(1.08f, 1.03f, 0.95f, 1.0f);
			Settings.bOverride_ColorGain = true;
			Settings.ColorGain = FVector4(1.07f, 1.0f, 0.95f, 1.0f);
			break;
		case EManyNamesRegionId::Greece:
			Settings.bOverride_ColorSaturation = true;
			Settings.ColorSaturation = FVector4(0.96f, 1.01f, 1.05f, 1.0f);
			Settings.bOverride_ColorGain = true;
			Settings.ColorGain = FVector4(0.95f, 1.0f, 1.06f, 1.0f);
			break;
		case EManyNamesRegionId::ItalicWest:
			Settings.bOverride_ColorSaturation = true;
			Settings.ColorSaturation = FVector4(0.95f, 0.93f, 0.9f, 1.0f);
			Settings.bOverride_ColorGain = true;
			Settings.ColorGain = FVector4(1.01f, 0.98f, 0.94f, 1.0f);
			break;
		case EManyNamesRegionId::Convergence:
		default:
			Settings.bOverride_ColorSaturation = true;
			Settings.ColorSaturation = FVector4(0.84f, 0.9f, 0.98f, 1.0f);
			Settings.bOverride_ColorGain = true;
			Settings.ColorGain = FVector4(0.87f, 0.93f, 1.02f, 1.0f);
			break;
		}
	}

	void SpawnEnvironmentController(UWorld* World, const FManyNamesEnvironmentProfile& EnvironmentProfile)
	{
		if (!World)
		{
			return;
		}

		AManyNamesEnvironmentController* Controller = World->SpawnActor<AManyNamesEnvironmentController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Controller)
		{
			return;
		}

		Controller->SetActorLabel(TEXT("EnvironmentController"));
		Controller->SetEnvironmentProfile(EnvironmentProfile);
	}

	bool SpawnPlayerStart(UWorld* World, const FVector& Location, float Yaw, const FManyNamesTerrainProfile& TerrainProfile)
	{
		if (!World)
		{
			return false;
		}

		const FManyNamesSpawnValidationResult Projection = ProjectLocationToGround(World, Location, TerrainProfile);
		if (!Projection.bHasGround)
		{
			UE_LOG(LogTemp, Error, TEXT("No valid ground found for PlayerStart at %s in region %d."), *Location.ToCompactString(), static_cast<int32>(TerrainProfile.RegionId));
			return false;
		}

		const FVector SafeLocation = Projection.GroundLocation + FVector(0.0f, 0.0f, TerrainProfile.SpawnSafetyLift);
		APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(SafeLocation, FRotator(0.0f, Yaw, 0.0f));
		if (PlayerStart)
		{
			PlayerStart->SetActorLabel(TEXT("PlayerStart"));
			return true;
		}

		return false;
	}

	void BuildOpeningMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Opening, Assets, RoleTags);
		const FManyNamesEnvironmentProfile EnvironmentProfile = BuildEnvironmentProfile(EManyNamesRegionId::Opening);
		const FManyNamesTerrainProfile TerrainProfile = BuildTerrainProfile(EManyNamesRegionId::Opening, Assets);
		ValidateNaniteProfile(TerrainProfile);
		SpawnSky(World, EnvironmentProfile.BaselineState);
		SpawnCinematicPostProcess(World, EManyNamesRegionId::Opening);
		SpawnEnvironmentController(World, EnvironmentProfile);
		BuildTerrainSystems(World, TerrainProfile);

		SpawnTerrainShelf(World, TEXT("WitnessPath"), Assets.Cube, FVector(-1200.0f, 780.0f, 24.0f), FVector(8.0f, 1.3f, 0.18f), Assets.Basalt, FRotator(0.0f, -8.0f, 0.0f));
		SpawnTerrainShelf(World, TEXT("GateCauseway"), Assets.Cube, FVector(2360.0f, -220.0f, 30.0f), FVector(10.0f, 1.2f, 0.18f), Assets.Basalt, FRotator(0.0f, 3.0f, 0.0f));
		SpawnTerrainShelf(World, TEXT("CrashTrenchShelf"), Assets.Cube, FVector(250.0f, 0.0f, -150.0f), FVector(11.0f, 3.0f, 0.5f), Assets.AshStone, FRotator(0.0f, 20.0f, 0.0f));
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
		if (!SpawnPlayerStart(World, TerrainProfile.PrimarySpawnLocation, 5.0f, TerrainProfile))
		{
			UE_LOG(LogTemp, Error, TEXT("Opening map build failed due to invalid PlayerStart projection."));
		}

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

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("WitnessAnchor"),
			FText::FromString(TEXT("Speak to the witness")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("OpeningWitness"), 0), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("OpeningWitness"), TEXT("Camera.WitnessCamp"), TEXT("OpeningWitness"), TEXT("opening_survivor_named"), TEXT("named_witness"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(-1860.0f, 960.0f, 88.0f),
			-35.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("opening_side_01"),
			{ TEXT("Story.Prologue.Complete") },
			true,
			TerrainProfile);

		SpawnNpcOnTerrain(World, TEXT("WitnessAttendant"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Opening.Survivors"), 0), Assets.IdleAnimation), FVector(-2300.0f, 1200.0f, 88.0f), 35.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("OpeningHealer"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("OpeningHealer"), 1), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("OpeningHealer"), TEXT("Camera.WitnessCamp"), TEXT("OpeningHealer"), TEXT("opening_survivor_named"), TEXT("named_healer"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-2320.0f, 760.0f, 88.0f), 18.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("OpeningQuartermaster"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("OpeningQuartermaster"), 2), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("OpeningQuartermaster"), NAME_None, TEXT("OpeningQuartermaster"), TEXT("opening_guard_group"), TEXT("named_guard"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-1500.0f, 760.0f, 88.0f), -18.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("WitnessGuard"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Opening.Guards"), 1), Assets.IdleAnimation), FVector(-1450.0f, 560.0f, 88.0f), -10.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("CampSurvivor_02"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Opening.Survivors"), 2), Assets.IdleAnimation), FVector(-2460.0f, 1180.0f, 88.0f), 14.0f, TerrainProfile);
		SpawnAmbientClusterOnTerrain(World, TEXT("OpeningCampCrowd"), RoleTags.AmbientCivilian, Assets, TEXT("Opening.Survivors"), FVector(-2920.0f, 980.0f, 88.0f), { FVector(0.0f, 0.0f, 0.0f), FVector(180.0f, 140.0f, 0.0f), FVector(340.0f, -120.0f, 0.0f), FVector(520.0f, 100.0f, 0.0f) }, 25.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("OpeningGateGuards"), RoleTags.AmbientGuard, Assets, TEXT("Opening.Guards"), FVector(4200.0f, -180.0f, 88.0f), { FVector(0.0f, 0.0f, 0.0f), FVector(280.0f, 280.0f, 0.0f), FVector(320.0f, -320.0f, 0.0f) }, 180.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("FallenCrew_Scout"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("OpeningHealer"), 0), Assets.DeathBack), FVector(-420.0f, -520.0f, 88.0f), 65.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("FallenCrew_Officer"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("OpeningQuartermaster"), 1), Assets.DeathLeft), FVector(-760.0f, 680.0f, 88.0f), -55.0f, TerrainProfile);
		if (Assets.HumanFryPose)
		{
			SpawnNpcOnTerrain(World, TEXT("FallenCrew_Remnant"), MakeNpcProfile(RoleTags.NamedScenic, Assets.HumanFryPose, nullptr), FVector(420.0f, -760.0f, 88.0f), 20.0f, TerrainProfile);
		}

		SpawnTravelGateOnTerrain(
			World,
			TEXT("EgyptGate"),
			FText::FromString(TEXT("Travel to Egypt")),
			FVector(3200.0f, -220.0f, 150.0f),
			FVector(1.8f, 1.3f, 2.8f),
			EManyNamesRegionId::Egypt,
			PickMesh(Assets.MastabaEntrance, Assets.Cube),
			Assets.Bronze,
			{ TEXT("State.Region.Opening.Complete") },
			TerrainProfile);
		SpawnTravelGateOnTerrain(
			World,
			TEXT("GreeceGate"),
			FText::FromString(TEXT("Travel to Greece")),
			FVector(3500.0f, 820.0f, 160.0f),
			FVector(1.6f, 1.6f, 1.6f),
			EManyNamesRegionId::Greece,
			PickMesh(Assets.GreeceDolmen, Assets.Cube),
			nullptr,
			{ TEXT("State.Region.Opening.Complete") },
			TerrainProfile);
		SpawnTravelGateOnTerrain(
			World,
			TEXT("ItalicGate"),
			FText::FromString(TEXT("Travel to Italic West")),
			FVector(3300.0f, -1180.0f, 170.0f),
			FVector(2.0f, 2.0f, 2.0f),
			EManyNamesRegionId::ItalicWest,
			PickMesh(Assets.ItalicBollard, Assets.Cube),
			nullptr,
			{ TEXT("State.Region.Opening.Complete") },
			TerrainProfile);
	}

	void BuildEgyptMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Egypt, Assets, RoleTags);
		const FManyNamesEnvironmentProfile EnvironmentProfile = BuildEnvironmentProfile(EManyNamesRegionId::Egypt);
		const FManyNamesTerrainProfile TerrainProfile = BuildTerrainProfile(EManyNamesRegionId::Egypt, Assets);
		ValidateNaniteProfile(TerrainProfile);
		SpawnSky(World, EnvironmentProfile.BaselineState);
		SpawnCinematicPostProcess(World, EManyNamesRegionId::Egypt);
		SpawnEnvironmentController(World, EnvironmentProfile);
		BuildTerrainSystems(World, TerrainProfile);

		SpawnTerrainShelf(World, TEXT("EgyptArchiveRoad"), Assets.Cube, FVector(1320.0f, 0.0f, 26.0f), FVector(14.0f, 1.0f, 0.16f), Assets.Basalt);
		SpawnTerrainShelf(World, TEXT("EgyptTempleCourt"), Assets.Cube, FVector(0.0f, -1180.0f, 40.0f), FVector(14.0f, 4.5f, 0.2f), Assets.SandStone);
		SpawnTerrainShelf(World, TEXT("EgyptNecropolisShelf"), Assets.Cube, FVector(0.0f, 3520.0f, 10.0f), FVector(10.0f, 15.0f, 0.6f), Assets.SandStone);
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

		if (!SpawnPlayerStart(World, TerrainProfile.PrimarySpawnLocation, 0.0f, TerrainProfile))
		{
			UE_LOG(LogTemp, Error, TEXT("Egypt map build failed due to invalid PlayerStart projection."));
		}

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("ArchiveKeeper"),
			FText::FromString(TEXT("Speak with the archive keeper")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("ArchiveKeeper"), 1), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("ArchiveKeeper"), TEXT("Camera.ArchiveKeeper"), TEXT("ArchiveKeeper"), TEXT("egypt_priest_scholar_named"), TEXT("named_scholar"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(1660.0f, 0.0f, 88.0f),
			90.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("egypt_main_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true,
			TerrainProfile);

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("FloodplainPetitioner"),
			FText::FromString(TEXT("Hear the floodplain petition")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("FloodplainPetitioner"), 2), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("FloodplainPetitioner"), TEXT("Camera.FloodplainPetitioner"), TEXT("FloodplainPetitioner"), TEXT("egypt_floodplain_named"), TEXT("named_petitioner"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(-2620.0f, -950.0f, 88.0f),
			20.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("egypt_side_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true,
			TerrainProfile);

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("RiverScribe"),
			FText::FromString(TEXT("Inspect the river ledgers")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("RiverScribe"), 5), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("RiverScribe"), TEXT("Camera.RiverLedger"), TEXT("RiverScribe"), TEXT("egypt_scribe_named"), TEXT("named_scribe"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(-5980.0f, -2480.0f, 88.0f),
			42.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("egypt_side_02"),
			{ TEXT("State.Region.Egypt.Complete") },
			true,
			TerrainProfile);

		SpawnNpcOnTerrain(World, TEXT("TemplePriest_01"), MakeNpcProfile(RoleTags.AmbientRitual, ResolveAmbientCharacter(Assets, TEXT("Egypt.Priests"), 0), Assets.IdleAnimation), FVector(-360.0f, -520.0f, 88.0f), 20.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("TemplePriest_02"), MakeNpcProfile(RoleTags.AmbientRitual, ResolveAmbientCharacter(Assets, TEXT("Egypt.Priests"), 1), Assets.IdleAnimation), FVector(420.0f, -640.0f, 88.0f), -25.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("ArchiveScribe_01"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Scribes"), 2), Assets.IdleAnimation), FVector(2220.0f, -280.0f, 88.0f), -90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("ArchiveScribe_02"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Scribes"), 3), Assets.IdleAnimation), FVector(2300.0f, 320.0f, 88.0f), -100.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("MarketVendor_01"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Vendors"), 4), Assets.IdleAnimation), FVector(-2500.0f, -1400.0f, 88.0f), 90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("MarketVendor_02"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Vendors"), 5), Assets.IdleAnimation), FVector(-2400.0f, 700.0f, 88.0f), -90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("TempleGuard_01"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Egypt.Guards"), 6), Assets.IdleAnimation), FVector(1050.0f, -860.0f, 88.0f), 180.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("TempleGuard_02"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Egypt.Guards"), 7), Assets.IdleAnimation), FVector(1050.0f, 860.0f, 88.0f), 180.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("NecropolisWatcher"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Egypt.Guards"), 8), Assets.IdleAnimation), FVector(0.0f, 3650.0f, 88.0f), 180.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Citizen_01"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Vendors"), 9), Assets.IdleAnimation), FVector(-1200.0f, 220.0f, 88.0f), 15.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Citizen_02"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Vendors"), 10), Assets.IdleAnimation), FVector(-900.0f, -40.0f, 88.0f), 120.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Citizen_03"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Egypt.Vendors"), 11), Assets.IdleAnimation), FVector(-1820.0f, 980.0f, 88.0f), -60.0f, TerrainProfile);
		SpawnAmbientClusterOnTerrain(World, TEXT("EgyptMarketCrowd"), RoleTags.AmbientCivilian, Assets, TEXT("Egypt.Vendors"), FVector(-3000.0f, -220.0f, 88.0f), { FVector(0.0f, 0.0f, 0.0f), FVector(260.0f, 520.0f, 0.0f), FVector(260.0f, -520.0f, 0.0f), FVector(480.0f, 860.0f, 0.0f), FVector(520.0f, -860.0f, 0.0f), FVector(760.0f, 180.0f, 0.0f) }, 90.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("EgyptTempleCrowd"), RoleTags.AmbientRitual, Assets, TEXT("Egypt.Priests"), FVector(0.0f, -420.0f, 88.0f), { FVector(-620.0f, -120.0f, 0.0f), FVector(620.0f, -120.0f, 0.0f), FVector(-420.0f, 220.0f, 0.0f), FVector(420.0f, 220.0f, 0.0f) }, 180.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("EgyptAdjudicatorCourt"), RoleTags.AmbientCivilian, Assets, TEXT("Egypt.Adjudicators"), FVector(-880.0f, -1100.0f, 88.0f), { FVector(-240.0f, 40.0f, 0.0f), FVector(220.0f, -20.0f, 0.0f), FVector(20.0f, 260.0f, 0.0f), FVector(-420.0f, 240.0f, 0.0f) }, 25.0f, TerrainProfile, 88.0f, true);
		SpawnNpcOnTerrain(World, TEXT("TempleCantor"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("TempleCantor"), 3), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("TempleCantor"), NAME_None, TEXT("TempleCantor"), TEXT("egypt_priest_group"), TEXT("named_ritual"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-260.0f, -760.0f, 88.0f), 10.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("GranarySteward"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("GranarySteward"), 4), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("GranarySteward"), NAME_None, TEXT("GranarySteward"), TEXT("egypt_market_group"), TEXT("named_steward"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-2120.0f, -1240.0f, 88.0f), 85.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("TempleAdjudicator"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("TempleAdjudicator"), 6), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("TempleAdjudicator"), NAME_None, TEXT("TempleAdjudicator"), TEXT("egypt_adjudicator_named"), TEXT("named_adjudicator"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-840.0f, -960.0f, 88.0f), 20.0f, TerrainProfile);
	}

	void BuildGreeceMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Greece, Assets, RoleTags);
		const FManyNamesEnvironmentProfile EnvironmentProfile = BuildEnvironmentProfile(EManyNamesRegionId::Greece);
		const FManyNamesTerrainProfile TerrainProfile = BuildTerrainProfile(EManyNamesRegionId::Greece, Assets);
		ValidateNaniteProfile(TerrainProfile);
		SpawnSky(World, EnvironmentProfile.BaselineState);
		SpawnCinematicPostProcess(World, EManyNamesRegionId::Greece);
		SpawnEnvironmentController(World, EnvironmentProfile);
		BuildTerrainSystems(World, TerrainProfile);

		SpawnTerrainShelf(World, TEXT("GreeceArrivalShelf"), Assets.Cube, FVector(-6200.0f, -120.0f, 36.0f), FVector(9.0f, 2.4f, 0.24f), Assets.Basalt);
		SpawnTerrainShelf(World, TEXT("SanctuaryCourt"), Assets.Cube, FVector(0.0f, -180.0f, 70.0f), FVector(10.0f, 7.0f, 0.8f), Assets.Plaster);
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

		if (!SpawnPlayerStart(World, TerrainProfile.PrimarySpawnLocation, 15.0f, TerrainProfile))
		{
			UE_LOG(LogTemp, Error, TEXT("Greece map build failed due to invalid PlayerStart projection."));
		}

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("StormHerald"),
			FText::FromString(TEXT("Confront the herald of the storm ruler")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("StormHerald"), 0), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("StormHerald"), TEXT("Camera.StormHerald"), TEXT("StormHerald"), TEXT("greece_ritual_named"), TEXT("named_herald"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(180.0f, -420.0f, 88.0f),
			180.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("greece_main_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true,
			TerrainProfile);

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("WarbandEnvoy"),
			FText::FromString(TEXT("Answer the warband envoy")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("WarbandEnvoy"), 1), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("WarbandEnvoy"), TEXT("Camera.WarbandEnvoy"), TEXT("WarbandEnvoy"), TEXT("greece_warband_named"), TEXT("named_envoy"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(2700.0f, 2080.0f, 88.0f),
			-140.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("greece_side_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true,
			TerrainProfile);

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("OathBearer"),
			FText::FromString(TEXT("Hear the oath-bearers")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("OathBearer"), 5), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("OathBearer"), TEXT("Camera.OathShelter"), TEXT("OathBearer"), TEXT("greece_oath_named"), TEXT("named_oath"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(-1860.0f, 540.0f, 88.0f),
			120.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("greece_side_02"),
			{ TEXT("State.Region.Greece.Complete") },
			true,
			TerrainProfile);

		SpawnNpcOnTerrain(World, TEXT("RitualSinger_01"), MakeNpcProfile(RoleTags.AmbientRitual, ResolveAmbientCharacter(Assets, TEXT("Greece.Heralds"), 0), Assets.IdleAnimation), FVector(-420.0f, -360.0f, 88.0f), 110.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("RitualSinger_02"), MakeNpcProfile(RoleTags.AmbientRitual, ResolveAmbientCharacter(Assets, TEXT("Greece.Heralds"), 1), Assets.IdleAnimation), FVector(420.0f, -350.0f, 88.0f), -110.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Guard_Cliff"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Greece.Guards"), 2), Assets.IdleAnimation), FVector(2350.0f, 1860.0f, 88.0f), -90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Guard_Court"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Greece.Guards"), 3), Assets.IdleAnimation), FVector(-1000.0f, 180.0f, 88.0f), 30.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Pilgrim_01"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Greece.Pilgrims"), 4), Assets.IdleAnimation), FVector(-1500.0f, -220.0f, 88.0f), 90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Pilgrim_02"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Greece.Pilgrims"), 5), Assets.IdleAnimation), FVector(1100.0f, 220.0f, 88.0f), -90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("Pilgrim_03"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Greece.Pilgrims"), 6), Assets.IdleAnimation), FVector(620.0f, 640.0f, 88.0f), 160.0f, TerrainProfile);
		SpawnAmbientClusterOnTerrain(World, TEXT("GreeceSanctuaryCrowd"), RoleTags.AmbientCivilian, Assets, TEXT("Greece.Pilgrims"), FVector(-240.0f, -120.0f, 88.0f), { FVector(-820.0f, 160.0f, 0.0f), FVector(-420.0f, 380.0f, 0.0f), FVector(240.0f, 420.0f, 0.0f), FVector(760.0f, 160.0f, 0.0f), FVector(320.0f, -260.0f, 0.0f) }, 90.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("GreeceRitualChorus"), RoleTags.AmbientRitual, Assets, TEXT("Greece.Heralds"), FVector(0.0f, -760.0f, 88.0f), { FVector(-360.0f, 0.0f, 0.0f), FVector(360.0f, 0.0f, 0.0f), FVector(-180.0f, 220.0f, 0.0f), FVector(180.0f, 220.0f, 0.0f) }, 170.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("GreeceOathkeepers"), RoleTags.AmbientRitual, Assets, TEXT("Greece.Oathkeepers"), FVector(-1320.0f, 420.0f, 88.0f), { FVector(-260.0f, 0.0f, 0.0f), FVector(260.0f, 0.0f, 0.0f), FVector(0.0f, 240.0f, 0.0f), FVector(0.0f, -240.0f, 0.0f) }, 150.0f, TerrainProfile, 88.0f, true);
		SpawnNpcOnTerrain(World, TEXT("SanctuaryKeeper"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("SanctuaryKeeper"), 3), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("SanctuaryKeeper"), NAME_None, TEXT("SanctuaryKeeper"), TEXT("greece_ritual_group"), TEXT("named_keeper"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-620.0f, -120.0f, 88.0f), 40.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("WarSinger"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("WarSinger"), 4), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("WarSinger"), NAME_None, TEXT("WarSinger"), TEXT("greece_pilgrim_group"), TEXT("named_ritual"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(840.0f, -180.0f, 88.0f), -40.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("HillOracle"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("HillOracle"), 6), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("HillOracle"), NAME_None, TEXT("HillOracle"), TEXT("greece_oracle_named"), TEXT("named_oracle"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(3120.0f, 2960.0f, 88.0f), -120.0f, TerrainProfile);
	}

	void BuildItalicMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::ItalicWest, Assets, RoleTags);
		const FManyNamesEnvironmentProfile EnvironmentProfile = BuildEnvironmentProfile(EManyNamesRegionId::ItalicWest);
		const FManyNamesTerrainProfile TerrainProfile = BuildTerrainProfile(EManyNamesRegionId::ItalicWest, Assets);
		ValidateNaniteProfile(TerrainProfile);
		SpawnSky(World, EnvironmentProfile.BaselineState);
		SpawnCinematicPostProcess(World, EManyNamesRegionId::ItalicWest);
		SpawnEnvironmentController(World, EnvironmentProfile);
		BuildTerrainSystems(World, TerrainProfile);

		SpawnTerrainShelf(World, TEXT("ItalicArrivalShelf"), Assets.Cube, FVector(-6200.0f, -360.0f, 48.0f), FVector(9.0f, 2.6f, 0.3f), Assets.Basalt);
		SpawnTerrainShelf(World, TEXT("HillSettlement"), Assets.Cube, FVector(-1200.0f, -600.0f, 120.0f), FVector(8.0f, 6.0f, 1.4f), Assets.Plaster);
		SpawnTerrainShelf(World, TEXT("ForgeLawChamber"), Assets.Cube, FVector(1800.0f, -200.0f, 220.0f), FVector(6.5f, 5.0f, 2.0f), Assets.Basalt);
		SpawnTerrainShelf(World, TEXT("RitualRoad"), Assets.Cube, FVector(250.0f, 1600.0f, 50.0f), FVector(17.0f, 2.0f, 0.25f), Assets.Basalt, FRotator(0.0f, 16.0f, 0.0f));
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
		if (!SpawnPlayerStart(World, TerrainProfile.PrimarySpawnLocation, 0.0f, TerrainProfile))
		{
			UE_LOG(LogTemp, Error, TEXT("Italic map build failed due to invalid PlayerStart projection."));
		}

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("MeasureKeeper"),
			FText::FromString(TEXT("Confront the keeper of measures")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("MeasureKeeper"), 1), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("MeasureKeeper"), TEXT("Camera.MeasureKeeper"), TEXT("MeasureKeeper"), TEXT("italic_civic_named"), TEXT("named_lawgiver"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(1680.0f, -80.0f, 88.0f),
			90.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("italic_main_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true,
			TerrainProfile);

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("BoundaryElder"),
			FText::FromString(TEXT("Hear the boundary dispute")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("BoundaryElder"), 2), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("BoundaryElder"), TEXT("Camera.BoundaryElder"), TEXT("BoundaryElder"), TEXT("italic_elder_named"), TEXT("named_elder"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(2420.0f, 1540.0f, 88.0f),
			-120.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("italic_side_01"),
			{ TEXT("State.Region.Opening.Complete") },
			true,
			TerrainProfile);

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("RoadMagistrate"),
			FText::FromString(TEXT("Judge the road's ownership")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("RoadMagistrate"), 5), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("RoadMagistrate"), TEXT("Camera.RoadMeasure"), TEXT("RoadMagistrate"), TEXT("italic_magistrate_named"), TEXT("named_magistrate"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(120.0f, 1540.0f, 88.0f),
			80.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("italic_side_02"),
			{ TEXT("State.Region.ItalicWest.Complete") },
			true,
			TerrainProfile);

		SpawnNpcOnTerrain(World, TEXT("ForgeGuard_01"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Italic.Guards"), 0), Assets.IdleAnimation), FVector(1080.0f, -420.0f, 88.0f), 45.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("ForgeGuard_02"), MakeNpcProfile(RoleTags.AmbientGuard, ResolveAmbientCharacter(Assets, TEXT("Italic.Guards"), 1), Assets.IdleAnimation), FVector(1100.0f, 300.0f, 88.0f), -45.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("RoadWorker_01"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Italic.Workers"), 2), Assets.IdleAnimation), FVector(-640.0f, 1380.0f, 88.0f), 90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("RoadWorker_02"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Italic.Workers"), 3), Assets.IdleAnimation), FVector(220.0f, 1760.0f, 88.0f), -90.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("RitualWitness"), MakeNpcProfile(RoleTags.AmbientRitual, ResolveAmbientCharacter(Assets, TEXT("Italic.Ritual"), 4), Assets.IdleAnimation), FVector(2080.0f, 1180.0f, 88.0f), 180.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("HillCitizen"), MakeNpcProfile(RoleTags.AmbientCivilian, ResolveAmbientCharacter(Assets, TEXT("Italic.Workers"), 5), Assets.IdleAnimation), FVector(-1420.0f, -460.0f, 88.0f), 0.0f, TerrainProfile);
		SpawnAmbientClusterOnTerrain(World, TEXT("ItalicSettlementCrowd"), RoleTags.AmbientCivilian, Assets, TEXT("Italic.Workers"), FVector(-1220.0f, -620.0f, 88.0f), { FVector(-620.0f, 140.0f, 0.0f), FVector(-180.0f, 360.0f, 0.0f), FVector(240.0f, 320.0f, 0.0f), FVector(620.0f, 180.0f, 0.0f) }, 25.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("ItalicBoundaryCrowd"), RoleTags.AmbientRitual, Assets, TEXT("Italic.Ritual"), FVector(2320.0f, 1480.0f, 88.0f), { FVector(-320.0f, 0.0f, 0.0f), FVector(320.0f, 0.0f, 0.0f), FVector(-140.0f, 220.0f, 0.0f), FVector(160.0f, 220.0f, 0.0f) }, 180.0f, TerrainProfile, 88.0f, true);
		SpawnAmbientClusterOnTerrain(World, TEXT("ItalicMagistrateCourt"), RoleTags.AmbientCivilian, Assets, TEXT("Italic.Magistrates"), FVector(-60.0f, 1580.0f, 88.0f), { FVector(-260.0f, 0.0f, 0.0f), FVector(260.0f, 0.0f, 0.0f), FVector(0.0f, 240.0f, 0.0f) }, 120.0f, TerrainProfile, 88.0f, true);
		SpawnNpcOnTerrain(World, TEXT("ForgeMatron"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("ForgeMatron"), 3), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("ForgeMatron"), NAME_None, TEXT("ForgeMatron"), TEXT("italic_worker_group"), TEXT("named_worker"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(1420.0f, 120.0f, 88.0f), 70.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("RoadSurveyor"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("RoadSurveyor"), 4), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("RoadSurveyor"), NAME_None, TEXT("RoadSurveyor"), TEXT("italic_guard_group"), TEXT("named_worker"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(-180.0f, 1480.0f, 88.0f), 95.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("ForgeApprentice"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("ForgeApprentice"), 6), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("ForgeApprentice"), NAME_None, TEXT("ForgeApprentice"), TEXT("italic_forge_named"), TEXT("named_apprentice"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(980.0f, 220.0f, 88.0f), 30.0f, TerrainProfile);
	}

	void BuildConvergenceMap(UWorld* World, const FManyNamesBuildAssets& Assets, const FManyNamesRoleTags& RoleTags)
	{
		ClearWorld(World);
		const FManyNamesRegionArtProfile ArtProfile = BuildRegionArtProfile(EManyNamesRegionId::Convergence, Assets, RoleTags);
		const FManyNamesEnvironmentProfile EnvironmentProfile = BuildEnvironmentProfile(EManyNamesRegionId::Convergence);
		const FManyNamesTerrainProfile TerrainProfile = BuildTerrainProfile(EManyNamesRegionId::Convergence, Assets);
		ValidateNaniteProfile(TerrainProfile);
		SpawnSky(World, EnvironmentProfile.BaselineState);
		SpawnCinematicPostProcess(World, EManyNamesRegionId::Convergence);
		SpawnEnvironmentController(World, EnvironmentProfile);
		BuildTerrainSystems(World, TerrainProfile);

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

		if (!SpawnPlayerStart(World, TerrainProfile.PrimarySpawnLocation, 0.0f, TerrainProfile))
		{
			UE_LOG(LogTemp, Error, TEXT("Convergence map build failed due to invalid PlayerStart projection."));
		}

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

		SpawnInteractableNpcOnTerrain(
			World,
			TEXT("BurialCustodian"),
			FText::FromString(TEXT("Question the burial custodians")),
			MakeNpcProfile(RoleTags.NamedInteractable, ResolveNamedCharacter(Assets, TEXT("BurialCustodian"), 5), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("BurialCustodian"), TEXT("Camera.ChorusVault"), TEXT("BurialCustodian"), TEXT("convergence_custodian_named"), TEXT("named_custodian"), EManyNamesCrowdBehaviorTier::StoryFacing, true),
			FVector(-620.0f, 2460.0f, 88.0f),
			35.0f,
			EManyNamesInteractionActionType::QuestDialogue,
			TEXT("convergence_side_02"),
			{ TEXT("State.Region.Convergence.Complete") },
			true,
			TerrainProfile);

		SpawnNpcOnTerrain(World, TEXT("BridgeWatcher"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("BridgeWatcher"), 0), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("BridgeWatcher"), TEXT("Camera.BridgeWatcher")), FVector(-460.0f, 2000.0f, 88.0f), 30.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("SystemsRemnant"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("SystemsRemnant"), 1), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("SystemsRemnant"), TEXT("Camera.SystemsRemnant")), FVector(540.0f, 1950.0f, 88.0f), -30.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("CompanionChorus"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("CompanionChorus"), 2), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("CompanionChorus"), NAME_None, TEXT("CompanionChorus"), TEXT("convergence_remnant_group"), TEXT("named_uncanny"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(0.0f, 1760.0f, 88.0f), 180.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("SignalArchivist"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("SignalArchivist"), 6), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("SignalArchivist"), NAME_None, TEXT("SignalArchivist"), TEXT("convergence_archivist_named"), TEXT("named_archivist"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(680.0f, 2560.0f, 88.0f), -150.0f, TerrainProfile);
		SpawnNpcOnTerrain(World, TEXT("LoyalistRemnant"), MakeNpcProfile(RoleTags.NamedScenic, ResolveNamedCharacter(Assets, TEXT("LoyalistRemnant"), 7), Assets.IdleAnimation, nullptr, FVector(1.0f), FVector(0.0f, 0.0f, -88.0f), true, TEXT("LoyalistRemnant"), NAME_None, TEXT("LoyalistRemnant"), TEXT("convergence_loyalist_named"), TEXT("named_loyalist"), EManyNamesCrowdBehaviorTier::StoryFacing, true), FVector(420.0f, 1420.0f, 88.0f), 140.0f, TerrainProfile);
		SpawnAmbientClusterOnTerrain(World, TEXT("ConvergenceRemnants"), RoleTags.AmbientCivilian, Assets, TEXT("Convergence.Remnants"), FVector(0.0f, 2080.0f, 88.0f), { FVector(-960.0f, -120.0f, 0.0f), FVector(920.0f, -80.0f, 0.0f), FVector(-620.0f, 260.0f, 0.0f) }, 180.0f, TerrainProfile);
		SpawnAmbientClusterOnTerrain(World, TEXT("ConvergenceCustodians"), RoleTags.AmbientRitual, Assets, TEXT("Convergence.Custodians"), FVector(-260.0f, 2520.0f, 88.0f), { FVector(-220.0f, 0.0f, 0.0f), FVector(220.0f, 0.0f, 0.0f), FVector(0.0f, 260.0f, 0.0f) }, 180.0f, TerrainProfile, 88.0f, true);
	}

	bool SaveBuiltMap(UWorld* World, const FString& MapPath)
	{
		return World && UEditorLoadingAndSavingUtils::SaveMap(World, MapPath);
	}

	bool ValidatePlayerStartCount(UWorld* World, const FManyNamesTerrainProfile& TerrainProfile)
	{
		if (!World)
		{
			return false;
		}

		int32 PlayerStartCount = 0;
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			++PlayerStartCount;
			const FManyNamesSpawnValidationResult Projection = ProjectLocationToGround(World, It->GetActorLocation(), TerrainProfile);
			if (!Projection.bHasGround)
			{
				UE_LOG(LogTemp, Error, TEXT("PlayerStart %s in region %d is not over blocking ground."), *It->GetActorLabel(), static_cast<int32>(TerrainProfile.RegionId));
				return false;
			}
		}

		if (PlayerStartCount != 1)
		{
			UE_LOG(LogTemp, Error, TEXT("Expected exactly one PlayerStart in region %d, found %d."), static_cast<int32>(TerrainProfile.RegionId), PlayerStartCount);
			return false;
		}

		return true;
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
	// Suppress the editor home-screen browser during headless/commandlet runs so it does
	// not produce spurious warnings.  Setting this here (rather than in DefaultEngine.ini)
	// keeps the home screen available for normal interactive editor sessions.
	if (IConsoleVariable* HomeScreenCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("HomeScreen.EnableHomeScreen")))
	{
		HomeScreenCVar->Set(0, ECVF_SetByCode);
	}

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
	if (!ValidatePlayerStartCount(OpeningWorld, BuildTerrainProfile(EManyNamesRegionId::Opening, Assets)))
	{
		return 1;
	}
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
	if (!ValidatePlayerStartCount(EgyptWorld, BuildTerrainProfile(EManyNamesRegionId::Egypt, Assets)))
	{
		return 1;
	}
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
	if (!ValidatePlayerStartCount(GreeceWorld, BuildTerrainProfile(EManyNamesRegionId::Greece, Assets)))
	{
		return 1;
	}
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
	if (!ValidatePlayerStartCount(ItalicWorld, BuildTerrainProfile(EManyNamesRegionId::ItalicWest, Assets)))
	{
		return 1;
	}
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
	if (!ValidatePlayerStartCount(ConvergenceWorld, BuildTerrainProfile(EManyNamesRegionId::Convergence, Assets)))
	{
		return 1;
	}
	if (!SaveBuiltMap(ConvergenceWorld, TEXT("/Game/Maps/L_Convergence")))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Convergence map."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("ManyNames world build completed for Opening, Egypt, Greece, Italic West, and Convergence."));
	return 0;
}
