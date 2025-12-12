// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "CampaignManager.generated.h"

class AFactionManager;
class AWorldMapManager;

/**
 * Victory condition types
 */
UENUM(BlueprintType)
enum class EVictoryCondition : uint8
{
	Conquest		UMETA(DisplayName = "Conquest"),        // Control all territories
	Economic		UMETA(DisplayName = "Economic"),        // Accumulate wealth
	Military		UMETA(DisplayName = "Military"),        // Destroy all enemy armies
	Diplomatic		UMETA(DisplayName = "Diplomatic")       // Allied victory
};

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

	// Turn management
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ProcessTurn();

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetCurrentTurn() const { return CurrentTurn; }

	// Victory conditions
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool CheckVictoryCondition(EFactionID FactionID, EVictoryCondition Condition) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool HasPlayerWon() const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool HasPlayerLost() const;

	// Game state
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void StartNewCampaign(EFactionID PlayerFaction);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsCampaignActive() const { return bCampaignActive; }

	// Save/Load (placeholder)
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void SaveCampaign(const FString& SaveName);

	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void LoadCampaign(const FString& SaveName);

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

	// References
	UPROPERTY()
	AFactionManager* FactionManager;

	UPROPERTY()
	AWorldMapManager* WorldMapManager;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnProcessed, int32, TurnNumber);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVictory, EFactionID, WinningFaction, EVictoryCondition, Condition);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefeat, EFactionID, LosingFaction);

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnTurnProcessed OnTurnProcessed;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnVictory OnVictory;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnDefeat OnDefeat;

private:
	void ProcessResourceProduction();
	void ProcessAIFactions();
	void CheckAllVictoryConditions();
};
