// Copyright Roman Empire Game. All Rights Reserved.

#include "Barracks.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Core/RomanEmpireGameMode.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABarracks::ABarracks()
{
	PrimaryActorTick.bCanEverTick = true;
	BuildingDefinitionID = TEXT("Barracks");

	// Initialize training
	bIsTraining = false;
	TrainingProgress = 0.0f;
	TrainingTime = 0.0f;
	CurrentlyTrainingID = NAME_None;
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
		CurrentlyTrainingID = NAME_None;

		// Start next in queue
		StartNextInQueue();
	}
}

bool ABarracks::TrainUnit(FName UnitID)
{
	if (!RomanEmpireContent::IsStableID(UnitID) || !IsComplete() ||
		!BuildingData.TrainableUnits.Contains(UnitID))
	{
		return false;
	}

	if (TrainingQueue.Num() >= MaxQueueSize)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Training queue is full"));
		return false;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(UnitID) : nullptr;
	const ARomanEmpireGameMode* GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	AFactionManager* FactionManager = GameMode ? GameMode->GetFactionManager() : nullptr;
	const URomanFactionDefinition* FactionAsset = Registry ? Registry->FindFaction(OwnerFaction) : nullptr;
	if (!Registry || !Registry->IsReady() || !UnitAsset || UnitAsset->UnitClass.IsNull() ||
		!FactionManager || !FactionAsset ||
		!UnitAsset->AvailableToFactions.Contains(FactionAsset->Definition.FactionID))
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot train unit: missing definition or faction resource authority"));
		return false;
	}

	const FUnitData& UnitDefinition = UnitAsset->Definition;
	if (UnitDefinition.TrainingTime <= 0.0f)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot train unit: %s has a non-positive training time"),
			*UnitID.ToString());
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
		CurrentlyTrainingID = UnitID;
		bIsTraining = true;
		TrainingProgress = 0.0f;
		TrainingTime = UnitDefinition.TrainingTime;
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Started training: %s"), *UnitID.ToString());
	}
	else
	{
		// Add to queue
		TrainingQueue.Add(UnitID);
		UE_LOG(LogRomanEmpire, Log, TEXT("Added to queue: %s (Queue size: %d)"), 
			*UnitID.ToString(), TrainingQueue.Num());
	}

	return true;
}

void ABarracks::CancelTraining()
{
	if (bIsTraining)
	{
		bIsTraining = false;
		TrainingProgress = 0.0f;
		CurrentlyTrainingID = NAME_None;
		
		// Accepted orders consume their full reservation. A cancellation-refund policy is deferred.
		
		StartNextInQueue();
	}
}

void ABarracks::SpawnTrainedUnit()
{
	UWorld* World = GetWorld();
	if (!World || CurrentlyTrainingID.IsNone())
	{
		return;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(CurrentlyTrainingID) : nullptr;
	UClass* UnitClass = UnitAsset ? UnitAsset->UnitClass.Get() : nullptr;
	if (!UnitClass)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot spawn trained unit %s: factory class is unavailable"),
			*CurrentlyTrainingID.ToString());
		return;
	}

	const FVector SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(SpawnOffset);
	const FRotator SpawnRotation = GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AUnitBase* NewUnit = World->SpawnActorDeferred<AUnitBase>(
		UnitClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	
	if (NewUnit && NewUnit->InitializeUnit(CurrentlyTrainingID, OwnerFaction))
	{
		UGameplayStatics::FinishSpawningActor(NewUnit, SpawnTransform);
		UE_LOG(LogRomanEmpire, Log, TEXT("Trained unit: %s"), *NewUnit->GetName());
	}
	else if (NewUnit)
	{
		NewUnit->Destroy();
	}
}

void ABarracks::StartNextInQueue()
{
	if (TrainingQueue.Num() > 0)
	{
		CurrentlyTrainingID = TrainingQueue[0];
		TrainingQueue.RemoveAt(0);
		bIsTraining = true;
		TrainingProgress = 0.0f;

		const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
		if (const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(CurrentlyTrainingID) : nullptr)
		{
			TrainingTime = UnitAsset->Definition.TrainingTime;
		}
	}
}

