// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRomanEmpire, Log, All);

// Game-wide constants
namespace RomanEmpireConstants
{
	// Movement speeds
	constexpr float UNIT_WALK_SPEED = 300.0f;
	constexpr float UNIT_RUN_SPEED = 600.0f;
	constexpr float CAVALRY_SPEED = 800.0f;

	// Combat
	constexpr float MELEE_RANGE = 200.0f;
	constexpr float RANGED_MAX_DISTANCE = 5000.0f;
}

