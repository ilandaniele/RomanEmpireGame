// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/DiplomacyState.h"
#include "GameFramework/Actor.h"
#include "Faction/FactionData.h"
#include "FactionManager.generated.h"

/**
 * Manages all factions in the game
 * Handles faction initialization, resources, diplomacy, and AI behavior
 */
UCLASS()
class ROMANEMPIREGAME_API AFactionManager : public AActor
{
	GENERATED_BODY()

public:
	AFactionManager();

	virtual void BeginPlay() override;

	// Faction access
	UFUNCTION(BlueprintPure, Category = "Faction")
	FFactionInfo GetFactionInfo(EFactionID FactionID) const;

	UFUNCTION(BlueprintPure, Category = "Faction")
	FFactionResourceAmounts GetFactionResources(EFactionID FactionID) const;

	UFUNCTION(BlueprintCallable, Category = "Faction|Resources")
	FFactionResourceTransactionResult ApplyFactionResourceDelta(
		EFactionID FactionID,
		const FFactionResourceDelta& Delta);

	UFUNCTION(BlueprintCallable, Category = "Faction|Resources")
	FFactionResourceTransactionResult TrySpendFactionResources(
		EFactionID FactionID,
		const FFactionResourceAmounts& Cost);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnFactionResourcesChanged,
		EFactionID, FactionID,
		FFactionResourceAmounts, NewBalance);

	UPROPERTY(BlueprintAssignable, Category = "Faction|Resources")
	FOnFactionResourcesChanged OnFactionResourcesChanged;

	// Diplomacy
	UFUNCTION(BlueprintPure, Category = "Faction|Diplomacy")
	EDiplomaticStatus GetDiplomaticStatus(EFactionID Faction1, EFactionID Faction2) const;

	UFUNCTION(BlueprintCallable, Category = "Faction|Diplomacy")
	FDiplomaticChangeResult SetDiplomaticStatus(
		EFactionID Faction1,
		EFactionID Faction2,
		EDiplomaticStatus NewStatus);

	UFUNCTION(BlueprintPure, Category = "Faction|Diplomacy")
	bool AreAtWar(EFactionID Faction1, EFactionID Faction2) const;

	UFUNCTION(BlueprintPure, Category = "Faction|Diplomacy")
	bool AreAllied(EFactionID Faction1, EFactionID Faction2) const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnDiplomaticStatusChanged,
		FDiplomaticPair, Pair,
		EDiplomaticStatus, NewStatus);

	UPROPERTY(BlueprintAssignable, Category = "Faction|Diplomacy")
	FOnDiplomaticStatusChanged OnDiplomaticStatusChanged;

	// Player faction
	UFUNCTION(BlueprintPure, Category = "Faction")
	EFactionID GetPlayerFaction() const { return PlayerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Faction")
	void SetPlayerFaction(EFactionID NewFaction);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerFactionChanged, EFactionID, NewFaction);

	UPROPERTY(BlueprintAssignable, Category = "Faction|Events")
	FOnPlayerFactionChanged OnPlayerFactionChanged;

	// AI turn processing
	UFUNCTION(BlueprintCallable, Category = "Faction|AI")
	void ProcessAITurns();

protected:
	// Faction data
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction|Setup")
	TMap<EFactionID, FFactionInfo> FactionInfoMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction|State")
	TMap<EFactionID, FFactionResourceAmounts> FactionResourcesMap;

	// Canonical normalized diplomacy state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction|Diplomacy")
	FDiplomacyState DiplomacyState;

	// Player's faction
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	EFactionID PlayerFaction;

private:
	bool InitializeFactionsFromContent();
	void InitializeDefaultDiplomacy();
	void ProcessAIFactionTurn(EFactionID FactionID);
};

