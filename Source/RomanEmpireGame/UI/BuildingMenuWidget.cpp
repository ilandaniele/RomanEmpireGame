// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingMenuWidget.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UBuildingMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind tab buttons
	if (MilitaryTabButton)
	{
		MilitaryTabButton->OnClicked.AddDynamic(this, &UBuildingMenuWidget::OnMilitaryTabClicked);
	}
	if (EconomicTabButton)
	{
		EconomicTabButton->OnClicked.AddDynamic(this, &UBuildingMenuWidget::OnEconomicTabClicked);
	}
	if (CivilTabButton)
	{
		CivilTabButton->OnClicked.AddDynamic(this, &UBuildingMenuWidget::OnCivilTabClicked);
	}
	if (DefenseTabButton)
	{
		DefenseTabButton->OnClicked.AddDynamic(this, &UBuildingMenuWidget::OnDefenseTabClicked);
	}

	CurrentCategory = EBuildingCategory::Military;
	PopulateBuildingList();
}

void UBuildingMenuWidget::PopulateBuildingList()
{
	// Create building entries - in a real implementation, this would
	// iterate over a data table of building definitions

	// Military
	FBuildingData BarracksData;
	BarracksData.BuildingType = EBuildingType::Barracks;
	BarracksData.Category = EBuildingCategory::Military;
	BarracksData.DisplayName = FText::FromString(TEXT("Barracks"));
	BarracksData.Cost.Gold = 400;
	BarracksData.Cost.Wood = 200;
	if (MilitaryBuildingsBox)
	{
		CreateBuildingButton(EBuildingType::Barracks, BarracksData, MilitaryBuildingsBox);
	}

	FBuildingData ArcheryData;
	ArcheryData.BuildingType = EBuildingType::ArcheryRange;
	ArcheryData.Category = EBuildingCategory::Military;
	ArcheryData.DisplayName = FText::FromString(TEXT("Archery Range"));
	ArcheryData.Cost.Gold = 350;
	ArcheryData.Cost.Wood = 250;
	if (MilitaryBuildingsBox)
	{
		CreateBuildingButton(EBuildingType::ArcheryRange, ArcheryData, MilitaryBuildingsBox);
	}

	// Economic
	FBuildingData FarmData;
	FarmData.BuildingType = EBuildingType::Farm;
	FarmData.Category = EBuildingCategory::Economic;
	FarmData.DisplayName = FText::FromString(TEXT("Farm"));
	FarmData.Cost.Gold = 100;
	FarmData.Cost.Wood = 50;
	if (EconomicBuildingsBox)
	{
		CreateBuildingButton(EBuildingType::Farm, FarmData, EconomicBuildingsBox);
	}

	// Civil
	FBuildingData ForumData;
	ForumData.BuildingType = EBuildingType::Forum;
	ForumData.Category = EBuildingCategory::Civil;
	ForumData.DisplayName = FText::FromString(TEXT("Forum"));
	ForumData.Cost.Gold = 500;
	ForumData.Cost.Stone = 300;
	if (CivilBuildingsBox)
	{
		CreateBuildingButton(EBuildingType::Forum, ForumData, CivilBuildingsBox);
	}

	// Defense
	FBuildingData WallData;
	WallData.BuildingType = EBuildingType::Wall;
	WallData.Category = EBuildingCategory::Defense;
	WallData.DisplayName = FText::FromString(TEXT("Wall"));
	WallData.Cost.Gold = 50;
	WallData.Cost.Stone = 100;
	if (DefenseBuildingsBox)
	{
		CreateBuildingButton(EBuildingType::Wall, WallData, DefenseBuildingsBox);
	}

	// Set initial visibility
	FilterByCategory(EBuildingCategory::Military);
}

void UBuildingMenuWidget::FilterByCategory(EBuildingCategory Category)
{
	CurrentCategory = Category;
	SetCategoryVisibility(Category);
}

void UBuildingMenuWidget::ShowAllBuildings()
{
	if (MilitaryBuildingsBox) MilitaryBuildingsBox->SetVisibility(ESlateVisibility::Visible);
	if (EconomicBuildingsBox) EconomicBuildingsBox->SetVisibility(ESlateVisibility::Visible);
	if (CivilBuildingsBox) CivilBuildingsBox->SetVisibility(ESlateVisibility::Visible);
	if (DefenseBuildingsBox) DefenseBuildingsBox->SetVisibility(ESlateVisibility::Visible);
}

void UBuildingMenuWidget::SetCategoryVisibility(EBuildingCategory Category)
{
	// Hide all
	if (MilitaryBuildingsBox) MilitaryBuildingsBox->SetVisibility(ESlateVisibility::Collapsed);
	if (EconomicBuildingsBox) EconomicBuildingsBox->SetVisibility(ESlateVisibility::Collapsed);
	if (CivilBuildingsBox) CivilBuildingsBox->SetVisibility(ESlateVisibility::Collapsed);
	if (DefenseBuildingsBox) DefenseBuildingsBox->SetVisibility(ESlateVisibility::Collapsed);

	// Show selected
	switch (Category)
	{
		case EBuildingCategory::Military:
			if (MilitaryBuildingsBox) MilitaryBuildingsBox->SetVisibility(ESlateVisibility::Visible);
			break;
		case EBuildingCategory::Economic:
			if (EconomicBuildingsBox) EconomicBuildingsBox->SetVisibility(ESlateVisibility::Visible);
			break;
		case EBuildingCategory::Civil:
			if (CivilBuildingsBox) CivilBuildingsBox->SetVisibility(ESlateVisibility::Visible);
			break;
		case EBuildingCategory::Defense:
			if (DefenseBuildingsBox) DefenseBuildingsBox->SetVisibility(ESlateVisibility::Visible);
			break;
	}
}

void UBuildingMenuWidget::CreateBuildingButton(EBuildingType BuildingType, const FBuildingData& Data, UVerticalBox* Container)
{
	// In a full implementation, this would instantiate the button template
	// and set up click handlers to broadcast OnBuildingSelected
	
	// The Blueprint version would create a button widget with:
	// - Icon image
	// - Name text
	// - Cost display
	// - Click handler calling OnBuildingSelected.Broadcast(BuildingType)
}

void UBuildingMenuWidget::OnMilitaryTabClicked()
{
	FilterByCategory(EBuildingCategory::Military);
}

void UBuildingMenuWidget::OnEconomicTabClicked()
{
	FilterByCategory(EBuildingCategory::Economic);
}

void UBuildingMenuWidget::OnCivilTabClicked()
{
	FilterByCategory(EBuildingCategory::Civil);
}

void UBuildingMenuWidget::OnDefenseTabClicked()
{
	FilterByCategory(EBuildingCategory::Defense);
}
