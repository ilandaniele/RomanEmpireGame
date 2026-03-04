// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpirePlayerController.h"
#include "RomanEmpireGameMode.h"
#include "RomanEmpireHUD.h"
#include "../RomanEmpireGame.h"
#include "../Units/UnitBase.h"
#include "../Building/BuildingBase.h"
#include "../Building/Barracks.h"
#include "../Building/BuildingPlacementComponent.h"
#include "../Camera/SeamlessZoomCamera.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/FloatingPawnMovement.h"

ARomanEmpirePlayerController::ARomanEmpirePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	
	CurrentSelectionMode = ERomanSelectionMode::None;
	CurrentZoomLevel = 0.3f;
	ZoomSpeed = 2.0f;
	bIsInFirstPersonMode = false;
	bIsBoxSelecting = false;
	PossessedUnit = nullptr;
	GameMode = nullptr;
	CachedCameraPawn = nullptr;

	DefaultMappingContext = nullptr;
	IA_Select = nullptr;
	IA_Command = nullptr;
	IA_Zoom = nullptr;
	IA_Move = nullptr;
	IA_Look = nullptr;
	IA_EnterFPS = nullptr;
	IA_BuildMenu = nullptr;
	IA_Attack = nullptr;
	IA_Block = nullptr;
	IA_EndTurn = nullptr;
	IA_BuildKey1 = nullptr;
	IA_BuildKey2 = nullptr;
	IA_BuildKey3 = nullptr;
	IA_BuildKey4 = nullptr;
	IA_BuildKey5 = nullptr;
	IA_BuildKey6 = nullptr;
	CurrentBuildingCost = 0;
	TargetZoomLevel = 0.5f;
	CurrentZoomLevel = 0.5f;
}

