// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;

public class RomanEmpireGame : ModuleRules
{
	public RomanEmpireGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Disable IWYU (Include What You Use) enforcement
		IWYUSupport = IWYUSupport.None;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore"
		});

		// Uncomment if using procedural mesh
		PublicDependencyModuleNames.Add("ProceduralMeshComponent");
		
		// Uncomment if using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}
