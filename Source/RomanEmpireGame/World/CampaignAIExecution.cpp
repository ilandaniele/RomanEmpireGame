// Copyright Roman Empire Game. All Rights Reserved.
#include "World/CampaignManager.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Faction/FactionManager.h"
#include "Templates/UnrealTemplate.h"
#include "World/WorldMapManager.h"

ECampaignTurnFailure ACampaignManager::ApplyCampaignAIProposal()
{
	if (ValidateCampaignAuthority(false, true) != ECampaignEncounterFailure::None) return ECampaignTurnFailure::TurnAlreadyProcessing;
	const auto Proposal = GetCampaignAIProposal();
	if (Proposal.Action == ECampaignAIAction::None) return ECampaignTurnFailure::None;
	if (Proposal.Action == ECampaignAIAction::Recruit)
		return RecruitUnitsForFaction(EFactionID::Carthage, Proposal.TerritoryID, Proposal.UnitID, Proposal.Count, Proposal.ArmyID).WasSuccessful()
			? ECampaignTurnFailure::None : ECampaignTurnFailure::ResourceTransactionFailed;
	if (Proposal.Action == ECampaignAIAction::Defend)
	{
		FCampaignArmy Army;
		if (!ArmyState.TryGetArmy(Proposal.ArmyID, Army) || Army.Faction != EFactionID::Carthage ||
			WorldMapManager->GetTerritoryOwner(Army.TerritoryID) != Army.Faction || WorldMapManager->GetTerritoryOwner(Proposal.TerritoryID) != Army.Faction ||
			!WorldMapManager->AreTerritoryIDsAdjacent(Army.TerritoryID, Proposal.TerritoryID)) return ECampaignTurnFailure::ResourceTransactionFailed;
		FCampaignArmyState Candidate = ArmyState;
		if (!Candidate.RelocateArmy(Proposal.ArmyID, Proposal.TerritoryID).WasSuccessful()) return ECampaignTurnFailure::ResourceTransactionFailed;
		TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
		TGuardValue<bool> MapGuard(WorldMapManager->bCampaignTransactionActive, true);
		TGuardValue<bool> FactionGuard(FactionManager->bCampaignTransferPending, true);
		ArmyState = MoveTemp(Candidate); OnCampaignArmiesChanged.Broadcast(); return ECampaignTurnFailure::None;
	}
	FCampaignEncounterState Candidate;
	if (PrepareEncounterForTravel(Proposal.ArmyID, Proposal.TerritoryID, Candidate, true) != ECampaignEncounterFailure::None)
		return ECampaignTurnFailure::ResourceTransactionFailed;
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> MapGuard(WorldMapManager->bCampaignTransactionActive, true);
	TGuardValue<bool> FactionGuard(FactionManager->bCampaignTransferPending, true);
	EncounterState = MoveTemp(Candidate); bAwaitingAITacticalTravel = true;
	OnCampaignEncounterChanged.Broadcast(); return ECampaignTurnFailure::None;
}

bool ACampaignManager::ValidateQueuedAIEncounter(FCampaignEncounterState& Out) const
{
	if (!bAwaitingAITacticalTravel || ValidateCampaignAuthority() != ECampaignEncounterFailure::None) return false;
	FCampaignEncounterCheckpoint Checkpoint;
	return EncounterState.ExportCheckpoint(Checkpoint) == ECampaignEncounterFailure::None &&
		Out.RestoreCheckpoint(Checkpoint, ArmyState, WorldMapManager->CampaignTerritoryState) == ECampaignEncounterFailure::None;
}

ECampaignTurnFailure ACampaignManager::DispatchQueuedAIEncounter()
{
	if (!bAwaitingAITacticalTravel) return ECampaignTurnFailure::None;
	auto* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (Session && Session->BeginQueuedAITacticalEncounter() == ECampaignSessionFailure::None && Session->HasPendingTransfer())
	{
		bAwaitingAITacticalTravel = false; return ECampaignTurnFailure::None;
	}
	// Keep the accepted identity/plan. Repair and retry never run production/AI/increment again.
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	OnCampaignEncounterChanged.Broadcast(); return ECampaignTurnFailure::AITacticalTravelFailed;
}

void ACampaignManager::CancelQueuedAIOnTerminalOutcome()
{
	if (bCampaignActive || !bAwaitingAITacticalTravel) return;
	if (EncounterState.CancelUnboundPending())
	{
		bAwaitingAITacticalTravel = false;
		TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
		OnCampaignEncounterChanged.Broadcast();
	}
}
