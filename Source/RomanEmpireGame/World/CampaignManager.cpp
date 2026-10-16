// Copyright Roman Empire Game. All Rights Reserved.

#include "CampaignManager.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Campaign/CampaignSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Battle/TacticalObjective.h"
#include "Building/Barracks.h"
#include "Core/RomanEmpireGameMode.h"
#include "Core/RomanEmpireGameState.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "World/WorldMapManager.h"
#include "World/TerritoryRegion.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/ScopeExit.h"

ACampaignManager::ACampaignManager()
{
	PrimaryActorTick.bCanEverTick = false;

	CurrentTurn = 1;
	bCampaignActive = false;
	MaxTurns = 100;
	ConquestVictoryTerritories = 7; // Control 7 of 9 territories
	EconomicVictoryGold = 10000;
	LastAutosaveRequestedTurn = 0;
	CampaignOutcome = ECampaignOutcome::InProgress;
	ResolvedVictoryCondition = EVictoryCondition::Conquest;

	FactionManager = nullptr;
	WorldMapManager = nullptr;
	TurnRandomStream.Initialize(PrototypeTurnRandomSeed);
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

	if (FactionManager)
	{
		FactionManager->OnFactionResourcesChanged.AddUniqueDynamic(
			this, &ACampaignManager::HandleFactionResourcesChanged);
		FactionManager->OnPlayerFactionChanged.AddUniqueDynamic(
			this, &ACampaignManager::HandlePlayerFactionChanged);
	}
	if (WorldMapManager)
	{
		WorldMapManager->OnTerritoryOwnerChanged.AddUniqueDynamic(
			this, &ACampaignManager::HandleTerritoryOwnerChanged);
	}

	// Start campaign automatically for prototype
	if (FactionManager)
	{
		StartNewCampaign(FactionManager->GetPlayerFaction());
	}
}

void ACampaignManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(FactionManager))
	{
		FactionManager->OnFactionResourcesChanged.RemoveDynamic(
			this, &ACampaignManager::HandleFactionResourcesChanged);
		FactionManager->OnPlayerFactionChanged.RemoveDynamic(
			this, &ACampaignManager::HandlePlayerFactionChanged);
	}
	if (IsValid(WorldMapManager))
	{
		WorldMapManager->OnTerritoryOwnerChanged.RemoveDynamic(
			this, &ACampaignManager::HandleTerritoryOwnerChanged);
	}
	ArmyState.Reset();
	EncounterState.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ACampaignManager::BuildStartingArmies(FCampaignArmyState& OutState) const
{
	OutState.Reset();
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* Unit = Registry ? Registry->FindUnit(TEXT("Legionary")) : nullptr;
	if (!Registry || !Registry->IsReady() || !Unit || !FactionManager || !WorldMapManager ||
		FactionManager->GetWorld() != GetWorld() || WorldMapManager->GetWorld() != GetWorld()) return false;
	// Preserve the existing prototype's 15/10 forces. Placement is the first lexical
	// owned region, independent of map actors; no new capital or recruitment rule.
	const TArray<FName> Territories = WorldMapManager->GetTerritoryIDs();
	for (const EFactionID Faction : FactionManager->GetActiveFactions())
	{
		const int32 Count = Faction == EFactionID::Rome ? 15 : Faction == EFactionID::Carthage ? 10 : 0;
		if (Count == 0) continue;
		const URomanFactionDefinition* Definition = Registry->FindFaction(Faction);
		if (!Definition || !Unit->AvailableToFactions.Contains(Definition->Definition.FactionID)) return false;
		for (const FName TerritoryID : Territories)
		{
			if (WorldMapManager->GetTerritoryOwner(TerritoryID) != Faction) continue;
			FCampaignArmy Army;
			Army.ArmyID = FGuid::NewGuid();
			Army.Faction = Faction;
			Army.TerritoryID = TerritoryID;
			FCampaignArmyUnitStack Stack;
			Stack.UnitID = Unit->Definition.UnitID;
			Stack.Count = Count;
			Army.Units.Add(Stack);
			if (!OutState.RegisterArmy(Army).WasSuccessful()) return false;
			break;
		}
	}
	return true;
}

