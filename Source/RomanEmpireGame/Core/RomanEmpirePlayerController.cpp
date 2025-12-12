// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpirePlayerController.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/Units/UnitBase.h"
#include "RomanEmpireGame/Building/BuildingBase.h"
#include "RomanEmpireGame/Building/BuildingPlacementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

ARomanEmpirePlayerController::ARomanEmpirePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	
	CurrentSelectionMode = ESelectionMode::None;
	CurrentZoomLevel = 0.3f; // Start at territory view
	ZoomSpeed = 2.0f;
	bIsInFirstPersonMode = false;
	bIsBoxSelecting = false;
	PossessedUnit = nullptr;
	GameMode = nullptr;
}

void ARomanEmpirePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Setup Enhanced Input
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Cache game mode
	GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	
	// Create building placement component
	BuildingPlacementComponent = NewObject<UBuildingPlacementComponent>(this);
	if (BuildingPlacementComponent)
	{
		BuildingPlacementComponent->RegisterComponent();
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Player Controller initialized"));
}

void ARomanEmpirePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Selection
		if (IA_Select)
		{
			EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnSelectPressed);
			EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Completed, this, &ARomanEmpirePlayerController::OnSelectReleased);
		}
		
		// Command (right click)
		if (IA_Command)
		{
			EnhancedInputComponent->BindAction(IA_Command, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnCommandPressed);
		}
		
		// Zoom
		if (IA_Zoom)
		{
			EnhancedInputComponent->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &ARomanEmpirePlayerController::OnZoomInput);
		}
		
		// Movement
		if (IA_Move)
		{
			EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ARomanEmpirePlayerController::OnMoveInput);
		}
		
		// Look
		if (IA_Look)
		{
			EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ARomanEmpirePlayerController::OnLookInput);
		}
		
		// Enter FPS
		if (IA_EnterFPS)
		{
			EnhancedInputComponent->BindAction(IA_EnterFPS, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnEnterFPSPressed);
		}
		
		// Build Menu
		if (IA_BuildMenu)
		{
			EnhancedInputComponent->BindAction(IA_BuildMenu, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildMenuPressed);
		}
		
		// Combat
		if (IA_Attack)
		{
			EnhancedInputComponent->BindAction(IA_Attack, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnAttackPressed);
		}
		
		if (IA_Block)
		{
			EnhancedInputComponent->BindAction(IA_Block, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBlockPressed);
			EnhancedInputComponent->BindAction(IA_Block, ETriggerEvent::Completed, this, &ARomanEmpirePlayerController::OnBlockReleased);
		}
	}
}

void ARomanEmpirePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateZoom(DeltaSeconds);
	
	// Update building placement preview if active
	if (CurrentSelectionMode == ESelectionMode::BuildingPlacement && BuildingPlacementComponent)
	{
		FHitResult HitResult;
		GetHitResultUnderCursor(ECC_Visibility, true, HitResult);
		BuildingPlacementComponent->UpdatePreview(HitResult.Location);
	}
}

void ARomanEmpirePlayerController::SelectUnit(AUnitBase* Unit)
{
	ClearSelection();
	if (Unit)
	{
		SelectedUnits.Add(Unit);
		Unit->SetSelected(true);
		UE_LOG(LogRomanEmpire, Verbose, TEXT("Selected unit: %s"), *Unit->GetName());
	}
}

void ARomanEmpirePlayerController::SelectUnits(const TArray<AUnitBase*>& Units)
{
	ClearSelection();
	for (AUnitBase* Unit : Units)
	{
		if (Unit)
		{
			SelectedUnits.Add(Unit);
			Unit->SetSelected(true);
		}
	}
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Selected %d units"), SelectedUnits.Num());
}

void ARomanEmpirePlayerController::ClearSelection()
{
	for (AUnitBase* Unit : SelectedUnits)
	{
		if (Unit)
		{
			Unit->SetSelected(false);
		}
	}
	SelectedUnits.Empty();
}

void ARomanEmpirePlayerController::StartBuildingPlacement(TSubclassOf<ABuildingBase> BuildingClass)
{
	if (BuildingPlacementComponent && BuildingClass)
	{
		CurrentSelectionMode = ESelectionMode::BuildingPlacement;
		BuildingPlacementComponent->StartPlacement(BuildingClass);
		UE_LOG(LogRomanEmpire, Log, TEXT("Started building placement"));
	}
}

void ARomanEmpirePlayerController::CancelBuildingPlacement()
{
	if (BuildingPlacementComponent)
	{
		BuildingPlacementComponent->CancelPlacement();
		CurrentSelectionMode = ESelectionMode::None;
		UE_LOG(LogRomanEmpire, Log, TEXT("Cancelled building placement"));
	}
}

void ARomanEmpirePlayerController::EnterFirstPersonMode(AUnitBase* UnitToPossess)
{
	if (!UnitToPossess)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Cannot enter FPS mode: no unit specified"));
		return;
	}
	
	PossessedUnit = UnitToPossess;
	bIsInFirstPersonMode = true;
	bShowMouseCursor = false;
	
	// Lock input to first-person mode
	SetInputMode(FInputModeGameOnly());
	
	// Notify game mode
	if (GameMode)
	{
		GameMode->SetGamePhase(EGamePhase::FirstPerson);
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Entered FPS mode with unit: %s"), *UnitToPossess->GetName());
}

