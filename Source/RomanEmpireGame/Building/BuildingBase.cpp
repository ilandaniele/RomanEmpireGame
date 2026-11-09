// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingBase.h"
#include "Building/BuildingPresentation.h"
#include "Audio/RomanFeedbackSubsystem.h"
#include "Core/RomanEmpireGameState.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Engine/World.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

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

	// Assign a visible cube mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		BuildingMesh->SetStaticMesh(CubeMesh.Object);
		PrototypeMesh = CubeMesh.Object;
		BuildingMesh->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f)); // 200x200x200cm building
	}

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
	CurrentHealth = 0;
	bIsSelected = false;

	// Reflected class references make the presentation assets reachable during cooking.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Valid(TEXT("/Game/Materials/RomanVisual/MI_PreviewValid"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Invalid(TEXT("/Game/Materials/RomanVisual/MI_PreviewInvalid"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Construction(TEXT("/Game/Materials/RomanVisual/MI_Construction"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Complete(TEXT("/Game/Materials/RomanVisual/MI_Surface"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Damaged(TEXT("/Game/Materials/RomanVisual/MI_Damaged"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Indicator(TEXT("/Game/Materials/RomanVisual/MI_Selection"));
	ValidPlacementMaterial = Valid.Object;
	InvalidPlacementMaterial = Invalid.Object;
	ConstructionMaterial = Construction.Object;
	CompleteMaterial = Complete.Object;
	DamagedMaterial = Damaged.Object;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UStaticMeshComponent* Edge = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("SelectionOutline%d"), Index));
		Edge->SetupAttachment(RootScene);
		Edge->SetStaticMesh(CubeMesh.Object);
		Edge->SetMaterial(0, Indicator.Object);
		Edge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Edge->SetCanEverAffectNavigation(false);
		Edge->SetCastShadow(false);
		Edge->SetVisibility(false);
		Edge->SetHiddenInGame(true);
		SelectionOutline.Add(Edge);
	}
}

void ABuildingBase::BeginPlay()
{
	Super::BeginPlay();

	if (!bBuildingInitialized && !InitializeBuilding(BuildingDefinitionID, OwnerFaction))
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Building %s has no valid content definition"), *GetName());
		SetActorTickEnabled(false);
		return;
	}

	CurrentHealth = BuildingData.MaxHealth;
	ApplyDefinitionPresentation();
	UpdateVisuals();
}

void ABuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bBuildingEndingPlay = true;
	SetSelected(false);
	// Remove local selection even for level removal, which need not fire OnDestroyed.
	OnBuildingStateChanged.Broadcast(this);
	Super::EndPlay(EndPlayReason);
}

bool ABuildingBase::IsSelectionReady() const
{
	return bBuildingInitialized && !bBuildingEndingPlay && HasActorBegunPlay() &&
		!IsActorBeingDestroyed() && CurrentHealth > 0 &&
		CurrentState != EBuildingState::Placing && CurrentState != EBuildingState::Destroyed;
}

