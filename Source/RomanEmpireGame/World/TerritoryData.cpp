// Copyright Roman Empire Game. All Rights Reserved.

#include "Campaign/TerritoryData.h"

bool FTerritoryDefinition::IsValid() const
{
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
	default: return TEXT("Unknown");
	}
}
