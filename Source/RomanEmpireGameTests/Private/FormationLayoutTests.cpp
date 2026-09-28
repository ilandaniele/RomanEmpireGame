// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/RomanFormationLayout.h"
#include "Misc/AutomationTest.h"
#include <initializer_list>
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormationLayoutTest, "RomanEmpireGame.Units.FormationLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFormationLayoutTest::RunTest(const FString& Parameters)
{
	for (int32 Count : { 1, 2, 3, 5, 15, 50, 100 })
	{
		TArray<FVector> Locations;
		for (int32 Index = 0; Index < Count; ++Index) Locations.Add(FVector(Index % 5 * 100, Index / 5 * 100, 96));
		const FVector Goal(2000, -500, 20);
		TArray<FVector> Slots, Again;
		TestTrue(TEXT("Finite formation builds"), FRomanFormationLayout::Build(Locations, Goal, 160, Slots));
		TestEqual(TEXT("Every selected member gets one destination"), Slots.Num(), Count);
		TestTrue(TEXT("Repeated layout builds"), FRomanFormationLayout::Build(Locations, Goal, 160, Again));
		TestTrue(TEXT("Identical inputs are deterministic"), Slots == Again);
		FVector Center = FVector::ZeroVector;
		for (int32 A = 0; A < Slots.Num(); ++A)
		{
			Center += Slots[A] / Count;
			TestEqual(TEXT("Slots retain destination elevation"), Slots[A].Z, Goal.Z);
			for (int32 B = A + 1; B < Slots.Num(); ++B)
				TestTrue(TEXT("All slots keep minimum spacing"), FVector::Dist2D(Slots[A], Slots[B]) >= 159.999);
		}
		TestTrue(TEXT("Incomplete last rank remains centered"), Center.Equals(Goal, 0.0001));
	}
	const TArray<FVector> Positions = { FVector(-10, 10, 0), FVector(10, -10, 0), FVector(-10, -10, 0), FVector(10, 10, 0) };
	TArray<FVector> Slots;
	TestTrue(TEXT("Spatial assignment builds"), FRomanFormationLayout::Build(Positions, FVector(1000, 0, 0), 160, Slots));
	TestTrue(TEXT("Rear right keeps rear right slot"), Slots[0].Equals(FVector(920, 80, 0)));
	TestTrue(TEXT("Front left keeps front left slot"), Slots[1].Equals(FVector(1080, -80, 0)));
	TArray<FVector> RotatedPositions;
	for (const FVector& P : Positions) RotatedPositions.Add(FVector(-P.Y, P.X, P.Z));
	TArray<FVector> RotatedSlots;
	TestTrue(TEXT("Rotated formation builds"), FRomanFormationLayout::Build(RotatedPositions, FVector(0, 1000, 0), 160, RotatedSlots));
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
		TestTrue(TEXT("Formation faces travel direction"), RotatedSlots[Index].Equals(FVector(-Slots[Index].Y, Slots[Index].X, Slots[Index].Z)));
	const TArray<FVector> Saved = Slots;
	const double NaN = std::numeric_limits<double>::quiet_NaN();
	const double Infinity = std::numeric_limits<double>::infinity();
	TestFalse(TEXT("Empty membership rejected"), FRomanFormationLayout::Build({}, FVector::ZeroVector, 160, Slots));
	for (double Spacing : { 0.0, -10.0, NaN, Infinity })
		TestFalse(TEXT("Invalid spacing rejected"), FRomanFormationLayout::Build(Positions, FVector::ZeroVector, Spacing, Slots));
	TestFalse(TEXT("Nonfinite goal rejected"), FRomanFormationLayout::Build(Positions, FVector(NaN, 0, 0), 160, Slots));
	TestFalse(TEXT("Unrepresentable slot spacing rejected"), FRomanFormationLayout::Build(Positions, FVector(1e100, 0, 0), 160, Slots));
	const TArray<FVector> Invalid = { FVector(Infinity, 0, 0) };
	TestFalse(TEXT("Nonfinite member rejected"), FRomanFormationLayout::Build(Invalid, FVector::ZeroVector, 160, Slots));
	TestTrue(TEXT("All rejected requests preserve existing output"), Slots == Saved);
	const TArray<FVector> Coincident = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };
	TestTrue(TEXT("Zero travel and coincident members have deterministic fallback"), FRomanFormationLayout::Build(Coincident, FVector::ZeroVector, 160, Slots));
	TestTrue(TEXT("Fallback still yields distinct slots"), !Slots[0].Equals(Slots[1]));
	return true;
}
#endif
