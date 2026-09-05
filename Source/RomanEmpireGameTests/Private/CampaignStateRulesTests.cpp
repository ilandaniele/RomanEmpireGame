// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Camera/ZoomConfiguration.h"
#include "Campaign/CampaignTurn.h"
#include "Core/GameplayMode.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCampaignTurnPipelineOrderTest,
	"RomanEmpireGame.Rules.Campaign.CanonicalTurnPipelineOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCampaignTurnPipelineOrderTest::RunTest(const FString& Parameters)
{
	const TArray<ECampaignTurnStep>& Pipeline = RomanEmpireCampaign::GetCanonicalTurnPipeline();
	const TArray<ECampaignTurnStep> Expected = {
		ECampaignTurnStep::ValidateRequest,
		ECampaignTurnStep::Production,
		ECampaignTurnStep::Upkeep,
		ECampaignTurnStep::AIActions,
		ECampaignTurnStep::ConflictResolution,
		ECampaignTurnStep::VictoryEvaluation,
		ECampaignTurnStep::IncrementTurn,
		ECampaignTurnStep::BroadcastTurnCompleted,
		ECampaignTurnStep::Autosave
	};

	TestEqual(TEXT("Pipeline contains every required step exactly once"), Pipeline.Num(), Expected.Num());
	for (int32 Index = 0; Index < FMath::Min(Pipeline.Num(), Expected.Num()); ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("Pipeline step %d is ordered"), Index),
			static_cast<uint8>(Pipeline[Index]),
			static_cast<uint8>(Expected[Index]));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGameplayModeAndZoomConfigurationTest,
	"RomanEmpireGame.Rules.Camera.CanonicalZoomConfigurationAndModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FGameplayModeAndZoomConfigurationTest::RunTest(const FString& Parameters)
{
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	TestTrue(TEXT("Default zoom configuration is internally ordered"), Zoom.IsValid());
	TestEqual(TEXT("Minimum is world view"), Zoom.Classify(Zoom.Minimum), EZoomLevel::World);
	TestEqual(TEXT("World boundary is inclusive"), Zoom.Classify(Zoom.WorldMaximum), EZoomLevel::World);
	TestEqual(TEXT("Territory boundary is strategic"), Zoom.Classify(Zoom.StrategicMaximum), EZoomLevel::Territory);
	TestEqual(TEXT("Default framing is tactical city view"), Zoom.Classify(Zoom.DefaultTactical), EZoomLevel::City);
	TestEqual(TEXT("Ground boundary is ground view"), Zoom.Classify(Zoom.GroundMaximum), EZoomLevel::Ground);
	TestEqual(TEXT("Maximum is first-person framing"), Zoom.Classify(Zoom.Maximum), EZoomLevel::FirstPerson);

	TestEqual(
		TEXT("World framing recommends strategic exploration"),
		RomanEmpireGameplay::GetExplorationModeForZoomLevel(EZoomLevel::World),
		EGameplayMode::Strategic);
	TestEqual(
		TEXT("City framing recommends tactical exploration"),
		RomanEmpireGameplay::GetExplorationModeForZoomLevel(EZoomLevel::City),
		EGameplayMode::Tactical);
	TestEqual(
		TEXT("First-person framing alone cannot enter possessed mode"),
		RomanEmpireGameplay::GetExplorationModeForZoomLevel(EZoomLevel::FirstPerson),
		EGameplayMode::Tactical);
	TestFalse(
		TEXT("Possessed mode is not an exploration mode inferred from zoom"),
		RomanEmpireGameplay::IsExplorationMode(EGameplayMode::Possessed));

	return true;
}

#endif
