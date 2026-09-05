// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingPlacementComponent.h"
#include "BuildingBase.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Core/RomanEmpireGameMode.h"
#include "Faction/FactionManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UBuildingPlacementComponent::UBuildingPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	bIsPlacing = false;
	bCanPlace = false;
	CurrentBuildingClass = nullptr;
	PreviewBuilding = nullptr;
	CurrentRotation = 0.0f;
	GridSize = 100.0f;   // 1 meter grid
	bSnapToGrid = true;
	LastTransactionFailure = EResourceTransactionFailure::None;
}

void UBuildingPlacementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuildingPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsPlacing && PreviewBuilding)
	{
		// Continuously validate placement
		bCanPlace = ValidatePlacement();
		PreviewBuilding->SetPlacementMode(true, bCanPlace);
	}
}

void UBuildingPlacementComponent::StartPlacement(FName BuildingID)
{
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanBuildingDefinition* Asset = Registry ? Registry->FindBuilding(BuildingID) : nullptr;
	UClass* BuildingClass = Asset ? Asset->BuildingClass.Get() : nullptr;
	if (!Registry || !Registry->IsReady() || !Asset || !BuildingClass)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot start placement: unknown building ID %s"),
			*BuildingID.ToString());
		return;
	}

	// Cancel any existing placement
	if (bIsPlacing)
	{
		CancelPlacement();
	}

	CurrentBuildingClass = BuildingClass;
	CurrentBuildingID = BuildingID;
	CurrentRotation = 0.0f;
	bIsPlacing = true;
	LastTransactionFailure = EResourceTransactionFailure::None;

	CreatePreviewBuilding();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f,
			PreviewBuilding ? FColor::Green : FColor::Red,
			FString::Printf(TEXT("BuildPlace: %s preview=%s"),
				*BuildingID.ToString(),
				PreviewBuilding ? TEXT("OK") : TEXT("FAILED")));
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Started placing building: %s"), *BuildingID.ToString());
}

void UBuildingPlacementComponent::UpdatePreview(const FVector& WorldLocation)
{
	if (!bIsPlacing || !PreviewBuilding)
	{
		return;
	}

	// Snap to grid if enabled
	FVector SnappedLocation = bSnapToGrid ? SnapToGrid(WorldLocation) : WorldLocation;
	CurrentPlacementLocation = SnappedLocation;

	// Update preview position and rotation
	FRotator Rotation(0.0f, CurrentRotation, 0.0f);
	PreviewBuilding->SetActorLocationAndRotation(SnappedLocation, Rotation);

	// Validate placement
	bCanPlace = ValidatePlacement();
	PreviewBuilding->SetPlacementMode(true, bCanPlace);
}

void UBuildingPlacementComponent::RotatePreview(float Degrees)
{
	if (!bIsPlacing)
	{
		return;
	}

	CurrentRotation = FMath::Fmod(CurrentRotation + Degrees, 360.0f);
	
	if (PreviewBuilding)
	{
		FRotator Rotation(0.0f, CurrentRotation, 0.0f);
		PreviewBuilding->SetActorRotation(Rotation);
	}
}

