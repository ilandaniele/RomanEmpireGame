// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanEmpireMainWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/RomanEmpireGameState.h"
#include "Core/RomanEmpirePlayerController.h"
#include "UI/BuildingMenuWidget.h"

namespace RomanCampaignArmyUI
{
	UTextBlock* Text(UWidgetTree* Tree, FName Name, const FText& Value)
	{
		auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		auto Font = Label->GetFont(); Font.Size = 16; Label->SetFont(Font);
		Label->SetText(Value); Label->SetAutoWrapText(true); return Label;
	}
	UButton* Button(UWidgetTree* Tree, FName Name, const FText& Value)
	{
		auto* Control = Tree->ConstructWidget<URomanHUDButton>(URomanHUDButton::StaticClass(), Name);
		Control->SetContent(Text(Tree, FName(*(Name.ToString() + TEXT("Label"))), Value)); return Control;
	}
	FText Failure(ECampaignArmyFailure Value)
	{ return StaticEnum<ECampaignArmyFailure>()->GetDisplayNameTextByValue(static_cast<int64>(Value)); }
	void Label(UButton* Control, const FString& Value) { CastChecked<UTextBlock>(Control->GetContent())->SetText(FText::FromString(Value)); }
}

void URomanEmpireMainWidget::BuildCampaignArmyActionControls(UVerticalBox* Details)
{
	using namespace RomanCampaignArmyUI;
	CampaignArmyActionsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CampaignArmyActionsPanel"));
	Details->AddChildToVerticalBox(CampaignArmyActionsPanel);
	CampaignUnitChoiceButton = Button(WidgetTree, TEXT("CampaignUnitChoiceButton"), FText::GetEmpty());
	CampaignUnitChoiceButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCampaignUnitChoiceClicked);
	CampaignArmyActionsPanel->AddChildToVerticalBox(CampaignUnitChoiceButton);
	auto* Quantity = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CampaignActionQuantity"));
	CampaignCountDecreaseButton = Button(WidgetTree, TEXT("CampaignCountDecreaseButton"), NSLOCTEXT("RomanCampaign", "Decrease", "-"));
	CampaignCountIncreaseButton = Button(WidgetTree, TEXT("CampaignCountIncreaseButton"), NSLOCTEXT("RomanCampaign", "Increase", "+"));
	CampaignActionCountText = Text(WidgetTree, TEXT("CampaignActionCountText"), FText::GetEmpty());
	CampaignCountDecreaseButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCampaignCountDecreaseClicked);
	CampaignCountIncreaseButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleCampaignCountIncreaseClicked);
	Quantity->AddChildToHorizontalBox(CampaignCountDecreaseButton);
	Quantity->AddChildToHorizontalBox(CampaignActionCountText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Quantity->AddChildToHorizontalBox(CampaignCountIncreaseButton);
	CampaignArmyActionsPanel->AddChildToVerticalBox(Quantity);
	CampaignRecruitmentQuoteText = Text(WidgetTree, TEXT("CampaignRecruitmentQuoteText"), FText::GetEmpty());
	CampaignArmyActionsPanel->AddChildToVerticalBox(CampaignRecruitmentQuoteText);
	auto* Recruit = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CampaignRecruitmentButtons"));
	RecruitNewArmyButton = Button(WidgetTree, TEXT("RecruitNewArmyButton"), NSLOCTEXT("RomanCampaign", "RecruitNew", "RECRUIT NEW ARMY"));
	RecruitSelectedArmyButton = Button(WidgetTree, TEXT("RecruitSelectedArmyButton"), NSLOCTEXT("RomanCampaign", "RecruitSelected", "RECRUIT SELECTED"));
	RecruitNewArmyButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleRecruitNewArmyClicked);
	RecruitSelectedArmyButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleRecruitSelectedArmyClicked);
	Recruit->AddChildToHorizontalBox(RecruitNewArmyButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Recruit->AddChildToHorizontalBox(RecruitSelectedArmyButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CampaignArmyActionsPanel->AddChildToVerticalBox(Recruit);
	AssignmentTargetButton = Button(WidgetTree, TEXT("AssignmentTargetButton"), FText::GetEmpty());
	AssignArmyUnitsButton = Button(WidgetTree, TEXT("AssignArmyUnitsButton"), NSLOCTEXT("RomanCampaign", "Assign", "ASSIGN TO SHOWN ARMY"));
	AssignmentTargetButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleAssignmentTargetClicked);
	AssignArmyUnitsButton->OnClicked.AddUniqueDynamic(this, &URomanEmpireMainWidget::HandleAssignArmyUnitsClicked);
	CampaignArmyActionsPanel->AddChildToVerticalBox(AssignmentTargetButton);
	CampaignArmyActionsPanel->AddChildToVerticalBox(AssignArmyUnitsButton);
	CampaignArmyActionsPanel->SetVisibility(ESlateVisibility::Collapsed);
}

