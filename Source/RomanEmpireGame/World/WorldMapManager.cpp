// Copyright Roman Empire Game. All Rights Reserved.

#include "WorldMapManager.h"
#include "RomanEmpireGame.h"
#include "World/TerritoryRegion.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	FTerritoryDefinition MakePrototypeTerritoryDefinition(
		const TCHAR* ID,
		const TCHAR* DisplayName,
		EFactionID StartingOwner)
	{
		FTerritoryDefinition Definition;
		Definition.TerritoryID = FName(ID);
		Definition.DisplayName = FText::FromString(DisplayName);
		Definition.Description = FText::FromString(
			FString::Printf(TEXT("Prototype campaign territory: %s"), DisplayName));
		Definition.StartingOwner = StartingOwner;
		Definition.TerrainType = ETerrainType::Plains;
		Definition.BonusResource = EResourceType::Gold;
		Definition.ResourceProduction.Gold = 50;
		Definition.ResourceProduction.Food = 30;
		return Definition;
	}
}

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
	CampaignTerritoryState.Reset();
	Territories.Reset();
	TerritoryActorsByID.Reset();
	
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
				RegisterExistingTerritory(Territory);
			}
		}
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("World Map Manager initialized with %d territories"), Territories.Num());
}

void AWorldMapManager::GenerateDefaultMap()
{
	if (Territories.Num() > 0 || CampaignTerritoryState.GetTerritoryCount() > 0)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Default map generation skipped because territories already exist"));
		return;
	}

	// Generate a small Mediterranean-style map
	// Layout:
	//   [Gaul]  [Alps]  [Greece]
	//   [Hispania] [Rome] [Macedonia]  
	//   [Carthage] [Sicily] [Egypt]

	// Row 1 - Northern territories
	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Gaul"), TEXT("Gallia"), EFactionID::Gaul),
		FVector(-TerritorySize, TerritorySize, 0));

	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Alps"), TEXT("Alpine Passes"), EFactionID::None),
		FVector(0, TerritorySize, 0));

	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Greece"), TEXT("Graecia"), EFactionID::None),
		FVector(TerritorySize, TerritorySize, 0));

	// Row 2 - Central territories
	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Hispania"), TEXT("Hispania"), EFactionID::None),
		FVector(-TerritorySize, 0, 0));

	// Rome - center of the map
	ATerritoryRegion* RomeTerritory = CreateTerritory(
		MakePrototypeTerritoryDefinition(TEXT("Rome"), TEXT("Italia"), EFactionID::Rome),
		FVector(0, 0, 0));

	if (RomeTerritory)
	{
		RomeTerritory->FoundSettlement(FText::FromString(TEXT("Roma")));
	}

	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Macedonia"), TEXT("Macedonia"), EFactionID::None),
		FVector(TerritorySize, 0, 0));

	// Row 3 - Southern territories
	ATerritoryRegion* CarthageTerritory = CreateTerritory(
		MakePrototypeTerritoryDefinition(TEXT("Africa"), TEXT("Africa"), EFactionID::Carthage),
		FVector(-TerritorySize, -TerritorySize, 0));

	if (CarthageTerritory)
	{
		CarthageTerritory->FoundSettlement(FText::FromString(TEXT("Carthago")));
	}

	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Sicily"), TEXT("Sicilia"), EFactionID::None),
		FVector(0, -TerritorySize, 0));

	CreateTerritory(MakePrototypeTerritoryDefinition(TEXT("Egypt"), TEXT("Aegyptus"), EFactionID::None),
		FVector(TerritorySize, -TerritorySize, 0));

	UE_LOG(LogRomanEmpire, Log, TEXT("Generated default Mediterranean map with %d territories"), Territories.Num());
}

ATerritoryRegion* AWorldMapManager::CreateTerritory(
	const FTerritoryDefinition& Definition,
	const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	ATerritoryRegion* NewTerritory = World->SpawnActorDeferred<ATerritoryRegion>(
		ATerritoryRegion::StaticClass(),
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewTerritory)
	{
		return nullptr;
	}

	if (!NewTerritory->InitializeTerritory(Definition, this))
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Territory %s rejected during actor initialization"),
			*Definition.TerritoryID.ToString());
		NewTerritory->Destroy();
		return nullptr;
	}

	const FTerritoryRegistrationResult Registration =
		CampaignTerritoryState.RegisterTerritory(Definition);
	if (!Registration.WasSuccessful())
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Territory %s rejected by campaign state (%s)"),
			*Definition.TerritoryID.ToString(),
			RomanEmpireTerritories::GetRegistrationFailureName(Registration.Failure));
		NewTerritory->Destroy();
		return nullptr;
	}

	UGameplayStatics::FinishSpawningActor(NewTerritory, SpawnTransform);
	Territories.Add(NewTerritory);
	TerritoryActorsByID.Add(Definition.TerritoryID, NewTerritory);
	return NewTerritory;
}

ATerritoryRegion* AWorldMapManager::GetTerritoryByID(FName TerritoryID) const
{
	if (const TObjectPtr<ATerritoryRegion>* Territory = TerritoryActorsByID.Find(TerritoryID))
	{
		return Territory->Get();
	}
	return nullptr;
}

FTerritoryDefinition AWorldMapManager::GetTerritoryDefinition(FName TerritoryID) const
{
	FTerritoryDefinition Definition;
	CampaignTerritoryState.TryGetDefinition(TerritoryID, Definition);
	return Definition;
}

EFactionID AWorldMapManager::GetTerritoryOwner(FName TerritoryID) const
{
	EFactionID TerritoryOwner = EFactionID::None;
	CampaignTerritoryState.TryGetOwner(TerritoryID, TerritoryOwner);
	return TerritoryOwner;
}

FTerritoryOwnershipChangeResult AWorldMapManager::ChangeTerritoryOwner(
	FName TerritoryID,
	EFactionID NewOwner)
{
	FTerritoryOwnershipChangeResult Result =
		CampaignTerritoryState.ChangeOwner(TerritoryID, NewOwner);
	if (Result.WasSuccessful() && Result.bOwnershipChanged)
	{
		if (ATerritoryRegion* Territory = GetTerritoryByID(TerritoryID))
		{
			Territory->RefreshOwnershipPresentation(Result.PreviousOwner, Result.NewOwner);
		}
		OnTerritoryOwnerChanged.Broadcast(
			TerritoryID, Result.PreviousOwner, Result.NewOwner);
		UE_LOG(LogRomanEmpire, Log, TEXT("Territory %s changed owner from %d to %d"),
			*TerritoryID.ToString(),
			static_cast<int32>(Result.PreviousOwner),
			static_cast<int32>(Result.NewOwner));
	}
	return Result;
}

int32 AWorldMapManager::GetFactionTerritoryCount(EFactionID FactionID) const
{
	return CampaignTerritoryState.GetFactionTerritoryCount(FactionID);
}

bool AWorldMapManager::RegisterExistingTerritory(ATerritoryRegion* Territory)
{
	if (!Territory)
	{
		return false;
	}

	const FTerritoryDefinition Definition = Territory->GetDefinition();
	const FTerritoryRegistrationResult Registration =
		CampaignTerritoryState.RegisterTerritory(Definition);
	if (!Registration.WasSuccessful() || !Territory->InitializeTerritory(Definition, this))
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Existing territory %s rejected (%s)"),
			*Definition.TerritoryID.ToString(),
			RomanEmpireTerritories::GetRegistrationFailureName(Registration.Failure));
		return false;
	}

	Territories.Add(Territory);
	TerritoryActorsByID.Add(Definition.TerritoryID, Territory);
	return true;
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

