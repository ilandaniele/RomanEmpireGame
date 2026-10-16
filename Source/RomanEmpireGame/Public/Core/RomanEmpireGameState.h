// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Campaign/CampaignArmyState.h"
#include "Faction/DiplomacyState.h"
#include "Campaign/CampaignEncounterState.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Campaign/TerritoryPresentation.h"
#include "Battle/TacticalBattleResult.h"
#include "Core/GameplayMode.h"
#include "Faction/FactionData.h"
#include "GameFramework/GameStateBase.h"
#include "RomanEmpireGameState.generated.h"

class ACampaignManager;
class AFactionManager;
class ARomanEmpireGameMode;
class AWorldMapManager;
class UTacticalBattleSubsystem;

/** Public single-player read/event contract. Values stay owned by their canonical
 * managers; this actor holds no wallet, turn, mode or outcome copy and accepts no commands.
 * Event payloads describe notifications, not a monotonic snapshot: observers may
 * commit nested commands. Presentation consumers should re-query current values.
 * This is not a replicated multiplayer read model. */
UCLASS()
class ROMANEMPIREGAME_API ARomanEmpireGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ARomanEmpireGameState();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Game State")
	bool IsGameplayInitialized() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	EGameplayMode GetCurrentGameplayMode() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	int32 GetCurrentTurn() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	EFactionID GetPlayerFaction() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	FFactionResourceAmounts GetFactionResources(EFactionID FactionID) const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	FCampaignObjectiveState GetObjectiveState() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	bool IsCampaignActive() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	int32 GetFactionTerritoryCount(EFactionID FactionID) const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	EVictoryCondition GetResolvedVictoryCondition() const;
	UFUNCTION(BlueprintPure, Category = "Game State")
	FTacticalBattleResult GetTacticalBattleResult() const;
	UFUNCTION(BlueprintPure, Category = "Game State|Armies")
	TArray<FCampaignArmy> GetCampaignArmies() const;
	UFUNCTION(BlueprintPure, Category = "Game State|Armies")
	bool TryGetCampaignArmy(FGuid ArmyID, FCampaignArmy& OutArmy) const;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPublicCampaignArmiesChanged);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Armies")
	FOnPublicCampaignArmiesChanged OnCampaignArmiesChanged;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPublicDiplomacyChanged, FDiplomaticPair, Pair, EDiplomaticStatus, Status);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Diplomacy") FOnPublicDiplomacyChanged OnDiplomacyChanged;
	UFUNCTION(BlueprintPure, Category = "Game State|Encounter")
	FCampaignEncounterPlan GetCampaignEncounterPlan() const;
	UFUNCTION(BlueprintPure, Category = "Game State|Encounter")
	bool HasAppliedCampaignEncounter(FGuid EncounterID) const;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPublicCampaignEncounterChanged);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Encounter")
	FOnPublicCampaignEncounterChanged OnCampaignEncounterChanged;

	/** Fresh, lexical-ID-ordered values. Empty before readiness or after teardown. */
	UFUNCTION(BlueprintPure, Category = "Game State|Territories")
	TArray<FTerritoryPresentationSnapshot> GetTerritorySnapshots() const;
	/** Unknown/not-ready returns false and resets OutSnapshot; None owner is valid. */
	UFUNCTION(BlueprintPure, Category = "Game State|Territories")
	bool TryGetTerritorySnapshot(FName TerritoryID, FTerritoryPresentationSnapshot& OutSnapshot) const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPublicGameplayInitialized);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPublicGameplayModeChanged, EGameplayMode, OldMode, EGameplayMode, NewMode);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPublicResourcesChanged, EFactionID, FactionID, FFactionResourceAmounts, NewBalance);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicPlayerFactionChanged, EFactionID, NewFaction);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPublicTurnStepStarted, int32, TurnNumber, ECampaignTurnStep, Step);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicTurnProcessed, int32, TurnNumber);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicCampaignStarted, EFactionID, PlayerFaction);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicObjectivesChanged, FCampaignObjectiveState, ObjectiveState);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPublicVictory, EFactionID, WinningFaction, EVictoryCondition, Condition);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicDefeat, EFactionID, LosingFaction);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicTacticalBattleChanged, FTacticalBattleResult, Result);
	/** Invalidation, not an ordered transaction log. Re-query after nested commands.
	 * PreviousOwner describes the source notification; NewOwner is current at relay time. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPublicTerritoryOwnerChanged,
		FName, TerritoryID, EFactionID, PreviousOwner, EFactionID, NewOwner);

	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicGameplayInitialized OnGameplayInitialized;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicGameplayModeChanged OnGameplayModeChanged;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicResourcesChanged OnFactionResourcesChanged;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicPlayerFactionChanged OnPlayerFactionChanged;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicTurnStepStarted OnTurnStepStarted;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicTurnProcessed OnTurnProcessed;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicCampaignStarted OnCampaignStarted;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicObjectivesChanged OnCampaignObjectivesChanged;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicVictory OnVictory;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicDefeat OnDefeat;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicTacticalBattleChanged OnTacticalBattleChanged;
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicTerritoryOwnerChanged OnTerritoryOwnerChanged;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPublicTerritorySettlementChanged, FName, TerritoryID);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPublicTerritorySettlementChanged OnTerritorySettlementChanged;

private:
	UFUNCTION() void HandleDiplomacyChanged(FDiplomaticPair Pair, EDiplomaticStatus Status);
	UPROPERTY(Transient)
	TWeakObjectPtr<ARomanEmpireGameMode> Rules;
	UPROPERTY(Transient)
	TWeakObjectPtr<AFactionManager> Factions;
	UPROPERTY(Transient)
	TWeakObjectPtr<ACampaignManager> Campaign;
	UPROPERTY(Transient)
	TWeakObjectPtr<AWorldMapManager> WorldMap;
	UPROPERTY(Transient)
	TWeakObjectPtr<UTacticalBattleSubsystem> TacticalBattle;
	bool bObserversBound = false;

	UFUNCTION()
	void HandleGameplayInitialized();
	UFUNCTION()
	void HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode);
	UFUNCTION()
	void HandleResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance);
	UFUNCTION()
	void HandlePlayerFactionChanged(EFactionID NewFaction);
	UFUNCTION()
	void HandleTurnStepStarted(int32 TurnNumber, ECampaignTurnStep Step);
	UFUNCTION()
	void HandleTurnProcessed(int32 TurnNumber);
	UFUNCTION()
	void HandleCampaignStarted(EFactionID PlayerFaction);
	UFUNCTION()
	void HandleObjectivesChanged(FCampaignObjectiveState ObjectiveState);
	UFUNCTION()
	void HandleVictory(EFactionID WinningFaction, EVictoryCondition Condition);
	UFUNCTION()
	void HandleDefeat(EFactionID LosingFaction);
	UFUNCTION()
	void HandleTacticalBattleChanged(FTacticalBattleResult Result);
	UFUNCTION()
	void HandleTerritoryOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner);
	UFUNCTION()
	void HandleTerritorySettlementChanged(FName TerritoryID);
	UFUNCTION()
	void HandleCampaignArmiesChanged();
	UFUNCTION()
	void HandleCampaignEncounterChanged();
};