TArray<FGuid> URomanEmpireMainWidget::GetAssignmentTargetChoices() const
{
	TArray<FGuid> Choices; FCampaignArmy Source; FTerritoryPresentationSnapshot Region;
	if (!IsLiveCampaignUI() || !CachedGameState->TryGetCampaignArmy(SelectedArmyID, Source) ||
		Source.Faction != CachedGameState->GetPlayerFaction() || !CachedGameState->TryGetTerritorySnapshot(Source.TerritoryID, Region) || Region.OwnerFaction != Source.Faction) return Choices;
	for (const auto& Army : CachedGameState->GetCampaignArmies())
		if (Army.ArmyID != Source.ArmyID && Army.Faction == Source.Faction && Army.TerritoryID == Source.TerritoryID) Choices.Add(Army.ArmyID);
	return Choices; // Public snapshots already have stable GUID order.
}
bool URomanEmpireMainWidget::SetCampaignActionCount(int32 Count)
{
	if (!IsLiveCampaignUI() || CachedController->GetCampaignUnitChoices().IsEmpty() || Count < 1 || Count > MaxCampaignActionCount) return false;
	CampaignActionCount = Count; RefreshCampaignArmyActions(); return true;
}
bool URomanEmpireMainWidget::SelectCampaignActionUnit(FName UnitID)
{
	if (!IsLiveCampaignUI() || !CachedController->GetCampaignUnitChoices().Contains(UnitID)) return false;
	CampaignActionUnitID = UnitID; RefreshCampaignArmyActions(); return true;
}
void URomanEmpireMainWidget::RefreshCampaignArmyActions()
{
	using namespace RomanCampaignArmyUI;
	if (!CampaignArmyActionsPanel) return;
	const auto Units = IsLiveCampaignUI() ? CachedController->GetCampaignUnitChoices() : TArray<FName>();
	const bool bActive = !Units.IsEmpty();
	CampaignArmyActionsPanel->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bActive) { CampaignActionUnitID = NAME_None; CampaignActionCount = 1; AssignmentTargetArmyID.Invalidate(); }
	else if (!Units.Contains(CampaignActionUnitID)) CampaignActionUnitID = Units.Contains(TEXT("Legionary")) ? FName(TEXT("Legionary")) : Units[0];
	Label(CampaignUnitChoiceButton, FString::Printf(TEXT("UNIT: %s (NEXT)"), *CampaignActionUnitID.ToString()));
	CampaignUnitChoiceButton->SetIsEnabled(bActive && Units.Num() > 1);
	CampaignActionCountText->SetText(FText::Format(NSLOCTEXT("RomanCampaign", "BatchCount", "  Quantity: {0} (1-100)"), FText::AsNumber(CampaignActionCount)));
	CampaignCountDecreaseButton->SetIsEnabled(bActive && CampaignActionCount > 1);
	CampaignCountIncreaseButton->SetIsEnabled(bActive && CampaignActionCount < MaxCampaignActionCount);
	const auto NewQuote = bActive ? CachedController->GetCampaignRecruitmentQuote(SelectedTerritoryID, CampaignActionUnitID, CampaignActionCount, FGuid()) : FCampaignRecruitmentQuote();
	const auto SelectedQuote = bActive && SelectedArmyID.IsValid() ? CachedController->GetCampaignRecruitmentQuote(SelectedTerritoryID, CampaignActionUnitID, CampaignActionCount, SelectedArmyID) : FCampaignRecruitmentQuote();
	RecruitNewArmyButton->SetIsEnabled(NewQuote.IsAvailable());
	RecruitSelectedArmyButton->SetIsEnabled(SelectedArmyID.IsValid() && SelectedQuote.IsAvailable());
	FString QuoteText;
	if (bActive)
	{
		QuoteText = NewQuote.IsAvailable() || NewQuote.Failure == ECampaignArmyFailure::InsufficientResources
			? RomanEmpireEconomy::DescribeResourceCost(NewQuote.Cost) : Failure(NewQuote.Failure).ToString();
		if (!NewQuote.IsAvailable() && NewQuote.Failure == ECampaignArmyFailure::InsufficientResources) QuoteText += TEXT(" | ") + Failure(NewQuote.Failure).ToString();
	}
	CampaignRecruitmentQuoteText->SetText(FText::FromString(QuoteText));
	RecruitSelectedArmyButton->SetToolTipText(Failure(SelectedQuote.Failure));
	const auto Targets = bActive ? GetAssignmentTargetChoices() : TArray<FGuid>();
	if (!Targets.Contains(AssignmentTargetArmyID)) AssignmentTargetArmyID = Targets.IsEmpty() ? FGuid() : Targets[0];
	AssignmentTargetButton->SetIsEnabled(Targets.Num() > 1);
	FCampaignArmy Target;
	const bool bHasTarget = AssignmentTargetArmyID.IsValid() && CachedGameState->TryGetCampaignArmy(AssignmentTargetArmyID, Target);
	Label(AssignmentTargetButton, bHasTarget ? FString::Printf(TEXT("TO: %s #%s (%d) (NEXT)"), *Target.TerritoryID.ToString(), *Target.ArmyID.ToString(EGuidFormats::Digits).Left(8), FCampaignArmyState::GetUnitCount(Target)) : TEXT("TO: no other owned army in this region"));
	AssignArmyUnitsButton->SetIsEnabled(bActive && CachedController->CanAssignCampaignArmyUnits(SelectedArmyID, AssignmentTargetArmyID, CampaignActionUnitID, CampaignActionCount));
}

