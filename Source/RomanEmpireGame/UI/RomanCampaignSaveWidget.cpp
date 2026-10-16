// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanEmpireMainWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Campaign/CampaignSaveSubsystem.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/RomanEmpireGameState.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/BuildingMenuWidget.h"

void URomanEmpireMainWidget::BuildCampaignSaveControls()
{
	CampaignSavePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignSavePanel"));
	CampaignSavePanel->SetBrushColor(FLinearColor(0.025f, 0.03f, 0.055f, 0.95f)); CampaignSavePanel->SetPadding(FMargin(5));
	auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CampaignSaveBox"));
	auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CampaignSaveActions"));
	const auto MakeButton = [&](FName Name, const TCHAR* Value)
	{
		auto* Button = WidgetTree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), Name);
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name.ToString() + TEXT("Label"))));
		auto Font = Label->GetFont(); Font.Size = 16; Label->SetFont(Font); Label->SetText(FText::FromString(Value));
		Label->SetAutoWrapText(true); Label->SetJustification(ETextJustify::Center);
		Button->SetContent(Label); return Button;
	};
	SaveCampaignButton = MakeButton(TEXT("SaveCampaignButton"), TEXT("SAVE"));
	LoadCampaignButton = MakeButton(TEXT("LoadCampaignButton"), TEXT("LOAD"));
	LoadAutosaveButton = MakeButton(TEXT("LoadAutosaveButton"), TEXT("LOAD AUTO"));
	AutosaveToggleButton = MakeButton(TEXT("AutosaveToggleButton"), TEXT("AUTO ON"));
	SaveCampaignButton->SetToolTipText(FText::FromString(TEXT("Save this strategic campaign in the fixed Manual slot, retaining its previous generation.")));
	LoadCampaignButton->SetToolTipText(FText::FromString(TEXT("Load the fixed Manual slot. Confirmation is required before abandoning unsaved progress.")));
	LoadAutosaveButton->SetToolTipText(FText::FromString(TEXT("Load the most recent verified end-turn autosave; confirmation is required.")));
	AutosaveToggleButton->SetToolTipText(FText::FromString(TEXT("Enable/disable automatic local saving at the completed strategic turn boundary. Saved with the campaign.")));
	SaveCampaignButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleSaveCampaignClicked);
	LoadCampaignButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleLoadCampaignClicked);
	LoadAutosaveButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleLoadAutosaveClicked);
	AutosaveToggleButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleAutosaveToggleClicked);
	for (UButton* Button : {SaveCampaignButton.Get(), LoadCampaignButton.Get(), LoadAutosaveButton.Get(), AutosaveToggleButton.Get()})
		Row->AddChildToHorizontalBox(Button)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Box->AddChildToVerticalBox(Row);
	SaveConfirmationRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SaveConfirmationRow"));
	ConfirmSaveActionButton = MakeButton(TEXT("ConfirmSaveActionButton"), TEXT("CONFIRM"));
	CancelSaveActionButton = MakeButton(TEXT("CancelSaveActionButton"), TEXT("CANCEL"));
	ConfirmSaveActionButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleConfirmSaveActionClicked);
	CancelSaveActionButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCancelSaveActionClicked);
	SaveConfirmationRow->AddChildToHorizontalBox(ConfirmSaveActionButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SaveConfirmationRow->AddChildToHorizontalBox(CancelSaveActionButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Box->AddChildToVerticalBox(SaveConfirmationRow);
	CampaignSaveStatus = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CampaignSaveStatus"));
	auto Font = CampaignSaveStatus->GetFont(); Font.Size = 14; CampaignSaveStatus->SetFont(Font); CampaignSaveStatus->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(CampaignSaveStatus); CampaignSavePanel->SetContent(Box);
	auto* SaveCanvasSlot = RootCanvas->AddChildToCanvas(CampaignSavePanel);
	SaveCanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f)); SaveCanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	SaveCanvasSlot->SetPosition(FVector2D(0, -12)); SaveCanvasSlot->SetSize(FVector2D(620, 110)); SaveCanvasSlot->SetZOrder(5);
	CampaignSavePanel->SetVisibility(ESlateVisibility::Collapsed);
}

