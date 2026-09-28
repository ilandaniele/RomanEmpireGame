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
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AIModule",
			"NavigationSystem",
			"InputCore",
			"Slate",
			"SlateCore",
			"HTTP",
			"Json",
			"ProceduralMeshComponent"
		});
	}
}
