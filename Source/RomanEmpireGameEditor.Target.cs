// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RomanEmpireGameEditorTarget : TargetRules
{
	public RomanEmpireGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("RomanEmpireGame");
	}
}
