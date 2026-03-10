using UnrealBuildTool;

public class ManyNames : ModuleRules
{
	public ManyNames(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"DeveloperSettings",
			"EnhancedInput",
			"GameplayTags",
			"Json",
			"JsonUtilities",
			"UMG"
		});

			PrivateDependencyModuleNames.AddRange(new[]
			{
				"Slate",
				"SlateCore"
			});

			if (Target.Type == TargetType.Editor)
			{
				PrivateDependencyModuleNames.AddRange(new[]
				{
					"UnrealEd",
					"AssetRegistry"
				});
			}
		}
	}
