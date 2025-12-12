// Copyright Roman Empire Game. All Rights Reserved.

#include "CampaignManager.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/Faction/FactionManager.h"
#include "RomanEmpireGame/World/WorldMapManager.h"
#include "RomanEmpireGame/World/TerritoryRegion.h"
#include "Kismet/GameplayStatics.h"

ACampaignManager::ACampaignManager()
{
	PrimaryActorTick.bCanEverTick = false;

	CurrentTurn = 1;
	bCampaignActive = false;
	MaxTurns = 100;
	ConquestVictoryTerritories = 7; // Control 7 of 9 territories
	EconomicVictoryGold = 10000;

	FactionManager = nullptr;
	WorldMapManager = nullptr;
}

void ACampaignManager::BeginPlay()
{
	Super::BeginPlay();

	// Find managers
	TArray<AActor*> FoundActors;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFactionManager::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		FactionManager = Cast<AFactionManager>(FoundActors[0]);
	}
	
	FoundActors.Empty();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldMapManager::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		WorldMapManager = Cast<AWorldMapManager>(FoundActors[0]);
	}

	// Start campaign automatically for prototype
	if (FactionManager)
	{
		StartNewCampaign(FactionManager->GetPlayerFaction());
	}
}

void ACampaignManager::StartNewCampaign(EFactionID PlayerFaction)
{
	CurrentTurn = 1;
	bCampaignActive = true;

	if (FactionManager)
	{
		FactionManager->SetPlayerFaction(PlayerFaction);
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("New campaign started with faction %d"), 
		static_cast<int32>(PlayerFaction));
}

void ACampaignManager::ProcessTurn()
{
	if (!bCampaignActive)
	{
		return;
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Processing turn %d"), CurrentTurn);

	// 1. Process resource production for all territories
	ProcessResourceProduction();

	// 2. Process AI faction actions
	ProcessAIFactions();

	// 3. Check victory/defeat conditions
	CheckAllVictoryConditions();

	// 4. Advance turn
	CurrentTurn++;
	OnTurnProcessed.Broadcast(CurrentTurn);

	// Check max turns
	if (CurrentTurn > MaxTurns)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("Max turns reached, campaign ending"));
		bCampaignActive = false;
	}
}

void ACampaignManager::ProcessResourceProduction()
{
	if (!WorldMapManager || !FactionManager)
	{
		return;
	}

	// For each territory, add resources to owning faction
	for (ATerritoryRegion* Territory : WorldMapManager->GetAllTerritories())
	{
		if (!Territory)
		{
			continue;
		}

		EFactionID Owner = Territory->GetOwnerFaction();
		if (Owner == EFactionID::None)
		{
			continue;
		}

		FFactionResources Production = Territory->GetResourceProduction();
		FactionManager->ModifyFactionResources(Owner, Production);
	}
}

void ACampaignManager::ProcessAIFactions()
{
	if (FactionManager)
	{
		FactionManager->ProcessAITurns();
	}
}

void ACampaignManager::CheckAllVictoryConditions()
{
	if (!FactionManager)
	{
		return;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();

	// Check conquest victory
	if (CheckVictoryCondition(PlayerFaction, EVictoryCondition::Conquest))
	{
		OnVictory.Broadcast(PlayerFaction, EVictoryCondition::Conquest);
		bCampaignActive = false;
		UE_LOG(LogRomanEmpire, Log, TEXT("Player achieved conquest victory!"));
	}

	// Check economic victory
	if (CheckVictoryCondition(PlayerFaction, EVictoryCondition::Economic))
	{
		OnVictory.Broadcast(PlayerFaction, EVictoryCondition::Economic);
		bCampaignActive = false;
		UE_LOG(LogRomanEmpire, Log, TEXT("Player achieved economic victory!"));
	}

	// Check if player lost all territories
	if (HasPlayerLost())
	{
		OnDefeat.Broadcast(PlayerFaction);
		bCampaignActive = false;
		UE_LOG(LogRomanEmpire, Log, TEXT("Player has been defeated!"));
	}
}

bool ACampaignManager::CheckVictoryCondition(EFactionID FactionID, EVictoryCondition Condition) const
{
	if (!FactionManager || !WorldMapManager)
	{
		return false;
	}

	switch (Condition)
	{
		case EVictoryCondition::Conquest:
		{
			int32 TerritoryCount = FactionManager->GetFactionTerritoryCount(FactionID);
			return TerritoryCount >= ConquestVictoryTerritories;
		}

		case EVictoryCondition::Economic:
		{
			FFactionResources Resources = FactionManager->GetFactionResources(FactionID);
			return Resources.Gold >= EconomicVictoryGold;
		}

		case EVictoryCondition::Military:
		{
			// TODO: Check if faction destroyed all enemy armies
			return false;
		}

		case EVictoryCondition::Diplomatic:
		{
			// TODO: Check allied victory
			return false;
		}

		default:
			return false;
	}
}

bool ACampaignManager::HasPlayerWon() const
{
	if (!FactionManager)
	{
		return false;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	
	return CheckVictoryCondition(PlayerFaction, EVictoryCondition::Conquest) ||
		   CheckVictoryCondition(PlayerFaction, EVictoryCondition::Economic);
}

bool ACampaignManager::HasPlayerLost() const
{
	if (!FactionManager)
	{
		return false;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	int32 TerritoryCount = FactionManager->GetFactionTerritoryCount(PlayerFaction);
	
	// Lost if no territories
	return TerritoryCount == 0;
}

void ACampaignManager::SaveCampaign(const FString& SaveName)
{
	// TODO: Implement save system using USaveGame
	UE_LOG(LogRomanEmpire, Log, TEXT("Saving campaign: %s"), *SaveName);
}

void ACampaignManager::LoadCampaign(const FString& SaveName)
{
	// TODO: Implement load system
	UE_LOG(LogRomanEmpire, Log, TEXT("Loading campaign: %s"), *SaveName);
}