TArray<FCampaignArmy> ACampaignManager::GetCampaignArmies() const
{
	return HasActorBegunPlay() && !IsActorBeingDestroyed() ? ArmyState.GetArmies() : TArray<FCampaignArmy>();
}

bool ACampaignManager::TryGetCampaignArmy(FGuid ArmyID, FCampaignArmy& OutArmy) const
{
	OutArmy = FCampaignArmy();
	return HasActorBegunPlay() && !IsActorBeingDestroyed() && ArmyState.TryGetArmy(ArmyID, OutArmy);
}

ECampaignEncounterFailure ACampaignManager::ValidateCampaignAuthority(bool bAllowResolvedCampaign, bool bAIStage, bool bAutosaveStage) const
{
	const UWorld* World = GetWorld();
	const ARomanEmpireGameMode* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
	if (!HasActorBegunPlay() || IsActorBeingDestroyed() || (!bCampaignActive && !bAllowResolvedCampaign) || !Mode || !Mode->IsGameplayInitialized() ||
		!State || !State->IsGameplayInitialized() || Mode->GetCampaignManager() != this ||
		Mode->GetFactionManager() != FactionManager || Mode->GetWorldMapManager() != WorldMapManager ||
		!IsValid(FactionManager) || !IsValid(WorldMapManager) ||
		FactionManager->GetWorld() != World || WorldMapManager->GetWorld() != World ||
		!FactionManager->HasActorBegunPlay() || !WorldMapManager->HasActorBegunPlay() ||
		FactionManager->IsActorBeingDestroyed() || WorldMapManager->IsActorBeingDestroyed()) return ECampaignEncounterFailure::Unavailable;
	const bool bAuthorizedAIStage = bAIStage && bIsProcessingTurn && ExecutingTurnStep == ECampaignTurnStep::AIActions;
	const bool bAuthorizedSaveStage = bAutosaveStage && bIsProcessingTurn && ExecutingTurnStep == ECampaignTurnStep::Autosave;
	if ((bAIStage && !bAuthorizedAIStage) || (bAutosaveStage && !bAuthorizedSaveStage) || (bAIStage && bAutosaveStage)) return ECampaignEncounterFailure::MutationInProgress;
	if (bCampaignTransferPending || bEncounterMutationActive || bIsStartingCampaign || (bIsProcessingTurn && !bAuthorizedAIStage && !bAuthorizedSaveStage) || WorldMapManager->bCampaignTransactionActive)
		return ECampaignEncounterFailure::MutationInProgress;
	return ECampaignEncounterFailure::None;
}

bool ACampaignManager::ValidateDeploymentContent(const FCampaignEncounterPlan& Plan) const
{
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!Registry || !Registry->IsReady()) return false;
	for (const FCampaignArmy& Army : Plan.Armies)
	{
		const URomanFactionDefinition* Faction = Registry->FindFaction(Army.Faction);
		if (!Faction || !FactionManager->IsFactionActive(Army.Faction)) return false;
		for (const FCampaignArmyUnitStack& Stack : Army.Units)
		{
			const URomanUnitDefinition* Unit = Registry->FindUnit(Stack.UnitID);
			if (!Unit || !Unit->UnitClass.Get() || !Unit->UnitClass.Get()->IsChildOf(AUnitBase::StaticClass()) ||
				!Unit->AvailableToFactions.Contains(Faction->Definition.FactionID)) return false;
		}
	}
	return true;
}

