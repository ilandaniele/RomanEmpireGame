// Copyright Roman Empire Game. All Rights Reserved.

#include "Core/RomanEmpireGameState.h"
#include "Core/RomanEmpireGameMode.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Content/RomanContentRegistry.h"
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
	TacticalBattle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (!Factions.IsValid() || !Campaign.IsValid() || !WorldMap.IsValid()) return;
	WorldMap->OnTerritoryOwnerChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleTerritoryOwnerChanged);
	WorldMap->OnTerritorySettlementChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleTerritorySettlementChanged);
	Factions->OnFactionResourcesChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleResourcesChanged);
	Factions->OnPlayerFactionChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandlePlayerFactionChanged);
	Factions->OnDiplomaticStatusChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleDiplomacyChanged);
	Campaign->OnTurnStepStarted.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleTurnStepStarted);
	Campaign->OnTurnProcessed.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleTurnProcessed);
	Campaign->OnCampaignStarted.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleCampaignStarted);
	Campaign->OnCampaignArmiesChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleCampaignArmiesChanged);
	Campaign->OnCampaignEncounterChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleCampaignEncounterChanged);
	Campaign->OnCampaignObjectivesChanged.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleObjectivesChanged);
	Campaign->OnVictory.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleVictory);
	Campaign->OnDefeat.AddUniqueDynamic(this, &ARomanEmpireGameState::HandleDefeat);
	if (TacticalBattle.IsValid())
	{
		TacticalBattle->OnTacticalBattleChanged.AddUniqueDynamic(
			this, &ARomanEmpireGameState::HandleTacticalBattleChanged);
	}
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
		Factions->OnDiplomaticStatusChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleDiplomacyChanged);
	}
	if (Campaign.IsValid())
	{
		Campaign->OnTurnStepStarted.RemoveDynamic(this, &ARomanEmpireGameState::HandleTurnStepStarted);
		Campaign->OnTurnProcessed.RemoveDynamic(this, &ARomanEmpireGameState::HandleTurnProcessed);
		Campaign->OnCampaignStarted.RemoveDynamic(this, &ARomanEmpireGameState::HandleCampaignStarted);
		Campaign->OnCampaignArmiesChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleCampaignArmiesChanged);
		Campaign->OnCampaignEncounterChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleCampaignEncounterChanged);
		Campaign->OnCampaignObjectivesChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleObjectivesChanged);
		Campaign->OnVictory.RemoveDynamic(this, &ARomanEmpireGameState::HandleVictory);
		Campaign->OnDefeat.RemoveDynamic(this, &ARomanEmpireGameState::HandleDefeat);
	}
	if (TacticalBattle.IsValid())
	{
		TacticalBattle->OnTacticalBattleChanged.RemoveDynamic(
			this, &ARomanEmpireGameState::HandleTacticalBattleChanged);
	}
	if (WorldMap.IsValid())
	{
		WorldMap->OnTerritoryOwnerChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleTerritoryOwnerChanged);
		WorldMap->OnTerritorySettlementChanged.RemoveDynamic(this, &ARomanEmpireGameState::HandleTerritorySettlementChanged);
	}
	bObserversBound = false;
	Rules.Reset();
	Factions.Reset();
	Campaign.Reset();
	WorldMap.Reset();
	TacticalBattle.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ARomanEmpireGameState::IsGameplayInitialized() const
{
	return bObserversBound && Rules.IsValid() && Rules->IsGameplayInitialized() &&
		Factions.IsValid() && Campaign.IsValid() && WorldMap.IsValid();
}

