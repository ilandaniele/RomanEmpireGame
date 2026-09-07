// Copyright Roman Empire Game. All Rights Reserved.

#include "RomanEmpireHUD.h"

#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Faction/FactionManager.h"
#include "Kismet/GameplayStatics.h"

ARomanEmpireHUD::ARomanEmpireHUD()
{
	bShowDebugOverlay = false;
	CachedGameMode = nullptr;
}

void ARomanEmpireHUD::BeginPlay()
{
	Super::BeginPlay();
	CachedGameMode = Cast<ARomanEmpireGameMode>(UGameplayStatics::GetGameMode(this));
}

void ARomanEmpireHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!bShowDebugOverlay || !Canvas || !GEngine)
	{
		return;
	}

	const AFactionManager* FactionManager = CachedGameMode ? CachedGameMode->GetFactionManager() : nullptr;
	const EFactionID PlayerFaction = FactionManager ? FactionManager->GetPlayerFaction() : EFactionID::None;
	const FFactionResourceAmounts Resources = FactionManager
		? FactionManager->GetFactionResources(PlayerFaction)
		: FFactionResourceAmounts();
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const EGameplayMode Mode = CachedGameMode
		? CachedGameMode->GetCurrentGameplayMode()
		: EGameplayMode::Tactical;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), 8.0f, 170.0f, 350.0f, 74.0f);
	UFont* Font = GEngine->GetSmallFont();
	if (!Font)
	{
		return;
	}

	DrawText(TEXT("CANVAS DEBUG (disabled by default)"), FLinearColor::Yellow, 16.0f, 177.0f, Font);
	DrawText(FString::Printf(TEXT("Authority: Mode=%s Turn=%d Gold=%d"),
		RomanEmpireGameplay::GetGameplayModeName(Mode),
		CachedGameMode ? CachedGameMode->GetCurrentTurn() : 0,
		Resources.Gold), FLinearColor::White, 16.0f, 197.0f, Font);
	DrawText(FString::Printf(TEXT("Definitions F/U/B/T=%d/%d/%d/%d"),
		Registry ? Registry->GetAllFactions().Num() : 0,
		Registry ? Registry->GetAllUnits().Num() : 0,
		Registry ? Registry->GetAllBuildings().Num() : 0,
		Registry ? Registry->GetAllTerritories().Num() : 0),
		FLinearColor(0.5f, 1.0f, 0.7f), 16.0f, 217.0f, Font);
}
