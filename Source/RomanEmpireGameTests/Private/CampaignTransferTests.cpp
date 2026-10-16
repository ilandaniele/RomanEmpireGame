// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Campaign/CampaignTransfer.h"
#include "Campaign/CampaignSave.h"
#include "Misc/Crc.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Class.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace RomanEmpireCampaignTransferTests
{
namespace
{
	const FGuid Attacker(10, 0, 0, 1), Defender(10, 0, 0, 2), EncounterID(11, 0, 0, 1);
	const FGuid AttackingSquad(12, 0, 0, 1), DefendingSquad(12, 0, 0, 2), PaidSquad(12, 0, 0, 3), ReceiptID(13, 0, 0, 1);
	template<typename T> bool Same(const T& A, const T& B)
	{ return T::StaticStruct()->CompareScriptStruct(&A, &B, 0); }
	FCampaignSquadBinding Binding(FGuid Squad, FGuid Army, int32 Count)
	{
		FCampaignSquadBinding Value; Value.SquadID = Squad; Value.ArmyID = Army;
		Value.UnitID = TEXT("Legionary"); Value.UnitCount = Count; return Value;
	}
	struct FFixture
	{
		FTerritoryCampaignState Catalog;
		FCampaignTransferSnapshot Snapshot;
		const TArray<EFactionID> Active = { EFactionID::Rome, EFactionID::Carthage };
		FFixture()
		{
			FTerritoryDefinition Source; Source.TerritoryID = TEXT("Source"); Source.DisplayName = FText::FromString(TEXT("Source"));
			Source.StartingOwner = EFactionID::Rome; Source.AdjacentTerritoryIDs = { TEXT("Target") }; Source.ResourceProduction.Gold = 9;
			FTerritoryDefinition Target = Source; Target.TerritoryID = TEXT("Target"); Target.DisplayName = FText::FromString(TEXT("Target"));
			Target.StartingOwner = EFactionID::Carthage; Target.AdjacentTerritoryIDs = { TEXT("Source"), TEXT("Other") };
			FTerritoryDefinition Other = Source; Other.TerritoryID = TEXT("Other"); Other.DisplayName = FText::FromString(TEXT("Other"));
			Other.StartingOwner = EFactionID::Gaul; Other.AdjacentTerritoryIDs = { TEXT("Target") };
			Catalog.RegisterTerritory(Source); Catalog.RegisterTerritory(Target); Catalog.RegisterTerritory(Other);
			FTerritoryCampaignState Live = Catalog;
			Live.FoundSettlement(TEXT("Source"), FText::FromString(TEXT("Saved settlement")));
			Snapshot.Territories = Live.GetRuntimeSnapshot();
			Snapshot.PlayerFaction = EFactionID::Rome;
			Snapshot.DestinationMap = FSoftObjectPath(TEXT("/Game/Maps/Origin.Origin")); // Syntax only; runtime validates real metadata.
			for (EFactionID ID : Active)
			{
				FCampaignFactionWallet Row; Row.Faction = ID;
				Row.Resources.Gold = 901; Row.Resources.Food = 802; Row.Resources.Iron = 703;
				Row.Resources.Wood = 604; Row.Resources.Stone = 505; Row.Resources.Population = 406;
				Snapshot.Factions.Add(Row);
				FCampaignArmy Army; Army.ArmyID = ID == EFactionID::Rome ? Attacker : Defender;
				Army.Faction = ID; Army.TerritoryID = ID == EFactionID::Rome ? TEXT("Source") : TEXT("Target");
				FCampaignArmyUnitStack Stack; Stack.UnitID = TEXT("Legionary"); Stack.Count = ID == EFactionID::Rome ? 15 : 10;
				Army.Units.Add(Stack); Snapshot.Armies.Add(Army);
			}
			FDiplomacyState Diplomacy; Diplomacy.TrySetStatus(EFactionID::Rome, EFactionID::Carthage, EDiplomaticStatus::War);
			Snapshot.Diplomacy = Diplomacy.GetSnapshot();
			Snapshot.Turn.CurrentTurn = 6; Snapshot.Turn.MaxTurns = 100;
			Snapshot.Turn.ConquestVictoryTerritories = 3; Snapshot.Turn.EconomicVictoryGold = 10000;
			Snapshot.Turn.LastAutosaveRequestedTurn = 5; Snapshot.Turn.bCampaignActive = true;
			FRandomStream Random(1337); for (int32 Index = 0; Index < 17; ++Index) Random.GetUnsignedInt();
			Snapshot.Turn.Random = FCampaignRandomCheckpoint::Capture(Random);
		}
		ECampaignTransferFailure Build(const FCampaignTransferSnapshot& Value, FCampaignTransferCandidates& Out) const
		{ return Value.BuildCandidates(Catalog, Active, Out); }
		void PrepareDeployment()
		{
			FCampaignTransferCandidates Values; Build(Snapshot, Values);
			Values.Encounter.Begin(EncounterID, TEXT("Camp"), Attacker, TEXT("Target"), EFactionID::Rome, Values.Armies, Values.Territories);
			Values.Encounter.ExportCheckpoint(Snapshot.Encounter);
			Snapshot.Phase = ECampaignTransferPhase::TacticalDeployment;
			Snapshot.StrategicReturnMap = Snapshot.DestinationMap;
			Snapshot.DestinationMap = FSoftObjectPath(TEXT("/Game/Maps/Battle.Battle"));
		}
	};
	bool SameCandidates(const FCampaignTransferCandidates& A, const FCampaignTransferCandidates& B)
	{
		if (A.PlayerFaction != B.PlayerFaction || A.Wallets.Num() != B.Wallets.Num()) return false;
		for (const auto& Pair : A.Wallets)
		{ const FFactionResourceAmounts* Other = B.Wallets.Find(Pair.Key); if (!Other || !Same(Pair.Value, *Other)) return false; }
		return Same(A.Diplomacy, B.Diplomacy) && Same(A.Territories, B.Territories) && Same(A.Armies, B.Armies) && Same(A.Encounter, B.Encounter) && Same(A.Turn, B.Turn);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTransferDomainsTest, "RomanEmpireGame.Rules.Campaign.TransferDomains",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTransferDomainsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F;
	FTerritoryCampaignState Territories = F.Catalog;
	TestTrue(TEXT("Live rows import independently of authored definitions"), Territories.ReplaceRuntimeSnapshot(F.Snapshot.Territories));
	const FTerritoryCampaignState Before = Territories;
	const auto Reject = [&](TArray<FTerritoryRuntimeSnapshot> Rows)
	{ TestFalse(TEXT("Malformed territory snapshot rejected"), Territories.ReplaceRuntimeSnapshot(Rows)); TestTrue(TEXT("No partial territory replacement"), Same(Territories, Before)); };
	auto Rows = F.Snapshot.Territories; Rows.Pop(); Reject(Rows);
	Rows = F.Snapshot.Territories; const FTerritoryRuntimeSnapshot DuplicateRow = Rows[0]; Rows.Add(DuplicateRow); Reject(Rows);
	Rows = F.Snapshot.Territories; Rows[1].TerritoryID = Rows[0].TerritoryID; Reject(Rows);
	Rows = F.Snapshot.Territories; Rows[0].TerritoryID = TEXT("Foreign"); Reject(Rows);
	Rows = F.Snapshot.Territories; Rows[0].Owner = static_cast<EFactionID>(255); Reject(Rows);
	Rows = F.Snapshot.Territories; Rows[0].Settlement.ProductionBonus.Gold = 1; Reject(Rows); // Unfounded Other.
	Rows = F.Snapshot.Territories; Rows[1].Settlement.SettlementName.Reset(); Reject(Rows); // Founded Source.
	Rows = F.Snapshot.Territories; Rows[1].Settlement.Population = 0; Reject(Rows);
	Rows = F.Snapshot.Territories; Rows[1].Settlement.ProductionBonus.Food = -1; Reject(Rows);
	Rows = F.Snapshot.Territories; Rows[1].Settlement.ProductionBonus.Gold = MAX_int32; Reject(Rows);
	Rows = Territories.GetRuntimeSnapshot(); Rows[0].Owner = EFactionID::Rome;
	EFactionID Owner; Territories.TryGetOwner(TEXT("Other"), Owner);
	TestEqual(TEXT("Export is detached from live ownership"), Owner, EFactionID::Gaul);
	TestTrue(TEXT("Valid full coverage replacement"), Territories.ReplaceRuntimeSnapshot(Rows));
	FTerritoryDefinition Definition; Territories.TryGetDefinition(TEXT("Source"), Definition);
	TestEqual(TEXT("Transfer cannot overwrite authored production"), Definition.ResourceProduction.Gold, 9);
	TestTrue(TEXT("Transfer preserves authored graph"), Territories.AreAdjacent(TEXT("Source"), TEXT("Target")));
	FDiplomacyState Diplomacy; Diplomacy.TrySetStatus(EFactionID::Rome, EFactionID::Carthage, EDiplomaticStatus::War);
	const FDiplomacyState OldDiplomacy = Diplomacy;
	const auto RejectDiplomacy = [&](TArray<FDiplomaticRelation> Relations)
	{ TestFalse(TEXT("Malformed diplomacy rejected"), Diplomacy.ReplaceFromSnapshot(Relations)); TestTrue(TEXT("Diplomacy unchanged on failure"), Same(Diplomacy, OldDiplomacy)); };
	auto Relations = Diplomacy.GetSnapshot(); const FDiplomaticRelation DuplicateRelation = Relations[0]; Relations.Add(DuplicateRelation); RejectDiplomacy(Relations);
	Relations = Diplomacy.GetSnapshot(); Swap(Relations[0].Pair.FirstFaction, Relations[0].Pair.SecondFaction); RejectDiplomacy(Relations);
	Relations = Diplomacy.GetSnapshot(); Relations[0].Status = EDiplomaticStatus::Invalid; RejectDiplomacy(Relations);
	Relations = Diplomacy.GetSnapshot(); Relations[0].Status = EDiplomaticStatus::Neutral; RejectDiplomacy(Relations);
	Relations = Diplomacy.GetSnapshot(); Relations[0].Status = EDiplomaticStatus::Allied;
	TestTrue(TEXT("Restore is not a gameplay transition from war to alliance"), Diplomacy.ReplaceFromSnapshot(Relations));
	TestEqual(TEXT("Restored alliance is symmetric"), Diplomacy.GetStatus(EFactionID::Carthage, EFactionID::Rome), EDiplomaticStatus::Allied);
	TestTrue(TEXT("Empty canonical storage restores implicit neutral"), Diplomacy.ReplaceFromSnapshot({}));
	TestEqual(TEXT("Neutral has no redundant stored row"), Diplomacy.GetStoredRelationCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTransferValidationTest, "RomanEmpireGame.Rules.Campaign.TransferValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTransferValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F; FCampaignTransferCandidates Out;
	TestEqual(TEXT("Complete strategic payload restores staged domains"), F.Build(F.Snapshot, Out), ECampaignTransferFailure::None);
	const FCampaignTransferCandidates Before = Out;
	const auto Reject = [&](const FCampaignTransferSnapshot& Value, ECampaignTransferFailure Failure)
	{ TestEqual(TEXT("Malformed transfer rejected with typed failure"), F.Build(Value, Out), Failure); TestTrue(TEXT("Every staged domain remains unchanged"), SameCandidates(Out, Before)); };
	auto Bad = F.Snapshot; Bad.SchemaVersion = 0; Reject(Bad, ECampaignTransferFailure::UnsupportedVersion);
	Bad = F.Snapshot; Bad.SchemaVersion++; Reject(Bad, ECampaignTransferFailure::UnsupportedVersion);
	Bad = F.Snapshot; Bad.Phase = static_cast<ECampaignTransferPhase>(255); Reject(Bad, ECampaignTransferFailure::InvalidPhase);
	Bad = F.Snapshot; Bad.DestinationMap.Reset(); Reject(Bad, ECampaignTransferFailure::InvalidMapReference);
	Bad = F.Snapshot; Bad.DestinationMap = FSoftObjectPath(TEXT("/Game/Maps/Origin.Origin:Actor")); Reject(Bad, ECampaignTransferFailure::InvalidMapReference);
	Bad = F.Snapshot; Bad.StrategicReturnMap = Bad.DestinationMap; Reject(Bad, ECampaignTransferFailure::InvalidMapReference);
	Bad = F.Snapshot; Bad.PlayerFaction = EFactionID::Gaul; Reject(Bad, ECampaignTransferFailure::InvalidFactionState);
	Bad = F.Snapshot; Bad.Factions.Pop(); Reject(Bad, ECampaignTransferFailure::InvalidFactionState);
	Bad = F.Snapshot; Bad.Factions[1].Faction = Bad.Factions[0].Faction; Reject(Bad, ECampaignTransferFailure::InvalidFactionState);
	Bad = F.Snapshot; Bad.Factions[0].Resources.Stone = -1; Reject(Bad, ECampaignTransferFailure::InvalidFactionState);
	Bad = F.Snapshot; Bad.Diplomacy[0].Pair.SecondFaction = EFactionID::Gaul; Reject(Bad, ECampaignTransferFailure::InvalidFactionState);
	Bad = F.Snapshot; Bad.Territories.Pop(); Reject(Bad, ECampaignTransferFailure::InvalidTerritories);
	Bad = F.Snapshot; Bad.Armies[0].TerritoryID = TEXT("Foreign"); Reject(Bad, ECampaignTransferFailure::InvalidArmies);
	Bad = F.Snapshot; Bad.Armies[0].TerritoryID = TEXT("Target"); Reject(Bad, ECampaignTransferFailure::InvalidArmies);
	Bad = F.Snapshot; Bad.Armies[0].Units[0].Count = 0; Reject(Bad, ECampaignTransferFailure::InvalidArmies);
	Bad = F.Snapshot; Bad.Encounter.AppliedEncounterIDs = { EncounterID, EncounterID }; Reject(Bad, ECampaignTransferFailure::InvalidEncounter);
	Bad = F.Snapshot; Bad.Encounter.EnrolledReceiptIDs.Add(FGuid()); Reject(Bad, ECampaignTransferFailure::InvalidEncounter);
	Bad = F.Snapshot; Bad.Turn.CurrentTurn = 0; Reject(Bad, ECampaignTransferFailure::InvalidTurnState);
	Bad = F.Snapshot; Bad.Turn.MaxTurns = MAX_int32; Reject(Bad, ECampaignTransferFailure::InvalidTurnState);
	Bad = F.Snapshot; Bad.Turn.LastAutosaveRequestedTurn = 7; Reject(Bad, ECampaignTransferFailure::InvalidTurnState);
	Bad = F.Snapshot; Bad.Turn.bCampaignActive = false; Reject(Bad, ECampaignTransferFailure::InvalidTurnState);
	Bad = F.Snapshot; Bad.Turn.Outcome = static_cast<ECampaignOutcome>(255); Reject(Bad, ECampaignTransferFailure::InvalidTurnState);
	FCampaignTurnCheckpoint Terminal = F.Snapshot.Turn; Terminal.CurrentTurn = Terminal.MaxTurns + 1;
	Terminal.bCampaignActive = false;
	for (ECampaignOutcome Outcome : { ECampaignOutcome::Victory, ECampaignOutcome::Defeat, ECampaignOutcome::Draw })
	{ Terminal.Outcome = Outcome; TestTrue(TEXT("Terminal boundary preserves outcomes resolved before final increment"), Terminal.IsValid()); }
	TestEqual(TEXT("Dormant authored owner preserved without activating a wallet"), Out.Territories.GetFactionTerritoryCount(EFactionID::Gaul), 1);
	TestFalse(TEXT("Dormant catalog faction is not a participant"), Out.Wallets.Contains(EFactionID::Gaul));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTransferEncounterTest, "RomanEmpireGame.Rules.Campaign.TransferEncounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTransferEncounterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F; F.PrepareDeployment(); FCampaignTransferCandidates Out;
	TestEqual(TEXT("Unmaterialized frozen deployment imports"), F.Build(F.Snapshot, Out), ECampaignTransferFailure::None);
	TestTrue(TEXT("Encounter identity retained"), Out.Encounter.GetPlan().EncounterID == EncounterID);
	TestTrue(TEXT("World-local bindings are not in a transfer"), Out.Encounter.GetBindings().IsEmpty());
	const FCampaignTransferCandidates Before = Out;
	const auto Reject = [&](const FCampaignTransferSnapshot& Value)
	{ TestEqual(TEXT("Unsafe deployment rejected"), F.Build(Value, Out), ECampaignTransferFailure::InvalidEncounter); TestTrue(TEXT("Unsafe restore leaves previous staged owners intact"), SameCandidates(Out, Before)); };
	auto Bad = F.Snapshot; Bad.Diplomacy.Reset(); Reject(Bad);
	Bad = F.Snapshot; Bad.Phase = ECampaignTransferPhase::Strategic; Bad.StrategicReturnMap.Reset(); Reject(Bad);
	Bad = F.Snapshot; Bad.Encounter.PendingPlan.Armies[0].Units[0].Count++; Reject(Bad);
	Bad = F.Snapshot; const FCampaignArmyUnitStack DuplicateStack = Bad.Encounter.PendingPlan.Armies[0].Units[0];
	Bad.Encounter.PendingPlan.Armies[0].Units.Add(DuplicateStack); Reject(Bad);
	Bad = F.Snapshot; Bad.Encounter.AppliedEncounterIDs.Add(EncounterID); Reject(Bad);
	Out.Encounter.BindSquad(EncounterID, Binding(AttackingSquad, Attacker, 15));
	FCampaignEncounterCheckpoint Checkpoint = F.Snapshot.Encounter;
	const FCampaignEncounterCheckpoint Previous = Checkpoint;
	TestEqual(TEXT("Allocated local roster cannot be travelled/revived as fresh deployment"), Out.Encounter.ExportCheckpoint(Checkpoint), ECampaignEncounterFailure::EncounterAlreadyRunning);
	TestTrue(TEXT("Failed export leaves caller's previous checkpoint intact"), Same(Checkpoint, Previous));
	Out.Encounter.BindSquad(EncounterID, Binding(DefendingSquad, Defender, 10));
	Out.Encounter.EnrollReinforcement(EncounterID, ReceiptID, Binding(PaidSquad, Attacker, 1), Out.Armies, Out.Territories);
	FTacticalBattleResult Result; Result.EncounterID = EncounterID; Result.ObjectiveID = TEXT("Camp"); Result.CapturedObjectiveID = TEXT("Camp");
	Result.Outcome = ETacticalBattleOutcome::PlayerVictory; Result.Winner = EFactionID::Rome;
	FTacticalBattleSquadResult Rome; Rome.SquadID = AttackingSquad; Rome.Faction = EFactionID::Rome; Rome.InitialUnits = 15; Rome.Survivors = 14; Rome.Casualties = 1;
	FTacticalBattleSquadResult Carthage; Carthage.SquadID = DefendingSquad; Carthage.Faction = EFactionID::Carthage; Carthage.InitialUnits = 10; Carthage.Casualties = 10;
	FTacticalBattleSquadResult Paid; Paid.SquadID = PaidSquad; Paid.Faction = EFactionID::Rome; Paid.InitialUnits = 1; Paid.Survivors = 1;
	Result.Squads = { Rome, Carthage, Paid };
	TestEqual(TEXT("Canonical complete result consumed before return export"), Out.Encounter.Apply(Result, Out.Armies, Out.Territories), ECampaignEncounterFailure::None);
	TestEqual(TEXT("Consumed outcome can export without local squads"), Out.Encounter.ExportCheckpoint(Checkpoint), ECampaignEncounterFailure::None);
	TestFalse(TEXT("Consumed plan cannot start another battle on return"), Checkpoint.PendingPlan.EncounterID.IsValid());
	TestEqual(TEXT("Paid receipt ledger survives safe boundary"), Checkpoint.EnrolledReceiptIDs.Num(), 1);
	F.Snapshot.Phase = ECampaignTransferPhase::Strategic; F.Snapshot.DestinationMap = F.Snapshot.StrategicReturnMap; F.Snapshot.StrategicReturnMap.Reset();
	F.Snapshot.Encounter = Checkpoint; F.Snapshot.Territories = Out.Territories.GetRuntimeSnapshot(); F.Snapshot.Armies = Out.Armies.GetArmies();
	FCampaignTransferCandidates Returned;
	TestEqual(TEXT("Updated strategic return payload validates"), F.Build(F.Snapshot, Returned), ECampaignTransferFailure::None);
	TestTrue(TEXT("Replay ledger survives transfer"), Returned.Encounter.HasApplied(EncounterID));
	TestFalse(TEXT("Return has no pending deployed roster"), Returned.Encounter.HasPendingEncounter());
	TestEqual(TEXT("Old result cannot reapply in restored owners"), Returned.Encounter.Apply(Result, Returned.Armies, Returned.Territories), ECampaignEncounterFailure::AlreadyApplied);
	TestEqual(TEXT("Captured ownership survives staged transfer"), Returned.Territories.GetFactionTerritoryCount(EFactionID::Rome), 2);
	Returned.Territories.ChangeOwner(TEXT("Other"), EFactionID::Carthage);
	FCampaignArmy NewDefender;
	NewDefender.ArmyID = FGuid(10, 0, 0, 3); NewDefender.Faction = EFactionID::Carthage; NewDefender.TerritoryID = TEXT("Other");
	FCampaignArmyUnitStack NewStack; NewStack.UnitID = TEXT("Legionary"); NewStack.Count = 10;
	NewDefender.Units.Add(NewStack); Returned.Armies.RegisterArmy(NewDefender);
	const FGuid NextEncounter(11, 0, 0, 3);
	TestEqual(TEXT("Restored owners can prepare a later adjacent deployment"), Returned.Encounter.Begin(NextEncounter,
		TEXT("NextCamp"), Attacker, TEXT("Other"), EFactionID::Rome, Returned.Armies, Returned.Territories), ECampaignEncounterFailure::None);
	Returned.Encounter.BindSquad(NextEncounter, Binding(FGuid(12, 0, 0, 4), Attacker, 15));
	Returned.Encounter.BindSquad(NextEncounter, Binding(FGuid(12, 0, 0, 5), NewDefender.ArmyID, 10));
	TestEqual(TEXT("Transferred paid receipt cannot mint another unit in later encounter"), Returned.Encounter.EnrollReinforcement(NextEncounter,
		ReceiptID, Binding(FGuid(12, 0, 0, 6), Attacker, 1), Returned.Armies, Returned.Territories), ECampaignEncounterFailure::InvalidBinding);
	TestEqual(TEXT("Transferred old outcome cannot consume later pending encounter"), Returned.Encounter.Apply(Result,
		Returned.Armies, Returned.Territories), ECampaignEncounterFailure::AlreadyApplied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTransferArchiveTest, "RomanEmpireGame.Rules.Campaign.TransferArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTransferArchiveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F; F.PrepareDeployment();
	F.Snapshot.Encounter.AppliedEncounterIDs.Add(FGuid(11, 0, 0, 2)); F.Snapshot.Encounter.EnrolledReceiptIDs.Add(ReceiptID);
	const auto Save = [&](FCampaignTransferSnapshot& Value, TArray<uint8>& Bytes)
	{
		FMemoryWriter Writer(Bytes, true); FObjectAndNameAsStringProxyArchive Archive(Writer, false); Archive.ArIsSaveGame = true;
		FCampaignTransferSnapshot::StaticStruct()->SerializeItem(Archive, &Value, nullptr);
		return !Archive.IsError();
	};
	TArray<uint8> Bytes, Again;
	TestTrue(TEXT("Actual reflected SaveGame archive succeeds"), Save(F.Snapshot, Bytes));
	TestTrue(TEXT("Repeated archive succeeds"), Save(F.Snapshot, Again));
	TestTrue(TEXT("Deterministic value payload bytes"), Bytes == Again);
	FCampaignTransferSnapshot Restored;
	FMemoryReader Reader(Bytes, true); FObjectAndNameAsStringProxyArchive Archive(Reader, false); Archive.ArIsSaveGame = true;
	FCampaignTransferSnapshot::StaticStruct()->SerializeItem(Archive, &Restored, nullptr);
	TestFalse(TEXT("Archive load has no error"), Archive.IsError());
	TestTrue(TEXT("Every nested SaveGame field round-trips"), Same(F.Snapshot, Restored));
	FCampaignTransferCandidates Out;
	TestEqual(TEXT("Loaded payload is still valid against the catalog"), F.Build(Restored, Out), ECampaignTransferFailure::None);
	TestEqual(TEXT("Saved nested food retained"), Out.Wallets[EFactionID::Rome].Food, 802);
	FTerritorySettlementState Settlement; Out.Territories.TryGetSettlement(TEXT("Source"), Settlement);
	TestEqual(TEXT("Saved settlement population retained"), Settlement.Population, 100);
	TestEqual(TEXT("Saved nested bonus retained"), Settlement.ProductionBonus.Food, 50);
	FRandomStream Original(1337); for (int32 Index = 0; Index < 17; ++Index) Original.GetUnsignedInt();
	FRandomStream Random;
	TestTrue(TEXT("Both seeds restore through engine reflection"), Restored.Turn.Random.Restore(Random));
	TestEqual(TEXT("Initial random seed survives"), Random.GetInitialSeed(), Original.GetInitialSeed());
	TestEqual(TEXT("Current random seed survives"), Random.GetCurrentSeed(), Original.GetCurrentSeed());
	for (int32 Index = 0; Index < 100; ++Index) TestEqual(TEXT("Next random samples unchanged after transfer"), Random.GetUnsignedInt(), Original.GetUnsignedInt());
	Random.Reset(); TestEqual(TEXT("Reset still uses original initial seed"), Random.GetCurrentSeed(), 1337);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignQueuedDefenseTransferTest, "RomanEmpireGame.Rules.Campaign.QueuedDefenseTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignQueuedDefenseTransferTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F; FCampaignTransferCandidates Out;
	TestEqual(TEXT("Base strategic checkpoint accepted"), F.Build(F.Snapshot, Out), ECampaignTransferFailure::None);
	TestEqual(TEXT("AI attack prepares inverse plan"), Out.Encounter.Begin(EncounterID, TEXT("Camp"), Defender,
		TEXT("Source"), EFactionID::Carthage, Out.Armies, Out.Territories, EFactionID::Rome), ECampaignEncounterFailure::None);
	TestEqual(TEXT("Accepted plan exports without world-local bindings"), Out.Encounter.ExportCheckpoint(F.Snapshot.Encounter), ECampaignEncounterFailure::None);
	F.Snapshot.Turn.bAwaitingAITacticalTravel = true;
	TestEqual(TEXT("Post-turn queued defense is save-safe"), F.Build(F.Snapshot, Out), ECampaignTransferFailure::None);
	TestEqual(TEXT("Human and attacker stay independent"), Out.Encounter.GetPlan().PlayerFaction, EFactionID::Rome);
	const FCampaignTransferCandidates Before = Out;
	const auto Reject = [&](const FCampaignTransferSnapshot& Bad, ECampaignTransferFailure Failure)
	{ TestEqual(TEXT("Invalid queued defense rejected"), F.Build(Bad, Out), Failure); TestTrue(TEXT("Failure is atomic"), SameCandidates(Out, Before)); };
	auto Bad = F.Snapshot; Bad.Turn.bAwaitingAITacticalTravel = false; Reject(Bad, ECampaignTransferFailure::InvalidEncounter);
	Bad = F.Snapshot; Bad.Encounter.PendingPlan = FCampaignEncounterPlan(); Reject(Bad, ECampaignTransferFailure::InvalidEncounter);
	Bad = F.Snapshot; Bad.Encounter.PendingPlan.PlayerFaction = EFactionID::Carthage; Reject(Bad, ECampaignTransferFailure::InvalidEncounter);
	Bad = F.Snapshot; Bad.SchemaVersion = 1; Reject(Bad, ECampaignTransferFailure::UnsupportedVersion);
	Bad = F.Snapshot; Bad.SchemaVersion = FCampaignTransferSnapshot::CurrentSchemaVersion + 1; Reject(Bad, ECampaignTransferFailure::UnsupportedVersion);
	TArray<uint8> Bytes;
	{ FMemoryWriter Writer(Bytes); FObjectAndNameAsStringProxyArchive Archive(Writer, false); Archive.ArIsSaveGame = true;
		FCampaignTransferSnapshot::StaticStruct()->SerializeItem(Archive, &F.Snapshot, nullptr); }
	FCampaignTransferSnapshot Loaded;
	{ FMemoryReader Reader(Bytes); FObjectAndNameAsStringProxyArchive Archive(Reader, false); Archive.ArIsSaveGame = true;
		FCampaignTransferSnapshot::StaticStruct()->SerializeItem(Archive, &Loaded, nullptr); }
	TestTrue(TEXT("Queued intent and human perspective survive reflected archive"), Same(Loaded, F.Snapshot));
	F.Snapshot.Phase = ECampaignTransferPhase::TacticalDeployment;
	F.Snapshot.StrategicReturnMap = F.Snapshot.DestinationMap;
	F.Snapshot.DestinationMap = FSoftObjectPath(TEXT("/Game/Maps/Battle.Battle"));
	Reject(F.Snapshot, ECampaignTransferFailure::InvalidEncounter);
	F.Snapshot.Turn.bAwaitingAITacticalTravel = false;
	TestEqual(TEXT("Actual inverse deployment imports after queue dispatched"), F.Build(F.Snapshot, Out), ECampaignTransferFailure::None);
	TestEqual(TEXT("Incoming AI attack remains Carthaginian"), Out.Encounter.GetPlan().Attacker, EFactionID::Carthage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignSaveEnvelopeTest, "RomanEmpireGame.Rules.SaveGame.EnvelopeRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignSaveEnvelopeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F;
	FCampaignSaveRecord Record; Record.Campaign = F.Snapshot; Record.Generation = 9; Record.Settings.bAutosaveEnabled = false;
	Record.Campaign.Encounter.AppliedEncounterIDs.Add(EncounterID); Record.Campaign.Encounter.EnrolledReceiptIDs.Add(ReceiptID);
	TArray<uint8> Bytes, Again;
	TestEqual(TEXT("Versioned envelope encodes saved values"), RomanCampaignSave::Encode(Record, Bytes), ECampaignSaveFailure::None);
	TestEqual(TEXT("Repeated encode succeeds"), RomanCampaignSave::Encode(Record, Again), ECampaignSaveFailure::None);
	TestTrue(TEXT("Stable envelope is deterministic"), Bytes == Again);
	FCampaignSaveRecord Loaded;
	TestEqual(TEXT("Envelope decodes"), RomanCampaignSave::Decode(Bytes, Loaded), ECampaignSaveFailure::None);
	TestTrue(TEXT("Generation/settings/every domain/RNG/ledgers round-trip"), Same(Record, Loaded));
	FCampaignTransferCandidates Candidates;
	TestEqual(TEXT("Decoded value still binds against current catalog"), F.Build(Loaded.Campaign, Candidates), ECampaignTransferFailure::None);
	FRandomStream Saved, Restored; Record.Campaign.Turn.Random.Restore(Saved); Loaded.Campaign.Turn.Random.Restore(Restored);
	for (int32 Index = 0; Index < 20; ++Index) TestEqual(TEXT("Next RNG values are identical"), Saved.GetUnsignedInt(), Restored.GetUnsignedInt());
	TStrongObjectPtr<URomanCampaignSaveGame> Object(NewObject<URomanCampaignSaveGame>()); Object->Record = Record;
	TArray<uint8> EngineBytes;
	TestTrue(TEXT("Dedicated USaveGame supports actual engine memory archive"), UGameplayStatics::SaveGameToMemory(Object.Get(), EngineBytes));
	TStrongObjectPtr<URomanCampaignSaveGame> EngineLoaded(Cast<URomanCampaignSaveGame>(UGameplayStatics::LoadGameFromMemory(EngineBytes)));
	if (TestNotNull(TEXT("Actual USaveGame class restores"), EngineLoaded.Get())) TestTrue(TEXT("Engine archive preserves the record"), Same(EngineLoaded->Record, Record));
	const auto Reject = [&](TArray<uint8> Bad, ECampaignSaveFailure Failure)
	{ TestEqual(TEXT("Malformed envelope fails explicitly"), RomanCampaignSave::Decode(Bad, Loaded), Failure); TestTrue(TEXT("Rejected decode cannot change previous saved value"), Same(Record, Loaded)); };
	Reject({}, ECampaignSaveFailure::CorruptData);
	auto Bad = Bytes; Bad.SetNum(RomanCampaignSave::HeaderBytes - 1); Reject(Bad, ECampaignSaveFailure::CorruptData);
	Bad = Bytes; Bad.Pop(); Reject(Bad, ECampaignSaveFailure::CorruptData);
	Bad = Bytes; Bad.Add(0); Reject(Bad, ECampaignSaveFailure::CorruptData);
	Bad = Bytes; Bad[0] ^= 1; Reject(Bad, ECampaignSaveFailure::CorruptData);
	Bad = Bytes; Bad.Last() ^= 1; Reject(Bad, ECampaignSaveFailure::CorruptData);
	Bad = Bytes; Bad[12] ^= 1; Reject(Bad, ECampaignSaveFailure::CorruptData);
	Bad = Bytes; for (int32 Index = 8; Index < 12; ++Index) Bad[Index] = 255; Reject(Bad, ECampaignSaveFailure::CorruptData);
	for (uint8 Version : { uint8(0), uint8(2) }) { Bad = Bytes; Bad[4] = Version; Reject(Bad, ECampaignSaveFailure::UnsupportedVersion); }
	const auto BeforeBytes = Bytes;
	auto Invalid = Record; Invalid.Campaign.SchemaVersion = 1;
	TestEqual(TEXT("Old nested transfer schema explicitly invalidated"), RomanCampaignSave::Encode(Invalid, Bytes), ECampaignSaveFailure::UnsupportedVersion);
	Invalid = Record; Invalid.Campaign.Phase = ECampaignTransferPhase::TacticalDeployment;
	TestEqual(TEXT("No incomplete tactical actor graph save"), RomanCampaignSave::Encode(Invalid, Bytes), ECampaignSaveFailure::UnsafeBoundary);
	Invalid = Record; Invalid.Generation = 0;
	TestEqual(TEXT("Bad generation cannot be saved"), RomanCampaignSave::Encode(Invalid, Bytes), ECampaignSaveFailure::InvalidCampaign);
	TestTrue(TEXT("Failed encoding preserves previous bytes"), BeforeBytes == Bytes);
	FFixture Queued; FCampaignTransferCandidates Pending;
	Queued.Build(Queued.Snapshot, Pending);
	TestEqual(TEXT("Save fixture accepts queued AI defense"), Pending.Encounter.Begin(EncounterID, TEXT("Camp"), Defender,
		TEXT("Source"), EFactionID::Carthage, Pending.Armies, Pending.Territories, EFactionID::Rome), ECampaignEncounterFailure::None);
	Pending.Encounter.ExportCheckpoint(Queued.Snapshot.Encounter); Queued.Snapshot.Turn.bAwaitingAITacticalTravel = true;
	FCampaignSaveRecord QueuedRecord; QueuedRecord.Campaign = Queued.Snapshot;
	TestEqual(TEXT("Queued post-turn intent is a safe strategic save"), RomanCampaignSave::Encode(QueuedRecord, Bytes), ECampaignSaveFailure::None);
	TestEqual(TEXT("Queued intent decodes"), RomanCampaignSave::Decode(Bytes, Loaded), ECampaignSaveFailure::None);
	TestTrue(TEXT("Accepted identity/human/retry flag round-trip unchanged"), Same(QueuedRecord, Loaded));
	TestEqual(TEXT("Loaded queued intent validates without materializing actors"), Queued.Build(Loaded.Campaign, Pending), ECampaignTransferFailure::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignSavePayloadValidationTest, "RomanEmpireGame.Rules.SaveGame.PayloadValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignSavePayloadValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFixture F;
	FCampaignSaveRecord Record; Record.Campaign = F.Snapshot; Record.Generation = 9;
	Record.Settings.bAutosaveEnabled = false;
	// Independent bounded wire fixtures deliberately bypass Encode's input validation.
	// No arbitrary offsets/counts, random fuzz, object-class headers or disk writes.
	const auto WrapPayload = [](const TArray<uint8>& Payload)
	{
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes, true);
		uint32 Magic = RomanCampaignSave::Magic, Version = RomanCampaignSave::CurrentVersion;
		uint32 CRC = FCrc::MemCrc32(Payload.GetData(), Payload.Num()); int32 Size = Payload.Num();
		Writer << Magic << Version << Size << CRC;
		Writer.Serialize(const_cast<uint8*>(Payload.GetData()), Size);
		return Bytes;
	};
	const auto SerializeValue = [&](const FCampaignSaveRecord& Value)
	{
		FCampaignSaveRecord Detached = Value;
		TArray<uint8> Payload; FMemoryWriter Writer(Payload, true);
		FObjectAndNameAsStringProxyArchive Archive(Writer, false); Archive.ArIsSaveGame = true;
		FCampaignSaveRecord::StaticStruct()->SerializeItem(Archive, &Detached, nullptr);
		TestFalse(TEXT("Controlled raw fixture serializes without archive error"), Archive.IsError());
		return Payload;
	};
	const auto Reject = [&](const TArray<uint8>& Payload, ECampaignSaveFailure Expected)
	{
		const auto Bytes = WrapPayload(Payload); const auto Before = Bytes;
		FMemoryReader Header(Bytes, true); uint32 Magic = 0, Version = 0, CRC = 0; int32 Size = 0;
		Header << Magic << Version << Size << CRC;
		TestFalse(TEXT("Malformed value has a readable outer header"), Header.IsError());
		TestEqual(TEXT("Malformed value has correct outer magic"), Magic, RomanCampaignSave::Magic);
		TestEqual(TEXT("Malformed value has supported outer version"), Version, RomanCampaignSave::CurrentVersion);
		TestTrue(TEXT("Malformed value has a nonempty bounded exact payload length"),
			Size > 0 && Size <= RomanCampaignSave::MaxPayloadBytes && Size == Bytes.Num() - RomanCampaignSave::HeaderBytes);
		TestEqual(TEXT("Malformed value has the correct CRC, not a checksum-only rejection"), CRC,
			FCrc::MemCrc32(Bytes.GetData() + RomanCampaignSave::HeaderBytes, Size));
		FCampaignSaveRecord Output = Record;
		TestEqual(TEXT("Malformed reflected value fails explicitly past the outer integrity checks"), RomanCampaignSave::Decode(Bytes, Output), Expected);
		TestTrue(TEXT("Rejected reflected value preserves ALL previous output fields"), Same(Output, Record));
		TestTrue(TEXT("Decode never mutates input bytes"), Bytes == Before);
	};
	const auto Payload = SerializeValue(Record);
	TArray<uint8> Encoded;
	if (!TestEqual(TEXT("Valid fixture encodes"), RomanCampaignSave::Encode(Record, Encoded), ECampaignSaveFailure::None)) return false;
	TestTrue(TEXT("Independent wire builder reproduces the production envelope exactly"), Encoded == WrapPayload(Payload));
	if (!TestTrue(TEXT("Valid archive has enough bytes for the bounded truncation cases"), Payload.Num() > 8)) return false;
	// Partial initial length prefixes and partial final terminators cannot be valid records.
	for (int32 Count : {1, 2, 3})
	{
		TArray<uint8> Prefix; Prefix.Append(Payload.GetData(), Count);
		Reject(Prefix, ECampaignSaveFailure::CorruptData);
		auto Truncated = Payload; Truncated.SetNum(Payload.Num() - Count);
		Reject(Truncated, ECampaignSaveFailure::CorruptData);
	}
	for (int32 Count : {1, 4})
	{
		auto Trailing = Payload; Trailing.AddZeroed(Count);
		Reject(Trailing, ECampaignSaveFailure::CorruptData);
	}
	for (int64 Generation : {int64(0), int64(-1), int64(MAX_int64)})
	{
		auto Invalid = Record; Invalid.Generation = Generation;
		Reject(SerializeValue(Invalid), ECampaignSaveFailure::InvalidCampaign);
	}
	auto Invalid = Record; Invalid.Campaign.SchemaVersion = FCampaignTransferSnapshot::CurrentSchemaVersion + 1;
	Reject(SerializeValue(Invalid), ECampaignSaveFailure::UnsupportedVersion);
	Invalid = Record; Invalid.Campaign.Phase = ECampaignTransferPhase::TacticalDeployment;
	Reject(SerializeValue(Invalid), ECampaignSaveFailure::UnsafeBoundary);
	Invalid = Record; Invalid.Campaign.DestinationMap.Reset();
	Reject(SerializeValue(Invalid), ECampaignSaveFailure::InvalidCampaign);
	Invalid = Record; Invalid.Campaign.StrategicReturnMap = Invalid.Campaign.DestinationMap;
	Reject(SerializeValue(Invalid), ECampaignSaveFailure::InvalidCampaign);
	FCampaignSaveRecord Recovered;
	TestEqual(TEXT("Valid decode still succeeds after all malformed fixtures"), RomanCampaignSave::Decode(Encoded, Recovered), ECampaignSaveFailure::None);
	TestTrue(TEXT("Recovered decode is exact across every field"), Same(Recovered, Record));
	return true;
}

} // namespace RomanEmpireCampaignTransferTests
#endif
