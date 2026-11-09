// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireGameMode.h"
#include "RomanEmpireGame.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Building/Barracks.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Content/RomanContentRegistry.h"
#include "Faction/FactionManager.h"
#include "World/WorldMapManager.h"
#include "World/CampaignManager.h"
#include "Battle/TacticalObjective.h"
#include "Camera/SeamlessZoomCamera.h"
#include "RomanEmpirePlayerController.h"
#include "RomanEmpireHUD.h"
#include "Units/UnitBase.h"
#include "Units/RomanUnitRegistry.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RomanEmpireGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/Box2D.h"

ARomanEmpireGameMode::ARomanEmpireGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CurrentGameplayMode = EGameplayMode::Tactical;
	
	FactionManager = nullptr;
	WorldMapManager = nullptr;
	CampaignManager = nullptr;
	PrototypeRandomStream.Initialize(PrototypeRandomSeed);

	DefaultPawnClass = ASeamlessZoomCamera::StaticClass();
	PlayerControllerClass = ARomanEmpirePlayerController::StaticClass();
	GameStateClass = ARomanEmpireGameState::StaticClass();
	HUDClass = ARomanEmpireHUD::StaticClass();
}

void ARomanEmpireGameMode::BeginPlay()
{
	Super::BeginPlay();
	PrototypeRandomStream.Initialize(PrototypeRandomSeed);
	UCampaignSessionSubsystem* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	const bool bIncomingSession = Session && Session->ExpectsIncomingWorld(GetWorld());
	if (bIncomingSession) bStartWithPrototypeEncounter = false;
	
	InitializeManagers();
	UE_LOG(LogRomanEmpire, Log, TEXT("Roman Empire Game Mode started - Turn %d"), GetCurrentTurn());
	SpawnWorldTerrain();
	if (bStartWithPrototypeEncounter) SpawnInitialUnits();
	SpawnInitialBuildings();
	if (bIncomingSession && !Session->RestoreIncomingWorld(GetWorld(), OptionsString)) return;
	const bool bTacticalSession = bIncomingSession && Session->GetPhase() == ECampaignTransferPhase::TacticalDeployment;
	
	// Set camera to city/tactical zoom so units are visible
	ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (CameraPawn)
	{
		const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
		const float InitialZoom = (bStartWithPrototypeEncounter || bTacticalSession) ? Zoom.DefaultTactical : Zoom.StrategicMaximum;
		CameraPawn->SetZoomLevel(InitialZoom);
		CameraPawn->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		if (bTacticalSession && CampaignManager->GetCampaignEncounterPlan().PlayerFaction == CampaignManager->GetCampaignEncounterPlan().Defender)
			if (const auto* Objective = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>()->GetObjectiveActor()) CameraPawn->SetActorLocation(Objective->GetActorLocation());
		SetExplorationModeForZoomLevel(CameraPawn->GetZoomLevelEnum());
		UE_LOG(LogRomanEmpire, Log, TEXT("Camera set to %s zoom %.2f"), bStartWithPrototypeEncounter ? TEXT("tactical") : TEXT("strategic"), InitialZoom);
	}
	if (!bStartWithPrototypeEncounter) SetGameplayMode(bTacticalSession ? EGameplayMode::Tactical : EGameplayMode::Strategic);
	bGameplayInitialized = FactionManager && WorldMapManager && CampaignManager;
	if (bGameplayInitialized)
	{
		OnGameplayInitialized.Broadcast();
		if (bIncomingSession) Session->OnSessionChanged.Broadcast();
	}
}

bool ARomanEmpireGameMode::CanRestartEncounter() const
{
	const UWorld* World = GetWorld();
	const UCampaignSessionSubsystem* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (Session && (Session->HasSession() || Session->HasPendingTransfer())) return false;
	const UTacticalBattleSubsystem* Battle = World ? World->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	return bGameplayInitialized && !bEncounterTravelPending && !IsActorBeingDestroyed() &&
		CampaignManager && CampaignManager->IsCampaignActive() && Battle && Battle->IsResolved();
}

bool ARomanEmpireGameMode::RestartEncounter()
{
	if (!CanRestartEncounter()) return false;
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (LevelName.IsEmpty()) return false;
	// Reload the sandbox. Persistent campaign return is a separate contract.
	bEncounterTravelPending = true;
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
	return true;
}

