// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireMainWidget.h"
#include "Campaign/CampaignSessionSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Building/Barracks.h"
#include "Building/BuildingBase.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Engine/World.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Input/RomanInputConfig.h"
#include "Input/RomanInputRemapping.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "RomanEmpireGame.h"
#include "UI/BuildingMenuWidget.h"
#include "UI/MinimapWidget.h"
#include "Components/SizeBox.h"
#include "UI/RomanTerritoryButton.h"
#include "Components/ScrollBox.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#include "Units/UnitBase.h"
#include "Engine/GameInstance.h"
#include "UI/RomanPlayerSettingsSubsystem.h"

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
	BindSessionEvents();
	BindPreferences();
	RefreshFromCanonicalState();
	UE_LOG(LogRomanEmpire, Log, TEXT("Production UMG root constructed; gameplay event bindings %s"),
		bCanonicalEventsBound ? TEXT("ready") : TEXT("awaiting initialization"));
}

void URomanEmpireMainWidget::NativeDestruct()
{
	if (TacticalMinimap) TacticalMinimap->SetPresentationActive(false);
	if (StrategicMinimap) StrategicMinimap->SetPresentationActive(false);
	ClearInteractionFeedback();
	HideBuildingMenu();
	UnbindCanonicalEvents();
	UnbindSessionEvents();
	UnbindPreferences();
	SelectedTerritoryID = NAME_None;
	SelectedArmyID.Invalidate();
	if (CancelTrainingButton) CancelTrainingButton->SetIsEnabled(false);
	if (RestartEncounterButton) RestartEncounterButton->SetIsEnabled(false);
	if (StrategicPanel) StrategicPanel->SetVisibility(ESlateVisibility::Collapsed);
	for (URomanTerritoryButton* Button : TerritoryButtons) Button->SetIsEnabled(false);
	Super::NativeDestruct();
}

TSharedRef<SWidget> URomanEmpireMainWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void URomanEmpireMainWidget::BindPreferences()
{
	UnbindPreferences();
	PreferencesSource=GetGameInstance()?GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>():nullptr;
	if (PreferencesSource.IsValid()) PreferencesChangedHandle=PreferencesSource->OnPreferencesChanged().AddUObject(this,&URomanEmpireMainWidget::RefreshTextScale);
	const auto* Subsystem=CachedController?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CachedController->GetLocalPlayer()):nullptr;
	BindingSettingsSource=Subsystem?Subsystem->GetUserSettings():nullptr;
	if(BindingSettingsSource.IsValid()) BindingSettingsSource->OnSettingsChanged.AddUniqueDynamic(this,&ThisClass::HandleBindingsChanged);
	RefreshTextScale();
}

void URomanEmpireMainWidget::UnbindPreferences()
{
	if (PreferencesSource.IsValid()) PreferencesSource->OnPreferencesChanged().Remove(PreferencesChangedHandle);
	if(BindingSettingsSource.IsValid()) BindingSettingsSource->OnSettingsChanged.RemoveDynamic(this,&ThisClass::HandleBindingsChanged);
	BindingSettingsSource.Reset();
	PreferencesChangedHandle.Reset(); PreferencesSource.Reset();
}

