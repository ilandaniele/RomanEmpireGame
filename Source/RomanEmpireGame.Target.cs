// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RomanEmpireGameTarget : TargetRules
{
	public RomanEmpireGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("RomanEmpireGame");
	}
}
