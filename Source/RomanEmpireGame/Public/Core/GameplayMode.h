// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/ZoomConfiguration.h"
#include "GameplayMode.generated.h"

/** Explicit gameplay authority mode. Possession is never inferred from zoom. */
UENUM(BlueprintType)
enum class EGameplayMode : uint8
{
	Strategic	UMETA(DisplayName = "Strategic"),
	Tactical	UMETA(DisplayName = "Tactical"),
	Possessed	UMETA(DisplayName = "Possessed")
};

namespace RomanEmpireGameplay
{
	ROMANEMPIREGAME_API bool IsExplorationMode(EGameplayMode Mode);
	ROMANEMPIREGAME_API EGameplayMode GetExplorationModeForZoomLevel(EZoomLevel ZoomLevel);
	ROMANEMPIREGAME_API const TCHAR* GetGameplayModeName(EGameplayMode Mode);
}
