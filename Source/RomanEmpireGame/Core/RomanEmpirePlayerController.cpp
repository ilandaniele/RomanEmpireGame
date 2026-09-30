// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpirePlayerController.h"
#include "RomanEmpireGameMode.h"
#include "Core/RomanEmpireGameState.h"
#include "RomanEmpireGame.h"
#include "Content/RomanContentRegistry.h"
#include "Units/RomanUnitRegistry.h"
#include "Units/RomanFormationLayout.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Units/UnitBase.h"
#include "Building/BuildingBase.h"
#include "Building/Barracks.h"
#include "Building/BuildingPlacementComponent.h"
#include "Camera/SeamlessZoomCamera.h"
#include "Camera/PlayerCameraManager.h"
#include "UI/RomanEmpireMainWidget.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputAction.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Input/RomanInputConfig.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "InputTriggers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/FloatingPawnMovement.h"

ARomanEmpirePlayerController::ARomanEmpirePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	
	CurrentSelectionMode = ERomanSelectionMode::None;
	bIsBoxSelecting = false;
	PossessedUnit = nullptr;
	GameMode = nullptr;
	CachedCameraPawn = nullptr;

	InputConfigurationAsset = TSoftObjectPtr<URomanInputConfig>(
		FSoftObjectPath(TEXT("/Game/Input/DA_RomanInputConfig.DA_RomanInputConfig")));
	SelectedBuilding = nullptr;
	MainWidgetClass = URomanEmpireMainWidget::StaticClass();
	MainWidget = nullptr;
}

bool ARomanEmpirePlayerController::LoadInputConfiguration()
{
	if (InputConfiguration)
	{
		return true;
	}
	URomanInputConfig* Configuration = InputConfigurationAsset.LoadSynchronous();
	TArray<FText> Errors;
	if (!Configuration || !Configuration->ValidateConfiguration(Errors))
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Input configuration is missing or invalid: %s"),
			*InputConfigurationAsset.ToString());
		for (const FText& Error : Errors)
		{
			UE_LOG(LogRomanEmpire, Error, TEXT("Input configuration: %s"), *Error.ToString());
		}
		return false;
	}
	InputConfiguration = Configuration;
	return true;
}

void ARomanEmpirePlayerController::BeginPlay()
{
	Super::BeginPlay();
	bControllerEndingPlay = false;

	// Keep commands on the rules object; public reads/events come from GameState.
	GameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	PublicGameState = GetWorld() ? GetWorld()->GetGameState<ARomanEmpireGameState>() : nullptr;
	if (PublicGameState)
	{
		PublicGameState->OnGameplayInitialized.AddUniqueDynamic(
			this, &ARomanEmpirePlayerController::HandleGameplayInitialized);
		PublicGameState->OnGameplayModeChanged.AddUniqueDynamic(
			this, &ARomanEmpirePlayerController::HandleGameplayModeChanged);
		PublicGameState->OnPlayerFactionChanged.AddUniqueDynamic(
			this, &ARomanEmpirePlayerController::HandlePlayerFactionChanged);
	}
	ApplyInputContexts();
	CachedCameraPawn = Cast<ASeamlessZoomCamera>(GetPawn());
	if (CachedCameraPawn)
	{
		CachedCameraPawn->OnZoomLevelChanged.AddDynamic(
			this, &ARomanEmpirePlayerController::HandleCameraZoomLevelChanged);
		if (GameMode)
		{
			GameMode->SetExplorationModeForZoomLevel(CachedCameraPawn->GetZoomLevelEnum());
		}
	}
	
	// Create building placement component
	BuildingPlacementComponent = NewObject<UBuildingPlacementComponent>(this);
	if (BuildingPlacementComponent)
	{
		BuildingPlacementComponent->RegisterComponent();
	}

	const TSubclassOf<URomanEmpireMainWidget> WidgetClass = MainWidgetClass
		? MainWidgetClass.Get()
		: URomanEmpireMainWidget::StaticClass();
	MainWidget = IsLocalController() ? CreateWidget<URomanEmpireMainWidget>(this, WidgetClass) : nullptr;
	if (MainWidget)
	{
		MainWidget->AddToViewport();
	}
	else if (IsLocalController())
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Failed to create the production UMG root widget"));
	}

	// Set input mode: game + UI, mouse visible, not locked
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Player Controller initialized with authored mode-specific input"));
}

void ARomanEmpirePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bControllerEndingPlay = true;
	ExitFirstPersonMode();
	CancelTransientInput();
	RemoveOwnedInputContexts();
	if (PublicGameState)
	{
		PublicGameState->OnGameplayInitialized.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandleGameplayInitialized);
		PublicGameState->OnGameplayModeChanged.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandleGameplayModeChanged);
		PublicGameState->OnPlayerFactionChanged.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandlePlayerFactionChanged);
	}
	if (MainWidget)
	{
		MainWidget->RemoveFromParent();
		MainWidget = nullptr;
	}
	ClearSelectedActors();
	if (CachedCameraPawn)
	{
		CachedCameraPawn->OnZoomLevelChanged.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandleCameraZoomLevelChanged);
	}
	if (PossessedUnit)
	{
		PossessedUnit->OnUnitDied.RemoveDynamic(this, &ARomanEmpirePlayerController::HandlePossessedUnitDied);
		PossessedUnit->OnDestroyed.RemoveDynamic(this, &ARomanEmpirePlayerController::HandlePossessedActorDestroyed);
		PossessedUnit->OnOwnerFactionChanged.RemoveDynamic(this, &ARomanEmpirePlayerController::HandlePossessedUnitOwnerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void ARomanEmpirePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC || !LoadInputConfiguration())
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Enhanced Input setup failed; gameplay controls are disabled"));
		return;
	}
	EIC->ClearBindingsForObject(this);
	EIC->BindAction(InputConfiguration->Select, ETriggerEvent::Started, this, &ThisClass::OnSelectPressed);
	EIC->BindAction(InputConfiguration->Select, ETriggerEvent::Completed, this, &ThisClass::OnSelectReleased);
	EIC->BindAction(InputConfiguration->Select, ETriggerEvent::Canceled, this, &ThisClass::OnSelectCanceled);
	EIC->BindActionValue(InputConfiguration->SelectionModifier);
	EIC->BindAction(InputConfiguration->Command, ETriggerEvent::Started, this, &ThisClass::OnCommandPressed);
	EIC->BindAction(InputConfiguration->CameraPan, ETriggerEvent::Triggered, this, &ThisClass::OnCameraPanInput);
	EIC->BindAction(InputConfiguration->Zoom, ETriggerEvent::Triggered, this, &ThisClass::OnZoomInput);
	EIC->BindAction(InputConfiguration->SoldierMove, ETriggerEvent::Triggered, this, &ThisClass::OnSoldierMoveInput);
	EIC->BindAction(InputConfiguration->Look, ETriggerEvent::Triggered, this, &ThisClass::OnLookInput);
	EIC->BindAction(InputConfiguration->EnterPossession, ETriggerEvent::Started, this, &ThisClass::OnEnterFPSPressed);
	EIC->BindAction(InputConfiguration->ExitPossession, ETriggerEvent::Started, this, &ThisClass::ExitFirstPersonMode);
	EIC->BindAction(InputConfiguration->BuildMenu, ETriggerEvent::Started, this, &ThisClass::OnBuildMenuPressed);
	EIC->BindAction(InputConfiguration->Attack, ETriggerEvent::Started, this, &ThisClass::OnAttackPressed);
	EIC->BindAction(InputConfiguration->Block, ETriggerEvent::Started, this, &ThisClass::OnBlockPressed);
	EIC->BindAction(InputConfiguration->Block, ETriggerEvent::Completed, this, &ThisClass::OnBlockReleased);
	EIC->BindAction(InputConfiguration->Block, ETriggerEvent::Canceled, this, &ThisClass::OnBlockReleased);
	EIC->BindAction(InputConfiguration->EndTurn, ETriggerEvent::Started, this, &ThisClass::OnEndTurnPressed);
	EIC->BindAction(InputConfiguration->Cancel, ETriggerEvent::Started, this, &ThisClass::OnCancelPressed);
	for (int32 Index = 0; Index < InputConfiguration->BuildingSlots.Num(); ++Index)
	{
		EIC->BindAction(InputConfiguration->BuildingSlots[Index], ETriggerEvent::Started,
			this, &ThisClass::OnBuildingSlotPressed, Index + 1);
	}
	ApplyInputContexts();
}

void ARomanEmpirePlayerController::RemoveOwnedInputContexts()
{
	if (InputConfiguration)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(InputConfiguration->StrategicContext);
			Subsystem->RemoveMappingContext(InputConfiguration->TacticalContext);
			Subsystem->RemoveMappingContext(InputConfiguration->PossessionContext);
			Subsystem->RemoveMappingContext(InputConfiguration->SharedContext);
		}
	}
	ActiveGameplayContext = nullptr;
}

void ARomanEmpirePlayerController::ApplyInputContexts()
{
	if (bControllerEndingPlay || !PublicGameState || !PublicGameState->IsGameplayInitialized() || !InputConfiguration || !IsLocalController())
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}
	UInputMappingContext* Desired = InputConfiguration->GetContextForMode(PublicGameState->GetCurrentGameplayMode());
	if (Desired == ActiveGameplayContext && HasActiveModeContext())
	{
		return;
	}
	CaptureHeldMovementKeys(Desired);
	CancelTransientInput();
	RemoveOwnedInputContexts();
	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	// Defer the rebuild: a transition may originate inside an action dispatch.
	Options.bForceImmediately = false;
	Subsystem->AddMappingContext(InputConfiguration->SharedContext, 100, Options);
	if (Desired)
	{
		Subsystem->AddMappingContext(Desired, 0, Options);
		ActiveGameplayContext = Desired;
	}
}

bool ARomanEmpirePlayerController::HasActiveModeContext() const
{
	const UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	return !bControllerEndingPlay && IsLocalController() && PublicGameState && PublicGameState->IsGameplayInitialized() && InputConfiguration &&
		ActiveGameplayContext && ActiveGameplayContext == InputConfiguration->GetContextForMode(PublicGameState->GetCurrentGameplayMode()) &&
		Subsystem && Subsystem->HasMappingContext(ActiveGameplayContext) &&
		Subsystem->HasMappingContext(InputConfiguration->SharedContext);
}

bool ARomanEmpirePlayerController::CanIssueTacticalCommands() const
{
	return HasActiveModeContext() && PublicGameState->GetCurrentGameplayMode() == EGameplayMode::Tactical &&
		PublicGameState && PublicGameState->IsCampaignActive();
}

bool ARomanEmpirePlayerController::CanEndTurn() const
{
	return HasActiveModeContext() && PublicGameState->GetCurrentGameplayMode() != EGameplayMode::Possessed &&
		PublicGameState && PublicGameState->IsCampaignActive();
}

bool ARomanEmpirePlayerController::CanControlCamera() const
{
	return HasActiveModeContext() && PublicGameState->GetCurrentGameplayMode() != EGameplayMode::Possessed &&
		Cast<ASeamlessZoomCamera>(GetPawn()) != nullptr;
}

bool ARomanEmpirePlayerController::CanControlSoldier() const
{
	return HasActiveModeContext() && IsInFirstPersonMode() && IsPlayerUnitEligible(PossessedUnit) &&
		GetPawn() == PossessedUnit && PublicGameState->IsCampaignActive();
}

bool ARomanEmpirePlayerController::IsPlayerUnitEligible(const AUnitBase* Unit) const
{
	return !bControllerEndingPlay && IsValid(PublicGameState) && PublicGameState->IsGameplayInitialized() &&
		PublicGameState->GetPlayerFaction() != EFactionID::None && IsValid(Unit) &&
		Unit->GetWorld() == GetWorld() && Unit->CanAcceptCommands() &&
		Unit->GetOwnerFaction() == PublicGameState->GetPlayerFaction();
}

