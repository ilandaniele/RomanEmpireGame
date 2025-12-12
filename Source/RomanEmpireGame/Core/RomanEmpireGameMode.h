// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RomanEmpireGameMode.generated.h"

class AFactionManager;
class AWorldMapManager;
class ACampaignManager;

/**
 * Game phase representing the current mode of gameplay
 */
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	Strategic	UMETA(DisplayName = "Strategic Phase"),    // Campaign map, managing factions
	Tactical	UMETA(DisplayName = "Tactical Phase"),     // RTS mode with building/units
	Combat		UMETA(DisplayName = "Combat Phase"),       // Battle happening
	FirstPerson UMETA(DisplayName = "First Person Phase")  // FPS mode controlling a soldier
};

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
	virtual void Tick(float DeltaSeconds) override;

	// Phase management
	UFUNCTION(BlueprintCallable, Category = "Game Phase")
	void SetGamePhase(EGamePhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Game Phase")
	EGamePhase GetCurrentPhase() const { return CurrentPhase; }

	// Called when zoom level changes to update game phase
	UFUNCTION(BlueprintCallable, Category = "Game Phase")
	void OnZoomLevelChanged(float ZoomLevel);

	// End turn in strategic phase
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void EndTurn();

	// Manager getters
	UFUNCTION(BlueprintPure, Category = "Managers")
	AFactionManager* GetFactionManager() const { return FactionManager; }

	UFUNCTION(BlueprintPure, Category = "Managers")
	AWorldMapManager* GetWorldMapManager() const { return WorldMapManager; }

	UFUNCTION(BlueprintPure, Category = "Managers")
	ACampaignManager* GetCampaignManager() const { return CampaignManager; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Phase")
	EGamePhase CurrentPhase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign")
	int32 CurrentTurn;

	// Manager references
	UPROPERTY()
	AFactionManager* FactionManager;

	UPROPERTY()
	AWorldMapManager* WorldMapManager;

	UPROPERTY()
	ACampaignManager* CampaignManager;

	// Phase change delegate
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGamePhaseChanged, EGamePhase, OldPhase, EGamePhase, NewPhase);
	
	UPROPERTY(BlueprintAssignable, Category = "Game Phase")
	FOnGamePhaseChanged OnGamePhaseChanged;

private:
	void InitializeManagers();
	EGamePhase DeterminePhaseFromZoom(float ZoomLevel) const;
};
