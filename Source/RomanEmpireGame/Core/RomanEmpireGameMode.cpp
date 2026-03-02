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
#include "../Units/UnitBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

ARomanEmpireGameMode::ARomanEmpireGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CurrentPhase = EGamePhase::Tactical;  // Start in tactical, not strategic
	CurrentTurn = 1;
	PlayerGold = 1000;
	
	FactionManager = nullptr;
	WorldMapManager = nullptr;
	CampaignManager = nullptr;

	DefaultPawnClass = ASeamlessZoomCamera::StaticClass();
	PlayerControllerClass = ARomanEmpirePlayerController::StaticClass();
	HUDClass = ARomanEmpireHUD::StaticClass();
}

void ARomanEmpireGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Roman Empire Game Mode started - Turn %d"), CurrentTurn);
	
	InitializeManagers();
	SpawnWorldTerrain();
	SpawnInitialUnits();
	SpawnInitialBuildings();
	
	// Set camera to city/tactical zoom so units are visible
	ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (CameraPawn)
	{
		CameraPawn->SetZoomLevel(0.55f);  // City view - units clearly visible
		CameraPawn->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		UE_LOG(LogRomanEmpire, Log, TEXT("Camera set to tactical zoom 0.55"));
	}
	
	UpdateHUDResources();
}

void ARomanEmpireGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
	if (ZoomLevel <= ZOOM_TERRITORY_MAX) return EGamePhase::Strategic;
	else if (ZoomLevel <= ZOOM_CITY_MAX) return EGamePhase::Tactical;
	else if (ZoomLevel <= ZOOM_GROUND_MAX) return EGamePhase::Combat;
	else return EGamePhase::FirstPerson;
}

void ARomanEmpireGameMode::EndTurn()
{
	CurrentTurn++;
	UE_LOG(LogRomanEmpire, Log, TEXT("Turn %d started"), CurrentTurn);
	if (CampaignManager) CampaignManager->ProcessTurn();
	UpdateHUDResources();

	// Enemy AI: all non-Rome units advance toward Rome's territory (origin area)
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		AUnitBase* Unit = Cast<AUnitBase>(Actor);
		if (Unit && Unit->IsAlive() && Unit->GetOwnerFaction() != EFactionID::Rome && Unit->GetOwnerFaction() != EFactionID::None)
		{
			// Move toward origin (Roman base area) with some randomness
			FVector CurrentPos = Unit->GetActorLocation();
			FVector ToRome = FVector(0.0f, 0.0f, 0.0f) - CurrentPos;
			ToRome.Z = 0.0f;
			if (ToRome.SizeSquared() > 10000.0f)
			{
				ToRome.Normalize();
				float AdvanceDistance = FMath::RandRange(500.0f, 2000.0f);
				FVector NewDest = CurrentPos + ToRome * AdvanceDistance;
				NewDest.Z = CurrentPos.Z;
				Unit->CommandMoveTo(NewDest);
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("Turn %d — Enemy forces advancing!"), CurrentTurn));
	}
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
	static TMap<FString, UStaticMesh*> Cache;
	UStaticMesh** Found = Cache.Find(Path);
	if (Found) return *Found;
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

	// Spawn a large green ground plane
	UStaticMesh* PlaneMesh = GetEngineMesh(TEXT("/Engine/BasicShapes/Plane"));
	if (!PlaneMesh) return;

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Main ground — 500m x 500m green terrain
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0, 0, -10), FRotator::ZeroRotator, SP);
	if (Ground)
	{
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
		float X = FMath::RandRange(-8000.0f, 8000.0f);
		float Y = FMath::RandRange(-8000.0f, 8000.0f);
		
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
			float CanopySize = FMath::RandRange(1.5f, 3.0f);
			AStaticMeshActor* Canopy = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(X, Y, 300), FRotator::ZeroRotator, SP);
			if (Canopy)
			{
				Canopy->GetStaticMeshComponent()->SetStaticMesh(SphereMesh);
				Canopy->SetActorScale3D(FVector(CanopySize, CanopySize, CanopySize * 0.7f));
				Canopy->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				float Green = FMath::RandRange(0.2f, 0.5f);
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
				FRotator WoodRot = FRotator(0, FMath::RandRange(0.0f, 30.0f), 90.0f);

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

	// Spawn Roman Legionaries in formation near the Roman camp
	FVector RomanBase = FVector(0.0f, 0.0f, 50.0f);
	
	// Formation: 3 rows of 5
	for (int32 Row = 0; Row < 3; Row++)
	{
		for (int32 Col = 0; Col < 5; Col++)
		{
			FVector SpawnLoc = RomanBase + FVector(Col * 200.0f - 400.0f, Row * 200.0f - 200.0f, 0.0f);
			
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			
			ALegionary* Unit = World->SpawnActor<ALegionary>(ALegionary::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
			if (Unit)
			{
				Unit->SetOwnerFaction(EFactionID::Rome);
				Unit->SetCurrentHealth(Unit->GetUnitData().BaseStats.MaxHealth);
			}
		}
	}
	
	// Spawn enemy Legionaries (Carthage) further away
	FVector EnemyBase = FVector(3000.0f, 0.0f, 50.0f);
	for (int32 i = 0; i < 10; i++)
	{
		FVector SpawnLoc = EnemyBase + FVector(
			FMath::RandRange(-400.0f, 400.0f),
			FMath::RandRange(-400.0f, 400.0f),
			0.0f
		);
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		ALegionary* Unit = World->SpawnActor<ALegionary>(ALegionary::StaticClass(), SpawnLoc, FRotator(0, 180, 0), SpawnParams);
		if (Unit)
		{
			Unit->SetOwnerFaction(EFactionID::Carthage);
		}
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Spawned 15 Roman + 10 Carthaginian units"));
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
	
	// Barracks (large red-brown building)
	AStaticMeshActor* Barracks = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(-600, -800, 150), FRotator::ZeroRotator, SP);
	if (Barracks)
	{
		Barracks->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		Barracks->SetActorScale3D(FVector(4.0f, 6.0f, 3.0f));
		Barracks->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		UMaterialInstanceDynamic* Mat = MakeColorMaterial(Barracks, FLinearColor(0.5f, 0.2f, 0.1f));
		if (Mat) Barracks->GetStaticMeshComponent()->SetMaterial(0, Mat);
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
		FVector WallPos;
		FVector WallScale;
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
	AStaticMeshActor* EnemyHQ = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(3000, 0, 150), FRotator::ZeroRotator, SP);
	if (EnemyHQ)
	{
		EnemyHQ->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		EnemyHQ->SetActorScale3D(FVector(3.5f, 5.0f, 3.0f));
		EnemyHQ->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		UMaterialInstanceDynamic* Mat = MakeColorMaterial(EnemyHQ, FLinearColor(0.3f, 0.15f, 0.4f));
		if (Mat) EnemyHQ->GetStaticMeshComponent()->SetMaterial(0, Mat);
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Buildings spawned: barracks, temple, 8 houses, walls, watchtower, enemy HQ"));
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

void ARomanEmpireGameMode::SubtractGold(int32 Amount)
{
	PlayerGold = FMath::Max(0, PlayerGold - Amount);

	// Update HUD immediately
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		ARomanEmpireHUD* HUD = Cast<ARomanEmpireHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->DisplayGold = FMath::Max(0, HUD->DisplayGold - Amount);
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
			FString::Printf(TEXT("-%d Gold (Remaining: %d)"), Amount, PlayerGold));
	}
}