bool ARomanEmpirePlayerController::IsPlayerBuildingEligible(const ABuildingBase* Building) const
{
	return !bControllerEndingPlay && IsValid(PublicGameState) && PublicGameState->IsGameplayInitialized() &&
		PublicGameState->GetPlayerFaction() != EFactionID::None && IsValid(Building) &&
		Building->GetWorld() == GetWorld() && Building->IsSelectionReady() &&
		Building->GetOwnerFaction() == PublicGameState->GetPlayerFaction();
}

void ARomanEmpirePlayerController::HandlePlayerFactionChanged(EFactionID NewFaction)
{
	(void)NewFaction;
	ReconcileUnitOwnership();
	// Unit callbacks may have replaced the selection: re-query the current building.
	if (SelectedBuilding && !IsPlayerBuildingEligible(SelectedBuilding)) ClearSelection();
}

void ARomanEmpirePlayerController::HandleSelectedUnitOwnerChanged(AUnitBase* Unit)
{
	(void)Unit;
	ReconcileUnitOwnership();
}

void ARomanEmpirePlayerController::HandlePossessedUnitOwnerChanged(AUnitBase* Unit)
{
	(void)Unit;
	ReconcileUnitOwnership();
}

void ARomanEmpirePlayerController::ReconcileUnitOwnership()
{
	// Cleanup is legal in every mode. Re-query after callbacks; event payloads may be stale.
	if (PossessedUnit && !IsPlayerUnitEligible(PossessedUnit)) ExitFirstPersonMode();
	if (SelectedUnits.ContainsByPredicate([this](const AUnitBase* Unit) { return !IsPlayerUnitEligible(Unit); }))
	{
		const TArray<AUnitBase*> PreviousSelection = SelectedUnits;
		SelectUnits(PreviousSelection);
	}
}

void ARomanEmpirePlayerController::HandleGameplayInitialized()
{
	ApplyInputContexts();
}

void ARomanEmpirePlayerController::HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode)
{
	// An external mode change must also restore the actual pawn, not just the input context.
	if (NewMode != EGameplayMode::Possessed && PossessedUnit)
	{
		ExitFirstPersonMode();
	}
	ApplyInputContexts();
}

void ARomanEmpirePlayerController::CancelTransientInput()
{
	bIsBoxSelecting = false;
	bAdditiveSelectionGesture = false;
	OnBlockReleased();
	CancelBuildingPlacement();
	if (MainWidget)
	{
		MainWidget->HideBuildingMenu();
	}
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->ConsumeMovementInputVector();
		if (UPawnMovementComponent* Movement = ControlledPawn->GetMovementComponent())
		{
			Movement->StopMovementImmediately();
		}
	}
}

void ARomanEmpirePlayerController::FlushPressedKeys()
{
	MovementKeysAwaitingRelease.Reset();
	CancelTransientInput();
	Super::FlushPressedKeys();
}

void ARomanEmpirePlayerController::CaptureHeldMovementKeys(UInputMappingContext* DestinationContext)
{
	if (!DestinationContext || !InputConfiguration) return;
	const UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	const UEnhancedInputUserSettings* Settings = Subsystem ? Subsystem->GetUserSettings() : nullptr;
	const UEnhancedPlayerMappableKeyProfile* Profile = Settings ? Settings->GetActiveKeyProfile() : nullptr;
	const FString ProfileID = Profile ? Profile->GetProfileIdString() : FString();
	// UE's rebuild ignore-held option only covers Boolean actions. Movement is Axis2D.
	// Inspect authored/profile mappings so this policy also follows future key remaps.
	for (const FEnhancedActionKeyMapping& Mapping : DestinationContext->GetMappingsForProfile(ProfileID))
	{
		if ((Mapping.Action == InputConfiguration->CameraPan || Mapping.Action == InputConfiguration->SoldierMove) &&
			Mapping.Key.IsDigital() && IsInputKeyDown(Mapping.Key))
		{
			MovementKeysAwaitingRelease.Add(Mapping.Key);
		}
	}
}

bool ARomanEmpirePlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (Params.Event == IE_Released)
	{
		MovementKeysAwaitingRelease.Remove(Params.Key);
	}
	return Super::InputKey(Params);
}

void ARomanEmpirePlayerController::OnSelectCanceled()
{
	bIsBoxSelecting = false;
	bAdditiveSelectionGesture = false;
}

void ARomanEmpirePlayerController::OnCancelPressed()
{
	if (PossessedUnit)
	{
		ExitFirstPersonMode();
	}
	else if (BuildingPlacementComponent && BuildingPlacementComponent->IsPlacing())
	{
		CancelBuildingPlacement();
	}
	else if (MainWidget && MainWidget->IsBuildingMenuVisible())
	{
		MainWidget->HideBuildingMenu();
	}
	else
	{
		bIsBoxSelecting = false;
		ClearSelection();
	}
}

void ARomanEmpirePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// Edge scrolling in RTS mode
	if (CanControlCamera())
	{
		HandleEdgeScroll(DeltaSeconds);
	}

	// Update building placement preview if active
	if (CurrentSelectionMode == ERomanSelectionMode::BuildingPlacement && BuildingPlacementComponent)
	{
		if (!CanIssueTacticalCommands())
		{
			CancelBuildingPlacement();
			return;
		}
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
	const UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!CanControlCamera() || !ViewportClient || !ViewportClient->Viewport || !ViewportClient->Viewport->HasFocus())
	{
		return;
	}
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
	SelectUnits({ Unit });
}

void ARomanEmpirePlayerController::SelectUnits(const TArray<AUnitBase*>& Units)
{
	const TArray<AUnitBase*> RequestedUnits = Units;
	ClearSelectedActors();
	for (AUnitBase* Unit : RequestedUnits)
	{
		if (IsPlayerUnitEligible(Unit) && !SelectedUnits.Contains(Unit))
		{
			SelectedUnits.Add(Unit);
			Unit->SetSelected(true);
			Unit->OnUnitDied.AddUniqueDynamic(this, &ARomanEmpirePlayerController::HandleSelectedUnitDied);
			Unit->OnDestroyed.AddUniqueDynamic(this, &ARomanEmpirePlayerController::HandleSelectedActorDestroyed);
			Unit->OnOwnerFactionChanged.AddUniqueDynamic(this, &ARomanEmpirePlayerController::HandleSelectedUnitOwnerChanged);
		}
	}
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Selected %d units"), SelectedUnits.Num());
	BroadcastSelectionChanged();
}

