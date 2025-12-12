// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireHUD.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/UI/RomanEmpireMainWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

ARomanEmpireHUD::ARomanEmpireHUD()
{
	bBuildingMenuVisible = false;
	bFPSMode = false;
	CurrentZoomLevel = 0.3f;
	MainWidget = nullptr;
}

void ARomanEmpireHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateWidgets();
}

void ARomanEmpireHUD::DrawHUD()
{
	Super::DrawHUD();

	// Custom HUD drawing (legacy style) can go here
	// Selection boxes, health bars over units, etc.
}

void ARomanEmpireHUD::CreateWidgets()
{
	if (MainWidgetClass)
	{
		MainWidget = CreateWidget<URomanEmpireMainWidget>(GetOwningPlayerController(), MainWidgetClass);
		if (MainWidget)
		{
			MainWidget->AddToViewport(0);
			UE_LOG(LogRomanEmpire, Log, TEXT("Main HUD widget created"));
		}
	}
	else
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("MainWidgetClass not set in HUD"));
	}
}

void ARomanEmpireHUD::ShowBuildingMenu()
{
	bBuildingMenuVisible = true;
	if (MainWidget)
	{
		MainWidget->ShowBuildingMenu();
	}
}

void ARomanEmpireHUD::HideBuildingMenu()
{
	bBuildingMenuVisible = false;
	if (MainWidget)
	{
		MainWidget->HideBuildingMenu();
	}
}

void ARomanEmpireHUD::ToggleBuildingMenu()
{
	if (bBuildingMenuVisible)
	{
		HideBuildingMenu();
	}
	else
	{
		ShowBuildingMenu();
	}
}

void ARomanEmpireHUD::UpdateUnitSelection(const TArray<AUnitBase*>& SelectedUnits)
{
	if (MainWidget)
	{
		MainWidget->UpdateUnitPanel(SelectedUnits);
	}
}

void ARomanEmpireHUD::ShowFPSOverlay(bool bShow)
{
	bFPSMode = bShow;
	if (MainWidget)
	{
		MainWidget->SetFPSMode(bShow);
	}
}

void ARomanEmpireHUD::UpdateResources(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population)
{
	if (MainWidget)
	{
		MainWidget->UpdateResourceDisplay(Gold, Food, Iron, Wood, Stone, Population);
	}
}

void ARomanEmpireHUD::OnZoomLevelChanged(float ZoomLevel)
{
	CurrentZoomLevel = ZoomLevel;
	UpdateUIVisibility();
}

void ARomanEmpireHUD::UpdateUIVisibility()
{
	if (!MainWidget)
	{
		return;
	}

	// Show/hide different UI elements based on zoom level
	bool bShowStrategicUI = CurrentZoomLevel < 0.4f;
	bool bShowTacticalUI = CurrentZoomLevel >= 0.4f && CurrentZoomLevel < 0.8f;
	bool bShowFPSUI = CurrentZoomLevel >= 0.8f || bFPSMode;

	MainWidget->SetStrategicUIVisible(bShowStrategicUI);
	MainWidget->SetTacticalUIVisible(bShowTacticalUI);
	MainWidget->SetFPSUIVisible(bShowFPSUI);
}
