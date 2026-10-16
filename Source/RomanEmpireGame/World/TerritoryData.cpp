// Copyright Roman Empire Game. All Rights Reserved.

#include "Campaign/TerritoryData.h"

namespace
{
	bool CombineProduction(const FFactionResourceDelta& Base, const FFactionResourceDelta& Bonus,
		FFactionResourceDelta& Out)
	{
		Out = FFactionResourceDelta();
		const int64 Values[] = { int64(Base.Gold) + Bonus.Gold, int64(Base.Food) + Bonus.Food,
			int64(Base.Iron) + Bonus.Iron, int64(Base.Wood) + Bonus.Wood,
			int64(Base.Stone) + Bonus.Stone, int64(Base.Population) + Bonus.Population };
		for (int64 Value : Values) if (Value < 0 || Value > MAX_int32) return false;
		Out.Gold = int32(Values[0]); Out.Food = int32(Values[1]); Out.Iron = int32(Values[2]);
		Out.Wood = int32(Values[3]); Out.Stone = int32(Values[4]); Out.Population = int32(Values[5]);
		return true;
	}
}

bool FTerritoryDefinition::IsValid() const
{
	TSet<FName> Neighbors;
	for (FName Neighbor : AdjacentTerritoryIDs)
	{
		if (Neighbor.IsNone() || Neighbor == TerritoryID || Neighbors.Contains(Neighbor)) return false;
		Neighbors.Add(Neighbor);
	}
	const bool bProductionIsNonNegative =
		ResourceProduction.Gold >= 0 &&
		ResourceProduction.Food >= 0 &&
		ResourceProduction.Iron >= 0 &&
		ResourceProduction.Wood >= 0 &&
		ResourceProduction.Stone >= 0 &&
		ResourceProduction.Population >= 0;

	return TerritoryID != NAME_None &&
		!DisplayName.IsEmpty() &&
		static_cast<uint8>(TerrainType) <= static_cast<uint8>(ETerrainType::Coast) &&
		static_cast<uint8>(BonusResource) <= static_cast<uint8>(EResourceType::Population) &&
		RomanEmpireTerritories::IsValidFactionOrUnowned(StartingOwner) &&
		bProductionIsNonNegative;
}

FTerritoryRegistrationResult FTerritoryCampaignState::RegisterTerritory(
	const FTerritoryDefinition& Definition)
{
	FTerritoryRegistrationResult Result;
	Result.TerritoryID = Definition.TerritoryID;

	if (!Definition.IsValid())
	{
		return Result;
	}

	if (TerritoryDefinitions.Contains(Definition.TerritoryID))
	{
		Result.Failure = ETerritoryRegistrationFailure::DuplicateID;
		return Result;
	}

	TerritoryDefinitions.Add(Definition.TerritoryID, Definition);
	TerritoryOwnership.Add(Definition.TerritoryID, Definition.StartingOwner);
	TerritorySettlements.Add(Definition.TerritoryID, FTerritorySettlementState());
	Result.Failure = ETerritoryRegistrationFailure::None;
	return Result;
}

FTerritoryOwnershipChangeResult FTerritoryCampaignState::ChangeOwner(
	FName TerritoryID,
	EFactionID NewOwner)
{
	FTerritoryOwnershipChangeResult Result;
	Result.TerritoryID = TerritoryID;
	Result.NewOwner = NewOwner;

	if (!RomanEmpireTerritories::IsValidFactionOrUnowned(NewOwner))
	{
		Result.Failure = ETerritoryOwnershipFailure::InvalidOwner;
		return Result;
	}

	EFactionID* CurrentOwner = TerritoryOwnership.Find(TerritoryID);
	if (!CurrentOwner)
	{
		return Result;
	}

	Result.PreviousOwner = *CurrentOwner;
	Result.Failure = ETerritoryOwnershipFailure::None;
	if (*CurrentOwner != NewOwner)
	{
		*CurrentOwner = NewOwner;
		Result.bOwnershipChanged = true;
	}

	return Result;
}

bool FTerritoryCampaignState::TryGetDefinition(
	FName TerritoryID,
	FTerritoryDefinition& OutDefinition) const
{
	if (const FTerritoryDefinition* Definition = TerritoryDefinitions.Find(TerritoryID))
	{
		OutDefinition = *Definition;
		return true;
	}
	return false;
}