bool ARomanEmpirePlayerController::IsSelectionModifierActive() const
{
	const UEnhancedPlayerInput* EnhancedInput = Cast<UEnhancedPlayerInput>(PlayerInput);
	if (!CanIssueTacticalCommands() || CurrentSelectionMode == ERomanSelectionMode::BuildingPlacement ||
		(MainWidget && MainWidget->IsBuildingMenuVisible()) || !InputConfiguration || !EnhancedInput ||
		!EnhancedInput->GetActionValue(InputConfiguration->SelectionModifier).Get<bool>()) return false;
	// Enhanced Input retains the evaluated action until its next processing pass after
	// FlushPressedKeys. Require a still-held resolved mapping to reject that stale value.
	// Reading the live mappings preserves remaps and supports either/both modifier keys.
	for (const FEnhancedActionKeyMapping& Mapping : EnhancedInput->GetEnhancedActionMappingsView())
	{
		if (Mapping.Action == InputConfiguration->SelectionModifier && IsInputKeyDown(Mapping.Key)) return true;
	}
	return false;
}

void ARomanEmpirePlayerController::ReportInteractionFeedback(const FText& Message)
{
	if (!bControllerEndingPlay) OnInteractionFeedback.Broadcast(Message);
}

void ARomanEmpirePlayerController::ModifyUnitSelection(const TArray<AUnitBase*>& Units, EUnitSelectionOperation Operation)
{
	if (!CanIssueTacticalCommands()) return;
	if (Operation != EUnitSelectionOperation::Replace && Operation != EUnitSelectionOperation::Add &&
		Operation != EUnitSelectionOperation::Toggle) return;
	TArray<AUnitBase*> Candidates;
	for (AUnitBase* Unit : Units) if (IsPlayerUnitEligible(Unit)) Candidates.AddUnique(Unit);
	if (Operation != EUnitSelectionOperation::Replace && Candidates.IsEmpty()) return;
	TArray<AUnitBase*> Result = Operation == EUnitSelectionOperation::Replace ? TArray<AUnitBase*>() : SelectedUnits;
	for (AUnitBase* Unit : Candidates)
	{
		if (Operation == EUnitSelectionOperation::Toggle && Result.Contains(Unit)) Result.Remove(Unit);
		else Result.AddUnique(Unit);
	}
	SelectUnits(Result); // One canonical commit path owns filtering, presentation and observers.
}

void ARomanEmpirePlayerController::SelectActorFromInput(AActor* Actor, bool bAdditive)
{
	if (!CanIssueTacticalCommands()) return;
	if (AUnitBase* Unit = Cast<AUnitBase>(Actor))
	{
		if (IsPlayerUnitEligible(Unit))
		{
			ModifyUnitSelection({ Unit }, bAdditive ? EUnitSelectionOperation::Toggle : EUnitSelectionOperation::Replace);
			return;
		}
	}
	else if (ABuildingBase* Building = Cast<ABuildingBase>(Actor))
	{
		if (IsPlayerBuildingEligible(Building)) { SelectBuilding(Building); return; }
	}
	else
	{
		if (!bAdditive) ClearSelection();
		return;
	}
	ReportInteractionFeedback(NSLOCTEXT("RomanSelection", "UnavailableActor", "Select an available unit or placed building owned by your faction."));
}

void ARomanEmpirePlayerController::ClearSelection()
{
	ClearSelectedActors();
	BroadcastSelectionChanged();
}

void ARomanEmpirePlayerController::SelectBuilding(ABuildingBase* Building)
{
	ClearSelectedActors();
	SelectedBuilding = IsPlayerBuildingEligible(Building) ? Building : nullptr;
	if (SelectedBuilding)
	{
		SelectedBuilding->SetSelected(true);
		SelectedBuilding->OnBuildingDestroyed.AddUniqueDynamic(
			this, &ARomanEmpirePlayerController::HandleSelectedBuildingDestroyed);
		SelectedBuilding->OnBuildingStateChanged.AddUniqueDynamic(
			this, &ARomanEmpirePlayerController::HandleSelectedBuildingStateChanged);
		SelectedBuilding->AActor::OnDestroyed.AddUniqueDynamic(
			this, &ARomanEmpirePlayerController::HandleSelectedActorDestroyed);
		UE_LOG(LogRomanEmpire, Verbose, TEXT("Selected building: %s"), *SelectedBuilding->GetName());
	}
	BroadcastSelectionChanged();
}

void ARomanEmpirePlayerController::ClearSelectedActors()
{
	for (AUnitBase* Unit : SelectedUnits)
	{
		if (IsValid(Unit))
		{
			Unit->SetSelected(false);
			Unit->OnUnitDied.RemoveDynamic(this, &ARomanEmpirePlayerController::HandleSelectedUnitDied);
			Unit->OnDestroyed.RemoveDynamic(this, &ARomanEmpirePlayerController::HandleSelectedActorDestroyed);
			Unit->OnOwnerFactionChanged.RemoveDynamic(this, &ARomanEmpirePlayerController::HandleSelectedUnitOwnerChanged);
		}
	}
	SelectedUnits.Empty();
	if (SelectedBuilding)
	{
		SelectedBuilding->SetSelected(false);
		SelectedBuilding->OnBuildingDestroyed.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandleSelectedBuildingDestroyed);
		SelectedBuilding->OnBuildingStateChanged.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandleSelectedBuildingStateChanged);
		SelectedBuilding->AActor::OnDestroyed.RemoveDynamic(
			this, &ARomanEmpirePlayerController::HandleSelectedActorDestroyed);
	}
	SelectedBuilding = nullptr;
}

