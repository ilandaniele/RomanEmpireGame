// Copyright Roman Empire Game. All Rights Reserved.

#include "CampaignManager.h"
#include "RomanEmpireGame.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "World/WorldMapManager.h"
#include "World/TerritoryRegion.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACampaignManager::ACampaignManager()
{
	PrimaryActorTick.bCanEverTick = false;

	CurrentTurn = 1;
	bCampaignActive = false;
	MaxTurns = 100;
	ConquestVictoryTerritories = 7; // Control 7 of 9 territories
	EconomicVictoryGold = 10000;
	LastAutosaveRequestedTurn = 0;

	FactionManager = nullptr;
	WorldMapManager = nullptr;
	TurnRandomStream.Initialize(PrototypeTurnRandomSeed);
}

void ACampaignManager::BeginPlay()
{
	Super::BeginPlay();

	// Find managers
	TArray<AActor*> FoundActors;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFactionManager::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		FactionManager = Cast<AFactionManager>(FoundActors[0]);
	}
	
	FoundActors.Empty();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldMapManager::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		WorldMapManager = Cast<AWorldMapManager>(FoundActors[0]);
	}

	// Start campaign automatically for prototype
	if (FactionManager)
	{
		StartNewCampaign(FactionManager->GetPlayerFaction());
	}
}

void ACampaignManager::StartNewCampaign(EFactionID PlayerFaction)
{
	CurrentTurn = 1;
	bCampaignActive = true;
	LastAutosaveRequestedTurn = 0;
	TurnRandomStream.Initialize(PrototypeTurnRandomSeed);

	if (FactionManager)
	{
		FactionManager->SetPlayerFaction(PlayerFaction);
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("New campaign started with faction %d"), 
		static_cast<int32>(PlayerFaction));
}

FCampaignTurnResult ACampaignManager::ProcessTurn()
{
	FCampaignTurnResult Result;
	Result.StartingTurn = CurrentTurn;
	Result.CompletedTurn = CurrentTurn;

	for (const ECampaignTurnStep Step : RomanEmpireCampaign::GetCanonicalTurnPipeline())
	{
		Result.FailedStep = Step;
		OnTurnStepStarted.Broadcast(CurrentTurn, Step);
		UE_LOG(LogRomanEmpire, Verbose, TEXT("Turn %d step: %s"),
			CurrentTurn, RomanEmpireCampaign::GetTurnStepName(Step));

		const ECampaignTurnFailure Failure = ExecuteTurnStep(Step);
		if (Failure != ECampaignTurnFailure::None)
		{
			Result.Failure = Failure;
			Result.CompletedTurn = CurrentTurn;
			UE_LOG(LogRomanEmpire, Warning, TEXT("Turn %d stopped at %s (%s)"),
				Result.StartingTurn,
				RomanEmpireCampaign::GetTurnStepName(Step),
				RomanEmpireCampaign::GetTurnFailureName(Failure));
			return Result;
		}

		Result.CompletedSteps.Add(Step);
		Result.CompletedTurn = CurrentTurn;
	}

	Result.Failure = ECampaignTurnFailure::None;
	UE_LOG(LogRomanEmpire, Log, TEXT("Turn %d completed; turn %d is now active"),
		Result.StartingTurn, Result.CompletedTurn);
	return Result;
}

ECampaignTurnFailure ACampaignManager::ExecuteTurnStep(ECampaignTurnStep Step)
{
	switch (Step)
	{
	case ECampaignTurnStep::ValidateRequest:
		return ValidateTurnRequest();
	case ECampaignTurnStep::Production:
		return ProcessResourceProduction();
	case ECampaignTurnStep::Upkeep:
		return ProcessUpkeep();
	case ECampaignTurnStep::AIActions:
		return ProcessAIFactions();
	case ECampaignTurnStep::ConflictResolution:
		return ResolveTurnConflicts();
	case ECampaignTurnStep::VictoryEvaluation:
		CheckAllVictoryConditions();
		return ECampaignTurnFailure::None;
	case ECampaignTurnStep::IncrementTurn:
		CurrentTurn++;
		if (CurrentTurn > MaxTurns)
		{
			UE_LOG(LogRomanEmpire, Log, TEXT("Max turns reached, campaign ending"));
			bCampaignActive = false;
		}
		return ECampaignTurnFailure::None;
	case ECampaignTurnStep::BroadcastTurnCompleted:
		OnTurnProcessed.Broadcast(CurrentTurn);
		return ECampaignTurnFailure::None;
	case ECampaignTurnStep::Autosave:
		return RequestAutosave();
	default:
		return ECampaignTurnFailure::CampaignInactive;
	}
}

