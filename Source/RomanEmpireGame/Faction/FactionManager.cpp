// Copyright Roman Empire Game. All Rights Reserved.

#include "FactionManager.h"
#include "RomanEmpireGame/RomanEmpireGame.h"

AFactionManager::AFactionManager()
{
	PrimaryActorTick.bCanEverTick = false;
	
	PlayerFaction = EFactionID::Rome; // Default player faction
}

void AFactionManager::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeDefaultFactions();
	InitializeDefaultDiplomacy();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Faction Manager initialized with %d factions"), FactionInfoMap.Num());
}

void AFactionManager::InitializeDefaultFactions()
{
	// Rome
	FFactionInfo Rome;
	Rome.FactionID = EFactionID::Rome;
	Rome.DisplayName = FText::FromString(TEXT("Roman Republic"));
	Rome.Description = FText::FromString(TEXT("The mighty Roman Republic, known for disciplined legions."));
	Rome.FactionColor = FLinearColor(0.8f, 0.1f, 0.1f, 1.0f); // Red
	Rome.InfantryBonus = 1.2f;
	Rome.CavalryBonus = 1.0f;
	Rome.EconomyBonus = 1.1f;
	Rome.UniqueUnitTypes.Add(TEXT("Legionary"));
	Rome.UniqueUnitTypes.Add(TEXT("Praetorian"));
	Rome.UniqueBuildingTypes.Add(TEXT("Colosseum"));
	FactionInfoMap.Add(EFactionID::Rome, Rome);

	// Carthage
	FFactionInfo Carthage;
	Carthage.FactionID = EFactionID::Carthage;
	Carthage.DisplayName = FText::FromString(TEXT("Carthage"));
	Carthage.Description = FText::FromString(TEXT("The great trading empire of the Mediterranean."));
	Carthage.FactionColor = FLinearColor(0.5f, 0.0f, 0.5f, 1.0f); // Purple
	Carthage.InfantryBonus = 0.9f;
	Carthage.CavalryBonus = 1.1f;
	Carthage.EconomyBonus = 1.3f;
	Carthage.UniqueUnitTypes.Add(TEXT("WarElephant"));
	Carthage.UniqueUnitTypes.Add(TEXT("SacredBand"));
	Carthage.UniqueBuildingTypes.Add(TEXT("TradePort"));
	FactionInfoMap.Add(EFactionID::Carthage, Carthage);

	// Gaul
	FFactionInfo Gaul;
	Gaul.FactionID = EFactionID::Gaul;
	Gaul.DisplayName = FText::FromString(TEXT("Gallic Tribes"));
	Gaul.Description = FText::FromString(TEXT("Fierce Celtic warriors from the northern forests."));
	Gaul.FactionColor = FLinearColor(0.1f, 0.6f, 0.1f, 1.0f); // Green
	Gaul.InfantryBonus = 1.1f;
	Gaul.CavalryBonus = 1.2f;
	Gaul.EconomyBonus = 0.9f;
	Gaul.UniqueUnitTypes.Add(TEXT("NakedFanatic"));
	Gaul.UniqueUnitTypes.Add(TEXT("NobleCavalry"));
	Gaul.UniqueBuildingTypes.Add(TEXT("SacredGrove"));
	FactionInfoMap.Add(EFactionID::Gaul, Gaul);

	// Initialize resources for each faction
	for (const auto& Pair : FactionInfoMap)
	{
		FFactionResources StartingResources;
		StartingResources.Gold = 1000;
		StartingResources.Food = 500;
		StartingResources.Iron = 200;
		StartingResources.Wood = 300;
		StartingResources.Stone = 200;
		StartingResources.Population = 100;
		
		// Apply economy bonus
		StartingResources.Gold = FMath::RoundToInt(StartingResources.Gold * Pair.Value.EconomyBonus);
		
		FactionResourcesMap.Add(Pair.Key, StartingResources);
	}
}

void AFactionManager::InitializeDefaultDiplomacy()
{
	// Set initial diplomatic relations
	// Rome vs Carthage - War (Punic Wars)
	FDiplomaticRelation RomeCarthage;
	RomeCarthage.OtherFaction = EFactionID::Carthage;
	RomeCarthage.Status = EDiplomaticStatus::War;
	RomeCarthage.RelationshipScore = -80;
	DiplomacyMatrix.Add(RomeCarthage);

	// Rome vs Gaul - Hostile
	FDiplomaticRelation RomeGaul;
	RomeGaul.OtherFaction = EFactionID::Gaul;
	RomeGaul.Status = EDiplomaticStatus::Hostile;
	RomeGaul.RelationshipScore = -40;
	DiplomacyMatrix.Add(RomeGaul);

	// Carthage vs Gaul - Neutral
	FDiplomaticRelation CarthageGaul;
	CarthageGaul.OtherFaction = EFactionID::Gaul;
	CarthageGaul.Status = EDiplomaticStatus::Neutral;
	CarthageGaul.RelationshipScore = 0;
	DiplomacyMatrix.Add(CarthageGaul);
}

