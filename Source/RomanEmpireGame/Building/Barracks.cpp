// Copyright Roman Empire Game. All Rights Reserved.

#include "Barracks.h"
#include "RomanEmpireGame.h"
#include "Core/RomanEmpireGameMode.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABarracks::ABarracks()
{
	PrimaryActorTick.bCanEverTick = true;

	// Setup barracks data
	BuildingData.BuildingType = EBuildingType::Barracks;
	BuildingData.Category = EBuildingCategory::Military;
	BuildingData.DisplayName = FText::FromString(TEXT("Barracks"));
	BuildingData.Description = FText::FromString(TEXT("Trains infantry units for your armies."));
	BuildingData.MaxHealth = 1500;
	BuildingData.Armor = 5;
	BuildingData.FootprintSize = FVector2D(4.0f, 4.0f);
	BuildingData.ConstructionTime = 30.0f;
	
	BuildingData.Cost.Gold = 400;
	BuildingData.Cost.Wood = 200;
	BuildingData.Cost.Stone = 100;

	BuildingData.TrainableUnits.Add(TEXT("Legionary"));
	BuildingData.TrainableUnits.Add(TEXT("Velites"));
	BuildingData.TrainableUnits.Add(TEXT("Triarii"));

	// Initialize training
	bIsTraining = false;
	TrainingProgress = 0.0f;
	TrainingTime = 10.0f;
	CurrentlyTraining = nullptr;
	SpawnOffset = FVector(300.0f, 0.0f, 0.0f);
	MaxQueueSize = 5;
}

void ABarracks::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsComplete() || !bIsTraining)
	{
		return;
	}

	// Progress training
	TrainingProgress += DeltaSeconds / TrainingTime;

	if (TrainingProgress >= 1.0f)
	{
		SpawnTrainedUnit();
		TrainingProgress = 0.0f;
		bIsTraining = false;
		CurrentlyTraining = nullptr;

		// Start next in queue
		StartNextInQueue();
	}
}

bool ABarracks::TrainUnit(TSubclassOf<AUnitBase> UnitClass)
{
	if (!UnitClass || !IsComplete())
	{
		return false;
	}

	if (TrainingQueue.Num() >= MaxQueueSize)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Training queue is full"));
		return false;
	}

	const AUnitBase* UnitDefaults = UnitClass->GetDefaultObject<AUnitBase>();
	const ARomanEmpireGameMode* GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	AFactionManager* FactionManager = GameMode ? GameMode->GetFactionManager() : nullptr;
	if (!UnitDefaults || !FactionManager || OwnerFaction == EFactionID::None)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot train unit: missing definition or faction resource authority"));
		return false;
	}

	const FUnitData UnitDefinition = UnitDefaults->GetUnitData();
	if (UnitDefinition.TrainingTime <= 0.0f)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot train unit: %s has a non-positive training time"),
			*UnitClass->GetName());
		return false;
	}

	const FFactionResourceTransactionResult SpendResult =
		FactionManager->TrySpendFactionResources(OwnerFaction, UnitDefinition.TrainingCost);
	if (!SpendResult.WasSuccessful())
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot train unit: resource transaction failed (%s)"),
			RomanEmpireEconomy::GetTransactionFailureName(SpendResult.Failure));
		return false;
	}

	if (!bIsTraining)
	{
		// Start training immediately
		CurrentlyTraining = UnitClass;
		bIsTraining = true;
		TrainingProgress = 0.0f;
		TrainingTime = UnitDefinition.TrainingTime;
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Started training: %s"), *UnitClass->GetName());
	}
	else
	{
		// Add to queue
		TrainingQueue.Add(UnitClass);
		UE_LOG(LogRomanEmpire, Log, TEXT("Added to queue: %s (Queue size: %d)"), 
			*UnitClass->GetName(), TrainingQueue.Num());
	}

	return true;
}

void ABarracks::CancelTraining()
{
	if (bIsTraining)
	{
		bIsTraining = false;
		TrainingProgress = 0.0f;
		CurrentlyTraining = nullptr;
		
		// Accepted orders consume their full reservation. A cancellation-refund policy is deferred.
		
		StartNextInQueue();
	}
}

void ABarracks::SpawnTrainedUnit()
{
	UWorld* World = GetWorld();
	if (!World || !CurrentlyTraining)
	{
		return;
	}

	FVector SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(SpawnOffset);
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AUnitBase* NewUnit = World->SpawnActor<AUnitBase>(CurrentlyTraining, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (NewUnit)
	{
		NewUnit->SetOwnerFaction(OwnerFaction);
		UE_LOG(LogRomanEmpire, Log, TEXT("Trained unit: %s"), *NewUnit->GetName());
	}
}

void ABarracks::StartNextInQueue()
{
	if (TrainingQueue.Num() > 0)
	{
		CurrentlyTraining = TrainingQueue[0];
		TrainingQueue.RemoveAt(0);
		bIsTraining = true;
		TrainingProgress = 0.0f;

		if (const AUnitBase* UnitDefaults = CurrentlyTraining->GetDefaultObject<AUnitBase>())
		{
			TrainingTime = UnitDefaults->GetUnitData().TrainingTime;
		}
	}
}

