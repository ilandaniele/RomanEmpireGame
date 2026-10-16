// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitActivity.generated.h"

/** Read projection, never a separately stored state machine.
 * Attacking includes both melee windup and recovery; a pending impact is consumed once.
 * Routed units remain alive/vulnerable but reject command and combat initiation.
 */
UENUM(BlueprintType)
enum class EUnitActivity : uint8
{
	Unavailable,
	Idle,
	Commanded,
	Attacking,
	Dead,
	Ended,
	Routed
};
