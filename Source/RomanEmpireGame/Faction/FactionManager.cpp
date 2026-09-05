// Copyright Roman Empire Game. All Rights Reserved.

#include "FactionManager.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"

AFactionManager::AFactionManager()
{
	PrimaryActorTick.bCanEverTick = false;
	
	PlayerFaction = EFactionID::Rome; // Default player faction
}

void AFactionManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (!InitializeFactionsFromContent())
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Faction Manager could not initialize because content definitions are invalid"));
		return;
	}
	InitializeDefaultDiplomacy();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Faction Manager initialized with %d factions"), FactionInfoMap.Num());
}

bool AFactionManager::InitializeFactionsFromContent()
{
	FactionInfoMap.Reset();
	FactionResourcesMap.Reset();
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!Registry || !Registry->IsReady())
	{
		return false;
	}

	for (const URomanFactionDefinition* Asset : Registry->GetAllFactions())
	{
		if (!Asset)
		{
			continue;
		}
		const FFactionInfo& Definition = Asset->Definition;
		FactionInfoMap.Add(Definition.RuntimeFaction, Definition);
		FactionResourcesMap.Add(Definition.RuntimeFaction, Definition.StartingResources);
	}

	return FactionInfoMap.Num() > 0;
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

