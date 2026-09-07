// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireMainWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Building/Barracks.h"
#include "Building/BuildingBase.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameMode.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Faction/FactionManager.h"
#include "Kismet/GameplayStatics.h"
#include "RomanEmpireGame.h"
#include "UI/BuildingMenuWidget.h"
#include "Units/UnitBase.h"
#include "World/CampaignManager.h"
#include "World/WorldMapManager.h"

namespace
{
	UTextBlock* CreateTextBlock(
		UWidgetTree* WidgetTree,
		const FName Name,
		const int32 FontSize,
		const FLinearColor& Color)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetAutoWrapText(true);
		return Text;
	}

	UBorder* CreatePanel(
		UWidgetTree* WidgetTree,
		const FName Name,
		const FLinearColor& Color,
		const FMargin& Padding = FMargin(10.0f))
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Border->SetBrushColor(Color);
		Border->SetPadding(Padding);
		return Border;
	}

	void PlaceFixed(
		UCanvasPanel* Canvas,
		UWidget* Widget,
		const FAnchors& Anchors,
		const FVector2D& Alignment,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
	}
}

void URomanEmpireMainWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildNativeWidgetTree();
}

void URomanEmpireMainWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildNativeWidgetTree();
	BindCanonicalEvents();
	RefreshFromCanonicalState();
	UE_LOG(LogRomanEmpire, Log, TEXT("Production UMG root constructed; gameplay event bindings %s"),
		bCanonicalEventsBound ? TEXT("ready") : TEXT("awaiting initialization"));
}

void URomanEmpireMainWidget::NativeDestruct()
{
	HideBuildingMenu();
	UnbindCanonicalEvents();
	Super::NativeDestruct();
}

TSharedRef<SWidget> URomanEmpireMainWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void URomanEmpireMainWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* TopBar = CreatePanel(
		WidgetTree,
		TEXT("TopBar"),
		FLinearColor(0.025f, 0.03f, 0.055f, 0.94f),
		FMargin(14.0f, 7.0f));
	TopBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlay* TopOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TopOverlay"));
	ResourceText = CreateTextBlock(WidgetTree, TEXT("ResourceText"), 17, FLinearColor(0.95f, 0.86f, 0.55f));
	PhaseText = CreateTextBlock(WidgetTree, TEXT("PhaseText"), 17, FLinearColor(0.75f, 0.85f, 1.0f));
	UOverlaySlot* ResourceSlot = TopOverlay->AddChildToOverlay(ResourceText);
	ResourceSlot->SetHorizontalAlignment(HAlign_Left);
	ResourceSlot->SetVerticalAlignment(VAlign_Center);
	UOverlaySlot* PhaseSlot = TopOverlay->AddChildToOverlay(PhaseText);
	PhaseSlot->SetHorizontalAlignment(HAlign_Right);
	PhaseSlot->SetVerticalAlignment(VAlign_Center);
	TopBar->SetContent(TopOverlay);
	UCanvasPanelSlot* TopSlot = RootCanvas->AddChildToCanvas(TopBar);
	TopSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
	TopSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 42.0f));

	UBorder* ObjectivePanel = CreatePanel(
		WidgetTree,
		TEXT("ObjectivePanel"),
		FLinearColor(0.04f, 0.045f, 0.075f, 0.90f));
	ObjectivePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	ObjectiveText = CreateTextBlock(WidgetTree, TEXT("ObjectiveText"), 15, FLinearColor(0.88f, 0.9f, 0.95f));
	ObjectivePanel->SetContent(ObjectiveText);
	PlaceFixed(RootCanvas, ObjectivePanel, FAnchors(0.0f, 0.0f), FVector2D::ZeroVector,
		FVector2D(10.0f, 52.0f), FVector2D(410.0f, 88.0f));

	DiagnosticPanel = CreatePanel(
		WidgetTree,
		TEXT("DiagnosticPanel"),
		FLinearColor(0.02f, 0.08f, 0.07f, 0.90f));
	DiagnosticPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
#if UE_BUILD_SHIPPING
	DiagnosticPanel->SetVisibility(ESlateVisibility::Collapsed);
