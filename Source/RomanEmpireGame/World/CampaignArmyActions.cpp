// Copyright Roman Empire Game. All Rights Reserved.
#include "World/CampaignManager.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Faction/FactionManager.h"
#include "Templates/UnrealTemplate.h"
#include "Units/UnitBase.h"
#include "World/WorldMapManager.h"

ECampaignArmyFailure ACampaignManager::ValidateStrategicArmyAction(bool bReadOnly) const
{
	const auto Authority = ValidateCampaignAuthority();
	// Event-driven UI can read the committed state while publication blocks mutators.
	// Travel/start remain unavailable, and commands always use the strict default.
	const bool bReadablePublication = bReadOnly && Authority == ECampaignEncounterFailure::MutationInProgress && !bCampaignTransferPending && !bIsStartingCampaign;
	if (Authority != ECampaignEncounterFailure::None && !bReadablePublication)
		return Authority == ECampaignEncounterFailure::MutationInProgress ? ECampaignArmyFailure::MutationInProgress : ECampaignArmyFailure::Unavailable;
	const auto* Instance = GetGameInstance();
	const auto* Session = Instance && Instance->GetWorld() == GetWorld() ? Instance->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (!Session || !Session->HasSession() || Session->HasPendingTransfer()) return ECampaignArmyFailure::Unavailable;
	if (Session->GetPhase() != ECampaignTransferPhase::Strategic || GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>()->GetCurrentGameplayMode() != EGameplayMode::Strategic)
		return ECampaignArmyFailure::WrongMode;
	const auto* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (!Battle) return ECampaignArmyFailure::Unavailable;
	if (EncounterState.HasPendingEncounter() || Battle->HasEncounter()) return ECampaignArmyFailure::EncounterPending;
	return ECampaignArmyFailure::None;
}

FCampaignArmyMutationResult ACampaignManager::AssignCampaignArmyUnits(FGuid SourceArmyID, FGuid DestinationArmyID, FName UnitID, int32 Count)
{
	FCampaignArmyMutationResult Result; Result.ArmyID = DestinationArmyID;
	Result.Failure = ValidateStrategicArmyAction();
	if (Result.Failure != ECampaignArmyFailure::None) return Result;
	FCampaignArmy Source, Destination;
	if (!ArmyState.TryGetArmy(SourceArmyID, Source) || !ArmyState.TryGetArmy(DestinationArmyID, Destination))
	{ Result.Failure = ECampaignArmyFailure::UnknownArmy; return Result; }
	const auto Player = FactionManager->GetPlayerFaction();
	if (Source.Faction != Player || Destination.Faction != Player || WorldMapManager->GetTerritoryOwner(Source.TerritoryID) != Player || WorldMapManager->GetTerritoryOwner(Destination.TerritoryID) != Player)
	{ Result.Failure = ECampaignArmyFailure::NotOwned; return Result; }
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> TerritoryGuard(WorldMapManager->bCampaignTransactionActive, true);
	TGuardValue<bool> FactionGuard(FactionManager->bCampaignTransferPending, true);
	Result = ArmyState.TransferUnits(SourceArmyID, DestinationArmyID, UnitID, Count);
	if (Result.WasSuccessful()) OnCampaignArmiesChanged.Broadcast();
	return Result;
}

FCampaignRecruitmentQuote ACampaignManager::GetCampaignRecruitmentQuote(FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID) const
{
	FCampaignRecruitmentQuote Quote; Quote.Failure = ValidateStrategicArmyAction(true);
	if (Quote.Failure != ECampaignArmyFailure::None) return Quote;
	return QuoteRecruitmentForFaction(FactionManager->GetPlayerFaction(), TerritoryID, UnitID, Count, DestinationArmyID);
}