void ARomanEmpirePlayerController::BroadcastSelectionChanged()
{
	// Do not expose a mutable container to observers which may issue nested selections.
	const TArray<AUnitBase*> Snapshot = SelectedUnits;
	OnSelectionChanged.Broadcast(Snapshot, SelectedBuilding);
}

void ARomanEmpirePlayerController::CommandSelectedUnitsMove(const FVector& Destination)
{
	if (!CanIssueTacticalCommands()) return;
	TArray<TWeakObjectPtr<AUnitBase>> Snapshot;
	TArray<FVector> Locations;
	double MaxRadius = 0;
	for (AUnitBase* Unit : SelectedUnits)
	{
		if (!IsPlayerUnitEligible(Unit)) continue;
		Snapshot.Add(Unit);
		Locations.Add(Unit->GetActorLocation());
		MaxRadius = FMath::Max(MaxRadius, static_cast<double>(Unit->GetCapsuleComponent()->GetScaledCapsuleRadius()));
	}
	TArray<FVector> Destinations;
	if (!FRomanFormationLayout::Build(Locations, Destination, FMath::Max(160.0, 2 * MaxRadius + 64), Destinations)) return;
	if (Snapshot.Num() > 1)
	{
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		for (int32 Index = 0; Index < Snapshot.Num(); ++Index)
		{
			FNavLocation Projected;
			// Tight projection preserves distinct slots; do not collapse a formation
			// onto the nearest edge or issue half a batch when the footprint cannot fit.
			if (!Nav || !Nav->ProjectPointToNavigation(Destinations[Index], Projected, FVector(16, 16, 250),
				&Snapshot[Index]->GetNavAgentPropertiesRef()))
			{
				ReportInteractionFeedback(NSLOCTEXT("RomanSelection", "FormationNoRoom", "Not enough navigable space for this formation."));
				return;
			}
			Destinations[Index] = Projected.Location;
		}
	}
	const uint64 Revision = ++TacticalCommandRevision;
	ReportInteractionFeedback(FText::GetEmpty());
	for (int32 Index = 0; Index < Snapshot.Num(); ++Index)
	{
		if (TacticalCommandRevision != Revision || !CanIssueTacticalCommands()) return;
		AUnitBase* Unit = Snapshot[Index].Get();
		if (IsPlayerUnitEligible(Unit) && SelectedUnits.Contains(Unit)) Unit->CommandMoveTo(Destinations[Index]);
	}
}

void ARomanEmpirePlayerController::CommandSelectedUnitsAttack(AUnitBase* Target)
{
	if (!CanIssueTacticalCommands()) return;
	if (!IsValid(Target) || Target->GetWorld() != GetWorld() ||
		!Target->IsAlive() || Target->GetOwnerFaction() == PublicGameState->GetPlayerFaction())
	{
		ReportInteractionFeedback(NSLOCTEXT("RomanSelection", "InvalidAttackTarget", "Choose a living enemy unit to attack."));
		return;
	}
	TArray<TWeakObjectPtr<AUnitBase>> Snapshot;
	for (AUnitBase* Unit : SelectedUnits) Snapshot.Add(Unit);
	if (Snapshot.IsEmpty()) return;
	const TWeakObjectPtr<AUnitBase> TargetHandle(Target);
	const uint64 Revision = ++TacticalCommandRevision;
	ReportInteractionFeedback(FText::GetEmpty());
	for (const TWeakObjectPtr<AUnitBase>& Member : Snapshot)
	{
		if (TacticalCommandRevision != Revision || !CanIssueTacticalCommands() || !TargetHandle.IsValid()) return;
		AUnitBase* Unit = Member.Get();
		if (IsPlayerUnitEligible(Unit) && SelectedUnits.Contains(Unit)) Unit->CommandAttack(TargetHandle.Get());
	}
}

void ARomanEmpirePlayerController::CommandSelectedUnitsStop()
{
	DispatchSelectedUnitCancellation(false);
}

void ARomanEmpirePlayerController::CommandSelectedUnitsHold()
{
	DispatchSelectedUnitCancellation(true);
}

void ARomanEmpirePlayerController::DispatchSelectedUnitCancellation(bool bHold)
{
	if (!CanIssueTacticalCommands() || SelectedUnits.IsEmpty()) return;
	TArray<TWeakObjectPtr<AUnitBase>> Snapshot;
	for (AUnitBase* Unit : SelectedUnits) Snapshot.Add(Unit);
	const uint64 Revision = ++TacticalCommandRevision;
	ReportInteractionFeedback(FText::GetEmpty());
	for (const TWeakObjectPtr<AUnitBase>& Member : Snapshot)
	{
		if (TacticalCommandRevision != Revision || !CanIssueTacticalCommands()) return;
		AUnitBase* Unit = Member.Get();
		if (!IsPlayerUnitEligible(Unit) || !SelectedUnits.Contains(Unit)) continue;
		if (bHold) Unit->CommandHold();
		else Unit->CommandStop();
	}
}

void ARomanEmpirePlayerController::HandleSelectedUnitDied(AUnitBase* Unit)
{
	HandleSelectedActorDestroyed(Unit);
}

void ARomanEmpirePlayerController::HandleSelectedBuildingDestroyed(ABuildingBase* Building)
{
	HandleSelectedActorDestroyed(Building);
}

void ARomanEmpirePlayerController::HandleSelectedBuildingStateChanged(ABuildingBase* Building)
{
	// State notifications include ownership and EndPlay; cleanup is legal in any mode.
	(void)Building;
	if (SelectedBuilding && !IsPlayerBuildingEligible(SelectedBuilding)) ClearSelection();
}

void ARomanEmpirePlayerController::HandleSelectedActorDestroyed(AActor* Actor)
{
	if (Actor == SelectedBuilding)
	{
		ClearSelection();
	}
	else if (AUnitBase* Unit = Cast<AUnitBase>(Actor); SelectedUnits.Contains(Unit))
	{
		TArray<AUnitBase*> RemainingUnits = SelectedUnits;
		RemainingUnits.Remove(Unit);
		SelectUnits(RemainingUnits);
	}
}