FCampaignArmyMutationResult ACampaignManager::MoveCampaignArmy(FGuid ArmyID, FName TargetTerritoryID)
{
	FCampaignArmyMutationResult Result; Result.ArmyID = ArmyID;
	const ECampaignEncounterFailure Authority = ValidateCampaignAuthority();
	if (Authority != ECampaignEncounterFailure::None)
	{
		Result.Failure = Authority == ECampaignEncounterFailure::MutationInProgress ? ECampaignArmyFailure::MutationInProgress : ECampaignArmyFailure::Unavailable;
		return Result;
	}
	if (EncounterState.HasPendingEncounter()) { Result.Failure = ECampaignArmyFailure::EncounterPending; return Result; }
	const ARomanEmpireGameMode* Mode = GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>();
	const UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (Mode->GetCurrentGameplayMode() != EGameplayMode::Strategic || (Battle && Battle->IsResolved()))
	{ Result.Failure = ECampaignArmyFailure::WrongMode; return Result; }
	FCampaignArmy Army;
	if (!ArmyState.TryGetArmy(ArmyID, Army)) { Result.Failure = ECampaignArmyFailure::UnknownArmy; return Result; }
	const EFactionID Player = FactionManager->GetPlayerFaction();
	if (!FactionManager->IsFactionActive(Player) || Army.Faction != Player || WorldMapManager->GetTerritoryOwner(Army.TerritoryID) != Player ||
		WorldMapManager->GetTerritoryOwner(TargetTerritoryID) != Player)
	{ Result.Failure = ECampaignArmyFailure::NotOwned; return Result; }
	if (!WorldMapManager->AreTerritoryIDsAdjacent(Army.TerritoryID, TargetTerritoryID))
	{ Result.Failure = ECampaignArmyFailure::NotAdjacent; return Result; }
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> TerritoryGuard(WorldMapManager->bCampaignTransactionActive, true);
	Result = ArmyState.RelocateArmy(ArmyID, TargetTerritoryID);
	if (Result.WasSuccessful()) OnCampaignArmiesChanged.Broadcast();
	return Result;
}

ECampaignEncounterFailure ACampaignManager::BeginCampaignEncounter(FGuid AttackingArmyID, FName TargetTerritoryID)
{
	const auto* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (Session && Session->HasSession()) return ECampaignEncounterFailure::WrongMode; // Sessions must use selected-map travel.
	FCampaignEncounterState Candidate;
	const ECampaignEncounterFailure Prepared = PrepareEncounterForTravel(AttackingArmyID, TargetTerritoryID, Candidate);
	if (Prepared != ECampaignEncounterFailure::None) return Prepared;
	ARomanEmpireGameMode* Mode = GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>();
	UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	const EFactionID Player = FactionManager->GetPlayerFaction();
	const EFactionID Defender = WorldMapManager->GetTerritoryOwner(TargetTerritoryID);
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> TerritoryGuard(WorldMapManager->bCampaignTransactionActive, true);
	TArray<AUnitBase*> Spawned;
	bool bCommitted = false;
	ON_SCOPE_EXIT { if (!bCommitted) for (AUnitBase* Unit : Spawned) if (IsValid(Unit)) Unit->Destroy(); };
	if (!Mode->MaterializeCampaignDeployment(Candidate, Spawned)) return ECampaignEncounterFailure::MutationFailed;
	if (!HasActorBegunPlay() || IsActorBeingDestroyed() || !IsValid(WorldMapManager) || !IsValid(FactionManager) ||
		!WorldMapManager->HasActorBegunPlay() || !FactionManager->HasActorBegunPlay() || !IsValid(Mode) ||
		Mode->GetCurrentGameplayMode() != EGameplayMode::Strategic ||
		FactionManager->GetPlayerFaction() != Player || !FactionManager->AreAtWar(Player, Defender) ||
		!ValidateDeploymentContent(Candidate.GetPlan())) return ECampaignEncounterFailure::StaleDeployment;
	const ECampaignEncounterFailure Validated = Candidate.ValidateDeployment(ArmyState, WorldMapManager->CampaignTerritoryState);
	if (Validated != ECampaignEncounterFailure::None) return Validated;
	const FCampaignEncounterState Previous = EncounterState;
	EncounterState = MoveTemp(Candidate); // Visible before the tactical subsystem's first notification.
	if (!Battle->BeginCampaignDeployment(EncounterState.GetPlan(), EncounterState.GetBindings()))
	{
		EncounterState = Previous;
		return ECampaignEncounterFailure::MutationFailed;
	}
	bCommitted = true;
	Mode->SetGameplayMode(EGameplayMode::Tactical);
	OnCampaignEncounterChanged.Broadcast();
	return ECampaignEncounterFailure::None;
}

