// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Battle/TacticalBattleState.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalBattleStateTest,
	"RomanEmpireGame.Rules.Battle.TacticalResultAndVictory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FTacticalBattleStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid RomanSquad(1, 2, 3, 4);
	const FGuid CarthaginianSquad(5, 6, 7, 8);
	const FGuid Reinforcement(9, 10, 11, 12);
	const TArray<FTacticalSquadSeed> Forces = {
		{ RomanSquad, EFactionID::Rome, 15 },
		{ CarthaginianSquad, EFactionID::Carthage, 10 }
	};
	FTacticalBattleState Battle;
	TestFalse(TEXT("Unstarted battle has no encounter"), Battle.HasEncounter());
	TestFalse(TEXT("Objective ID is required"), Battle.BeginEncounter(NAME_None, EFactionID::Rome, EFactionID::Carthage, Forces));
	TestFalse(TEXT("An enemy force is required"), Battle.BeginEncounter(TEXT("EnemyCamp"), EFactionID::Rome,
		EFactionID::Carthage, { Forces[0] }));
	TestFalse(TEXT("Duplicate squad handles reject the whole setup"), Battle.BeginEncounter(TEXT("EnemyCamp"),
		EFactionID::Rome, EFactionID::Carthage, { Forces[0], Forces[0], Forces[1] }));
	TestFalse(TEXT("Rejected setup leaves no encounter"), Battle.HasEncounter());
	TestTrue(TEXT("Known forces start one encounter"), Battle.BeginEncounter(TEXT("EnemyCamp"),
		EFactionID::Rome, EFactionID::Carthage, Forces));
	TestFalse(TEXT("An active encounter cannot be restarted implicitly"), Battle.BeginEncounter(TEXT("EnemyCamp"),
		EFactionID::Rome, EFactionID::Carthage, Forces));
	FTacticalBattleResult Result = Battle.GetResult();
	TestEqual(TEXT("Starting squads are reported in insertion order"), Result.Squads.Num(), 2);
	TestEqual(TEXT("Roman seed survives intact"), Result.Squads[0].Survivors, 15);
	TestEqual(TEXT("Enemy seed survives intact"), Result.Squads[1].Survivors, 10);
	TestFalse(TEXT("Enemy cannot claim the player objective"), Battle.CaptureObjective(EFactionID::Carthage));
	TestFalse(TEXT("Unknown squad cannot produce a casualty"), Battle.RecordCasualty(Reinforcement));
	TestFalse(TEXT("Overkill is rejected atomically"), Battle.RecordCasualty(CarthaginianSquad, 11));
	TestTrue(TEXT("Newly trained singleton can join the result"), Battle.RegisterReinforcement({ Reinforcement, EFactionID::Rome, 1 }));
	TestFalse(TEXT("Reinforcement handle cannot be counted twice"), Battle.RegisterReinforcement({ Reinforcement, EFactionID::Rome, 1 }));
	TestTrue(TEXT("Enemy casualties are recorded exactly"), Battle.RecordCasualty(CarthaginianSquad, 10));
	Result = Battle.GetResult();
	TestEqual(TEXT("Eliminating defenders alone does not capture the objective"), Result.Outcome, ETacticalBattleOutcome::InProgress);
	TestEqual(TEXT("Defender casualties equal the original squad size"), Result.Squads[1].Casualties, 10);
	TestEqual(TEXT("Reinforcement remains a survivor"), Result.Squads[2].Survivors, 1);
	TestTrue(TEXT("Player capture resolves the battle once"), Battle.CaptureObjective(EFactionID::Rome));
	TestFalse(TEXT("Resolved objective cannot be captured twice"), Battle.CaptureObjective(EFactionID::Rome));
	TestFalse(TEXT("Resolved battle ignores later casualties"), Battle.RecordCasualty(RomanSquad));
	Result = Battle.GetResult();
	TestEqual(TEXT("One evaluator reports victory"), Result.Outcome, ETacticalBattleOutcome::PlayerVictory);
	TestEqual(TEXT("Winner is the player faction"), Result.Winner, EFactionID::Rome);
	TestEqual(TEXT("Captured objective retains its stable ID"), Result.CapturedObjectiveID, FName(TEXT("EnemyCamp")));
	TestEqual(TEXT("Three squad snapshots are carried"), Result.Squads.Num(), 3);

	Battle.Reset();
	TestFalse(TEXT("Reset clears the previous result"), Battle.GetResult().IsResolved());
	TestTrue(TEXT("Reset permits a fresh deterministic encounter"), Battle.BeginEncounter(TEXT("EnemyCamp"),
		EFactionID::Rome, EFactionID::Carthage, {
			{ RomanSquad, EFactionID::Rome, 1 }, { CarthaginianSquad, EFactionID::Carthage, 1 } }));
	TestTrue(TEXT("Last player casualty is accepted"), Battle.RecordCasualty(RomanSquad));
	Result = Battle.GetResult();
	TestEqual(TEXT("Loss of every player unit resolves defeat"), Result.Outcome, ETacticalBattleOutcome::PlayerDefeat);
	TestEqual(TEXT("Defeat names the enemy winner"), Result.Winner, EFactionID::Carthage);
	TestTrue(TEXT("Defeat captures no objective"), Result.CapturedObjectiveID.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTacticalDefenseRolesTest,
	"RomanEmpireGame.Rules.Battle.DefenseRoles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FTacticalDefenseRolesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid Roman(20, 0, 0, 1), Carthaginian(20, 0, 0, 2);
	const TArray<FTacticalSquadSeed> Forces = { { Roman, EFactionID::Rome, 15 }, { Carthaginian, EFactionID::Carthage, 10 } };
	FTacticalBattleState Battle;
	TestFalse(TEXT("Third-party attacker rejected"), Battle.BeginEncounter(TEXT("Camp"), EFactionID::Rome,
		EFactionID::Carthage, Forces, FGuid(), EFactionID::Gaul));
	const auto Begin = [&]()
	{
		Battle.Reset();
		TestTrue(TEXT("Human may defend against AI"), Battle.BeginEncounter(TEXT("Camp"), EFactionID::Rome,
			EFactionID::Carthage, Forces, FGuid(21, 0, 0, 1), EFactionID::Carthage));
		TestEqual(TEXT("Attacker is independent of human"), Battle.GetAttackingFaction(), EFactionID::Carthage);
		TestEqual(TEXT("Human is defender"), Battle.GetDefendingFaction(), EFactionID::Rome);
	};
	Begin();
	TestFalse(TEXT("Defender cannot capture own camp"), Battle.CaptureObjective(EFactionID::Rome));
	TestFalse(TEXT("Living defenders prevent AI capture"), Battle.CaptureObjective(EFactionID::Carthage));
	TestTrue(TEXT("Defender elimination recorded"), Battle.RecordCasualty(Roman, 15));
	TestEqual(TEXT("Elimination alone is not occupation"), Battle.GetResult().Outcome, ETacticalBattleOutcome::InProgress);
	TestTrue(TEXT("AI casualty counted before capture"), Battle.RecordCasualty(Carthaginian, 2));
	TestTrue(TEXT("AI occupation resolves human defeat"), Battle.CaptureObjective(EFactionID::Carthage));
	TestEqual(TEXT("Defeat is human-relative"), Battle.GetResult().Outcome, ETacticalBattleOutcome::PlayerDefeat);
	TestEqual(TEXT("Winning faction remains AI"), Battle.GetResult().Winner, EFactionID::Carthage);
	TestEqual(TEXT("Captured camp retained"), Battle.GetResult().CapturedObjectiveID, FName(TEXT("Camp")));
	TestEqual(TEXT("AI survivors preserved"), Battle.GetResult().Squads[1].Survivors, 8);
	TestFalse(TEXT("Terminal result cannot be changed"), Battle.RecordCasualty(Carthaginian, 8));
	Begin();
	TestTrue(TEXT("Defeating attackers resolves defense"), Battle.RecordCasualty(Carthaginian, 10));
	TestEqual(TEXT("Human defense victory"), Battle.GetResult().Outcome, ETacticalBattleOutcome::PlayerVictory);
	TestEqual(TEXT("Defender winner"), Battle.GetResult().Winner, EFactionID::Rome);
	TestTrue(TEXT("Successful defense captures nothing"), Battle.GetResult().CapturedObjectiveID.IsNone());
	Begin();
	Battle.RecordCasualty(Roman, 15); Battle.RecordCasualty(Carthaginian, 10);
	TestEqual(TEXT("Mutual destruction before occupation draws"), Battle.GetResult().Outcome, ETacticalBattleOutcome::Draw);
	TestEqual(TEXT("Draw has no winner"), Battle.GetResult().Winner, EFactionID::None);
	TestTrue(TEXT("Draw captures nothing"), Battle.GetResult().CapturedObjectiveID.IsNone());
	Battle.Reset();
	TestEqual(TEXT("Reset clears attack role"), Battle.GetAttackingFaction(), EFactionID::None);
	return true;
}

#endif
