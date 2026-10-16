// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Campaign/CampaignArmyState.h"
#include "Campaign/CampaignAIPlanning.h"
#include "Campaign/CampaignEncounterState.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Campaign/CampaignTransfer.h"
#include "GameFramework/Actor.h"
#include "Faction/FactionData.h"
#include "Math/RandomStream.h"
#include "CampaignManager.generated.h"

class AFactionManager;
class AWorldMapManager;
class ABarracks;
class AUnitBase;
struct FTrainingReservation;

/**
 * Manages the campaign/strategic layer of the game
 * Handles turn progression, victory conditions, and game state
 */
UCLASS()
class ROMANEMPIREGAME_API ACampaignManager : public AActor
{
	GENERATED_BODY()

public:
	ACampaignManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Turn management
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	FCampaignTurnResult ProcessTurn();

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetCurrentTurn() const { return CurrentTurn; }

	// Victory conditions
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool CheckVictoryCondition(EFactionID FactionID, EVictoryCondition Condition) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool HasPlayerWon() const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool HasPlayerLost() const;

	UFUNCTION(BlueprintPure, Category = "Campaign|Objectives")
	FCampaignObjectiveState GetObjectiveState() const;

	UFUNCTION(BlueprintPure, Category = "Campaign|Objectives")
	ECampaignOutcome GetCampaignOutcome() const { return CampaignOutcome; }

	UFUNCTION(BlueprintPure, Category = "Campaign|Objectives")
	EVictoryCondition GetResolvedVictoryCondition() const { return ResolvedVictoryCondition; }

	// Game state
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void StartNewCampaign(EFactionID PlayerFaction);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsCampaignActive() const { return bCampaignActive; }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetLastAutosaveRequestedTurn() const { return LastAutosaveRequestedTurn; }

