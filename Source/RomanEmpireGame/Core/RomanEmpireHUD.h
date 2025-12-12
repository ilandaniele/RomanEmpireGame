// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RomanEmpireHUD.generated.h"

class URomanEmpireMainWidget;
class UBuildingMenuWidget;
class UUnitPanelWidget;
class UMinimapWidget;
class UResourceDisplayWidget;

/**
 * Main HUD class managing all UI elements
 * Adapts display based on current game phase and zoom level
 */
UCLASS()
class ROMANEMPIREGAME_API ARomanEmpireHUD : public AHUD
{
	GENERATED_BODY()

public:
	ARomanEmpireHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	// Widget access
	UFUNCTION(BlueprintPure, Category = "HUD")
	URomanEmpireMainWidget* GetMainWidget() const { return MainWidget; }

	// UI control
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateUnitSelection(const TArray<class AUnitBase*>& SelectedUnits);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowFPSOverlay(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateResources(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population);

	// Zoom level based UI
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnZoomLevelChanged(float ZoomLevel);

protected:
	// Widget classes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Widgets")
	TSubclassOf<URomanEmpireMainWidget> MainWidgetClass;

	// Widget instances
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Widgets")
	URomanEmpireMainWidget* MainWidget;

	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|State")
	bool bBuildingMenuVisible;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|State")
	bool bFPSMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|State")
	float CurrentZoomLevel;

private:
	void CreateWidgets();
	void UpdateUIVisibility();
};
