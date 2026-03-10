using UnrealBuildTool;
using System.Collections.Generic;

public class ManyNamesEditorTarget : TargetRules
{
	public ManyNamesEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ManyNames");
	}
}
