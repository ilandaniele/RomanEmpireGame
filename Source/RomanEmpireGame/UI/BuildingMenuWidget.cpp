// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Input/RomanInputConfig.h"
#include "Input/RomanInputRemapping.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "RomanEmpireGame.h"
#include "Styling/SlateTypes.h"

URomanHUDButton::URomanHUDButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InitIsFocusable(false);

	// Opaque dark fills keep HUD labels readable over bright tactical terrain.
	// Retain the native button's padding, sounds, input, and accessibility behavior.
	FButtonStyle HUDStyle = GetStyle();
	HUDStyle.SetNormal(FSlateRoundedBoxBrush(
		FLinearColor(0.025f, 0.032f, 0.05f), 3.0f,
		FLinearColor(0.28f, 0.23f, 0.13f), 1.0f));
	HUDStyle.SetHovered(FSlateRoundedBoxBrush(
		FLinearColor(0.075f, 0.10f, 0.145f), 3.0f,
		FLinearColor(0.85f, 0.63f, 0.25f), 1.5f));
	HUDStyle.SetPressed(FSlateRoundedBoxBrush(
		FLinearColor(0.012f, 0.018f, 0.03f), 3.0f,
		FLinearColor(1.0f, 0.76f, 0.30f), 2.0f));
	HUDStyle.SetDisabled(FSlateRoundedBoxBrush(
		FLinearColor(0.055f, 0.055f, 0.06f), 3.0f,
		FLinearColor(0.12f, 0.12f, 0.13f), 1.0f));
	SetStyle(HUDStyle);
}

void URomanBuildingButton::InitializeBuildingButton(
	FName InBuildingID,
	EBuildingCategory InCategory)
{
	BuildingID = InBuildingID;
	BuildingCategory = InCategory;
	OnClicked.RemoveDynamic(this, &URomanBuildingButton::HandleClicked);
	OnClicked.AddDynamic(this, &URomanBuildingButton::HandleClicked);
}

void URomanBuildingButton::HandleClicked()
{
	if (!BuildingID.IsNone())
	{
		OnBuildingButtonClicked.Broadcast(BuildingID);
	}
}

void UBuildingMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildNativeWidgetTree();
}

void UBuildingMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildNativeWidgetTree();
	PopulateBuildingList();
}

TSharedRef<SWidget> UBuildingMenuWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void UBuildingMenuWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	BuildingListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildingList"));
	WidgetTree->RootWidget = BuildingListBox;
}

void UBuildingMenuWidget::PopulateBuildingList()
{
	if (!BuildingListBox)
	{
		return;
	}

	BuildingListBox->ClearChildren();
	BuildingButtons.Reset();

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!Registry || !Registry->IsReady())
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Building menu cannot populate before content registry initialization"));
		return;
	}

	TArray<const URomanBuildingDefinition*> Definitions = Registry->GetAllBuildings();
	Definitions.Sort([](const URomanBuildingDefinition& Left, const URomanBuildingDefinition& Right)
	{
		return Left.QuickSlot < Right.QuickSlot;
	});

	for (const URomanBuildingDefinition* Asset : Definitions)
	{
		if (Asset)
		{
			CreateBuildingButton(Asset->Definition.BuildingID, Asset->Definition, Asset->QuickSlot);
		}
	}
	ApplyFilter();
	RefreshShortcutLabels();
}

void UBuildingMenuWidget::CreateBuildingButton(
	FName BuildingID,
	const FBuildingData& Data,
	int32 QuickSlot)
{
	URomanBuildingButton* Button = WidgetTree->ConstructWidget<URomanBuildingButton>(
		URomanBuildingButton::StaticClass(),
		*FString::Printf(TEXT("Build_%s"), *BuildingID.ToString()));
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		*FString::Printf(TEXT("BuildLabel_%s"), *BuildingID.ToString()));
	if (!Button || !Label)
	{
		return;
	}

	Button->InitializeBuildingButton(BuildingID, Data.Category);
	Button->OnBuildingButtonClicked.AddUniqueDynamic(
		this, &UBuildingMenuWidget::HandleBuildingButtonClicked);
	Label->SetText(FText::FromString(FString::Printf(
		TEXT("[%d] %s  |  %s"),
		QuickSlot,
		*Data.DisplayName.ToString(),
		*RomanEmpireEconomy::DescribeResourceCost(Data.Cost))));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.90f, 0.75f)));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 14;
	Label->SetFont(Font);
	Label->SetAutoWrapText(true);
	Button->AddChild(Label);
	BuildingListBox->AddChildToVerticalBox(Button);
	BuildingButtons.Add(Button);
}

void UBuildingMenuWidget::RefreshShortcutLabels()
{
	const auto* Controller=GetOwningPlayer<ARomanEmpirePlayerController>();
	const auto* Config=Controller?Controller->GetInputConfiguration():nullptr;
	const auto* Subsystem=Controller?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()):nullptr;
	const auto* Registry=URomanContentRegistrySubsystem::Get(this);
	if(!Config || !Registry) return;
	for(URomanBuildingButton* Button:BuildingButtons)
	{
		const auto* Definition=Registry->FindBuilding(Button->GetBuildingID());
		if(!Definition || !Config->BuildingSlots.IsValidIndex(Definition->QuickSlot-1)) continue;
		const FString Key=RomanInputRemapping::DescribeKeys(Config->TacticalContext,Config->BuildingSlots[Definition->QuickSlot-1],Subsystem?Subsystem->GetUserSettings():nullptr);
		if(auto* Label=Cast<UTextBlock>(Button->GetContent())) Label->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s  |  %s"),*Key,*Definition->Definition.DisplayName.ToString(),*RomanEmpireEconomy::DescribeResourceCost(Definition->Definition.Cost))));
	}
}
void UBuildingMenuWidget::FilterByCategory(EBuildingCategory Category)
{
	CurrentCategory = Category;
	bShowAllCategories = false;
	ApplyFilter();
}

void UBuildingMenuWidget::ShowAllBuildings()
{
	bShowAllCategories = true;
	ApplyFilter();
}

void UBuildingMenuWidget::ApplyFilter()
{
	for (URomanBuildingButton* Button : BuildingButtons)
	{
		if (Button)
		{
			const bool bVisible = bShowAllCategories || Button->GetBuildingCategory() == CurrentCategory;
			Button->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UBuildingMenuWidget::HandleBuildingButtonClicked(FName BuildingID)
{
	OnBuildingSelected.Broadcast(BuildingID);
}
