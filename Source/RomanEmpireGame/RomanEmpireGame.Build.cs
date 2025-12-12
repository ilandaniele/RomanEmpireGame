// Copyright Roman Empire Game. All Rights Reserved.

using UnrealBuildTool;

public class RomanEmpireGame : ModuleRules
{
	public RomanEmpireGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"UMG",
			"Slate",
			"SlateCore",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Landscape",
			"PhysicsCore",
			"RenderCore"
		});

		// Uncomment if using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}
