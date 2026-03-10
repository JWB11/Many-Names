using UnrealBuildTool;
using System.Collections.Generic;

public class ManyNamesTarget : TargetRules
{
	public ManyNamesTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ManyNames");
	}
}
