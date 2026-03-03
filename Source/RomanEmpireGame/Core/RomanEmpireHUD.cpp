// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireHUD.h"
#include "../RomanEmpireGame.h"
#include "../Units/UnitBase.h"
#include "RomanEmpireGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

ARomanEmpireHUD::ARomanEmpireHUD()
{
	bBuildingMenuVisible = false;
	bFPSMode = false;
	CurrentZoomLevel = 0.3f;

	DisplayGold = 0;
	DisplayFood = 0;
	DisplayIron = 0;
	DisplayWood = 0;
	DisplayStone = 0;
	DisplayPopulation = 0;

	CachedGameMode = nullptr;
}

void ARomanEmpireHUD::BeginPlay()
{
	Super::BeginPlay();
	
	CachedGameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
	
	UE_LOG(LogRomanEmpire, Log, TEXT("HUD initialized (Canvas-based, no Widget Blueprints needed)"));
}

void ARomanEmpireHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	DrawResourceBar();
	DrawPhaseIndicator();
	DrawControlsHelp();
	DrawUnitPanel();
	DrawSelectionBoxes();

	if (bBuildingMenuVisible)
	{
		DrawBuildingMenu();
	}

	DrawHealthBars();

	// Version display (bottom-right)
	UFont* Font = GEngine->GetSmallFont();
	if (Font)
	{
		DrawText(TEXT("v0.2.2"), FLinearColor(0.7f, 0.7f, 0.7f, 0.8f),
			Canvas->SizeX - 60.0f, Canvas->SizeY - 25.0f, Font);

		// Faction name (below resource bar)
		DrawText(TEXT("ROMAN EMPIRE"), FLinearColor(0.9f, 0.2f, 0.1f),
			10.0f, 30.0f, Font);

		// View mode label (right side, below resource bar)
		FString ViewLabel = CurrentZoomLevel <= 0.25f ? TEXT("STRATEGIC VIEW") : TEXT("TACTICAL VIEW");
		FLinearColor ViewColor = CurrentZoomLevel <= 0.25f ? FLinearColor(0.3f, 0.6f, 1.0f) : FLinearColor(0.2f, 0.8f, 0.3f);
		DrawText(ViewLabel, ViewColor, Canvas->SizeX - 160.0f, 30.0f, Font);
	}
}

void ARomanEmpireHUD::DrawResourceBar()
{
	const float BarHeight = 32.0f;
	const float ScreenWidth = Canvas->SizeX;
	
	// Background bar
	FLinearColor BarBG(0.05f, 0.05f, 0.1f, 0.85f);
	DrawRect(BarBG, 0, 0, ScreenWidth, BarHeight);

	// Resource text
	UFont* Font = GEngine->GetSmallFont();
	if (!Font) return;

	float X = 15.0f;
	float Y = 8.0f;
	float Spacing = 140.0f;

	// Gold
	DrawText(FString::Printf(TEXT("Gold: %d"), DisplayGold), FLinearColor(1.0f, 0.84f, 0.0f), X, Y, Font);
	X += Spacing;

	// Food
	DrawText(FString::Printf(TEXT("Food: %d"), DisplayFood), FLinearColor(0.4f, 0.8f, 0.2f), X, Y, Font);
	X += Spacing;

	// Iron
	DrawText(FString::Printf(TEXT("Iron: %d"), DisplayIron), FLinearColor(0.7f, 0.7f, 0.75f), X, Y, Font);
	X += Spacing;

	// Wood
	DrawText(FString::Printf(TEXT("Wood: %d"), DisplayWood), FLinearColor(0.6f, 0.4f, 0.2f), X, Y, Font);
	X += Spacing;

	// Stone
	DrawText(FString::Printf(TEXT("Stone: %d"), DisplayStone), FLinearColor(0.6f, 0.6f, 0.6f), X, Y, Font);
	X += Spacing;

	// Population
	DrawText(FString::Printf(TEXT("Pop: %d"), DisplayPopulation), FLinearColor(0.9f, 0.9f, 0.9f), X, Y, Font);
}