FFactionInfo AFactionManager::GetFactionInfo(EFactionID FactionID) const
{
	if (const FFactionInfo* Info = FactionInfoMap.Find(FactionID))
	{
		return *Info;
	}
	return FFactionInfo();
}

FFactionResources AFactionManager::GetFactionResources(EFactionID FactionID) const
{
	if (const FFactionResources* Resources = FactionResourcesMap.Find(FactionID))
	{
		return *Resources;
	}
	return FFactionResources();
}

void AFactionManager::ModifyFactionResources(EFactionID FactionID, const FFactionResources& Delta)
{
	if (FFactionResources* Resources = FactionResourcesMap.Find(FactionID))
	{
		Resources->Gold += Delta.Gold;
		Resources->Food += Delta.Food;
		Resources->Iron += Delta.Iron;
		Resources->Wood += Delta.Wood;
		Resources->Stone += Delta.Stone;
		Resources->Population += Delta.Population;
	}
}

bool AFactionManager::CanFactionAfford(EFactionID FactionID, const FFactionResources& Cost) const
{
	if (const FFactionResources* Resources = FactionResourcesMap.Find(FactionID))
	{
		return Resources->CanAfford(Cost);
	}
	return false;
}

bool AFactionManager::DeductFactionResources(EFactionID FactionID, const FFactionResources& Cost)
{
	if (FFactionResources* Resources = FactionResourcesMap.Find(FactionID))
	{
		if (Resources->CanAfford(Cost))
		{
			Resources->Deduct(Cost);
			return true;
		}
	}
	return false;
}

EDiplomaticStatus AFactionManager::GetDiplomaticStatus(EFactionID Faction1, EFactionID Faction2) const
{
	if (Faction1 == Faction2)
	{
		return EDiplomaticStatus::Allied; // Same faction
	}

	for (const FDiplomaticRelation& Relation : DiplomacyMatrix)
	{
		if (Relation.OtherFaction == Faction2)
		{
			return Relation.Status;
		}
	}
	return EDiplomaticStatus::Neutral;
}

void AFactionManager::SetDiplomaticStatus(EFactionID Faction1, EFactionID Faction2, EDiplomaticStatus NewStatus)
{
	for (FDiplomaticRelation& Relation : DiplomacyMatrix)
	{
		if (Relation.OtherFaction == Faction2)
		{
			Relation.Status = NewStatus;
			UE_LOG(LogRomanEmpire, Log, TEXT("Diplomatic status changed between factions"));
			return;
		}
	}

	// Add new relation if not found
	FDiplomaticRelation NewRelation;
	NewRelation.OtherFaction = Faction2;
	NewRelation.Status = NewStatus;
	NewRelation.RelationshipScore = 0;
	DiplomacyMatrix.Add(NewRelation);
}

bool AFactionManager::AreAtWar(EFactionID Faction1, EFactionID Faction2) const
{
	return GetDiplomaticStatus(Faction1, Faction2) == EDiplomaticStatus::War;
}

bool AFactionManager::AreAllied(EFactionID Faction1, EFactionID Faction2) const
{
	return GetDiplomaticStatus(Faction1, Faction2) == EDiplomaticStatus::Allied;
}

void AFactionManager::SetPlayerFaction(EFactionID NewFaction)
{
	PlayerFaction = NewFaction;
	UE_LOG(LogRomanEmpire, Log, TEXT("Player faction set to: %d"), static_cast<int32>(NewFaction));
}

void AFactionManager::AssignTerritoryToFaction(FName TerritoryID, EFactionID FactionID)
{
	TerritoryOwnership.Add(TerritoryID, FactionID);
}

int32 AFactionManager::GetFactionTerritoryCount(EFactionID FactionID) const
{
	int32 Count = 0;
	for (const auto& Pair : TerritoryOwnership)
	{
		if (Pair.Value == FactionID)
		{
			Count++;
		}
	}
	return Count;
}

void AFactionManager::ProcessAITurns()
{
	for (const auto& Pair : FactionInfoMap)
	{
		if (Pair.Key != PlayerFaction)
		{
			ProcessAIFactionTurn(Pair.Key);
		}
	}
}

void AFactionManager::ProcessAIFactionTurn(EFactionID FactionID)
{
	// Basic AI: Generate income based on territory
	int32 TerritoryCount = GetFactionTerritoryCount(FactionID);
	
	FFactionResources Income;
	Income.Gold = 100 * TerritoryCount;
	Income.Food = 50 * TerritoryCount;
	
	ModifyFactionResources(FactionID, Income);
	
	UE_LOG(LogRomanEmpire, Verbose, TEXT("AI faction %d processed turn, gained %d gold"), 
		static_cast<int32>(FactionID), Income.Gold);
}