ECampaignEncounterFailure ACampaignManager::ApplyCurrentEncounterResult()
{
	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (!HasActorBegunPlay() || IsActorBeingDestroyed() || !Battle) return ECampaignEncounterFailure::Unavailable;
	const FTacticalBattleResult Result = Battle->GetResult();
	if (bEncounterMutationActive) return ECampaignEncounterFailure::MutationInProgress;
	if (EncounterState.HasApplied(Result.EncounterID)) return ECampaignEncounterFailure::AlreadyApplied;
	const ECampaignEncounterFailure Authority = ValidateCampaignAuthority();
	if (Authority != ECampaignEncounterFailure::None) return Authority;
	const FCampaignEncounterPlan Plan = EncounterState.GetPlan();
	if (!EncounterState.HasPendingEncounter()) return ECampaignEncounterFailure::NoEncounter;
	if (FactionManager->GetPlayerFaction() != Plan.PlayerFaction || !FactionManager->AreAtWar(Plan.Attacker, Plan.Defender))
		return ECampaignEncounterFailure::StaleDeployment;
	if (!ValidateDeploymentContent(Plan)) return ECampaignEncounterFailure::InvalidContent;
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> TerritoryGuard(WorldMapManager->bCampaignTransactionActive, true);
	FTerritoryOwnershipChangeResult Ownership;
	Ownership.TerritoryID = Plan.TargetTerritoryID;
	Ownership.PreviousOwner = WorldMapManager->GetTerritoryOwner(Plan.TargetTerritoryID);
	const ECampaignEncounterFailure Applied = EncounterState.Apply(Result, ArmyState, WorldMapManager->CampaignTerritoryState);
	if (Applied != ECampaignEncounterFailure::None) return Applied;
	Ownership.Failure = ETerritoryOwnershipFailure::None;
	Ownership.NewOwner = WorldMapManager->GetTerritoryOwner(Plan.TargetTerritoryID);
	Ownership.bOwnershipChanged = Ownership.NewOwner != Ownership.PreviousOwner;
	CheckAllVictoryConditions(); // Outcome is current before territory/army observers run.
	WorldMapManager->PublishOwnershipChange(Ownership);
	if (!Ownership.bOwnershipChanged) BroadcastObjectiveProgress();
	OnCampaignArmiesChanged.Broadcast();
	OnCampaignEncounterChanged.Broadcast();
	return ECampaignEncounterFailure::None;
}

bool ACampaignManager::PrepareTrainingReservation(FTrainingReservation& Reservation) const
{
	if (bCampaignTransferPending) return false;
	const auto* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (Session && Session->HasSession() && Session->GetPhase() == ECampaignTransferPhase::Strategic) return false;
	if (!EncounterState.HasPendingEncounter()) return !bEncounterMutationActive;
	if (ValidateCampaignAuthority() != ECampaignEncounterFailure::None) return false;
	const ARomanEmpireGameMode* Mode = GetWorld()->GetAuthGameMode<ARomanEmpireGameMode>();
	const UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	const FCampaignEncounterPlan Plan = EncounterState.GetPlan();
	if (!Mode || Mode->GetCurrentGameplayMode() != EGameplayMode::Tactical || !Battle || Battle->IsResolved() ||
		Battle->GetResult().EncounterID != Plan.EncounterID || !ValidateDeploymentContent(Plan) ||
		EncounterState.ValidateDeployment(ArmyState, WorldMapManager->CampaignTerritoryState) != ECampaignEncounterFailure::None ||
		FactionManager->GetPlayerFaction() != Plan.Attacker || !FactionManager->AreAtWar(Plan.Attacker, Plan.Defender)) return false;
	FGuid Destination;
	for (const FCampaignArmy& Army : Plan.Armies)
	{
		if (Army.Faction != Reservation.PayingFaction) continue;
		if (Destination.IsValid()) return false; // No implicit choice among multiple defending armies.
		Destination = Army.ArmyID;
	}
	if (!Destination.IsValid()) return false;
	Reservation.EncounterID = Plan.EncounterID;
	Reservation.CampaignArmyID = Destination;
	return true;
}

