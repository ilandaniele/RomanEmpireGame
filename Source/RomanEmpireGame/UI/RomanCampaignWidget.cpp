// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanEmpireMainWidget.h"
#include "UI/RomanArmyButton.h"
#include "Blueprint/WidgetTree.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/RomanEmpireGameState.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UI/BuildingMenuWidget.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace RomanCampaignUI
{
	UTextBlock* Text(UWidgetTree* Tree, FName Name, const FText& Value, int32 Size = 14)
	{
		auto* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		auto Font = Widget->GetFont(); Font.Size = Size; Widget->SetFont(Font);
		Widget->SetText(Value); Widget->SetAutoWrapText(true); return Widget;
	}
	UButton* Button(UWidgetTree* Tree, FName Name, const FText& Value)
	{
		auto* Widget = Tree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), Name);
		Widget->SetContent(Text(Tree, FName(*(Name.ToString() + TEXT("Label"))), Value, 18));
		return Widget;
	}
	FText Failure(UCampaignSessionSubsystem* Session, ECampaignSessionFailure Result)
	{
		FString Message = FString::Printf(TEXT("Campaign command rejected: %s"), *StaticEnum<ECampaignSessionFailure>()->GetDisplayNameTextByValue(static_cast<int64>(Result)).ToString());
		if (Result == ECampaignSessionFailure::TransferRejected && Session)
			Message += TEXT(" — ") + StaticEnum<ECampaignTransferFailure>()->GetDisplayNameTextByValue(static_cast<int64>(Session->GetLastTransferFailure())).ToString();
		return FText::FromString(Message);
	}
}

void URomanEmpireMainWidget::BuildCampaignControls(UVerticalBox* Commands, UVerticalBox* Outcome, UVerticalBox* Details)
{
	using namespace RomanCampaignUI;
	EnterCampaignButton = Button(WidgetTree, TEXT("EnterCampaignButton"), NSLOCTEXT("RomanCampaign", "Enter", "ENTER CAMPAIGN"));
	EnterCampaignButton->SetToolTipText(NSLOCTEXT("RomanCampaign", "EnterHint", "Leave the sandbox and open a strategic session, retaining the canonical campaign. Outstanding paid orders are refunded before travel."));
	EnterCampaignButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleEnterCampaignClicked);
	Commands->AddChildToVerticalBox(EnterCampaignButton);
	ContinueCampaignButton = Button(WidgetTree, TEXT("ContinueCampaignButton"), NSLOCTEXT("RomanCampaign", "Continue", "CONTINUE TO CAMPAIGN"));
	ContinueCampaignButton->SetToolTipText(NSLOCTEXT("RomanCampaign", "ContinueHint", "Apply this battle's actual result once, settle paid orders, and return to the strategic campaign."));
	ContinueCampaignButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleContinueCampaignClicked);
	ContinueCampaignButton->SetVisibility(ESlateVisibility::Collapsed);
	Outcome->AddChildToVerticalBox(ContinueCampaignButton);
	ArmyList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CampaignArmyList"));
	ArmyList->SetOrientation(Orient_Horizontal);
	Details->AddChildToVerticalBox(Text(WidgetTree, TEXT("ArmyTitle"), NSLOCTEXT("RomanCampaign", "ArmyTitle", "ARMIES — SELECT, THEN CHOOSE A REGION")));
	Details->AddChildToVerticalBox(ArmyList);
	ArmyDetailText = Text(WidgetTree, TEXT("ArmyDetailText"), FText::GetEmpty());
	Details->AddChildToVerticalBox(ArmyDetailText);
	auto* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CampaignArmyActions"));
	MoveArmyButton = Button(WidgetTree, TEXT("MoveArmyButton"), NSLOCTEXT("RomanCampaign", "Move", "MOVE ARMY"));
	StartBattleButton = Button(WidgetTree, TEXT("StartBattleButton"), NSLOCTEXT("RomanCampaign", "Battle", "START BATTLE"));
	MoveArmyButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleMoveArmyClicked);
	StartBattleButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleStartBattleClicked);
	Actions->AddChildToHorizontalBox(MoveArmyButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Actions->AddChildToHorizontalBox(StartBattleButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Details->AddChildToVerticalBox(Actions);
	BuildCampaignArmyActionControls(Details);
	CampaignRecoveryPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignRecoveryPanel"));
	CampaignRecoveryPanel->SetBrushColor(FLinearColor(0.07f, 0.015f, 0.015f, 0.98f)); CampaignRecoveryPanel->SetPadding(FMargin(15));
	auto* Recovery = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CampaignRecoveryBox"));
	CampaignRecoveryText = Text(WidgetTree, TEXT("CampaignRecoveryText"), FText::GetEmpty(), 20);
	Recovery->AddChildToVerticalBox(CampaignRecoveryText);
	RetryCampaignButton = Button(WidgetTree, TEXT("RetryCampaignButton"), NSLOCTEXT("RomanCampaign", "Retry", "RETRY CAMPAIGN TRANSFER"));
	RetryCampaignButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleRetryCampaignClicked);
	Recovery->AddChildToVerticalBox(RetryCampaignButton); CampaignRecoveryPanel->SetContent(Recovery);
	auto* RecoverySlot = RootCanvas->AddChildToCanvas(CampaignRecoveryPanel);
	RecoverySlot->SetAnchors(FAnchors(0.5f, 0.5f)); RecoverySlot->SetAlignment(FVector2D(0.5f, 0.5f)); RecoverySlot->SetSize(FVector2D(560, 180)); RecoverySlot->SetZOrder(20);
	CampaignRecoveryPanel->SetVisibility(ESlateVisibility::Collapsed);
}

