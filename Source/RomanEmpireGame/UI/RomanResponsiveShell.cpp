// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanEmpireMainWidget.h"
#include "UI/MinimapWidget.h"
#include "Core/RomanEmpirePlayerController.h"
#include "UI/BuildingMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void URomanEmpireMainWidget::BuildResponsiveShell()
{
	// Construction only. Existing widgets/delegates/command ownership are retained.
	// Flex/scroll arrangements react to geometry/DPI, not a viewport polling tick.
	auto* Safe = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("HUDSafeZone"));
	auto* Shell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResponsiveHUDShell"));
	Safe->SetContent(Shell); Safe->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Shell->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto* ShellSlot = RootCanvas->AddChildToCanvas(Safe);
	ShellSlot->SetAnchors(FAnchors(0,0,1,1)); ShellSlot->SetOffsets(FMargin(10));
	ShellSlot->SetZOrder(0);
	const auto Move = [](UWidget* Widget) { if (Widget) Widget->RemoveFromParent(); };
	const auto Column = [](UHorizontalBox* Row, UWidget* Widget, float Weight)
	{
		auto* Child = Row->AddChildToHorizontalBox(Widget);
		FSlateChildSize Size(ESlateSizeRule::Fill); Size.Value = Weight;
		Child->SetSize(Size); Child->SetPadding(FMargin(5,0)); return Child;
	};
	auto* TopBar = CastChecked<UBorder>(GetWidgetFromName(TEXT("TopBar")));
	Move(TopBar); Move(ResourceText); Move(PhaseText);
	auto* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResponsiveHeader"));
	ResourceText->SetAutoWrapText(true);
	Header->AddChildToHorizontalBox(ResourceText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	auto* PhaseSlot = Header->AddChildToHorizontalBox(PhaseText);
	PhaseSlot->SetPadding(FMargin(16,0,0,0)); PhaseSlot->SetVerticalAlignment(VAlign_Center);
	TopBar->SetContent(Header); Shell->AddChildToVerticalBox(TopBar);
	TopBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto* SettingsButton=WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(),TEXT("OpenSettingsButton"));
	auto* SettingsLabel=WidgetTree->ConstructWidget<UTextBlock>(); SettingsLabel->SetText(FText::FromString(TEXT("Settings (F10)")));
	FSlateFontInfo SettingsFont=SettingsLabel->GetFont(); SettingsFont.Size=14; SettingsLabel->SetFont(SettingsFont);
	SettingsButton->SetContent(SettingsLabel); SettingsButton->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleSettingsClicked);
	Header->AddChildToHorizontalBox(SettingsButton)->SetPadding(FMargin(12,0,0,0));

	auto* Information = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResponsiveInformation"));
	Information->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto* ObjectivePanel = GetWidgetFromName(TEXT("ObjectivePanel"));
	Move(ObjectivePanel); Column(Information, ObjectivePanel, 1.f)->SetVerticalAlignment(VAlign_Top);
	auto* Notices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResponsiveNotices"));
	for (UWidget* Widget : {static_cast<UWidget*>(CampaignSavePanel.Get()),
		static_cast<UWidget*>(DiagnosticPanel.Get()), static_cast<UWidget*>(InteractionFeedbackPanel.Get())})
	{
		Move(Widget); Notices->AddChildToVerticalBox(Widget)->SetPadding(FMargin(0,0,0,4));
	}
	Column(Information, Notices, 1.6f)->SetVerticalAlignment(VAlign_Top);
	Move(TacticalMinimap);
	TacticalMinimapFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TacticalMinimapFrame"));
	TacticalMinimapFrame->SetWidthOverride(260); TacticalMinimapFrame->SetHeightOverride(240);
	TacticalMinimapFrame->SetContent(TacticalMinimap);
	Information->AddChildToHorizontalBox(TacticalMinimapFrame)->SetPadding(FMargin(5,0));
	Shell->AddChildToVerticalBox(Information)->SetPadding(FMargin(0,8));

	auto* Body = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ResponsiveBody"));
	Body->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Shell->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	// Both strategic columns scroll as complete documents. Recruitment/battle
	// buttons cannot be pushed outside the viewport by a long region description.
	auto* StrategicBox = CastChecked<UHorizontalBox>(GetWidgetFromName(TEXT("StrategicBox")));
	auto* Regions = GetWidgetFromName(TEXT("RegionsBox"));
	auto* Details = CastChecked<UVerticalBox>(GetWidgetFromName(TEXT("StrategicDetails")));
	auto* OldDetailScroll = CastChecked<UScrollBox>(GetWidgetFromName(TEXT("TerritoryDetailScroll")));
	Move(TerritoryDetailText); Details->RemoveChild(OldDetailScroll);
	Details->InsertChildAt(0, TerritoryDetailText);
	Move(Regions); Move(Details);
	auto* RegionScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResponsiveRegionScroll"));
	auto* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResponsiveDetailScroll"));
	RegionScroll->AddChild(Regions); DetailScroll->AddChild(Details);
	Column(StrategicBox, RegionScroll, 1.1f); Column(StrategicBox, DetailScroll, 1.f);
	Move(StrategicPanel);
	auto* StrategyFit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StrategicFit"));
	StrategyFit->SetMaxDesiredWidth(1600); StrategyFit->SetContent(StrategicPanel);
	auto* StrategySlot = Body->AddChildToOverlay(StrategyFit);
	StrategySlot->SetHorizontalAlignment(HAlign_Center);
	StrategySlot->SetVerticalAlignment(VAlign_Fill);
	// SizeBox visibility follows the panel's desired size; its empty area passes
	// through pointer input. The authoritative strategic visibility remains unchanged.
	StrategyFit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Move(BuildMenuPanel);
	auto* MenuContents = BuildMenuPanel->GetContent(); Move(MenuContents);
	auto* MenuScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResponsiveBuildingMenuScroll"));
	MenuScroll->AddChild(MenuContents); BuildMenuPanel->SetContent(MenuScroll);
	auto* MenuFit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BuildingMenuFit"));
	MenuFit->SetWidthOverride(390); MenuFit->SetMaxDesiredHeight(320); MenuFit->SetContent(BuildMenuPanel);
	auto* MenuSlot = Body->AddChildToOverlay(MenuFit);
	MenuSlot->SetHorizontalAlignment(HAlign_Left); MenuSlot->SetVerticalAlignment(VAlign_Center);
	MenuFit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	auto* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResponsiveFooter"));
	auto* FooterFit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FooterFit"));
	// Leave room for a complete two-line strategic command at 200% text/DPI125.
	// Footer documents retain their existing scroll boxes; font size is unchanged.
	FooterFit->SetMaxDesiredHeight(180); FooterFit->SetContent(Footer);
	Shell->AddChildToVerticalBox(FooterFit)->SetPadding(FMargin(0,8,0,0));
	auto* Controls = GetWidgetFromName(TEXT("ControlsPanel")); Move(Controls);
	auto* ControlsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("ResponsiveControlsScroll"));
	Controls->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ControlsScroll->AddChild(Controls);
	Column(Footer, ControlsScroll, 0.8f)->SetVerticalAlignment(VAlign_Fill);
	auto* Center = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ResponsiveFooterCenter"));
	Move(SelectionPanel); Move(PossessionText);
	auto* SelectionContent = SelectionPanel->GetContent(); Move(SelectionContent);
	auto* SelectionScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResponsiveSelectionScroll"));
	SelectionScroll->AddChild(SelectionContent); SelectionPanel->SetContent(SelectionScroll);
	auto* SelectionSlot = Center->AddChildToOverlay(SelectionPanel);
	SelectionSlot->SetHorizontalAlignment(HAlign_Fill); SelectionSlot->SetVerticalAlignment(VAlign_Fill);
	PossessionPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PossessionPanel"));
	PossessionPanel->SetBrushColor(FLinearColor(0.025f,0.03f,0.055f,0.94f));
	PossessionPanel->SetPadding(FMargin(10,7)); PossessionPanel->SetContent(PossessionText);
	PossessionPanel->SetVisibility(ESlateVisibility::Collapsed);
	auto* PossessionSlot = Center->AddChildToOverlay(PossessionPanel);
	PossessionSlot->SetHorizontalAlignment(HAlign_Fill);
	PossessionSlot->SetVerticalAlignment(VAlign_Bottom);
	Column(Footer, Center, 1.8f);
	CastChecked<UHorizontalBoxSlot>(TrainButton->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CastChecked<UHorizontalBoxSlot>(CancelTrainingButton->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	auto* Commands = GetWidgetFromName(TEXT("CommandsBox")); Move(Commands);
	auto* CommandsFit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CommandsFit"));
	auto* CommandsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("ResponsiveCommandsScroll"));
	CommandsScroll->AddChild(Commands);
	CommandsFit->SetWidthOverride(260); CommandsFit->SetContent(CommandsScroll);
	auto* CommandsSlot = Footer->AddChildToHorizontalBox(CommandsFit);
	CommandsSlot->SetVerticalAlignment(VAlign_Fill); CommandsSlot->SetPadding(FMargin(5,0));

	// Modal outcome remains above the flex shell and never uses a hardcoded
	// hit-test rectangle. Its complete content scrolls in a proportional inset.
	auto* OutcomeCanvas = CastChecked<UCanvasPanel>(OutcomePanel->GetContent());
	auto* OutcomeBox = GetWidgetFromName(TEXT("OutcomeBox")); Move(OutcomeBox);
	auto* OutcomeScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResponsiveOutcomeScroll"));
	OutcomeScroll->AddChild(OutcomeBox);
	auto* OutcomeContentSlot = OutcomeCanvas->AddChildToCanvas(OutcomeScroll);
	OutcomeContentSlot->SetAnchors(FAnchors(0.15f,0.25f,0.85f,0.75f));
	OutcomeContentSlot->SetOffsets(FMargin(10));
	OutcomeText->SetWrapTextAt(0); OutcomeText->SetAutoWrapText(true);
	CastChecked<UCanvasPanelSlot>(OutcomePanel->Slot)->SetZOrder(100);
	auto* CrosshairSlot = CastChecked<UCanvasPanelSlot>(CrosshairText->Slot);
	CrosshairSlot->SetAutoSize(true);
	CrosshairSlot->SetZOrder(10);
}

void URomanEmpireMainWidget::HandleSettingsClicked()
{
	if(auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer())) Controller->OpenSettings();
}
