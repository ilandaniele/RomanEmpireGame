// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;

public class RomanEmpireGameTests : ModuleRules
{
	public RomanEmpireGameTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RomanEmpireGame",
			"UMG",
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd"
		});
	}
}
