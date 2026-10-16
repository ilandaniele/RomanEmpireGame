// Copyright Roman Empire Game. All Rights Reserved.
#include "World/CampaignManager.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Battle/TacticalObjective.h"
#include "Building/Barracks.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "World/WorldMapManager.h"
#include "World/TerritoryRegion.h"
#include "Misc/ScopeExit.h"
#include "Templates/UnrealTemplate.h"

ECampaignEncounterFailure ACampaignManager::PrepareEncounterForTravel(FGuid ArmyID, FName Target, FCampaignEncounterState& Out, bool bAIStage) const
{
	const ECampaignEncounterFailure Authority = ValidateCampaignAuthority(false, bAIStage);
	if (Authority != ECampaignEncounterFailure::None) return Authority;
	const ARomanEmpireGameMode* Mode = GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>();
	const UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (Mode->GetCurrentGameplayMode() != EGameplayMode::Strategic) return ECampaignEncounterFailure::WrongMode;
	if (!Battle || Battle->HasEncounter()) return ECampaignEncounterFailure::EncounterAlreadyRunning;
	const EFactionID Player = FactionManager->GetPlayerFaction();
	const EFactionID Attacker = bAIStage ? EFactionID::Carthage : Player;
	const EFactionID Defender = WorldMapManager->GetTerritoryOwner(Target);
	if (!FactionManager->IsFactionActive(Player) || !FactionManager->IsFactionActive(Attacker) || !FactionManager->IsFactionActive(Defender)) return ECampaignEncounterFailure::InvalidTarget;
	if (Player != EFactionID::Rome || (bAIStage ? Defender != Player : Defender != EFactionID::Carthage)) return ECampaignEncounterFailure::UnsupportedEncounter;
	if (!FactionManager->AreAtWar(Attacker, Defender)) return ECampaignEncounterFailure::NotAtWar;
	const ATacticalObjective* Objective = Battle->GetObjectiveActor();
	if (!IsValid(Objective) || Objective->GetWorld() != GetWorld()) return ECampaignEncounterFailure::InvalidObjectiveID;
	FCampaignEncounterState Candidate = EncounterState;
	const ECampaignEncounterFailure Result = Candidate.Begin(FGuid::NewGuid(), Objective->GetObjectiveID(), ArmyID,
		Target, Attacker, ArmyState, WorldMapManager->CampaignTerritoryState, Player);
	if (Result != ECampaignEncounterFailure::None) return Result;
	if (!ValidateDeploymentContent(Candidate.GetPlan())) return ECampaignEncounterFailure::InvalidContent;
	Out = MoveTemp(Candidate);
	return ECampaignEncounterFailure::None;
}