void ARomanEmpirePlayerController::HandlePossessedUnitDied(AUnitBase* Unit)
{
	if (Unit == PossessedUnit)
	{
		ExitFirstPersonMode();
	}
}

void ARomanEmpirePlayerController::HandlePossessedActorDestroyed(AActor* Actor)
{
	if (Actor == PossessedUnit)
	{
		ExitFirstPersonMode();
	}
}

void ARomanEmpirePlayerController::StartBuildingPlacement(FName BuildingID)
{
	if (CanIssueTacticalCommands() && BuildingPlacementComponent && RomanEmpireContent::IsStableID(BuildingID))
	{
		bIsBoxSelecting = false;
		BuildingPlacementComponent->StartPlacement(BuildingID);
		CurrentSelectionMode = BuildingPlacementComponent->IsPlacing()
			? ERomanSelectionMode::BuildingPlacement : ERomanSelectionMode::None;
		UE_LOG(LogRomanEmpire, Log, TEXT("Started building placement for %s"), *BuildingID.ToString());
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
	if (bExitingPossession || PossessedUnit || !CanIssueTacticalCommands() || !IsPlayerUnitEligible(UnitToPossess))
	{
		UE_LOG(LogRomanEmpire, Verbose, TEXT("Possession request requires a living player unit in tactical mode"));
		return;
	}

	// Store the camera pawn so we can restore it later
	APawn* CurrentPawn = GetPawn();
	if (ASeamlessZoomCamera* CamPawn = Cast<ASeamlessZoomCamera>(CurrentPawn))
	{
		CachedCameraPawn = CamPawn;
	}
	if (!IsValid(CachedCameraPawn))
	{
		return;
	}
	CancelTransientInput();

	AUnitBase* PreviousUnit = PossessedUnit;
	PossessedUnit = UnitToPossess;
	bShowMouseCursor = false;

	// Observe invalidation before camera activation, which broadcasts synchronously.
	UnitToPossess->OnUnitDied.AddUniqueDynamic(this, &ARomanEmpirePlayerController::HandlePossessedUnitDied);
	UnitToPossess->OnDestroyed.AddUniqueDynamic(this, &ARomanEmpirePlayerController::HandlePossessedActorDestroyed);
	UnitToPossess->OnOwnerFactionChanged.AddUniqueDynamic(this, &ARomanEmpirePlayerController::HandlePossessedUnitOwnerChanged);
	auto IsEntryCurrent = [this, UnitToPossess]()
	{
		return PossessedUnit == UnitToPossess && IsPlayerUnitEligible(UnitToPossess) && UnitToPossess->IsPossessedByPlayer();
	};
	auto CancelEntry = [this, UnitToPossess]()
	{
		if (PossessedUnit == UnitToPossess) ExitFirstPersonMode();
	};
	UnitToPossess->SetPossessedByPlayer(true);
	if (!IsEntryCurrent()) { CancelEntry(); return; }

	// Actually possess the unit pawn so the camera follows it
	Possess(UnitToPossess);
	if (!IsEntryCurrent() || GetPawn() != UnitToPossess) { CancelEntry(); return; }

	SetInputMode(FInputModeGameOnly());
	if (!IsEntryCurrent()) { CancelEntry(); return; }

	if (GameMode)
	{
		GameMode->SetGameplayMode(EGameplayMode::Possessed);
	}
	if (!IsEntryCurrent() || !IsInFirstPersonMode()) { CancelEntry(); return; }
	if (APlayerCameraManager* CameraManager = PlayerCameraManager)
	{
		PreviousViewPitchMin = CameraManager->ViewPitchMin;
		PreviousViewPitchMax = CameraManager->ViewPitchMax;
		PossessionCameraManager = CameraManager;
		CameraManager->ViewPitchMin = -80.0f;
		CameraManager->ViewPitchMax = 80.0f;
	}
	UE_LOG(LogRomanEmpire, Log, TEXT("Entered FPS mode with unit: %s"), *UnitToPossess->GetName());
	OnPossessedUnitChanged.Broadcast(PreviousUnit, PossessedUnit);
}

void ARomanEmpirePlayerController::ExitFirstPersonMode()
{
	if (bExitingPossession || !PossessedUnit)
	{
		return;
	}

	AUnitBase* PreviousUnit = PossessedUnit;
	{
		TGuardValue<bool> ExitGuard(bExitingPossession, true);
		if (APlayerCameraManager* CameraManager = PossessionCameraManager.Get())
		{
			CameraManager->ViewPitchMin = PreviousViewPitchMin;
			CameraManager->ViewPitchMax = PreviousViewPitchMax;
		}
		PossessionCameraManager.Reset();
		CancelTransientInput();
		// Commit absence before deactivation/Possess callbacks; recursive exits are no-ops.
		PossessedUnit = nullptr;
		if (IsValid(PreviousUnit))
		{
			PreviousUnit->OnUnitDied.RemoveDynamic(this, &ARomanEmpirePlayerController::HandlePossessedUnitDied);
			PreviousUnit->OnDestroyed.RemoveDynamic(this, &ARomanEmpirePlayerController::HandlePossessedActorDestroyed);
			PreviousUnit->OnOwnerFactionChanged.RemoveDynamic(this, &ARomanEmpirePlayerController::HandlePossessedUnitOwnerChanged);
			PreviousUnit->SetPossessedByPlayer(false);
		}

		// Re-possess the camera pawn
		if (IsValid(CachedCameraPawn))
		{
			Possess(CachedCameraPawn);
		}
		if (IsValid(PreviousUnit) && PreviousUnit->IsAlive())
		{
			PreviousUnit->RestoreTacticalController();
		}

		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		if (GameMode && PublicGameState && PublicGameState->GetCurrentGameplayMode() == EGameplayMode::Possessed)
		{
			const EZoomLevel ZoomLevel = CachedCameraPawn
				? CachedCameraPawn->GetZoomLevelEnum()
				: EZoomLevel::City;
			GameMode->SetGameplayMode(RomanEmpireGameplay::GetExplorationModeForZoomLevel(ZoomLevel));
		}
	}
	UE_LOG(LogRomanEmpire, Log, TEXT("Exited FPS mode — camera restored"));
	OnPossessedUnitChanged.Broadcast(PreviousUnit, nullptr);
}


void ARomanEmpirePlayerController::SetTargetZoom(float NewZoom)
{
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	if (ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(GetPawn()))
	{
		CameraPawn->SetTargetZoomLevel(
			FMath::Clamp(NewZoom, Zoom.Minimum, Zoom.UserControlledMaximum));
	}
}

float ARomanEmpirePlayerController::GetCurrentZoom() const
{
	const ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(GetPawn());
	return CameraPawn
		? CameraPawn->GetZoomLevel()
		: RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical;
}

bool ARomanEmpirePlayerController::IsInFirstPersonMode() const
{
	return PublicGameState
		? PublicGameState->GetCurrentGameplayMode() == EGameplayMode::Possessed
		: PossessedUnit != nullptr;
}

void ARomanEmpirePlayerController::OnSelectPressed()
{
	if (!CanIssueTacticalCommands() || (MainWidget && MainWidget->IsBuildingMenuVisible()))
	{
		return;
	}

	if (CurrentSelectionMode == ERomanSelectionMode::BuildingPlacement)
	{
		if (BuildingPlacementComponent && BuildingPlacementComponent->CanPlace())
		{
			if (BuildingPlacementComponent->ConfirmPlacement())
			{
				CurrentSelectionMode = ERomanSelectionMode::None;
			}
		}
		return;
	}
	
	bIsBoxSelecting = GetMousePosition(BoxSelectStart.X, BoxSelectStart.Y);
	bAdditiveSelectionGesture = bIsBoxSelecting && IsSelectionModifierActive();
}

void ARomanEmpirePlayerController::OnSelectReleased()
{
	const bool bShouldSelect = bIsBoxSelecting && CanIssueTacticalCommands() &&
		(!MainWidget || !MainWidget->IsBuildingMenuVisible());
	const bool bAdditive = bAdditiveSelectionGesture;
	bIsBoxSelecting = false;
	bAdditiveSelectionGesture = false;
	if (bShouldSelect)
	{
		PerformBoxSelect(bAdditive);
	}
}

void ARomanEmpirePlayerController::OnCommandPressed()
{
	if (!CanIssueTacticalCommands() || !PublicGameState || (MainWidget && MainWidget->IsBuildingMenuVisible()))
	{
		return;
	}
	const EFactionID PlayerFaction = PublicGameState->GetPlayerFaction();
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

				if (HitUnit && HitUnit->GetOwnerFaction() != PlayerFaction && HitUnit->IsAlive())
				{
					// Attack enemy!
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
							FString::Printf(TEXT("Attacking: %s"), *HitUnit->GetName()));
					}
					CommandSelectedUnitsAttack(HitUnit);
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
					CommandSelectedUnitsMove(Dest);
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

						CommandSelectedUnitsMove(Dest);
					}
				}
			}
		}
	}
}