	UFUNCTION(BlueprintPure, Category = "Campaign|Armies")
	TArray<FCampaignArmy> GetCampaignArmies() const;
	UFUNCTION(BlueprintPure, Category = "Campaign|Armies")
	bool TryGetCampaignArmy(FGuid ArmyID, FCampaignArmy& OutArmy) const;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampaignArmiesChanged);
	UPROPERTY(BlueprintAssignable, Category = "Campaign|Armies")
	FOnCampaignArmiesChanged OnCampaignArmiesChanged;

	UFUNCTION(BlueprintCallable, Category = "Campaign|Armies")
	FCampaignArmyMutationResult MoveCampaignArmy(FGuid ArmyID, FName TargetTerritoryID);
	/** Strategic quantities only. Requires a ready Strategic session; no actor is spawned. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Armies")
	FCampaignArmyMutationResult AssignCampaignArmyUnits(FGuid SourceArmyID, FGuid DestinationArmyID, FName UnitID, int32 Count);
	UFUNCTION(BlueprintPure, Category = "Campaign|Armies")
	FCampaignRecruitmentQuote GetCampaignRecruitmentQuote(FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID) const;
	/** Invalid destination requests a fresh army at the selected owned settlement. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Armies")
	FCampaignArmyMutationResult RecruitCampaignUnits(FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID);
	/** Read-only proposal for the slice AI. Execution belongs to the canonical turn pipeline. */
	UFUNCTION(BlueprintPure, Category = "Campaign|AI")
	FCampaignAIProposal GetCampaignAIProposal() const;
	UFUNCTION(BlueprintPure, Category = "Campaign|AI")
	bool IsAwaitingAITacticalTravel() const { return bAwaitingAITacticalTravel; }
	UFUNCTION(BlueprintCallable, Category = "Campaign|Encounter")
	ECampaignEncounterFailure BeginCampaignEncounter(FGuid AttackingArmyID, FName TargetTerritoryID);
	/** Reads the canonical subsystem result; callers cannot submit invented casualties. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Encounter")
	ECampaignEncounterFailure ApplyCurrentEncounterResult();
	UFUNCTION(BlueprintPure, Category = "Campaign|Encounter")
	FCampaignEncounterPlan GetCampaignEncounterPlan() const;
	UFUNCTION(BlueprintPure, Category = "Campaign|Encounter")
	bool HasAppliedCampaignEncounter(FGuid EncounterID) const;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampaignEncounterChanged);
	UPROPERTY(BlueprintAssignable, Category = "Campaign|Encounter")
	FOnCampaignEncounterChanged OnCampaignEncounterChanged;

	// Save/Load (placeholder)
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void SaveCampaign(const FString& SaveName);

	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void LoadCampaign(const FString& SaveName);

	// Public event surface consumed by UMG, controllers, and tests.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnTurnStepStarted,
		int32, TurnNumber,
		ECampaignTurnStep, Step);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnProcessed, int32, TurnNumber);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCampaignAutosaveRequested, int32, TurnNumber);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCampaignStarted, EFactionID, PlayerFaction);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
		FOnCampaignObjectivesChanged,
		FCampaignObjectiveState, ObjectiveState);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVictory, EFactionID, WinningFaction, EVictoryCondition, Condition);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefeat, EFactionID, LosingFaction);

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnTurnStepStarted OnTurnStepStarted;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnTurnProcessed OnTurnProcessed;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnCampaignAutosaveRequested OnCampaignAutosaveRequested;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnCampaignStarted OnCampaignStarted;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnCampaignObjectivesChanged OnCampaignObjectivesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnVictory OnVictory;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnDefeat OnDefeat;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign")
	int32 CurrentTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign")
	bool bCampaignActive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign")
	int32 MaxTurns;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign")
	int32 ConquestVictoryTerritories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign")
	int32 EconomicVictoryGold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign")
	int32 LastAutosaveRequestedTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Objectives")
	ECampaignOutcome CampaignOutcome;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Objectives")
	EVictoryCondition ResolvedVictoryCondition;

	// References
	UPROPERTY()
	TObjectPtr<AFactionManager> FactionManager;

	UPROPERTY()
	TObjectPtr<AWorldMapManager> WorldMapManager;

private:
	friend class UCampaignSessionSubsystem;
	friend class UCampaignSaveSubsystem;
	ECampaignTransferFailure CaptureForSave(FCampaignTransferSnapshot& Out, bool bAutosaveBoundary = false) const;
	ECampaignTransferFailure ValidateSavedCampaign(const FCampaignTransferSnapshot& Snapshot) const;
	ECampaignEncounterFailure PrepareEncounterForTravel(FGuid ArmyID, FName Target, FCampaignEncounterState& Out, bool bAIStage = false) const;
	ECampaignTransferFailure CaptureForTravel(ECampaignTransferPhase Phase, const FSoftObjectPath& Destination,
		const FSoftObjectPath& ReturnMap, const FCampaignEncounterState& PreparedEncounter, FCampaignTransferSnapshot& Out);
	ECampaignTransferFailure RestoreFromTravel(const FCampaignTransferSnapshot& Snapshot);
	void CancelTravelFreeze();
	void FreezeForTravel();
	bool bCampaignTransferPending = false;
	friend class ABarracks;
	bool PrepareTrainingReservation(FTrainingReservation& Reservation) const;
	bool EnrollTrainedUnit(ABarracks* Producer, AUnitBase* Unit, const FTrainingReservation& Reservation);
	UPROPERTY()
	FCampaignArmyState ArmyState;
	UPROPERTY()
	FCampaignEncounterState EncounterState;
	bool bEncounterMutationActive = false;
	ECampaignEncounterFailure ValidateCampaignAuthority(bool bAllowResolvedCampaign = false, bool bAIStage = false, bool bAutosaveStage = false) const;
	FCampaignRecruitmentQuote QuoteRecruitmentForFaction(EFactionID Faction, FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID) const;
	FCampaignArmyMutationResult RecruitUnitsForFaction(EFactionID Faction, FName TerritoryID, FName UnitID, int32 Count, FGuid DestinationArmyID);
	ECampaignTurnFailure ApplyCampaignAIProposal();
	ECampaignTurnFailure DispatchQueuedAIEncounter();
	void CancelQueuedAIOnTerminalOutcome();
	bool ValidateQueuedAIEncounter(FCampaignEncounterState& Out) const;
	UPROPERTY()
	bool bAwaitingAITacticalTravel = false;
	ECampaignTurnStep ExecutingTurnStep = ECampaignTurnStep::ValidateRequest;
	ECampaignArmyFailure ValidateStrategicArmyAction(bool bReadOnly = false) const;
	bool ValidateDeploymentContent(const FCampaignEncounterPlan& Plan) const;
	bool BuildStartingArmies(FCampaignArmyState& OutState) const;
	static constexpr int32 PrototypeTurnRandomSeed = 1337;
	FRandomStream TurnRandomStream;
	bool bIsProcessingTurn = false;
	bool bIsStartingCampaign = false;

	ECampaignTurnFailure ExecuteTurnStep(ECampaignTurnStep Step);
	ECampaignTurnFailure ValidateTurnRequest() const;
	ECampaignTurnFailure ProcessResourceProduction();
	ECampaignTurnFailure ProcessUpkeep();
	ECampaignTurnFailure ProcessAIFactions();
	ECampaignTurnFailure ResolveTurnConflicts();
	ECampaignTurnFailure RequestAutosave();
	void CheckAllVictoryConditions();
	void BroadcastObjectiveProgress();
	bool IsPlayerDefeatConditionMet() const;

	UFUNCTION()
	void HandleFactionResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance);

	UFUNCTION()
	void HandlePlayerFactionChanged(EFactionID PlayerFaction);

	UFUNCTION()
	void HandleTerritoryOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner);
};