bool FTerritoryCampaignState::TryGetOwner(FName TerritoryID, EFactionID& OutOwner) const
{
	if (const EFactionID* Owner = TerritoryOwnership.Find(TerritoryID))
	{
		OutOwner = *Owner;
		return true;
	}
	return false;
}

int32 FTerritoryCampaignState::GetFactionTerritoryCount(EFactionID FactionID) const
{
	int32 Count = 0;
	for (const TPair<FName, EFactionID>& Pair : TerritoryOwnership)
	{
		if (Pair.Value == FactionID)
		{
			++Count;
		}
	}
	return Count;
}

void FTerritoryCampaignState::Reset()
{
	TerritoryDefinitions.Reset();
	TerritoryOwnership.Reset();
	TerritorySettlements.Reset();
}

FSettlementFoundingResult FTerritoryCampaignState::FoundSettlement(FName TerritoryID, const FText& Name)
{
	FSettlementFoundingResult Result;
	Result.TerritoryID = TerritoryID;
	FTerritorySettlementState* Current = TerritorySettlements.Find(TerritoryID);
	const FTerritoryDefinition* Definition = TerritoryDefinitions.Find(TerritoryID);
	if (!Current || !Definition) return Result;
	if (Name.ToString().TrimStartAndEnd().IsEmpty())
	{
		Result.Failure = ESettlementFoundingFailure::InvalidName;
		return Result;
	}
	if (Current->bHasSettlement)
	{
		Result.Failure = ESettlementFoundingFailure::AlreadyFounded;
		return Result;
	}
	FTerritorySettlementState Candidate;
	Candidate.bHasSettlement = true;
	Candidate.SettlementName = Name;
	Candidate.Population = 100;
	Candidate.ProductionBonus.Gold = 100;
	Candidate.ProductionBonus.Food = 50;
	FFactionResourceDelta Production;
	if (!CombineProduction(Definition->ResourceProduction, Candidate.ProductionBonus, Production))
	{
		Result.Failure = ESettlementFoundingFailure::ProductionOverflow;
		return Result;
	}
	*Current = MoveTemp(Candidate);
	Result.Failure = ESettlementFoundingFailure::None;
	return Result;
}

bool FTerritoryCampaignState::TryGetSettlement(FName TerritoryID, FTerritorySettlementState& OutSettlement) const
{
	OutSettlement = FTerritorySettlementState();
	const FTerritorySettlementState* Settlement = TerritorySettlements.Find(TerritoryID);
	if (!Settlement) return false;
	OutSettlement = *Settlement;
	return true;
}

bool FTerritoryCampaignState::TryGetResourceProduction(FName TerritoryID, FFactionResourceDelta& OutProduction) const
{
	OutProduction = FFactionResourceDelta();
	const FTerritoryDefinition* Definition = TerritoryDefinitions.Find(TerritoryID);
	const FTerritorySettlementState* Settlement = TerritorySettlements.Find(TerritoryID);
	return Definition && Settlement && CombineProduction(Definition->ResourceProduction, Settlement->ProductionBonus, OutProduction);
}

bool FTerritoryCampaignState::HasValidAdjacency() const
{
	for (const TPair<FName, FTerritoryDefinition>& Pair : TerritoryDefinitions)
	{
		for (FName NeighborID : Pair.Value.AdjacentTerritoryIDs)
		{
			const FTerritoryDefinition* Neighbor = TerritoryDefinitions.Find(NeighborID);
			if (!Neighbor || !Neighbor->AdjacentTerritoryIDs.Contains(Pair.Key)) return false;
		}
	}
	return true;
}

bool FTerritoryCampaignState::AreAdjacent(FName FirstID, FName SecondID) const
{
	if (FirstID == SecondID || !HasValidAdjacency()) return false;
	const FTerritoryDefinition* First = TerritoryDefinitions.Find(FirstID);
	return First && First->AdjacentTerritoryIDs.Contains(SecondID);
}

TArray<FName> FTerritoryCampaignState::GetAdjacentIDs(FName TerritoryID) const
{
	if (!HasValidAdjacency()) return {};
	const FTerritoryDefinition* Definition = TerritoryDefinitions.Find(TerritoryID);
	TArray<FName> Result = Definition ? Definition->AdjacentTerritoryIDs : TArray<FName>();
	Result.Sort(FNameLexicalLess());
	return Result;
}

