// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingBase.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ABuildingBase::ABuildingBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Create building mesh
	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootScene);
	BuildingMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Create collision box
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootScene);
	CollisionBox->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	CollisionBox->SetCollisionProfileName(TEXT("BlockAll"));

	// Create scaffolding (visible during construction)
	ConstructionScaffolding = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConstructionScaffolding"));
	ConstructionScaffolding->SetupAttachment(RootScene);
	ConstructionScaffolding->SetVisibility(false);

	// Initialize state
	CurrentState = EBuildingState::Placing;
	OwnerFaction = EFactionID::None;
	ConstructionProgress = 0.0f;
	CurrentHealth = 1000;
	bIsSelected = false;

	// Default materials will be set in Blueprint
	ValidPlacementMaterial = nullptr;
	InvalidPlacementMaterial = nullptr;
	ConstructionMaterial = nullptr;
	CompleteMaterial = nullptr;
}

void ABuildingBase::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHealth = BuildingData.MaxHealth;
	UpdateVisuals();
}

void ABuildingBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentState == EBuildingState::Constructing)
	{
		UpdateConstruction(DeltaSeconds);
	}
}

void ABuildingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// Update collision box size based on footprint
	if (CollisionBox)
	{
		FVector Extent(
			BuildingData.FootprintSize.X * 50.0f,  // Convert to Unreal units
			BuildingData.FootprintSize.Y * 50.0f,
			200.0f  // Height
		);
		CollisionBox->SetBoxExtent(Extent);
	}
}

void ABuildingBase::SetOwnerFaction(EFactionID NewOwner)
{
	OwnerFaction = NewOwner;
	UE_LOG(LogRomanEmpire, Log, TEXT("Building %s now owned by faction %d"), 
		*GetName(), static_cast<int32>(NewOwner));
}

void ABuildingBase::StartConstruction()
{
	if (CurrentState == EBuildingState::Placing)
	{
		CurrentState = EBuildingState::Constructing;
		ConstructionProgress = 0.0f;
		
		if (ConstructionScaffolding)
		{
			ConstructionScaffolding->SetVisibility(true);
		}
		
		UpdateVisuals();
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Started construction of %s"), 
			*BuildingData.DisplayName.ToString());
	}
}

void ABuildingBase::UpdateConstruction(float DeltaSeconds)
{
	if (CurrentState != EBuildingState::Constructing)
	{
		return;
	}

	// Progress construction
	float ProgressRate = 1.0f / BuildingData.ConstructionTime;
	ConstructionProgress += ProgressRate * DeltaSeconds;

	// Visual progress - raise building from ground
	if (BuildingMesh)
	{
		FVector BaseLocation = RootScene->GetComponentLocation();
		float TargetZ = FMath::Lerp(-200.0f, 0.0f, ConstructionProgress);
		BuildingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, TargetZ));
	}

	if (ConstructionProgress >= 1.0f)
	{
		ConstructionProgress = 1.0f;
		OnConstructionFinished();
	}
}

void ABuildingBase::OnConstructionFinished()
{
	CurrentState = EBuildingState::Complete;
	
	if (ConstructionScaffolding)
	{
		ConstructionScaffolding->SetVisibility(false);
	}
	
	UpdateVisuals();
	OnConstructionComplete.Broadcast(this);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Construction complete: %s"), 
		*BuildingData.DisplayName.ToString());
}

void ABuildingBase::TakeDamage(float Damage, AActor* DamageSource)
{
	if (CurrentState == EBuildingState::Destroyed)
	{
		return;
	}

	// Apply armor reduction
	float ActualDamage = FMath::Max(0.0f, Damage - BuildingData.Armor);
	CurrentHealth = FMath::Max(0, CurrentHealth - FMath::RoundToInt(ActualDamage));

	OnBuildingDamaged.Broadcast(this, ActualDamage);

	if (CurrentHealth <= 0)
	{
		OnDestroyed();
	}
	else if (GetHealthPercent() < 0.5f)
	{
		CurrentState = EBuildingState::Damaged;
		UpdateVisuals();
	}
}

void ABuildingBase::Repair(float Amount)
{
	if (CurrentState == EBuildingState::Destroyed)
	{
		return;
	}

	CurrentHealth = FMath::Min(BuildingData.MaxHealth, CurrentHealth + FMath::RoundToInt(Amount));
	
	if (CurrentHealth > BuildingData.MaxHealth / 2)
	{
		CurrentState = EBuildingState::Complete;
		UpdateVisuals();
	}
}

void ABuildingBase::OnDestroyed()
{
	CurrentState = EBuildingState::Destroyed;
	OnBuildingDestroyed.Broadcast(this);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Building destroyed: %s"), 
		*BuildingData.DisplayName.ToString());
	
	// TODO: Spawn destruction effects, rubble, etc.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

bool ABuildingBase::CanPlaceAt(const FVector& Location) const
{
	// Check for overlapping actors
	TArray<AActor*> OverlappingActors;
	CollisionBox->GetOverlappingActors(OverlappingActors);
	
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor != this && Cast<ABuildingBase>(Actor))
		{
			return false; // Can't overlap other buildings
		}
	}
	
	// TODO: Check terrain type, resources, etc.
	return true;
}

void ABuildingBase::SetSelected(bool bNewSelected)
{
	bIsSelected = bNewSelected;
	// TODO: Show selection indicator
}

void ABuildingBase::SetPlacementMode(bool bPlacing, bool bValidPlacement)
{
	if (bPlacing)
	{
		CurrentState = EBuildingState::Placing;
		
		// Set ghost material
		UMaterialInterface* MaterialToUse = bValidPlacement ? ValidPlacementMaterial : InvalidPlacementMaterial;
		if (MaterialToUse && BuildingMesh)
		{
			BuildingMesh->SetMaterial(0, MaterialToUse);
		}
		
		// Disable collision during placement
		SetActorEnableCollision(false);
	}
	else
	{
		// Re-enable collision
		SetActorEnableCollision(true);
		UpdateVisuals();
	}
}

void ABuildingBase::UpdateVisuals()
{
	if (!BuildingMesh)
	{
		return;
	}

	UMaterialInterface* MaterialToUse = nullptr;
	
	switch (CurrentState)
	{
		case EBuildingState::Placing:
			MaterialToUse = ValidPlacementMaterial;
			break;
		case EBuildingState::Constructing:
			MaterialToUse = ConstructionMaterial;
			break;
		case EBuildingState::Complete:
		case EBuildingState::Damaged:
			MaterialToUse = CompleteMaterial;
			break;
		default:
			break;
	}
	
	if (MaterialToUse)
	{
		BuildingMesh->SetMaterial(0, MaterialToUse);
	}
}