void ARomanEmpirePlayerController::ExitFirstPersonMode()
{
	if (!bIsInFirstPersonMode)
	{
		return;
	}
	
	bIsInFirstPersonMode = false;
	PossessedUnit = nullptr;
	bShowMouseCursor = true;
	
	// Restore RTS input mode
	SetInputMode(FInputModeGameAndUI());
	
	// Notify game mode
	if (GameMode)
	{
		GameMode->OnZoomLevelChanged(CurrentZoomLevel);
	}
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Exited FPS mode"));
}

void ARomanEmpirePlayerController::SetTargetZoom(float NewZoom)
{
	CurrentZoomLevel = FMath::Clamp(NewZoom, 0.0f, 1.0f);
	
	if (GameMode)
	{
		GameMode->OnZoomLevelChanged(CurrentZoomLevel);
	}
}

void ARomanEmpirePlayerController::OnSelectPressed()
{
	if (bIsInFirstPersonMode)
	{
		return; // In FPS mode, select = attack
	}
	
	if (CurrentSelectionMode == ESelectionMode::BuildingPlacement)
	{
		// Confirm building placement
		if (BuildingPlacementComponent && BuildingPlacementComponent->CanPlace())
		{
			BuildingPlacementComponent->ConfirmPlacement();
			CurrentSelectionMode = ESelectionMode::None;
		}
		return;
	}
	
	// Start box selection
	GetMousePosition(BoxSelectStart.X, BoxSelectStart.Y);
	bIsBoxSelecting = true;
}

void ARomanEmpirePlayerController::OnSelectReleased()
{
	if (bIsBoxSelecting)
	{
		PerformBoxSelect();
		bIsBoxSelecting = false;
	}
}

void ARomanEmpirePlayerController::OnCommandPressed()
{
	if (bIsInFirstPersonMode)
	{
		return; // In FPS mode, command = block
	}
	
	if (CurrentSelectionMode == ESelectionMode::BuildingPlacement)
	{
		CancelBuildingPlacement();
		return;
	}
	
	// Issue movement command to selected units
	if (SelectedUnits.Num() > 0)
	{
		FHitResult HitResult;
		if (GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
		{
			for (AUnitBase* Unit : SelectedUnits)
			{
				if (Unit)
				{
					Unit->CommandMoveTo(HitResult.Location);
				}
			}
		}
	}
}

void ARomanEmpirePlayerController::OnZoomInput(const FInputActionValue& Value)
{
	float ZoomDelta = Value.Get<float>() * ZoomSpeed * GetWorld()->GetDeltaSeconds();
	SetTargetZoom(CurrentZoomLevel + ZoomDelta);
}

void ARomanEmpirePlayerController::OnMoveInput(const FInputActionValue& Value)
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		FVector2D MoveValue = Value.Get<FVector2D>();
		PossessedUnit->MoveInput(MoveValue);
	}
}

void ARomanEmpirePlayerController::OnLookInput(const FInputActionValue& Value)
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		FVector2D LookValue = Value.Get<FVector2D>();
		PossessedUnit->LookInput(LookValue);
	}
}

void ARomanEmpirePlayerController::OnEnterFPSPressed()
{
	if (bIsInFirstPersonMode)
	{
		ExitFirstPersonMode();
	}
	else if (SelectedUnits.Num() > 0)
	{
		EnterFirstPersonMode(SelectedUnits[0]);
	}
}

void ARomanEmpirePlayerController::OnBuildMenuPressed()
{
	// Toggle build menu - TODO: Implement UI
	UE_LOG(LogRomanEmpire, Log, TEXT("Build menu toggled"));
}

void ARomanEmpirePlayerController::OnAttackPressed()
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		PossessedUnit->PerformAttack();
	}
}

void ARomanEmpirePlayerController::OnBlockPressed()
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		PossessedUnit->StartBlocking();
	}
}

void ARomanEmpirePlayerController::OnBlockReleased()
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		PossessedUnit->StopBlocking();
	}
}

void ARomanEmpirePlayerController::UpdateZoom(float DeltaTime)
{
	// Smooth zoom interpolation handled by camera component
}

void ARomanEmpirePlayerController::PerformBoxSelect()
{
	FVector2D CurrentMousePos;
	GetMousePosition(CurrentMousePos.X, CurrentMousePos.Y);
	
	// Check if it's a click (small movement) or drag (box select)
	float Distance = FVector2D::Distance(BoxSelectStart, CurrentMousePos);
	
	if (Distance < 10.0f)
	{
		// Single click selection
		AActor* HitActor = GetActorUnderCursor();
		if (AUnitBase* Unit = Cast<AUnitBase>(HitActor))
		{
			SelectUnit(Unit);
		}
		else
		{
			ClearSelection();
		}
	}
	else
	{
		// Box selection - find all units within the box
		// TODO: Implement proper screen-space box selection
		ClearSelection();
	}
}

AActor* ARomanEmpirePlayerController::GetActorUnderCursor() const
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Pawn, true, HitResult))
	{
		return HitResult.GetActor();
	}
	return nullptr;
}