bool ACampaignManager::EnrollTrainedUnit(ABarracks* Producer, AUnitBase* Unit, const FTrainingReservation& Reservation)
{
	if (ValidateCampaignAuthority() != ECampaignEncounterFailure::None || !IsValid(Producer) || !IsValid(Unit) ||
		Producer->GetWorld() != GetWorld() || Producer->IsActorBeingDestroyed() || !Producer->HasActorBegunPlay() ||
		!Producer->IsComplete() || !Producer->IsTraining() || !Producer->bIsUpdatingProduction ||
		Producer->GetOwnerFaction() != Reservation.PayingFaction || Unit->GetOwnerFaction() != Reservation.PayingFaction ||
		Unit->GetUnitDefinitionID() != Reservation.UnitID || !Reservation.ReceiptID.IsValid()) return false;
	const FTrainingReservation Current = Producer->GetCurrentTrainingReservation();
	if (!FTrainingReservation::StaticStruct()->CompareScriptStruct(&Current, &Reservation, 0)) return false;
	FTrainingReservation Validated = Reservation;
	if (!PrepareTrainingReservation(Validated) || Validated.EncounterID != Reservation.EncounterID ||
		Validated.CampaignArmyID != Reservation.CampaignArmyID || !Reservation.EncounterID.IsValid()) return false;
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* Definition = Registry ? Registry->FindUnit(Reservation.UnitID) : nullptr;
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(Reservation.PayingFaction) : nullptr;
	if (!Definition || !Faction || !Definition->UnitClass.Get() || !Unit->IsA(Definition->UnitClass.Get()) ||
		!Definition->AvailableToFactions.Contains(Faction->Definition.FactionID)) return false;
	UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	TGuardValue<bool> CampaignGuard(bEncounterMutationActive, true);
	TGuardValue<bool> TerritoryGuard(WorldMapManager->bCampaignTransactionActive, true);
	FTacticalBattleState PreparedBattle;
	FGuid SquadID;
	if (!Battle->bCampaignDeployment || !Battle->PrepareReinforcement(Unit, PreparedBattle, SquadID)) return false;
	FCampaignEncounterState PreparedEncounter = EncounterState;
	FCampaignArmyState PreparedArmies = ArmyState;
	FCampaignSquadBinding Binding;
	Binding.SquadID = SquadID; Binding.ArmyID = Reservation.CampaignArmyID; Binding.UnitID = Reservation.UnitID; Binding.UnitCount = 1;
	if (PreparedEncounter.EnrollReinforcement(Reservation.EncounterID, Reservation.ReceiptID, Binding,
		PreparedArmies, WorldMapManager->CampaignTerritoryState) != ECampaignEncounterFailure::None) return false;
	// No observer runs until all three authorities and tracking are committed.
	ArmyState = MoveTemp(PreparedArmies);
	EncounterState = MoveTemp(PreparedEncounter);
	Battle->CommitReinforcement(Unit, SquadID, MoveTemp(PreparedBattle));
	Battle->BroadcastResult();
	OnCampaignArmiesChanged.Broadcast();
	OnCampaignEncounterChanged.Broadcast();
	return true;
}

FCampaignEncounterPlan ACampaignManager::GetCampaignEncounterPlan() const
{
	return HasActorBegunPlay() && !IsActorBeingDestroyed() ? EncounterState.GetPlan() : FCampaignEncounterPlan();
}

bool ACampaignManager::HasAppliedCampaignEncounter(FGuid EncounterID) const
{
	return HasActorBegunPlay() && !IsActorBeingDestroyed() && EncounterState.HasApplied(EncounterID);
}

void ACampaignManager::StartNewCampaign(EFactionID PlayerFaction)
{
	if (bCampaignTransferPending || bEncounterMutationActive) return;
	if (bIsProcessingTurn || bIsStartingCampaign)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot restart the campaign from an active campaign notification"));
		return;
	}
	TGuardValue<bool> StartingCampaignGuard(bIsStartingCampaign, true);
	FCampaignArmyState StartingArmies;
	if (!BuildStartingArmies(StartingArmies))
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot initialize campaign armies from current content/territories"));
		return;
	}
	ArmyState = MoveTemp(StartingArmies);
	EncounterState.Reset();
	bAwaitingAITacticalTravel = false;

	CurrentTurn = 1;
	bCampaignActive = true;
	LastAutosaveRequestedTurn = 0;
	CampaignOutcome = ECampaignOutcome::InProgress;
	ResolvedVictoryCondition = EVictoryCondition::Conquest;
	TurnRandomStream.Initialize(PrototypeTurnRandomSeed);

	if (FactionManager)
	{
		FactionManager->SetPlayerFaction(PlayerFaction);
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("New campaign started with faction %d"), 
		static_cast<int32>(PlayerFaction));
	BroadcastObjectiveProgress();
	OnCampaignStarted.Broadcast(FactionManager ? FactionManager->GetPlayerFaction() : PlayerFaction);
	OnCampaignArmiesChanged.Broadcast();
	OnCampaignEncounterChanged.Broadcast();
}

