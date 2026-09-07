// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "GameFramework/Actor.h"
#include "Faction/FactionData.h"
#include "Math/RandomStream.h"
#include "CampaignManager.generated.h"

class AFactionManager;
class AWorldMapManager;

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