void ARomanEmpireGameMode::SetGameplayMode(EGameplayMode NewMode)
{
	if (CurrentGameplayMode != NewMode)
	{
		const EGameplayMode OldMode = CurrentGameplayMode;
		CurrentGameplayMode = NewMode;
		UE_LOG(LogRomanEmpire, Log, TEXT("Gameplay mode changed from %s to %s"),
			RomanEmpireGameplay::GetGameplayModeName(OldMode),
			RomanEmpireGameplay::GetGameplayModeName(NewMode));
		OnGameplayModeChanged.Broadcast(OldMode, NewMode);
	}
}

void ARomanEmpireGameMode::SetExplorationModeForZoomLevel(EZoomLevel ZoomLevel)
{
	if (CurrentGameplayMode != EGameplayMode::Possessed)
	{
		SetGameplayMode(RomanEmpireGameplay::GetExplorationModeForZoomLevel(ZoomLevel));
	}
}

int32 ARomanEmpireGameMode::GetCurrentTurn() const
{
	return CampaignManager ? CampaignManager->GetCurrentTurn() : 0;
}

FCampaignTurnResult ARomanEmpireGameMode::EndTurn()
{
	if (CampaignManager)
	{
		return CampaignManager->ProcessTurn();
	}

	FCampaignTurnResult Result;
	Result.Failure = ECampaignTurnFailure::MissingCampaignManager;
	return Result;
}

void ARomanEmpireGameMode::InitializeManagers()
{
	UWorld* World = GetWorld();
	if (!World) return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	FactionManager = World->SpawnActor<AFactionManager>(AFactionManager::StaticClass(), SpawnParams);
	if (FactionManager) UE_LOG(LogRomanEmpire, Log, TEXT("Faction Manager initialized"));
	
	WorldMapManager = World->SpawnActor<AWorldMapManager>(AWorldMapManager::StaticClass(), SpawnParams);
	if (WorldMapManager) UE_LOG(LogRomanEmpire, Log, TEXT("World Map Manager initialized"));
	
	CampaignManager = World->SpawnActor<ACampaignManager>(ACampaignManager::StaticClass(), SpawnParams);
	if (CampaignManager) UE_LOG(LogRomanEmpire, Log, TEXT("Campaign Manager initialized"));
}

static UStaticMesh* GetEngineMesh(const TCHAR* Path)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	if (const auto* Found = Cache.Find(Path); Found && Found->IsValid()) return Found->Get();
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path);
	if (Mesh) Cache.Add(Path, Mesh);
	return Mesh;
}

static UMaterialInstanceDynamic* MakeColorMaterial(UObject* Outer, FLinearColor Color)
{
	UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (!BaseMat) return nullptr;
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Outer);
	if (MID) MID->SetVectorParameterValue(TEXT("Color"), Color);
	return MID;
}