void URomanEmpireMainWidget::BindSaveEvents()
{
	CachedSave = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>() : nullptr;
	if (CachedSave) CachedSave->OnSaveStateChanged.AddUniqueDynamic(this, &URomanEmpireMainWidget::ScheduleSaveRefresh);
}

void URomanEmpireMainWidget::UnbindSaveEvents()
{
	if (CachedSave) CachedSave->OnSaveStateChanged.RemoveDynamic(this, &URomanEmpireMainWidget::ScheduleSaveRefresh);
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(SaveRefreshTimer);
	SaveRefreshTimer.Invalidate(); CachedSave = nullptr; PendingSaveAction = 0;
	if (CampaignSavePanel) CampaignSavePanel->SetVisibility(ESlateVisibility::Collapsed);
	for (UButton* Button : {SaveCampaignButton.Get(), LoadCampaignButton.Get(), LoadAutosaveButton.Get(), AutosaveToggleButton.Get(), ConfirmSaveActionButton.Get(), CancelSaveActionButton.Get()})
		if (Button) Button->SetIsEnabled(false);
}

bool URomanEmpireMainWidget::IsLiveSaveUI() const
{
	return IsLiveCampaignUI() && CachedController->HasActorBegunPlay() && CachedController->IsLocalController() &&
		IsValid(CachedSave) && CachedSave->GetGameInstance()->GetWorld() == GetWorld() &&
		CachedGameState->GetCurrentGameplayMode() != EGameplayMode::Possessed &&
		(!CachedSession->HasSession() || CachedSession->GetPhase() == ECampaignTransferPhase::Strategic);
}

void URomanEmpireMainWidget::ScheduleSaveRefresh()
{
	if (!GetWorld() || SaveRefreshTimer.IsValid()) return;
	// Exactly one coalesced event-driven refresh after operation/turn guards release. Not UI Tick/polling.
	SaveRefreshTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{ SaveRefreshTimer.Invalidate(); RefreshSaveControls(); }));
}

