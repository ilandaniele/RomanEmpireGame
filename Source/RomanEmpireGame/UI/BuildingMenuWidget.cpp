// Copyright Roman Empire Game. All Rights Reserved.

#include "BuildingMenuWidget.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
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
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	if (!Registry || !Registry->IsReady())
	{
		return;
	}

	for (const URomanBuildingDefinition* Asset : Registry->GetAllBuildings())
	{
		if (!Asset)
		{
			continue;
		}

		UVerticalBox* Container = nullptr;
		switch (Asset->Definition.Category)
		{
		case EBuildingCategory::Military: Container = MilitaryBuildingsBox; break;
		case EBuildingCategory::Economic: Container = EconomicBuildingsBox; break;
		case EBuildingCategory::Civil: Container = CivilBuildingsBox; break;
		case EBuildingCategory::Defense: Container = DefenseBuildingsBox; break;
		default: break;
		}
		if (Container)
		{
			CreateBuildingButton(Asset->Definition.BuildingID, Asset->Definition, Container);
		}
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

void UBuildingMenuWidget::CreateBuildingButton(FName BuildingID, const FBuildingData& Data, UVerticalBox* Container)
{
	// In a full implementation, this would instantiate the button template
	// and set up click handlers to broadcast OnBuildingSelected
	
	// The Blueprint version would create a button widget with:
	// - Icon image
	// - Name text
	// - Cost display
	// - Click handler calling OnBuildingSelected.Broadcast(BuildingID)
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

