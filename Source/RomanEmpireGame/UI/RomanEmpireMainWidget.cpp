// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireMainWidget.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/Units/UnitBase.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"

void URomanEmpireMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bFPSModeActive = false;

	// Hide building menu initially
	if (BuildingMenuPanel)
	{
		BuildingMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Hide FPS overlay initially
	if (FPSOverlay)
	{
		FPSOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetupBuildingButtons();

	UE_LOG(LogRomanEmpire, Log, TEXT("Main Widget constructed"));
}

void URomanEmpireMainWidget::ShowBuildingMenu()
{
	if (BuildingMenuPanel)
	{
		BuildingMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void URomanEmpireMainWidget::HideBuildingMenu()
{
	if (BuildingMenuPanel)
	{
		BuildingMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::UpdateUnitPanel(const TArray<AUnitBase*>& SelectedUnits)
{
	if (!UnitPanel)
	{
		return;
	}

	if (SelectedUnits.Num() == 0)
	{
		UnitPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UnitPanel->SetVisibility(ESlateVisibility::Visible);

	// Show first selected unit's info
	AUnitBase* FirstUnit = SelectedUnits[0];
	if (FirstUnit)
	{
		if (UnitNameText)
		{
			UnitNameText->SetText(FirstUnit->GetUnitData().DisplayName);
		}

		if (UnitHealthBar)
		{
			UnitHealthBar->SetPercent(FirstUnit->GetHealthPercent());
		}
	}
}

void URomanEmpireMainWidget::UpdateResourceDisplay(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population)
{
	if (GoldText)
	{
		GoldText->SetText(FText::AsNumber(Gold));
	}
	if (FoodText)
	{
		FoodText->SetText(FText::AsNumber(Food));
	}
	if (IronText)
	{
		IronText->SetText(FText::AsNumber(Iron));
	}
	if (WoodText)
	{
		WoodText->SetText(FText::AsNumber(Wood));
	}
	if (StoneText)
	{
		StoneText->SetText(FText::AsNumber(Stone));
	}
	if (PopulationText)
	{
		PopulationText->SetText(FText::AsNumber(Population));
	}
}

void URomanEmpireMainWidget::SetFPSMode(bool bEnabled)
{
	bFPSModeActive = bEnabled;

	if (FPSOverlay)
	{
		FPSOverlay->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Hide RTS UI in FPS mode
	if (BuildingMenuPanel)
	{
		BuildingMenuPanel->SetVisibility(bEnabled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (UnitPanel)
	{
		UnitPanel->SetVisibility(bEnabled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void URomanEmpireMainWidget::SetStrategicUIVisible(bool bVisible)
{
	if (StrategicOverlay)
	{
		StrategicOverlay->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::SetTacticalUIVisible(bool bVisible)
{
	// Tactical mode shows building menu and unit panel
	if (BuildingMenuPanel && !bFPSModeActive)
	{
		// Don't show if explicitly hidden
	}
	if (UnitPanel && !bFPSModeActive)
	{
		UnitPanel->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::SetFPSUIVisible(bool bVisible)
{
	if (FPSOverlay)
	{
		FPSOverlay->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Show crosshair in FPS mode
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void URomanEmpireMainWidget::UpdateMinimap()
{
	// Minimap rendering would be done via render target
	// This is a placeholder for the Blueprint implementation
}

void URomanEmpireMainWidget::SetupBuildingButtons()
{
	// Building buttons would be created dynamically or bound from Blueprint
	// This sets up click handlers for each building type
}
