// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Camera/ZoomConfiguration.h"
#include "Campaign/CampaignTurn.h"
#include "Campaign/TerritoryData.h"
#include "Core/GameplayMode.h"
#include "Faction/DiplomacyState.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTerritoryCampaignStateTest,
	"RomanEmpireGame.Rules.Campaign.CanonicalTerritoryOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FTerritoryCampaignStateTest::RunTest(const FString& Parameters)
{
	FTerritoryDefinition Rome;
	Rome.TerritoryID = TEXT("Rome");
	Rome.DisplayName = FText::FromString(TEXT("Italia"));
	Rome.StartingOwner = EFactionID::Rome;
	Rome.ResourceProduction.Gold = 50;

	FTerritoryCampaignState State;
	const FTerritoryRegistrationResult Registration = State.RegisterTerritory(Rome);
	TestTrue(TEXT("A complete stable definition registers"), Registration.WasSuccessful());
	TestEqual(TEXT("One definition creates one ownership entry"), State.GetTerritoryCount(), 1);

	EFactionID Owner = EFactionID::None;
	TestTrue(TEXT("Registered owner can be queried"), State.TryGetOwner(TEXT("Rome"), Owner));
	TestEqual(TEXT("Starting owner is canonical"), Owner, EFactionID::Rome);
	TestEqual(TEXT("Rome initially has one territory"), State.GetFactionTerritoryCount(EFactionID::Rome), 1);

	const FTerritoryOwnershipChangeResult Transfer =
		State.ChangeOwner(TEXT("Rome"), EFactionID::Carthage);
	TestTrue(TEXT("Known territory ownership can change"), Transfer.WasSuccessful());
	TestTrue(TEXT("Transfer reports an observable change"), Transfer.bOwnershipChanged);
	TestEqual(TEXT("Transfer records previous owner"), Transfer.PreviousOwner, EFactionID::Rome);
	TestEqual(TEXT("Transfer records new owner"), Transfer.NewOwner, EFactionID::Carthage);
	TestEqual(TEXT("Rome loses the transferred territory"), State.GetFactionTerritoryCount(EFactionID::Rome), 0);
	TestEqual(TEXT("Carthage gains the transferred territory"), State.GetFactionTerritoryCount(EFactionID::Carthage), 1);

	Owner = EFactionID::None;
	TestTrue(TEXT("Transferred owner remains queryable"), State.TryGetOwner(TEXT("Rome"), Owner));
	TestEqual(TEXT("The single canonical entry contains the new owner"), Owner, EFactionID::Carthage);

	const FTerritoryOwnershipChangeResult Unknown =
		State.ChangeOwner(TEXT("Atlantis"), EFactionID::Rome);
	TestEqual(TEXT("Unknown IDs are rejected"), Unknown.Failure, ETerritoryOwnershipFailure::UnknownTerritory);
	TestEqual(TEXT("Rejected ownership changes do not add state"), State.GetTerritoryCount(), 1);

	FTerritoryDefinition Duplicate = Rome;
	Duplicate.DisplayName = FText::FromString(TEXT("Duplicate Italia"));
	const FTerritoryRegistrationResult DuplicateResult = State.RegisterTerritory(Duplicate);
	TestEqual(TEXT("Duplicate stable IDs are rejected"), DuplicateResult.Failure, ETerritoryRegistrationFailure::DuplicateID);

	FTerritoryDefinition Invalid;
	const FTerritoryRegistrationResult InvalidResult = State.RegisterTerritory(Invalid);
	TestEqual(TEXT("Incomplete definitions are rejected before registration"), InvalidResult.Failure, ETerritoryRegistrationFailure::InvalidDefinition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDiplomacySymmetryAndTransitionsTest,
	"RomanEmpireGame.Rules.Campaign.DiplomacySymmetryAndTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FDiplomacySymmetryAndTransitionsTest::RunTest(const FString& Parameters)
{
	const FDiplomaticPair Forward =
		RomanEmpireDiplomacy::NormalizePair(EFactionID::Rome, EFactionID::Carthage);
	const FDiplomaticPair Reverse =
		RomanEmpireDiplomacy::NormalizePair(EFactionID::Carthage, EFactionID::Rome);
	TestTrue(TEXT("Faction-pair normalization is order independent"), Forward == Reverse);
	TestTrue(TEXT("Normalized pair is valid and ordered"), Forward.IsValid());

	FDiplomacyState State;
	TestEqual(TEXT("Unstored valid pairs are neutral"),
		State.GetStatus(EFactionID::Rome, EFactionID::Carthage), EDiplomaticStatus::Neutral);

	const FDiplomaticChangeResult DeclareWar =
		State.TrySetStatus(EFactionID::Rome, EFactionID::Carthage, EDiplomaticStatus::War);
	TestTrue(TEXT("Neutral may transition to war"), DeclareWar.WasSuccessful());
	TestEqual(TEXT("War is symmetric in forward order"),
		State.GetStatus(EFactionID::Rome, EFactionID::Carthage), EDiplomaticStatus::War);
	TestEqual(TEXT("War is symmetric in reverse order"),
		State.GetStatus(EFactionID::Carthage, EFactionID::Rome), EDiplomaticStatus::War);
	TestEqual(TEXT("One unordered pair occupies one stored relation"), State.GetStoredRelationCount(), 1);

	const FDiplomaticChangeResult DirectAlliance =
		State.TrySetStatus(EFactionID::Carthage, EFactionID::Rome, EDiplomaticStatus::Allied);
	TestEqual(TEXT("War cannot jump directly to alliance"),
		DirectAlliance.Failure, EDiplomaticChangeFailure::IllegalTransition);
	TestEqual(TEXT("Rejected transition preserves war"),
		State.GetStatus(EFactionID::Rome, EFactionID::Carthage), EDiplomaticStatus::War);

	TestTrue(TEXT("War may transition to neutrality"),
		State.TrySetStatus(EFactionID::Rome, EFactionID::Carthage, EDiplomaticStatus::Neutral).WasSuccessful());
	TestTrue(TEXT("Neutral may transition to alliance"),
		State.TrySetStatus(EFactionID::Carthage, EFactionID::Rome, EDiplomaticStatus::Allied).WasSuccessful());
	TestEqual(TEXT("Alliance is symmetric"),
		State.GetStatus(EFactionID::Rome, EFactionID::Carthage), EDiplomaticStatus::Allied);

	const FDiplomaticChangeResult Self =
		State.TrySetStatus(EFactionID::Rome, EFactionID::Rome, EDiplomaticStatus::Allied);
	TestEqual(TEXT("Self relationships are rejected"), Self.Failure, EDiplomaticChangeFailure::SelfRelationship);
	TestEqual(TEXT("Self relationship queries are invalid"),
		State.GetStatus(EFactionID::Rome, EFactionID::Rome), EDiplomaticStatus::Invalid);

	const FDiplomaticChangeResult InvalidFaction =
		State.TrySetStatus(EFactionID::None, EFactionID::Rome, EDiplomaticStatus::War);
	TestEqual(TEXT("Unowned is not a diplomatic faction"),
		InvalidFaction.Failure, EDiplomaticChangeFailure::InvalidFaction);
	TestEqual(TEXT("Invalid faction queries are invalid"),
		State.GetStatus(EFactionID::None, EFactionID::Rome), EDiplomaticStatus::Invalid);
	return true;
}

#endif