void URomanEmpireMainWidget::RefreshTextScale()
{
	if (PreferencesSource.IsValid()) TextScaleProjection.Apply(WidgetTree,PreferencesSource->GetPreferences().TextScale);
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
	// Initial construction is reparented into the flex header below; phase stays
	// single-line in its own column, resources wrap within their allocated column.
	ResourceText->SetAutoWrapText(false);
	PhaseText->SetAutoWrapText(false);
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
	BattleText = CreateTextBlock(WidgetTree, TEXT("BattleText"), 13, FLinearColor(1.0f, 0.78f, 0.45f));
	UVerticalBox* ObjectiveBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ObjectiveBox"));
	ObjectiveBox->AddChildToVerticalBox(ObjectiveText);
	ObjectiveBox->AddChildToVerticalBox(BattleText);
	ObjectivePanel->SetContent(ObjectiveBox);
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
	CancelTrainingButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("CancelTrainingButton"));
	UTextBlock* CancelLabel = CreateTextBlock(WidgetTree, TEXT("CancelTrainingLabel"), 15, FLinearColor(1.0f, 0.85f, 0.35f));
	CancelLabel->SetText(NSLOCTEXT("RomanProduction", "CancelTraining", "CANCEL + REFUND"));
	CancelTrainingButton->SetContent(CancelLabel);
	CancelTrainingButton->SetToolTipText(NSLOCTEXT("RomanProduction", "CancelRefundTooltip", "Cancel the active training order and refund its full paid cost. The next prepaid order starts automatically."));
	CancelTrainingButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCancelTrainingClicked);
	CancelTrainingButton->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBox* UnitCommandRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("UnitCommandRow"));
	StopButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("StopButton"));
	UTextBlock* StopLabel = CreateTextBlock(WidgetTree, TEXT("StopButtonText"), 16, FLinearColor(1.0f, 0.78f, 0.42f));
	StopLabel->SetText(NSLOCTEXT("RomanCommands", "Stop", "STOP"));
	StopButton->AddChild(StopLabel);
	StopButton->SetToolTipText(NSLOCTEXT("RomanCommands", "StopTooltip", "Cancel the selected units' orders. Idle units may acquire enemies again."));
	StopButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleStopClicked);
	HoldButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("HoldButton"));
	UTextBlock* HoldLabel = CreateTextBlock(WidgetTree, TEXT("HoldButtonText"), 16, FLinearColor(0.74f, 0.93f, 1.0f));
	HoldLabel->SetText(NSLOCTEXT("RomanCommands", "Hold", "HOLD"));
	HoldButton->AddChild(HoldLabel);
	HoldButton->SetToolTipText(NSLOCTEXT("RomanCommands", "HoldTooltip", "Keep selected units in place until a new order. They will not acquire enemies automatically."));
	HoldButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleHoldClicked);
	UnitCommandRow->AddChildToHorizontalBox(StopButton)->SetPadding(FMargin(0, 2, 10, 0));
	UnitCommandRow->AddChildToHorizontalBox(HoldButton)->SetPadding(FMargin(0, 2, 0, 0));
	SelectionBox->AddChildToVerticalBox(SelectionText);
	SelectionBox->AddChildToVerticalBox(UnitCommandRow);
	SelectionBox->AddChildToVerticalBox(ProductionText);
	UHorizontalBox* TrainingCommands = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TrainingCommands"));
	TrainingCommands->AddChildToHorizontalBox(TrainButton)->SetPadding(FMargin(0, 0, 10, 0));
	TrainingCommands->AddChildToHorizontalBox(CancelTrainingButton);
	SelectionBox->AddChildToVerticalBox(TrainingCommands);
	SelectionPanel->SetContent(SelectionBox);
	PlaceFixed(RootCanvas, SelectionPanel, FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f),
		FVector2D(0.0f, -12.0f), FVector2D(650.0f, 140.0f));

	InteractionFeedbackPanel = CreatePanel(WidgetTree, TEXT("InteractionFeedbackPanel"),
		FLinearColor(0.025f, 0.03f, 0.055f, 0.95f));
	InteractionFeedbackText = CreateTextBlock(WidgetTree, TEXT("InteractionFeedbackText"), 16,
		FLinearColor(1.0f, 0.85f, 0.35f));
	InteractionFeedbackText->SetJustification(ETextJustify::Center);
	InteractionFeedbackPanel->SetContent(InteractionFeedbackText);
	PlaceFixed(RootCanvas, InteractionFeedbackPanel, FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f),
		FVector2D(0.0f, -164.0f), FVector2D(650.0f, 70.0f));
	ClearInteractionFeedback();

	PossessionText = CreateTextBlock(WidgetTree, TEXT("PossessionText"), 19, FLinearColor::White);
	PossessionText->SetJustification(ETextJustify::Center);
	PossessionText->SetVisibility(ESlateVisibility::Collapsed);
	PlaceFixed(RootCanvas, PossessionText, FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f),
		FVector2D(0.0f, -25.0f), FVector2D(800.0f, 60.0f));
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
	BuildLabel->SetText(FText::FromString(TEXT("BUILD MENU")));
	BuildMenuButton->AddChild(BuildLabel);
	BuildMenuButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleBuildMenuClicked);
	EndTurnButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("EndTurnButton"));
	UTextBlock* EndTurnLabel = CreateTextBlock(WidgetTree, TEXT("EndTurnButtonText"), 18, FLinearColor::White);
	EndTurnLabel->SetAutoWrapText(true);
	EndTurnLabel->SetText(FText::FromString(TEXT("END TURN")));
	EndTurnButton->AddChild(EndTurnLabel);
	EndTurnButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleEndTurnClicked);
	CommandsBox->AddChildToVerticalBox(BuildMenuButton);
	CommandsBox->AddChildToVerticalBox(EndTurnButton);
	PlaceFixed(RootCanvas, CommandsBox, FAnchors(1.0f, 1.0f), FVector2D(1.0f, 1.0f),
		FVector2D(-10.0f, -12.0f), FVector2D(270.0f, 120.0f));

	StrategicPanel = CreatePanel(WidgetTree, TEXT("StrategicPanel"), FLinearColor(0.025f, 0.03f, 0.055f, 0.97f));
	StrategicPanel->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBox* StrategicBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StrategicBox"));
	UVerticalBox* RegionsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RegionsBox"));
	UTextBlock* RegionsTitle = CreateTextBlock(WidgetTree, TEXT("RegionsTitle"), 18, FLinearColor(0.95f, 0.86f, 0.55f));
	RegionsTitle->SetText(NSLOCTEXT("RomanTerritories", "Title", "CAMPAIGN REGIONS"));
	RegionsBox->AddChildToVerticalBox(RegionsTitle);
	TerritoryGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("TerritoryGrid"));
	TerritoryGrid->SetSlotPadding(FMargin(3.0f));
	TerritoryGrid->SetMinDesiredSlotWidth(170.0f);
	TerritoryGrid->SetMinDesiredSlotHeight(100.0f);
	RegionsBox->AddChildToVerticalBox(TerritoryGrid);
	StrategicMinimap = CreateWidget<UMinimapWidget>(GetOwningPlayer(), UMinimapWidget::StaticClass(), TEXT("StrategicMinimap"));
	StrategicMinimap->SetPresentationRoot(this);
	USizeBox* RegionMapSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RegionMapSize"));
	RegionMapSize->SetHeightOverride(260.f);
	RegionMapSize->SetContent(StrategicMinimap);
	RegionsBox->AddChildToVerticalBox(RegionMapSize);
	TacticalMinimap = CreateWidget<UMinimapWidget>(GetOwningPlayer(), UMinimapWidget::StaticClass(), TEXT("TacticalMinimap"));
	TacticalMinimap->SetPresentationRoot(this);
	PlaceFixed(RootCanvas, TacticalMinimap, FAnchors(1,0), FVector2D(1,0), FVector2D(-10,52), FVector2D(240,240));
	UHorizontalBoxSlot* RegionsSlot = StrategicBox->AddChildToHorizontalBox(RegionsBox);
	FSlateChildSize RegionsSize(ESlateSizeRule::Fill);
	RegionsSize.Value = 1.4f;
	RegionsSlot->SetSize(RegionsSize);
	RegionsSlot->SetPadding(FMargin(0, 0, 14, 0));
	TerritoryDetailText = CreateTextBlock(WidgetTree, TEXT("TerritoryDetailText"), 14, FLinearColor::White);
	UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TerritoryDetailScroll"));
	DetailScroll->AddChild(TerritoryDetailText);
	UVerticalBox* StrategicDetails = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StrategicDetails"));
	StrategicDetails->AddChildToVerticalBox(DetailScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StrategicBox->AddChildToHorizontalBox(StrategicDetails)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StrategicPanel->SetContent(StrategicBox);
	PlaceFixed(RootCanvas, StrategicPanel, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(1100.0f, 660.0f));

	OutcomePanel = CreatePanel(
		WidgetTree,
		TEXT("OutcomePanel"),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	OutcomePanel->SetVisibility(ESlateVisibility::Collapsed);
	OutcomeText = CreateTextBlock(WidgetTree, TEXT("OutcomeText"), 40, FLinearColor(1.0f, 0.85f, 0.15f));
	OutcomeText->SetJustification(ETextJustify::Center);
	OutcomeText->SetAutoWrapText(false);
	OutcomeText->SetWrapTextAt(640.0f);
	UCanvasPanel* OutcomeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("OutcomeCanvas"));
	UVerticalBox* OutcomeBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OutcomeBox"));
	OutcomeBox->AddChildToVerticalBox(OutcomeText);
	RestartEncounterButton = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), TEXT("RestartEncounterButton"));
	UTextBlock* RestartLabel = CreateTextBlock(WidgetTree, TEXT("RestartEncounterLabel"), 22, FLinearColor::White);
	RestartLabel->SetAutoWrapText(false);
	RestartLabel->SetText(NSLOCTEXT("RomanBattle", "RestartEncounter", "RESTART ENCOUNTER"));
	RestartEncounterButton->SetContent(RestartLabel);
	RestartEncounterButton->SetToolTipText(NSLOCTEXT("RomanBattle", "RestartSandbox", "Reload this sandbox encounter. This does not apply results to a persistent campaign."));
	RestartEncounterButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleRestartEncounterClicked);
	OutcomeBox->AddChildToVerticalBox(RestartEncounterButton);
	BuildCampaignControls(CommandsBox, OutcomeBox, StrategicDetails);
	BuildCampaignSaveControls();
	PlaceFixed(OutcomeCanvas, OutcomeBox, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(640.0f, 220.0f));
	OutcomePanel->SetContent(OutcomeCanvas);
	UCanvasPanelSlot* OutcomeSlot = RootCanvas->AddChildToCanvas(OutcomePanel);
	OutcomeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	OutcomeSlot->SetOffsets(FMargin(0.0f));
	BuildResponsiveShell();
}