bool ABuildingBase::InitializeBuilding(FName DefinitionID, EFactionID NewOwner)
{
	if (bBuildingInitialized || !RomanEmpireContent::IsStableID(DefinitionID))
	{
		return false;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanBuildingDefinition* Asset = Registry ? Registry->FindBuilding(DefinitionID) : nullptr;
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(NewOwner) : nullptr;
	if (!Registry || !Registry->IsReady() || !Asset ||
		(NewOwner != EFactionID::None &&
			(!Faction || !Asset->AvailableToFactions.Contains(Faction->Definition.FactionID))))
	{
		return false;
	}
	if (RomanEmpireBuildingPresentation::Validate(Asset->Definition) != RomanEmpireBuildingPresentation::EFailure::None)
	{
		return false;
	}

	BuildingDefinitionID = DefinitionID;
	BuildingData = Asset->Definition;
	OwnerFaction = NewOwner;
	bBuildingInitialized = true;
	ApplyDefinitionPresentation();
	return true;
}

void ABuildingBase::ApplyDefinitionPresentation()
{
	if (!BuildingMesh || !bBuildingInitialized)
	{
		return;
	}

	const bool bAuthored = UsesAuthoredPresentation();
	if (bAuthored)
	{
		// BeginPlay-only factories must receive the same definition-owned box as deferred placement.
		CollisionBox->SetBoxExtent(FVector(BuildingData.FootprintSize.X * 50.0f, BuildingData.FootprintSize.Y * 50.0f, RomanEmpireBuildingPresentation::PlacementHalfHeight));
		if (!bAuthoredPresentationApplied)
		{
			BuildingMesh->SetStaticMesh(BuildingData.PresentationMesh);
			BuildingMesh->EmptyOverrideMaterials();
			BuildingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			BuildingMesh->SetCanEverAffectNavigation(false);
			// Keep state/faction feedback separate from every authored PBR slot.
			ConstructionScaffolding->SetStaticMesh(PrototypeMesh);
			ConstructionScaffolding->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ConstructionScaffolding->SetCanEverAffectNavigation(false);
			ConstructionScaffolding->SetCastShadow(false);
			bAuthoredPresentationApplied = true;
		}
		FTransform Transform = BuildingData.PresentationTransform;
		if (CurrentState == EBuildingState::Constructing)
			Transform.AddToTranslation(FVector(0, 0, FMath::Lerp(-200.0f, 0.0f, FMath::Clamp(ConstructionProgress, 0.0f, 1.0f))));
		BuildingMesh->SetRelativeTransform(Transform);
		ConstructionScaffolding->SetRelativeLocation(FVector(0, 0, -RomanEmpireBuildingPresentation::PlacementHalfHeight + 3));
		ConstructionScaffolding->SetRelativeScale3D(FVector(BuildingData.FootprintSize.X, BuildingData.FootprintSize.Y, .035));
	}
	else BuildingMesh->SetRelativeScale3D(BuildingData.PrototypeScale);
	const float HalfX = (bAuthored ? BuildingData.FootprintSize.X * 50.0f : FMath::Max(BuildingData.FootprintSize.X * 50.0f, BuildingData.PrototypeScale.X * 50.0f)) + 8.0f;
	const float HalfY = (bAuthored ? BuildingData.FootprintSize.Y * 50.0f : FMath::Max(BuildingData.FootprintSize.Y * 50.0f, BuildingData.PrototypeScale.Y * 50.0f)) + 8.0f;
	const float BaseZ = -(bAuthored ? RomanEmpireBuildingPresentation::PlacementHalfHeight : BuildingData.PrototypeScale.Z * 50.0f) + 3.0f;
	for (int32 Index = 0; Index < SelectionOutline.Num(); ++Index)
	{
		const bool bHorizontal = Index < 2;
		const float Sign = Index % 2 == 0 ? -1.0f : 1.0f;
		SelectionOutline[Index]->SetRelativeLocation(bHorizontal ? FVector(0, Sign * HalfY, BaseZ) : FVector(Sign * HalfX, 0, BaseZ));
		SelectionOutline[Index]->SetRelativeScale3D(bHorizontal ? FVector((HalfX * 2 + 8) / 100, 0.08f, 0.04f) : FVector(0.08f, HalfY * 2 / 100, 0.04f));
	}
	UpdateVisuals();
}

void ABuildingBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (CurrentState == EBuildingState::Constructing && !(Battle && Battle->IsResolved()))
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
			RomanEmpireBuildingPresentation::PlacementHalfHeight
		);
		CollisionBox->SetBoxExtent(Extent);
	}
	if (UsesAuthoredPresentation()) ApplyDefinitionPresentation();
}

void ABuildingBase::SetOwnerFaction(EFactionID NewOwner)
{
	if (OwnerFaction == NewOwner || bBuildingEndingPlay || IsActorBeingDestroyed() || CurrentState == EBuildingState::Destroyed)
	{
		return;
	}
	OwnerFaction = NewOwner;
	UpdateVisuals();
	OnBuildingStateChanged.Broadcast(this);
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
		OnBuildingStateChanged.Broadcast(this);
	}
}

void ABuildingBase::CompleteInitialConstruction()
{
	if (!bBuildingInitialized || !HasActorBegunPlay() || CurrentState != EBuildingState::Placing) return;
	StartConstruction();
	ConstructionProgress = 1.0f;
	if (BuildingMesh) BuildingMesh->SetRelativeLocation(UsesAuthoredPresentation() ? BuildingData.PresentationTransform.GetTranslation() : FVector::ZeroVector);
	OnConstructionFinished();
}

void ABuildingBase::UpdateConstruction(float DeltaSeconds)
{
	if (CurrentState != EBuildingState::Constructing)
	{
		return;
	}

	// Progress construction
	const int32 PreviousPercent = FMath::FloorToInt(ConstructionProgress * 100.0f);
	float ProgressRate = 1.0f / BuildingData.ConstructionTime;
	ConstructionProgress += ProgressRate * DeltaSeconds;

	// Visual progress - raise building from ground
	if (BuildingMesh)
	{
		const bool bAuthored = UsesAuthoredPresentation();
		const float Progress = bAuthored ? FMath::Clamp(ConstructionProgress, 0.0f, 1.0f) : ConstructionProgress;
		const float TargetZ = FMath::Lerp(-200.0f, 0.0f, Progress);
		const FVector Base = bAuthored ? BuildingData.PresentationTransform.GetTranslation() : FVector::ZeroVector;
		BuildingMesh->SetRelativeLocation(Base + FVector(0.0f, 0.0f, TargetZ));
	}

	if (ConstructionProgress >= 1.0f)
	{
		ConstructionProgress = 1.0f;
		OnConstructionFinished();
	}
	else if (PreviousPercent != FMath::FloorToInt(ConstructionProgress * 100.0f))
	{
		OnBuildingStateChanged.Broadcast(this);
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
	OnBuildingStateChanged.Broadcast(this);
	UWorld* World = GetWorld();
	const auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
	if (!IsActorBeingDestroyed() && CurrentState == EBuildingState::Complete && State && OwnerFaction == State->GetPlayerFaction())
		if (auto* Audio = World->GetSubsystem<URomanFeedbackSubsystem>()) Audio->PlayCue(ERomanFeedbackCue::Complete);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Construction complete: %s"), 
		*BuildingData.DisplayName.ToString());
}