#endif
	DiagnosticText = CreateTextBlock(WidgetTree, TEXT("DiagnosticText"), 13, FLinearColor(0.55f, 1.0f, 0.75f));
	DiagnosticPanel->SetContent(DiagnosticText);
	PlaceFixed(RootCanvas, DiagnosticPanel, FAnchors(1.0f, 0.0f), FVector2D(1.0f, 0.0f),
		FVector2D(-10.0f, 52.0f), FVector2D(500.0f, 104.0f));

	BuildMenuPanel = CreatePanel(
		WidgetTree,
		TEXT("BuildMenuPanel"),
		FLinearColor(0.08f, 0.035f, 0.015f, 0.95f));
	BuildingMenu = WidgetTree->ConstructWidget<UBuildingMenuWidget>(
		UBuildingMenuWidget::StaticClass(), TEXT("BuildingMenu"));
	BuildingMenu->OnBuildingSelected.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleBuildingSelected);
	BuildMenuPanel->SetContent(BuildingMenu);
	BuildMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	PlaceFixed(RootCanvas, BuildMenuPanel, FAnchors(0.0f, 0.5f), FVector2D(0.0f, 0.5f),
		FVector2D(10.0f, 0.0f), FVector2D(390.0f, 300.0f));

	SelectionPanel = CreatePanel(
		WidgetTree,
		TEXT("SelectionPanel"),
		FLinearColor(0.035f, 0.04f, 0.075f, 0.95f));
	SelectionPanel->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBox* SelectionBox = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SelectionBox"));
	SelectionText = CreateTextBlock(WidgetTree, TEXT("SelectionText"), 17, FLinearColor::White);
	ProductionText = CreateTextBlock(WidgetTree, TEXT("ProductionText"), 14, FLinearColor(0.7f, 0.95f, 0.7f));
	TrainButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("TrainButton"));
	TrainButtonText = CreateTextBlock(WidgetTree, TEXT("TrainButtonText"), 15, FLinearColor(0.25f, 1.0f, 0.35f));
	TrainButton->AddChild(TrainButtonText);
	TrainButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleTrainClicked);
	TrainButton->SetVisibility(ESlateVisibility::Collapsed);
	SelectionBox->AddChildToVerticalBox(SelectionText);
	SelectionBox->AddChildToVerticalBox(ProductionText);
	SelectionBox->AddChildToVerticalBox(TrainButton);
	SelectionPanel->SetContent(SelectionBox);
	PlaceFixed(RootCanvas, SelectionPanel, FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f),
		FVector2D(0.0f, -12.0f), FVector2D(650.0f, 140.0f));

	PossessionText = CreateTextBlock(WidgetTree, TEXT("PossessionText"), 19, FLinearColor::White);
	PossessionText->SetJustification(ETextJustify::Center);
	PossessionText->SetVisibility(ESlateVisibility::Collapsed);
	PlaceFixed(RootCanvas, PossessionText, FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f),
		FVector2D(0.0f, -25.0f), FVector2D(600.0f, 60.0f));
	CrosshairText = CreateTextBlock(WidgetTree, TEXT("CrosshairText"), 30, FLinearColor::White);
	CrosshairText->SetText(FText::FromString(TEXT("+")));
	CrosshairText->SetJustification(ETextJustify::Center);
	CrosshairText->SetVisibility(ESlateVisibility::Collapsed);
	PlaceFixed(RootCanvas, CrosshairText, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(38.0f, 44.0f));

	UBorder* ControlsPanel = CreatePanel(WidgetTree, TEXT("ControlsPanel"),
		FLinearColor(0.025f, 0.03f, 0.055f, 0.85f));
	ControlsPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	ControlsText = CreateTextBlock(WidgetTree, TEXT("ControlsText"), 14, FLinearColor(0.86f, 0.87f, 0.9f));
	ControlsPanel->SetContent(ControlsText);
	PlaceFixed(RootCanvas, ControlsPanel, FAnchors(0.0f, 1.0f), FVector2D(0.0f, 1.0f),
		FVector2D(10.0f, -12.0f), FVector2D(360.0f, 180.0f));

	UVerticalBox* CommandsBox = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("CommandsBox"));
	BuildMenuButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("BuildMenuButton"));
	UTextBlock* BuildLabel = CreateTextBlock(WidgetTree, TEXT("BuildMenuButtonText"), 18, FLinearColor::White);
	BuildLabel->SetText(FText::FromString(TEXT("BUILD MENU [B]")));
	BuildMenuButton->AddChild(BuildLabel);
	BuildMenuButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleBuildMenuClicked);
	EndTurnButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("EndTurnButton"));
	UTextBlock* EndTurnLabel = CreateTextBlock(WidgetTree, TEXT("EndTurnButtonText"), 18, FLinearColor::White);
	EndTurnLabel->SetText(FText::FromString(TEXT("END TURN [T]")));
	EndTurnButton->AddChild(EndTurnLabel);
	EndTurnButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleEndTurnClicked);
	CommandsBox->AddChildToVerticalBox(BuildMenuButton);
	CommandsBox->AddChildToVerticalBox(EndTurnButton);
	PlaceFixed(RootCanvas, CommandsBox, FAnchors(1.0f, 1.0f), FVector2D(1.0f, 1.0f),
		FVector2D(-10.0f, -12.0f), FVector2D(270.0f, 86.0f));

	OutcomePanel = CreatePanel(
		WidgetTree,
		TEXT("OutcomePanel"),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	OutcomePanel->SetVisibility(ESlateVisibility::Collapsed);
	OutcomeText = CreateTextBlock(WidgetTree, TEXT("OutcomeText"), 40, FLinearColor(1.0f, 0.85f, 0.15f));
	OutcomeText->SetJustification(ETextJustify::Center);
	OutcomePanel->SetContent(OutcomeText);
	UCanvasPanelSlot* OutcomeSlot = RootCanvas->AddChildToCanvas(OutcomePanel);
	OutcomeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	OutcomeSlot->SetOffsets(FMargin(0.0f));
}