FCampaignTurnResult ACampaignManager::ProcessTurn()
{
	FCampaignTurnResult Result;
	Result.StartingTurn = CurrentTurn;
	Result.CompletedTurn = CurrentTurn;
	if (bIsProcessingTurn || bIsStartingCampaign)
	{
		Result.Failure = ECampaignTurnFailure::TurnAlreadyProcessing; return Result;
	}
	if (bCampaignTransferPending || bEncounterMutationActive)
	{
		Result.Failure = ECampaignTurnFailure::EncounterPending;
		return Result;
	}
	if (bAwaitingAITacticalTravel)
	{
		Result.bWasAITacticalTravelRetry = true; Result.FailedStep = ECampaignTurnStep::AITacticalTravel;
		Result.Failure = DispatchQueuedAIEncounter(); return Result;
	}
	if (EncounterState.HasPendingEncounter()) { Result.Failure = ECampaignTurnFailure::EncounterPending; return Result; }
	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (Battle && Battle->IsResolved())
	{
		Result.Failure = ECampaignTurnFailure::EncounterResolved;
		return Result;
	}
	{
	TGuardValue<bool> ProcessingTurnGuard(bIsProcessingTurn, true);

	for (const ECampaignTurnStep Step : RomanEmpireCampaign::GetCanonicalTurnPipeline())
	{
		Result.FailedStep = Step;
		OnTurnStepStarted.Broadcast(CurrentTurn, Step);
		UE_LOG(LogRomanEmpire, Verbose, TEXT("Turn %d step: %s"),
			CurrentTurn, RomanEmpireCampaign::GetTurnStepName(Step));

		const ECampaignTurnFailure Failure = ExecuteTurnStep(Step);
		if (Failure != ECampaignTurnFailure::None)
		{
			Result.Failure = Failure;
			Result.CompletedTurn = CurrentTurn;
			UE_LOG(LogRomanEmpire, Warning, TEXT("Turn %d stopped at %s (%s)"),
				Result.StartingTurn,
				RomanEmpireCampaign::GetTurnStepName(Step),
				RomanEmpireCampaign::GetTurnFailureName(Failure));
			return Result;
		}

		Result.CompletedSteps.Add(Step);
		Result.CompletedTurn = CurrentTurn;
	}
	} // Release the turn guard BEFORE capturing or dispatching selected-map travel.

	Result.Failure = ECampaignTurnFailure::None;
	if (bAwaitingAITacticalTravel)
	{
		Result.Failure = DispatchQueuedAIEncounter();
		if (Result.Failure != ECampaignTurnFailure::None) Result.FailedStep = ECampaignTurnStep::AITacticalTravel;
	}
	UE_LOG(LogRomanEmpire, Log, TEXT("Turn %d completed; turn %d is now active"),
		Result.StartingTurn, Result.CompletedTurn);
	return Result;
}

ECampaignTurnFailure ACampaignManager::ExecuteTurnStep(ECampaignTurnStep Step)
{
	TGuardValue<ECampaignTurnStep> StepGuard(ExecutingTurnStep, Step);
	switch (Step)
	{
	case ECampaignTurnStep::ValidateRequest:
		return ValidateTurnRequest();
	case ECampaignTurnStep::Production:
		return ProcessResourceProduction();
	case ECampaignTurnStep::Upkeep:
		return ProcessUpkeep();
	case ECampaignTurnStep::AIActions:
		return ProcessAIFactions();
	case ECampaignTurnStep::ConflictResolution:
		return ResolveTurnConflicts();
	case ECampaignTurnStep::VictoryEvaluation:
		CheckAllVictoryConditions();
		CancelQueuedAIOnTerminalOutcome();
		return ECampaignTurnFailure::None;
	case ECampaignTurnStep::IncrementTurn:
		CurrentTurn++;
		if (CurrentTurn > MaxTurns && CampaignOutcome == ECampaignOutcome::InProgress)
		{
			UE_LOG(LogRomanEmpire, Log, TEXT("Max turns reached, campaign ending"));
			bCampaignActive = false;
			CampaignOutcome = ECampaignOutcome::Draw;
			BroadcastObjectiveProgress();
		}
		CancelQueuedAIOnTerminalOutcome();
		return ECampaignTurnFailure::None;
	case ECampaignTurnStep::BroadcastTurnCompleted:
		OnTurnProcessed.Broadcast(CurrentTurn);
		return ECampaignTurnFailure::None;
	case ECampaignTurnStep::Autosave:
		return RequestAutosave();
	default:
		return ECampaignTurnFailure::CampaignInactive;
	}
}

