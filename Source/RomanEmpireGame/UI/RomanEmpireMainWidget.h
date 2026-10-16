// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Battle/TacticalBattleResult.h"
#include "Building/ProductionEvent.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Core/GameplayMode.h"
#include "Faction/FactionData.h"
#include "Faction/DiplomacyState.h"
#include "TimerManager.h"
#include "UI/RomanTextScaleProjection.h"
#include "RomanEmpireMainWidget.generated.h"

class ABuildingBase;
class ABarracks;
class ARomanEmpireGameState;
class ARomanEmpirePlayerController;
class AUnitBase;
class UBorder;
class UBuildingMenuWidget;
class UButton;
class UCanvasPanel;
class UMinimapWidget;
class USizeBox;
class UTextBlock;
class UUniformGridPanel;
class URomanTerritoryButton;
class URomanArmyButton;
class UCampaignSessionSubsystem;
class UCampaignSaveSubsystem;
class UHorizontalBox;
class UScrollBox;
class UVerticalBox;
class URomanPlayerSettingsSubsystem;
class UEnhancedInputUserSettings;

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

	/** Local inspection only; does not change campaign or tactical selection. */
	UFUNCTION(BlueprintCallable, Category = "UI|Territories")
	bool SelectTerritory(FName TerritoryID);
	UFUNCTION(BlueprintPure, Category = "UI|Territories")
	FName GetSelectedTerritoryID() const { return SelectedTerritoryID; }
	UFUNCTION(BlueprintCallable, Category = "UI|Armies") bool SelectCampaignArmy(FGuid ArmyID);
	UFUNCTION(BlueprintPure, Category = "UI|Armies") FGuid GetSelectedCampaignArmyID() const { return SelectedArmyID; }
	UFUNCTION(BlueprintCallable, Category = "UI|Armies") bool SetCampaignActionCount(int32 Count);
	UFUNCTION(BlueprintPure, Category = "UI|Armies") int32 GetCampaignActionCount() const { return CampaignActionCount; }
	UFUNCTION(BlueprintCallable, Category = "UI|Armies") bool SelectCampaignActionUnit(FName UnitID);
	UFUNCTION(BlueprintPure, Category = "UI|Armies") FName GetCampaignActionUnitID() const { return CampaignActionUnitID; }
	UFUNCTION(BlueprintPure, Category = "UI|Armies") FGuid GetAssignmentTargetArmyID() const { return AssignmentTargetArmyID; }

