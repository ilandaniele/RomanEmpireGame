// Copyright Roman Empire Game. All Rights Reserved.

#include "FactionManager.h"
#include "RomanEmpireGame.h"

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
	Rome.StartingResources = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::Rome);
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
	Carthage.StartingResources = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::Carthage);
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
	Gaul.StartingResources = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::Gaul);
	Gaul.UniqueUnitTypes.Add(TEXT("NakedFanatic"));
	Gaul.UniqueUnitTypes.Add(TEXT("NobleCavalry"));
	Gaul.UniqueBuildingTypes.Add(TEXT("SacredGrove"));
	FactionInfoMap.Add(EFactionID::Gaul, Gaul);

	// Initialize resources for each faction
	for (const auto& Pair : FactionInfoMap)
	{
		FactionResourcesMap.Add(Pair.Key, Pair.Value.StartingResources);
	}
}

void AFactionManager::InitializeDefaultDiplomacy()
{
	DiplomacyState.Reset();
	DiplomacyState.TrySetStatus(EFactionID::Rome, EFactionID::Carthage, EDiplomaticStatus::War);
	DiplomacyState.TrySetStatus(EFactionID::Rome, EFactionID::Gaul, EDiplomaticStatus::War);
	// Carthage and Gaul remain in the implicit canonical Neutral state.
}

FFactionInfo AFactionManager::GetFactionInfo(EFactionID FactionID) const
{
	if (const FFactionInfo* Info = FactionInfoMap.Find(FactionID))
	{
		return *Info;
	}
	return FFactionInfo();
}

FFactionResourceAmounts AFactionManager::GetFactionResources(EFactionID FactionID) const
{
	if (const FFactionResourceAmounts* Resources = FactionResourcesMap.Find(FactionID))
	{
		return *Resources;
	}
	return FFactionResourceAmounts();
}

FFactionResourceTransactionResult AFactionManager::ApplyFactionResourceDelta(
	EFactionID FactionID,
	const FFactionResourceDelta& Delta)
{
	if (FFactionResourceAmounts* Resources = FactionResourcesMap.Find(FactionID))
	{
		FFactionResourceTransactionResult Result = RomanEmpireEconomy::TryApplyResourceDelta(*Resources, Delta);
		if (Result.WasSuccessful() && !Delta.IsZero())
		{
			OnFactionResourcesChanged.Broadcast(FactionID, *Resources);
		}
		return Result;
	}

	FFactionResourceTransactionResult Result;
	Result.Failure = EResourceTransactionFailure::InvalidFaction;
	return Result;
}

FFactionResourceTransactionResult AFactionManager::TrySpendFactionResources(
	EFactionID FactionID,
	const FFactionResourceAmounts& Cost)
{
	if (FFactionResourceAmounts* Resources = FactionResourcesMap.Find(FactionID))
	{
		FFactionResourceTransactionResult Result = RomanEmpireEconomy::TrySpendResources(*Resources, Cost);
		if (Result.WasSuccessful() && !Cost.IsZero())
		{
			OnFactionResourcesChanged.Broadcast(FactionID, *Resources);
		}
		return Result;
	}

	FFactionResourceTransactionResult Result;
	Result.Failure = EResourceTransactionFailure::InvalidFaction;
	return Result;
}

EDiplomaticStatus AFactionManager::GetDiplomaticStatus(EFactionID Faction1, EFactionID Faction2) const
{
	return DiplomacyState.GetStatus(Faction1, Faction2);
}

FDiplomaticChangeResult AFactionManager::SetDiplomaticStatus(
	EFactionID Faction1,
	EFactionID Faction2,
	EDiplomaticStatus NewStatus)
{
	FDiplomaticChangeResult Result = DiplomacyState.TrySetStatus(Faction1, Faction2, NewStatus);
	if (Result.WasSuccessful())
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("Diplomatic status changed between factions %d and %d to %d"),
			static_cast<int32>(Result.Pair.FirstFaction),
			static_cast<int32>(Result.Pair.SecondFaction),
			static_cast<int32>(NewStatus));
		OnDiplomaticStatusChanged.Broadcast(Result.Pair, NewStatus);
	}
	return Result;
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
	// Campaign production is processed exactly once by ACampaignManager before this stage.
	UE_LOG(LogRomanEmpire, Verbose, TEXT("AI faction %d processed its action stage"),
		static_cast<int32>(FactionID));
}