void ABuildingBase::ApplyBuildingDamage(float Damage, AActor* DamageSource)
{
	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	if (CurrentState == EBuildingState::Destroyed || (Battle && Battle->IsResolved()) || !FMath::IsFinite(Damage) || Damage <= 0.0f)
	{
		return;
	}

	// Apply armor reduction
	float ActualDamage = FMath::Max(0.0f, Damage - BuildingData.Armor);
	CurrentHealth = FMath::Max(0, CurrentHealth - FMath::RoundToInt(ActualDamage));
	UpdateVisuals();

	OnBuildingDamaged.Broadcast(this, ActualDamage);

	if (CurrentHealth <= 0)
	{
		OnDestroyed();
	}
	else if (GetHealthPercent() < 0.5f)
	{
		// Construction must keep progressing when damaged before completion.
		if (CurrentState == EBuildingState::Complete)
		{
			CurrentState = EBuildingState::Damaged;
		}
	}
	UpdateVisuals();
	OnBuildingStateChanged.Broadcast(this);
}

void ABuildingBase::Repair(float Amount)
{
	if (CurrentState == EBuildingState::Destroyed)
	{
		return;
	}

	CurrentHealth = FMath::Min(BuildingData.MaxHealth, CurrentHealth + FMath::RoundToInt(Amount));
	
	if (CurrentHealth > BuildingData.MaxHealth / 2 && CurrentState == EBuildingState::Damaged)
	{
		CurrentState = EBuildingState::Complete;
	}
	UpdateVisuals();
	OnBuildingStateChanged.Broadcast(this);
}

void ABuildingBase::OnDestroyed()
{
	CurrentState = EBuildingState::Destroyed;
	SetSelected(false);
	OnBuildingDestroyed.Broadcast(this);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Building destroyed: %s"), 
		*BuildingData.DisplayName.ToString());
	
	// TODO: Spawn destruction effects, rubble, etc.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

FVector ABuildingBase::GetPlacementHalfExtent() const
{
	return CollisionBox ? CollisionBox->GetScaledBoxExtent() : FVector::ZeroVector;
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
	for (UStaticMeshComponent* Edge : SelectionOutline)
	{
		Edge->SetVisibility(bIsSelected);
		Edge->SetHiddenInGame(!bIsSelected);
	}
}

void ABuildingBase::SetPlacementMode(bool bPlacing, bool bValidPlacement)
{
	if (bBuildingEndingPlay || IsActorBeingDestroyed() || CurrentState == EBuildingState::Destroyed) return;
	const EBuildingState PreviousState = CurrentState;
	if (bPlacing)
	{
		CurrentState = EBuildingState::Placing;
		
		bPlacementVisualValid = bValidPlacement;
		UpdateVisuals();
		
		// Disable collision during placement
		SetActorEnableCollision(false);
	}
	else
	{
		// Re-enable collision
		SetActorEnableCollision(true);
		UpdateVisuals();
	}
	if (CurrentState != PreviousState) OnBuildingStateChanged.Broadcast(this);
}

void ABuildingBase::UpdateVisuals()
{
	if (!BuildingMesh || !bBuildingInitialized || bBuildingEndingPlay)
	{
		return;
	}

	UMaterialInterface* MaterialToUse = nullptr;
	
	switch (CurrentState)
	{
		case EBuildingState::Placing:
			MaterialToUse = bPlacementVisualValid ? ValidPlacementMaterial : InvalidPlacementMaterial;
			break;
		case EBuildingState::Constructing:
			MaterialToUse = ConstructionMaterial;
			break;
		case EBuildingState::Complete:
			MaterialToUse = CompleteMaterial;
			break;
		case EBuildingState::Damaged:
			MaterialToUse = DamagedMaterial;
			break;
		default:
			break;
	}
	
	if (MaterialToUse)
	{
		UMaterialInstanceDynamic* MID = VisualMaterials.FindRef(MaterialToUse);
		if (!MID)
		{
			MID = UMaterialInstanceDynamic::Create(MaterialToUse, this);
			if (!MID) return;
			VisualMaterials.Add(MaterialToUse, MID);
		}
		if (CurrentState == EBuildingState::Complete || CurrentState == EBuildingState::Damaged)
		{
			FLinearColor Color = BuildingData.PrototypeColor;
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
			const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(OwnerFaction) : nullptr;
			if (Faction) Color = FMath::Lerp(Color, Faction->Definition.FactionColor, 0.25f);
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetScalarParameterValue(TEXT("Damage"), 1.0f - GetHealthPercent());
		}
		if (UsesAuthoredPresentation())
		{
			ConstructionScaffolding->SetMaterial(0, MID);
			ConstructionScaffolding->SetVisibility(CurrentState != EBuildingState::Destroyed);
		}
		else BuildingMesh->SetMaterial(0, MID);
	}
}