void URomanEmpireMainWidget::HandleCampaignUnitChoiceClicked()
{
	if (!IsLiveCampaignUI()) return;
	const auto Choices = CachedController->GetCampaignUnitChoices(); if (Choices.IsEmpty()) return;
	SelectCampaignActionUnit(Choices[(Choices.IndexOfByKey(CampaignActionUnitID) + 1) % Choices.Num()]);
}
void URomanEmpireMainWidget::HandleCampaignCountDecreaseClicked() { SetCampaignActionCount(CampaignActionCount - 1); }
void URomanEmpireMainWidget::HandleCampaignCountIncreaseClicked() { SetCampaignActionCount(CampaignActionCount + 1); }
void URomanEmpireMainWidget::HandleAssignmentTargetClicked()
{
	if (!IsLiveCampaignUI() || CachedController->GetCampaignUnitChoices().IsEmpty()) return;
	const auto Choices = GetAssignmentTargetChoices(); if (Choices.IsEmpty()) return;
	AssignmentTargetArmyID = Choices[(Choices.IndexOfByKey(AssignmentTargetArmyID) + 1) % Choices.Num()]; RefreshCampaignArmyActions();
}
void URomanEmpireMainWidget::ExecuteCampaignRecruitment(FGuid DestinationArmyID)
{
	if (!IsLiveCampaignUI() || !CachedController->GetCampaignRecruitmentQuote(SelectedTerritoryID, CampaignActionUnitID, CampaignActionCount, DestinationArmyID).IsAvailable()) return;
	const auto Result = CachedController->RequestRecruitCampaignUnits(SelectedTerritoryID, CampaignActionUnitID, CampaignActionCount, DestinationArmyID);
	if (Result.WasSuccessful()) SelectCampaignArmy(Result.ArmyID);
	else HandleInteractionFeedback(RomanCampaignArmyUI::Failure(Result.Failure));
	RefreshCampaignArmyActions();
}
void URomanEmpireMainWidget::HandleRecruitNewArmyClicked() { ExecuteCampaignRecruitment(FGuid()); }
void URomanEmpireMainWidget::HandleRecruitSelectedArmyClicked() { if (SelectedArmyID.IsValid()) ExecuteCampaignRecruitment(SelectedArmyID); }
void URomanEmpireMainWidget::HandleAssignArmyUnitsClicked()
{
	if (!IsLiveCampaignUI() || !CachedController->CanAssignCampaignArmyUnits(SelectedArmyID, AssignmentTargetArmyID, CampaignActionUnitID, CampaignActionCount)) return;
	const auto Result = CachedController->RequestAssignCampaignArmyUnits(SelectedArmyID, AssignmentTargetArmyID, CampaignActionUnitID, CampaignActionCount);
	if (Result.WasSuccessful() && !SelectedArmyID.IsValid()) SelectCampaignArmy(Result.ArmyID);
	else if (!Result.WasSuccessful()) HandleInteractionFeedback(RomanCampaignArmyUI::Failure(Result.Failure));
	RefreshCampaignArmyActions();
}