void ARomanEmpireHUD::DrawPhaseIndicator()
{
	UFont* Font = GEngine->GetSmallFont();
	if (!Font) return;

	FString PhaseText = TEXT("Strategic Phase");
	FLinearColor PhaseColor = FLinearColor::White;
	int32 Turn = 1;

	if (CachedGameMode)
	{
		switch (CachedGameMode->GetCurrentPhase())
		{
		case EGamePhase::Strategic:
			PhaseText = TEXT("STRATEGIC VIEW");
			PhaseColor = FLinearColor(0.3f, 0.6f, 1.0f);
			break;
		case EGamePhase::Tactical:
			PhaseText = TEXT("TACTICAL VIEW");
			PhaseColor = FLinearColor(1.0f, 0.8f, 0.2f);
			break;
		case EGamePhase::Combat:
			PhaseText = TEXT("COMBAT");
			PhaseColor = FLinearColor(1.0f, 0.2f, 0.2f);
			break;
		case EGamePhase::FirstPerson:
			PhaseText = TEXT("FIRST PERSON");
			PhaseColor = FLinearColor(0.2f, 1.0f, 0.2f);
			break;
		}
		Turn = CachedGameMode->GetCurrentTurn();
	}

	float RightX = Canvas->SizeX - 200.0f;
	// Phase background
	DrawRect(FLinearColor(0.05f, 0.05f, 0.1f, 0.85f), RightX, 40.0f, 190.0f, 50.0f);
	DrawText(PhaseText, PhaseColor, RightX + 10.0f, 45.0f, Font);
	DrawText(FString::Printf(TEXT("Turn: %d"), Turn), FLinearColor::White, RightX + 10.0f, 65.0f, Font);
}

void ARomanEmpireHUD::DrawControlsHelp()
{
	UFont* Font = GEngine->GetSmallFont();
	if (!Font) return;

	float X = 10.0f;
	float Y = Canvas->SizeY - 140.0f;
	FLinearColor HelpColor(0.7f, 0.7f, 0.7f, 0.7f);

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), X, Y - 5.0f, 220.0f, 135.0f);
	
	DrawText(TEXT("Controls:"), FLinearColor::White, X + 5.0f, Y, Font); Y += 16.0f;
	DrawText(TEXT("WASD / Edge - Move Camera"), HelpColor, X + 5.0f, Y, Font); Y += 14.0f;
	DrawText(TEXT("Mouse Wheel - Zoom"), HelpColor, X + 5.0f, Y, Font); Y += 14.0f;
	DrawText(TEXT("LMB - Select Unit"), HelpColor, X + 5.0f, Y, Font); Y += 14.0f;
	DrawText(TEXT("RMB - Move Selected"), HelpColor, X + 5.0f, Y, Font); Y += 14.0f;
	DrawText(TEXT("B - Building Menu"), HelpColor, X + 5.0f, Y, Font); Y += 14.0f;
	DrawText(TEXT("F - FPS Mode (select unit first)"), HelpColor, X + 5.0f, Y, Font); Y += 14.0f;
	DrawText(TEXT("T - End Turn"), HelpColor, X + 5.0f, Y, Font);
}

void ARomanEmpireHUD::DrawUnitPanel()
{
	if (CurrentSelectedUnits.Num() == 0) return;

	UFont* Font = GEngine->GetSmallFont();
	if (!Font) return;

	float PanelWidth = 250.0f;
	float PanelHeight = 80.0f;
	float X = (Canvas->SizeX - PanelWidth) / 2.0f;
	float Y = Canvas->SizeY - PanelHeight - 10.0f;

	// Background
	DrawRect(FLinearColor(0.05f, 0.05f, 0.1f, 0.85f), X, Y, PanelWidth, PanelHeight);

	if (CurrentSelectedUnits.Num() == 1)
	{
		AUnitBase* Unit = CurrentSelectedUnits[0];
		if (Unit)
		{
			DrawText(Unit->GetUnitDisplayName(), FLinearColor::White, X + 10.0f, Y + 5.0f, Font);
			
			// Health bar
			float HealthPct = Unit->GetHealthPercent();
			FLinearColor HealthColor = FMath::Lerp(FLinearColor::Red, FLinearColor::Green, HealthPct);
			DrawRect(FLinearColor(0.2f, 0.2f, 0.2f), X + 10.0f, Y + 25.0f, 230.0f, 12.0f);
			DrawRect(HealthColor, X + 10.0f, Y + 25.0f, 230.0f * HealthPct, 12.0f);
			DrawText(FString::Printf(TEXT("HP: %d"), Unit->GetCurrentHealth()), FLinearColor::White, X + 10.0f, Y + 40.0f, Font);
			DrawText(FString::Printf(TEXT("Morale: %d"), Unit->GetCurrentMorale()), FLinearColor::White, X + 130.0f, Y + 40.0f, Font);
		}
	}
	else
	{
		DrawText(FString::Printf(TEXT("%d units selected"), CurrentSelectedUnits.Num()), FLinearColor::White, X + 10.0f, Y + 10.0f, Font);
	}
}