FCampaignRecruitmentQuote ACampaignManager::QuoteRecruitmentForFaction(EFactionID PayingFaction, FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID) const
{
	FCampaignRecruitmentQuote Quote;
	if (Count <= 0) { Quote.Failure = ECampaignArmyFailure::InvalidUnitCount; return Quote; }
	if (!RomanEmpireContent::IsStableID(TerritoryID) || !WorldMapManager->GetTerritoryIDs().Contains(TerritoryID))
	{ Quote.Failure = ECampaignArmyFailure::InvalidTerritoryID; return Quote; }
	const auto Player = PayingFaction;
	if (WorldMapManager->GetTerritoryOwner(TerritoryID) != Player) { Quote.Failure = ECampaignArmyFailure::NotOwned; return Quote; }
	FTerritorySettlementState Settlement;
	if (!WorldMapManager->TryGetTerritorySettlement(TerritoryID, Settlement) || !Settlement.bHasSettlement) { Quote.Failure = ECampaignArmyFailure::SettlementRequired; return Quote; }
	const auto* Registry = URomanContentRegistrySubsystem::Get(this);
	const auto* Faction = Registry ? Registry->FindFaction(Player) : nullptr;
	const auto* Unit = Registry ? Registry->FindUnit(UnitID) : nullptr;
	if (!Registry || !Registry->IsReady() || !Faction || !Unit || Unit->Definition.UnitID != UnitID || !Unit->UnitClass.Get() || !Unit->UnitClass.Get()->IsChildOf(AUnitBase::StaticClass()) ||
		!Unit->AvailableToFactions.Contains(Faction->Definition.FactionID) || !Unit->Definition.TrainingCost.IsNonNegative())
	{ Quote.Failure = ECampaignArmyFailure::InvalidContent; return Quote; }
	if (DestinationArmyID.IsValid())
	{
		FCampaignArmy Destination;
		if (!ArmyState.TryGetArmy(DestinationArmyID, Destination)) { Quote.Failure = ECampaignArmyFailure::UnknownArmy; return Quote; }
		if (Destination.Faction != Player) { Quote.Failure = ECampaignArmyFailure::NotOwned; return Quote; }
		if (Destination.TerritoryID != TerritoryID) { Quote.Failure = ECampaignArmyFailure::NotCoLocated; return Quote; }
		FCampaignArmyState Candidate = ArmyState;
		Quote.Failure = Candidate.AddUnits(DestinationArmyID, UnitID, Count).Failure;
		if (Quote.Failure != ECampaignArmyFailure::None) return Quote;
	}
	const FFactionResourceAmounts& PerUnit = Unit->Definition.TrainingCost;
	const auto Scale = [Count](int32 Value, int32& Out) { const int64 Total = int64(Value) * Count; if (Total > MAX_int32) return false; Out = int32(Total); return true; };
	FFactionResourceAmounts Cost;
	if (!Scale(PerUnit.Gold, Cost.Gold) || !Scale(PerUnit.Food, Cost.Food) || !Scale(PerUnit.Iron, Cost.Iron) ||
		!Scale(PerUnit.Wood, Cost.Wood) || !Scale(PerUnit.Stone, Cost.Stone) || !Scale(PerUnit.Population, Cost.Population))
	{ Quote.Failure = ECampaignArmyFailure::CostOverflow; return Quote; }
	Quote.Cost = Cost;
	FFactionResourceAmounts Balance = FactionManager->GetFactionResources(Player);
	Quote.Failure = RomanEmpireEconomy::TrySpendResources(Balance, Cost).WasSuccessful() ? ECampaignArmyFailure::None : ECampaignArmyFailure::InsufficientResources;
	return Quote;
}

FCampaignArmyMutationResult ACampaignManager::RecruitCampaignUnits(FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID)
{
	FCampaignArmyMutationResult Result; Result.ArmyID = DestinationArmyID;
	Result.Failure = ValidateStrategicArmyAction();
	if (Result.Failure != ECampaignArmyFailure::None) return Result;
	return RecruitUnitsForFaction(FactionManager->GetPlayerFaction(), TerritoryID, UnitID, Count, DestinationArmyID);
}

FCampaignArmyMutationResult ACampaignManager::RecruitUnitsForFaction(EFactionID PayingFaction, FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID)
{
	FCampaignArmyMutationResult Result; Result.ArmyID = DestinationArmyID;
	const auto Quote = QuoteRecruitmentForFaction(PayingFaction, TerritoryID, UnitID, Count, DestinationArmyID);
	Result.Failure = Quote.Failure;
	if (!Quote.IsAvailable()) return Result;
	const auto Player = PayingFaction;
	FCampaignArmyState Candidate = ArmyState;
	if (DestinationArmyID.IsValid()) Result = Candidate.AddUnits(DestinationArmyID, UnitID, Count);
	else
	{
		FCampaignArmy Army; Army.ArmyID = FGuid::NewGuid(); Army.Faction = Player; Army.TerritoryID = TerritoryID;
		FCampaignArmyUnitStack Stack; Stack.UnitID = UnitID; Stack.Count = Count; Army.Units.Add(Stack);
		Result = Candidate.RegisterArmy(Army);
	}
	if (!Result.WasSuccessful()) return Result;
	FFactionResourceAmounts Balance = FactionManager->GetFactionResources(Player);
	if (!RomanEmpireEconomy::TrySpendResources(Balance, Quote.Cost).WasSuccessful())
	{ Result.Failure = ECampaignArmyFailure::InsufficientResources; return Result; }
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> TerritoryGuard(WorldMapManager->bCampaignTransactionActive, true);
	TGuardValue<bool> FactionGuard(FactionManager->bCampaignTransferPending, true);
	// Existing owners commit both domains before either notification. Never use debit-then-rollback events.
	ArmyState = MoveTemp(Candidate);
	FactionManager->FactionResourcesMap.FindChecked(Player) = Balance;
	if (!Quote.Cost.IsZero()) FactionManager->OnFactionResourcesChanged.Broadcast(Player, Balance);
	OnCampaignArmiesChanged.Broadcast();
	return Result;
}
