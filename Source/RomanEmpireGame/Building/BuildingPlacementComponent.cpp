// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingPlacementComponent.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/Building/BuildingBase.h"
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

void UBuildingPlacementComponent::StartPlacement(TSubclassOf<ABuildingBase> BuildingClass)
{
	if (!BuildingClass)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot start placement: null building class"));
		return;
	}

	// Cancel any existing placement
	if (bIsPlacing)
	{
		CancelPlacement();
	}

	CurrentBuildingClass = BuildingClass;
	CurrentRotation = 0.0f;
	bIsPlacing = true;

	CreatePreviewBuilding();

	UE_LOG(LogRomanEmpire, Log, TEXT("Started placing building: %s"), *BuildingClass->GetName());
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
	if (!bIsPlacing || !bCanPlace || !CurrentBuildingClass)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot confirm placement: invalid state"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Destroy preview
	DestroyPreviewBuilding();

	// Spawn actual building
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FRotator Rotation(0.0f, CurrentRotation, 0.0f);
	ABuildingBase* NewBuilding = World->SpawnActor<ABuildingBase>(
		CurrentBuildingClass, 
		CurrentPlacementLocation, 
		Rotation, 
		SpawnParams
	);

	if (NewBuilding)
	{
		// Set owner faction (TODO: Get from player controller)
		NewBuilding->SetOwnerFaction(EFactionID::Rome);
		
		// Start construction
		NewBuilding->StartConstruction();
		
		OnBuildingPlaced.Broadcast(NewBuilding);
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Building placed and construction started: %s"), 
			*NewBuilding->GetName());
	}

	// Reset state
	bIsPlacing = false;
	bCanPlace = false;
	CurrentBuildingClass = nullptr;

	return NewBuilding != nullptr;
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

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PreviewBuilding = World->SpawnActor<ABuildingBase>(
		CurrentBuildingClass, 
		FVector::ZeroVector, 
		FRotator::ZeroRotator, 
		SpawnParams
	);

	if (PreviewBuilding)
	{
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

	// Check if building can be placed at current location
	if (!PreviewBuilding->CanPlaceAt(CurrentPlacementLocation))
	{
		return false;
	}

	// TODO: Additional checks:
	// - Resource requirements
	// - Within territory
	// - Not in water
	// - Sufficient distance from other buildings

	return true;
}
