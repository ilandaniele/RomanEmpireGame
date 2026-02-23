// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireGameMode.h"
#include "../RomanEmpireGame.h"
#include "../Faction/FactionManager.h"
#include "../World/WorldMapManager.h"
#include "../World/CampaignManager.h"
#include "../Camera/SeamlessZoomCamera.h"
#include "RomanEmpirePlayerController.h"
#include "RomanEmpireHUD.h"
#include "../Units/Legionary.h"
#include "Kismet/GameplayStatics.h"

ARomanEmpireGameMode::ARomanEmpireGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CurrentPhase = EGamePhase::Strategic;
	CurrentTurn = 1;
	
	FactionManager = nullptr;
	WorldMapManager = nullptr;
	CampaignManager = nullptr;

	// Set custom gameplay classes
	DefaultPawnClass = ASeamlessZoomCamera::StaticClass();
	PlayerControllerClass = ARomanEmpirePlayerController::StaticClass();
	HUDClass = ARomanEmpireHUD::StaticClass();
}

void ARomanEmpireGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Roman Empire Game Mode started - Turn %d"), CurrentTurn);
	
	InitializeManagers();
	SpawnInitialUnits();
	UpdateHUDResources();
}

void ARomanEmpireGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Periodically update HUD resources
	static float ResourceUpdateTimer = 0.0f;
	ResourceUpdateTimer += DeltaSeconds;
	if (ResourceUpdateTimer >= 0.5f)
	{
		ResourceUpdateTimer = 0.0f;
		UpdateHUDResources();
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
	
	if (CampaignManager)
	{
		CampaignManager->ProcessTurn();
	}
	
	UpdateHUDResources();
}

void ARomanEmpireGameMode::InitializeManagers()
{
	UWorld* World = GetWorld();
	if (!World) return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	FactionManager = World->SpawnActor<AFactionManager>(AFactionManager::StaticClass(), SpawnParams);
	if (FactionManager)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("Faction Manager initialized"));
	}
	
	WorldMapManager = World->SpawnActor<AWorldMapManager>(AWorldMapManager::StaticClass(), SpawnParams);
	if (WorldMapManager)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("World Map Manager initialized"));
	}
	
	CampaignManager = World->SpawnActor<ACampaignManager>(ACampaignManager::StaticClass(), SpawnParams);
	if (CampaignManager)
	{
		UE_LOG(LogRomanEmpire, Log, TEXT("Campaign Manager initialized"));
	}
}

void ARomanEmpireGameMode::SpawnInitialUnits()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector BaseLocation = FVector(0.0f, 0.0f, 200.0f);
	
	for (int32 i = 0; i < 5; i++)
	{
		FVector SpawnOffset = FVector(
			FMath::RandRange(-300.0f, 300.0f),
			FMath::RandRange(-300.0f, 300.0f),
			0.0f
		);
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		ALegionary* Unit = World->SpawnActor<ALegionary>(ALegionary::StaticClass(), BaseLocation + SpawnOffset, FRotator::ZeroRotator, SpawnParams);
		if (Unit)
		{
			Unit->SetOwnerFaction(EFactionID::Rome);
			UE_LOG(LogRomanEmpire, Log, TEXT("Spawned Legionary #%d"), i + 1);
		}
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Initial Roman units spawned"));
}

void ARomanEmpireGameMode::UpdateHUDResources()
{
	if (!FactionManager) return;
	
	EFactionID PlayerFaction = FactionManager->GetPlayerFaction();
	FFactionResources Resources = FactionManager->GetFactionResources(PlayerFaction);
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			ARomanEmpireHUD* HUD = Cast<ARomanEmpireHUD>(PC->GetHUD());
			if (HUD)
			{
				HUD->UpdateResources(Resources.Gold, Resources.Food, Resources.Iron, Resources.Wood, Resources.Stone, Resources.Population);
			}
		}
	}
}
