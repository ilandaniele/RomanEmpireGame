// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Building/BuildingTypes.h"
#include "BuildingPlacementComponent.generated.h"

class ABuildingBase;

/**
 * Component handling building placement mechanics (Age of Empires style)
 * Shows ghost preview, validates placement, and spawns buildings on confirm
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROMANEMPIREGAME_API UBuildingPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildingPlacementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Placement control
	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void StartPlacement(FName BuildingID);

	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void UpdatePreview(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void RotatePreview(float Degrees);

	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	bool ConfirmPlacement();

	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void CancelPlacement();

	// State queries
	UFUNCTION(BlueprintPure, Category = "Building|Placement")
	bool IsPlacing() const { return bIsPlacing; }

	UFUNCTION(BlueprintPure, Category = "Building|Placement")
	bool CanPlace() const { return bCanPlace; }

	UFUNCTION(BlueprintPure, Category = "Building|Placement")
	FName GetCurrentBuildingID() const { return CurrentBuildingID; }

	UFUNCTION(BlueprintPure, Category = "Building|Placement")
	EResourceTransactionFailure GetLastTransactionFailure() const { return LastTransactionFailure; }

protected:
	// Currently placing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	bool bIsPlacing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	bool bCanPlace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	TSubclassOf<ABuildingBase> CurrentBuildingClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	FName CurrentBuildingID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	TObjectPtr<ABuildingBase> PreviewBuilding;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	FVector CurrentPlacementLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Placement")
	float CurrentRotation;

	// Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Placement")
	float GridSize; // Snap to grid

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Placement")
	bool bSnapToGrid;

	EResourceTransactionFailure LastTransactionFailure;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingPlaced, ABuildingBase*, Building);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlacementCancelled);

	UPROPERTY(BlueprintAssignable, Category = "Building|Placement")
	FOnBuildingPlaced OnBuildingPlaced;

	UPROPERTY(BlueprintAssignable, Category = "Building|Placement")
	FOnPlacementCancelled OnPlacementCancelled;

private:
	void CreatePreviewBuilding();
	void DestroyPreviewBuilding();
	FVector SnapToGrid(const FVector& Location) const;
	bool ValidatePlacement();
};