void ARomanEmpireGameMode::SpawnWorldTerrain()
{
	UWorld* World = GetWorld();
	if (!World) return;
	// L_Testbed has an authored floor that predates the buildable-ground marker.
	// Future maps should tag intended building surfaces explicitly in the editor.
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* Floor = *It;
		const UStaticMeshComponent* Mesh = Floor->GetStaticMeshComponent();
		if (Floor->GetName().StartsWith(TEXT("Floor_")) && Mesh &&
			Mesh->Bounds.BoxExtent.X >= 3000.0 && Mesh->Bounds.BoxExtent.Y >= 3000.0)
		{
			Floor->Tags.AddUnique(TEXT("BuildableGround"));
		}
	}

	// Spawn a large green ground plane
	UStaticMesh* PlaneMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Plane"));
	if (!PlaneMesh) return;

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Main ground — 500m x 500m green terrain
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0, 0, -10), FRotator::ZeroRotator, SP);
	if (Ground)
	{
		Ground->Tags.Add(TEXT("BuildableGround"));
		Ground->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		Ground->SetActorScale3D(FVector(500.0f, 500.0f, 1.0f));  // 50,000 x 50,000 cm = 500m
		Ground->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Ground->GetStaticMeshComponent()->SetCollisionResponseToAllChannels(ECR_Block);
		
		UMaterialInstanceDynamic* GreenGrass = MakeColorMaterial(Ground, FLinearColor(0.15f, 0.35f, 0.1f));
		if (GreenGrass) Ground->GetStaticMeshComponent()->SetMaterial(0, GreenGrass);
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Ground terrain spawned"));
	}

	// Roman road (darker strip going through the center)
	AStaticMeshActor* Road = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0, 0, -5), FRotator::ZeroRotator, SP);
	if (Road)
	{
		Road->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		Road->SetActorScale3D(FVector(200.0f, 3.0f, 1.0f));
		Road->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		UMaterialInstanceDynamic* RoadMat = MakeColorMaterial(Road, FLinearColor(0.25f, 0.20f, 0.15f));
		if (RoadMat) Road->GetStaticMeshComponent()->SetMaterial(0, RoadMat);
	}

	// River (blue strip)
	AStaticMeshActor* River = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(3000, 0, -3), FRotator(0, 30, 0), SP);
	if (River)
	{
		River->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		River->SetActorScale3D(FVector(150.0f, 4.0f, 1.0f));
		River->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		UMaterialInstanceDynamic* WaterMat = MakeColorMaterial(River, FLinearColor(0.1f, 0.3f, 0.6f));
		if (WaterMat) River->GetStaticMeshComponent()->SetMaterial(0, WaterMat);
	}

	// Trees (green spheres scattered around)
	UStaticMesh* SphereMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Sphere"));
	UStaticMesh* CylinderMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	
	for (int32 i = 0; i < 30; i++)
	{
		float X = PrototypeRandomStream.FRandRange(-8000.0f, 8000.0f);
		float Y = PrototypeRandomStream.FRandRange(-8000.0f, 8000.0f);
		
		// Skip if too close to base
		if (FMath::Abs(X) < 1500.0f && FMath::Abs(Y) < 1500.0f) continue;
		
		// Tree trunk
		if (CylinderMesh)
		{
			AStaticMeshActor* Trunk = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(X, Y, 100), FRotator::ZeroRotator, SP);
			if (Trunk)
			{
				Trunk->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
				Trunk->SetActorScale3D(FVector(0.3f, 0.3f, 2.0f));
				Trunk->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UMaterialInstanceDynamic* BrownMat = MakeColorMaterial(Trunk, FLinearColor(0.35f, 0.2f, 0.08f));
				if (BrownMat) Trunk->GetStaticMeshComponent()->SetMaterial(0, BrownMat);
			}
		}
		
		// Tree canopy
		if (SphereMesh)
		{
			float CanopySize = PrototypeRandomStream.FRandRange(1.5f, 3.0f);
			AStaticMeshActor* Canopy = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(X, Y, 300), FRotator::ZeroRotator, SP);
			if (Canopy)
			{
				Canopy->GetStaticMeshComponent()->SetStaticMesh(SphereMesh);
				Canopy->SetActorScale3D(FVector(CanopySize, CanopySize, CanopySize * 0.7f));
				Canopy->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				float Green = PrototypeRandomStream.FRandRange(0.2f, 0.5f);
				UMaterialInstanceDynamic* LeafMat = MakeColorMaterial(Canopy, FLinearColor(0.05f, Green, 0.05f));
				if (LeafMat) Canopy->GetStaticMeshComponent()->SetMaterial(0, LeafMat);
			}
		}
	}

	// Hills (large flattened spheres)
	if (SphereMesh)
	{
		struct FHillData { FVector Pos; float Size; };
		TArray<FHillData> Hills = {
			{{ -5000, -4000, -200 }, 15.0f},
			{{ 6000, 3000, -200 }, 12.0f},
			{{ -3000, 5000, -200 }, 10.0f},
			{{ 7000, -5000, -200 }, 18.0f}
		};

		for (const auto& Hill : Hills)
		{
			AStaticMeshActor* HillActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Hill.Pos, FRotator::ZeroRotator, SP);
			if (HillActor)
			{
				HillActor->GetStaticMeshComponent()->SetStaticMesh(SphereMesh);
				HillActor->SetActorScale3D(FVector(Hill.Size, Hill.Size, Hill.Size * 0.3f));
				HillActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HillActor->GetStaticMeshComponent()->SetCollisionResponseToAllChannels(ECR_Block);
				UMaterialInstanceDynamic* HillMat = MakeColorMaterial(HillActor, FLinearColor(0.2f, 0.35f, 0.12f));
				if (HillMat) HillActor->GetStaticMeshComponent()->SetMaterial(0, HillMat);
			}
		}
	}

	// Gold mines (golden cubes with tall marker poles)
	UStaticMesh* CubeMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh && CylinderMesh)
	{
		struct FMineData { FVector Pos; };
		TArray<FMineData> Mines = {
			{{ 2500, -2000, 0 }},   // Between camps — contested
			{{ -4000, 3000, 0 }},   // Near river
			{{ 5000, -4500, 0 }}    // Near big hill
		};

		for (const auto& Mine : Mines)
		{
			// Gold cube (ore deposit)
			AStaticMeshActor* GoldCube = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Mine.Pos + FVector(0, 0, 50), FRotator::ZeroRotator, SP);
			if (GoldCube)
			{
				GoldCube->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
				GoldCube->SetActorScale3D(FVector(2.0f, 2.0f, 1.5f));
				GoldCube->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UMaterialInstanceDynamic* GoldMat = MakeColorMaterial(GoldCube, FLinearColor(0.85f, 0.65f, 0.05f)); // Gold
				if (GoldMat) GoldCube->GetStaticMeshComponent()->SetMaterial(0, GoldMat);
			}
			// Mine marker pole
			AStaticMeshActor* Pole = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Mine.Pos + FVector(0, 0, 200), FRotator::ZeroRotator, SP);
			if (Pole)
			{
				Pole->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
				Pole->SetActorScale3D(FVector(0.15f, 0.15f, 3.0f));
				Pole->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UMaterialInstanceDynamic* PoleMat = MakeColorMaterial(Pole, FLinearColor(0.95f, 0.75f, 0.1f)); // Bright gold
				if (PoleMat) Pole->GetStaticMeshComponent()->SetMaterial(0, PoleMat);
			}
		}
		UE_LOG(LogRomanEmpire, Log, TEXT("Spawned 3 gold mines"));
	}

	// Lumber yards (stacked brown cylinders = log piles)
	if (CylinderMesh)
	{
		struct FLumberData { FVector Pos; };
		TArray<FLumberData> LumberYards = {
			{{ -3000, -3000, 0 }},  // Forest edge
			{{ 4000, 4000, 0 }},    // Opposite forest edge
			{{ -6000, 1000, 0 }}    // Near other trees
		};

		for (const auto& Lumber : LumberYards)
		{
			// Create a pile of 3 logs
			for (int32 LogIdx = 0; LogIdx < 3; LogIdx++)
			{
				FVector WoodPos = Lumber.Pos + FVector(LogIdx * 80.0f - 80.0f, 0, 30.0f + LogIdx * 15.0f);
				FRotator WoodRot = FRotator(0, PrototypeRandomStream.FRandRange(0.0f, 30.0f), 90.0f);

				AStaticMeshActor* WoodActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), WoodPos, WoodRot, SP);
				if (WoodActor)
				{
					WoodActor->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
					WoodActor->SetActorScale3D(FVector(0.4f, 0.4f, 1.5f));
					WoodActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					UMaterialInstanceDynamic* WoodMat = MakeColorMaterial(WoodActor, FLinearColor(0.4f, 0.25f, 0.1f)); // Wood brown
					if (WoodMat) WoodActor->GetStaticMeshComponent()->SetMaterial(0, WoodMat);
				}
			}
		}
		UE_LOG(LogRomanEmpire, Log, TEXT("Spawned 3 lumber yards"));
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("World terrain spawned with ground, road, river, trees, hills, gold mines, lumber"));
}

