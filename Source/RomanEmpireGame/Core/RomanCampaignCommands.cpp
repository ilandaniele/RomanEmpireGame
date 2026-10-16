// Copyright Roman Empire Game. All Rights Reserved.
#include "Core/RomanEmpirePlayerController.h"
#include "Core/RomanEmpireGameMode.h"
#include "Core/RomanEmpireGameState.h"
#include "Content/RomanContentRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "World/CampaignManager.h"

UCampaignSessionSubsystem* ARomanEmpirePlayerController::GetCurrentCampaignSession() const
{
	const UWorld* World = GetWorld();
	UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
	return !bControllerEndingPlay && HasActorBegunPlay() && !IsActorBeingDestroyed() && IsLocalController() &&
		Instance && Instance->GetWorld() == World ? Instance->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
}

bool ARomanEmpirePlayerController::CanEnterCampaignSession() const
{
	const auto* Session = GetCurrentCampaignSession();
	return Session && !Session->HasSession() && !Session->HasPendingTransfer() && PublicGameState &&
		PublicGameState->IsGameplayInitialized() && PublicGameState->GetCurrentGameplayMode() != EGameplayMode::Possessed;
}
ECampaignSessionFailure ARomanEmpirePlayerController::EnterCampaignSession()
{
	return CanEnterCampaignSession() ? GetCurrentCampaignSession()->EnterStrategicCampaign() : ECampaignSessionFailure::Unavailable;
}
bool ARomanEmpirePlayerController::CanContinueCampaign() const
{
	const auto* Session = GetCurrentCampaignSession();
	return Session && Session->HasSession() && !Session->HasPendingTransfer() &&
		Session->GetPhase() == ECampaignTransferPhase::TacticalDeployment && PublicGameState &&
		PublicGameState->IsGameplayInitialized() && PublicGameState->GetTacticalBattleResult().IsResolved();
}
ECampaignSessionFailure ARomanEmpirePlayerController::ContinueCampaign()
{
	return CanContinueCampaign() ? GetCurrentCampaignSession()->ReturnToStrategicCampaign() : ECampaignSessionFailure::Unavailable;
}
bool ARomanEmpirePlayerController::CanRetryCampaignTransfer() const
{
	const auto* Session = GetCurrentCampaignSession();
	return Session && Session->CanRetryPendingTransfer();
}
ECampaignSessionFailure ARomanEmpirePlayerController::RetryCampaignTransfer()
{
	return CanRetryCampaignTransfer() ? GetCurrentCampaignSession()->RetryPendingTransfer() : ECampaignSessionFailure::Unavailable;
}
bool ARomanEmpirePlayerController::CanIssueCampaignActions() const
{
	const auto* Session = GetCurrentCampaignSession();
	return Session && Session->HasSession() && !Session->HasPendingTransfer() &&
		Session->GetPhase() == ECampaignTransferPhase::Strategic && PublicGameState && PublicGameState->IsGameplayInitialized() &&
		PublicGameState->IsCampaignActive() && PublicGameState->GetCurrentGameplayMode() == EGameplayMode::Strategic &&
		!PublicGameState->GetCampaignEncounterPlan().EncounterID.IsValid() && !PublicGameState->GetTacticalBattleResult().EncounterID.IsValid();
}
bool ARomanEmpirePlayerController::CanMoveCampaignArmy(FGuid ArmyID, FName TargetTerritoryID) const
{
	if (!CanIssueCampaignActions()) return false;
	FCampaignArmy Army; FTerritoryPresentationSnapshot Source, Target;
	return PublicGameState->TryGetCampaignArmy(ArmyID, Army) && Army.Faction == PublicGameState->GetPlayerFaction() &&
		PublicGameState->TryGetTerritorySnapshot(Army.TerritoryID, Source) &&
		PublicGameState->TryGetTerritorySnapshot(TargetTerritoryID, Target) && Source.OwnerFaction == Army.Faction &&
		Target.OwnerFaction == Army.Faction && Source.AdjacentTerritoryIDs.Contains(TargetTerritoryID);
}
FCampaignArmyMutationResult ARomanEmpirePlayerController::RequestMoveCampaignArmy(FGuid ArmyID, FName TargetTerritoryID)
{
	if (CanMoveCampaignArmy(ArmyID, TargetTerritoryID) && GameMode && GameMode->GetCampaignManager())
		return GameMode->GetCampaignManager()->MoveCampaignArmy(ArmyID, TargetTerritoryID);
	FCampaignArmyMutationResult Rejected; Rejected.ArmyID = ArmyID; Rejected.Failure = ECampaignArmyFailure::Unavailable;
	return Rejected;
}
bool ARomanEmpirePlayerController::CanStartCampaignBattle(FGuid ArmyID, FName TargetTerritoryID) const
{
	if (!CanIssueCampaignActions()) return false;
	FCampaignArmy Army; FTerritoryPresentationSnapshot Source, Target;
	const auto* Registry = URomanContentRegistrySubsystem::Get(this);
	const auto* Territory = Registry ? Registry->FindTerritory(TargetTerritoryID) : nullptr;
	return PublicGameState->GetPlayerFaction() == EFactionID::Rome &&
		PublicGameState->TryGetCampaignArmy(ArmyID, Army) && Army.Faction == EFactionID::Rome &&
		PublicGameState->TryGetTerritorySnapshot(Army.TerritoryID, Source) && Source.OwnerFaction == EFactionID::Rome &&
		Source.AdjacentTerritoryIDs.Contains(TargetTerritoryID) &&
		PublicGameState->TryGetTerritorySnapshot(TargetTerritoryID, Target) && Target.OwnerFaction == EFactionID::Carthage &&
		GameMode && GameMode->GetFactionManager() && GameMode->GetFactionManager()->AreAtWar(Army.Faction, Target.OwnerFaction) &&
		Territory && Territory->HasValidTacticalMap();
}
ECampaignSessionFailure ARomanEmpirePlayerController::RequestStartCampaignBattle(FGuid ArmyID, FName TargetTerritoryID)
{
	return CanStartCampaignBattle(ArmyID, TargetTerritoryID) ?
		GetCurrentCampaignSession()->BeginTacticalEncounter(ArmyID, TargetTerritoryID) : ECampaignSessionFailure::EncounterRejected;
}