void ARomanEmpirePlayerController::CreateInputActionsAndMappings()
{
	// Create Input Actions programmatically
	IA_Select = NewObject<UInputAction>(this, TEXT("IA_Select"));
	IA_Select->ValueType = EInputActionValueType::Boolean;

	IA_Command = NewObject<UInputAction>(this, TEXT("IA_Command"));
	IA_Command->ValueType = EInputActionValueType::Boolean;

	IA_Zoom = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	IA_Zoom->ValueType = EInputActionValueType::Axis1D;

	IA_Move = NewObject<UInputAction>(this, TEXT("IA_Move"));
	IA_Move->ValueType = EInputActionValueType::Axis2D;

	IA_Look = NewObject<UInputAction>(this, TEXT("IA_Look"));
	IA_Look->ValueType = EInputActionValueType::Axis2D;

	IA_EnterFPS = NewObject<UInputAction>(this, TEXT("IA_EnterFPS"));
	IA_EnterFPS->ValueType = EInputActionValueType::Boolean;

	IA_BuildMenu = NewObject<UInputAction>(this, TEXT("IA_BuildMenu"));
	IA_BuildMenu->ValueType = EInputActionValueType::Boolean;

	IA_Attack = NewObject<UInputAction>(this, TEXT("IA_Attack"));
	IA_Attack->ValueType = EInputActionValueType::Boolean;

	IA_Block = NewObject<UInputAction>(this, TEXT("IA_Block"));
	IA_Block->ValueType = EInputActionValueType::Boolean;

	IA_EndTurn = NewObject<UInputAction>(this, TEXT("IA_EndTurn"));
	IA_EndTurn->ValueType = EInputActionValueType::Boolean;

	// Create Input Mapping Context
	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("DefaultMappingContext"));

	// --- Key bindings ---

	// LMB = Select
	FEnhancedActionKeyMapping& SelectMapping = DefaultMappingContext->MapKey(IA_Select, EKeys::LeftMouseButton);

	// RMB = Command (move/attack)
	FEnhancedActionKeyMapping& CommandMapping = DefaultMappingContext->MapKey(IA_Command, EKeys::RightMouseButton);

	// Mouse Wheel = Zoom
	FEnhancedActionKeyMapping& ZoomMapping = DefaultMappingContext->MapKey(IA_Zoom, EKeys::MouseWheelAxis);

	// WASD = Move (2D axis: W=Y+, S=Y-, A=X-, D=X+)
    FEnhancedActionKeyMapping& MoveW = DefaultMappingContext->MapKey(IA_Move, EKeys::W);
    {
        UInputModifierSwizzleAxis* SwizzleYX = NewObject<UInputModifierSwizzleAxis>(this);
        SwizzleYX->Order = EInputAxisSwizzle::YXZ;
        MoveW.Modifiers.Add(SwizzleYX);
    }

    FEnhancedActionKeyMapping& MoveS = DefaultMappingContext->MapKey(IA_Move, EKeys::S);
    {
        UInputModifierSwizzleAxis* SwizzleYX = NewObject<UInputModifierSwizzleAxis>(this);
        SwizzleYX->Order = EInputAxisSwizzle::YXZ;
        MoveS.Modifiers.Add(SwizzleYX);
        UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
        MoveS.Modifiers.Add(Negate);
    }

    FEnhancedActionKeyMapping& MoveD = DefaultMappingContext->MapKey(IA_Move, EKeys::D);
    // D = positive X, no modifiers needed

    FEnhancedActionKeyMapping& MoveA = DefaultMappingContext->MapKey(IA_Move, EKeys::A);
    {
        UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
        MoveA.Modifiers.Add(Negate);
    }

	// Mouse XY = Look (for FPS mode)
	FEnhancedActionKeyMapping& LookMapping = DefaultMappingContext->MapKey(IA_Look, EKeys::Mouse2D);

	// F = Enter/Exit FPS mode
	FEnhancedActionKeyMapping& FPSMapping = DefaultMappingContext->MapKey(IA_EnterFPS, EKeys::F);

	// B = Build menu toggle
	FEnhancedActionKeyMapping& BuildMapping = DefaultMappingContext->MapKey(IA_BuildMenu, EKeys::B);

	// Left mouse = Attack (in FPS mode, same as select)
	// Right mouse = Block (in FPS mode, same as command)

	// T = End turn
	FEnhancedActionKeyMapping& EndTurnMapping = DefaultMappingContext->MapKey(IA_EndTurn, EKeys::T);

	// 1 = Build Barracks (when building menu is open)
	IA_BuildKey1 = NewObject<UInputAction>(this, TEXT("IA_BuildKey1"));
	IA_BuildKey1->ValueType = EInputActionValueType::Boolean;
	FEnhancedActionKeyMapping& BuildKey1Mapping = DefaultMappingContext->MapKey(IA_BuildKey1, EKeys::One);

	// 2-6 = Other buildings
	IA_BuildKey2 = NewObject<UInputAction>(this, TEXT("IA_BuildKey2"));
	IA_BuildKey2->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(IA_BuildKey2, EKeys::Two);

	IA_BuildKey3 = NewObject<UInputAction>(this, TEXT("IA_BuildKey3"));
	IA_BuildKey3->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(IA_BuildKey3, EKeys::Three);

	IA_BuildKey4 = NewObject<UInputAction>(this, TEXT("IA_BuildKey4"));
	IA_BuildKey4->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(IA_BuildKey4, EKeys::Four);

	IA_BuildKey5 = NewObject<UInputAction>(this, TEXT("IA_BuildKey5"));
	IA_BuildKey5->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(IA_BuildKey5, EKeys::Five);

	IA_BuildKey6 = NewObject<UInputAction>(this, TEXT("IA_BuildKey6"));
	IA_BuildKey6->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(IA_BuildKey6, EKeys::Six);

	UE_LOG(LogRomanEmpire, Log, TEXT("Input actions and mappings created programmatically"));
}

void ARomanEmpirePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Create input actions and mappings programmatically
	CreateInputActionsAndMappings();

	// Setup Enhanced Input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			UE_LOG(LogRomanEmpire, Log, TEXT("Enhanced Input mapping context registered"));
		}
	}
	
	// BIND INPUT ACTIONS — must happen here, AFTER actions are created
	// (SetupInputComponent runs before BeginPlay, so actions would be null there)
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_Select)
		{
			EIC->BindAction(IA_Select, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnSelectPressed);
			EIC->BindAction(IA_Select, ETriggerEvent::Completed, this, &ARomanEmpirePlayerController::OnSelectReleased);
		}
		if (IA_Command)
		{
			EIC->BindAction(IA_Command, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnCommandPressed);
		}
		if (IA_Zoom)
		{
			EIC->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &ARomanEmpirePlayerController::OnZoomInput);
		}
		if (IA_Move)
		{
			EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ARomanEmpirePlayerController::OnMoveInput);
		}
		if (IA_Look)
		{
			EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ARomanEmpirePlayerController::OnLookInput);
		}
		if (IA_EnterFPS)
		{
			EIC->BindAction(IA_EnterFPS, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnEnterFPSPressed);
		}
		if (IA_BuildMenu)
		{
			EIC->BindAction(IA_BuildMenu, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildMenuPressed);
		}
		if (IA_Attack)
		{
			EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnAttackPressed);
		}
		if (IA_Block)
		{
			EIC->BindAction(IA_Block, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBlockPressed);
			EIC->BindAction(IA_Block, ETriggerEvent::Completed, this, &ARomanEmpirePlayerController::OnBlockReleased);
		}
		if (IA_EndTurn)
		{
			EIC->BindAction(IA_EndTurn, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnEndTurnPressed);
		}
		if (IA_BuildKey1)
		{
			EIC->BindAction(IA_BuildKey1, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildingKey1Pressed);
		}
		if (IA_BuildKey2)
		{
			EIC->BindAction(IA_BuildKey2, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildingKey2Pressed);
		}
		if (IA_BuildKey3)
		{
			EIC->BindAction(IA_BuildKey3, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildingKey3Pressed);
		}
		if (IA_BuildKey4)
		{
			EIC->BindAction(IA_BuildKey4, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildingKey4Pressed);
		}
		if (IA_BuildKey5)
		{
			EIC->BindAction(IA_BuildKey5, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildingKey5Pressed);
		}
		if (IA_BuildKey6)
		{
			EIC->BindAction(IA_BuildKey6, ETriggerEvent::Started, this, &ARomanEmpirePlayerController::OnBuildingKey6Pressed);
		}
		UE_LOG(LogRomanEmpire, Log, TEXT("All input actions bound in BeginPlay"));
	}
	else
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("FAILED to get EnhancedInputComponent — input will not work!"));
	}
	
	// Cache game mode
	GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	
	// Create building placement component
	BuildingPlacementComponent = NewObject<UBuildingPlacementComponent>(this);
	if (BuildingPlacementComponent)
	{
		BuildingPlacementComponent->RegisterComponent();
	}

	// Set input mode: game + UI, mouse visible, not locked
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Player Controller initialized — all input active"));
}

void ARomanEmpirePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// NOTE: Input bindings are done in BeginPlay() because
	// SetupInputComponent runs before BeginPlay, and our
	// input actions are created programmatically in BeginPlay.
	// Binding here would fail silently since IA pointers are null.
}

void ARomanEmpirePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateZoom(DeltaSeconds);

	// Smooth zoom interpolation
	if (FMath::Abs(CurrentZoomLevel - TargetZoomLevel) > 0.001f)
	{
		CurrentZoomLevel = FMath::FInterpTo(CurrentZoomLevel, TargetZoomLevel, DeltaSeconds, 5.0f);
		if (ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(GetPawn()))
		{
			CameraPawn->SetTargetZoomLevel(CurrentZoomLevel);
		}
		if (GameMode)
		{
			GameMode->OnZoomLevelChanged(CurrentZoomLevel);
		}
	}
	
	// Edge scrolling in RTS mode
	if (!bIsInFirstPersonMode)
	{
		HandleEdgeScroll(DeltaSeconds);
	}

	// Update building placement preview if active
	if (CurrentSelectionMode == ERomanSelectionMode::BuildingPlacement && BuildingPlacementComponent)
	{
		FVector WorldPos, WorldDir;
		if (DeprojectMousePositionToWorld(WorldPos, WorldDir))
		{
			FVector TraceEnd = WorldPos + WorldDir * 100000.0f;
			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.bTraceComplex = false;
			if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldPos, TraceEnd, ECC_WorldStatic, Params))
			{
				BuildingPlacementComponent->UpdatePreview(HitResult.Location);
			}
			else if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldPos, TraceEnd, ECC_Visibility, Params))
			{
				BuildingPlacementComponent->UpdatePreview(HitResult.Location);
			}
			else
			{
				// Fallback: project onto Z=0 plane
				if (FMath::Abs(WorldDir.Z) > 0.001f)
				{
					float T = -WorldPos.Z / WorldDir.Z;
					if (T > 0.0f)
					{
						FVector GroundPos = WorldPos + WorldDir * T;
						GroundPos.Z = 0.0f;
						BuildingPlacementComponent->UpdatePreview(GroundPos);
					}
				}
			}
		}
	}
}

