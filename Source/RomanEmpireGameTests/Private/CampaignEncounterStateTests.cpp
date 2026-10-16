// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Battle/TacticalBattleState.h"
#include "Campaign/CampaignEncounterState.h"
#include "UObject/Class.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace RomanEmpireCampaignEncounterStateTests
{
namespace
{
	const FGuid ArmyA(1, 0, 0, 1), ArmyB(1, 0, 0, 2), ArmyC(1, 0, 0, 3), Encounter(2, 0, 0, 1);
	const FGuid SquadA(3, 0, 0, 1), SquadB(3, 0, 0, 2), SquadC(3, 0, 0, 3), SquadD(3, 0, 0, 4);
	FCampaignArmy MakeArmy(FGuid ID, EFactionID Faction, FName Territory, int32 Count)
	{
		FCampaignArmy Army; Army.ArmyID = ID; Army.Faction = Faction; Army.TerritoryID = Territory;
		FCampaignArmyUnitStack Stack; Stack.UnitID = TEXT("Legionary"); Stack.Count = Count;
		Army.Units.Add(Stack); return Army;
	}
	FCampaignSquadBinding Bind(FGuid Squad, FGuid Army, FName Unit, int32 Count)
	{
		FCampaignSquadBinding Value; Value.SquadID = Squad; Value.ArmyID = Army; Value.UnitID = Unit; Value.UnitCount = Count; return Value;
	}
	struct FFixture
	{
		FCampaignArmyState Armies;
		FTerritoryCampaignState Territories;
		FCampaignEncounterState EncounterState;
		FFixture()
		{
			FTerritoryDefinition Source; Source.TerritoryID = TEXT("Source"); Source.DisplayName = FText::FromString(TEXT("Source"));
			Source.StartingOwner = EFactionID::Rome; Source.AdjacentTerritoryIDs = { TEXT("Target") };
			FTerritoryDefinition Target; Target.TerritoryID = TEXT("Target"); Target.DisplayName = FText::FromString(TEXT("Target"));
			Target.StartingOwner = EFactionID::Carthage; Target.AdjacentTerritoryIDs = { TEXT("Source") };
			FTerritoryDefinition Other; Other.TerritoryID = TEXT("Other"); Other.DisplayName = FText::FromString(TEXT("Other")); Other.StartingOwner = EFactionID::Rome;
			Territories.RegisterTerritory(Source); Territories.RegisterTerritory(Target); Territories.RegisterTerritory(Other);
			Armies.RegisterArmy(MakeArmy(ArmyA, EFactionID::Rome, TEXT("Source"), 15));
			Armies.RegisterArmy(MakeArmy(ArmyB, EFactionID::Carthage, TEXT("Target"), 10));
			Armies.RegisterArmy(MakeArmy(ArmyC, EFactionID::Rome, TEXT("Other"), 7));
		}
		ECampaignEncounterFailure Begin()
		{
			return EncounterState.Begin(Encounter, TEXT("Camp"), ArmyA, TEXT("Target"), EFactionID::Rome, Armies, Territories);
		}
		void BindAll()
		{
			EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyA, TEXT("Legionary"), 6));
			EncounterState.BindSquad(Encounter, Bind(SquadB, ArmyA, TEXT("Legionary"), 9));
			EncounterState.BindSquad(Encounter, Bind(SquadC, ArmyB, TEXT("Legionary"), 10));
		}
	};
	bool SameArmies(const FCampaignArmyState& A, const FCampaignArmyState& B)
	{
		return FCampaignArmyState::StaticStruct()->CompareScriptStruct(&A, &B, 0);
	}
	bool SameTerritories(const FTerritoryCampaignState& A, const FTerritoryCampaignState& B)
	{
		return FTerritoryCampaignState::StaticStruct()->CompareScriptStruct(&A, &B, 0);
	}
	FTacticalBattleSquadResult Row(FGuid ID, EFactionID Faction, int32 Initial, int32 Alive)
	{
		FTacticalBattleSquadResult Value; Value.SquadID = ID; Value.Faction = Faction;
		Value.InitialUnits = Initial; Value.Survivors = Alive; Value.Casualties = Initial - Alive; return Value;
	}
	FTacticalBattleResult Victory()
	{
		FTacticalBattleResult Value; Value.EncounterID = Encounter; Value.ObjectiveID = TEXT("Camp");
		Value.CapturedObjectiveID = TEXT("Camp"); Value.Outcome = ETacticalBattleOutcome::PlayerVictory; Value.Winner = EFactionID::Rome;
		Value.Squads = { Row(SquadC, EFactionID::Carthage, 10, 0), Row(SquadB, EFactionID::Rome, 9, 7), Row(SquadA, EFactionID::Rome, 6, 0) };
		return Value;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterDeploymentTest, "RomanEmpireGame.Rules.Campaign.EncounterDeployment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterDeploymentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F;
	const auto Begin = [&](FGuid ID, FName Objective, FGuid Army, FName Target, EFactionID Faction)
	{
		return F.EncounterState.Begin(ID, Objective, Army, Target, Faction, F.Armies, F.Territories);
	};
	TestEqual(TEXT("Missing encounter identity"), Begin(FGuid(), TEXT("Camp"), ArmyA, TEXT("Target"), EFactionID::Rome), ECampaignEncounterFailure::InvalidEncounterID);
	TestEqual(TEXT("Missing objective"), Begin(Encounter, NAME_None, ArmyA, TEXT("Target"), EFactionID::Rome), ECampaignEncounterFailure::InvalidObjectiveID);
	TestEqual(TEXT("Unknown army"), Begin(Encounter, TEXT("Camp"), FGuid(), TEXT("Target"), EFactionID::Rome), ECampaignEncounterFailure::InvalidAttacker);
	TestEqual(TEXT("Wrong attacking faction"), Begin(Encounter, TEXT("Camp"), ArmyA, TEXT("Target"), EFactionID::Carthage), ECampaignEncounterFailure::InvalidAttacker);
	TestEqual(TEXT("Invalid faction"), Begin(Encounter, TEXT("Camp"), ArmyA, TEXT("Target"), static_cast<EFactionID>(255)), ECampaignEncounterFailure::InvalidAttacker);
	TestEqual(TEXT("Unknown target"), Begin(Encounter, TEXT("Camp"), ArmyA, TEXT("Missing"), EFactionID::Rome), ECampaignEncounterFailure::InvalidTarget);
	TestEqual(TEXT("Friendly target"), Begin(Encounter, TEXT("Camp"), ArmyA, TEXT("Other"), EFactionID::Rome), ECampaignEncounterFailure::InvalidTarget);
	TestEqual(TEXT("Disconnected source"), Begin(Encounter, TEXT("Camp"), ArmyC, TEXT("Target"), EFactionID::Rome), ECampaignEncounterFailure::NotAdjacent);
	F.Territories.ChangeOwner(TEXT("Source"), EFactionID::Carthage);
	TestEqual(TEXT("Army must occupy own source"), F.Begin(), ECampaignEncounterFailure::InvalidAttacker);
	F.Territories.ChangeOwner(TEXT("Source"), EFactionID::Rome);
	TestFalse(TEXT("All failed starts leave no pending deployment"), F.EncounterState.HasPendingEncounter());
	TestEqual(TEXT("Adjacent authoritative deployment succeeds"), F.Begin(), ECampaignEncounterFailure::None);
	TestEqual(TEXT("Cannot overwrite pending deployment"), Begin(FGuid::NewGuid(), TEXT("Other"), ArmyA, TEXT("Target"), EFactionID::Rome), ECampaignEncounterFailure::EncounterPending);
	FCampaignEncounterPlan Detached = F.EncounterState.GetPlan(); Detached.Armies.Reset(); Detached.ObjectiveID = NAME_None;
	TestEqual(TEXT("Frozen roster query is detached"), F.EncounterState.GetPlan().Armies.Num(), 2);
	TestEqual(TEXT("Unknown encounter cannot bind"), F.EncounterState.BindSquad(FGuid::NewGuid(), Bind(SquadA, ArmyA, TEXT("Legionary"), 6)), ECampaignEncounterFailure::ResultMismatch);
	TestEqual(TEXT("No squad identity"), F.EncounterState.BindSquad(Encounter, Bind(FGuid(), ArmyA, TEXT("Legionary"), 1)), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("Strategic ID cannot become tactical ID"), F.EncounterState.BindSquad(Encounter, Bind(ArmyB, ArmyA, TEXT("Legionary"), 1)), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("Unplanned army cannot bind"), F.EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyC, TEXT("Legionary"), 1)), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("Wrong content cannot bind"), F.EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyA, TEXT("Archer"), 1)), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("Nonpositive quantity"), F.EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyA, TEXT("Legionary"), 0)), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("First partial stack binding"), F.EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyA, TEXT("Legionary"), 6)), ECampaignEncounterFailure::None);
	TestEqual(TEXT("Duplicate squad rejected"), F.EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyB, TEXT("Legionary"), 1)), ECampaignEncounterFailure::DuplicateSquad);
	TestEqual(TEXT("Total cannot exceed inventory"), F.EncounterState.BindSquad(Encounter, Bind(SquadB, ArmyA, TEXT("Legionary"), MAX_int32)), ECampaignEncounterFailure::OverDeployment);
	TestEqual(TEXT("Incomplete deployment cannot lose unrepresented units"), F.EncounterState.Apply(Victory(), F.Armies, F.Territories), ECampaignEncounterFailure::IncompleteDeployment);
	TestEqual(TEXT("Rejected bindings leave only first"), F.EncounterState.GetBindings().Num(), 1);
	auto DetachedBindings = F.EncounterState.GetBindings(); DetachedBindings[0].UnitCount = 999;
	TestEqual(TEXT("Binding reads detached"), F.EncounterState.GetBindings()[0].UnitCount, 6);
	F.EncounterState.Reset(); TestFalse(TEXT("Reset clears deployment"), F.EncounterState.HasPendingEncounter());
	TestTrue(TEXT("Reset clears bindings"), F.EncounterState.GetBindings().IsEmpty());
	FFixture Empty; Empty.Armies.RemoveUnits(ArmyB, TEXT("Legionary"), 10);
	TestEqual(TEXT("No implicit empty-territory battle policy"), Empty.Begin(), ECampaignEncounterFailure::NoDefenders);
	FFixture ThirdParty; ThirdParty.Armies.RegisterArmy(MakeArmy(FGuid(9, 0, 0, 1), EFactionID::Gaul, TEXT("Target"), 1));
	TestEqual(TEXT("No implicit third-party force policy"), ThirdParty.Begin(), ECampaignEncounterFailure::InvalidTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterApplicationTest, "RomanEmpireGame.Rules.Campaign.EncounterApplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterApplicationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F; F.Begin(); F.BindAll();
	const FCampaignArmyState BeforeArmies = F.Armies;
	const FTerritoryCampaignState BeforeTerritories = F.Territories;
	const auto Reject = [&](const FTacticalBattleResult& Bad, ECampaignEncounterFailure Failure)
	{
		TestEqual(TEXT("Malformed result rejected"), F.EncounterState.Apply(Bad, F.Armies, F.Territories), Failure);
		TestTrue(TEXT("Rejection preserves every army"), SameArmies(F.Armies, BeforeArmies));
		TestTrue(TEXT("Rejection preserves ownership and all territory data"), SameTerritories(F.Territories, BeforeTerritories));
		TestTrue(TEXT("Rejection preserves pending encounter"), F.EncounterState.HasPendingEncounter());
		TestFalse(TEXT("Rejected result not consumed"), F.EncounterState.HasApplied(Encounter));
	};
	FTacticalBattleResult Bad = Victory(); Bad.EncounterID = FGuid::NewGuid(); Reject(Bad, ECampaignEncounterFailure::ResultMismatch);
	Bad = Victory(); Bad.EncounterID.Invalidate(); Reject(Bad, ECampaignEncounterFailure::ResultMismatch);
	Bad = Victory(); Bad.ObjectiveID = TEXT("Other"); Reject(Bad, ECampaignEncounterFailure::ResultMismatch);
	Bad = Victory(); Bad.Squads.Pop(); Reject(Bad, ECampaignEncounterFailure::ResultMismatch);
	Bad = Victory(); Bad.Squads[2] = Bad.Squads[1]; Reject(Bad, ECampaignEncounterFailure::ResultMismatch);
	Bad = Victory(); Bad.Squads[2].SquadID = SquadD; Reject(Bad, ECampaignEncounterFailure::ResultMismatch);
	Bad = Victory(); Bad.Squads[2].Faction = EFactionID::Carthage; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Squads[2].InitialUnits = 7; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Squads[2].Survivors = -1; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Squads[2].Casualties = -1; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Squads[2].Survivors = MAX_int32; Bad.Squads[2].Casualties = MAX_int32; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Winner = EFactionID::Carthage; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.CapturedObjectiveID = NAME_None; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Squads[0] = Row(SquadC, EFactionID::Carthage, 10, 1); Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Squads[1] = Row(SquadB, EFactionID::Rome, 9, 0); Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Outcome = ETacticalBattleOutcome::InProgress; Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	Bad = Victory(); Bad.Outcome = static_cast<ETacticalBattleOutcome>(255); Reject(Bad, ECampaignEncounterFailure::InvalidResult);
	TestEqual(TEXT("Mapped split-stack casualties and capture apply together"), F.EncounterState.Apply(Victory(), F.Armies, F.Territories), ECampaignEncounterFailure::None);
	FCampaignArmy Read; F.Armies.TryGetArmy(ArmyA, Read);
	TestEqual(TEXT("Survivors aggregate from both squads"), FCampaignArmyState::GetUnitCount(Read), 7);
	TestEqual(TEXT("Attacker occupies captured target"), Read.TerritoryID, FName(TEXT("Target")));
	TestFalse(TEXT("Dead defending army retired"), F.Armies.TryGetArmy(ArmyB, Read));
	F.Armies.TryGetArmy(ArmyC, Read); TestEqual(TEXT("Unrelated army untouched"), FCampaignArmyState::GetUnitCount(Read), 7);
	EFactionID Owner; F.Territories.TryGetOwner(TEXT("Target"), Owner); TestEqual(TEXT("Ownership committed"), Owner, EFactionID::Rome);
	const auto CommittedArmies = F.Armies; const auto CommittedTerritories = F.Territories;
	TestTrue(TEXT("Applied identity recorded"), F.EncounterState.HasApplied(Encounter));
	TestFalse(TEXT("Applied deployment no longer pending"), F.EncounterState.HasPendingEncounter());
	TestEqual(TEXT("Replay rejected"), F.EncounterState.Apply(Victory(), F.Armies, F.Territories), ECampaignEncounterFailure::AlreadyApplied);
	TestEqual(TEXT("Cannot bind after application"), F.EncounterState.BindSquad(Encounter, Bind(SquadD, ArmyA, TEXT("Legionary"), 1)), ECampaignEncounterFailure::AlreadyApplied);
	TestEqual(TEXT("Cannot reuse applied encounter ID"), F.Begin(), ECampaignEncounterFailure::AlreadyApplied);
	TestTrue(TEXT("Replay does not double casualties"), SameArmies(F.Armies, CommittedArmies));
	TestTrue(TEXT("Replay does not change territories"), SameTerritories(F.Territories, CommittedTerritories));
	// A later encounter replaces only the frozen pending plan, not the consumed-ID ledger.
	F.Territories.ChangeOwner(TEXT("Source"), EFactionID::Carthage);
	const FGuid LaterID(2, 0, 0, 2), LaterDefender(1, 0, 0, 4);
	F.Armies.RegisterArmy(MakeArmy(LaterDefender, EFactionID::Carthage, TEXT("Source"), 1));
	TestEqual(TEXT("New encounter may begin after completed commit"), F.EncounterState.Begin(LaterID, TEXT("Camp2"), ArmyA,
		TEXT("Source"), EFactionID::Rome, F.Armies, F.Territories), ECampaignEncounterFailure::None);
	TestEqual(TEXT("New deployment starts unbound"), F.EncounterState.GetBindings().Num(), 0);
	TestTrue(TEXT("Old consumed ID survives next deployment"), F.EncounterState.HasApplied(Encounter));
	TestEqual(TEXT("Old result cannot affect newer pending encounter"), F.EncounterState.Apply(Victory(), F.Armies, F.Territories), ECampaignEncounterFailure::AlreadyApplied);
	TestEqual(TEXT("New pending identity preserved"), F.EncounterState.GetPlan().EncounterID, LaterID);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterStaleTest, "RomanEmpireGame.Rules.Campaign.EncounterStaleAndResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterStaleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	for (int32 Mutation = 0; Mutation < 7; ++Mutation)
	{
		FFixture F; F.Begin(); F.BindAll();
		switch (Mutation)
		{
		case 0: F.Armies.AddUnits(ArmyA, TEXT("Legionary"), 1); break;
		case 1: F.Armies.RelocateArmy(ArmyA, TEXT("Other")); break;
		case 2: F.Armies.RemoveUnits(ArmyB, TEXT("Legionary"), 10); break;
		case 3: F.Territories.ChangeOwner(TEXT("Target"), EFactionID::Rome); break;
		case 4: F.Territories.ChangeOwner(TEXT("Source"), EFactionID::Carthage); break;
		case 5: F.Armies.RegisterArmy(MakeArmy(FGuid(9, 0, 0, 2), EFactionID::Carthage, TEXT("Target"), 1)); break;
		case 6:
			F.Armies.ReplaceFromSnapshot({ MakeArmy(FGuid::NewGuid(), EFactionID::Rome, TEXT("Source"), 15),
				MakeArmy(FGuid::NewGuid(), EFactionID::Carthage, TEXT("Target"), 10) }); break;
		}
		const auto BeforeArmies = F.Armies; const auto BeforeTerritories = F.Territories;
		TestEqual(TEXT("Changed deployment/source owner/target roster/new campaign rejects"), F.EncounterState.Apply(Victory(), F.Armies, F.Territories), ECampaignEncounterFailure::StaleDeployment);
		TestTrue(TEXT("Stale rejection preserves newer army state"), SameArmies(F.Armies, BeforeArmies));
		TestTrue(TEXT("Stale rejection preserves newer territory state"), SameTerritories(F.Territories, BeforeTerritories));
	}
	FFixture Unrelated; Unrelated.Begin(); Unrelated.BindAll(); Unrelated.Armies.AddUnits(ArmyC, TEXT("Archer"), 2);
	TestEqual(TEXT("Unrelated army change does not stale deployment"), Unrelated.EncounterState.Apply(Victory(), Unrelated.Armies, Unrelated.Territories), ECampaignEncounterFailure::None);
	FCampaignArmy Read; Unrelated.Armies.TryGetArmy(ArmyC, Read); TestEqual(TEXT("Unrelated change survives commit"), FCampaignArmyState::GetUnitCount(Read), 9);
	for (bool bDraw : { false, true })
	{
		FFixture F; F.Begin(); F.BindAll(); FTacticalBattleResult Result = Victory();
		Result.Outcome = bDraw ? ETacticalBattleOutcome::Draw : ETacticalBattleOutcome::PlayerDefeat;
		Result.Winner = bDraw ? EFactionID::None : EFactionID::Carthage; Result.CapturedObjectiveID = NAME_None;
		Result.Squads = { Row(SquadA, EFactionID::Rome, 6, 0), Row(SquadB, EFactionID::Rome, 9, 0), Row(SquadC, EFactionID::Carthage, 10, bDraw ? 0 : 4) };
		TestEqual(TEXT("Defeat/draw applies without capture"), F.EncounterState.Apply(Result, F.Armies, F.Territories), ECampaignEncounterFailure::None);
		TestFalse(TEXT("Lost attacking army retired"), F.Armies.TryGetArmy(ArmyA, Read));
		EFactionID Owner; F.Territories.TryGetOwner(TEXT("Target"), Owner); TestEqual(TEXT("Defeat/draw leaves ownership"), Owner, EFactionID::Carthage);
		if (bDraw) TestFalse(TEXT("Draw retires both forces"), F.Armies.TryGetArmy(ArmyB, Read));
		else { F.Armies.TryGetArmy(ArmyB, Read); TestEqual(TEXT("Defender survivors preserved"), FCampaignArmyState::GetUnitCount(Read), 4); }
	}
	FFixture Live; Live.Begin(); Live.BindAll(); FTacticalBattleState Battle;
	TestTrue(TEXT("Evaluator accepts authorized encounter identity"), Battle.BeginEncounter(TEXT("Camp"), EFactionID::Rome, EFactionID::Carthage,
		{ { SquadA, EFactionID::Rome, 6 }, { SquadB, EFactionID::Rome, 9 }, { SquadC, EFactionID::Carthage, 10 } }, Encounter));
	TestEqual(TEXT("Evaluator publishes exact deployment identity"), Battle.GetResult().EncounterID, Encounter);
	Battle.RecordCasualty(SquadA, 6); Battle.RecordCasualty(SquadB, 2); Battle.RecordCasualty(SquadC, 10);
	TestEqual(TEXT("Kills alone cannot apply victory"), Live.EncounterState.Apply(Battle.GetResult(), Live.Armies, Live.Territories), ECampaignEncounterFailure::InvalidResult);
	Battle.CaptureObjective(EFactionID::Rome);
	TestEqual(TEXT("Canonical evaluator result applies through explicit mapping"), Live.EncounterState.Apply(Battle.GetResult(), Live.Armies, Live.Territories), ECampaignEncounterFailure::None);
	Battle.Reset(); TestFalse(TEXT("Reset drops encounter identity"), Battle.GetResult().EncounterID.IsValid());
	TestTrue(TEXT("Sandbox evaluator creates its own distinct identity"), Battle.BeginEncounter(TEXT("Camp"), EFactionID::Rome, EFactionID::Carthage,
		{ { SquadA, EFactionID::Rome, 1 }, { SquadC, EFactionID::Carthage, 1 } }));
	TestTrue(TEXT("Default encounter identity is valid"), Battle.GetResult().EncounterID.IsValid());
	TestTrue(TEXT("Sandbox is not silently authorized campaign encounter"), Battle.GetResult().EncounterID != Encounter);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterCompositionTest, "RomanEmpireGame.Rules.Campaign.EncounterComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterCompositionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F;
	F.Armies.AddUnits(ArmyA, TEXT("Archer"), 3);
	const FGuid ExtraDefender(1, 0, 0, 4), ExtraSquad(3, 0, 0, 5);
	FCampaignArmy Defense = MakeArmy(ExtraDefender, EFactionID::Carthage, TEXT("Target"), 4);
	Defense.Units[0].UnitID = TEXT("Archer"); F.Armies.RegisterArmy(Defense);
	F.Territories.FoundSettlement(TEXT("Target"), FText::FromString(TEXT("Preserved settlement")));
	FTerritorySettlementState SettlementBefore; F.Territories.TryGetSettlement(TEXT("Target"), SettlementBefore);
	FFactionResourceDelta ProductionBefore; F.Territories.TryGetResourceProduction(TEXT("Target"), ProductionBefore);
	TestEqual(TEXT("All target defenders included"), F.Begin(), ECampaignEncounterFailure::None);
	TestEqual(TEXT("Frozen plan includes both defending armies"), F.EncounterState.GetPlan().Armies.Num(), 3);
	F.BindAll();
	F.EncounterState.BindSquad(Encounter, Bind(SquadD, ArmyA, TEXT("Archer"), 3));
	F.EncounterState.BindSquad(Encounter, Bind(ExtraSquad, ExtraDefender, TEXT("Archer"), 4));
	FTacticalBattleResult Result = Victory();
	Result.Squads.Add(Row(SquadD, EFactionID::Rome, 3, 2));
	Result.Squads.Add(Row(ExtraSquad, EFactionID::Carthage, 4, 0));
	FTacticalBattleResult Unmapped = Result; Unmapped.Squads.Add(Row(FGuid::NewGuid(), EFactionID::Rome, 1, 1));
	TestEqual(TEXT("Unmapped tactical reinforcement cannot mint strategic units"), F.EncounterState.Apply(Unmapped, F.Armies, F.Territories), ECampaignEncounterFailure::ResultMismatch);
	TestEqual(TEXT("Multi-content/multi-defender apply"), F.EncounterState.Apply(Result, F.Armies, F.Territories), ECampaignEncounterFailure::None);
	FCampaignArmy Read; F.Armies.TryGetArmy(ArmyA, Read);
	TestEqual(TEXT("Both survivor content stacks retained"), Read.Units.Num(), 2);
	TestEqual(TEXT("Archer casualties affect only Archer content"), Read.Units[0].UnitID, FName(TEXT("Archer")));
	TestEqual(TEXT("Archer survivors"), Read.Units[0].Count, 2);
	TestEqual(TEXT("Legionary survivors"), Read.Units[1].Count, 7);
	TestFalse(TEXT("Second defending army retired too"), F.Armies.TryGetArmy(ExtraDefender, Read));
	FTerritorySettlementState SettlementAfter; F.Territories.TryGetSettlement(TEXT("Target"), SettlementAfter);
	TestTrue(TEXT("Capture preserves canonical settlement values"), FTerritorySettlementState::StaticStruct()->CompareScriptStruct(&SettlementBefore, &SettlementAfter, 0));
	FFactionResourceDelta ProductionAfter; F.Territories.TryGetResourceProduction(TEXT("Target"), ProductionAfter);
	TestTrue(TEXT("Capture preserves effective production"), FFactionResourceDelta::StaticStruct()->CompareScriptStruct(&ProductionBefore, &ProductionAfter, 0));
	for (int32 InvalidKind = 0; InvalidKind < 3; ++InvalidKind)
	{
		FFixture Bad; Bad.Begin(); Bad.BindAll();
		FTacticalBattleResult Defeat = Victory(); Defeat.Outcome = ETacticalBattleOutcome::PlayerDefeat;
		Defeat.Winner = EFactionID::Carthage; Defeat.CapturedObjectiveID = NAME_None;
		Defeat.Squads = { Row(SquadA, EFactionID::Rome, 6, 0), Row(SquadB, EFactionID::Rome, 9, 0), Row(SquadC, EFactionID::Carthage, 10, 1) };
		if (InvalidKind == 0) Defeat.Winner = EFactionID::Rome;
		else if (InvalidKind == 1) Defeat.CapturedObjectiveID = TEXT("Camp");
		else Defeat.Squads[0] = Row(SquadA, EFactionID::Rome, 6, 1);
		const auto Before = Bad.Armies; const auto TerritoryBefore = Bad.Territories;
		TestEqual(TEXT("Contradictory defeat contract rejected"), Bad.EncounterState.Apply(Defeat, Bad.Armies, Bad.Territories), ECampaignEncounterFailure::InvalidResult);
		TestTrue(TEXT("Rejected defeat leaves armies intact"), SameArmies(Before, Bad.Armies));
		TestTrue(TEXT("Rejected defeat leaves territories intact"), SameTerritories(TerritoryBefore, Bad.Territories));
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterReinforcementTest, "RomanEmpireGame.Rules.Campaign.EncounterReinforcement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterReinforcementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F; F.Begin(); F.BindAll();
	const FGuid Receipt(4, 0, 0, 1), SecondReceipt(4, 0, 0, 2), ArcherSquad(3, 0, 0, 5);
	const FCampaignArmyState Before = F.Armies;
	const FCampaignEncounterState BeforeEncounter = F.EncounterState;
	const auto Reject = [&](FGuid ID, FGuid Paid, FCampaignSquadBinding Binding, ECampaignEncounterFailure Failure)
	{
		TestEqual(TEXT("Invalid enrollment rejected"), F.EncounterState.EnrollReinforcement(ID, Paid, Binding, F.Armies, F.Territories), Failure);
		TestTrue(TEXT("Invalid enrollment preserves canonical armies"), SameArmies(Before, F.Armies));
		TestTrue(TEXT("Invalid enrollment preserves snapshot, bindings and receipt ledger"),
			FCampaignEncounterState::StaticStruct()->CompareScriptStruct(&BeforeEncounter, &F.EncounterState, 0));
	};
	Reject(FGuid::NewGuid(), Receipt, Bind(SquadD, ArmyA, TEXT("Legionary"), 1), ECampaignEncounterFailure::ResultMismatch);
	Reject(Encounter, FGuid(), Bind(SquadD, ArmyA, TEXT("Legionary"), 1), ECampaignEncounterFailure::InvalidBinding);
	Reject(Encounter, Receipt, Bind(SquadD, ArmyC, TEXT("Legionary"), 1), ECampaignEncounterFailure::InvalidBinding);
	Reject(Encounter, Receipt, Bind(SquadA, ArmyA, TEXT("Legionary"), 1), ECampaignEncounterFailure::DuplicateSquad);
	Reject(Encounter, Receipt, Bind(ArmyA, ArmyA, TEXT("Legionary"), 1), ECampaignEncounterFailure::InvalidBinding);
	Reject(Encounter, Receipt, Bind(SquadD, ArmyA, NAME_None, 1), ECampaignEncounterFailure::InvalidBinding);
	Reject(Encounter, Receipt, Bind(SquadD, ArmyA, TEXT("Legionary"), 2), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("Paid singleton extends all values atomically"), F.EncounterState.EnrollReinforcement(Encounter, Receipt,
		Bind(SquadD, ArmyA, TEXT("Legionary"), 1), F.Armies, F.Territories), ECampaignEncounterFailure::None);
	TestEqual(TEXT("Updated snapshot remains fully bound and current"), F.EncounterState.ValidateDeployment(F.Armies, F.Territories), ECampaignEncounterFailure::None);
	FCampaignArmy Read; F.Armies.TryGetArmy(ArmyA, Read);
	TestEqual(TEXT("Inventory credits exactly one trained unit"), FCampaignArmyState::GetUnitCount(Read), 16);
	TestEqual(TEXT("Receipt replay cannot mint another squad"), F.EncounterState.EnrollReinforcement(Encounter, Receipt,
		Bind(ArcherSquad, ArmyA, TEXT("Archer"), 1), F.Armies, F.Territories), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("A new paid content stack can enroll explicitly"), F.EncounterState.EnrollReinforcement(Encounter, SecondReceipt,
		Bind(ArcherSquad, ArmyA, TEXT("Archer"), 1), F.Armies, F.Territories), ECampaignEncounterFailure::None);
	FTacticalBattleResult Result = Victory(); Result.Squads.Add(Row(SquadD, EFactionID::Rome, 1, 1));
	Result.Squads.Add(Row(ArcherSquad, EFactionID::Rome, 1, 0));
	TestEqual(TEXT("Mapped reinforcements participate in complete casualty apply"), F.EncounterState.Apply(Result, F.Armies, F.Territories), ECampaignEncounterFailure::None);
	F.Armies.TryGetArmy(ArmyA, Read);
	TestEqual(TEXT("Paid survivor remains while dead new stack is removed"), FCampaignArmyState::GetUnitCount(Read), 8);
	TestEqual(TEXT("Cannot enroll after outcome was consumed"), F.EncounterState.EnrollReinforcement(Encounter, FGuid::NewGuid(),
		Bind(FGuid::NewGuid(), ArmyA, TEXT("Legionary"), 1), F.Armies, F.Territories), ECampaignEncounterFailure::AlreadyApplied);
	FFixture Stale; Stale.Begin(); Stale.BindAll(); Stale.Armies.RemoveUnits(ArmyA, TEXT("Legionary"), 1);
	TestEqual(TEXT("Enrollment cannot overwrite independently changed inventory"), Stale.EncounterState.EnrollReinforcement(Encounter, Receipt,
		Bind(SquadD, ArmyA, TEXT("Legionary"), 1), Stale.Armies, Stale.Territories), ECampaignEncounterFailure::StaleDeployment);
	FFixture Overflow; Overflow.Armies.AddUnits(ArmyA, TEXT("Legionary"), MAX_int32 - 15); Overflow.Begin();
	Overflow.EncounterState.BindSquad(Encounter, Bind(SquadA, ArmyA, TEXT("Legionary"), MAX_int32));
	Overflow.EncounterState.BindSquad(Encounter, Bind(SquadC, ArmyB, TEXT("Legionary"), 10));
	const FCampaignArmyState OverflowBefore = Overflow.Armies;
	TestEqual(TEXT("Count overflow fails before any commit"), Overflow.EncounterState.EnrollReinforcement(Encounter, Receipt,
		Bind(SquadD, ArmyA, TEXT("Legionary"), 1), Overflow.Armies, Overflow.Territories), ECampaignEncounterFailure::MutationFailed);
	TestTrue(TEXT("Overflow preserves every army"), SameArmies(OverflowBefore, Overflow.Armies));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignDefenseResultsTest, "RomanEmpireGame.Rules.Campaign.DefenseResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignDefenseResultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	for (const ETacticalBattleOutcome Expected : { ETacticalBattleOutcome::PlayerVictory, ETacticalBattleOutcome::PlayerDefeat, ETacticalBattleOutcome::Draw })
	{
		FFixture F;
		TestEqual(TEXT("Inverse encounter accepted"), F.EncounterState.Begin(Encounter, TEXT("Camp"), ArmyB,
			TEXT("Source"), EFactionID::Carthage, F.Armies, F.Territories, EFactionID::Rome), ECampaignEncounterFailure::None);
		FCampaignEncounterCheckpoint Checkpoint;
		TestEqual(TEXT("Unbound defense exports"), F.EncounterState.ExportCheckpoint(Checkpoint), ECampaignEncounterFailure::None);
		TestEqual(TEXT("Human role retained in checkpoint"), Checkpoint.PendingPlan.PlayerFaction, EFactionID::Rome);
		FCampaignEncounterState Restored;
		TestEqual(TEXT("Inverse role restores"), Restored.RestoreCheckpoint(Checkpoint, F.Armies, F.Territories), ECampaignEncounterFailure::None);
		TestTrue(TEXT("Unbound cancellation succeeds"), Restored.CancelUnboundPending());
		TestFalse(TEXT("Cancellation leaves no plan"), Restored.HasPendingEncounter());
		F.BindAll();
		TestFalse(TEXT("Materialized plan cannot be cancelled"), F.EncounterState.CancelUnboundPending());
		FTacticalBattleState Battle;
		TestTrue(TEXT("Actual evaluator starts inverse deployment"), Battle.BeginEncounter(TEXT("Camp"), EFactionID::Rome,
			EFactionID::Carthage, { { SquadA, EFactionID::Rome, 6 }, { SquadB, EFactionID::Rome, 9 },
				{ SquadC, EFactionID::Carthage, 10 } }, Encounter, EFactionID::Carthage));
		if (Expected == ETacticalBattleOutcome::PlayerVictory)
		{ Battle.RecordCasualty(SquadA, 2); Battle.RecordCasualty(SquadC, 10); }
		else
		{
			Battle.RecordCasualty(SquadA, 6); Battle.RecordCasualty(SquadB, 9);
			if (Expected == ETacticalBattleOutcome::PlayerDefeat)
			{ Battle.RecordCasualty(SquadC, 2); Battle.CaptureObjective(EFactionID::Carthage); }
			else Battle.RecordCasualty(SquadC, 10);
		}
		const FTacticalBattleResult Result = Battle.GetResult();
		TestEqual(TEXT("Evaluator supplies human-relative outcome"), Result.Outcome, Expected);
		const auto BeforeArmies = F.Armies; const auto BeforeTerritories = F.Territories;
		auto Bad = Result; Bad.Winner = EFactionID::Gaul;
		TestTrue(TEXT("Foreign winner rejected"), F.EncounterState.Apply(Bad, F.Armies, F.Territories) != ECampaignEncounterFailure::None);
		Bad = Result; Bad.Outcome = Expected == ETacticalBattleOutcome::PlayerVictory ? ETacticalBattleOutcome::PlayerDefeat : ETacticalBattleOutcome::PlayerVictory;
		TestTrue(TEXT("Attacker-relative or fabricated human outcome rejected"), F.EncounterState.Apply(Bad, F.Armies, F.Territories) != ECampaignEncounterFailure::None);
		Bad = Result; Bad.CapturedObjectiveID = TEXT("ForeignCamp");
		TestTrue(TEXT("Foreign capture rejected"), F.EncounterState.Apply(Bad, F.Armies, F.Territories) != ECampaignEncounterFailure::None);
		TestTrue(TEXT("Rejected results preserve armies"), SameArmies(F.Armies, BeforeArmies));
		TestTrue(TEXT("Rejected results preserve territory"), SameTerritories(F.Territories, BeforeTerritories));
		TestEqual(TEXT("Canonical defense result applies"), F.EncounterState.Apply(Result, F.Armies, F.Territories), ECampaignEncounterFailure::None);
		TestEqual(TEXT("Defense result is consumed once"), F.EncounterState.Apply(Result, F.Armies, F.Territories), ECampaignEncounterFailure::AlreadyApplied);
		EFactionID Owner = EFactionID::None; F.Territories.TryGetOwner(TEXT("Source"), Owner);
		TestEqual(TEXT("Only actual AI capture changes owner"), Owner,
			Expected == ETacticalBattleOutcome::PlayerDefeat ? EFactionID::Carthage : EFactionID::Rome);
		FCampaignArmy Army;
		TestEqual(TEXT("Defender army existence matches survival"), F.Armies.TryGetArmy(ArmyA, Army), Expected == ETacticalBattleOutcome::PlayerVictory);
		if (Expected == ETacticalBattleOutcome::PlayerVictory) TestEqual(TEXT("Defense casualties conserved"), Army.Units[0].Count, 13);
		TestEqual(TEXT("Attacker army existence matches survival"), F.Armies.TryGetArmy(ArmyB, Army), Expected == ETacticalBattleOutcome::PlayerDefeat);
		if (Expected == ETacticalBattleOutcome::PlayerDefeat)
		{ TestEqual(TEXT("Original AI army relocates on capture"), Army.TerritoryID, FName(TEXT("Source"))); TestEqual(TEXT("AI casualties conserved"), Army.Units[0].Count, 8); }
		TestTrue(TEXT("Unrelated army remains"), F.Armies.TryGetArmy(ArmyC, Army));
		TestEqual(TEXT("Unrelated quantity untouched"), Army.Units[0].Count, 7);
	}
	return true;
}

} // namespace RomanEmpireCampaignEncounterStateTests
#endif
