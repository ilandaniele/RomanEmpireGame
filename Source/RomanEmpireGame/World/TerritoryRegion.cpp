// Copyright Roman Empire Game. All Rights Reserved.

#include "TerritoryRegion.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Building/BuildingBase.h"
#include "Units/UnitBase.h"
#include "World/WorldMapManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ATerritoryRegion::ATerritoryRegion()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create bounds for territory detection
	TerritoryBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("TerritoryBounds"));
	SetRootComponent(TerritoryBounds);
	TerritoryBounds->SetBoxExtent(FVector(5000.0f, 5000.0f, 1000.0f)); // 100m x 100m territory
	TerritoryBounds->SetCollisionProfileName(TEXT("OverlapAll"));

	// Create visual mesh
	TerritoryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerritoryMesh"));
	TerritoryMesh->SetupAttachment(TerritoryBounds);
	TerritoryMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// Initialize
	WorldMapManager = nullptr;
	bHasSettlement = false;
	Population = 0;
	MaxSettlementSlots = 10;
}

void ATerritoryRegion::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateTerritoryColor();
}

bool ATerritoryRegion::InitializeTerritory(
	const FTerritoryDefinition& Definition,
	AWorldMapManager* InWorldMapManager)
{
	if (!Definition.IsValid() || !InWorldMapManager)
	{
		return false;
	}
	if (bTerritoryInitialized)
	{
		return WorldMapManager == InWorldMapManager &&
			TerritoryDefinition.TerritoryID == Definition.TerritoryID;
	}

	TerritoryDefinition = Definition;
	WorldMapManager = InWorldMapManager;
	bTerritoryInitialized = true;
	UpdateTerritoryColor();
	return true;
}

EFactionID ATerritoryRegion::GetOwnerFaction() const
{
	return WorldMapManager
		? WorldMapManager->GetTerritoryOwner(TerritoryDefinition.TerritoryID)
		: TerritoryDefinition.StartingOwner;
}

FTerritoryDefinition ATerritoryRegion::GetDefinition() const
{
	return WorldMapManager
		? WorldMapManager->GetTerritoryDefinition(TerritoryDefinition.TerritoryID)
		: TerritoryDefinition;
}

FText ATerritoryRegion::GetDisplayName() const
{
	return GetDefinition().DisplayName;
}

ETerrainType ATerritoryRegion::GetTerrainType() const
{
	return GetDefinition().TerrainType;
}

void ATerritoryRegion::RefreshOwnershipPresentation(
	EFactionID PreviousOwner,
	EFactionID NewOwner)
{
	UpdateTerritoryColor();
	OnTerritoryOwnerChanged.Broadcast(this, NewOwner);
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Territory actor %s refreshed owner %d -> %d"),
		*TerritoryDefinition.TerritoryID.ToString(),
		static_cast<int32>(PreviousOwner),
		static_cast<int32>(NewOwner));
}

FFactionResourceDelta ATerritoryRegion::GetResourceProduction() const
{
	FFactionResourceDelta Production = GetDefinition().ResourceProduction;
	Production.Gold += SettlementProductionBonus.Gold;
	Production.Food += SettlementProductionBonus.Food;
	Production.Iron += SettlementProductionBonus.Iron;
	Production.Wood += SettlementProductionBonus.Wood;
	Production.Stone += SettlementProductionBonus.Stone;
	Production.Population += SettlementProductionBonus.Population;
	return Production;
}

bool ATerritoryRegion::IsContested() const
{
	TArray<AUnitBase*> UnitsInTerritory = GetUnitsInTerritory();
	
	TSet<EFactionID> FactionsPresent;
	for (AUnitBase* Unit : UnitsInTerritory)
	{
		if (Unit && Unit->IsAlive())
		{
			FactionsPresent.Add(Unit->GetOwnerFaction());
		}
	}
	
	// Contested if multiple factions present
	return FactionsPresent.Num() > 1;
}

void ATerritoryRegion::FoundSettlement(const FText& Name)
{
	if (bHasSettlement)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Territory %s already has a settlement"),
			*TerritoryDefinition.TerritoryID.ToString());
		return;
	}

	bHasSettlement = true;
	SettlementName = Name;
	Population = 100; // Starting population

	// Boost resource production with settlement
	SettlementProductionBonus.Gold = 100;
	SettlementProductionBonus.Food = 50;

	OnSettlementFounded.Broadcast(this);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Settlement %s founded in territory %s"), 
		*Name.ToString(), *TerritoryDefinition.TerritoryID.ToString());
}

void ATerritoryRegion::RegisterBuilding(ABuildingBase* Building)
{
	if (Building && !Buildings.Contains(Building))
	{
		if (Buildings.Num() < MaxSettlementSlots)
		{
			Buildings.Add(Building);
		}
		else
		{
			UE_LOG(LogRomanEmpire, Warning, TEXT("Territory %s is at max building capacity"), 
				*TerritoryDefinition.TerritoryID.ToString());
		}
	}
}

TArray<ABuildingBase*> ATerritoryRegion::GetBuildings() const
{
	TArray<ABuildingBase*> Result;
	Result.Reserve(Buildings.Num());
	for (ABuildingBase* Building : Buildings)
	{
		Result.Add(Building);
	}
	return Result;
}

TArray<AUnitBase*> ATerritoryRegion::GetUnitsInTerritory() const
{
	TArray<AUnitBase*> Units;
	
	TArray<AActor*> OverlappingActors;
	TerritoryBounds->GetOverlappingActors(OverlappingActors, AUnitBase::StaticClass());
	
	for (AActor* Actor : OverlappingActors)
	{
		if (AUnitBase* Unit = Cast<AUnitBase>(Actor))
		{
			Units.Add(Unit);
		}
	}
	
	return Units;
}

void ATerritoryRegion::UpdateTerritoryColor()
{
	if (!TerritoryMesh)
	{
		return;
	}

	// TODO: Apply faction color to territory mesh material
	// Will need dynamic material instances

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(GetOwnerFaction()) : nullptr;
	FLinearColor Color = Faction ? Faction->Definition.FactionColor : FLinearColor::Gray;
	Color.A = 0.3f;

	// Apply color through dynamic material
	// TerritoryMesh->SetVectorParameterValueOnMaterials(TEXT("FactionColor"), Color);
}

