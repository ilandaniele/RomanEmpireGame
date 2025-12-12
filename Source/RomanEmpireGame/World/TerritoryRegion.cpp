// Copyright Roman Empire Game. All Rights Reserved.

#include "TerritoryRegion.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/Building/BuildingBase.h"
#include "RomanEmpireGame/Units/UnitBase.h"
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
	TerritoryID = NAME_None;
	TerrainType = ETerrainType::Plains;
	OwnerFaction = EFactionID::None;
	bHasSettlement = false;
	Population = 0;
	MaxSettlementSlots = 10;
	BonusResource = EResourceType::Gold;

	// Default resource production
	ResourceProduction.Gold = 50;
	ResourceProduction.Food = 30;
}

void ATerritoryRegion::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateTerritoryColor();
}

void ATerritoryRegion::SetOwnerFaction(EFactionID NewOwner)
{
	if (OwnerFaction != NewOwner)
	{
		OwnerFaction = NewOwner;
		UpdateTerritoryColor();
		OnTerritoryOwnerChanged.Broadcast(this, NewOwner);
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Territory %s now owned by faction %d"), 
			*TerritoryID.ToString(), static_cast<int32>(NewOwner));
	}
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
		UE_LOG(LogRomanEmpire, Warning, TEXT("Territory %s already has a settlement"), *TerritoryID.ToString());
		return;
	}

	bHasSettlement = true;
	SettlementName = Name;
	Population = 100; // Starting population

	// Boost resource production with settlement
	ResourceProduction.Gold += 100;
	ResourceProduction.Food += 50;

	OnSettlementFounded.Broadcast(this);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Settlement %s founded in territory %s"), 
		*Name.ToString(), *TerritoryID.ToString());
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
				*TerritoryID.ToString());
		}
	}
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

	FLinearColor Color;
	switch (OwnerFaction)
	{
		case EFactionID::Rome:
			Color = FLinearColor(0.8f, 0.1f, 0.1f, 0.3f); // Red
			break;
		case EFactionID::Carthage:
			Color = FLinearColor(0.5f, 0.0f, 0.5f, 0.3f); // Purple
			break;
		case EFactionID::Gaul:
			Color = FLinearColor(0.1f, 0.6f, 0.1f, 0.3f); // Green
			break;
		default:
			Color = FLinearColor(0.5f, 0.5f, 0.5f, 0.3f); // Gray for neutral
			break;
	}

	// Apply color through dynamic material
	// TerritoryMesh->SetVectorParameterValueOnMaterials(TEXT("FactionColor"), Color);
}