void URomanEmpireMainWidget::BindCanonicalEvents()
{
	if (bCanonicalEventsBound)
	{
		return;
	}

	CachedController = GetOwningPlayer<ARomanEmpirePlayerController>();
	CachedGameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	if (CachedGameMode)
	{
		CachedGameMode->OnGameplayInitialized.AddUniqueDynamic(
			this, &URomanEmpireMainWidget::HandleGameplayInitialized);
	}
	if (!CachedGameMode || !CachedGameMode->IsGameplayInitialized())
	{
		return;
	}
	CachedFactionManager = CachedGameMode ? CachedGameMode->GetFactionManager() : nullptr;
	CachedWorldMapManager = CachedGameMode ? CachedGameMode->GetWorldMapManager() : nullptr;
	CachedCampaignManager = CachedGameMode ? CachedGameMode->GetCampaignManager() : nullptr;

	if (!CachedController || !CachedGameMode || !CachedFactionManager ||
		!CachedWorldMapManager || !CachedCampaignManager)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("UMG root could not resolve every canonical gameplay authority"));
		return;
	}

	CachedController->OnSelectionChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleSelectionChanged);
	CachedController->OnPossessedUnitChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandlePossessedUnitChanged);
	CachedFactionManager->OnFactionResourcesChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleResourcesChanged);
	CachedFactionManager->OnPlayerFactionChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandlePlayerFactionChanged);
	CachedGameMode->OnGameplayModeChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleGameplayModeChanged);
	CachedCampaignManager->OnTurnStepStarted.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTurnStepStarted);
	CachedCampaignManager->OnTurnProcessed.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTurnProcessed);
	CachedCampaignManager->OnCampaignStarted.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleCampaignStarted);
	CachedCampaignManager->OnCampaignObjectivesChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleObjectivesChanged);
	CachedCampaignManager->OnVictory.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleVictory);
	CachedCampaignManager->OnDefeat.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleDefeat);
	bCanonicalEventsBound = true;

	BindSelectionEvents(CachedController->GetSelectedUnits(), CachedController->GetSelectedBuilding());
}