TArray<FName> ARomanEmpirePlayerController::GetCampaignUnitChoices() const
{
	TArray<FName> Choices;
	if (!CanIssueCampaignActions()) return Choices;
	const auto* Registry = URomanContentRegistrySubsystem::Get(this);
	const auto* Faction = Registry && Registry->IsReady() ? Registry->FindFaction(PublicGameState->GetPlayerFaction()) : nullptr;
	if (!Faction) return Choices;
	for (const auto* Unit : Registry->GetAllUnits())
		if (Unit && Unit->UnitClass.Get() && Unit->UnitClass.Get()->IsChildOf(AUnitBase::StaticClass()) &&
			Unit->AvailableToFactions.Contains(Faction->Definition.FactionID)) Choices.Add(Unit->Definition.UnitID);
	Choices.Sort(FNameLexicalLess()); return Choices;
}

FCampaignRecruitmentQuote ARomanEmpirePlayerController::GetCampaignRecruitmentQuote(FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID) const
{
	return CanIssueCampaignActions() && IsValid(GameMode) && IsValid(GameMode->GetCampaignManager())
		? GameMode->GetCampaignManager()->GetCampaignRecruitmentQuote(TerritoryID, UnitID, Count, DestinationArmyID) : FCampaignRecruitmentQuote();
}
FCampaignArmyMutationResult ARomanEmpirePlayerController::RequestRecruitCampaignUnits(FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID)
{
	const auto Quote = GetCampaignRecruitmentQuote(TerritoryID, UnitID, Count, DestinationArmyID);
	if (Quote.IsAvailable()) return GameMode->GetCampaignManager()->RecruitCampaignUnits(TerritoryID, UnitID, Count, DestinationArmyID);
	FCampaignArmyMutationResult Result; Result.ArmyID = DestinationArmyID; Result.Failure = Quote.Failure; return Result;
}
bool ARomanEmpirePlayerController::CanAssignCampaignArmyUnits(FGuid SourceArmyID, FGuid DestinationArmyID, FName UnitID, int32 Count) const
{
	if (!CanIssueCampaignActions() || Count <= 0 || SourceArmyID == DestinationArmyID) return false;
	FCampaignArmy Source, Destination; FTerritoryPresentationSnapshot Region;
	if (!PublicGameState->TryGetCampaignArmy(SourceArmyID, Source) || !PublicGameState->TryGetCampaignArmy(DestinationArmyID, Destination) ||
		Source.Faction != PublicGameState->GetPlayerFaction() || Destination.Faction != Source.Faction || Source.TerritoryID != Destination.TerritoryID ||
		!PublicGameState->TryGetTerritorySnapshot(Source.TerritoryID, Region) || Region.OwnerFaction != Source.Faction) return false;
	FCampaignArmyState Candidate;
	return Candidate.ReplaceFromSnapshot({Source, Destination}).WasSuccessful() && Candidate.TransferUnits(SourceArmyID, DestinationArmyID, UnitID, Count).WasSuccessful();
}
FCampaignArmyMutationResult ARomanEmpirePlayerController::RequestAssignCampaignArmyUnits(FGuid SourceArmyID, FGuid DestinationArmyID, FName UnitID, int32 Count)
{
	if (CanAssignCampaignArmyUnits(SourceArmyID, DestinationArmyID, UnitID, Count) && IsValid(GameMode) && IsValid(GameMode->GetCampaignManager()))
		return GameMode->GetCampaignManager()->AssignCampaignArmyUnits(SourceArmyID, DestinationArmyID, UnitID, Count);
	FCampaignArmyMutationResult Result; Result.ArmyID = DestinationArmyID; Result.Failure = ECampaignArmyFailure::Unavailable; return Result;
}
