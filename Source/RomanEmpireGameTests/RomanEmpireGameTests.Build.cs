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
			"Json", // Native private art-integration receipts; Editor test module only.
			"Engine",
			"AIModule",
			"NavigationSystem",
			"RomanEmpireGame",
			"UMG",
			"Slate",
			"SlateCore",
			"RenderCore", // Headless projection viewport fixture; Editor-only module.
			"ImageCore", // Real player pixel buffer ownership; Editor-only capture fixture.
			"ScreenShotComparisonTools", // Native reviewed-baseline comparison; Editor-only.
			"RHI", // Verify the actual baseline renderer, not a command-line assumption.
			"InputCore",
			"ApplicationCore",
			"EnhancedInput",
			"GameplayTags",
			"UnrealEd",
            "FunctionalTesting",
            "AssetTools",
            "DeveloperToolSettings",
			"AnimGraph", // Explicit opt-in Editor-only animation asset authoring/graph verification.
			"AnimGraphRuntime",
			"BlueprintGraph",
			"KismetCompiler",
			"AssetRegistry"
		});
	}
}