void URomanEmpireMainWidget::BindCanonicalEvents()
{
	if (bCanonicalEventsBound) return;
	CachedController = GetOwningPlayer<ARomanEmpirePlayerController>();
	CachedGameState = GetWorld() ? GetWorld()->GetGameState<ARomanEmpireGameState>() : nullptr;
	if (CachedGameState)
	{
		CachedGameState->OnGameplayInitialized.AddUniqueDynamic(
			this, &URomanEmpireMainWidget::HandleGameplayInitialized);
	}
	if (!CachedGameState || !CachedGameState->IsGameplayInitialized()) return;
	if (!CachedController)
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("UMG root could not resolve its owning controller"));
		return;
	}

	CachedController->OnSelectionChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleSelectionChanged);
	CachedController->OnInteractionFeedback.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleInteractionFeedback);
	CachedController->OnPossessedUnitChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandlePossessedUnitChanged);
	CachedGameState->OnFactionResourcesChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleResourcesChanged);
	CachedGameState->OnPlayerFactionChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandlePlayerFactionChanged);
	CachedGameState->OnGameplayModeChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleGameplayModeChanged);
	CachedGameState->OnTurnStepStarted.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTurnStepStarted);
	CachedGameState->OnTurnProcessed.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTurnProcessed);
	CachedGameState->OnCampaignStarted.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleCampaignStarted);
	CachedGameState->OnCampaignObjectivesChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleObjectivesChanged);
	CachedGameState->OnTacticalBattleChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTacticalBattleChanged);
	CachedGameState->OnTerritoryOwnerChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTerritoryOwnerChanged);
	CachedGameState->OnTerritorySettlementChanged.AddUniqueDynamic(
		this, &URomanEmpireMainWidget::HandleTerritorySettlementChanged);
	CachedGameState->OnCampaignArmiesChanged.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCampaignArmiesChanged);
	CachedGameState->OnCampaignEncounterChanged.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCampaignArmiesChanged);
	CachedGameState->OnDiplomacyChanged.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleDiplomacyChanged);
	CachedGameState->OnVictory.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleVictory);
	CachedGameState->OnDefeat.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleDefeat);
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
		CachedController->OnInteractionFeedback.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleInteractionFeedback);
		CachedController->OnPossessedUnitChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandlePossessedUnitChanged);
	}
	if (CachedGameState)
	{
		CachedGameState->OnGameplayInitialized.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleGameplayInitialized);
		CachedGameState->OnFactionResourcesChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleResourcesChanged);
		CachedGameState->OnPlayerFactionChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandlePlayerFactionChanged);
		CachedGameState->OnGameplayModeChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleGameplayModeChanged);
		CachedGameState->OnTurnStepStarted.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTurnStepStarted);
		CachedGameState->OnTurnProcessed.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTurnProcessed);
		CachedGameState->OnCampaignStarted.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleCampaignStarted);
		CachedGameState->OnCampaignObjectivesChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleObjectivesChanged);
		CachedGameState->OnTacticalBattleChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTacticalBattleChanged);
		CachedGameState->OnTerritoryOwnerChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTerritoryOwnerChanged);
		CachedGameState->OnTerritorySettlementChanged.RemoveDynamic(
			this, &URomanEmpireMainWidget::HandleTerritorySettlementChanged);
		CachedGameState->OnCampaignArmiesChanged.RemoveDynamic(this, &URomanEmpireMainWidget::HandleCampaignArmiesChanged);
		CachedGameState->OnCampaignEncounterChanged.RemoveDynamic(this, &URomanEmpireMainWidget::HandleCampaignArmiesChanged);
		CachedGameState->OnDiplomacyChanged.RemoveDynamic(this, &URomanEmpireMainWidget::HandleDiplomacyChanged);
		CachedGameState->OnVictory.RemoveDynamic(this, &URomanEmpireMainWidget::HandleVictory);
		CachedGameState->OnDefeat.RemoveDynamic(this, &URomanEmpireMainWidget::HandleDefeat);
	}
	bCanonicalEventsBound = false;
	CachedGameState = nullptr;
	CachedController = nullptr;
}

