// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/ZoomConfiguration.h"
#include "Core/GameplayMode.h"
#include "Faction/FactionData.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "RomanEmpirePlayerController.generated.h"

class ARomanEmpireGameMode;
class ARomanEmpireGameState;
class UInputMappingContext;
class URomanInputConfig;
class AUnitBase;
class ABuildingBase;
class ABarracks;
class ALegionary;
class UBuildingPlacementComponent;
class ASeamlessZoomCamera;
class URomanEmpireMainWidget;

/**
 * Selection mode for the player
 */
UENUM(BlueprintType)
enum class ERomanSelectionMode : uint8
{
	None,
	SingleSelect,
	BoxSelect,
	BuildingPlacement
};

/**
 * Main player controller handling input for all game modes
 * Binds authored Enhanced Input assets and owns only its mode-specific contexts.
 */
UCLASS()
class ROMANEMPIREGAME_API ARomanEmpirePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARomanEmpirePlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void FlushPressedKeys() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	virtual bool ShouldFlushKeysWhenViewportFocusChanges() const override { return true; }

	UFUNCTION(BlueprintPure, Category = "Input")
	bool CanIssueTacticalCommands() const;

	UFUNCTION(BlueprintPure, Category = "Input")
	bool CanEndTurn() const;

	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool RequestEndTurn();

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool StartBuildingPlacementByQuickSlot(int32 QuickSlot);

	UFUNCTION(BlueprintPure, Category = "Input")
	URomanInputConfig* GetInputConfiguration() const { return InputConfiguration; }

	UFUNCTION(BlueprintPure, Category = "Input")
	UInputMappingContext* GetActiveGameplayContext() const { return ActiveGameplayContext; }

	UFUNCTION(BlueprintCallable, Category = "Input")
	void CancelTransientInput();

	UFUNCTION(BlueprintPure, Category = "Selection")
	bool IsBoxSelecting() const { return bIsBoxSelecting; }

	// Selection
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnit(AUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnits(const TArray<AUnitBase*>& Units);

	/** Screen-space rectangle command shared by drag selection and deterministic checks. */
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnitsInScreenRect(const FVector2D& Start, const FVector2D& End);

	/** Ownership/lifecycle eligibility, independent of the current input mode. */
	UFUNCTION(BlueprintPure, Category = "Selection")
	bool IsPlayerUnitEligible(const AUnitBase* Unit) const;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void CommandSelectedUnitsMove(const FVector& Destination);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void CommandSelectedUnitsAttack(AUnitBase* Target);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "Selection")
	TArray<AUnitBase*> GetSelectedUnits() const { return SelectedUnits; }

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectBuilding(ABuildingBase* Building);

	UFUNCTION(BlueprintPure, Category = "Selection")
	ABuildingBase* GetSelectedBuilding() const { return SelectedBuilding; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnSelectionChanged,
		const TArray<AUnitBase*>&, Units,
		ABuildingBase*, Building);

	UPROPERTY(BlueprintAssignable, Category = "Selection|Events")
	FOnSelectionChanged OnSelectionChanged;

	// Building
	UFUNCTION(BlueprintCallable, Category = "Building")
	void StartBuildingPlacement(FName BuildingID);

	UFUNCTION(BlueprintCallable, Category = "Building")
	void CancelBuildingPlacement();

	void OnBuildingKey1Pressed();
	void OnBuildingKey2Pressed();
	void OnBuildingKey3Pressed();
	void OnBuildingKey4Pressed();
	void OnBuildingKey5Pressed();
	void OnBuildingKey6Pressed();

	// FPS Mode
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void EnterFirstPersonMode(AUnitBase* UnitToPossess);

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void ExitFirstPersonMode();

	UFUNCTION(BlueprintPure, Category = "FPS")
	bool IsInFirstPersonMode() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnPossessedUnitChanged,
		AUnitBase*, PreviousUnit,
		AUnitBase*, NewUnit);

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnPossessedUnitChanged OnPossessedUnitChanged;

	// Zoom control
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetTargetZoom(float NewZoom);

	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCurrentZoom() const;

	// Selected building (for info panel / training)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<ABuildingBase> SelectedBuilding;

	// Building select / training
	bool OnTrainButtonClicked();

	UFUNCTION(BlueprintPure, Category = "UI")
	URomanEmpireMainWidget* GetMainWidget() const { return MainWidget; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<URomanInputConfig> InputConfigurationAsset;

	UPROPERTY(Transient)
	TObjectPtr<URomanInputConfig> InputConfiguration;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveGameplayContext;

	// Selection state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	TArray<AUnitBase*> SelectedUnits;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	ERomanSelectionMode CurrentSelectionMode;

	// Possession target; the canonical mode itself lives in GameMode.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<AUnitBase> PossessedUnit;

	// Cached camera pawn for restoring after FPS mode
	UPROPERTY()
	TObjectPtr<ASeamlessZoomCamera> CachedCameraPawn;

	// Building placement
	UPROPERTY()
	TObjectPtr<UBuildingPlacementComponent> BuildingPlacementComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<URomanEmpireMainWidget> MainWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<URomanEmpireMainWidget> MainWidget;

	// Input handlers
	void OnSelectPressed();
	void OnSelectReleased();
	void OnCommandPressed();
	void OnZoomInput(const FInputActionValue& Value);
	void OnCameraPanInput(const FInputActionValue& Value);
	void OnSoldierMoveInput(const FInputActionValue& Value);
	void OnSelectCanceled();
	void OnCancelPressed();
	void OnBuildingSlotPressed(int32 QuickSlot);
	void OnLookInput(const FInputActionValue& Value);
	void OnEnterFPSPressed();
	void OnBuildMenuPressed();
	void OnAttackPressed();
	void OnBlockPressed();
	void OnBlockReleased();
	void OnEndTurnPressed();

	UFUNCTION()
	void HandleCameraZoomLevelChanged(EZoomLevel OldLevel, EZoomLevel NewLevel);

	// Edge scroll for RTS camera
	void HandleEdgeScroll(float DeltaTime);

private:
	// Box selection
	FVector2D BoxSelectStart;
	bool bIsBoxSelecting;

	// Cache game mode
	UPROPERTY()
	TObjectPtr<ARomanEmpireGameMode> GameMode;

	UPROPERTY(Transient)
	TObjectPtr<ARomanEmpireGameState> PublicGameState;

	bool LoadInputConfiguration();
	void ApplyInputContexts();
	void RemoveOwnedInputContexts();
	bool HasActiveModeContext() const;
	bool CanControlSoldier() const;
	bool CanControlCamera() const;
	void CaptureHeldMovementKeys(UInputMappingContext* DestinationContext);
	TSet<FKey> MovementKeysAwaitingRelease;

	UFUNCTION()
	void HandleGameplayInitialized();

	UFUNCTION()
	void HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode);
	UFUNCTION()
	void HandlePlayerFactionChanged(EFactionID NewFaction);
	UFUNCTION()
	void HandleSelectedUnitOwnerChanged(AUnitBase* Unit);
	UFUNCTION()
	void HandlePossessedUnitOwnerChanged(AUnitBase* Unit);
	void ReconcileUnitOwnership();
	bool bExitingPossession = false;
	bool bControllerEndingPlay = false;
	void PerformBoxSelect();
	void ClearSelectedActors();
	void BroadcastSelectionChanged();

	UFUNCTION()
	void HandleSelectedUnitDied(AUnitBase* Unit);

	UFUNCTION()
	void HandleSelectedBuildingDestroyed(ABuildingBase* Building);

	UFUNCTION()
	void HandleSelectedActorDestroyed(AActor* Actor);

	UFUNCTION()
	void HandlePossessedUnitDied(AUnitBase* Unit);

	UFUNCTION()
	void HandlePossessedActorDestroyed(AActor* Actor);
	AActor* GetActorUnderCursor() const;
};