void ARomanEmpirePlayerController::HandleEdgeScroll(float DeltaTime)
{
	ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(GetPawn());
	if (!CameraPawn) return;

	// Validate viewport
	int32 SizeX, SizeY;
	GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return;

	// Get mouse position - validate it's actually inside the viewport
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY)) return; // Mouse not over window
	
	// Additional safety: if mouse is at exactly (0,0) it's likely invalid
	// Also reject if mouse is outside viewport bounds
	if ((MouseX <= 1.0f && MouseY <= 1.0f) || 
		MouseX < 0.0f || MouseY < 0.0f || 
		MouseX >= SizeX || MouseY >= SizeY)
	{
		return;
	}

	const float EdgeThreshold = 15.0f;
	FVector2D PanDirection = FVector2D::ZeroVector;

	if (MouseX < EdgeThreshold) PanDirection.X -= 1.0f;           // Left
	if (MouseX > SizeX - EdgeThreshold) PanDirection.X += 1.0f;   // Right
	if (MouseY < EdgeThreshold) PanDirection.Y += 1.0f;            // Up/Forward
	if (MouseY > SizeY - EdgeThreshold) PanDirection.Y -= 1.0f;    // Down/Backward

	if (!PanDirection.IsNearlyZero())
	{
		PanDirection.Normalize();
		CameraPawn->PanCamera(PanDirection);
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
		CurrentSelectionMode = ERomanSelectionMode::BuildingPlacement;
		BuildingPlacementComponent->StartPlacement(BuildingClass);
		UE_LOG(LogRomanEmpire, Log, TEXT("Started building placement"));
	}
}

