// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Campaign/CampaignArmyState.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FCampaignArmy MakeArmy(FGuid ID, EFactionID Faction, FName Territory, int32 Count)
	{
		FCampaignArmy Army;
		Army.ArmyID = ID; Army.Faction = Faction; Army.TerritoryID = Territory;
		FCampaignArmyUnitStack Stack;
		Stack.UnitID = TEXT("Legionary"); Stack.Count = Count;
		Army.Units.Add(Stack);
		return Army;
	}
	bool Same(const FCampaignArmyState& A, const FCampaignArmyState& B)
	{
		return FCampaignArmyState::StaticStruct()->CompareScriptStruct(&A, &B, 0);
	}
	TArray<uint8> Serialize(const FCampaignArmyState& State)
	{
		FCampaignArmyState Copy = State;
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		FObjectAndNameAsStringProxyArchive Archive(Writer, false);
		Archive.ArIsSaveGame = true;
		FCampaignArmyState::StaticStruct()->SerializeItem(Archive, &Copy, nullptr);
		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignArmyStateTest, "RomanEmpireGame.Rules.Campaign.ArmyState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignArmyStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FCampaignArmyState State;
	const FGuid First(1, 2, 3, 4), Second(1, 2, 3, 5);
	const FCampaignArmy Rome = MakeArmy(First, EFactionID::Rome, TEXT("Rome"), 15);
	const FCampaignArmy Carthage = MakeArmy(Second, EFactionID::Carthage, TEXT("Africa"), 10);
	TestTrue(TEXT("Register Rome army"), State.RegisterArmy(Rome).WasSuccessful());
	TestTrue(TEXT("Register Carthage army"), State.RegisterArmy(Carthage).WasSuccessful());
	const FCampaignArmyState Baseline = State;
	TestEqual(TEXT("Duplicate cannot replace inventory"), State.RegisterArmy(Rome).Failure, ECampaignArmyFailure::DuplicateArmyID);
	const auto Reject = [&](FCampaignArmy Army, ECampaignArmyFailure Failure)
	{
		TestEqual(TEXT("Malformed army has typed rejection"), State.RegisterArmy(Army).Failure, Failure);
		TestTrue(TEXT("Malformed army leaves entire roster unchanged"), Same(State, Baseline));
	};
	FCampaignArmy Bad = Rome; Bad.ArmyID.Invalidate(); Reject(Bad, ECampaignArmyFailure::InvalidArmyID);
	Bad = MakeArmy(FGuid(5, 0, 0, 1), EFactionID::None, TEXT("Rome"), 1); Reject(Bad, ECampaignArmyFailure::InvalidFaction);
	Bad.Faction = static_cast<EFactionID>(255); Reject(Bad, ECampaignArmyFailure::InvalidFaction);
	Bad.Faction = EFactionID::Rome; Bad.TerritoryID = TEXT(" \t "); Reject(Bad, ECampaignArmyFailure::InvalidTerritoryID);
	Bad.TerritoryID = TEXT("Rome"); Bad.Units[0].UnitID = NAME_None; Reject(Bad, ECampaignArmyFailure::InvalidUnitID);
	Bad.Units[0].UnitID = TEXT("Legionary"); Bad.Units[0].Count = 0; Reject(Bad, ECampaignArmyFailure::InvalidUnitCount);
	Bad.Units[0].Count = -1; Reject(Bad, ECampaignArmyFailure::InvalidUnitCount);
	Bad.Units.Reset(); Reject(Bad, ECampaignArmyFailure::InvalidUnitCount);
	Bad = MakeArmy(FGuid(5, 0, 0, 1), EFactionID::Rome, TEXT("Rome"), 1);
	const FCampaignArmyUnitStack DuplicateStack = Bad.Units[0];
	Bad.Units.Add(DuplicateStack); Reject(Bad, ECampaignArmyFailure::DuplicateUnitID);
	Bad.Units[1].UnitID = TEXT("Archer"); Bad.Units[0].Count = MAX_int32; Reject(Bad, ECampaignArmyFailure::CountOverflow);
	TestEqual(TEXT("Unknown add rejects"), State.AddUnits(FGuid(), TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::UnknownArmy);
	TestEqual(TEXT("Nonpositive add rejects"), State.AddUnits(First, TEXT("Legionary"), 0).Failure, ECampaignArmyFailure::InvalidUnitCount);
	TestEqual(TEXT("Add cannot overflow total across stacks"), State.AddUnits(First, TEXT("Archer"), MAX_int32).Failure, ECampaignArmyFailure::CountOverflow);
	TestEqual(TEXT("Over-removal rejects"), State.RemoveUnits(First, TEXT("Legionary"), 16).Failure, ECampaignArmyFailure::InsufficientUnits);
	TestEqual(TEXT("Absent stack rejects"), State.RemoveUnits(First, TEXT("Archer"), 1).Failure, ECampaignArmyFailure::InsufficientUnits);
	TestEqual(TEXT("Invalid location rejects"), State.RelocateArmy(First, NAME_None).Failure, ECampaignArmyFailure::InvalidTerritoryID);
	TestTrue(TEXT("Rejected mutations preserve complete baseline"), Same(State, Baseline));
	TestTrue(TEXT("Merge existing stack"), State.AddUnits(First, TEXT("Legionary"), 5).WasSuccessful());
	TestTrue(TEXT("Add second content stack"), State.AddUnits(First, TEXT("Archer"), 3).WasSuccessful());
	FCampaignArmy Read;
	State.TryGetArmy(First, Read);
	TestEqual(TEXT("Content IDs have deterministic lexical order"), Read.Units[0].UnitID, FName(TEXT("Archer")));
	TestEqual(TEXT("Combined quantity"), FCampaignArmyState::GetUnitCount(Read), 23);
	Read.Units[0].Count = 999; Read.TerritoryID = TEXT("Other");
	State.TryGetArmy(First, Read);
	TestEqual(TEXT("Query is detached"), FCampaignArmyState::GetUnitCount(Read), 23);
	TestEqual(TEXT("Read mutation cannot relocate canonical army"), Read.TerritoryID, FName(TEXT("Rome")));
	TestTrue(TEXT("Remove depleted stack"), State.RemoveUnits(First, TEXT("Archer"), 3).WasSuccessful());
	TestTrue(TEXT("Relocate identity without changing composition"), State.RelocateArmy(First, TEXT("Sicily")).WasSuccessful());
	State.TryGetArmy(First, Read);
	TestEqual(TEXT("Persistent identity retained"), Read.ArmyID, First);
	TestEqual(TEXT("Relocation committed"), Read.TerritoryID, FName(TEXT("Sicily")));
	const FCampaignArmyState BeforeBadImport = State;
	TestFalse(TEXT("Mixed valid/invalid snapshot rejects atomically"), State.ReplaceFromSnapshot({ Carthage, Bad }).WasSuccessful());
	TestTrue(TEXT("Failed import preserves every old army"), Same(State, BeforeBadImport));
	TestEqual(TEXT("Duplicate import rejects atomically"), State.ReplaceFromSnapshot({ Rome, Rome }).Failure, ECampaignArmyFailure::DuplicateArmyID);
	TestTrue(TEXT("Duplicate import preserves every old army"), Same(State, BeforeBadImport));
	TestTrue(TEXT("Reverse insertion imports"), State.ReplaceFromSnapshot({ Carthage, Rome }).WasSuccessful());
	TestTrue(TEXT("Canonical snapshots ignore insertion order"), Same(State, Baseline));
	TestTrue(TEXT("Complete loss retires empty army"), State.RemoveUnits(First, TEXT("Legionary"), 15).WasSuccessful());
	TestFalse(TEXT("Retired lookup rejects and clears stale output"), State.TryGetArmy(First, Read));
	TestFalse(TEXT("Cleared output has no invented identity"), Read.ArmyID.IsValid());
	TestTrue(TEXT("Cleared output has no stale units"), Read.Units.IsEmpty());
	State.Reset(); TestEqual(TEXT("Reset clears roster"), State.GetArmyCount(), 0);
	FCampaignArmy Boundary = Rome; Boundary.Units[0].Count = MAX_int32;
	TestTrue(TEXT("Exact count limit is valid"), State.RegisterArmy(Boundary).WasSuccessful());
	TestEqual(TEXT("Exact limit cannot wrap on add"), State.AddUnits(First, TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::CountOverflow);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignArmyTransferTest, "RomanEmpireGame.Rules.Campaign.ArmyAssignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignArmyTransferTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid A(1, 0, 0, 1), B(1, 0, 0, 2), C(1, 0, 0, 3), D(1, 0, 0, 4);
	FCampaignArmyState State;
	State.ReplaceFromSnapshot({ MakeArmy(A, EFactionID::Rome, TEXT("Rome"), 15), MakeArmy(B, EFactionID::Rome, TEXT("Rome"), 3),
		MakeArmy(C, EFactionID::Carthage, TEXT("Rome"), 10), MakeArmy(D, EFactionID::Rome, TEXT("Sicily"), 2) });
	const FCampaignArmyState Baseline = State;
	const auto Reject = [&](FGuid From, FGuid To, FName Unit, int32 Count, ECampaignArmyFailure Expected)
	{
		TestEqual(TEXT("Assignment rejects with typed reason"), State.TransferUnits(From, To, Unit, Count).Failure, Expected);
		TestTrue(TEXT("Rejection leaves every inventory byte unchanged"), Serialize(State) == Serialize(Baseline));
	};
	Reject(FGuid(), B, TEXT("Legionary"), 1, ECampaignArmyFailure::InvalidArmyID);
	Reject(A, FGuid(8, 0, 0, 1), TEXT("Legionary"), 1, ECampaignArmyFailure::UnknownArmy);
	Reject(A, A, TEXT("Legionary"), 1, ECampaignArmyFailure::SameArmy);
	Reject(A, C, TEXT("Legionary"), 1, ECampaignArmyFailure::InvalidFaction);
	Reject(A, D, TEXT("Legionary"), 1, ECampaignArmyFailure::NotCoLocated);
	Reject(A, B, NAME_None, 1, ECampaignArmyFailure::InvalidUnitID);
	Reject(A, B, TEXT("Archer"), 1, ECampaignArmyFailure::InsufficientUnits);
	Reject(A, B, TEXT("Legionary"), 0, ECampaignArmyFailure::InvalidUnitCount);
	Reject(A, B, TEXT("Legionary"), -1, ECampaignArmyFailure::InvalidUnitCount);
	Reject(A, B, TEXT("Legionary"), 16, ECampaignArmyFailure::InsufficientUnits);
	TestTrue(TEXT("Valid partial assignment commits"), State.TransferUnits(A, B, TEXT("Legionary"), 5).WasSuccessful());
	FCampaignArmy Source, Target;
	State.TryGetArmy(A, Source); State.TryGetArmy(B, Target);
	TestEqual(TEXT("Source retains identity and remainder"), FCampaignArmyState::GetUnitCount(Source), 10);
	TestEqual(TEXT("Destination merges quantity without minting"), FCampaignArmyState::GetUnitCount(Target), 8);
	TestTrue(TEXT("Assign all remaining units"), State.TransferUnits(A, B, TEXT("Legionary"), 10).WasSuccessful());
	TestFalse(TEXT("Empty source retires, rather than leaving invalid save row"), State.TryGetArmy(A, Source));
	State.TryGetArmy(B, Target); TestEqual(TEXT("Total quantity conserved"), FCampaignArmyState::GetUnitCount(Target), 18);
	TestEqual(TEXT("Destination stable identity retained"), Target.ArmyID, B);
	TestTrue(TEXT("Multi-content source is valid"), State.AddUnits(B, TEXT("Archer"), 4).WasSuccessful());
	TestTrue(TEXT("Bring third friendly army to same region"), State.RelocateArmy(D, TEXT("Rome")).WasSuccessful());
	TestTrue(TEXT("Transfer a new destination content stack"), State.TransferUnits(B, D, TEXT("Archer"), 4).WasSuccessful());
	State.TryGetArmy(D, Target); TestEqual(TEXT("Destination remains lexically content-sorted"), Target.Units[0].UnitID, FName(TEXT("Archer")));
	State.TryGetArmy(B, Source); TestEqual(TEXT("Depleted stack removed but source army survives"), Source.Units.Num(), 1);
	FCampaignArmyState Overflow;
	Overflow.ReplaceFromSnapshot({ MakeArmy(A, EFactionID::Rome, TEXT("Rome"), 1), MakeArmy(B, EFactionID::Rome, TEXT("Rome"), MAX_int32) });
	const auto Before = Serialize(Overflow);
	TestEqual(TEXT("Overflow detected after candidate source removal"), Overflow.TransferUnits(A, B, TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::CountOverflow);
	TestTrue(TEXT("Overflow rolls back both candidate rows, including source retirement"), Before == Serialize(Overflow));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignArmySerializationTest, "RomanEmpireGame.Rules.Campaign.ArmySerialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignArmySerializationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FCampaignArmy A = MakeArmy(FGuid(1, 2, 3, 4), EFactionID::Rome, TEXT("Rome"), 15);
	const FCampaignArmy B = MakeArmy(FGuid(1, 2, 3, 5), EFactionID::Carthage, TEXT("Africa"), 10);
	FCampaignArmyState Source, Reverse;
	Source.ReplaceFromSnapshot({ A, B }); Reverse.ReplaceFromSnapshot({ B, A });
	TArray<uint8> Bytes = Serialize(Source);
	TestTrue(TEXT("SaveGame archive contains serialized values"), Bytes.Num() > 0);
	TestTrue(TEXT("Canonical ordering yields identical serialized bytes"), Bytes == Serialize(Reverse));
	FCampaignArmyState Decoded;
	FMemoryReader Reader(Bytes, true);
	FObjectAndNameAsStringProxyArchive Archive(Reader, false);
	Archive.ArIsSaveGame = true;
	FCampaignArmyState::StaticStruct()->SerializeItem(Archive, &Decoded, nullptr);
	TestFalse(TEXT("Real SaveGame reflection archive has no decode error"), Archive.IsError());
	TestTrue(TEXT("IDs, factions, territories and composition survive round-trip"), Same(Source, Decoded));
	FCampaignArmyState Imported;
	TestTrue(TEXT("Decoded snapshot validates into fresh owner"), Imported.ReplaceFromSnapshot(Decoded.GetArmies()).WasSuccessful());
	Source.Reset();
	TestEqual(TEXT("New owner survives original owner reset"), Imported.GetArmyCount(), 2);
	for (const UScriptStruct* Struct : { FCampaignArmyState::StaticStruct(), FCampaignArmy::StaticStruct(), FCampaignArmyUnitStack::StaticStruct() })
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* Property = *It;
			if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) Property = Array->Inner;
			TestFalse(TEXT("Serialized army values contain no actor/UObject references"), Property->IsA<FObjectPropertyBase>());
			TestTrue(TEXT("Every army schema field is SaveGame tracked"), It->HasAnyPropertyFlags(CPF_SaveGame));
		}
	}
	return true;
}
#endif