void URomanEmpireMainWidget::BindSelectionEvents(
	const TArray<AUnitBase*>& Units,
	ABuildingBase* Building)
{
	UnbindSelectionEvents();
	ObservedUnit = Units.IsEmpty() ? nullptr : Units[0];
	if (CachedGameState && CachedGameState->GetCurrentGameplayMode() == EGameplayMode::Possessed)
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
	if (CachedGameState)
	{
		UpdateResourceDisplay(CachedGameState->GetFactionResources(CachedGameState->GetPlayerFaction()));
	}
	RefreshModeAndTurn();
	if (CachedGameState)
	{
		const FCampaignObjectiveState State = CachedGameState->GetObjectiveState();
		RefreshObjectives(State);
		RefreshBattle(CachedGameState->GetTacticalBattleResult());
		RefreshOutcome(State.Outcome);
	}
	RefreshSelection();
	RefreshDiagnostic();
	RefreshCommands();
	RefreshStrategicTerritories();
	RefreshCampaignControls();
	RefreshTextScale();
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
	const EGameplayMode Mode = CachedGameState
		? CachedGameState->GetCurrentGameplayMode()
		: EGameplayMode::Tactical;
	const int32 Turn = CachedGameState ? CachedGameState->GetCurrentTurn() : 0;
	PhaseText->SetText(FText::FromString(FString::Printf(
		TEXT("%s   |   Turn %d"),
		RomanEmpireGameplay::GetGameplayModeName(Mode),
		Turn)));
}

