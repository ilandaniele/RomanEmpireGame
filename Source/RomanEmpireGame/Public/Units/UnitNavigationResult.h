// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UnitNavigationResult.generated.h"

/** Execution report, not a second owner of movement intent. */
UENUM(BlueprintType)
enum class EUnitNavigationResult : uint8
{
	None,
	Moving,
	Arrived,
	Unreachable
};
