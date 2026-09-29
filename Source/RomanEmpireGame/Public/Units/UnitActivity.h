// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitActivity.generated.h"

/** Read projection, never a separately stored state machine.
 * Attacking includes both melee windup and recovery; a pending impact is consumed once.
 * Routed gameplay policy is intentionally not implied by zero morale.
 */
UENUM(BlueprintType)
enum class EUnitActivity : uint8
{
	Unavailable,
	Idle,
	Commanded,
	Attacking,
	Dead,
	Ended
};