void ARomanEmpireGameMode::SpawnInitialUnits()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AUnitBase*> RomanUnits;
	TArray<AUnitBase*> CarthaginianUnits;
	// Strategic inventory owns starting quantities/content IDs. These actors and
	// squad GUIDs are encounter-local materialization, not campaign army identities.
	const TArray<FCampaignArmy> Armies = CampaignManager ? CampaignManager->GetCampaignArmies() : TArray<FCampaignArmy>();
	for (const FCampaignArmy& Army : Armies)
	{
		if (Army.Faction != EFactionID::Rome && Army.Faction != EFactionID::Carthage) continue;
		for (const FCampaignArmyUnitStack& Stack : Army.Units)
		{
			for (int32 Index = 0; Index < Stack.Count; ++Index)
			{
				const bool bRoman = Army.Faction == EFactionID::Rome;
				const int32 FormationIndex = RomanUnits.Num();
				const FVector SpawnLoc = bRoman
					? FVector((FormationIndex % 5) * 200.0f - 400.0f, (FormationIndex / 5) * 200.0f - 200.0f, 50.0f)
					: FVector(3000.0f + PrototypeRandomStream.FRandRange(-400.0f, 400.0f),
						PrototypeRandomStream.FRandRange(-400.0f, 400.0f), 50.0f);
				if (AUnitBase* Unit = SpawnUnitFromDefinition(Stack.UnitID, Army.Faction,
					FTransform(FRotator(0.0f, bRoman ? 0.0f : 180.0f, 0.0f), SpawnLoc)))
				{
					if (bRoman) RomanUnits.Add(Unit);
					else CarthaginianUnits.Add(Unit);
				}
			}
		}
	}
	if (URomanUnitRegistrySubsystem* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>())
	{
		const FGuid RomanSquad = Registry->CreateSquad(RomanUnits);
		const FGuid CarthaginianSquad = Registry->CreateSquad(CarthaginianUnits);
		UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
		if (!Battle || !Battle->BeginEncounter(TEXT("CarthaginianCamp"), RomanSquad, CarthaginianSquad))
		{
			UE_LOG(LogRomanEmpire, Error, TEXT("Cannot initialize the tactical encounter from starting squads"));
		}
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Spawned %d Roman + %d Carthaginian units from campaign inventory"), RomanUnits.Num(), CarthaginianUnits.Num());
}

