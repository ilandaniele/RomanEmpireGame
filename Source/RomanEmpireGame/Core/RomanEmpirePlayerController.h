// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "RomanEmpirePlayerController.generated.h"

class ARomanEmpireGameMode;
class UInputMappingContext;
class UInputAction;
class AUnitBase;
class ABuildingBase;
class UBuildingPlacementComponent;
class ASeamlessZoomCamera;

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

	// Building
	UFUNCTION(BlueprintCallable, Category = "Building")
	void StartBuildingPlacement(TSubclassOf<ABuildingBase> BuildingClass);

	UFUNCTION(BlueprintCallable, Category = "Building")
	void CancelBuildingPlacement();

	void OnBuildingKey1Pressed();

	// FPS Mode
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void EnterFirstPersonMode(AUnitBase* UnitToPossess);

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void ExitFirstPersonMode();

	UFUNCTION(BlueprintPure, Category = "FPS")
	bool IsInFirstPersonMode() const { return bIsInFirstPersonMode; }

	// Zoom control
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetTargetZoom(float NewZoom);

	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCurrentZoom() const { return CurrentZoomLevel; }

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

	// Selection state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	TArray<AUnitBase*> SelectedUnits;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	ERomanSelectionMode CurrentSelectionMode;

	// Zoom state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float CurrentZoomLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float ZoomSpeed;

	// FPS state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	bool bIsInFirstPersonMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	AUnitBase* PossessedUnit;

	// Cached camera pawn for restoring after FPS mode
	UPROPERTY()
	ASeamlessZoomCamera* CachedCameraPawn;

	// Building placement
	UPROPERTY()
	UBuildingPlacementComponent* BuildingPlacementComponent;

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

	// Edge scroll for RTS camera
	void HandleEdgeScroll(float DeltaTime);

private:
	// Box selection
	FVector2D BoxSelectStart;
	bool bIsBoxSelecting;

	// Cache game mode
	ARomanEmpireGameMode* GameMode;

	void CreateInputActionsAndMappings();
	void UpdateZoom(float DeltaTime);
	void PerformBoxSelect();
	AActor* GetActorUnderCursor() const;
};