void ARomanEmpirePlayerController::CancelBuildingPlacement()
{
	if (BuildingPlacementComponent)
	{
		BuildingPlacementComponent->CancelPlacement();
		CurrentSelectionMode = ERomanSelectionMode::None;
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

	// Store the camera pawn so we can restore it later
	APawn* CurrentPawn = GetPawn();
	if (ASeamlessZoomCamera* CamPawn = Cast<ASeamlessZoomCamera>(CurrentPawn))
	{
		CachedCameraPawn = CamPawn;
	}

	PossessedUnit = UnitToPossess;
	bIsInFirstPersonMode = true;
	bShowMouseCursor = false;

	// Mark unit as player-possessed
	UnitToPossess->SetPossessedByPlayer(true);

	// Actually possess the unit pawn so the camera follows it
	Possess(UnitToPossess);

	SetInputMode(FInputModeGameOnly());

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

	// Unpossess the unit
	if (PossessedUnit)
	{
		PossessedUnit->SetPossessedByPlayer(false);

		// Spawn AI controller for the unit again
		PossessedUnit->SpawnDefaultController();
	}

	// Re-possess the camera pawn
	if (CachedCameraPawn)
	{
		Possess(CachedCameraPawn);
	}

	PossessedUnit = nullptr;
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	if (GameMode)
	{
		GameMode->OnZoomLevelChanged(CurrentZoomLevel);
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("Exited FPS mode — camera restored"));
}


void ARomanEmpirePlayerController::SetTargetZoom(float NewZoom)
{
	// Cap at 0.70 — Tactical max. FPS is explicit mode only.
	TargetZoomLevel = FMath::Clamp(NewZoom, 0.0f, 0.70f);
}

void ARomanEmpirePlayerController::OnSelectPressed()
{
	if (bIsInFirstPersonMode)
	{
		OnAttackPressed();
		return;
	}

	// Block interactions in Strategic view (zoom < 0.25)
	if (CurrentZoomLevel < 0.25f)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("Zoom in for Tactical View to interact"));
		return;
	}
	
	if (CurrentSelectionMode == ERomanSelectionMode::BuildingPlacement)
	{
		if (BuildingPlacementComponent && BuildingPlacementComponent->CanPlace())
		{
			if (GameMode)
			{
				GameMode->SubtractGold(CurrentBuildingCost);
			}
			BuildingPlacementComponent->ConfirmPlacement();
			CurrentSelectionMode = ERomanSelectionMode::None;
		}
		return;
	}
	
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
		OnBlockPressed();
		return;
	}
	
	if (CurrentSelectionMode == ERomanSelectionMode::BuildingPlacement)
	{
		CancelBuildingPlacement();
		return;
	}
	
	// Issue movement command to selected units
	if (SelectedUnits.Num() > 0)
	{
		// Manual line trace from camera through mouse position — most reliable
		FVector WorldPos, WorldDir;
		if (DeprojectMousePositionToWorld(WorldPos, WorldDir))
		{
			FVector TraceEnd = WorldPos + WorldDir * 50000.0f;
			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.bTraceComplex = false;

			if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldPos, TraceEnd, ECC_WorldStatic, Params))
			{
				FVector Dest = HitResult.Location;

				// Check if we clicked on an enemy unit
				AUnitBase* HitUnit = Cast<AUnitBase>(HitResult.GetActor());
				if (!HitUnit)
				{
					// Try parent actor
					if (HitResult.GetActor())
					{
						HitUnit = Cast<AUnitBase>(HitResult.GetActor()->GetOwner());
					}
				}

				if (HitUnit && HitUnit->GetOwnerFaction() != EFactionID::Rome && HitUnit->IsAlive())
				{
					// Attack enemy!
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
							FString::Printf(TEXT("Attacking: %s"), *HitUnit->GetName()));
					}
					for (AUnitBase* Unit : SelectedUnits)
					{
						if (Unit && Unit->GetOwnerFaction() == EFactionID::Rome)
						{
							Unit->CommandAttack(HitUnit);
						}
					}
				}
				else
				{
					// Move to location
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
							FString::Printf(TEXT("Move %d units to: %.0f, %.0f"),
								SelectedUnits.Num(), Dest.X, Dest.Y));
					}
					for (AUnitBase* Unit : SelectedUnits)
					{
						if (Unit && Unit->GetOwnerFaction() == EFactionID::Rome)
						{
							Unit->CommandMoveTo(Dest);
						}
					}
				}
			}
			else
			{
				// Fallback: project onto Z=0 plane
				if (FMath::Abs(WorldDir.Z) > 0.001f)
				{
					float T = -WorldPos.Z / WorldDir.Z;
					if (T > 0.0f)
					{
						FVector Dest = WorldPos + WorldDir * T;
						Dest.Z = 0.0f;

						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
								FString::Printf(TEXT("Move (fallback) to: %.0f, %.0f"),
									Dest.X, Dest.Y));
						}

						for (AUnitBase* Unit : SelectedUnits)
						{
							if (Unit && Unit->GetOwnerFaction() == EFactionID::Rome)
							{
								Unit->CommandMoveTo(Dest);
							}
						}
					}
				}
			}
		}
	}
}

void ARomanEmpirePlayerController::OnZoomInput(const FInputActionValue& Value)
{
	// Fixed step per scroll notch
	float ScrollVal = Value.Get<float>();
	if (ScrollVal > 0.0f)
		SetTargetZoom(CurrentZoomLevel + 0.03f);
	else if (ScrollVal < 0.0f)
		SetTargetZoom(CurrentZoomLevel - 0.03f);
}

