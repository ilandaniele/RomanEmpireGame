// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RomanEmpireMainWidget.generated.h"

class UCanvasPanel;
class UHorizontalBox;
class UVerticalBox;
class UTextBlock;
class UButton;
class UImage;
class UProgressBar;
class UBorder;
class AUnitBase;

/**
 * Main HUD widget containing all sub-panels
 * Manages strategic, tactical, and FPS UI modes
 */
UCLASS()
class ROMANEMPIREGAME_API URomanEmpireMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Building menu
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideBuildingMenu();

	// Unit panel
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateUnitPanel(const TArray<AUnitBase*>& SelectedUnits);

	// Resources
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateResourceDisplay(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population);

	// FPS mode
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetFPSMode(bool bEnabled);

	// Visibility control
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetStrategicUIVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetTacticalUIVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetFPSUIVisible(bool bVisible);

	// Minimap
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateMinimap();

protected:
	// Main containers
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "UI")
	UCanvasPanel* RootCanvas;

	// Resource bar (top)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UHorizontalBox* ResourceBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UTextBlock* GoldText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UTextBlock* FoodText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UTextBlock* IronText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UTextBlock* WoodText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UTextBlock* StoneText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Resources")
	UTextBlock* PopulationText;

	// Building menu (left side)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Building")
	UVerticalBox* BuildingMenuPanel;

	// Unit panel (bottom)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Unit")
	UHorizontalBox* UnitPanel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Unit")
	UImage* UnitPortrait;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Unit")
	UTextBlock* UnitNameText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Unit")
	UProgressBar* UnitHealthBar;

	// Minimap (bottom right)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Minimap")
	UBorder* MinimapContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Minimap")
	UImage* MinimapImage;

	// FPS overlay
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|FPS")
	UCanvasPanel* FPSOverlay;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|FPS")
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|FPS")
	UProgressBar* StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|FPS")
	UImage* CrosshairImage;

	// Strategic overlay (world map)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Strategic")
	UCanvasPanel* StrategicOverlay;

	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|State")
	bool bFPSModeActive;

private:
	void SetupBuildingButtons();
};