void ARomanEmpirePlayerController::OnZoomInput(const FInputActionValue& Value)
{
	if (!CanControlCamera()) return;
	const ASeamlessZoomCamera* CameraPawn = Cast<ASeamlessZoomCamera>(GetPawn());
	if (!CameraPawn)
	{
		return;
	}

	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	const float ScrollVal = Value.Get<float>();
	if (ScrollVal > 0.0f)
	{
		SetTargetZoom(CameraPawn->GetTargetZoomLevel() + Zoom.InputStep);
	}
	else if (ScrollVal < 0.0f)
	{
		SetTargetZoom(CameraPawn->GetTargetZoomLevel() - Zoom.InputStep);
	}
}

void ARomanEmpirePlayerController::OnCameraPanInput(const FInputActionValue& Value)
{
	if (CanControlCamera() && MovementKeysAwaitingRelease.IsEmpty())
	{
		CastChecked<ASeamlessZoomCamera>(GetPawn())->PanCamera(Value.Get<FVector2D>());
	}
}

void ARomanEmpirePlayerController::OnSoldierMoveInput(const FInputActionValue& Value)
{
	if (CanControlSoldier() && MovementKeysAwaitingRelease.IsEmpty())
	{
		PossessedUnit->MoveInput(Value.Get<FVector2D>());
	}
}

void ARomanEmpirePlayerController::OnLookInput(const FInputActionValue& Value)
{
	if (CanControlSoldier())
	{
		FVector2D LookValue = Value.Get<FVector2D>();
		PossessedUnit->LookInput(LookValue);
	}
}

void ARomanEmpirePlayerController::OnEnterFPSPressed()
{
	if (CanIssueTacticalCommands() && !SelectedUnits.IsEmpty())
	{
		EnterFirstPersonMode(SelectedUnits[0]);
	}
}

void ARomanEmpirePlayerController::OnBuildMenuPressed()
{
	if (CanIssueTacticalCommands() && MainWidget)
	{
		bIsBoxSelecting = false;
		bAdditiveSelectionGesture = false;
		CancelBuildingPlacement();
		MainWidget->ToggleBuildingMenu();
	}
}

void ARomanEmpirePlayerController::OnBuildingSlotPressed(int32 QuickSlot)
{
	StartBuildingPlacementByQuickSlot(QuickSlot);
}

void ARomanEmpirePlayerController::OnBuildingKey1Pressed()
{
	StartBuildingPlacementByQuickSlot(1);
}

void ARomanEmpirePlayerController::OnBuildingKey2Pressed()
{
	StartBuildingPlacementByQuickSlot(2);
}

void ARomanEmpirePlayerController::OnBuildingKey3Pressed()
{
	StartBuildingPlacementByQuickSlot(3);
}

void ARomanEmpirePlayerController::OnBuildingKey4Pressed()
{
	StartBuildingPlacementByQuickSlot(4);
}

void ARomanEmpirePlayerController::OnBuildingKey5Pressed()
{
	StartBuildingPlacementByQuickSlot(5);
}