TArray<FName> FTerritoryCampaignState::GetTerritoryIDs() const
{
	TArray<FName> Result;
	TerritoryDefinitions.GetKeys(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

TArray<FTerritoryRuntimeSnapshot> FTerritoryCampaignState::GetRuntimeSnapshot() const
{
	TArray<FTerritoryRuntimeSnapshot> Result;
	for (FName ID : GetTerritoryIDs())
	{
		FTerritoryRuntimeSnapshot Row; Row.TerritoryID = ID;
		FTerritorySettlementState Current;
		TryGetOwner(ID, Row.Owner); TryGetSettlement(ID, Current);
		Row.Settlement.bHasSettlement = Current.bHasSettlement;
		Row.Settlement.SettlementName = Current.SettlementName.ToString();
		Row.Settlement.Population = Current.Population;
		Row.Settlement.ProductionBonus = Current.ProductionBonus;
		Result.Add(MoveTemp(Row));
	}
	return Result;
}

bool FTerritoryCampaignState::ReplaceRuntimeSnapshot(const TArray<FTerritoryRuntimeSnapshot>& Snapshot)
{
	if (Snapshot.Num() != TerritoryDefinitions.Num() || !HasValidAdjacency()) return false;
	TMap<FName, EFactionID> Owners;
	TMap<FName, FTerritorySettlementState> Settlements;
	for (const FTerritoryRuntimeSnapshot& Row : Snapshot)
	{
		const FTerritoryDefinition* Definition = TerritoryDefinitions.Find(Row.TerritoryID);
		const FTerritorySettlementSnapshot& Settlement = Row.Settlement;
		if (!Definition || Owners.Contains(Row.TerritoryID) || !RomanEmpireTerritories::IsValidFactionOrUnowned(Row.Owner)) return false;
		const FFactionResourceDelta& Bonus = Settlement.ProductionBonus;
		if (Bonus.Gold < 0 || Bonus.Food < 0 || Bonus.Iron < 0 || Bonus.Wood < 0 || Bonus.Stone < 0 || Bonus.Population < 0) return false;
		if (Settlement.bHasSettlement)
		{
			if (Settlement.SettlementName.TrimStartAndEnd().IsEmpty() || Settlement.Population <= 0) return false;
		}
		else if (!Settlement.SettlementName.IsEmpty() || Settlement.Population != 0 || !Bonus.IsZero()) return false;
		FFactionResourceDelta Production;
		if (!CombineProduction(Definition->ResourceProduction, Bonus, Production)) return false;
		FTerritorySettlementState Live;
		Live.bHasSettlement = Settlement.bHasSettlement;
		Live.SettlementName = FText::FromString(Settlement.SettlementName);
		Live.Population = Settlement.Population; Live.ProductionBonus = Bonus;
		Owners.Add(Row.TerritoryID, Row.Owner); Settlements.Add(Row.TerritoryID, MoveTemp(Live));
	}
	TerritoryOwnership = MoveTemp(Owners); TerritorySettlements = MoveTemp(Settlements);
	return true;
}

bool RomanEmpireTerritories::IsValidFactionOrUnowned(EFactionID FactionID)
{
	return static_cast<uint8>(FactionID) <= static_cast<uint8>(EFactionID::Britannia);
}

const TCHAR* RomanEmpireTerritories::GetRegistrationFailureName(
	ETerritoryRegistrationFailure Failure)
{
	switch (Failure)
	{
	case ETerritoryRegistrationFailure::None: return TEXT("None");
	case ETerritoryRegistrationFailure::InvalidDefinition: return TEXT("InvalidDefinition");
	case ETerritoryRegistrationFailure::DuplicateID: return TEXT("DuplicateID");
	default: return TEXT("Unknown");
	}
}

const TCHAR* RomanEmpireTerritories::GetOwnershipFailureName(ETerritoryOwnershipFailure Failure)
{
	switch (Failure)
	{
	case ETerritoryOwnershipFailure::None: return TEXT("None");
	case ETerritoryOwnershipFailure::UnknownTerritory: return TEXT("UnknownTerritory");
	case ETerritoryOwnershipFailure::InvalidOwner: return TEXT("InvalidOwner");
	case ETerritoryOwnershipFailure::TransactionInProgress: return TEXT("TransactionInProgress");
	default: return TEXT("Unknown");
	}
}
