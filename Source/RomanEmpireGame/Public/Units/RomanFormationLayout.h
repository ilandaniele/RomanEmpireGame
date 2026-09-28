// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

/** Stateless selection-level layout. Actor orders remain the sole execution owner. */
struct ROMANEMPIREGAME_API FRomanFormationLayout
{
	/** Centered ranks facing travel, with spatial assignment to limit crossing.
	 * Output follows input member order. Invalid input leaves OutDestinations unchanged.
	 * No implicit squad expansion, world queries, persistent identity or per-frame planning.
	 */
	static bool Build(TConstArrayView<FVector> Locations, const FVector& Destination,
		double Spacing, TArray<FVector>& OutDestinations);
};
