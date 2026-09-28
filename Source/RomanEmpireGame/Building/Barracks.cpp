// Copyright Roman Empire Game. All Rights Reserved.

#include "Barracks.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Core/RomanEmpireGameMode.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "Units/RomanUnitRegistry.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Templates/UnrealTemplate.h"

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
	if (bIsUpdatingProduction)
	{
		return;
	}
	Super::Tick(DeltaSeconds);

	if (!IsComplete() || !bIsTraining)
	{
		return;
	}
	TGuardValue<bool> UpdatingProductionGuard(bIsUpdatingProduction, true);

	// Progress training
	TrainingProgress += DeltaSeconds / TrainingTime;

	if (TrainingProgress >= 1.0f)
	{
		const bool bSpawnedUnit = SpawnTrainedUnit();
		FinishCurrentOrder(bSpawnedUnit ? EProductionEventType::Completed : EProductionEventType::Failed);
	}
}

bool ABarracks::TrainUnit(FName UnitID)
{
	if (bIsUpdatingProduction)
	{
		return false;
	}
	TGuardValue<bool> UpdatingProductionGuard(bIsUpdatingProduction, true);

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
		BroadcastProductionEvent(EProductionEventType::Started, UnitID);
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Started training: %s"), *UnitID.ToString());
	}
	else
	{
		// Add to queue
		TrainingQueue.Add(UnitID);
		BroadcastProductionEvent(EProductionEventType::Queued, UnitID);
		UE_LOG(LogRomanEmpire, Log, TEXT("Added to queue: %s (Queue size: %d)"), 
			*UnitID.ToString(), TrainingQueue.Num());
	}

	return true;
}

void ABarracks::CancelTraining()
{
	if (bIsUpdatingProduction)
	{
		return;
	}
	TGuardValue<bool> UpdatingProductionGuard(bIsUpdatingProduction, true);

	if (bIsTraining)
	{
		// Accepted orders consume their full reservation. A cancellation-refund policy is deferred.
		FinishCurrentOrder(EProductionEventType::Cancelled);
	}
}

bool ABarracks::SpawnTrainedUnit()
{
	UWorld* World = GetWorld();
	if (!World || CurrentlyTrainingID.IsNone())
	{
		return false;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(CurrentlyTrainingID) : nullptr;
	UClass* UnitClass = UnitAsset ? UnitAsset->UnitClass.Get() : nullptr;
	if (!UnitClass)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot spawn trained unit %s: factory class is unavailable"),
			*CurrentlyTrainingID.ToString());
		return false;
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
		if (IsValid(NewUnit) && NewUnit->IsAlive())
		{
			if (URomanUnitRegistrySubsystem* Units = World->GetSubsystem<URomanUnitRegistrySubsystem>())
			{
				Units->CreateSquad({ NewUnit });
			}
			UE_LOG(LogRomanEmpire, Log, TEXT("Trained unit: %s"), *NewUnit->GetName());
			return true;
		}
	}
	else if (NewUnit)
	{
		NewUnit->Destroy();
	}
	UE_LOG(LogRomanEmpire, Warning, TEXT("Training failed to create unit %s"), *CurrentlyTrainingID.ToString());
	return false;
}

FName ABarracks::StartNextInQueue(TArray<FName>& FailedOrders)
{
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	while (!TrainingQueue.IsEmpty())
	{
		const FName NextUnitID = TrainingQueue[0];
		TrainingQueue.RemoveAt(0);

		const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(NextUnitID) : nullptr;
		if (!UnitAsset || !UnitAsset->UnitClass.Get() || UnitAsset->Definition.TrainingTime <= 0.0f)
		{
			FailedOrders.Add(NextUnitID);
			UE_LOG(LogRomanEmpire, Warning, TEXT("Queued production definition %s is unavailable"),
				*NextUnitID.ToString());
			continue;
		}
		CurrentlyTrainingID = NextUnitID;
		bIsTraining = true;
		TrainingProgress = 0.0f;
		TrainingTime = UnitAsset->Definition.TrainingTime;
		return NextUnitID;
	}
	return NAME_None;
}

void ABarracks::FinishCurrentOrder(EProductionEventType Type)
{
	const FName FinishedUnitID = CurrentlyTrainingID;
	bIsTraining = false;
	TrainingProgress = 0.0f;
	TrainingTime = 0.0f;
	CurrentlyTrainingID = NAME_None;

	TArray<FName> FailedOrders;
	const FName StartedUnitID = StartNextInQueue(FailedOrders);

	// Every observer sees the next active order and final pending count, never a half-advanced queue.
	BroadcastProductionEvent(Type, FinishedUnitID);
	for (const FName FailedUnitID : FailedOrders)
	{
		BroadcastProductionEvent(EProductionEventType::Failed, FailedUnitID);
	}
	if (!StartedUnitID.IsNone())
	{
		BroadcastProductionEvent(EProductionEventType::Started, StartedUnitID);
	}
}

void ABarracks::BroadcastProductionEvent(EProductionEventType Type, FName UnitID)
{
	FProductionEvent Event;
	Event.Type = Type;
	Event.UnitID = UnitID;
	Event.PendingOrders = TrainingQueue.Num();
	OnProductionChanged.Broadcast(this, Event);
}