ECampaignTransferFailure ACampaignManager::CaptureForTravel(ECampaignTransferPhase Phase, const FSoftObjectPath& Destination,
	const FSoftObjectPath& ReturnMap, const FCampaignEncounterState& PreparedEncounter, FCampaignTransferSnapshot& Out)
{
	const ECampaignEncounterFailure Authority = ValidateCampaignAuthority(true);
	if (Authority != ECampaignEncounterFailure::None)
		return Authority == ECampaignEncounterFailure::MutationInProgress ? ECampaignTransferFailure::MutationInProgress : ECampaignTransferFailure::Unavailable;
	FCampaignTransferSnapshot Snapshot;
	Snapshot.Phase = Phase; Snapshot.DestinationMap = Destination; Snapshot.StrategicReturnMap = ReturnMap;
	Snapshot.PlayerFaction = FactionManager->PlayerFaction;
	Snapshot.Diplomacy = FactionManager->DiplomacyState.GetSnapshot();
	Snapshot.Territories = WorldMapManager->CampaignTerritoryState.GetRuntimeSnapshot();
	Snapshot.Armies = ArmyState.GetArmies();
	if (PreparedEncounter.ExportCheckpoint(Snapshot.Encounter) != ECampaignEncounterFailure::None)
		return ECampaignTransferFailure::InvalidEncounter;
	Snapshot.Turn.CurrentTurn = CurrentTurn; Snapshot.Turn.MaxTurns = MaxTurns;
	Snapshot.Turn.ConquestVictoryTerritories = ConquestVictoryTerritories; Snapshot.Turn.EconomicVictoryGold = EconomicVictoryGold;
	Snapshot.Turn.LastAutosaveRequestedTurn = LastAutosaveRequestedTurn; Snapshot.Turn.bCampaignActive = bCampaignActive;
	Snapshot.Turn.bAwaitingAITacticalTravel = Phase == ECampaignTransferPhase::Strategic && bAwaitingAITacticalTravel;
	Snapshot.Turn.Outcome = CampaignOutcome; Snapshot.Turn.VictoryCondition = ResolvedVictoryCondition;
	Snapshot.Turn.Random = FCampaignRandomCheckpoint::Capture(TurnRandomStream);

	// One bounded scan at travel, never per unit/tick/UI. No producer or wallet changes
	// until ALL receipts and their combined credit have passed the canonical economy.
	TMap<EFactionID, FFactionResourceAmounts> Wallets = FactionManager->FactionResourcesMap;
	TArray<ABarracks*> Producers;
	TMap<ABarracks*, TArray<FTrainingReservation>> Orders;
	TSet<FGuid> ReceiptIDs;
	for (TActorIterator<ABarracks> It(GetWorld()); It; ++It)
	{
		ABarracks* Producer = *It;
		if (Producer->IsActorBeingDestroyed() || !Producer->HasActorBegunPlay() || Producer->bIsUpdatingProduction)
			return ECampaignTransferFailure::MutationInProgress;
		Producers.Add(Producer);
		TArray<FTrainingReservation>& Receipts = Orders.Add(Producer);
		if (Producer->bIsTraining) Receipts.Add(Producer->ActiveTrainingOrder);
		Receipts.Append(Producer->TrainingQueue);
		for (const FTrainingReservation& Receipt : Receipts)
		{
			FFactionResourceAmounts* Balance = Wallets.Find(Receipt.PayingFaction);
			if (!Balance || !Receipt.ReceiptID.IsValid() || ReceiptIDs.Contains(Receipt.ReceiptID) ||
				!Receipt.PaidCost.IsNonNegative() || Snapshot.Encounter.EnrolledReceiptIDs.Contains(Receipt.ReceiptID))
				return ECampaignTransferFailure::RefundRejected;
			ReceiptIDs.Add(Receipt.ReceiptID);
			const FFactionResourceAmounts& Paid = Receipt.PaidCost;
			FFactionResourceDelta Refund;
			Refund.Gold = Paid.Gold; Refund.Food = Paid.Food; Refund.Iron = Paid.Iron;
			Refund.Wood = Paid.Wood; Refund.Stone = Paid.Stone; Refund.Population = Paid.Population;
			if (!RomanEmpireEconomy::TryApplyResourceDelta(*Balance, Refund).WasSuccessful())
				return ECampaignTransferFailure::RefundRejected;
		}
	}
	const TArray<EFactionID> Active = FactionManager->GetActiveFactions();
	for (const EFactionID Faction : Active)
	{
		FCampaignFactionWallet Wallet; Wallet.Faction = Faction; Wallet.Resources = Wallets.FindChecked(Faction);
		Snapshot.Factions.Add(Wallet);
	}
	FCampaignTransferCandidates Checked;
	const ECampaignTransferFailure Valid = Snapshot.BuildCandidates(WorldMapManager->CampaignTerritoryState, Active, Checked);
	if (Valid != ECampaignTransferFailure::None) return Valid;
	FCampaignEncounterPlan AllContent; AllContent.Armies = Snapshot.Armies;
	if (!ValidateDeploymentContent(AllContent)) return ECampaignTransferFailure::InvalidContent;

	const TMap<EFactionID, FFactionResourceAmounts> PreviousWallets = FactionManager->FactionResourcesMap;
	FreezeForTravel();
	FactionManager->FactionResourcesMap = MoveTemp(Wallets);
	for (ABarracks* Producer : Producers)
	{
		Producer->ActiveTrainingOrder = FTrainingReservation(); Producer->TrainingQueue.Reset();
		Producer->bIsTraining = false; Producer->bAwaitingFailureRefund = false;
		Producer->TrainingProgress = 0.0f; Producer->TrainingTime = 0.0f;
	}
	Out = MoveTemp(Snapshot);
	// Every observer sees all credits and cleared queues. External mutators stay
	// frozen until teardown or an engine travel failure, not just this stack frame.
	for (const EFactionID Faction : Active)
	{
		const FFactionResourceAmounts Balance = FactionManager->GetFactionResources(Faction);
		if (!FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Balance, &PreviousWallets.FindChecked(Faction), 0))
			FactionManager->OnFactionResourcesChanged.Broadcast(Faction, Balance);
	}
	for (ABarracks* Producer : Producers)
		if (IsValid(Producer) && !Producer->IsActorBeingDestroyed())
			for (const FTrainingReservation& Receipt : Orders.FindChecked(Producer))
				Producer->BroadcastProductionEvent(EProductionEventType::Cancelled, Receipt.UnitID);
	return ECampaignTransferFailure::None;
}

void ACampaignManager::FreezeForTravel()
{
	bCampaignTransferPending = true;
	if (IsValid(FactionManager)) FactionManager->bCampaignTransferPending = true;
	if (IsValid(WorldMapManager)) WorldMapManager->bCampaignTransactionActive = true;
	for (TActorIterator<ABarracks> It(GetWorld()); It; ++It) It->bIsUpdatingProduction = true;
}