void URomanEmpireMainWidget::UnbindCanonicalEvents()
{
	UnbindSelectionEvents();
	if (CachedController)
	{
		CachedController->OnSelectionChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleSelectionChanged);
		CachedController->OnPossessedUnitChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandlePossessedUnitChanged);
	}
	if (CachedFactionManager)
	{
		CachedFactionManager->OnFactionResourcesChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleResourcesChanged);
		CachedFactionManager->OnPlayerFactionChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandlePlayerFactionChanged);
	}
	if (CachedGameMode)
	{
		CachedGameMode->OnGameplayInitialized.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleGameplayInitialized);
		CachedGameMode->OnGameplayModeChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleGameplayModeChanged);
	}
	if (CachedCampaignManager)
	{
		CachedCampaignManager->OnTurnStepStarted.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTurnStepStarted);
		CachedCampaignManager->OnTurnProcessed.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTurnProcessed);
		CachedCampaignManager->OnCampaignStarted.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleCampaignStarted);
		CachedCampaignManager->OnCampaignObjectivesChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleObjectivesChanged);
		CachedCampaignManager->OnVictory.RemoveDynamic(this, &URomanEmpireMainWidget::HandleVictory);
		CachedCampaignManager->OnDefeat.RemoveDynamic(this, &URomanEmpireMainWidget::HandleDefeat);
	}
	bCanonicalEventsBound = false;
}

void URomanEmpireMainWidget::BindSelectionEvents(
	const TArray<AUnitBase*>& Units,
	ABuildingBase* Building)
{
	UnbindSelectionEvents();
	ObservedUnit = Units.IsEmpty() ? nullptr : Units[0];
	if (CachedGameMode && CachedGameMode->GetCurrentGameplayMode() == EGameplayMode::Possessed)
	{
		ObservedUnit = CachedController ? Cast<AUnitBase>(CachedController->GetPawn()) : nullptr;
	}
	ObservedBuilding = Building;
	ObservedBarracks = Cast<ABarracks>(Building);

	if (ObservedUnit)
	{
		ObservedUnit->OnUnitVitalsChanged.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleUnitVitalsChanged);
	}
	if (ObservedBuilding)
	{
		ObservedBuilding->OnBuildingStateChanged.AddUniqueDynamic(
			this, &URomanEmpireMainWidget::HandleBuildingStateChanged);
	}
	if (ObservedBarracks)
	{
		ObservedBarracks->OnProductionChanged.AddUniqueDynamic(
			this, &URomanEmpireMainWidget::HandleProductionChanged);
	}
}

void URomanEmpireMainWidget::UnbindSelectionEvents()
{
	if (ObservedUnit)
	{
		ObservedUnit->OnUnitVitalsChanged.RemoveDynamic(this, &URomanEmpireMainWidget::HandleUnitVitalsChanged);
	}
	if (ObservedBuilding)
	{
		ObservedBuilding->OnBuildingStateChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleBuildingStateChanged);
	}
	if (ObservedBarracks)
	{
		ObservedBarracks->OnProductionChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleProductionChanged);
	}
	ObservedUnit = nullptr;
	ObservedBuilding = nullptr;
	ObservedBarracks = nullptr;
}

void URomanEmpireMainWidget::RefreshFromCanonicalState()
{
	if (CachedFactionManager)
	{
		UpdateResourceDisplay(CachedFactionManager->GetFactionResources(CachedFactionManager->GetPlayerFaction()));
	}
	RefreshModeAndTurn();
	if (CachedCampaignManager)
	{
		const FCampaignObjectiveState State = CachedCampaignManager->GetObjectiveState();
		RefreshObjectives(State);
		RefreshOutcome(State.Outcome);
	}
	RefreshSelection();
	RefreshDiagnostic();
	RefreshCommands();
}

void URomanEmpireMainWidget::UpdateResourceDisplay(const FFactionResourceAmounts& Resources)
{
	if (ResourceText)
	{
		ResourceText->SetText(FText::FromString(FString::Printf(
			TEXT("Aurei %d   Grain %d   Iron %d   Wood %d   Stone %d   Population %d"),
			Resources.Gold,
			Resources.Food,
			Resources.Iron,
			Resources.Wood,
			Resources.Stone,
			Resources.Population)));
	}
}