bool UBuildingPlacementComponent::ConfirmPlacement()
{
	if (!bIsPlacing || !bCanPlace || !CurrentBuildingClass || CurrentBuildingID.IsNone())
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot confirm placement: invalid state"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ARomanEmpireGameMode* GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	AFactionManager* FactionManager = GameMode ? GameMode->GetFactionManager() : nullptr;
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanBuildingDefinition* Asset = Registry ? Registry->FindBuilding(CurrentBuildingID) : nullptr;
	if (!FactionManager || !Asset)
	{
		LastTransactionFailure = EResourceTransactionFailure::InvalidFaction;
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot confirm placement: missing faction resource authority"));
		return false;
	}

	const FRotator Rotation(0.0f, CurrentRotation, 0.0f);
	const FTransform PlacementTransform(Rotation, CurrentPlacementLocation);
	ABuildingBase* NewBuilding = World->SpawnActorDeferred<ABuildingBase>(
		CurrentBuildingClass,
		PlacementTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewBuilding)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot confirm placement: deferred building spawn failed"));
		return false;
	}

	const EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	const FFactionResourceTransactionResult SpendResult =
		FactionManager->TrySpendFactionResources(PlayerFaction, Asset->Definition.Cost);
	LastTransactionFailure = SpendResult.Failure;
	if (!SpendResult.WasSuccessful())
	{
		NewBuilding->Destroy();
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot confirm placement: resource transaction failed (%s)"),
			RomanEmpireEconomy::GetTransactionFailureName(SpendResult.Failure));
		return false;
	}

	if (!NewBuilding->InitializeBuilding(CurrentBuildingID, PlayerFaction))
	{
		FFactionResourceDelta Refund;
		Refund.Gold = Asset->Definition.Cost.Gold;
		Refund.Food = Asset->Definition.Cost.Food;
		Refund.Iron = Asset->Definition.Cost.Iron;
		Refund.Wood = Asset->Definition.Cost.Wood;
		Refund.Stone = Asset->Definition.Cost.Stone;
		Refund.Population = Asset->Definition.Cost.Population;
		FactionManager->ApplyFactionResourceDelta(PlayerFaction, Refund);
		NewBuilding->Destroy();
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot confirm placement: building definition initialization failed"));
		return false;
	}
	UGameplayStatics::FinishSpawningActor(NewBuilding, PlacementTransform);
	NewBuilding->StartConstruction();

	DestroyPreviewBuilding();
	OnBuildingPlaced.Broadcast(NewBuilding);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
			FString::Printf(TEXT("Building placed: %s"), *NewBuilding->GetName()));
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Building placed and construction started: %s"),
		*NewBuilding->GetName());

	// Reset state
	bIsPlacing = false;
	bCanPlace = false;
	CurrentBuildingClass = nullptr;
	CurrentBuildingID = NAME_None;

	return true;
}

void UBuildingPlacementComponent::CancelPlacement()
{
	if (!bIsPlacing)
	{
		return;
	}

	DestroyPreviewBuilding();

	bIsPlacing = false;
	bCanPlace = false;
	CurrentBuildingClass = nullptr;
	CurrentBuildingID = NAME_None;
	CurrentRotation = 0.0f;

	OnPlacementCancelled.Broadcast();

	UE_LOG(LogRomanEmpire, Log, TEXT("Building placement cancelled"));
}

void UBuildingPlacementComponent::CreatePreviewBuilding()
{
	UWorld* World = GetWorld();
	if (!World || !CurrentBuildingClass)
	{
		return;
	}

	const FTransform PreviewTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	PreviewBuilding = World->SpawnActorDeferred<ABuildingBase>(
		CurrentBuildingClass,
		PreviewTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (PreviewBuilding)
	{
		const ARomanEmpireGameMode* GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
		const AFactionManager* FactionManager = GameMode ? GameMode->GetFactionManager() : nullptr;
		const EFactionID PreviewFaction = FactionManager ? FactionManager->GetPlayerFaction() : EFactionID::None;
		if (!PreviewBuilding->InitializeBuilding(CurrentBuildingID, PreviewFaction))
		{
			PreviewBuilding->Destroy();
			PreviewBuilding = nullptr;
			return;
		}
		UGameplayStatics::FinishSpawningActor(PreviewBuilding, PreviewTransform);
		PreviewBuilding->SetPlacementMode(true, false);
	}
}

void UBuildingPlacementComponent::DestroyPreviewBuilding()
{
	if (PreviewBuilding)
	{
		PreviewBuilding->Destroy();
		PreviewBuilding = nullptr;
	}
}

FVector UBuildingPlacementComponent::SnapToGrid(const FVector& Location) const
{
	if (!bSnapToGrid || GridSize <= 0.0f)
	{
		return Location;
	}

	float SnappedX = FMath::RoundToFloat(Location.X / GridSize) * GridSize;
	float SnappedY = FMath::RoundToFloat(Location.Y / GridSize) * GridSize;
	
	return FVector(SnappedX, SnappedY, Location.Z);
}

bool UBuildingPlacementComponent::ValidatePlacement()
{
	if (!PreviewBuilding)
	{
		return false;
	}

	// For now, all placements are valid (on ground)
	return true;
}

