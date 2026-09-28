// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/RomanFormationLayout.h"
#include "Algo/Sort.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
	}
}

bool FRomanFormationLayout::Build(TConstArrayView<FVector> Locations, const FVector& Destination,
	double Spacing, TArray<FVector>& OutDestinations)
{
	if (Locations.IsEmpty() || !IsFiniteVector(Destination) || !FMath::IsFinite(Spacing) || Spacing <= 0) return false;
	FVector Center = FVector::ZeroVector;
	for (const FVector& Location : Locations)
	{
		if (!IsFiniteVector(Location)) return false;
		Center += Location / Locations.Num();
	}
	const FVector Delta(Destination.X - Center.X, Destination.Y - Center.Y, 0);
	if (!IsFiniteVector(Center) || !IsFiniteVector(Delta) || !FMath::IsFinite(Delta.SizeSquared())) return false;
	const FVector Forward = Delta.IsNearlyZero() ? FVector::ForwardVector : Delta.GetSafeNormal();
	const FVector Right(-Forward.Y, Forward.X, 0);
	for (const FVector& Location : Locations)
		if (!FMath::IsFinite(FVector::DotProduct(Location, Forward)) || !FMath::IsFinite(FVector::DotProduct(Location, Right))) return false;
	const int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<double>(Locations.Num())));
	TArray<int32> Members;
	for (int32 Index = 0; Index < Locations.Num(); ++Index) Members.Add(Index);
	Members.Sort([&](int32 A, int32 B)
	{
		const double AheadA = FVector::DotProduct(Locations[A], Forward);
		const double AheadB = FVector::DotProduct(Locations[B], Forward);
		return AheadA == AheadB ? A < B : AheadA > AheadB;
	});
	TArray<FVector> Slots;
	Slots.SetNumUninitialized(Locations.Num());
	FVector MeanOffset = FVector::ZeroVector;
	for (int32 First = 0, Row = 0; First < Members.Num(); First += Columns, ++Row)
	{
		const int32 Width = FMath::Min(Columns, Members.Num() - First);
		Algo::Sort(MakeArrayView(Members.GetData() + First, Width), [&](int32 A, int32 B)
		{
			const double SideA = FVector::DotProduct(Locations[A], Right);
			const double SideB = FVector::DotProduct(Locations[B], Right);
			return SideA == SideB ? A < B : SideA < SideB;
		});
		for (int32 Column = 0; Column < Width; ++Column)
		{
			const FVector Offset = -Forward * (Row * Spacing) + Right * ((Column - (Width - 1) * 0.5) * Spacing);
			if (!IsFiniteVector(Offset)) return false;
			Slots[Members[First + Column]] = Offset;
			MeanOffset += Offset / Members.Num();
		}
	}
	for (FVector& Slot : Slots)
	{
		const FVector Offset = Slot - MeanOffset;
		Slot = Destination + Offset;
		// Finite but extreme world coordinates can erase all spacing through rounding.
		if (!IsFiniteVector(Slot) || !(Slot - Destination).Equals(Offset, Spacing * 0.000001)) return false;
	}
	OutDestinations = MoveTemp(Slots);
	return true;
}
