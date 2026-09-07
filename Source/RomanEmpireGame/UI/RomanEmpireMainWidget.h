// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Building/ProductionEvent.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Core/GameplayMode.h"
#include "Faction/FactionData.h"
#include "RomanEmpireMainWidget.generated.h"

class ABuildingBase;
class ABarracks;
class ACampaignManager;
class AFactionManager;
class ARomanEmpireGameMode;
class ARomanEmpirePlayerController;
class AUnitBase;
class AWorldMapManager;
class UBorder;
class UBuildingMenuWidget;
class UButton;
class UCanvasPanel;
class UTextBlock;

/**
 * Production UMG root. It observes authoritative gameplay objects and never
 * owns resource, turn, objective, production, possession, or victory state.
 */
UCLASS()
class ROMANEMPIREGAME_API URomanEmpireMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "UI|Building")
	void ShowBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "UI|Building")
	void HideBuildingMenu();

	UFUNCTION(BlueprintCallable, Category = "UI|Building")
	void ToggleBuildingMenu();

	UFUNCTION(BlueprintPure, Category = "UI|Building")
	bool IsBuildingMenuVisible() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Resources")
	void UpdateResourceDisplay(const FFactionResourceAmounts& Resources);

	UFUNCTION(BlueprintCallable, Category = "UI|Diagnostics")
	void SetDiagnosticVisible(bool bVisible);

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResourceText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DiagnosticText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProductionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OutcomeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TrainButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PossessionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CrosshairText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ControlsText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BuildMenuPanel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SelectionPanel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DiagnosticPanel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> OutcomePanel;

	UPROPERTY(Transient)
	TObjectPtr<UBuildingMenuWidget> BuildingMenu;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TrainButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BuildMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<ARomanEmpirePlayerController> CachedController;

	UPROPERTY(Transient)
	TObjectPtr<ARomanEmpireGameMode> CachedGameMode;

	UPROPERTY(Transient)
	TObjectPtr<AFactionManager> CachedFactionManager;

	UPROPERTY(Transient)
	TObjectPtr<AWorldMapManager> CachedWorldMapManager;

	UPROPERTY(Transient)
	TObjectPtr<ACampaignManager> CachedCampaignManager;

	UPROPERTY(Transient)
	TObjectPtr<AUnitBase> ObservedUnit;

	UPROPERTY(Transient)
	TObjectPtr<ABuildingBase> ObservedBuilding;

	UPROPERTY(Transient)
	TObjectPtr<ABarracks> ObservedBarracks;

	bool bCanonicalEventsBound = false;

	void BuildNativeWidgetTree();
	void BindCanonicalEvents();
	void UnbindCanonicalEvents();
	void BindSelectionEvents(const TArray<AUnitBase*>& Units, ABuildingBase* Building);
	void UnbindSelectionEvents();
	void RefreshFromCanonicalState();
	void RefreshModeAndTurn();
	void RefreshCommands();
	void RefreshObjectives(const FCampaignObjectiveState& State);
	void RefreshSelection();
	void RefreshDiagnostic();
	void RefreshOutcome(ECampaignOutcome Outcome);

	UFUNCTION()
	void HandleSelectionChanged(const TArray<AUnitBase*>& Units, ABuildingBase* Building);

	UFUNCTION()
	void HandleResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance);

	UFUNCTION()
	void HandleGameplayInitialized();

	UFUNCTION()
	void HandlePlayerFactionChanged(EFactionID PlayerFaction);

	UFUNCTION()
	void HandleCampaignStarted(EFactionID PlayerFaction);

	UFUNCTION()
	void HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode);

	UFUNCTION()
	void HandleTurnStepStarted(int32 TurnNumber, ECampaignTurnStep Step);

	UFUNCTION()
	void HandleTurnProcessed(int32 TurnNumber);

	UFUNCTION()
	void HandleProductionChanged(ABarracks* Producer, FProductionEvent Event);

	UFUNCTION()
	void HandlePossessedUnitChanged(AUnitBase* PreviousUnit, AUnitBase* NewUnit);

	UFUNCTION()
	void HandleObjectivesChanged(FCampaignObjectiveState State);

	UFUNCTION()
	void HandleVictory(EFactionID WinningFaction, EVictoryCondition Condition);

	UFUNCTION()
	void HandleDefeat(EFactionID LosingFaction);

	UFUNCTION()
	void HandleUnitVitalsChanged(AUnitBase* Unit);

	UFUNCTION()
	void HandleBuildingStateChanged(ABuildingBase* Building);

	UFUNCTION()
	void HandleBuildingSelected(FName BuildingID);

	UFUNCTION()
	void HandleTrainClicked();

	UFUNCTION()
	void HandleEndTurnClicked();

	UFUNCTION()
	void HandleBuildMenuClicked();
};