bool ARomanEmpireGameMode::MaterializeCampaignDeployment(FCampaignEncounterState& Deployment, TArray<AUnitBase*>& SpawnedUnits)
{
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (!Registry || !SpawnedUnits.IsEmpty()) return false;
	const FCampaignEncounterPlan Plan = Deployment.GetPlan();
	int32 AttackerIndex = 0, DefenderIndex = 0;
	for (const FCampaignArmy& Army : Plan.Armies)
	{
		const bool bAttacker = Army.Faction == Plan.Attacker;
		for (const FCampaignArmyUnitStack& Stack : Army.Units)
		{
			TArray<AUnitBase*> Members;
			for (int32 Index = 0; Index < Stack.Count; ++Index)
			{
				int32& FormationIndex = bAttacker ? AttackerIndex : DefenderIndex;
				const FVector Position((bAttacker ? 0.0 : 3000.0) + (FormationIndex % 5) * 200.0 - 400.0,
					(FormationIndex / 5) * 200.0 + (!bAttacker && Plan.PlayerFaction == Plan.Defender ? 550.0 : -200.0), 50.0);
				++FormationIndex;
				AUnitBase* Unit = SpawnUnitFromDefinition(Stack.UnitID, Army.Faction,
					FTransform(FRotator(0.0, bAttacker ? 0.0 : 180.0, 0.0), Position));
				if (!IsValid(Unit)) return false;
				SpawnedUnits.Add(Unit); Members.Add(Unit);
				Unit->CommandHold();
			}
			FCampaignSquadBinding Binding;
			Binding.SquadID = Registry->CreateSquad(Members); Binding.ArmyID = Army.ArmyID;
			Binding.UnitID = Stack.UnitID; Binding.UnitCount = Stack.Count;
			if (Deployment.BindSquad(Plan.EncounterID, Binding) != ECampaignEncounterFailure::None) return false;
		}
	}
	return true;
}

AUnitBase* ARomanEmpireGameMode::SpawnUnitFromDefinition(
	FName UnitID,
	EFactionID OwnerFaction,
	const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* Asset = Registry ? Registry->FindUnit(UnitID) : nullptr;
	UClass* UnitClass = Asset ? Asset->UnitClass.Get() : nullptr;
	if (!World || !UnitClass)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot spawn unit definition %s"), *UnitID.ToString());
		return nullptr;
	}

	AUnitBase* Unit = World->SpawnActorDeferred<AUnitBase>(
		UnitClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Unit || !Unit->InitializeUnit(UnitID, OwnerFaction))
	{
		if (Unit)
		{
			Unit->Destroy();
		}
		UE_LOG(LogRomanEmpire, Error, TEXT("Unit definition %s rejected during initialization"),
			*UnitID.ToString());
		return nullptr;
	}

	UGameplayStatics::FinishSpawningActor(Unit, SpawnTransform);
	return Unit;
}

