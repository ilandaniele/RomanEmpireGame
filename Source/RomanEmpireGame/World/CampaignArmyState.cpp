// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignArmyState.h"
#include "Content/RomanContentDefinitions.h"

namespace
{
	FCampaignArmyMutationResult Result(FGuid ArmyID, ECampaignArmyFailure Failure)
	{
		FCampaignArmyMutationResult Value;
		Value.ArmyID = ArmyID;
		Value.Failure = Failure;
		return Value;
	}
	bool GuidLess(const FCampaignArmy& Left, const FCampaignArmy& Right)
	{
		const FGuid& A = Left.ArmyID;
		const FGuid& B = Right.ArmyID;
		if (A.A != B.A) return A.A < B.A;
		if (A.B != B.B) return A.B < B.B;
		if (A.C != B.C) return A.C < B.C;
		return A.D < B.D;
	}
}

int32 FCampaignArmyState::FindArmy(FGuid ArmyID) const
{
	return Armies.IndexOfByPredicate([ArmyID](const FCampaignArmy& Army) { return Army.ArmyID == ArmyID; });
}

void FCampaignArmyState::SortUnits(FCampaignArmy& Army)
{
	Army.Units.Sort([](const FCampaignArmyUnitStack& A, const FCampaignArmyUnitStack& B) { return A.UnitID.LexicalLess(B.UnitID); });
}

ECampaignArmyFailure FCampaignArmyState::ValidateArmy(const FCampaignArmy& Army)
{
	if (!Army.ArmyID.IsValid()) return ECampaignArmyFailure::InvalidArmyID;
	if (!RomanEmpireContent::IsRuntimeFaction(Army.Faction)) return ECampaignArmyFailure::InvalidFaction;
	if (!RomanEmpireContent::IsStableID(Army.TerritoryID)) return ECampaignArmyFailure::InvalidTerritoryID;
	if (Army.Units.IsEmpty()) return ECampaignArmyFailure::InvalidUnitCount;
	TSet<FName> Seen;
	int64 Total = 0;
	for (const FCampaignArmyUnitStack& Stack : Army.Units)
	{
		if (!RomanEmpireContent::IsStableID(Stack.UnitID)) return ECampaignArmyFailure::InvalidUnitID;
		if (Seen.Contains(Stack.UnitID)) return ECampaignArmyFailure::DuplicateUnitID;
		if (Stack.Count <= 0) return ECampaignArmyFailure::InvalidUnitCount;
		Seen.Add(Stack.UnitID);
		Total += Stack.Count;
		if (Total > MAX_int32) return ECampaignArmyFailure::CountOverflow;
	}
	return ECampaignArmyFailure::None;
}

int32 FCampaignArmyState::GetUnitCount(const FCampaignArmy& Army)
{
	if (ValidateArmy(Army) != ECampaignArmyFailure::None) return 0;
	int32 Total = 0;
	for (const FCampaignArmyUnitStack& Stack : Army.Units) Total += Stack.Count;
	return Total;
}

FCampaignArmyMutationResult FCampaignArmyState::RegisterArmy(const FCampaignArmy& Army)
{
	const ECampaignArmyFailure Failure = ValidateArmy(Army);
	if (Failure != ECampaignArmyFailure::None) return Result(Army.ArmyID, Failure);
	if (FindArmy(Army.ArmyID) != INDEX_NONE) return Result(Army.ArmyID, ECampaignArmyFailure::DuplicateArmyID);
	FCampaignArmy Copy = Army;
	SortUnits(Copy);
	Armies.Add(MoveTemp(Copy));
	Armies.Sort(GuidLess);
	return Result(Army.ArmyID, ECampaignArmyFailure::None);
}

FCampaignArmyMutationResult FCampaignArmyState::AddUnits(FGuid ArmyID, FName UnitID, int32 Count)
{
	const int32 Index = FindArmy(ArmyID);
	if (Index == INDEX_NONE) return Result(ArmyID, ECampaignArmyFailure::UnknownArmy);
	if (!RomanEmpireContent::IsStableID(UnitID)) return Result(ArmyID, ECampaignArmyFailure::InvalidUnitID);
	if (Count <= 0) return Result(ArmyID, ECampaignArmyFailure::InvalidUnitCount);
	if (int64(GetUnitCount(Armies[Index])) + Count > MAX_int32) return Result(ArmyID, ECampaignArmyFailure::CountOverflow);
	FCampaignArmy& Army = Armies[Index];
	if (FCampaignArmyUnitStack* Stack = Army.Units.FindByPredicate([UnitID](const FCampaignArmyUnitStack& Value) { return Value.UnitID == UnitID; }))
		Stack->Count += Count;
	else
	{
		FCampaignArmyUnitStack NewStack;
		NewStack.UnitID = UnitID;
		NewStack.Count = Count;
		Army.Units.Add(NewStack);
		SortUnits(Army);
	}
	return Result(ArmyID, ECampaignArmyFailure::None);
}

