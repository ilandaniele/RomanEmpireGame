// Copyright Roman Empire Game. All Rights Reserved.

#include "WorldMapManager.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "World/TerritoryRegion.h"
#include "Engine/World.h"
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

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!Registry || !Registry->IsReady())
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Default map generation requires a valid content registry"));
		return;
	}

	for (const URomanTerritoryDefinition* Asset : Registry->GetAllTerritories())
	{
		if (!Asset)
		{
			continue;
		}

		ATerritoryRegion* Territory = CreateTerritory(Asset->Definition, Asset->MapLocation);
		if (Territory && !Asset->StartingSettlementName.IsEmpty())
		{
			Territory->FoundSettlement(Asset->StartingSettlementName);
		}
	}

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

