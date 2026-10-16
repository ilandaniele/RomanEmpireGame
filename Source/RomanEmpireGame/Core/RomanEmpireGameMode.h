// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/ZoomConfiguration.h"
#include "Campaign/CampaignTurn.h"
#include "Campaign/CampaignEncounterState.h"
#include "Core/GameplayMode.h"
#include "Faction/FactionData.h"
#include "GameFramework/GameModeBase.h"
#include "Math/RandomStream.h"
#include "RomanEmpireGameMode.generated.h"

class AFactionManager;
class AWorldMapManager;
class ACampaignManager;
class AUnitBase;

/**
 * Main game mode for Roman Empire Game
 * Manages transitions between strategic, tactical, and FPS modes
 */
UCLASS()
class ROMANEMPIREGAME_API ARomanEmpireGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARomanEmpireGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Battle")
	bool CanRestartEncounter() const;
	UFUNCTION(BlueprintCallable, Category = "Battle")
	bool RestartEncounter();

	// Explicit gameplay mode. Possession is never inferred from a raw zoom value.
	UFUNCTION(BlueprintCallable, Category = "Gameplay Mode")
	void SetGameplayMode(EGameplayMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Gameplay Mode")
	EGameplayMode GetCurrentGameplayMode() const { return CurrentGameplayMode; }

	// Applies exploration-mode policy from a semantic camera band, never from a float threshold.
	UFUNCTION(BlueprintCallable, Category = "Gameplay Mode")
	void SetExplorationModeForZoomLevel(EZoomLevel ZoomLevel);

	// Execute the canonical campaign turn pipeline.
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	FCampaignTurnResult EndTurn();

	// Manager getters
	UFUNCTION(BlueprintPure, Category = "Managers")
	AFactionManager* GetFactionManager() const { return FactionManager; }

	UFUNCTION(BlueprintPure, Category = "Managers")
	AWorldMapManager* GetWorldMapManager() const { return WorldMapManager; }

	UFUNCTION(BlueprintPure, Category = "Managers")
	ACampaignManager* GetCampaignManager() const { return CampaignManager; }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetCurrentTurn() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnGameplayModeChanged,
		EGameplayMode, OldMode,
		EGameplayMode, NewMode);

	UPROPERTY(BlueprintAssignable, Category = "Gameplay Mode")
	FOnGameplayModeChanged OnGameplayModeChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameplayInitialized);

	UPROPERTY(BlueprintAssignable, Category = "Managers")
	FOnGameplayInitialized OnGameplayInitialized;

	UFUNCTION(BlueprintPure, Category = "Managers")
	bool IsGameplayInitialized() const { return bGameplayInitialized; }

protected:
	/** Sandbox fallback. Strategic/session worlds suppress prototype unit seeding. */
	UPROPERTY(EditDefaultsOnly, Category = "Campaign|Bootstrap")
	bool bStartWithPrototypeEncounter = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay Mode")
	EGameplayMode CurrentGameplayMode;

	// Manager references
	UPROPERTY()
	TObjectPtr<AFactionManager> FactionManager;

	UPROPERTY()
	TObjectPtr<AWorldMapManager> WorldMapManager;

	UPROPERTY()
	TObjectPtr<ACampaignManager> CampaignManager;

private:
	friend class ACampaignManager;
	bool MaterializeCampaignDeployment(FCampaignEncounterState& Deployment, TArray<AUnitBase*>& SpawnedUnits);
	bool bGameplayInitialized = false;
	bool bEncounterTravelPending = false;
	static constexpr int32 PrototypeRandomSeed = 1337;
	FRandomStream PrototypeRandomStream;

	void InitializeManagers();
	AUnitBase* SpawnUnitFromDefinition(
		FName UnitID,
		EFactionID OwnerFaction,
		const FTransform& SpawnTransform);
	void SpawnInitialUnits();
	void SpawnInitialBuildings();
	void SpawnWorldTerrain();
};

