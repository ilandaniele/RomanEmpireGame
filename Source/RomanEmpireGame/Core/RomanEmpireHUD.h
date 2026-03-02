// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RomanEmpireHUD.generated.h"

class AUnitBase;
class ARomanEmpireGameMode;

/**
 * Main HUD class using Canvas-based drawing (no Widget Blueprints needed)
 * Draws resource bar, phase indicator, unit info, and minimap directly
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
	void UpdateResources(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnZoomLevelChanged(float ZoomLevel);

protected:
	// State
	bool bBuildingMenuVisible;
	bool bFPSMode;
	float CurrentZoomLevel;

public:
	// Resource values (public for GameMode access)
	int32 DisplayGold;
	int32 DisplayFood;
	int32 DisplayIron;
	int32 DisplayWood;
	int32 DisplayStone;
	int32 DisplayPopulation;

	// Cached selections
	TArray<AUnitBase*> CurrentSelectedUnits;

	// Cache
	ARomanEmpireGameMode* CachedGameMode;

private:
	void DrawResourceBar();
	void DrawPhaseIndicator();
	void DrawUnitPanel();
	void DrawBuildingMenu();
	void DrawMinimap();
	void DrawSelectionBoxes();
	void DrawControlsHelp();
	void DrawHealthBars();
};