ECampaignTurnFailure ACampaignManager::ValidateTurnRequest() const
{
	if (!bCampaignActive) return ECampaignTurnFailure::CampaignInactive;
	if (!FactionManager) return ECampaignTurnFailure::MissingFactionManager;
	if (!WorldMapManager) return ECampaignTurnFailure::MissingWorldMapManager;
	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ProcessResourceProduction()
{
	if (!FactionManager) return ECampaignTurnFailure::MissingFactionManager;
	if (!WorldMapManager) return ECampaignTurnFailure::MissingWorldMapManager;

	// Dormant authored polities retain ownership but do not participate in slice economy.
	for (FName TerritoryID : WorldMapManager->GetTerritoryIDs())
	{
		const EFactionID TerritoryOwner = WorldMapManager->GetTerritoryOwner(TerritoryID);
		if (!FactionManager->IsFactionActive(TerritoryOwner))
		{
			continue;
		}

		FFactionResourceDelta Production;
		if (!WorldMapManager->TryGetTerritoryProduction(TerritoryID, Production))
		{
			return ECampaignTurnFailure::ResourceTransactionFailed;
		}
		const FFactionResourceTransactionResult Result =
			FactionManager->ApplyFactionResourceDelta(TerritoryOwner, Production);
		if (!Result.WasSuccessful())
		{
			UE_LOG(LogRomanEmpire, Error, TEXT("Production failed for faction %d (%s)"),
				static_cast<int32>(TerritoryOwner),
				RomanEmpireEconomy::GetTransactionFailureName(Result.Failure));
			return ECampaignTurnFailure::ResourceTransactionFailed;
		}
	}

	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ProcessUpkeep()
{
	// Upkeep rules are introduced with production gameplay; the ordered stage is explicit now.
	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ProcessAIFactions()
{
	if (!FactionManager)
	{
		return ECampaignTurnFailure::MissingFactionManager;
	}

	FactionManager->ProcessAITurns();
	const auto* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (Session && Session->HasSession()) return ApplyCampaignAIProposal();
	TurnRandomStream.Initialize(PrototypeTurnRandomSeed + CurrentTurn);

	// Prototype tactical AI runs once within the campaign AI stage.
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllUnits);
	for (AActor* Actor : AllUnits)
	{
		AUnitBase* Unit = Cast<AUnitBase>(Actor);
		if (!Unit || !Unit->IsAlive() || !FactionManager->IsFactionActive(Unit->GetOwnerFaction()) ||
			Unit->GetOwnerFaction() == FactionManager->GetPlayerFaction())
		{
			continue;
		}

		const FVector CurrentPosition = Unit->GetActorLocation();
		FVector DirectionToRome = -CurrentPosition;
		DirectionToRome.Z = 0.0f;
		if (DirectionToRome.SizeSquared() > 10000.0f)
		{
			DirectionToRome.Normalize();
			const float AdvanceDistance = TurnRandomStream.FRandRange(500.0f, 2000.0f);
			FVector Destination = CurrentPosition + DirectionToRome * AdvanceDistance;
			Destination.Z = CurrentPosition.Z;
			Unit->CommandMoveTo(Destination);
		}
	}

	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ResolveTurnConflicts()
{
	// Strategic conflict resolution rules are introduced with canonical territory state in M2.3.
	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::RequestAutosave()
{
	LastAutosaveRequestedTurn = CurrentTurn;
	OnCampaignAutosaveRequested.Broadcast(CurrentTurn);
	if (auto* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>() : nullptr)
		Save->AutosaveFromCampaign(this); // I/O failure is surfaced by persistence, never rolls back/repeats a committed turn.
	return ECampaignTurnFailure::None;
}

void ACampaignManager::CheckAllVictoryConditions()
{
	if (!FactionManager || CampaignOutcome != ECampaignOutcome::InProgress)
	{
		return;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();

	// Check conquest victory
	if (CheckVictoryCondition(PlayerFaction, EVictoryCondition::Conquest))
	{
		bCampaignActive = false;
		CampaignOutcome = ECampaignOutcome::Victory;
		ResolvedVictoryCondition = EVictoryCondition::Conquest;
		BroadcastObjectiveProgress();
		OnVictory.Broadcast(PlayerFaction, ResolvedVictoryCondition);
		UE_LOG(LogRomanEmpire, Log, TEXT("Player achieved conquest victory!"));
		return;
	}

	// Check economic victory
	if (CheckVictoryCondition(PlayerFaction, EVictoryCondition::Economic))
	{
		bCampaignActive = false;
		CampaignOutcome = ECampaignOutcome::Victory;
		ResolvedVictoryCondition = EVictoryCondition::Economic;
		BroadcastObjectiveProgress();
		OnVictory.Broadcast(PlayerFaction, ResolvedVictoryCondition);
		UE_LOG(LogRomanEmpire, Log, TEXT("Player achieved economic victory!"));
		return;
	}

	// Check if player lost all territories
	if (IsPlayerDefeatConditionMet())
	{
		bCampaignActive = false;
		CampaignOutcome = ECampaignOutcome::Defeat;
		BroadcastObjectiveProgress();
		OnDefeat.Broadcast(PlayerFaction);
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
			const int32 TerritoryCount = WorldMapManager->GetFactionTerritoryCount(FactionID);
			return TerritoryCount >= ConquestVictoryTerritories;
		}

		case EVictoryCondition::Economic:
		{
			const FFactionResourceAmounts Resources = FactionManager->GetFactionResources(FactionID);
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
	return CampaignOutcome == ECampaignOutcome::Victory;
}

bool ACampaignManager::HasPlayerLost() const
{
	return CampaignOutcome == ECampaignOutcome::Defeat;
}

bool ACampaignManager::IsPlayerDefeatConditionMet() const
{
	if (!FactionManager || !WorldMapManager)
	{
		return false;
	}

	const EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	const int32 TerritoryCount = WorldMapManager->GetFactionTerritoryCount(PlayerFaction);
	
	// Lost if no territories
	return TerritoryCount == 0;
}

FCampaignObjectiveState ACampaignManager::GetObjectiveState() const
{
	FCampaignObjectiveState State;
	State.RequiredTerritories = ConquestVictoryTerritories;
	State.RequiredGold = EconomicVictoryGold;
	State.Outcome = CampaignOutcome;

	if (!FactionManager)
	{
		return State;
	}

	const EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	State.CurrentGold = FactionManager->GetFactionResources(PlayerFaction).Gold;
	if (WorldMapManager)
	{
		State.ControlledTerritories = WorldMapManager->GetFactionTerritoryCount(PlayerFaction);
	}
	return State;
}

void ACampaignManager::BroadcastObjectiveProgress()
{
	OnCampaignObjectivesChanged.Broadcast(GetObjectiveState());
}

void ACampaignManager::HandleFactionResourcesChanged(
	EFactionID FactionID,
	FFactionResourceAmounts NewBalance)
{
	(void)NewBalance;
	if (FactionManager && FactionID == FactionManager->GetPlayerFaction())
	{
		BroadcastObjectiveProgress();
	}
}

void ACampaignManager::HandleTerritoryOwnerChanged(
	FName TerritoryID,
	EFactionID PreviousOwner,
	EFactionID NewOwner)
{
	(void)TerritoryID;
	(void)PreviousOwner;
	(void)NewOwner;
	BroadcastObjectiveProgress();
}

void ACampaignManager::HandlePlayerFactionChanged(EFactionID PlayerFaction)
{
	(void)PlayerFaction;
	BroadcastObjectiveProgress();
}

void ACampaignManager::SaveCampaign(const FString& SaveName)
{
	(void)SaveName; // Legacy entry point uses the fixed manual domain, never a caller-provided path.
	if (auto* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>() : nullptr) Save->SaveCampaign();
}

void ACampaignManager::LoadCampaign(const FString& SaveName)
{
	(void)SaveName;
	if (auto* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>() : nullptr) Save->LoadCampaign();
}