void URomanEmpireMainWidget::RefreshCommands()
{
	const EGameplayMode Mode = CachedGameState
		? CachedGameState->GetCurrentGameplayMode() : EGameplayMode::Tactical;
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
		const bool bRetry = CachedController && CachedController->IsAwaitingAITacticalTravel();
		EndTurnButton->SetToolTipText(FText::FromString(bRetry ? TEXT("Retry the accepted AI attack without advancing another turn") : TEXT("Advance one campaign turn")));
		EndTurnButton->SetVisibility(Mode == EGameplayMode::Possessed
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (!bCanBuild)
	{
		HideBuildingMenu();
	}
	RefreshInputLabels();
}

void URomanEmpireMainWidget::HandleBindingsChanged(UEnhancedInputUserSettings* Settings)
{
	RefreshInputLabels(); // Labels only: do not latch paused command enablement into the resumed HUD.
}
void URomanEmpireMainWidget::RefreshInputLabels()
{
	const auto* Config=CachedController?CachedController->GetInputConfiguration():nullptr;
	if(!Config) return;
	const EGameplayMode Mode=CachedGameState?CachedGameState->GetCurrentGameplayMode():EGameplayMode::Tactical;
	const UInputMappingContext* Context=Config->GetContextForMode(Mode);
	const auto Key=[&](const UInputAction* Action){return RomanInputRemapping::DescribeKeys(Context,Action,BindingSettingsSource.Get());};
	const FString Build=RomanInputRemapping::DescribeKeys(Config->TacticalContext,Config->BuildMenu,BindingSettingsSource.Get());
	if(BuildMenuButton) if(auto* Label=Cast<UTextBlock>(BuildMenuButton->GetContent())) Label->SetText(FText::FromString(FString::Printf(TEXT("BUILD MENU [%s]"),*Build)));
	if(EndTurnButton) if(auto* Label=Cast<UTextBlock>(EndTurnButton->GetContent())) Label->SetText(FText::FromString(FString::Printf(TEXT("%s [%s]"),CachedController && CachedController->IsAwaitingAITacticalTravel()?TEXT("RETRY"):TEXT("END TURN"),*Key(Config->EndTurn))));
	if(ControlsText)
	{
		FString Help;
		if(Mode==EGameplayMode::Possessed) Help=FString::Printf(TEXT("SOLDIER CONTROLS\n%s: move | Mouse: look\n%s: attack | Hold %s: block\n%s / Esc: return to camera\nF10: settings"),*Key(Config->SoldierMove),*Key(Config->Attack),*Key(Config->Block),*Key(Config->ExitPossession));
		else if(Mode==EGameplayMode::Strategic) Help=FString::Printf(TEXT("STRATEGIC CONTROLS\nClick a region: inspect\n%s / screen edge: move camera\nMouse wheel: zoom into tactical view\n%s: end turn | F10: settings"),*Key(Config->CameraPan),*Key(Config->EndTurn));
		else
		{
			TArray<FString> Slots; for(const auto& SlotAction:Config->BuildingSlots) Slots.Add(Key(SlotAction));
			Help=FString::Printf(TEXT("TACTICAL CONTROLS\n%s / screen edge: move camera\nWheel: zoom | %s / drag: select\nHold %s + select: toggle / add\n%s: order | %s: possess unit\nSelected units: STOP / HOLD buttons\n%s: menu | %s: build (menu open)\n%s: end turn | Esc: cancel / clear\nF10: settings"),*Key(Config->CameraPan),*Key(Config->Select),*Key(Config->SelectionModifier),*Key(Config->Command),*Key(Config->EnterPossession),*Build,*FString::Join(Slots,TEXT(" / ")),*Key(Config->EndTurn));
		}
		ControlsText->SetText(FText::FromString(Help));
	}
	if(BuildingMenu) BuildingMenu->RefreshShortcutLabels();
}

bool URomanEmpireMainWidget::SelectTerritory(FName TerritoryID)
{
	if (!bCanonicalEventsBound || !IsInViewport()) return false;
	if (!IsValid(CachedGameState) || !CachedGameState->IsGameplayInitialized() ||
		CachedGameState->GetCurrentGameplayMode() != EGameplayMode::Strategic)
	{
		RefreshStrategicTerritories();
		return false;
	}
	FTerritoryPresentationSnapshot Snapshot;
	const bool bFound = CachedGameState->TryGetTerritorySnapshot(TerritoryID, Snapshot);
	SelectedTerritoryID = bFound ? TerritoryID : NAME_None;
	RefreshStrategicTerritories();
	return bFound;
}

void URomanEmpireMainWidget::HandleTerritoryClicked(FName TerritoryID)
{
	SelectTerritory(TerritoryID);
}

void URomanEmpireMainWidget::HandleTerritoryOwnerChanged(
	FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner)
{
	(void)TerritoryID;
	(void)PreviousOwner;
	(void)NewOwner;
	// Notifications may be overtaken by another synchronous observer.
	RefreshStrategicTerritories();
}

void URomanEmpireMainWidget::RefreshStrategicTerritories()
{
	RefreshCampaignArmies();
	if (!StrategicPanel || !TerritoryGrid || !TerritoryDetailText) return;
	const bool bShow = bCanonicalEventsBound && IsValid(CachedGameState) &&
		CachedGameState->IsGameplayInitialized() && CachedGameState->GetCurrentGameplayMode() == EGameplayMode::Strategic;
	if (StrategicMinimap) StrategicMinimap->SetPresentationActive(bShow && IsInViewport());
	if (TacticalMinimap) TacticalMinimap->SetPresentationActive(!bShow && IsInViewport() &&
		bCanonicalEventsBound && CachedGameState && CachedGameState->IsGameplayInitialized());
	if (TacticalMinimapFrame) TacticalMinimapFrame->SetVisibility(TacticalMinimap && TacticalMinimap->IsPresentationActive()
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	StrategicPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bShow)
	{
		SelectedTerritoryID = NAME_None;
		for (URomanTerritoryButton* Button : TerritoryButtons) Button->SetIsEnabled(false);
		return;
	}
	const TArray<FTerritoryPresentationSnapshot> Rows = CachedGameState->GetTerritorySnapshots();
	bool bSameIDs = Rows.Num() == TerritoryButtons.Num();
	for (int32 Index = 0; bSameIDs && Index < Rows.Num(); ++Index)
	{
		bSameIDs = TerritoryButtons[Index]->GetTerritoryID() == Rows[Index].TerritoryID;
	}
	if (!bSameIDs)
	{
		for (URomanTerritoryButton* Button : TerritoryButtons)
		{
			Button->OnTerritoryClicked.RemoveDynamic(this, &URomanEmpireMainWidget::HandleTerritoryClicked);
		}
		TerritoryGrid->ClearChildren();
		TerritoryButtons.Reset();
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			const FName ID = Rows[Index].TerritoryID;
			const FName ButtonName = MakeUniqueObjectName(WidgetTree, URomanTerritoryButton::StaticClass(),
				FName(*FString::Printf(TEXT("TerritoryButton_%s"), *ID.ToString())));
			URomanTerritoryButton* Button = WidgetTree->ConstructWidget<URomanTerritoryButton>(URomanTerritoryButton::StaticClass(), ButtonName);
			Button->InitializeTerritoryButton(ID);
			Button->OnTerritoryClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleTerritoryClicked);
			const FName LabelName = MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(),
				FName(*FString::Printf(TEXT("TerritoryLabel_%s"), *ID.ToString())));
			UTextBlock* Label = CreateTextBlock(WidgetTree, LabelName, 14, FLinearColor::White);
			Label->SetAutoWrapText(true);
			Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
			Button->SetContent(Label);
			TerritoryGrid->AddChildToUniformGrid(Button, Index / 3, Index % 3);
			TerritoryButtons.Add(Button);
		}
		RefreshTextScale(); // Newly authored labels use the current preference without a tick scan.
	}
	const FTerritoryPresentationSnapshot* Selected = Rows.FindByPredicate([this](const FTerritoryPresentationSnapshot& Row)
	{
		return Row.TerritoryID == SelectedTerritoryID;
	});
	if (!Selected) SelectedTerritoryID = NAME_None;
	auto OwnerLabel = [](const FTerritoryPresentationSnapshot& Row)
	{
		if (Row.OwnerFaction == EFactionID::None) return NSLOCTEXT("RomanTerritories", "Unowned", "Unowned");
		return !Row.OwnerDisplayName.IsEmpty() ? Row.OwnerDisplayName :
			StaticEnum<EFactionID>()->GetDisplayNameTextByValue(static_cast<int64>(Row.OwnerFaction));
	};
	auto ActivityLabel = [](const FTerritoryPresentationSnapshot& Row)
	{
		return Row.bOwnerActive ? NSLOCTEXT("RomanTerritories", "Active", "Active participant") :
			(Row.OwnerFaction == EFactionID::None ? NSLOCTEXT("RomanTerritories", "NoParticipant", "No participant") :
			NSLOCTEXT("RomanTerritories", "Dormant", "Dormant faction"));
	};
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const FTerritoryPresentationSnapshot& Row = Rows[Index];
		URomanTerritoryButton* Button = TerritoryButtons[Index];
		Button->SetIsEnabled(true); // Dormant/unowned regions can still be inspected.
		Button->SetBackgroundColor(FMath::Lerp(FLinearColor::White, Row.OwnerColor, 0.25f));
		Button->SetToolTipText(FText::Format(NSLOCTEXT("RomanTerritories", "CardTooltip", "{0}\n{1} | {2}"),
			Row.DisplayName, OwnerLabel(Row), ActivityLabel(Row)));
		const FText CardActivity = Row.bOwnerActive ? NSLOCTEXT("RomanTerritories", "CardActive", "Active") :
			(Row.OwnerFaction == EFactionID::None ? NSLOCTEXT("RomanTerritories", "CardNoOwner", "No owner") :
			NSLOCTEXT("RomanTerritories", "CardDormant", "Dormant"));
		CastChecked<UTextBlock>(Button->GetContent())->SetText(FText::Format(
			NSLOCTEXT("RomanTerritories", "Card", "{0}{1}\n{2}\n{3}"),
			Row.TerritoryID == SelectedTerritoryID ? FText::FromString(TEXT("* ")) : FText::GetEmpty(),
			Row.DisplayName, OwnerLabel(Row), CardActivity));
	}
	if (!Selected)
	{
		TerritoryDetailText->SetText(Rows.IsEmpty() ? NSLOCTEXT("RomanTerritories", "Unavailable", "Territory data unavailable.") :
			NSLOCTEXT("RomanTerritories", "ChooseArmyTarget", "Select a region to inspect.\n\nBase and effective production are shown separately. Dormant/unowned regions do not credit a wallet.\nSelect an army, then a target region to move or start battle."));
		return;
	}
	TArray<FString> NeighborIDs;
	for (FName ID : Selected->AdjacentTerritoryIDs) NeighborIDs.Add(ID.ToString());
	const FFactionResourceDelta& Production = Selected->BaseResourceProduction;
	FFormatNamedArguments Args;
	Args.Add(TEXT("Name"), Selected->DisplayName);
	Args.Add(TEXT("ID"), FText::FromName(Selected->TerritoryID));
	Args.Add(TEXT("Description"), Selected->Description);
	Args.Add(TEXT("Owner"), OwnerLabel(*Selected));
	Args.Add(TEXT("Activity"), ActivityLabel(*Selected));
	Args.Add(TEXT("Terrain"), StaticEnum<ETerrainType>()->GetDisplayNameTextByValue(static_cast<int64>(Selected->TerrainType)));
	Args.Add(TEXT("Neighbors"), NeighborIDs.IsEmpty() ? NSLOCTEXT("RomanTerritories", "NoNeighbors", "None") : FText::FromString(FString::Join(NeighborIDs, TEXT(", "))));
	Args.Add(TEXT("Gold"), FText::AsNumber(Production.Gold));
	Args.Add(TEXT("Food"), FText::AsNumber(Production.Food));
	Args.Add(TEXT("Iron"), FText::AsNumber(Production.Iron));
	Args.Add(TEXT("Wood"), FText::AsNumber(Production.Wood));
	Args.Add(TEXT("Stone"), FText::AsNumber(Production.Stone));
	Args.Add(TEXT("Population"), FText::AsNumber(Production.Population));
	Args.Add(TEXT("Settlement"), Selected->Settlement.bHasSettlement ? Selected->Settlement.SettlementName :
		NSLOCTEXT("RomanTerritories", "NoSettlement", "None"));
	Args.Add(TEXT("Residents"), FText::AsNumber(Selected->Settlement.Population));
	const FFactionResourceDelta& Effective = Selected->EffectiveResourceProduction;
	Args.Add(TEXT("EffectiveGold"), FText::AsNumber(Effective.Gold));
	Args.Add(TEXT("EffectiveFood"), FText::AsNumber(Effective.Food));
	Args.Add(TEXT("EffectiveIron"), FText::AsNumber(Effective.Iron));
	Args.Add(TEXT("EffectiveWood"), FText::AsNumber(Effective.Wood));
	Args.Add(TEXT("EffectiveStone"), FText::AsNumber(Effective.Stone));
	Args.Add(TEXT("EffectivePopulation"), FText::AsNumber(Effective.Population));
	TerritoryDetailText->SetText(FText::Format(NSLOCTEXT("RomanTerritories", "DetailLive",
		"{Name} [{ID}]\n{Owner} | {Activity}\nTerrain: {Terrain}\nNeighbor IDs: {Neighbors}\nSettlement: {Settlement} | Residents: {Residents}\n\nBase production (not effective yield):\nGold {Gold} | Food {Food} | Iron {Iron}\nWood {Wood} | Stone {Stone} | Population {Population}\n\nEffective production per turn:\nGold {EffectiveGold} | Food {EffectiveFood} | Iron {EffectiveIron}\nWood {EffectiveWood} | Stone {EffectiveStone} | Population {EffectivePopulation}\nDormant/unowned: no wallet credit.\n\n{Description}"), Args));
}