FCampaignArmyMutationResult FCampaignArmyState::RemoveUnits(FGuid ArmyID, FName UnitID, int32 Count)
{
	const int32 Index = FindArmy(ArmyID);
	if (Index == INDEX_NONE) return Result(ArmyID, ECampaignArmyFailure::UnknownArmy);
	if (!RomanEmpireContent::IsStableID(UnitID)) return Result(ArmyID, ECampaignArmyFailure::InvalidUnitID);
	if (Count <= 0) return Result(ArmyID, ECampaignArmyFailure::InvalidUnitCount);
	FCampaignArmy& Army = Armies[Index];
	const int32 StackIndex = Army.Units.IndexOfByPredicate([UnitID](const FCampaignArmyUnitStack& Value) { return Value.UnitID == UnitID; });
	if (StackIndex == INDEX_NONE || Army.Units[StackIndex].Count < Count) return Result(ArmyID, ECampaignArmyFailure::InsufficientUnits);
	Army.Units[StackIndex].Count -= Count;
	if (Army.Units[StackIndex].Count == 0) Army.Units.RemoveAt(StackIndex);
	if (Army.Units.IsEmpty()) Armies.RemoveAt(Index);
	return Result(ArmyID, ECampaignArmyFailure::None);
}

FCampaignArmyMutationResult FCampaignArmyState::RelocateArmy(FGuid ArmyID, FName TerritoryID)
{
	const int32 Index = FindArmy(ArmyID);
	if (Index == INDEX_NONE) return Result(ArmyID, ECampaignArmyFailure::UnknownArmy);
	if (!RomanEmpireContent::IsStableID(TerritoryID)) return Result(ArmyID, ECampaignArmyFailure::InvalidTerritoryID);
	Armies[Index].TerritoryID = TerritoryID;
	return Result(ArmyID, ECampaignArmyFailure::None);
}

FCampaignArmyMutationResult FCampaignArmyState::TransferUnits(FGuid SourceArmyID, FGuid DestinationArmyID, FName UnitID, int32 Count)
{
	if (!SourceArmyID.IsValid() || !DestinationArmyID.IsValid()) return Result(DestinationArmyID, ECampaignArmyFailure::InvalidArmyID);
	if (SourceArmyID == DestinationArmyID) return Result(DestinationArmyID, ECampaignArmyFailure::SameArmy);
	FCampaignArmy Source, Destination;
	if (!TryGetArmy(SourceArmyID, Source) || !TryGetArmy(DestinationArmyID, Destination)) return Result(DestinationArmyID, ECampaignArmyFailure::UnknownArmy);
	if (Source.Faction != Destination.Faction) return Result(DestinationArmyID, ECampaignArmyFailure::InvalidFaction);
	if (Source.TerritoryID != Destination.TerritoryID) return Result(DestinationArmyID, ECampaignArmyFailure::NotCoLocated);
	FCampaignArmyState Candidate = *this;
	const auto Removed = Candidate.RemoveUnits(SourceArmyID, UnitID, Count);
	if (!Removed.WasSuccessful()) return Result(DestinationArmyID, Removed.Failure);
	const auto Added = Candidate.AddUnits(DestinationArmyID, UnitID, Count);
	if (!Added.WasSuccessful()) return Added;
	*this = MoveTemp(Candidate);
	return Added;
}

bool FCampaignArmyState::TryGetArmy(FGuid ArmyID, FCampaignArmy& OutArmy) const
{
	OutArmy = FCampaignArmy();
	const int32 Index = FindArmy(ArmyID);
	if (Index == INDEX_NONE) return false;
	OutArmy = Armies[Index];
	return true;
}

FCampaignArmyMutationResult FCampaignArmyState::ReplaceFromSnapshot(const TArray<FCampaignArmy>& Snapshot)
{
	FCampaignArmyState Candidate;
	for (const FCampaignArmy& Army : Snapshot)
	{
		const FCampaignArmyMutationResult Added = Candidate.RegisterArmy(Army);
		if (!Added.WasSuccessful()) return Added;
	}
	Armies = MoveTemp(Candidate.Armies);
	return Result(FGuid(), ECampaignArmyFailure::None);
}
