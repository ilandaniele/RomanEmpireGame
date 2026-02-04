// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;

public class RomanEmpireGame : ModuleRules
{
	public RomanEmpireGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Enable IWYU for cleaner includes
		bEnforceIWYU = false;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"UMG"
		});

		// Uncomment if using procedural mesh
		// PublicDependencyModuleNames.Add("ProceduralMeshComponent");
		
		// Uncomment if using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}