ECampaignTurnFailure ACampaignManager::ValidateTurnRequest() const
{
	if (!bCampaignActive) return ECampaignTurnFailure::CampaignInactive;
	if (!FactionManager) return ECampaignTurnFailure::MissingFactionManager;
	if (!WorldMapManager) return ECampaignTurnFailure::MissingWorldMapManager;
	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ProcessResourceProduction()
{
	if (!FactionManager) return ECampaignTurnFailure::MissingFactionManager;
	if (!WorldMapManager) return ECampaignTurnFailure::MissingWorldMapManager;

	// For each territory, add resources to owning faction
	for (ATerritoryRegion* Territory : WorldMapManager->GetAllTerritories())
	{
		if (!Territory)
		{
			continue;
		}

		EFactionID TerritoryOwner = Territory->GetOwnerFaction();
		if (TerritoryOwner == EFactionID::None)
		{
			continue;
		}

		const FFactionResourceDelta Production = Territory->GetResourceProduction();
		const FFactionResourceTransactionResult Result =
			FactionManager->ApplyFactionResourceDelta(TerritoryOwner, Production);
		if (!Result.WasSuccessful())
		{
			UE_LOG(LogRomanEmpire, Error, TEXT("Production failed for faction %d (%s)"),
				static_cast<int32>(TerritoryOwner),
				RomanEmpireEconomy::GetTransactionFailureName(Result.Failure));
			return ECampaignTurnFailure::ResourceTransactionFailed;
		}
	}

	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ProcessUpkeep()
{
	// Upkeep rules are introduced with production gameplay; the ordered stage is explicit now.
	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ProcessAIFactions()
{
	if (!FactionManager)
	{
		return ECampaignTurnFailure::MissingFactionManager;
	}

	FactionManager->ProcessAITurns();
	TurnRandomStream.Initialize(PrototypeTurnRandomSeed + CurrentTurn);

	// Prototype tactical AI runs once within the campaign AI stage.
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllUnits);
	for (AActor* Actor : AllUnits)
	{
		AUnitBase* Unit = Cast<AUnitBase>(Actor);
		if (!Unit || !Unit->IsAlive() || Unit->GetOwnerFaction() == EFactionID::Rome ||
			Unit->GetOwnerFaction() == EFactionID::None)
		{
			continue;
		}

		const FVector CurrentPosition = Unit->GetActorLocation();
		FVector DirectionToRome = -CurrentPosition;
		DirectionToRome.Z = 0.0f;
		if (DirectionToRome.SizeSquared() > 10000.0f)
		{
			DirectionToRome.Normalize();
			const float AdvanceDistance = TurnRandomStream.FRandRange(500.0f, 2000.0f);
			FVector Destination = CurrentPosition + DirectionToRome * AdvanceDistance;
			Destination.Z = CurrentPosition.Z;
			Unit->CommandMoveTo(Destination);
		}
	}

	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::ResolveTurnConflicts()
{
	// Strategic conflict resolution rules are introduced with canonical territory state in M2.3.
	return ECampaignTurnFailure::None;
}

ECampaignTurnFailure ACampaignManager::RequestAutosave()
{
	LastAutosaveRequestedTurn = CurrentTurn;
	OnCampaignAutosaveRequested.Broadcast(CurrentTurn);
	SaveCampaign(FString::Printf(TEXT("Autosave_Turn_%d"), CurrentTurn));
	return ECampaignTurnFailure::None;
}

void ACampaignManager::CheckAllVictoryConditions()
{
	if (!FactionManager)
	{
		return;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();

	// Check conquest victory
	if (CheckVictoryCondition(PlayerFaction, EVictoryCondition::Conquest))
	{
		OnVictory.Broadcast(PlayerFaction, EVictoryCondition::Conquest);
		bCampaignActive = false;
		UE_LOG(LogRomanEmpire, Log, TEXT("Player achieved conquest victory!"));
		return;
	}

	// Check economic victory
	if (CheckVictoryCondition(PlayerFaction, EVictoryCondition::Economic))
	{
		OnVictory.Broadcast(PlayerFaction, EVictoryCondition::Economic);
		bCampaignActive = false;
		UE_LOG(LogRomanEmpire, Log, TEXT("Player achieved economic victory!"));
		return;
	}

	// Check if player lost all territories
	if (HasPlayerLost())
	{
		OnDefeat.Broadcast(PlayerFaction);
		bCampaignActive = false;
		UE_LOG(LogRomanEmpire, Log, TEXT("Player has been defeated!"));
	}
}

bool ACampaignManager::CheckVictoryCondition(EFactionID FactionID, EVictoryCondition Condition) const
{
	if (!FactionManager || !WorldMapManager)
	{
		return false;
	}

	switch (Condition)
	{
		case EVictoryCondition::Conquest:
		{
			int32 TerritoryCount = FactionManager->GetFactionTerritoryCount(FactionID);
			return TerritoryCount >= ConquestVictoryTerritories;
		}

		case EVictoryCondition::Economic:
		{
			const FFactionResourceAmounts Resources = FactionManager->GetFactionResources(FactionID);
			return Resources.Gold >= EconomicVictoryGold;
		}

		case EVictoryCondition::Military:
		{
			// TODO: Check if faction destroyed all enemy armies
			return false;
		}

		case EVictoryCondition::Diplomatic:
		{
			// TODO: Check allied victory
			return false;
		}

		default:
			return false;
	}
}

bool ACampaignManager::HasPlayerWon() const
{
	if (!FactionManager)
	{
		return false;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	
	return CheckVictoryCondition(PlayerFaction, EVictoryCondition::Conquest) ||
		   CheckVictoryCondition(PlayerFaction, EVictoryCondition::Economic);
}

bool ACampaignManager::HasPlayerLost() const
{
	if (!FactionManager)
	{
		return false;
	}

	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	int32 TerritoryCount = FactionManager->GetFactionTerritoryCount(PlayerFaction);
	
	// Lost if no territories
	return TerritoryCount == 0;
}

void ACampaignManager::SaveCampaign(const FString& SaveName)
{
	// TODO: Implement save system using USaveGame
	UE_LOG(LogRomanEmpire, Log, TEXT("Saving campaign: %s"), *SaveName);
}

void ACampaignManager::LoadCampaign(const FString& SaveName)
{
	// TODO: Implement load system
	UE_LOG(LogRomanEmpire, Log, TEXT("Loading campaign: %s"), *SaveName);
}