void ARomanEmpireHUD::DrawBuildingMenu()
{
	UFont* Font = GEngine->GetSmallFont();
	if (!Font) return;

	float PanelWidth = 200.0f;
	float PanelHeight = 300.0f;
	float X = 10.0f;
	float Y = 50.0f;

	// Background
	DrawRect(FLinearColor(0.05f, 0.02f, 0.0f, 0.9f), X, Y, PanelWidth, PanelHeight);
	DrawText(TEXT("=== BUILDINGS ==="), FLinearColor(1.0f, 0.84f, 0.0f), X + 10.0f, Y + 10.0f, Font);

	float ItemY = Y + 35.0f;
	float ItemSpacing = 25.0f;

	// Building list (placeholder — could be made clickable with hit testing)
	DrawText(TEXT("[1] Barracks (200 Gold)"), FLinearColor::White, X + 15.0f, ItemY, Font); ItemY += ItemSpacing;
	DrawText(TEXT("[2] Farm (100 Gold)"), FLinearColor::White, X + 15.0f, ItemY, Font); ItemY += ItemSpacing;
	DrawText(TEXT("[3] Mine (150 Gold)"), FLinearColor::White, X + 15.0f, ItemY, Font); ItemY += ItemSpacing;
	DrawText(TEXT("[4] Lumber Mill (120 Gold)"), FLinearColor::White, X + 15.0f, ItemY, Font); ItemY += ItemSpacing;
	DrawText(TEXT("[5] Wall (80 Gold)"), FLinearColor::White, X + 15.0f, ItemY, Font); ItemY += ItemSpacing;
	DrawText(TEXT("[6] Temple (300 Gold)"), FLinearColor::White, X + 15.0f, ItemY, Font); ItemY += ItemSpacing;

	ItemY += 15.0f;
	DrawText(TEXT("Press B to close"), FLinearColor(0.6f, 0.6f, 0.6f), X + 15.0f, ItemY, Font);
}

void ARomanEmpireHUD::DrawMinimap()
{
	// TODO: Minimap rendering
}

void ARomanEmpireHUD::DrawSelectionBoxes()
{
	// Draw selection circles above selected units
	for (AUnitBase* Unit : CurrentSelectedUnits)
	{
		if (!Unit) continue;

		FVector WorldPos = Unit->GetActorLocation() + FVector(0, 0, 220.0f);
		FVector2D ScreenPos;
		if (GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
		{
			// Draw green selection indicator
			DrawRect(FLinearColor(0.0f, 1.0f, 0.0f, 0.7f), ScreenPos.X - 8.0f, ScreenPos.Y - 2.0f, 16.0f, 4.0f);
		}
	}
}

void ARomanEmpireHUD::ShowBuildingMenu()
{
	bBuildingMenuVisible = true;
}

void ARomanEmpireHUD::HideBuildingMenu()
{
	bBuildingMenuVisible = false;
}

void ARomanEmpireHUD::ToggleBuildingMenu()
{
	bBuildingMenuVisible = !bBuildingMenuVisible;
}

void ARomanEmpireHUD::UpdateUnitSelection(const TArray<AUnitBase*>& SelectedUnits)
{
	CurrentSelectedUnits = SelectedUnits;
}

void ARomanEmpireHUD::UpdateResources(int32 Gold, int32 Food, int32 Iron, int32 Wood, int32 Stone, int32 Population)
{
	DisplayGold = Gold;
	DisplayFood = Food;
	DisplayIron = Iron;
	DisplayWood = Wood;
	DisplayStone = Stone;
	DisplayPopulation = Population;
}

void ARomanEmpireHUD::OnZoomLevelChanged(float ZoomLevel)
{
	CurrentZoomLevel = ZoomLevel;
}

void ARomanEmpireHUD::DrawHealthBars()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	UFont* Font = GEngine->GetSmallFont();

	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		AUnitBase* Unit = Cast<AUnitBase>(Actor);
		if (!Unit || !Unit->IsAlive()) continue;

		// Project world location to screen
		FVector WorldPos = Unit->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f); // Above head
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
		{
			// Only draw if on-screen
			if (ScreenPos.X < 0 || ScreenPos.X > Canvas->SizeX ||
				ScreenPos.Y < 0 || ScreenPos.Y > Canvas->SizeY) continue;

			float BarWidth = 60.0f;
			float BarHeight = 6.0f;
			float BarX = ScreenPos.X - BarWidth / 2.0f;
			float BarY = ScreenPos.Y - BarHeight;

			float HealthPct = (float)Unit->GetCurrentHealth() / FMath::Max(1, Unit->GetMaxHealth());

			// Background (red)
			DrawRect(FLinearColor(0.6f, 0.0f, 0.0f, 0.8f), BarX, BarY, BarWidth, BarHeight);

			// Foreground (green)
			FLinearColor HealthColor = FLinearColor::LerpUsingHSV(
				FLinearColor(0.8f, 0.0f, 0.0f), FLinearColor(0.0f, 0.8f, 0.0f), HealthPct);
			DrawRect(HealthColor, BarX, BarY, BarWidth * HealthPct, BarHeight);

			// Border
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), BarX, BarY, BarWidth, 1.0f);
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), BarX, BarY + BarHeight - 1.0f, BarWidth, 1.0f);
		}
	}
}
