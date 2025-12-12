// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RomanEmpireGame/Building/BuildingBase.h"
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
	void TrainUnit(TSubclassOf<class AUnitBase> UnitClass);

	UFUNCTION(BlueprintPure, Category = "Barracks")
	float GetTrainingProgress() const { return TrainingProgress; }

	UFUNCTION(BlueprintPure, Category = "Barracks")
	bool IsTraining() const { return bIsTraining; }

	UFUNCTION(BlueprintCallable, Category = "Barracks")
	void CancelTraining();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	bool bIsTraining;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	float TrainingProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	float TrainingTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	TSubclassOf<AUnitBase> CurrentlyTraining;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barracks")
	FVector SpawnOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barracks")
	int32 MaxQueueSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barracks")
	TArray<TSubclassOf<AUnitBase>> TrainingQueue;

private:
	void SpawnTrainedUnit();
	void StartNextInQueue();
};
