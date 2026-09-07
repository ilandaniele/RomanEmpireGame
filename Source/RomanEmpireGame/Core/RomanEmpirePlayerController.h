// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/ZoomConfiguration.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "RomanEmpirePlayerController.generated.h"

class ARomanEmpireGameMode;
class UInputMappingContext;
class UInputAction;
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
 * Creates Enhanced Input actions programmatically (no .uasset files needed)
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

	// Selection
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnit(AUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnits(const TArray<AUnitBase*>& Units);

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
	// Enhanced Input — created programmatically, no Blueprint assets needed
	UPROPERTY()
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY()
	UInputAction* IA_Select;

	UPROPERTY()
	UInputAction* IA_Command;

	UPROPERTY()
	UInputAction* IA_Zoom;

	UPROPERTY()
	UInputAction* IA_Move;

	UPROPERTY()
	UInputAction* IA_Look;

	UPROPERTY()
	UInputAction* IA_EnterFPS;

	UPROPERTY()
	UInputAction* IA_BuildMenu;

	UPROPERTY()
	UInputAction* IA_Attack;

	UPROPERTY()
	UInputAction* IA_Block;

	UPROPERTY()
	UInputAction* IA_EndTurn;

	UPROPERTY()
	UInputAction* IA_BuildKey1;

	UPROPERTY()
	UInputAction* IA_BuildKey2;

	UPROPERTY()
	UInputAction* IA_BuildKey3;

	UPROPERTY()
	UInputAction* IA_BuildKey4;

	UPROPERTY()
	UInputAction* IA_BuildKey5;

	UPROPERTY()
	UInputAction* IA_BuildKey6;

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
	void OnMoveInput(const FInputActionValue& Value);
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

	void CreateInputActionsAndMappings();
	void StartBuildingPlacementByQuickSlot(int32 QuickSlot);
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