void URomanEmpireMainWidget::RefreshSaveControls()
{
	if (!CampaignSavePanel || !CampaignSaveStatus) return;
	const bool bLive = IsLiveSaveUI();
	const bool bSave = bLive && CachedSave->CanSaveCampaign();
	const bool bManual = bLive && CachedSave->CanLoadCampaign();
	const bool bAuto = bLive && CachedSave->CanLoadCampaign(ECampaignSaveSlot::Autosave);
	CampaignSavePanel->SetVisibility(bLive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	// The responsive header owns layout in both sandbox and campaign; no
	// fixed Canvas repositioning, nor overlap with bottom selection/production.
	if (PendingSaveAction && (!bLive || PendingSaveTurn != CachedGameState->GetCurrentTurn() ||
		(PendingSaveAction == 1 ? !bSave : PendingSaveAction == 2 ? !bManual : !bAuto))) PendingSaveAction = 0;
	SaveCampaignButton->SetIsEnabled(bSave && !PendingSaveAction);
	LoadCampaignButton->SetIsEnabled(bManual && !PendingSaveAction);
	LoadAutosaveButton->SetIsEnabled(bAuto && !PendingSaveAction);
	AutosaveToggleButton->SetIsEnabled(bLive && !CachedSession->HasPendingTransfer() && !PendingSaveAction);
	SaveConfirmationRow->SetVisibility(PendingSaveAction ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ConfirmSaveActionButton->SetIsEnabled(PendingSaveAction != 0); CancelSaveActionButton->SetIsEnabled(PendingSaveAction != 0);
	if (!bLive) return;
	CastChecked<UTextBlock>(AutosaveToggleButton->GetContent())->SetText(FText::FromString(CachedSave->GetSettings().bAutosaveEnabled ? TEXT("AUTO ON") : TEXT("AUTO OFF")));
	if (PendingSaveAction)
		CampaignSaveStatus->SetText(FText::FromString(PendingSaveAction == 1 ? TEXT("Overwrite Manual save? The previous generation is retained.") :
			PendingSaveAction == 2 ? TEXT("Load Manual save? Unsaved campaign progress will be discarded.") : TEXT("Load Autosave? Unsaved campaign progress will be discarded.")));
	else if (CachedSave->GetLastFailure() != ECampaignSaveFailure::None)
		CampaignSaveStatus->SetText(FText::Format(NSLOCTEXT("RomanSave", "Failure", "Local save/load: {0}"), StaticEnum<ECampaignSaveFailure>()->GetDisplayNameTextByValue(static_cast<int64>(CachedSave->GetLastFailure()))));
	else if (CachedSession->HasPendingTransfer()) CampaignSaveStatus->SetText(NSLOCTEXT("RomanSave", "Pending", "Campaign transfer pending; restoration is not yet complete."));
	else
		CampaignSaveStatus->SetText(FText::Format(NSLOCTEXT("RomanSave", "Status", "{2}Local saves | Last save/load turn: {0}{1}"), FText::AsNumber(CachedSave->GetLastSavedTurn()),
			CachedSave->WasBackupRecovered() ? NSLOCTEXT("RomanSave", "Recovered", " | Backup recovered") : FText::GetEmpty(), CampaignSaveNotice));
}

void URomanEmpireMainWidget::RequestSaveUIAction(int32 Action)
{
	if (!IsLiveSaveUI() || PendingSaveAction || Action < 1 || Action > 3) return;
	if (Action == 1 ? !CachedSave->CanSaveCampaign() : !CachedSave->CanLoadCampaign(Action == 2 ? ECampaignSaveSlot::Manual : ECampaignSaveSlot::Autosave)) return;
	PendingSaveAction = Action; PendingSaveTurn = CachedGameState->GetCurrentTurn();
	if (Action == 1 && !CachedSave->CanLoadCampaign()) HandleConfirmSaveActionClicked(); // First save needs no overwrite confirmation.
	else RefreshSaveControls();
}
void URomanEmpireMainWidget::HandleSaveCampaignClicked() { RequestSaveUIAction(1); }
void URomanEmpireMainWidget::HandleLoadCampaignClicked() { RequestSaveUIAction(2); }
void URomanEmpireMainWidget::HandleLoadAutosaveClicked() { RequestSaveUIAction(3); }
void URomanEmpireMainWidget::HandleConfirmSaveActionClicked()
{
	if (!IsLiveSaveUI() || !PendingSaveAction || PendingSaveTurn != CachedGameState->GetCurrentTurn()) return;
	const int32 Action = PendingSaveAction; PendingSaveAction = 0;
	const auto Failure = Action == 1 ? CachedSave->SaveCampaign() : CachedSave->LoadCampaign(Action == 2 ? ECampaignSaveSlot::Manual : ECampaignSaveSlot::Autosave);
	if (Failure == ECampaignSaveFailure::None)
		CampaignSaveNotice = Action == 1 ? NSLOCTEXT("RomanSave", "Saved", "Manual saved. ") : FText::GetEmpty();
	RefreshSaveControls();
}
void URomanEmpireMainWidget::HandleCancelSaveActionClicked() { if (IsLiveSaveUI()) { PendingSaveAction = 0; RefreshSaveControls(); } }
void URomanEmpireMainWidget::HandleAutosaveToggleClicked()
{
	if (IsLiveSaveUI() && !PendingSaveAction) CachedSave->SetAutosaveEnabled(!CachedSave->GetSettings().bAutosaveEnabled);
	RefreshSaveControls();
}
