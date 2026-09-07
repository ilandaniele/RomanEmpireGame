// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/ProductionEvent.h"
#include "BuildingBase.h"
#include "Barracks.generated.h"

/**
 * Roman Barracks - Trains infantry units
 */
UCLASS()
class ROMANEMPIREGAME_API ABarracks : public ABuildingBase
{
	GENERATED_BODY()

public:
	ABarracks();

	// Unit training
	UFUNCTION(BlueprintCallable, Category = "Barracks")
	bool TrainUnit(FName UnitID);

	UFUNCTION(BlueprintPure, Category = "Barracks")
	float GetTrainingProgress() const { return TrainingProgress; }

	UFUNCTION(BlueprintPure, Category = "Barracks")
	bool IsTraining() const { return bIsTraining; }

	UFUNCTION(BlueprintPure, Category = "Barracks")
	float GetTrainingTimeRemaining() const { return bIsTraining ? FMath::Max(0.0f, TrainingTime * (1.0f - TrainingProgress)) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Barracks")
	FName GetCurrentlyTrainingID() const { return CurrentlyTrainingID; }

	UFUNCTION(BlueprintPure, Category = "Barracks")
	int32 GetPendingOrderCount() const { return TrainingQueue.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Barracks")
	void CancelTraining();

	virtual void Tick(float DeltaSeconds) override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnProductionChanged,
		ABarracks*, Producer,
		FProductionEvent, Event);

	UPROPERTY(BlueprintAssignable, Category = "Barracks|Events")
	FOnProductionChanged OnProductionChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	bool bIsTraining;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	float TrainingProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	float TrainingTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	FName CurrentlyTrainingID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barracks")
	FVector SpawnOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barracks")
	int32 MaxQueueSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	TArray<FName> TrainingQueue;

private:
	// Resource and production delegates may call back into gameplay synchronously.
	bool bIsUpdatingProduction = false;
	bool SpawnTrainedUnit();
	FName StartNextInQueue(TArray<FName>& FailedOrders);
	void FinishCurrentOrder(EProductionEventType Type);
	void BroadcastProductionEvent(EProductionEventType Type, FName UnitID);
};

