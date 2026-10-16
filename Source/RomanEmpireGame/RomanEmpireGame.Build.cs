// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;

public class RomanEmpireGame : ModuleRules
{
	public RomanEmpireGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;
		PrivateIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"EnhancedInput",
			"InputCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AnimGraphRuntime", // Cooked blend/evaluator/slot nodes; Editor graph tools stay test-only.
			"AIModule",
			"NavigationSystem",
			"GameplayTags",
			"Slate",
			"SlateCore",
			"HTTP",
			"Json"
		});
	}
}
