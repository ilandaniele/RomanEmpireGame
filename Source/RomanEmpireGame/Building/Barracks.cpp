// Copyright Roman Empire Game. All Rights Reserved.

#include "Barracks.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Core/RomanEmpireGameMode.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "Units/RomanUnitRegistry.h"
#include "World/CampaignManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/ScopeExit.h"

ABarracks::ABarracks()
{
	PrimaryActorTick.bCanEverTick = true;
	BuildingDefinitionID = TEXT("Barracks");

	// Initialize training
	bIsTraining = false;
	TrainingProgress = 0.0f;
	TrainingTime = 0.0f;
	SpawnOffset = FVector(300.0f, 0.0f, 0.0f);
	MaxQueueSize = 5;
}

void ABarracks::Tick(float DeltaSeconds)
{
	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (bIsUpdatingProduction || (Battle && Battle->IsResolved()))
	{
		return;
	}
	Super::Tick(DeltaSeconds);

	if (!IsComplete() || !bIsTraining)
	{
		return;
	}
	TGuardValue<bool> UpdatingProductionGuard(bIsUpdatingProduction, true);

	if (bAwaitingFailureRefund)
	{
		if (TryRefundReservation(ActiveTrainingOrder)) FinishCurrentOrder(EProductionEventType::Failed);
		return;
	}

	// Progress training
	TrainingProgress += DeltaSeconds / TrainingTime;

	if (TrainingProgress >= 1.0f)
	{
		const bool bSpawnedUnit = SpawnTrainedUnit();
		if (bSpawnedUnit)
		{
			FinishCurrentOrder(EProductionEventType::Completed);
		}
		else
		{
			// Never discard a paid receipt or attempt a second spawn while credit is blocked.
			bAwaitingFailureRefund = true;
			TrainingProgress = 1.0f;
			if (TryRefundReservation(ActiveTrainingOrder)) FinishCurrentOrder(EProductionEventType::Failed);
			else BroadcastProductionEvent(EProductionEventType::RefundPending, GetCurrentlyTrainingID());
		}
	}
}

bool ABarracks::TrainUnit(FName UnitID)
{
	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (bIsUpdatingProduction || (Battle && Battle->IsResolved()))
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

	// Resource observers may edit definitions or ownership synchronously.
	const FUnitData UnitDefinition = UnitAsset->Definition;
	FTrainingReservation Reservation;
	Reservation.UnitID = UnitID;
	Reservation.PayingFaction = OwnerFaction;
	Reservation.PaidCost = UnitDefinition.TrainingCost;
	Reservation.ReceiptID = FGuid::NewGuid();
	ACampaignManager* Campaign = GameMode->GetCampaignManager();
	if (Campaign && !Campaign->PrepareTrainingReservation(Reservation)) return false;
	if (!FMath::IsFinite(UnitDefinition.TrainingTime) || UnitDefinition.TrainingTime <= 0.0f)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot train unit: %s has a non-positive training time"),
			*UnitID.ToString());
		return false;
	}

	const FFactionResourceTransactionResult SpendResult =
		FactionManager->TrySpendFactionResources(Reservation.PayingFaction, Reservation.PaidCost);
	if (!SpendResult.WasSuccessful())
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot train unit: resource transaction failed (%s)"),
			RomanEmpireEconomy::GetTransactionFailureName(SpendResult.Failure));
		return false;
	}

	if (!bIsTraining)
	{
		// Start training immediately
		ActiveTrainingOrder = Reservation;
		bIsTraining = true;
		TrainingProgress = 0.0f;
		TrainingTime = UnitDefinition.TrainingTime;
		BroadcastProductionEvent(EProductionEventType::Started, UnitID);
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Started training: %s"), *UnitID.ToString());
	}
	else
	{
		// Add to queue
		TrainingQueue.Add(Reservation);
		BroadcastProductionEvent(EProductionEventType::Queued, UnitID);
		UE_LOG(LogRomanEmpire, Log, TEXT("Added to queue: %s (Queue size: %d)"), 
			*UnitID.ToString(), TrainingQueue.Num());
	}

	return true;
}

void ABarracks::CancelTraining()
{
	TryCancelTraining();
}

bool ABarracks::TryCancelTraining()
{
	if (bIsUpdatingProduction || !bIsTraining || !IsValid(this)) return false;
	TGuardValue<bool> UpdatingProductionGuard(bIsUpdatingProduction, true);
	if (!TryRefundReservation(ActiveTrainingOrder)) return false;
	FinishCurrentOrder(EProductionEventType::Cancelled);
	return true;
}

