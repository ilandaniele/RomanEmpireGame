// Copyright Roman Empire Game. All Rights Reserved.

#include "WorldMapManager.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/World/TerritoryRegion.h"
#include "Kismet/GameplayStatics.h"

AWorldMapManager::AWorldMapManager()
{
	PrimaryActorTick.bCanEverTick = false;

	MapWidth = 5;
	MapHeight = 3;
	TerritorySize = 10000.0f; // 100 meters per territory
}

void AWorldMapManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Find existing territories or generate map
	TArray<AActor*> FoundTerritories;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATerritoryRegion::StaticClass(), FoundTerritories);
	
	if (FoundTerritories.Num() == 0)
	{
		GenerateDefaultMap();
	}
	else
	{
		for (AActor* Actor : FoundTerritories)
		{
			if (ATerritoryRegion* Territory = Cast<ATerritoryRegion>(Actor))
			{
				Territories.Add(Territory);
			}
		}
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("World Map Manager initialized with %d territories"), Territories.Num());
}

void AWorldMapManager::GenerateDefaultMap()
{
	// Generate a small Mediterranean-style map
	// Layout:
	//   [Gaul]  [Alps]  [Greece]
	//   [Hispania] [Rome] [Macedonia]  
	//   [Carthage] [Sicily] [Egypt]

	// Row 1 - Northern territories
	CreateTerritory(TEXT("Gaul"), 
		FText::FromString(TEXT("Gallia")), 
		FVector(-TerritorySize, TerritorySize, 0), 
		EFactionID::Gaul);

	CreateTerritory(TEXT("Alps"), 
		FText::FromString(TEXT("Alpine Passes")), 
		FVector(0, TerritorySize, 0), 
		EFactionID::None);

	CreateTerritory(TEXT("Greece"), 
		FText::FromString(TEXT("Graecia")), 
		FVector(TerritorySize, TerritorySize, 0), 
		EFactionID::None);

	// Row 2 - Central territories
	CreateTerritory(TEXT("Hispania"), 
		FText::FromString(TEXT("Hispania")), 
		FVector(-TerritorySize, 0, 0), 
		EFactionID::None);

	// Rome - center of the map
	ATerritoryRegion* RomeTerritory = nullptr;
	CreateTerritory(TEXT("Rome"), 
		FText::FromString(TEXT("Italia")), 
		FVector(0, 0, 0), 
		EFactionID::Rome);
	
	// Find Rome and found its settlement
	RomeTerritory = GetTerritoryByID(TEXT("Rome"));
	if (RomeTerritory)
	{
		RomeTerritory->FoundSettlement(FText::FromString(TEXT("Roma")));
	}

	CreateTerritory(TEXT("Macedonia"), 
		FText::FromString(TEXT("Macedonia")), 
		FVector(TerritorySize, 0, 0), 
		EFactionID::None);

	// Row 3 - Southern territories
	ATerritoryRegion* CarthageTerritory = nullptr;
	CreateTerritory(TEXT("Africa"), 
		FText::FromString(TEXT("Africa")), 
		FVector(-TerritorySize, -TerritorySize, 0), 
		EFactionID::Carthage);
	
	CarthageTerritory = GetTerritoryByID(TEXT("Africa"));
	if (CarthageTerritory)
	{
		CarthageTerritory->FoundSettlement(FText::FromString(TEXT("Carthago")));
	}

	CreateTerritory(TEXT("Sicily"), 
		FText::FromString(TEXT("Sicilia")), 
		FVector(0, -TerritorySize, 0), 
		EFactionID::None);

	CreateTerritory(TEXT("Egypt"), 
		FText::FromString(TEXT("Aegyptus")), 
		FVector(TerritorySize, -TerritorySize, 0), 
		EFactionID::None);

	UE_LOG(LogRomanEmpire, Log, TEXT("Generated default Mediterranean map with %d territories"), Territories.Num());
}

void AWorldMapManager::CreateTerritory(FName ID, const FText& Name, const FVector& Location, EFactionID StartingOwner)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ATerritoryRegion* NewTerritory = World->SpawnActor<ATerritoryRegion>(
		ATerritoryRegion::StaticClass(),
		Location,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewTerritory)
	{
		// Can't set directly as UPROPERTY is EditAnywhere, not code-settable
		// In a real implementation, add setter functions
		NewTerritory->SetOwnerFaction(StartingOwner);
		Territories.Add(NewTerritory);
	}
}

ATerritoryRegion* AWorldMapManager::GetTerritoryByID(FName TerritoryID) const
{
	for (ATerritoryRegion* Territory : Territories)
	{
		if (Territory && Territory->GetTerritoryID() == TerritoryID)
		{
			return Territory;
		}
	}
	return nullptr;
}

ATerritoryRegion* AWorldMapManager::GetTerritoryAtLocation(const FVector& Location) const
{
	for (ATerritoryRegion* Territory : Territories)
	{
		if (!Territory)
		{
			continue;
		}

		// Check if location is within territory bounds
		FVector TerritoryLocation = Territory->GetActorLocation();
		float HalfSize = TerritorySize * 0.5f;
		
		if (FMath::Abs(Location.X - TerritoryLocation.X) <= HalfSize &&
			FMath::Abs(Location.Y - TerritoryLocation.Y) <= HalfSize)
		{
			return Territory;
		}
	}
	return nullptr;
}

TArray<ATerritoryRegion*> AWorldMapManager::GetAdjacentTerritories(ATerritoryRegion* Territory) const
{
	TArray<ATerritoryRegion*> Adjacent;
	
	if (!Territory)
	{
		return Adjacent;
	}

	FVector Location = Territory->GetActorLocation();
	
	for (ATerritoryRegion* Other : Territories)
	{
		if (Other && Other != Territory && AreTerritoriesAdjacent(Territory, Other))
		{
			Adjacent.Add(Other);
		}
	}
	
	return Adjacent;
}

bool AWorldMapManager::AreTerritoriesAdjacent(ATerritoryRegion* Territory1, ATerritoryRegion* Territory2) const
{
	if (!Territory1 || !Territory2)
	{
		return false;
	}

	float Distance = FVector::Distance(Territory1->GetActorLocation(), Territory2->GetActorLocation());
	
	// Adjacent if within 1.5 territory sizes (allows for diagonal)
	return Distance <= TerritorySize * 1.5f;
}
