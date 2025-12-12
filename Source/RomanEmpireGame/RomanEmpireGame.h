// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRomanEmpire, Log, All);

// Game-wide constants
namespace RomanEmpireConstants
{
	// Zoom levels (0.0 to 1.0)
	constexpr float ZOOM_WORLD_MIN = 0.0f;
	constexpr float ZOOM_WORLD_MAX = 0.2f;
	constexpr float ZOOM_TERRITORY_MAX = 0.4f;
	constexpr float ZOOM_CITY_MAX = 0.6f;
	constexpr float ZOOM_GROUND_MAX = 0.8f;
	constexpr float ZOOM_FPS_MAX = 1.0f;

	// Camera heights per zoom level
	constexpr float CAMERA_HEIGHT_WORLD = 50000.0f;     // 500 meters
	constexpr float CAMERA_HEIGHT_TERRITORY = 20000.0f; // 200 meters
	constexpr float CAMERA_HEIGHT_CITY = 5000.0f;       // 50 meters
	constexpr float CAMERA_HEIGHT_GROUND = 500.0f;      // 5 meters
	constexpr float CAMERA_HEIGHT_FPS = 180.0f;         // 1.8 meters (eye level)

	// Movement speeds
	constexpr float UNIT_WALK_SPEED = 300.0f;
	constexpr float UNIT_RUN_SPEED = 600.0f;
	constexpr float CAVALRY_SPEED = 800.0f;

	// Combat
	constexpr float MELEE_RANGE = 200.0f;
	constexpr float RANGED_MAX_DISTANCE = 5000.0f;
}
