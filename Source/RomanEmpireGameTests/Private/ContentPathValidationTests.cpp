// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/SoftObjectPath.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRequiredContentPathValidationTest,
	"RomanEmpireGame.Content.RequiredPathsAndMapDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FRequiredContentPathValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	auto TestClassPath = [this](const FString& Label, const UClass* ActualClass, const TCHAR* ExpectedPath)
	{
		if (TestNotNull(*Label, ActualClass))
		{
			TestEqual(*Label, ActualClass->GetPathName(), FString(ExpectedPath));
		}
	};

	const TArray<FSoftObjectPath> RequiredAssets = {
		FSoftObjectPath(TEXT("/Game/Maps/L_Testbed.L_Testbed")),
		FSoftObjectPath(TEXT("/Game/Maps/L_VerticalSlice.L_VerticalSlice")),
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Plane.Plane")),
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")),
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")),
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")),
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Cone.Cone")),
		FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"))
	};

	for (const FSoftObjectPath& AssetPath : RequiredAssets)
	{
		UObject* LoadedAsset = AssetPath.TryLoad();
		TestNotNull(*FString::Printf(TEXT("Required content path resolves: %s"), *AssetPath.ToString()), LoadedAsset);
	}

	for (const TCHAR* MapPath : {
		TEXT("/Game/Maps/L_Testbed.L_Testbed"),
		TEXT("/Game/Maps/L_VerticalSlice.L_VerticalSlice")
	})
	{
		const UWorld* World = Cast<UWorld>(FSoftObjectPath(MapPath).TryLoad());
		if (!TestNotNull(*FString::Printf(TEXT("Map loads as a UWorld: %s"), MapPath), World))
		{
			continue;
		}

		const AWorldSettings* WorldSettings = World->GetWorldSettings();
		TestNotNull(*FString::Printf(TEXT("Map has WorldSettings: %s"), MapPath), WorldSettings);
		if (WorldSettings)
		{
			TestClassPath(
				FString::Printf(TEXT("Map explicitly selects ARomanEmpireGameMode: %s"), MapPath),
				WorldSettings->DefaultGameMode.Get(),
				TEXT("/Script/RomanEmpireGame.RomanEmpireGameMode"));
			TestTrue(
				*FString::Printf(TEXT("Map uses dynamic lighting: %s"), MapPath),
				WorldSettings->bForceNoPrecomputedLighting);
		}
	}

	const UClass* GameModeClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.RomanEmpireGameMode")).TryLoadClass<AGameModeBase>();
	if (!TestNotNull(TEXT("ARomanEmpireGameMode reflected class resolves"), GameModeClass))
	{
		return false;
	}

	const AGameModeBase* GameModeDefaults = Cast<AGameModeBase>(GameModeClass->GetDefaultObject());
	if (!TestNotNull(TEXT("ARomanEmpireGameMode class default object resolves"), GameModeDefaults))
	{
		return false;
	}

	TestClassPath(
		TEXT("GameMode explicitly selects its pawn"),
		GameModeDefaults->DefaultPawnClass.Get(),
		TEXT("/Script/RomanEmpireGame.SeamlessZoomCamera"));
	TestClassPath(
		TEXT("GameMode explicitly selects its player controller"),
		GameModeDefaults->PlayerControllerClass.Get(),
		TEXT("/Script/RomanEmpireGame.RomanEmpirePlayerController"));
	TestClassPath(
		TEXT("GameMode explicitly selects its game state"),
		GameModeDefaults->GameStateClass.Get(),
		TEXT("/Script/RomanEmpireGame.RomanEmpireGameState"));
	TestClassPath(
		TEXT("GameMode explicitly selects its HUD"),
		GameModeDefaults->HUDClass.Get(),
		TEXT("/Script/RomanEmpireGame.RomanEmpireHUD"));

	return true;
}

#endif