void ARomanEmpireGameMode::SpawnInitialBuildings()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* CubeMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Cube"));
	UStaticMesh* CylinderMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	UStaticMesh* ConeMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Cone"));
	
	if (!CubeMesh) return;

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// --- ROMAN CAMP ---
	
	// The scenario's existing Roman barracks is a real, completed producer, not a decorative mesh.
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanBuildingDefinition* BarracksAsset = Registry ? Registry->FindBuilding(TEXT("Barracks")) : nullptr;
	UClass* BarracksClass = BarracksAsset ? BarracksAsset->BuildingClass.Get() : nullptr;
	FBox2D AuthoredBarracksFootprint(EForceInit::ForceInit);
	bool bHasAuthoredBarracks = false;
	if (BarracksClass && BarracksClass->IsChildOf(ABarracks::StaticClass()))
	{
		const FTransform Transform(FRotator::ZeroRotator, FVector(-600.0, -800.0, 200.0));
		ABarracks* Barracks = World->SpawnActorDeferred<ABarracks>(
			BarracksClass, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Barracks && Barracks->InitializeBuilding(TEXT("Barracks"), EFactionID::Rome))
		{
			UGameplayStatics::FinishSpawningActor(Barracks, Transform);
			if (IsValid(Barracks))
			{
				Barracks->CompleteInitialConstruction();
				if (Barracks->UsesAuthoredPresentation())
				{
					const FVector Center = Barracks->GetActorLocation();
					const FVector Half = Barracks->GetPlacementHalfExtent();
					AuthoredBarracksFootprint = FBox2D(FVector2D(Center.X-Half.X,Center.Y-Half.Y), FVector2D(Center.X+Half.X,Center.Y+Half.Y));
					bHasAuthoredBarracks = true;
				}
			}
		}
		else if (Barracks)
		{
			Barracks->Destroy();
		}
	}
	else
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Cannot spawn initial Roman barracks: authored Barracks class is unavailable"));
	}

	// Roman Forum (central plaza marker — flat gold cube)
	AStaticMeshActor* Forum = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(-300, -600, 10), FRotator::ZeroRotator, SP);
	if (Forum)
	{
		Forum->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		Forum->SetActorScale3D(FVector(5.0f, 5.0f, 0.2f));
		Forum->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UMaterialInstanceDynamic* Mat = MakeColorMaterial(Forum, FLinearColor(0.7f, 0.6f, 0.3f));
		if (Mat) Forum->GetStaticMeshComponent()->SetMaterial(0, Mat);
	}

	// Temple (tall column + roof)
	if (CylinderMesh)
	{
		// 4 columns
		for (int32 c = 0; c < 4; c++)
		{
			float cx = -800.0f + (c % 2) * 300.0f;
			float cy = 400.0f + (c / 2) * 300.0f;
			AStaticMeshActor* Column = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(cx, cy, 200), FRotator::ZeroRotator, SP);
			if (Column)
			{
				Column->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
				Column->SetActorScale3D(FVector(0.4f, 0.4f, 4.0f));
				Column->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				UMaterialInstanceDynamic* Mat = MakeColorMaterial(Column, FLinearColor(0.85f, 0.82f, 0.75f));
				if (Mat) Column->GetStaticMeshComponent()->SetMaterial(0, Mat);
			}
		}
	}

	// Temple roof
	if (ConeMesh)
	{
		AStaticMeshActor* Roof = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(-650, 550, 500), FRotator::ZeroRotator, SP);
		if (Roof)
		{
			Roof->GetStaticMeshComponent()->SetStaticMesh(ConeMesh);
			Roof->SetActorScale3D(FVector(5.0f, 5.0f, 2.0f));
			Roof->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UMaterialInstanceDynamic* Mat = MakeColorMaterial(Roof, FLinearColor(0.6f, 0.15f, 0.1f));
			if (Mat) Roof->GetStaticMeshComponent()->SetMaterial(0, Mat);
		}
	}

	// Houses (small cubes with red roofs)
	for (int32 h = 0; h < 8; h++)
	{
		float hx = -1200.0f + (h % 4) * 350.0f;
		float hy = -1200.0f + (h / 4) * 500.0f;
		// Generated decorative markers must not occupy a real authored building's footprint.
		// Include their roof overhang, not only the smaller colliding wall cube.
		if (bHasAuthoredBarracks && AuthoredBarracksFootprint.Intersect(
			FBox2D(FVector2D(hx-125,hy-150),FVector2D(hx+125,hy+150)))) continue;
		
		// Wall
		AStaticMeshActor* House = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(hx, hy, 75), FRotator::ZeroRotator, SP);
		if (House)
		{
			House->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			House->SetActorScale3D(FVector(2.0f, 2.5f, 1.5f));
			House->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			UMaterialInstanceDynamic* Mat = MakeColorMaterial(House, FLinearColor(0.75f, 0.65f, 0.5f));
			if (Mat) House->GetStaticMeshComponent()->SetMaterial(0, Mat);
		}
		
		// Roof
		if (ConeMesh)
		{
			AStaticMeshActor* HouseRoof = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(hx, hy, 200), FRotator::ZeroRotator, SP);
			if (HouseRoof)
			{
				HouseRoof->GetStaticMeshComponent()->SetStaticMesh(ConeMesh);
				HouseRoof->SetActorScale3D(FVector(2.5f, 3.0f, 1.0f));
				HouseRoof->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UMaterialInstanceDynamic* Mat = MakeColorMaterial(HouseRoof, FLinearColor(0.55f, 0.15f, 0.08f));
				if (Mat) HouseRoof->GetStaticMeshComponent()->SetMaterial(0, Mat);
			}
		}
	}

	// Camp walls (long thin cubes forming a perimeter)
	for (int32 w = 0; w < 4; w++)
	{
		FVector WallPos = FVector::ZeroVector;
		FVector WallScale = FVector::OneVector;
		switch(w) {
			case 0: WallPos = FVector(0, -2000, 75); WallScale = FVector(30.0f, 0.3f, 1.5f); break;   // South
			case 1: WallPos = FVector(0, 1500, 75); WallScale = FVector(30.0f, 0.3f, 1.5f); break;    // North
			case 2: WallPos = FVector(-1800, -250, 75); WallScale = FVector(0.3f, 25.0f, 1.5f); break; // West
			case 3: WallPos = FVector(1800, -250, 75); WallScale = FVector(0.3f, 25.0f, 1.5f); break;  // East
		}
		
		AStaticMeshActor* Wall = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), WallPos, FRotator::ZeroRotator, SP);
		if (Wall)
		{
			Wall->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			Wall->SetActorScale3D(WallScale);
			Wall->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			UMaterialInstanceDynamic* Mat = MakeColorMaterial(Wall, FLinearColor(0.4f, 0.35f, 0.3f));
			if (Mat) Wall->GetStaticMeshComponent()->SetMaterial(0, Mat);
		}
	}

	// Watchtower at gate
	if (CylinderMesh)
	{
		AStaticMeshActor* Tower = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(1800, -250, 200), FRotator::ZeroRotator, SP);
		if (Tower)
		{
			Tower->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
			Tower->SetActorScale3D(FVector(1.2f, 1.2f, 5.0f));
			Tower->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			UMaterialInstanceDynamic* Mat = MakeColorMaterial(Tower, FLinearColor(0.45f, 0.38f, 0.3f));
			if (Mat) Tower->GetStaticMeshComponent()->SetMaterial(0, Mat);
		}
	}

	// --- ENEMY CAMP (Carthaginian) ---
	ATacticalObjective* EnemyHQ = World->SpawnActor<ATacticalObjective>(ATacticalObjective::StaticClass(), FVector(3000, 0, 0), FRotator::ZeroRotator, SP);
	if (EnemyHQ)
	{
		EnemyHQ->GetObjectiveMesh()->SetStaticMesh(CubeMesh);
		UMaterialInstanceDynamic* Mat = MakeColorMaterial(EnemyHQ, FLinearColor(0.3f, 0.15f, 0.4f));
		if (Mat) EnemyHQ->GetObjectiveMesh()->SetMaterial(0, Mat);
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Buildings spawned: barracks, temple, 8 houses, walls, watchtower, enemy HQ"));
}