void ARomanEmpireGameState::HandleDiplomacyChanged(FDiplomaticPair Pair, EDiplomaticStatus Status)
{
	if (IsGameplayInitialized()) OnDiplomacyChanged.Broadcast(Pair, Factions->GetDiplomaticStatus(Pair.FirstFaction, Pair.SecondFaction));
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

FTacticalBattleResult ARomanEmpireGameState::GetTacticalBattleResult() const
{
	return TacticalBattle.IsValid() ? TacticalBattle->GetResult() : FTacticalBattleResult();
}

TArray<FCampaignArmy> ARomanEmpireGameState::GetCampaignArmies() const
{
	return IsGameplayInitialized() && Campaign->GetWorld() == GetWorld()
		? Campaign->GetCampaignArmies() : TArray<FCampaignArmy>();
}

bool ARomanEmpireGameState::TryGetCampaignArmy(FGuid ArmyID, FCampaignArmy& OutArmy) const
{
	OutArmy = FCampaignArmy();
	return IsGameplayInitialized() && Campaign->GetWorld() == GetWorld() && Campaign->TryGetCampaignArmy(ArmyID, OutArmy);
}

void ARomanEmpireGameState::HandleCampaignArmiesChanged()
{
	if (IsGameplayInitialized() && Campaign->GetWorld() == GetWorld()) OnCampaignArmiesChanged.Broadcast();
}

FCampaignEncounterPlan ARomanEmpireGameState::GetCampaignEncounterPlan() const
{
	return IsGameplayInitialized() && Campaign->GetWorld() == GetWorld()
		? Campaign->GetCampaignEncounterPlan() : FCampaignEncounterPlan();
}

bool ARomanEmpireGameState::HasAppliedCampaignEncounter(FGuid EncounterID) const
{
	return IsGameplayInitialized() && Campaign->GetWorld() == GetWorld() && Campaign->HasAppliedCampaignEncounter(EncounterID);
}

void ARomanEmpireGameState::HandleCampaignEncounterChanged()
{
	if (IsGameplayInitialized() && Campaign->GetWorld() == GetWorld()) OnCampaignEncounterChanged.Broadcast();
}

bool ARomanEmpireGameState::TryGetTerritorySnapshot(
	FName TerritoryID, FTerritoryPresentationSnapshot& OutSnapshot) const
{
	OutSnapshot = FTerritoryPresentationSnapshot();
	if (!IsGameplayInitialized() || WorldMap->GetWorld() != GetWorld() || Factions->GetWorld() != GetWorld()) return false;
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!Registry || !Registry->IsReady()) return false;
	const URomanTerritoryDefinition* Asset = Registry->FindTerritory(TerritoryID);
	const FTerritoryDefinition Definition = WorldMap->GetTerritoryDefinition(TerritoryID);
	if (!Asset || TerritoryID.IsNone() || Definition.TerritoryID != TerritoryID) return false;
	FTerritorySettlementState Settlement;
	FFactionResourceDelta EffectiveProduction;
	if (!WorldMap->TryGetTerritorySettlement(TerritoryID, Settlement) ||
		!WorldMap->TryGetTerritoryProduction(TerritoryID, EffectiveProduction)) return false;

	OutSnapshot.TerritoryID = TerritoryID;
	OutSnapshot.DisplayName = Definition.DisplayName;
	OutSnapshot.Description = Definition.Description;
	OutSnapshot.TerrainType = Definition.TerrainType;
	OutSnapshot.MapLocation = Asset->MapLocation;
	OutSnapshot.BaseResourceProduction = Definition.ResourceProduction;
	OutSnapshot.Settlement = MoveTemp(Settlement);
	OutSnapshot.EffectiveResourceProduction = EffectiveProduction;
	OutSnapshot.BonusResource = Definition.BonusResource;
	OutSnapshot.AdjacentTerritoryIDs = WorldMap->GetAdjacentTerritoryIDs(TerritoryID);
	OutSnapshot.OwnerFaction = WorldMap->GetTerritoryOwner(TerritoryID);
	OutSnapshot.bOwnerActive = Factions->IsFactionActive(OutSnapshot.OwnerFaction);
	if (const URomanFactionDefinition* OwnerAsset = Registry->FindFaction(OutSnapshot.OwnerFaction))
	{
		OutSnapshot.OwnerDisplayName = OwnerAsset->Definition.DisplayName;
		OutSnapshot.OwnerColor = OwnerAsset->Definition.FactionColor;
	}
	return true;
}

TArray<FTerritoryPresentationSnapshot> ARomanEmpireGameState::GetTerritorySnapshots() const
{
	TArray<FTerritoryPresentationSnapshot> Result;
	if (!IsGameplayInitialized() || WorldMap->GetWorld() != GetWorld()) return Result;
	for (FName ID : WorldMap->GetTerritoryIDs())
	{
		FTerritoryPresentationSnapshot Snapshot;
		// Fail closed instead of silently returning a partial catalog.
		if (!TryGetTerritorySnapshot(ID, Snapshot)) return {};
		Result.Add(MoveTemp(Snapshot));
	}
	return Result;
}

void ARomanEmpireGameState::HandleTerritoryOwnerChanged(
	FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner)
{
	(void)NewOwner;
	FTerritoryPresentationSnapshot Current;
	if (TryGetTerritorySnapshot(TerritoryID, Current))
	{
		OnTerritoryOwnerChanged.Broadcast(TerritoryID, PreviousOwner, Current.OwnerFaction);
	}
}

void ARomanEmpireGameState::HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode)
{
	OnGameplayModeChanged.Broadcast(OldMode, NewMode);
}

void ARomanEmpireGameState::HandleTerritorySettlementChanged(FName TerritoryID)
{
	FTerritoryPresentationSnapshot Current;
	if (TryGetTerritorySnapshot(TerritoryID, Current)) OnTerritorySettlementChanged.Broadcast(TerritoryID);
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

void ARomanEmpireGameState::HandleTacticalBattleChanged(FTacticalBattleResult Result)
{
	(void)Result;
	OnTacticalBattleChanged.Broadcast(GetTacticalBattleResult());
}
