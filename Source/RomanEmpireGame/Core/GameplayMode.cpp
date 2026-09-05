// Copyright Roman Empire Game. All Rights Reserved.

#include "Core/GameplayMode.h"

bool RomanEmpireGameplay::IsExplorationMode(EGameplayMode Mode)
{
	return Mode == EGameplayMode::Strategic || Mode == EGameplayMode::Tactical;
}

EGameplayMode RomanEmpireGameplay::GetExplorationModeForZoomLevel(EZoomLevel ZoomLevel)
{
	return ZoomLevel == EZoomLevel::World || ZoomLevel == EZoomLevel::Territory
		? EGameplayMode::Strategic
		: EGameplayMode::Tactical;
}

const TCHAR* RomanEmpireGameplay::GetGameplayModeName(EGameplayMode Mode)
{
	switch (Mode)
	{
	case EGameplayMode::Strategic: return TEXT("Strategic");
	case EGameplayMode::Tactical: return TEXT("Tactical");
	case EGameplayMode::Possessed: return TEXT("Possessed");
	default: return TEXT("Unknown");
	}
}