private:
	TWeakObjectPtr<URomanPlayerSettingsSubsystem> PreferencesSource;
	TWeakObjectPtr<UEnhancedInputUserSettings> BindingSettingsSource;
	FDelegateHandle PreferencesChangedHandle;
	FRomanTextScaleProjection TextScaleProjection;
	void BindPreferences();
	void UnbindPreferences();
	void RefreshTextScale();
	void RefreshInputLabels();
	UFUNCTION() void HandleBindingsChanged(UEnhancedInputUserSettings* Settings);
	UFUNCTION() void HandleSettingsClicked();
	static constexpr int32 MaxCampaignActionCount = 100; // HUD batch limit; canonical backend validates its own range.
	UPROPERTY(Transient) int32 CampaignActionCount = 1;
	UPROPERTY(Transient) FName CampaignActionUnitID;
	UPROPERTY(Transient) FGuid AssignmentTargetArmyID;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CampaignArmyActionsPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignActionCountText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignRecruitmentQuoteText;
	UPROPERTY(Transient) TObjectPtr<UButton> CampaignUnitChoiceButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CampaignCountDecreaseButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CampaignCountIncreaseButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RecruitNewArmyButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RecruitSelectedArmyButton;
	UPROPERTY(Transient) TObjectPtr<UButton> AssignmentTargetButton;
	UPROPERTY(Transient) TObjectPtr<UButton> AssignArmyUnitsButton;
	void BuildCampaignArmyActionControls(UVerticalBox* Details);
	void RefreshCampaignArmyActions();
	TArray<FGuid> GetAssignmentTargetChoices() const;
	void ExecuteCampaignRecruitment(FGuid DestinationArmyID);
	UFUNCTION() void HandleCampaignUnitChoiceClicked();
	UFUNCTION() void HandleCampaignCountDecreaseClicked();
	UFUNCTION() void HandleCampaignCountIncreaseClicked();
	UFUNCTION() void HandleRecruitNewArmyClicked();
	UFUNCTION() void HandleRecruitSelectedArmyClicked();
	UFUNCTION() void HandleAssignmentTargetClicked();
	UFUNCTION() void HandleAssignArmyUnitsClicked();
	UPROPERTY(Transient) TObjectPtr<UCampaignSessionSubsystem> CachedSession;
	UPROPERTY(Transient) TObjectPtr<UCampaignSaveSubsystem> CachedSave;
	UPROPERTY(Transient) TObjectPtr<UBorder> CampaignSavePanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignSaveStatus;
	UPROPERTY(Transient) TObjectPtr<UButton> SaveCampaignButton;
	UPROPERTY(Transient) TObjectPtr<UButton> LoadCampaignButton;
	UPROPERTY(Transient) TObjectPtr<UButton> LoadAutosaveButton;
	UPROPERTY(Transient) TObjectPtr<UButton> AutosaveToggleButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmSaveActionButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelSaveActionButton;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> SaveConfirmationRow;
	FTimerHandle SaveRefreshTimer;
	int32 PendingSaveAction = 0, PendingSaveTurn = 0;
	FText CampaignSaveNotice;
	void BuildCampaignSaveControls();
	void BindSaveEvents();
	void UnbindSaveEvents();
	bool IsLiveSaveUI() const;
	void RefreshSaveControls();
	void RequestSaveUIAction(int32 Action);
	UFUNCTION() void ScheduleSaveRefresh();
	UFUNCTION() void HandleSaveCampaignClicked();
	UFUNCTION() void HandleLoadCampaignClicked();
	UFUNCTION() void HandleLoadAutosaveClicked();
	UFUNCTION() void HandleAutosaveToggleClicked();
	UFUNCTION() void HandleConfirmSaveActionClicked();
	UFUNCTION() void HandleCancelSaveActionClicked();
	UPROPERTY(Transient) TObjectPtr<UButton> EnterCampaignButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ContinueCampaignButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RetryCampaignButton;
	UPROPERTY(Transient) TObjectPtr<UButton> MoveArmyButton;
	UPROPERTY(Transient) TObjectPtr<UButton> StartBattleButton;
	UPROPERTY(Transient) TObjectPtr<UBorder> CampaignRecoveryPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignRecoveryText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ArmyDetailText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ArmyList;
	UPROPERTY(Transient) TArray<TObjectPtr<URomanArmyButton>> ArmyButtons;
	UPROPERTY(Transient) FGuid SelectedArmyID;
	void BuildCampaignControls(UVerticalBox* Commands, UVerticalBox* Outcome, UVerticalBox* Details);
	void BindSessionEvents();
	void UnbindSessionEvents();
	bool IsLiveCampaignUI(bool bRequireReady = true) const;
	void RefreshCampaignControls();
	void RefreshCampaignArmies();
	UFUNCTION() void HandleCampaignSessionChanged();
	UFUNCTION() void HandleCampaignArmiesChanged();
	UFUNCTION() void HandleArmyClicked(FGuid ArmyID);
	UFUNCTION() void HandleDiplomacyChanged(FDiplomaticPair Pair, EDiplomaticStatus Status);
	UFUNCTION() void HandleEnterCampaignClicked();
	UFUNCTION() void HandleContinueCampaignClicked();
	UFUNCTION() void HandleRetryCampaignClicked();
	UFUNCTION() void HandleMoveArmyClicked();
	UFUNCTION() void HandleStartBattleClicked();
	UPROPERTY(Transient)
	TObjectPtr<UBorder> StrategicPanel;
	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> TerritoryGrid;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TerritoryDetailText;
	UPROPERTY(Transient)
	TArray<TObjectPtr<URomanTerritoryButton>> TerritoryButtons;
	UPROPERTY(Transient)
	FName SelectedTerritoryID = NAME_None;
	void RefreshStrategicTerritories();
	UFUNCTION()
	void HandleTerritoryClicked(FName TerritoryID);
	UFUNCTION()
	void HandleTerritoryOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner);
	UFUNCTION()
	void HandleTerritorySettlementChanged(FName TerritoryID);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UMinimapWidget> TacticalMinimap;
	UPROPERTY(Transient) TObjectPtr<UMinimapWidget> StrategicMinimap;
	UPROPERTY(Transient) TObjectPtr<USizeBox> TacticalMinimapFrame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResourceText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BattleText;

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
	TObjectPtr<UBorder> PossessionPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CrosshairText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ControlsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InteractionFeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> InteractionFeedbackPanel;

	FTimerHandle InteractionFeedbackTimer;
	void ClearInteractionFeedback();
	UFUNCTION()
	void HandleInteractionFeedback(FText Message);

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
	TObjectPtr<UButton> CancelTrainingButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartEncounterButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> StopButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> HoldButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BuildMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<ARomanEmpirePlayerController> CachedController;

	UPROPERTY(Transient)
	TObjectPtr<ARomanEmpireGameState> CachedGameState;

	UPROPERTY(Transient)
	TObjectPtr<AUnitBase> ObservedUnit;

	UPROPERTY(Transient)
	TObjectPtr<ABuildingBase> ObservedBuilding;

	UPROPERTY(Transient)
	TObjectPtr<ABarracks> ObservedBarracks;

	bool bCanonicalEventsBound = false;

	void BuildNativeWidgetTree();
	void BuildResponsiveShell();
	void BindCanonicalEvents();
	void UnbindCanonicalEvents();
	void BindSelectionEvents(const TArray<AUnitBase*>& Units, ABuildingBase* Building);
	void UnbindSelectionEvents();
	void RefreshFromCanonicalState();
	void RefreshModeAndTurn();
	void RefreshCommands();
	void RefreshObjectives(const FCampaignObjectiveState& State);
	void RefreshBattle(const FTacticalBattleResult& Result);
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
	void HandleTacticalBattleChanged(FTacticalBattleResult Result);

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
	void HandleCancelTrainingClicked();
	UFUNCTION()
	void HandleRestartEncounterClicked();
	UFUNCTION()
	void HandleStopClicked();
	UFUNCTION()
	void HandleHoldClicked();

	UFUNCTION()
	void HandleEndTurnClicked();

	UFUNCTION()
	void HandleBuildMenuClicked();
};
