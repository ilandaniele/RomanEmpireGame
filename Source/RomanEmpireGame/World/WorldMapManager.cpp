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
	
	if (!CampaignTerritoryState.HasValidAdjacency())
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Campaign adjacency references must be known and reciprocal"));
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
		return IsValid(Territory->Get()) ? Territory->Get() : nullptr;
	}
	return nullptr;
}

FTerritoryDefinition AWorldMapManager::GetTerritoryDefinition(FName TerritoryID) const
{
	FTerritoryDefinition Definition;
	CampaignTerritoryState.TryGetDefinition(TerritoryID, Definition);
	return Definition;
}

TArray<FName> AWorldMapManager::GetTerritoryIDs() const
{
	return CampaignTerritoryState.GetTerritoryIDs();
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
	if (bCampaignTransactionActive)
	{
		FTerritoryOwnershipChangeResult Rejected;
		Rejected.TerritoryID = TerritoryID;
		Rejected.Failure = ETerritoryOwnershipFailure::TransactionInProgress;
		return Rejected;
	}
	FTerritoryOwnershipChangeResult Result =
		CampaignTerritoryState.ChangeOwner(TerritoryID, NewOwner);
	PublishOwnershipChange(Result);
	return Result;
}

void AWorldMapManager::PublishOwnershipChange(const FTerritoryOwnershipChangeResult& Result)
{
	if (Result.WasSuccessful() && Result.bOwnershipChanged)
	{
		if (ATerritoryRegion* Territory = GetTerritoryByID(Result.TerritoryID))
		{
			Territory->RefreshOwnershipPresentation(Result.PreviousOwner, Result.NewOwner);
		}
		OnTerritoryOwnerChanged.Broadcast(
			Result.TerritoryID, Result.PreviousOwner, Result.NewOwner);
		UE_LOG(LogRomanEmpire, Log, TEXT("Territory %s changed owner from %d to %d"),
			*Result.TerritoryID.ToString(),
			static_cast<int32>(Result.PreviousOwner),
			static_cast<int32>(Result.NewOwner));
	}
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
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanTerritoryDefinition* Asset = Registry && Registry->IsReady()
		? Registry->FindTerritory(Definition.TerritoryID) : nullptr;
	if (Asset && !Asset->StartingSettlementName.IsEmpty())
	{
		FoundTerritorySettlement(Definition.TerritoryID, Asset->StartingSettlementName);
	}
	return true;
}

FSettlementFoundingResult AWorldMapManager::FoundTerritorySettlement(FName TerritoryID, const FText& Name)
{
	if (bCampaignTransactionActive)
	{
		FSettlementFoundingResult Rejected;
		Rejected.TerritoryID = TerritoryID;
		Rejected.Failure = ESettlementFoundingFailure::TransactionInProgress;
		return Rejected;
	}
	const FSettlementFoundingResult Result = CampaignTerritoryState.FoundSettlement(TerritoryID, Name);
	if (Result.WasSuccessful())
	{
		// State is committed before any observer, including legacy actor observers.
		if (ATerritoryRegion* Territory = GetTerritoryByID(TerritoryID); IsValid(Territory))
		{
			Territory->RefreshSettlementPresentation();
		}
		OnTerritorySettlementChanged.Broadcast(TerritoryID);
	}
	return Result;
}

bool AWorldMapManager::TryGetTerritorySettlement(FName TerritoryID, FTerritorySettlementState& OutSettlement) const
{
	return CampaignTerritoryState.TryGetSettlement(TerritoryID, OutSettlement);
}

bool AWorldMapManager::TryGetTerritoryProduction(FName TerritoryID, FFactionResourceDelta& OutProduction) const
{
	return CampaignTerritoryState.TryGetResourceProduction(TerritoryID, OutProduction);
}

ATerritoryRegion* AWorldMapManager::GetTerritoryAtLocation(const FVector& Location) const
{
	for (ATerritoryRegion* Territory : Territories)
	{
		if (!IsValid(Territory))
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

TArray<FName> AWorldMapManager::GetAdjacentTerritoryIDs(FName TerritoryID) const
{
	return CampaignTerritoryState.GetAdjacentIDs(TerritoryID);
}

bool AWorldMapManager::AreTerritoryIDsAdjacent(FName FirstID, FName SecondID) const
{
	return CampaignTerritoryState.AreAdjacent(FirstID, SecondID);
}

TArray<ATerritoryRegion*> AWorldMapManager::GetAdjacentTerritories(ATerritoryRegion* Territory) const
{
	TArray<ATerritoryRegion*> Adjacent;
	if (!IsValid(Territory) || GetTerritoryByID(Territory->GetTerritoryID()) != Territory)
	{
		return Adjacent;
	}

	for (FName NeighborID : GetAdjacentTerritoryIDs(Territory->GetTerritoryID()))
	{
		if (ATerritoryRegion* Other = GetTerritoryByID(NeighborID); IsValid(Other))
		{
			Adjacent.Add(Other);
		}
	}
	
	return Adjacent;
}

bool AWorldMapManager::AreTerritoriesAdjacent(ATerritoryRegion* Territory1, ATerritoryRegion* Territory2) const
{
	if (!IsValid(Territory1) || !IsValid(Territory2) ||
		GetTerritoryByID(Territory1->GetTerritoryID()) != Territory1 ||
		GetTerritoryByID(Territory2->GetTerritoryID()) != Territory2)
	{
		return false;
	}

	return AreTerritoryIDsAdjacent(Territory1->GetTerritoryID(), Territory2->GetTerritoryID());
}

