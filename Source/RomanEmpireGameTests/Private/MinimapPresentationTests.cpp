// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "UI/MinimapPresentation.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapProjectionTest, "RomanEmpireGame.UI.MinimapProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMinimapProjectionTest::RunTest(const FString&)
{
	const FMinimapProjection Map{FVector2D(-100,-50), FVector2D(300,150)};
	TestTrue(TEXT("Finite nonzero rectangle"), Map.IsValid());
	TestEqual(TEXT("Center"), Map.Project(FVector2D(100,50)), FVector2D(0.5,0.5));
	TestEqual(TEXT("Clamped corners"), Map.Project(FVector2D(-1000,1000)), FVector2D(0,1));
	TestEqual(TEXT("Inverse center"), Map.Unproject(FVector2D(0.5,0.5)), FVector2D(100,50));
	TestEqual(TEXT("Inverse clamped"), Map.Unproject(FVector2D(5,-2)), FVector2D(300,-50));
	for (int32 I=0; I<=10; ++I)
	{
		const FVector2D P(-100+40*I, -50+20*I);
		TestTrue(TEXT("Round trip"), Map.Unproject(Map.Project(P)).Equals(P, 0.000001));
	}
	TestFalse(TEXT("Degenerate rejected"), FMinimapProjection{FVector2D(1,1),FVector2D(1,2)}.IsValid());
	TestFalse(TEXT("Inverted rejected"), FMinimapProjection{FVector2D(5,5),FVector2D(1,1)}.IsValid());
	const double NaN = std::numeric_limits<double>::quiet_NaN();
	const double Inf = std::numeric_limits<double>::infinity();
	TestFalse(TEXT("NaN rejected"), FMinimapProjection{FVector2D(NaN,0),FVector2D(10,10)}.IsValid());
	TestFalse(TEXT("Infinity rejected"), FMinimapProjection{FVector2D(0,0),FVector2D(Inf,10)}.IsValid());
	TestEqual(TEXT("Invalid point safe"), Map.Project(FVector2D(NaN,1)), FVector2D::ZeroVector);
	return true;
}
#endif