void ARomanEmpirePlayerController::OnMoveInput(const FInputActionValue& Value)
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		FVector2D MoveValue = Value.Get<FVector2D>();
		PossessedUnit->MoveInput(MoveValue);
	}
	else
	{
		// RTS mode: WASD moves the camera using PanCamera
		ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(GetPawn());
		if (CameraPawn)
		{
			FVector2D MoveValue = Value.Get<FVector2D>();
			CameraPawn->PanCamera(MoveValue);
		}
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
	if (ARomanEmpireHUD* REHUD = Cast<ARomanEmpireHUD>(GetHUD()))
	{
		REHUD->ToggleBuildingMenu();
	}

	// If building menu is now visible, enter building placement with Barracks
	// (The user presses B to open, then 1-6 keys or clicks to select a building)
	UE_LOG(LogRomanEmpire, Log, TEXT("Build menu toggled"));
}

void ARomanEmpirePlayerController::OnBuildingKey1Pressed()
{
	CurrentBuildingCost = 200;
	StartBuildingPlacement(ABarracks::StaticClass());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Building: Barracks (200 Gold)"));
}

void ARomanEmpirePlayerController::OnBuildingKey2Pressed()
{
	CurrentBuildingCost = 100;
	StartBuildingPlacement(ABuildingBase::StaticClass());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Building: Farm (100 Gold)"));
}

void ARomanEmpirePlayerController::OnBuildingKey3Pressed()
{
	CurrentBuildingCost = 150;
	StartBuildingPlacement(ABuildingBase::StaticClass());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Building: Mine (150 Gold)"));
}

void ARomanEmpirePlayerController::OnBuildingKey4Pressed()
{
	CurrentBuildingCost = 120;
	StartBuildingPlacement(ABuildingBase::StaticClass());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Building: Lumber Mill (120 Gold)"));
}

void ARomanEmpirePlayerController::OnBuildingKey5Pressed()
{
	CurrentBuildingCost = 80;
	StartBuildingPlacement(ABuildingBase::StaticClass());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Building: Wall (80 Gold)"));
}

void ARomanEmpirePlayerController::OnBuildingKey6Pressed()
{
	CurrentBuildingCost = 300;
	StartBuildingPlacement(ABuildingBase::StaticClass());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Building: Temple (300 Gold)"));
}

void ARomanEmpirePlayerController::OnAttackPressed()
{
	if (bIsInFirstPersonMode && PossessedUnit)
	{
		// FPS melee attack: line trace 200cm forward from camera
		FVector CamLoc;
		FRotator CamRot;
		GetPlayerViewPoint(CamLoc, CamRot);

		FVector TraceStart = CamLoc;
		FVector TraceEnd = CamLoc + CamRot.Vector() * 200.0f;

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(PossessedUnit);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Pawn, Params))
		{
			AUnitBase* HitUnit = Cast<AUnitBase>(HitResult.GetActor());
			if (HitUnit && HitUnit->GetOwnerFaction() != EFactionID::Rome && HitUnit->IsAlive())
			{
				HitUnit->TakeCombatDamage(25.0f, PossessedUnit, false);
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
						FString::Printf(TEXT("HIT! %s (-25 HP)"), *HitUnit->GetName()));
				}
			}
		}

		// Always play punch animation
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

void ARomanEmpirePlayerController::OnEndTurnPressed()
{
	if (GameMode)
	{
		GameMode->EndTurn();
		UE_LOG(LogRomanEmpire, Log, TEXT("Turn ended by player"));
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
		// Box selection — find all units within screen rect
		ClearSelection();
		
		TArray<AActor*> AllUnits;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllUnits);
		
		FVector2D MinBox(FMath::Min(BoxSelectStart.X, CurrentMousePos.X), FMath::Min(BoxSelectStart.Y, CurrentMousePos.Y));
		FVector2D MaxBox(FMath::Max(BoxSelectStart.X, CurrentMousePos.X), FMath::Max(BoxSelectStart.Y, CurrentMousePos.Y));
		
		for (AActor* Actor : AllUnits)
		{
			AUnitBase* Unit = Cast<AUnitBase>(Actor);
			if (!Unit) continue;
			
			FVector2D ScreenPos;
			if (ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenPos))
			{
				if (ScreenPos.X >= MinBox.X && ScreenPos.X <= MaxBox.X &&
					ScreenPos.Y >= MinBox.Y && ScreenPos.Y <= MaxBox.Y)
				{
					SelectedUnits.Add(Unit);
					Unit->SetSelected(true);
				}
			}
		}
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Box selected %d units"), SelectedUnits.Num());
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
