// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

struct FBuildingData;

namespace RomanEmpireBuildingPresentation
{
// Existing placement box contract; presentation never changes its dimensions.
inline constexpr float PlacementHalfHeight = 200.0f;
enum class EFailure : uint8 { None, InvalidTransform, InvalidGeometry, OutsideFootprint, NotGrounded };

/** Pure inspection, no loading/mutation. A null optional mesh preserves legacy behaviour. */
ROMANEMPIREGAME_API EFailure Validate(const FBuildingData& Data);
}
