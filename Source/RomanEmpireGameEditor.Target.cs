// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RomanEmpireGameEditorTarget : TargetRules
{
	public RomanEmpireGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "RomanEmpireGame", "RomanEmpireGameTests" });
	}
}
