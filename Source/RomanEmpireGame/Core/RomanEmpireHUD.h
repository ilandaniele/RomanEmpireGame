// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RomanEmpireHUD.generated.h"

class AUnitBase;
class ABuildingBase;
class ABarracks;
class ARomanEmpireGameMode;

/**
 * Main HUD - fully Canvas-based (no Widget Blueprint required)
 * Draws resource bar, phase indicator, building menu, unit panels,
 * health bars, building panel, and victory overlay.
 */
UCLASS()
class ROMANEMPIREGAME_API ARomanEmpireHUD : public AHUD
{
	GENERATED_BODY()

public:
	ARomanEmpireHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	// UI control
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateUnitSelection(const TArray<AUnitBase*>& SelectedUnits);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowFPSOverlay(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateResources(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnZoomLevelChanged(float ZoomLevel);

	// Building selection (called by player controller on click)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSelectedBuilding(ABuildingBase* Building) { HUDSelectedBuilding = Building; }

	// Kill tracking (called externally when enemy dies)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void IncrementKills() { EnemiesKilled++; }

	// Train button bounds (read by PlayerController for hit-testing)
	FVector2D TrainButtonMin;
	FVector2D TrainButtonMax;

protected:
	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|State")
	bool bBuildingMenuVisible;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|State")
	bool bFPSMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|State")
	float CurrentZoomLevel;

	// Displayed resource values (updated via UpdateResources)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Resources")
	int32 DisplayGold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Resources")
	int32 DisplayFood;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Resources")
	int32 DisplayIron;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Resources")
	int32 DisplayWood;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Resources")
	int32 DisplayStone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Resources")
	int32 DisplayPopulation;

	// Currently selected units
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Selection")
	TArray<AUnitBase*> CurrentSelectedUnits;

	// Currently selected building
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Selection")
	ABuildingBase* HUDSelectedBuilding;

	// Victory state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Victory")
	bool bVictory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Victory")
	bool bDefeated;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Victory")
	int32 EnemiesKilled;

	// Cached references
	UPROPERTY()
	ARomanEmpireGameMode* CachedGameMode;

private:
	float LastVictoryCheckTime;

	// Draw sub-routines
	void DrawResourceBar();
	void DrawPhaseIndicator();
	void DrawControlsHelp();
	void DrawUnitPanel();
	void DrawBuildingMenu();
	void DrawMinimap();
	void DrawSelectionBoxes();
	void DrawHealthBars();
	void DrawBuildingPanel();
	void DrawVictoryOverlay();
	void CheckVictoryCondition();
};