bool URomanEmpireMainWidget::IsLiveCampaignUI(bool bRequireReady) const
{
	return IsInViewport() && IsVisible() && IsValid(CachedController) && !CachedController->IsActorBeingDestroyed() &&
		CachedController->GetWorld() == GetWorld() && IsValid(CachedSession) &&
		CachedSession->GetGameInstance()->GetWorld() == GetWorld() &&
		(!bRequireReady || (bCanonicalEventsBound && IsValid(CachedGameState) && CachedGameState->IsGameplayInitialized()));
}
void URomanEmpireMainWidget::BindSessionEvents()
{
	BindSaveEvents();
	CachedSession = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>() : nullptr;
	if (CachedSession) CachedSession->OnSessionChanged.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCampaignSessionChanged);
}
void URomanEmpireMainWidget::UnbindSessionEvents()
{
	UnbindSaveEvents();
	if (CachedSession) CachedSession->OnSessionChanged.RemoveDynamic(this, &URomanEmpireMainWidget::HandleCampaignSessionChanged);
	CachedSession = nullptr;
	for (UButton* Button : {EnterCampaignButton.Get(), ContinueCampaignButton.Get(), RetryCampaignButton.Get(), MoveArmyButton.Get(), StartBattleButton.Get()})
		if (Button) Button->SetIsEnabled(false);
	for (URomanArmyButton* Button : ArmyButtons) Button->SetIsEnabled(false);
	if (CampaignRecoveryPanel) CampaignRecoveryPanel->SetVisibility(ESlateVisibility::Collapsed);
	RefreshCampaignArmyActions();
}
void URomanEmpireMainWidget::RefreshCampaignControls()
{
	RefreshSaveControls();
	const bool bLive = IsLiveCampaignUI();
	if (EnterCampaignButton)
	{
		EnterCampaignButton->SetVisibility(CachedSession && CachedSession->HasSession() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		EnterCampaignButton->SetIsEnabled(bLive && CachedController->CanEnterCampaignSession());
	}
	if (ContinueCampaignButton)
	{
		const bool bMappedTerminal = bLive && CachedSession->HasSession() && CachedSession->GetPhase() == ECampaignTransferPhase::TacticalDeployment && CachedGameState->GetTacticalBattleResult().IsResolved();
		ContinueCampaignButton->SetVisibility(bMappedTerminal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		ContinueCampaignButton->SetIsEnabled(bMappedTerminal && CachedController->CanContinueCampaign());
	}
	const bool bRecovery = IsLiveCampaignUI(false) && CachedSession->HasPendingTransfer() && CachedSession->GetLastFailure() == ECampaignSessionFailure::RestoreFailed;
	if (CampaignRecoveryPanel) CampaignRecoveryPanel->SetVisibility(bRecovery ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (RetryCampaignButton) RetryCampaignButton->SetIsEnabled(bRecovery && CachedController->CanRetryCampaignTransfer());
	if (bRecovery && CampaignRecoveryText)
		CampaignRecoveryText->SetText(FText::Format(NSLOCTEXT("RomanCampaign", "Recovery", "Campaign transfer blocked: {0}\nYour campaign is retained. Fix the cause, then retry."), StaticEnum<ECampaignTransferFailure>()->GetDisplayNameTextByValue(static_cast<int64>(CachedSession->GetLastTransferFailure()))));
}
bool URomanEmpireMainWidget::SelectCampaignArmy(FGuid ArmyID)
{
	FCampaignArmy Army;
	const bool bValid = IsLiveCampaignUI() && CachedGameState->GetCurrentGameplayMode() == EGameplayMode::Strategic && CachedGameState->TryGetCampaignArmy(ArmyID, Army);
	SelectedArmyID = bValid ? ArmyID : FGuid(); RefreshCampaignArmies(); return bValid;
}
void URomanEmpireMainWidget::RefreshCampaignArmies()
{
	if (!ArmyList || !ArmyDetailText) return;
	const bool bShow = IsLiveCampaignUI() && CachedGameState->GetCurrentGameplayMode() == EGameplayMode::Strategic;
	auto Armies = bShow ? CachedGameState->GetCampaignArmies() : TArray<FCampaignArmy>();
	// Snapshot GUID/map order is not a stable presentation order. Sort only this
	// UI copy; canonical ownership, selection IDs and command routing are unchanged.
	Armies.Sort([](const FCampaignArmy& A, const FCampaignArmy& B)
	{
		if (A.Faction != B.Faction) return static_cast<uint8>(A.Faction) < static_cast<uint8>(B.Faction);
		if (A.TerritoryID != B.TerritoryID) return A.TerritoryID.LexicalLess(B.TerritoryID);
		return A.ArmyID < B.ArmyID;
	});
	bool bSameIDs = ArmyButtons.Num() == Armies.Num();
	for (int32 I = 0; bSameIDs && I < Armies.Num(); ++I) bSameIDs = ArmyButtons[I]->GetArmyID() == Armies[I].ArmyID;
	if (!bSameIDs)
	{
		for (URomanArmyButton* Button : ArmyButtons) Button->OnArmyClicked.RemoveDynamic(this, &URomanEmpireMainWidget::HandleArmyClicked);
		ArmyList->ClearChildren(); ArmyButtons.Reset();
		for (const auto& Army : Armies)
		{
			const FString Key = Army.ArmyID.ToString(EGuidFormats::Digits);
			const FName ButtonName = MakeUniqueObjectName(WidgetTree, URomanArmyButton::StaticClass(), FName(*(TEXT("ArmyButton_") + Key)));
			auto* Button = WidgetTree->ConstructWidget<URomanArmyButton>(URomanArmyButton::StaticClass(), ButtonName);
			Button->InitializeArmyButton(Army.ArmyID); Button->OnArmyClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleArmyClicked);
			const FName LabelName = MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), FName(*(TEXT("ArmyLabel_") + Key)));
			Button->SetContent(RomanCampaignUI::Text(WidgetTree, LabelName, FText::GetEmpty()));
			ArmyList->AddChild(Button); ArmyButtons.Add(Button);
		}
		RefreshTextScale();
	}
	FCampaignArmy Selected;
	if (!bShow || !CachedGameState->TryGetCampaignArmy(SelectedArmyID, Selected)) SelectedArmyID.Invalidate();
	for (int32 I = 0; I < Armies.Num(); ++I)
	{
		int64 Count = 0; for (const auto& Stack : Armies[I].Units) Count += Stack.Count;
		const FText Faction = StaticEnum<EFactionID>()->GetDisplayNameTextByValue(static_cast<int64>(Armies[I].Faction));
		CastChecked<UTextBlock>(ArmyButtons[I]->GetContent())->SetText(FText::FromString(FString::Printf(TEXT("%s%s: %lld"), Armies[I].ArmyID == SelectedArmyID ? TEXT("* ") : TEXT(""), *Faction.ToString(), Count)));
		ArmyButtons[I]->SetIsEnabled(bShow && !(CachedSession && CachedSession->HasPendingTransfer()));
	}
	FString Detail = TEXT("Select an army and a target region.");
	if (SelectedArmyID.IsValid())
	{
		Detail = FString::Printf(TEXT("Army at %s\n"), *Selected.TerritoryID.ToString());
		for (const auto& Stack : Selected.Units) Detail += FString::Printf(TEXT("%s: %d  "), *Stack.UnitID.ToString(), Stack.Count);
		Detail += FString::Printf(TEXT("\nTarget: %s"), *SelectedTerritoryID.ToString());
	}
	ArmyDetailText->SetText(FText::FromString(Detail));
	MoveArmyButton->SetIsEnabled(bShow && CachedController->CanMoveCampaignArmy(SelectedArmyID, SelectedTerritoryID));
	StartBattleButton->SetIsEnabled(bShow && CachedController->CanStartCampaignBattle(SelectedArmyID, SelectedTerritoryID));
	RefreshCampaignArmyActions();
}
void URomanEmpireMainWidget::HandleCampaignSessionChanged()
{
	RefreshCampaignControls(); RefreshCampaignArmies(); RefreshCommands();
	if (CachedGameState) RefreshOutcome(CachedGameState->GetObjectiveState().Outcome);
}
void URomanEmpireMainWidget::HandleCampaignArmiesChanged() { RefreshCampaignArmies(); RefreshCommands(); }
void URomanEmpireMainWidget::HandleArmyClicked(FGuid ArmyID) { SelectCampaignArmy(ArmyID); }
void URomanEmpireMainWidget::HandleDiplomacyChanged(FDiplomaticPair Pair, EDiplomaticStatus Status) { RefreshCampaignArmies(); }
void URomanEmpireMainWidget::HandleEnterCampaignClicked()
{
	if (!IsLiveCampaignUI() || !CachedController->CanEnterCampaignSession()) return;
	const auto Result = CachedController->EnterCampaignSession();
	if (Result != ECampaignSessionFailure::None) HandleInteractionFeedback(RomanCampaignUI::Failure(CachedSession, Result));
	RefreshCampaignControls();
}
void URomanEmpireMainWidget::HandleContinueCampaignClicked()
{
	if (!IsLiveCampaignUI() || !CachedController->CanContinueCampaign()) return;
	const auto Result = CachedController->ContinueCampaign();
	if (Result != ECampaignSessionFailure::None) HandleInteractionFeedback(RomanCampaignUI::Failure(CachedSession, Result));
	RefreshCampaignControls();
}
void URomanEmpireMainWidget::HandleRetryCampaignClicked()
{
	if (!IsLiveCampaignUI(false) || !CachedController->CanRetryCampaignTransfer()) return;
	const auto Result = CachedController->RetryCampaignTransfer();
	if (Result != ECampaignSessionFailure::None) HandleInteractionFeedback(RomanCampaignUI::Failure(CachedSession, Result));
	RefreshCampaignControls();
}
void URomanEmpireMainWidget::HandleMoveArmyClicked()
{
	if (!IsLiveCampaignUI() || !CachedController->CanMoveCampaignArmy(SelectedArmyID, SelectedTerritoryID)) return;
	const auto Result = CachedController->RequestMoveCampaignArmy(SelectedArmyID, SelectedTerritoryID);
	if (!Result.WasSuccessful()) HandleInteractionFeedback(NSLOCTEXT("RomanCampaign", "MoveFailed", "Army move rejected; recheck the source and target."));
	RefreshCampaignArmies();
}
void URomanEmpireMainWidget::HandleStartBattleClicked()
{
	if (!IsLiveCampaignUI() || !CachedController->CanStartCampaignBattle(SelectedArmyID, SelectedTerritoryID)) return;
	const auto Result = CachedController->RequestStartCampaignBattle(SelectedArmyID, SelectedTerritoryID);
	if (Result != ECampaignSessionFailure::None) HandleInteractionFeedback(RomanCampaignUI::Failure(CachedSession, Result));
	RefreshCampaignControls(); RefreshCampaignArmies();
}
