// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireGameMode.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/Faction/FactionManager.h"
#include "RomanEmpireGame/World/WorldMapManager.h"
#include "RomanEmpireGame/World/CampaignManager.h"
#include "Kismet/GameplayStatics.h"

ARomanEmpireGameMode::ARomanEmpireGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CurrentPhase = EGamePhase::Strategic;
	CurrentTurn = 1;
	
	FactionManager = nullptr;
	WorldMapManager = nullptr;
	CampaignManager = nullptr;
}

void ARomanEmpireGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Roman Empire Game Mode started - Turn %d"), CurrentTurn);
	
	InitializeManagers();
}

void ARomanEmpireGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// Game logic updates based on current phase
	switch (CurrentPhase)
	{
		case EGamePhase::Strategic:
			// Strategic/campaign updates
			break;
		case EGamePhase::Tactical:
			// RTS mode updates
			break;
		case EGamePhase::Combat:
			// Battle updates
			break;
		case EGamePhase::FirstPerson:
			// FPS mode updates
			break;
	}
}

void ARomanEmpireGameMode::SetGamePhase(EGamePhase NewPhase)
{
	if (CurrentPhase != NewPhase)
	{
		EGamePhase OldPhase = CurrentPhase;
		CurrentPhase = NewPhase;
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Game phase changed from %d to %d"), 
			static_cast<int32>(OldPhase), static_cast<int32>(NewPhase));
		
		OnGamePhaseChanged.Broadcast(OldPhase, NewPhase);
	}
}

void ARomanEmpireGameMode::OnZoomLevelChanged(float ZoomLevel)
{
	EGamePhase NewPhase = DeterminePhaseFromZoom(ZoomLevel);
	SetGamePhase(NewPhase);
}

EGamePhase ARomanEmpireGameMode::DeterminePhaseFromZoom(float ZoomLevel) const
{
	using namespace RomanEmpireConstants;
	
	if (ZoomLevel <= ZOOM_TERRITORY_MAX)
	{
		return EGamePhase::Strategic;
	}
	else if (ZoomLevel <= ZOOM_CITY_MAX)
	{
		return EGamePhase::Tactical;
	}
	else if (ZoomLevel <= ZOOM_GROUND_MAX)
	{
		return EGamePhase::Combat;
	}
	else
	{
		return EGamePhase::FirstPerson;
	}
}

void ARomanEmpireGameMode::EndTurn()
{
	if (CurrentPhase != EGamePhase::Strategic)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot end turn outside of Strategic phase"));
		return;
	}
	
	CurrentTurn++;
	UE_LOG(LogRomanEmpire, Log, TEXT("Turn %d started"), CurrentTurn);
	
	// Process AI faction turns
	if (CampaignManager)
	{
		CampaignManager->ProcessTurn();
	}
}

void ARomanEmpireGameMode::InitializeManagers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	// Spawn Faction Manager
	FactionManager = World->SpawnActor<AFactionManager>(AFactionManager::StaticClass(), SpawnParams);
	if (FactionManager)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("Faction Manager initialized"));
	}
	
	// Spawn World Map Manager
	WorldMapManager = World->SpawnActor<AWorldMapManager>(AWorldMapManager::StaticClass(), SpawnParams);
	if (WorldMapManager)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("World Map Manager initialized"));
	}
	
	// Spawn Campaign Manager
	CampaignManager = World->SpawnActor<ACampaignManager>(ACampaignManager::StaticClass(), SpawnParams);
	if (CampaignManager)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("Campaign Manager initialized"));
	}
}
