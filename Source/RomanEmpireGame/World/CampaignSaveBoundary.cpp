// Copyright Roman Empire Game. All Rights Reserved.
#include "World/CampaignManager.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Faction/FactionManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "World/WorldMapManager.h"

ECampaignTransferFailure ACampaignManager::ValidateSavedCampaign(const FCampaignTransferSnapshot& Snapshot) const
{
	if (!IsValid(FactionManager) || !IsValid(WorldMapManager)) return ECampaignTransferFailure::Unavailable;
	if (Snapshot.Phase != ECampaignTransferPhase::Strategic) return ECampaignTransferFailure::InvalidPhase;
	FCampaignTransferCandidates Candidate;
	const auto Failure = Snapshot.BuildCandidates(WorldMapManager->CampaignTerritoryState, FactionManager->GetActiveFactions(), Candidate);
	if (Failure != ECampaignTransferFailure::None) return Failure;
	FCampaignEncounterPlan Content; Content.Armies = Snapshot.Armies;
	return ValidateDeploymentContent(Content) ? ECampaignTransferFailure::None : ECampaignTransferFailure::InvalidContent;
}

ECampaignTransferFailure ACampaignManager::CaptureForSave(FCampaignTransferSnapshot& Out, bool bAutosaveBoundary) const
{
	const auto Authority = ValidateCampaignAuthority(true, false, bAutosaveBoundary);
	if (Authority != ECampaignEncounterFailure::None)
		return Authority == ECampaignEncounterFailure::MutationInProgress ? ECampaignTransferFailure::MutationInProgress : ECampaignTransferFailure::Unavailable;
	const auto* Session = GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
	const auto* Mode = GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>();
	const auto* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (!Session || !Session->HasSession() || Session->HasPendingTransfer() || Session->GetPhase() != ECampaignTransferPhase::Strategic ||
		Mode->GetCurrentGameplayMode() != EGameplayMode::Strategic || !Battle || Battle->HasEncounter()) return ECampaignTransferFailure::InvalidPhase;
	FCampaignTransferSnapshot Snapshot;
	const FString Package = UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName());
	Snapshot.DestinationMap = FSoftObjectPath(Package + TEXT(".") + FPackageName::GetShortName(Package));
	Snapshot.PlayerFaction = FactionManager->PlayerFaction; Snapshot.Diplomacy = FactionManager->DiplomacyState.GetSnapshot();
	Snapshot.Territories = WorldMapManager->CampaignTerritoryState.GetRuntimeSnapshot(); Snapshot.Armies = ArmyState.GetArmies();
	if (EncounterState.ExportCheckpoint(Snapshot.Encounter) != ECampaignEncounterFailure::None) return ECampaignTransferFailure::InvalidEncounter;
	Snapshot.Turn.CurrentTurn = CurrentTurn; Snapshot.Turn.MaxTurns = MaxTurns;
	Snapshot.Turn.ConquestVictoryTerritories = ConquestVictoryTerritories; Snapshot.Turn.EconomicVictoryGold = EconomicVictoryGold;
	Snapshot.Turn.LastAutosaveRequestedTurn = LastAutosaveRequestedTurn; Snapshot.Turn.bCampaignActive = bCampaignActive;
	Snapshot.Turn.bAwaitingAITacticalTravel = bAwaitingAITacticalTravel; Snapshot.Turn.Outcome = CampaignOutcome;
	Snapshot.Turn.VictoryCondition = ResolvedVictoryCondition; Snapshot.Turn.Random = FCampaignRandomCheckpoint::Capture(TurnRandomStream);
	for (const auto ID : FactionManager->GetActiveFactions())
	{ FCampaignFactionWallet Wallet; Wallet.Faction = ID; Wallet.Resources = FactionManager->GetFactionResources(ID); Snapshot.Factions.Add(Wallet); }
	const auto Failure = ValidateSavedCampaign(Snapshot);
	if (Failure != ECampaignTransferFailure::None) return Failure;
	Out = MoveTemp(Snapshot); return ECampaignTransferFailure::None; // No freeze, refunds, events, actor scan or owner mutation.
}