bool ABarracks::TryRefundReservation(const FTrainingReservation& Reservation)
{
	const ARomanEmpireGameMode* GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	AFactionManager* FactionManager = GameMode ? GameMode->GetFactionManager() : nullptr;
	if (!IsValid(FactionManager) || FactionManager->GetWorld() != GetWorld()) return false;
	const FFactionResourceAmounts& Paid = Reservation.PaidCost;
	FFactionResourceDelta Refund;
	Refund.Gold = Paid.Gold; Refund.Food = Paid.Food; Refund.Iron = Paid.Iron;
	Refund.Wood = Paid.Wood; Refund.Stone = Paid.Stone; Refund.Population = Paid.Population;
	const FFactionResourceTransactionResult Result =
		FactionManager->ApplyFactionResourceDelta(Reservation.PayingFaction, Refund);
	return Result.WasSuccessful();
}

bool ABarracks::SpawnTrainedUnit()
{
	UWorld* World = GetWorld();
	if (!World || GetCurrentlyTrainingID().IsNone())
	{
		return false;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(GetCurrentlyTrainingID()) : nullptr;
	UClass* UnitClass = UnitAsset ? UnitAsset->UnitClass.Get() : nullptr;
	if (!UnitClass)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot spawn trained unit %s: factory class is unavailable"),
			*GetCurrentlyTrainingID().ToString());
		return false;
	}

	if (OwnerFaction != ActiveTrainingOrder.PayingFaction) return false;
	const FVector SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(SpawnOffset);
	const FRotator SpawnRotation = GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AUnitBase* NewUnit = World->SpawnActorDeferred<AUnitBase>(
		UnitClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	bool bCommitted = false;
	ON_SCOPE_EXIT { if (!bCommitted && IsValid(NewUnit)) NewUnit->Destroy(); };
	
	if (NewUnit && NewUnit->InitializeUnit(GetCurrentlyTrainingID(), OwnerFaction))
	{
		UGameplayStatics::FinishSpawningActor(NewUnit, SpawnTransform);
		if (IsValid(NewUnit) && NewUnit->IsAlive())
		{
			URomanUnitRegistrySubsystem* Units = World->GetSubsystem<URomanUnitRegistrySubsystem>();
			if (!Units || !Units->CreateSquad({ NewUnit }).IsValid()) return false;
			ARomanEmpireGameMode* Mode = World->GetAuthGameMode<ARomanEmpireGameMode>();
			ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
			UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (ActiveTrainingOrder.EncounterID.IsValid())
			{
				if (!Campaign || !Campaign->EnrollTrainedUnit(this, NewUnit, ActiveTrainingOrder)) return false;
			}
			else if (Battle && Battle->HasEncounter() && !Battle->RegisterReinforcement(NewUnit)) return false;
			bCommitted = true;
			UE_LOG(LogRomanEmpire, Log, TEXT("Trained unit: %s"), *NewUnit->GetName());
			return true;
		}
	}
	UE_LOG(LogRomanEmpire, Warning, TEXT("Training failed to create unit %s"), *GetCurrentlyTrainingID().ToString());
	return false;
}

FName ABarracks::StartNextInQueue()
{
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!TrainingQueue.IsEmpty())
	{
		const FTrainingReservation NextOrder = TrainingQueue[0];
		const FName NextUnitID = NextOrder.UnitID;
		TrainingQueue.RemoveAt(0);

		const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(NextUnitID) : nullptr;
		ActiveTrainingOrder = NextOrder;
		bIsTraining = true;
		TrainingProgress = 0.0f;
		if (!UnitAsset || !UnitAsset->UnitClass.Get() || !FMath::IsFinite(UnitAsset->Definition.TrainingTime) || UnitAsset->Definition.TrainingTime <= 0.0f)
		{
			// Promote the receipt, not an unchecked definition. Retry only its refund on Tick.
			bAwaitingFailureRefund = true;
			TrainingProgress = 1.0f;
			TrainingTime = 0.0f;
			UE_LOG(LogRomanEmpire, Warning, TEXT("Queued production definition %s is unavailable"),
				*NextUnitID.ToString());
		}
		else
		{
			TrainingTime = UnitAsset->Definition.TrainingTime;
		}
		return NextUnitID;
	}
	return NAME_None;
}

void ABarracks::FinishCurrentOrder(EProductionEventType Type)
{
	const FName FinishedUnitID = GetCurrentlyTrainingID();
	bIsTraining = false;
	TrainingProgress = 0.0f;
	TrainingTime = 0.0f;
	ActiveTrainingOrder = FTrainingReservation();
	bAwaitingFailureRefund = false;

	const FName StartedUnitID = StartNextInQueue();

	// Every observer sees the next active order and final pending count, never a half-advanced queue.
	BroadcastProductionEvent(Type, FinishedUnitID);
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