void URomanEmpireMainWidget::RefreshModeAndTurn()
{
	if (!PhaseText)
	{
		return;
	}
	const EGameplayMode Mode = CachedGameMode
		? CachedGameMode->GetCurrentGameplayMode()
		: EGameplayMode::Tactical;
	const int32 Turn = CachedCampaignManager ? CachedCampaignManager->GetCurrentTurn() : 0;
	PhaseText->SetText(FText::FromString(FString::Printf(
		TEXT("%s   |   Turn %d"),
		RomanEmpireGameplay::GetGameplayModeName(Mode),
		Turn)));
}

void URomanEmpireMainWidget::RefreshCommands()
{
	const EGameplayMode Mode = CachedGameMode
		? CachedGameMode->GetCurrentGameplayMode() : EGameplayMode::Tactical;
	const bool bCanBuild = CachedController && CachedController->CanIssueTacticalCommands();
	if (BuildMenuButton)
	{
		BuildMenuButton->SetIsEnabled(bCanBuild);
		BuildMenuButton->SetVisibility(Mode == EGameplayMode::Possessed
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (EndTurnButton)
	{
		EndTurnButton->SetIsEnabled(CachedController && CachedController->CanEndTurn());
		EndTurnButton->SetVisibility(Mode == EGameplayMode::Possessed
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (!bCanBuild)
	{
		HideBuildingMenu();
	}
	if (ControlsText)
	{
		const TCHAR* Help = TEXT("TACTICAL CONTROLS\nWASD / screen edge: move camera\nWheel: zoom | Click / drag: select\nRight click: order | F: possess unit\nB: menu | 1-6: build (menu open)\nT: end turn | Esc: cancel / clear");
		if (Mode == EGameplayMode::Possessed)
		{
			Help = TEXT("SOLDIER CONTROLS\nWASD: move | Mouse: look\nLeft click: attack | Hold right click: block\nF / Esc: return to camera");
		}
		else if (Mode == EGameplayMode::Strategic)
		{
			Help = TEXT("STRATEGIC CONTROLS\nWASD / screen edge: move camera\nMouse wheel: zoom into tactical view\nT: end turn | Esc: clear selection");
		}
		ControlsText->SetText(FText::FromString(Help));
	}
}

void URomanEmpireMainWidget::RefreshObjectives(const FCampaignObjectiveState& State)
{
	if (ObjectiveText)
	{
		ObjectiveText->SetText(FText::FromString(FString::Printf(
			TEXT("OBJECTIVES\nConquest: %d / %d territories   |   Economy: %d / %d aurei"),
			State.ControlledTerritories,
			State.RequiredTerritories,
			State.CurrentGold,
			State.RequiredGold)));
	}
}

void URomanEmpireMainWidget::RefreshSelection()
{
	if (TrainButton) TrainButton->SetIsEnabled(false);
	if (!SelectionPanel || !SelectionText || !ProductionText || !TrainButton)
	{
		return;
	}
	const bool bPossessed = CachedGameMode &&
		CachedGameMode->GetCurrentGameplayMode() == EGameplayMode::Possessed;
	if (CrosshairText)
	{
		CrosshairText->SetVisibility(bPossessed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (PossessionText)
	{
		PossessionText->SetVisibility(bPossessed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bPossessed)
		{
			const AUnitBase* Soldier = CachedController ? Cast<AUnitBase>(CachedController->GetPawn()) : nullptr;
			PossessionText->SetText(Soldier
				? FText::FromString(FString::Printf(TEXT("%s\nHP %d / %d   |   Stamina %.0f%%   |   Morale %d"),
					*Soldier->GetUnitDisplayName(), Soldier->GetCurrentHealth(), Soldier->GetMaxHealth(),
					Soldier->GetStaminaPercent() * 100.0f, Soldier->GetCurrentMorale()))
				: FText::GetEmpty());
		}
	}
	if (bPossessed)
	{
		SelectionPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const TArray<AUnitBase*> Units = CachedController
		? CachedController->GetSelectedUnits()
		: TArray<AUnitBase*>();
	ABuildingBase* Building = CachedController ? CachedController->GetSelectedBuilding() : nullptr;
	if (!Units.IsEmpty())
	{
		SelectionPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TrainButton->SetVisibility(ESlateVisibility::Collapsed);
		ProductionText->SetVisibility(ESlateVisibility::Collapsed);
		if (Units.Num() == 1 && Units[0])
		{
			SelectionText->SetText(FText::FromString(FString::Printf(
				TEXT("%s   |   HP %d / %d   |   Morale %d"),
				*Units[0]->GetUnitDisplayName(),
				Units[0]->GetCurrentHealth(),
				Units[0]->GetMaxHealth(),
				Units[0]->GetCurrentMorale())));
		}
		else
		{
			SelectionText->SetText(FText::FromString(FString::Printf(
				TEXT("%d units selected"), Units.Num())));
		}
		return;
	}

	if (!Building)
	{
		SelectionPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SelectionPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const FBuildingData Data = Building->GetBuildingData();
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(Building->GetOwnerFaction()) : nullptr;
	SelectionText->SetText(FText::FromString(FString::Printf(
		TEXT("%s   |   Owner %s   |   Integrity %.0f%%   |   Construction %.0f%%"),
		*Data.DisplayName.ToString(),
		Faction ? *Faction->Definition.DisplayName.ToString() : TEXT("Unowned"),
		Building->GetHealthPercent() * 100.0f,
		Building->GetConstructionProgress() * 100.0f)));

	ABarracks* Barracks = Cast<ABarracks>(Building);
	if (!Barracks)
	{
		ProductionText->SetVisibility(ESlateVisibility::Collapsed);
		TrainButton->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ProductionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	const FName ActiveUnitID = Barracks->GetCurrentlyTrainingID();
	if (Barracks->IsTraining())
	{
		const URomanUnitDefinition* ActiveUnit = Registry ? Registry->FindUnit(ActiveUnitID) : nullptr;
		ProductionText->SetText(FText::FromString(FString::Printf(
			TEXT("Production: %s active   |   %d pending"),
			ActiveUnit ? *ActiveUnit->Definition.DisplayName.ToString() : *ActiveUnitID.ToString(),
			Barracks->GetPendingOrderCount())));
	}
	else
	{
		ProductionText->SetText(FText::FromString(TEXT("Production: idle")));
	}

	const bool bCanTrain = Building->IsComplete() && CachedController &&
		CachedController->CanIssueTacticalCommands() &&
		CachedFactionManager && Building->GetOwnerFaction() == CachedFactionManager->GetPlayerFaction() &&
		!Data.TrainableUnits.IsEmpty();
	TrainButton->SetVisibility(bCanTrain ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bCanTrain && TrainButtonText)
	{
		const URomanUnitDefinition* Unit = Registry ? Registry->FindUnit(Data.TrainableUnits[0]) : nullptr;
		// Use the same atomic spending rules on a local query copy; this never mutates a wallet.
		FFactionResourceAmounts Available = CachedFactionManager->GetFactionResources(Building->GetOwnerFaction());
		const bool bAffordable = Unit &&
			RomanEmpireEconomy::TrySpendResources(Available, Unit->Definition.TrainingCost).WasSuccessful();
		TrainButton->SetIsEnabled(bAffordable);
		TrainButtonText->SetText(FText::FromString(Unit
			? FString::Printf(TEXT("TRAIN %s   |   %s"),
				*Unit->Definition.DisplayName.ToString(),
				*RomanEmpireEconomy::DescribeResourceCost(Unit->Definition.TrainingCost))
			: TEXT("TRAIN")));
	}
}

void URomanEmpireMainWidget::RefreshDiagnostic()
{
	if (!DiagnosticText)
	{
		return;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const EFactionID PlayerFaction = CachedFactionManager
		? CachedFactionManager->GetPlayerFaction()
		: EFactionID::None;
	const FFactionResourceAmounts Resources = CachedFactionManager
		? CachedFactionManager->GetFactionResources(PlayerFaction)
		: FFactionResourceAmounts();
	const int32 TerritoryCount = CachedWorldMapManager
		? CachedWorldMapManager->GetFactionTerritoryCount(PlayerFaction)
		: 0;
	const EGameplayMode Mode = CachedGameMode
		? CachedGameMode->GetCurrentGameplayMode()
		: EGameplayMode::Tactical;

	DiagnosticText->SetText(FText::FromString(FString::Printf(
		TEXT("CANONICAL STATE\nMode=%s  Turn=%d  PlayerTerritories=%d  Gold=%d\nDefinitions F/U/B/T=%d/%d/%d/%d"),
		RomanEmpireGameplay::GetGameplayModeName(Mode),
		CachedCampaignManager ? CachedCampaignManager->GetCurrentTurn() : 0,
		TerritoryCount,
		Resources.Gold,
		Registry ? Registry->GetAllFactions().Num() : 0,
		Registry ? Registry->GetAllUnits().Num() : 0,
		Registry ? Registry->GetAllBuildings().Num() : 0,
		Registry ? Registry->GetAllTerritories().Num() : 0)));
}

void URomanEmpireMainWidget::RefreshOutcome(ECampaignOutcome Outcome)
{
	if (!OutcomePanel || !OutcomeText)
	{
		return;
	}
	if (Outcome == ECampaignOutcome::InProgress)
	{
		OutcomePanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	OutcomePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Outcome == ECampaignOutcome::Victory)
	{
		OutcomeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.1f)));
		const TCHAR* Condition = CachedCampaignManager &&
			CachedCampaignManager->GetResolvedVictoryCondition() == EVictoryCondition::Economic
			? TEXT("Economic")
			: TEXT("Conquest");
		OutcomeText->SetText(FText::FromString(FString::Printf(TEXT("VICTORIA\n%s victory"), Condition)));
	}
	else if (Outcome == ECampaignOutcome::Defeat)
	{
		OutcomeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.15f, 0.12f)));
		OutcomeText->SetText(FText::FromString(TEXT("DEFEAT\nYour faction controls no territories")));
	}
	else
	{
		OutcomeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
		OutcomeText->SetText(FText::FromString(TEXT("CAMPAIGN COMPLETE\nTurn limit reached")));
	}
}

void URomanEmpireMainWidget::ShowBuildingMenu()
{
	if (!BuildMenuPanel || !IsInViewport() || !IsVisible() || !CachedController ||
		!CachedController->CanIssueTacticalCommands())
	{
		return;
	}
	BuildMenuPanel->SetVisibility(ESlateVisibility::Visible);
}

void URomanEmpireMainWidget::HideBuildingMenu()
{
	if (BuildMenuPanel)
	{
		BuildMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::ToggleBuildingMenu()
{
	if (IsBuildingMenuVisible())
	{
		HideBuildingMenu();
	}
	else
	{
		ShowBuildingMenu();
	}
}

bool URomanEmpireMainWidget::IsBuildingMenuVisible() const
{
	return IsInViewport() && IsVisible() && RootCanvas && RootCanvas->IsVisible() &&
		BuildMenuPanel && BuildMenuPanel->IsVisible() && BuildingMenu && BuildingMenu->IsVisible();
}

void URomanEmpireMainWidget::SetDiagnosticVisible(bool bVisible)
{
#if UE_BUILD_SHIPPING
	bVisible = false;
#endif
	if (DiagnosticPanel)
	{
		DiagnosticPanel->SetVisibility(
			bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::HandleSelectionChanged(
	const TArray<AUnitBase*>& Units,
	ABuildingBase* Building)
{
	BindSelectionEvents(Units, Building);
	RefreshSelection();
	RefreshDiagnostic();
}

void URomanEmpireMainWidget::HandleResourcesChanged(
	EFactionID FactionID,
	FFactionResourceAmounts NewBalance)
{
	if (CachedFactionManager && FactionID == CachedFactionManager->GetPlayerFaction())
	{
		UpdateResourceDisplay(NewBalance);
		RefreshSelection();
		RefreshDiagnostic();
	}
}

void URomanEmpireMainWidget::HandleGameplayInitialized()
{
	BindCanonicalEvents();
	RefreshFromCanonicalState();
}

void URomanEmpireMainWidget::HandlePlayerFactionChanged(EFactionID PlayerFaction)
{
	(void)PlayerFaction;
	RefreshFromCanonicalState();
}

void URomanEmpireMainWidget::HandleCampaignStarted(EFactionID PlayerFaction)
{
	(void)PlayerFaction;
	RefreshFromCanonicalState();
}

void URomanEmpireMainWidget::HandleGameplayModeChanged(EGameplayMode OldMode, EGameplayMode NewMode)
{
	(void)OldMode;
	if (NewMode != EGameplayMode::Tactical)
	{
		HideBuildingMenu();
	}
	if (CachedController)
	{
		BindSelectionEvents(CachedController->GetSelectedUnits(), CachedController->GetSelectedBuilding());
	}
	RefreshModeAndTurn();
	RefreshSelection();
	RefreshDiagnostic();
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleTurnStepStarted(int32 TurnNumber, ECampaignTurnStep Step)
{
	if (Step == ECampaignTurnStep::BroadcastTurnCompleted || Step == ECampaignTurnStep::Autosave)
	{
		RefreshModeAndTurn();
		return;
	}
	if (PhaseText)
	{
		PhaseText->SetText(FText::FromString(FString::Printf(
			TEXT("Turn %d   |   %s"), TurnNumber, RomanEmpireCampaign::GetTurnStepName(Step))));
	}
}

void URomanEmpireMainWidget::HandleTurnProcessed(int32 TurnNumber)
{
	(void)TurnNumber;
	RefreshModeAndTurn();
	RefreshDiagnostic();
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleProductionChanged(ABarracks* Producer, FProductionEvent Event)
{
	(void)Event;
	if (Producer == ObservedBarracks)
	{
		RefreshSelection();
		RefreshDiagnostic();
	}
}

void URomanEmpireMainWidget::HandlePossessedUnitChanged(AUnitBase* PreviousUnit, AUnitBase* NewUnit)
{
	(void)PreviousUnit;
	(void)NewUnit;
	if (CachedController)
	{
		BindSelectionEvents(CachedController->GetSelectedUnits(), CachedController->GetSelectedBuilding());
	}
	RefreshModeAndTurn();
	RefreshSelection();
	RefreshDiagnostic();
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleObjectivesChanged(FCampaignObjectiveState State)
{
	RefreshObjectives(State);
	RefreshOutcome(State.Outcome);
	RefreshSelection();
	RefreshDiagnostic();
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleVictory(EFactionID WinningFaction, EVictoryCondition Condition)
{
	(void)WinningFaction;
	(void)Condition;
	RefreshOutcome(ECampaignOutcome::Victory);
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleDefeat(EFactionID LosingFaction)
{
	(void)LosingFaction;
	RefreshOutcome(ECampaignOutcome::Defeat);
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleUnitVitalsChanged(AUnitBase* Unit)
{
	if (Unit == ObservedUnit)
	{
		RefreshSelection();
	}
}

void URomanEmpireMainWidget::HandleBuildingStateChanged(ABuildingBase* Building)
{
	if (Building == ObservedBuilding)
	{
		RefreshSelection();
	}
}

void URomanEmpireMainWidget::HandleBuildingSelected(FName BuildingID)
{
	if (IsBuildingMenuVisible() && CachedController && CachedController->CanIssueTacticalCommands())
	{
		CachedController->StartBuildingPlacement(BuildingID);
		HideBuildingMenu();
	}
}

void URomanEmpireMainWidget::HandleTrainClicked()
{
	if (CachedController && CachedController->CanIssueTacticalCommands() &&
		!CachedController->OnTrainButtonClicked() && ProductionText)
	{
		ProductionText->SetText(FText::FromString(TEXT("Cannot train: check resources, building state, and queue capacity")));
	}
}

void URomanEmpireMainWidget::HandleEndTurnClicked()
{
	if (CachedController && CachedController->RequestEndTurn())
	{
		RefreshFromCanonicalState();
	}
}

void URomanEmpireMainWidget::HandleBuildMenuClicked()
{
	ToggleBuildingMenu();
}
