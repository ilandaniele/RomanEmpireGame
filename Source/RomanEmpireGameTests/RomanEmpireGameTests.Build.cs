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
			"AIModule",
			"NavigationSystem",
			"RomanEmpireGame",
			"UMG",
			"Slate",
			"SlateCore",
			"RenderCore", // Headless projection viewport fixture; Editor-only module.
			"InputCore",
			"ApplicationCore",
			"EnhancedInput",
			"UnrealEd"
		});
	}
}
