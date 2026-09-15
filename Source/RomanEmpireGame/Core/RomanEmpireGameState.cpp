// Copyright Roman Empire Game. All Rights Reserved.

#include "Core/RomanEmpireGameState.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/World.h"
#include "Faction/FactionManager.h"
#include "RomanEmpireGame.h"
#include "World/CampaignManager.h"
#include "World/WorldMapManager.h"

ARomanEmpireGameState::ARomanEmpireGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARomanEmpireGameState::BeginPlay()
{
	Super::BeginPlay();
	Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	if (Rules.IsValid())
	{
		Rules->OnGameplayInitialized.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleGameplayInitialized);
		Rules->OnGameplayModeChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleGameplayModeChanged);
		// Works both before and after GameMode's normal manager initialization.
		HandleGameplayInitialized();
	}
}

void ARomanEmpireGameState::HandleGameplayInitialized()
{
	if (bObserversBound || !Rules.IsValid() || !Rules->IsGameplayInitialized()) return;
	Factions = Rules->GetFactionManager();
	Campaign = Rules->GetCampaignManager();
	WorldMap = Rules->GetWorldMapManager();
	if (!Factions.IsValid() || !Campaign.IsValid() || !WorldMap.IsValid()) return;
	Factions->OnFactionResourcesChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleResourcesChanged);
	Factions->OnPlayerFactionChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandlePlayerFactionChanged);
	Campaign->OnTurnStepStarted.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleTurnStepStarted);
	Campaign->OnTurnProcessed.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleTurnProcessed);
	Campaign->OnCampaignStarted.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleCampaignStarted);
	Campaign->OnCampaignObjectivesChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleObjectivesChanged);
	Campaign->OnVictory.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleVictory);
	Campaign->OnDefeat.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleDefeat);
	bObserversBound = true;
	UE_LOG(LogRomanEmpire, Log, TEXT("Public GameState ready; canonical gameplay reads and event relays bound"));
	OnGameplayInitialized.Broadcast();
}

void ARomanEmpireGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Rules.IsValid())
	{
		Rules->OnGameplayInitialized.RemoveDynamic(this, &ARomanEmpireGameState::HandleGameplayInitialized);
		Rules->OnGameplayModeChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleGameplayModeChanged);
	}
	if (Factions.IsValid())
	{
		Factions->OnFactionResourcesChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleResourcesChanged);
		Factions->OnPlayerFactionChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandlePlayerFactionChanged);
	}
	if (Campaign.IsValid())
	{
		Campaign->OnTurnStepStarted.RemoveDynamic(this, &ARomanEmpireGameState::HandleTurnStepStarted);
		Campaign->OnTurnProcessed.RemoveDynamic(this, &ARomanEmpireGameState::HandleTurnProcessed);
		Campaign->OnCampaignStarted.RemoveDynamic(this, &ARomanEmpireGameState::HandleCampaignStarted);
		Campaign->OnCampaignObjectivesChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleObjectivesChanged);
		Campaign->OnVictory.RemoveDynamic(this, &ARomanEmpireGameState::HandleVictory);
		Campaign->OnDefeat.RemoveDynamic(this, &ARomanEmpireGameState::HandleDefeat);
	}
	bObserversBound = false;
	Rules.Reset();
	Factions.Reset();
	Campaign.Reset();
	WorldMap.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ARomanEmpireGameState::IsGameplayInitialized() const
{
	return bObserversBound && Rules.IsValid() && Rules->IsGameplayInitialized() &&
		Factions.IsValid() && Campaign.IsValid() && WorldMap.IsValid();
}

EGameplayMode ARomanEmpireGameState::GetCurrentGameplayMode() const
{
	return Rules.IsValid() ? Rules->GetCurrentGameplayMode() : EGameplayMode::Tactical;
}

int32 ARomanEmpireGameState::GetCurrentTurn() const
{
	return Campaign.IsValid() ? Campaign->GetCurrentTurn() : 0;
}

EFactionID ARomanEmpireGameState::GetPlayerFaction() const
{
	return Factions.IsValid() ? Factions->GetPlayerFaction() : EFactionID::None;
}

FFactionResourceAmounts ARomanEmpireGameState::GetFactionResources(EFactionID FactionID) const
{
	return Factions.IsValid() ? Factions->GetFactionResources(FactionID) : FFactionResourceAmounts();
}

FCampaignObjectiveState ARomanEmpireGameState::GetObjectiveState() const
{
	return Campaign.IsValid() ? Campaign->GetObjectiveState() : FCampaignObjectiveState();
}

bool ARomanEmpireGameState::IsCampaignActive() const
{
	return IsGameplayInitialized() && Campaign->IsCampaignActive();
}

int32 ARomanEmpireGameState::GetFactionTerritoryCount(EFactionID FactionID) const
{
	return WorldMap.IsValid() ? WorldMap->GetFactionTerritoryCount(FactionID) : 0;
}

EVictoryCondition ARomanEmpireGameState::GetResolvedVictoryCondition() const
{
	return Campaign.IsValid() ? Campaign->GetResolvedVictoryCondition() : EVictoryCondition::Conquest;
}

void ARomanEmpireGameState::HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode)
{
	OnGameplayModeChanged.Broadcast(OldMode, NewMode);
}

void ARomanEmpireGameState::HandleResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance)
{
	(void)NewBalance;
	// A preceding observer can commit another transaction synchronously. Publish
	// the current authoritative balance, never revive an older notification payload.
	OnFactionResourcesChanged.Broadcast(FactionID, GetFactionResources(FactionID));
}

void ARomanEmpireGameState::HandlePlayerFactionChanged(EFactionID NewFaction)
{
	OnPlayerFactionChanged.Broadcast(NewFaction);
}

void ARomanEmpireGameState::HandleTurnStepStarted(int32 TurnNumber, ECampaignTurnStep Step)
{
	OnTurnStepStarted.Broadcast(TurnNumber, Step);
}

void ARomanEmpireGameState::HandleTurnProcessed(int32 TurnNumber)
{
	OnTurnProcessed.Broadcast(TurnNumber);
}

void ARomanEmpireGameState::HandleCampaignStarted(EFactionID PlayerFaction)
{
	OnCampaignStarted.Broadcast(PlayerFaction);
}

void ARomanEmpireGameState::HandleObjectivesChanged(FCampaignObjectiveState ObjectiveState)
{
	OnCampaignObjectivesChanged.Broadcast(ObjectiveState);
}

void ARomanEmpireGameState::HandleVictory(EFactionID WinningFaction, EVictoryCondition Condition)
{
	OnVictory.Broadcast(WinningFaction, Condition);
}

void ARomanEmpireGameState::HandleDefeat(EFactionID LosingFaction)
{
	OnDefeat.Broadcast(LosingFaction);
}