void ARomanEmpirePlayerController::OnBuildingKey6Pressed()
{
	StartBuildingPlacementByQuickSlot(6);
}

bool ARomanEmpirePlayerController::StartBuildingPlacementByQuickSlot(int32 QuickSlot)
{
	if (!CanIssueTacticalCommands() || !MainWidget || !MainWidget->IsBuildingMenuVisible() ||
		QuickSlot < 1 || QuickSlot > URomanInputConfig::BuildingSlotCount)
	{
		return false;
	}
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanBuildingDefinition* Asset = Registry ? Registry->FindBuildingByQuickSlot(QuickSlot) : nullptr;
	if (!Asset)
	{
		return false;
	}
	StartBuildingPlacement(Asset->Definition.BuildingID);
	const bool bStarted = BuildingPlacementComponent && BuildingPlacementComponent->IsPlacing() &&
		BuildingPlacementComponent->GetCurrentBuildingID() == Asset->Definition.BuildingID;
	if (bStarted)
	{
		MainWidget->HideBuildingMenu();
	}
	return bStarted;
}

void ARomanEmpirePlayerController::OnAttackPressed()
{
	if (CanControlSoldier())
	{
		PossessedUnit->PerformAttack();
	}
}

void ARomanEmpirePlayerController::OnBlockPressed()
{
	if (CanControlSoldier())
	{
		PossessedUnit->StartBlocking();
	}
}

void ARomanEmpirePlayerController::OnBlockReleased()
{
	// Cleanup must work after permissions or the canonical mode have already changed.
	if (IsValid(PossessedUnit))
	{
		PossessedUnit->StopBlocking();
	}
}

void ARomanEmpirePlayerController::OnEndTurnPressed()
{
	RequestEndTurn();
}

bool ARomanEmpirePlayerController::RequestEndTurn()
{
	if (!CanEndTurn())
	{
		return false;
	}
	const FCampaignTurnResult Result = GameMode->EndTurn();
	return Result.WasSuccessful();
}

void ARomanEmpirePlayerController::HandleCameraZoomLevelChanged(
	EZoomLevel OldLevel,
	EZoomLevel NewLevel)
{
	if (GameMode)
	{
		GameMode->SetExplorationModeForZoomLevel(NewLevel);
	}

	UE_LOG(LogRomanEmpire, Verbose, TEXT("Camera zoom band changed from %d to %d"),
		static_cast<int32>(OldLevel), static_cast<int32>(NewLevel));
}

void ARomanEmpirePlayerController::SelectUnitsInScreenRect(const FVector2D& Start, const FVector2D& End, bool bAdditive)
{
	if (!CanIssueTacticalCommands() || Start.ContainsNaN() || End.ContainsNaN()) return;
	const URomanUnitRegistrySubsystem* Registry = GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>();
	if (!Registry) return;
	const TArray<AUnitBase*> Candidates = Registry->GetUnitsForFaction(PublicGameState->GetPlayerFaction());
	const FVector2D MinBox(FMath::Min(Start.X, End.X), FMath::Min(Start.Y, End.Y));
	const FVector2D MaxBox(FMath::Max(Start.X, End.X), FMath::Max(Start.Y, End.Y));
	TArray<AUnitBase*> UnitsInBox;
	for (AUnitBase* Unit : Candidates)
	{
		if (!IsPlayerUnitEligible(Unit)) continue;
		FVector2D ScreenPos = FVector2D::ZeroVector;
		if (ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenPos) &&
			ScreenPos.X >= MinBox.X && ScreenPos.X <= MaxBox.X &&
			ScreenPos.Y >= MinBox.Y && ScreenPos.Y <= MaxBox.Y)
		{
			UnitsInBox.Add(Unit);
		}
	}
	ModifyUnitSelection(UnitsInBox, bAdditive ? EUnitSelectionOperation::Add : EUnitSelectionOperation::Replace);
	UE_LOG(LogRomanEmpire, Log, TEXT("Box selected %d units"), SelectedUnits.Num());
}

void ARomanEmpirePlayerController::PerformBoxSelect(bool bAdditive)
{
	FVector2D CurrentMousePos = FVector2D::ZeroVector;
	if (!GetMousePosition(CurrentMousePos.X, CurrentMousePos.Y)) return;
	
	float Distance = FVector2D::Distance(BoxSelectStart, CurrentMousePos);
	
	if (Distance < 10.0f)
	{
		SelectActorFromInput(GetActorUnderCursor(), bAdditive);
	}
	else
	{
		SelectUnitsInScreenRect(BoxSelectStart, CurrentMousePos, bAdditive);
	}
}

AActor* ARomanEmpirePlayerController::GetActorUnderCursor() const
{
	FHitResult HitResult;
	// First try Pawn channel (units)
	if (GetHitResultUnderCursor(ECC_Pawn, true, HitResult) && HitResult.GetActor())
	{
		return HitResult.GetActor();
	}
	// Fallback: WorldDynamic channel (buildings)
	if (GetHitResultUnderCursor(ECC_WorldDynamic, true, HitResult) && HitResult.GetActor())
	{
		return HitResult.GetActor();
	}
	return nullptr;
}

bool ARomanEmpirePlayerController::OnTrainButtonClicked()
{
	ABarracks* Barracks = Cast<ABarracks>(SelectedBuilding);
	if (!IsPlayerBuildingEligible(Barracks) || !CanIssueTacticalCommands())
	{
		return false;
	}

	const TArray<FName>& TrainableUnits = Barracks->GetBuildingData().TrainableUnits;
	if (TrainableUnits.IsEmpty())
	{
		return false;
	}

	const FName UnitID = TrainableUnits[0];
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* UnitAsset = Registry ? Registry->FindUnit(UnitID) : nullptr;
	const FString UnitName = UnitAsset ? UnitAsset->Definition.DisplayName.ToString() : UnitID.ToString();
	if (Barracks->TrainUnit(UnitID))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
			FString::Printf(TEXT("%s training request accepted"), *UnitName));
		return true;
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
		FString::Printf(TEXT("%s training request rejected"), *UnitName));
	return false;
}