void ACampaignManager::CancelTravelFreeze()
{
	if (!bCampaignTransferPending) return;
	bCampaignTransferPending = false;
	if (IsValid(FactionManager)) FactionManager->bCampaignTransferPending = false;
	if (IsValid(WorldMapManager)) WorldMapManager->bCampaignTransactionActive = false;
	for (TActorIterator<ABarracks> It(GetWorld()); It; ++It) It->bIsUpdatingProduction = false;
}

ECampaignTransferFailure ACampaignManager::RestoreFromTravel(const FCampaignTransferSnapshot& Snapshot)
{
	ARomanEmpireGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	if (!HasActorBegunPlay() || IsActorBeingDestroyed() || !Mode || Mode->IsGameplayInitialized() ||
		Mode->GetCampaignManager() != this || Mode->GetFactionManager() != FactionManager ||
		Mode->GetWorldMapManager() != WorldMapManager || !IsValid(FactionManager) || !IsValid(WorldMapManager) ||
		FactionManager->GetWorld() != GetWorld() || WorldMapManager->GetWorld() != GetWorld() ||
		!FactionManager->HasActorBegunPlay() || !WorldMapManager->HasActorBegunPlay()) return ECampaignTransferFailure::Unavailable;
	FCampaignTransferCandidates Candidate;
	const ECampaignTransferFailure Checked = Snapshot.BuildCandidates(WorldMapManager->CampaignTerritoryState,
		FactionManager->GetActiveFactions(), Candidate);
	if (Checked != ECampaignTransferFailure::None) return Checked;
	FCampaignEncounterPlan AllContent; AllContent.Armies = Snapshot.Armies;
	if (!ValidateDeploymentContent(AllContent)) return ECampaignTransferFailure::InvalidContent;
	FRandomStream Random;
	if (!Candidate.Turn.Random.Restore(Random)) return ECampaignTransferFailure::InvalidTurnState;
	UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (!Battle || Battle->HasEncounter()) return ECampaignTransferFailure::InvalidEncounter;
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> MapGuard(WorldMapManager->bCampaignTransactionActive, true);
	TGuardValue<bool> WalletGuard(FactionManager->bCampaignTransferPending, true);
	TArray<AUnitBase*> Spawned;
	bool bCommitted = false;
	ON_SCOPE_EXIT { if (!bCommitted) for (AUnitBase* Unit : Spawned) if (IsValid(Unit)) Unit->Destroy(); };
	if (Snapshot.Phase == ECampaignTransferPhase::TacticalDeployment)
	{
		const ATacticalObjective* Objective = Battle->GetObjectiveActor();
		if (!Objective || Objective->GetObjectiveID() != Candidate.Encounter.GetPlan().ObjectiveID ||
			!Mode->MaterializeCampaignDeployment(Candidate.Encounter, Spawned)) return ECampaignTransferFailure::InvalidEncounter;
		if (Candidate.Encounter.ValidateDeployment(Candidate.Armies, Candidate.Territories) != ECampaignEncounterFailure::None)
			return ECampaignTransferFailure::InvalidEncounter;
	}
	// Quiet all-domain commit, BEFORE readiness or the first tactical publication.
	FactionManager->FactionResourcesMap = MoveTemp(Candidate.Wallets);
	FactionManager->PlayerFaction = Candidate.PlayerFaction; FactionManager->DiplomacyState = MoveTemp(Candidate.Diplomacy);
	WorldMapManager->CampaignTerritoryState = MoveTemp(Candidate.Territories);
	ArmyState = MoveTemp(Candidate.Armies); EncounterState = MoveTemp(Candidate.Encounter);
	CurrentTurn = Candidate.Turn.CurrentTurn; MaxTurns = Candidate.Turn.MaxTurns;
	ConquestVictoryTerritories = Candidate.Turn.ConquestVictoryTerritories; EconomicVictoryGold = Candidate.Turn.EconomicVictoryGold;
	LastAutosaveRequestedTurn = Candidate.Turn.LastAutosaveRequestedTurn; bCampaignActive = Candidate.Turn.bCampaignActive;
	bAwaitingAITacticalTravel = Candidate.Turn.bAwaitingAITacticalTravel;
	CampaignOutcome = Candidate.Turn.Outcome; ResolvedVictoryCondition = Candidate.Turn.VictoryCondition;
	TurnRandomStream = Random;
	for (ATerritoryRegion* Territory : WorldMapManager->GetAllTerritories())
	{
		if (!IsValid(Territory)) continue;
		const EFactionID TerritoryOwner = WorldMapManager->GetTerritoryOwner(Territory->GetTerritoryID());
		Territory->RefreshOwnershipPresentation(TerritoryOwner, TerritoryOwner); Territory->RefreshSettlementPresentation();
	}
	if (Snapshot.Phase == ECampaignTransferPhase::TacticalDeployment &&
		!Battle->BeginCampaignDeployment(EncounterState.GetPlan(), EncounterState.GetBindings()))
		return ECampaignTransferFailure::InvalidEncounter;
	bCommitted = true;
	return ECampaignTransferFailure::None;
}
