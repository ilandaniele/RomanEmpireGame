// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RomanEmpireGame/Core/RomanEmpireGameMode.h"
#include "InputActionValue.h"
#include "RomanEmpirePlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AUnitBase;
class ABuildingBase;
class UBuildingPlacementComponent;
class USeamlessZoomCamera;

/**
 * Selection mode for the player
 */
UENUM(BlueprintType)
enum class ESelectionMode : uint8
{
	None,
	SingleSelect,
	BoxSelect,
	BuildingPlacement
};

/**
 * Main player controller handling input for all game modes
 * Adapts behavior based on current game phase and zoom level
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
	// Enhanced Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Select;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Command;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Zoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_EnterFPS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_BuildMenu;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Attack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Block;

	// Selection state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	TArray<AUnitBase*> SelectedUnits;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	ESelectionMode CurrentSelectionMode;

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

private:
	// Box selection
	FVector2D BoxSelectStart;
	bool bIsBoxSelecting;

	// Cache game mode
	ARomanEmpireGameMode* GameMode;

	void UpdateZoom(float DeltaTime);
	void PerformBoxSelect();
	AActor* GetActorUnderCursor() const;
};