void URomanEmpireMainWidget::HandleTerritorySettlementChanged(FName TerritoryID)
{
	(void)TerritoryID;
	RefreshStrategicTerritories();
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

void URomanEmpireMainWidget::RefreshBattle(const FTacticalBattleResult& Result)
{
	if (!BattleText) return;
	if (Result.ObjectiveID.IsNone())
	{
		BattleText->SetText(FText::FromString(TEXT("BATTLE  awaiting encounter")));
		return;
	}
	const EFactionID PlayerFaction = CachedGameState ? CachedGameState->GetPlayerFaction() : EFactionID::None;
	int32 OwnAlive = 0;
	int32 OwnTotal = 0;
	int32 FoeAlive = 0;
	int32 FoeTotal = 0;
	for (const FTacticalBattleSquadResult& Squad : Result.Squads)
	{
		if (Squad.Faction == PlayerFaction)
		{
			OwnAlive += Squad.Survivors;
			OwnTotal += Squad.InitialUnits;
		}
		else
		{
			FoeAlive += Squad.Survivors;
			FoeTotal += Squad.InitialUnits;
		}
	}
	const auto Plan = CachedGameState ? CachedGameState->GetCampaignEncounterPlan() : FCampaignEncounterPlan();
	const bool bDefending = Plan.EncounterID.IsValid() && Plan.PlayerFaction == Plan.Defender;
	const TCHAR* Status = Result.Outcome == ETacticalBattleOutcome::PlayerVictory ? TEXT("Victory") :
		Result.Outcome == ETacticalBattleOutcome::PlayerDefeat ? TEXT("Defeat") : Result.Outcome == ETacticalBattleOutcome::Draw ? TEXT("Draw") :
		bDefending ? TEXT("Defend camp") : TEXT("Capture camp");
	BattleText->SetText(FText::FromString(FString::Printf(
		TEXT("BATTLE  Allies %d/%d | Foes %d/%d | %s"),
		OwnAlive, OwnTotal, FoeAlive, FoeTotal, Status)));
}

void URomanEmpireMainWidget::RefreshSelection()
{
	if (TrainButton) TrainButton->SetIsEnabled(false);
	if (CancelTrainingButton)
	{
		CancelTrainingButton->SetVisibility(ESlateVisibility::Collapsed);
		CancelTrainingButton->SetIsEnabled(false);
	}
	if (StopButton) StopButton->SetVisibility(ESlateVisibility::Collapsed);
	if (HoldButton) HoldButton->SetVisibility(ESlateVisibility::Collapsed);
	if (!SelectionPanel || !SelectionText || !ProductionText || !TrainButton)
	{
		return;
	}
	const bool bPossessed = CachedGameState &&
		CachedGameState->GetCurrentGameplayMode() == EGameplayMode::Possessed;
	if (PossessionPanel)
		PossessionPanel->SetVisibility(bPossessed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
				? FText::FromString(FString::Printf(TEXT("%s\nHP %d / %d   |   Stamina %.0f%%   |   Morale %d   |   Guard %s"),
					*Soldier->GetUnitDisplayName(), Soldier->GetCurrentHealth(), Soldier->GetMaxHealth(),
					Soldier->GetStaminaPercent() * 100.0f, Soldier->GetCurrentMorale(),
					Soldier->IsBlocking() ? TEXT("UP") : TEXT("DOWN")))
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
		const bool bCanCommand = CachedController && CachedController->CanIssueTacticalCommands();
		if (StopButton)
		{
			StopButton->SetVisibility(bCanCommand ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			StopButton->SetIsEnabled(bCanCommand);
		}
		if (HoldButton)
		{
			HoldButton->SetVisibility(bCanCommand ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			HoldButton->SetIsEnabled(bCanCommand);
		}
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

	if (!CachedController || !CachedController->IsPlayerBuildingEligible(Building))
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
	if (Barracks->IsAwaitingTrainingRefund())
	{
		ProductionText->SetText(FText::Format(
			NSLOCTEXT("RomanProduction", "AwaitingRefund", "Production failed: refund pending | {0} pending"),
			FText::AsNumber(Barracks->GetPendingOrderCount())));
	}
	else if (Barracks->IsTraining())
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
		CachedController->IsPlayerBuildingEligible(Building) &&
		!Data.TrainableUnits.IsEmpty();
	TrainButton->SetVisibility(bCanTrain ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	const bool bCanCancel = CachedController && CachedController->CanCancelSelectedTraining();
	if (CancelTrainingButton)
	{
		CancelTrainingButton->SetVisibility(bCanCancel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CancelTrainingButton->SetIsEnabled(bCanCancel);
	}
	if (bCanTrain && TrainButtonText)
	{
		const URomanUnitDefinition* Unit = Registry ? Registry->FindUnit(Data.TrainableUnits[0]) : nullptr;
		// Use the same atomic spending rules on a local query copy; this never mutates a wallet.
		FFactionResourceAmounts Available = CachedGameState->GetFactionResources(Building->GetOwnerFaction());
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
	// Keep canonical diagnostic values current without covering the native FPS
	// aim point at accessible text/DPI sizes. This is presentation, not state.
	if (DiagnosticPanel) DiagnosticPanel->SetVisibility(CachedGameState && CachedGameState->GetCurrentGameplayMode() == EGameplayMode::Possessed
		? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const EFactionID PlayerFaction = CachedGameState
		? CachedGameState->GetPlayerFaction()
		: EFactionID::None;
	const FFactionResourceAmounts Resources = CachedGameState
		? CachedGameState->GetFactionResources(PlayerFaction)
		: FFactionResourceAmounts();
	const int32 TerritoryCount = CachedGameState
		? CachedGameState->GetFactionTerritoryCount(PlayerFaction)
		: 0;
	const EGameplayMode Mode = CachedGameState
		? CachedGameState->GetCurrentGameplayMode()
		: EGameplayMode::Tactical;

	DiagnosticText->SetText(FText::FromString(FString::Printf(
		TEXT("CANONICAL STATE\nMode=%s  Turn=%d  PlayerTerritories=%d  Gold=%d\nDefinitions F/U/B/T=%d/%d/%d/%d"),
		RomanEmpireGameplay::GetGameplayModeName(Mode),
		CachedGameState ? CachedGameState->GetCurrentTurn() : 0,
		TerritoryCount,
		Resources.Gold,
		Registry ? Registry->GetAllFactions().Num() : 0,
		Registry ? Registry->GetAllUnits().Num() : 0,
		Registry ? Registry->GetAllBuildings().Num() : 0,
		Registry ? Registry->GetAllTerritories().Num() : 0)));
}

void URomanEmpireMainWidget::RefreshOutcome(ECampaignOutcome Outcome)
{
	RefreshCampaignControls();
	if (!OutcomePanel || !OutcomeText)
	{
		return;
	}
	if (RestartEncounterButton)
	{
		RestartEncounterButton->SetVisibility(ESlateVisibility::Collapsed);
		RestartEncounterButton->SetIsEnabled(false);
	}
	if (Outcome == ECampaignOutcome::InProgress)
	{
		const FTacticalBattleResult Battle = CachedGameState ? CachedGameState->GetTacticalBattleResult() : FTacticalBattleResult();
		if (Battle.IsResolved())
		{
			OutcomePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			const TCHAR* Title = Battle.Outcome == ETacticalBattleOutcome::PlayerVictory ? TEXT("TACTICAL VICTORY") :
				Battle.Outcome == ETacticalBattleOutcome::PlayerDefeat ? TEXT("TACTICAL DEFEAT") : TEXT("TACTICAL DRAW");
			const FString Detail = Battle.CapturedObjectiveID.IsNone() ? TEXT("Encounter resolved") :
				Battle.Outcome == ETacticalBattleOutcome::PlayerDefeat ? TEXT("Your camp was captured") :
				FString::Printf(TEXT("Captured: %s"), *Battle.CapturedObjectiveID.ToString());
			OutcomeText->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), Title, *Detail)));
			OutcomeText->SetColorAndOpacity(FSlateColor(Battle.Outcome == ETacticalBattleOutcome::PlayerVictory
				? FLinearColor(1.0f, 0.85f, 0.2f) : FLinearColor(1.0f, 0.4f, 0.35f)));
			if (RestartEncounterButton)
			{
				RestartEncounterButton->SetVisibility(CachedSession && CachedSession->HasSession() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
				RestartEncounterButton->SetIsEnabled(IsInViewport() && bCanonicalEventsBound && CachedController && CachedController->CanRestartEncounter());
			}
			return;
		}
		OutcomePanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	OutcomePanel->SetVisibility(ContinueCampaignButton && ContinueCampaignButton->GetVisibility() == ESlateVisibility::Visible
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::HitTestInvisible);
	if (Outcome == ECampaignOutcome::Victory)
	{
		OutcomeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.1f)));
		const TCHAR* Condition = CachedGameState &&
			CachedGameState->GetResolvedVictoryCondition() == EVictoryCondition::Economic
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

void URomanEmpireMainWidget::ClearInteractionFeedback()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(InteractionFeedbackTimer);
	if (InteractionFeedbackPanel) InteractionFeedbackPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (InteractionFeedbackText)
	{
		InteractionFeedbackText->SetText(FText::GetEmpty());
		InteractionFeedbackText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::HandleInteractionFeedback(FText Message)
{
	ClearInteractionFeedback();
	if (Message.IsEmpty() || !InteractionFeedbackText || !InteractionFeedbackPanel || !GetWorld()) return;
	InteractionFeedbackText->SetText(Message);
	InteractionFeedbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	InteractionFeedbackPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	GetWorld()->GetTimerManager().SetTimer(InteractionFeedbackTimer, this,
		&URomanEmpireMainWidget::ClearInteractionFeedback, 3.0f, false);
}

void URomanEmpireMainWidget::HandleSelectionChanged(
	const TArray<AUnitBase*>& Units,
	ABuildingBase* Building)
{
	ClearInteractionFeedback();
	(void)Units;
	(void)Building;
	// A preceding observer can commit another selection before this callback runs.
	if (CachedController)
	{
		BindSelectionEvents(CachedController->GetSelectedUnits(), CachedController->GetSelectedBuilding());
	}
	RefreshSelection();
	RefreshDiagnostic();
}

void URomanEmpireMainWidget::HandleResourcesChanged(
	EFactionID FactionID,
	FFactionResourceAmounts NewBalance)
{
	(void)NewBalance;
	if (CachedGameState && FactionID == CachedGameState->GetPlayerFaction())
	{
		UpdateResourceDisplay(CachedGameState->GetFactionResources(FactionID));
		RefreshCampaignArmyActions();
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
	RefreshStrategicTerritories();
	ScheduleSaveRefresh(); // Existing coalesced read runs after transition guards release.
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
	ScheduleSaveRefresh();
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
	(void)State;
	if (!CachedGameState) return;
	// Another observer can commit a nested transaction before this callback runs.
	// The notification is an invalidation signal, not a newer-than-current snapshot.
	const FCampaignObjectiveState CurrentState = CachedGameState->GetObjectiveState();
	RefreshObjectives(CurrentState);
	RefreshOutcome(CurrentState.Outcome);
	RefreshSelection();
	RefreshDiagnostic();
	RefreshCommands();
}

void URomanEmpireMainWidget::HandleTacticalBattleChanged(FTacticalBattleResult Result)
{
	(void)Result;
	if (!CachedGameState) return;
	RefreshBattle(CachedGameState->GetTacticalBattleResult());
	RefreshOutcome(CachedGameState->GetObjectiveState().Outcome);
	RefreshSelection();
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

void URomanEmpireMainWidget::HandleStopClicked()
{
	if (CachedController) CachedController->CommandSelectedUnitsStop();
}

void URomanEmpireMainWidget::HandleCancelTrainingClicked()
{
	if (!IsInViewport() || !bCanonicalEventsBound || !IsValid(CachedGameState) ||
		!CachedGameState->IsGameplayInitialized() || !CachedController ||
		!CachedController->CanCancelSelectedTraining()) return;
	if (!CachedController->CancelSelectedTraining() && ProductionText)
	{
		ProductionText->SetText(NSLOCTEXT("RomanProduction", "RefundFailed", "Cannot cancel: refund unavailable; training order retained."));
	}
}

void URomanEmpireMainWidget::HandleRestartEncounterClicked()
{
	if (!IsInViewport() || !bCanonicalEventsBound || !IsValid(CachedGameState) ||
		!CachedGameState->IsGameplayInitialized() || !CachedController || !CachedController->CanRestartEncounter()) return;
	CachedController->RestartEncounter();
	RefreshOutcome(CachedGameState->GetObjectiveState().Outcome);
}

void URomanEmpireMainWidget::HandleHoldClicked()
{
	if (CachedController) CachedController->CommandSelectedUnitsHold();
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
