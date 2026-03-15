// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireMainWidget.h"
#include "../RomanEmpireGame.h"
#include "../Units/UnitBase.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
// Automation Driver — stable widget IDs for E2E testing (unreal-engine-automation skill)
#include "AutomationDriver/Public/IDriverElement.h"
#include "AutomationDriver/Public/DriverConfiguration.h"

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

	// ── Automation IDs (unreal-engine-automation skill) ──────────────────────
	// Attach FDriverMetaData::Id to each Slate widget for By::Id selectors.
	// These IDs are used by test_romanempire_e2e.py and Automation Driver.
	auto SetId = [](UWidget* W, FName Id)
	{
		if (!W) return;
		TSharedPtr<SWidget> Slate = W->GetCachedWidget();
		if (Slate.IsValid())
		{
			Slate->AddMetadata(MakeShared<FDriverMetaData::Id>(Id));
		}
	};

	// Resource bar labels
	SetId(GoldText,       FName("lbl_gold"));
	SetId(FoodText,       FName("lbl_food"));
	SetId(IronText,       FName("lbl_iron"));
	SetId(WoodText,       FName("lbl_wood"));
	SetId(StoneText,      FName("lbl_stone"));
	SetId(PopulationText, FName("lbl_population"));

	// Building menu container
	SetId(BuildingMenuPanel, FName("panel_build_menu"));

	// Unit panel
	SetId(UnitPanel,     FName("panel_unit"));
	SetId(UnitNameText,  FName("lbl_unit_name"));
	SetId(UnitHealthBar, FName("bar_unit_health"));

	// FPS HUD elements
	SetId(FPSOverlay,    FName("overlay_fps"));
	SetId(CrosshairImage,FName("img_crosshair"));
	SetId(HealthBar,     FName("bar_health"));
	SetId(StaminaBar,    FName("bar_stamina"));

	// Minimap
	SetId(MinimapContainer, FName("panel_minimap"));
	SetId(MinimapImage,     FName("img_minimap"));

	// Strategic overlay
	SetId(StrategicOverlay, FName("overlay_strategic"));
	// ─────────────────────────────────────────────────────────────────────────

	UE_LOG(LogRomanEmpire, Log, TEXT("Main Widget constructed — Automation IDs assigned"));
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

