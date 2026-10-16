// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/VerticalBox.h"
#include "UI/RomanTextScaleProjection.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanTextScaleTest,"RomanEmpireGame.Preferences.TextScaleProjection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanTextScaleTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWidgetTree> Tree(NewObject<UWidgetTree>());
	UVerticalBox* Panel=Tree->ConstructWidget<UVerticalBox>(); Tree->RootWidget=Panel;
	UTextBlock* First=Tree->ConstructWidget<UTextBlock>(); Panel->AddChildToVerticalBox(First);
	FSlateFontInfo Font=First->GetFont(); Font.Size=17; First->SetFont(Font);
	USpinBox* Spin=Tree->ConstructWidget<USpinBox>(); Font.Size=16; Spin->SetFont(Font); Panel->AddChildToVerticalBox(Spin);
	FRomanTextScaleProjection Projection;
	TestTrue(TEXT("Apply actual layout typography"),Projection.Apply(Tree.Get(),1.5f));
	TestEqual(TEXT("Fractional font size preserved"),First->GetFont().Size,25.5f);
	TestEqual(TEXT("Editable numeric font also scales in layout"),Spin->GetFont().Size,24.f);
	Projection.Apply(Tree.Get(),1.5f); TestEqual(TEXT("Repeated refresh never compounds"),First->GetFont().Size,25.5f);
	UTextBlock* Later=Tree->ConstructWidget<UTextBlock>(); Font.Size=12; Later->SetFont(Font); Panel->AddChildToVerticalBox(Later);
	Projection.Apply(Tree.Get(),2); TestEqual(TEXT("Existing label uses original base"),First->GetFont().Size,34.f);
	TestEqual(TEXT("New dynamic label receives current scale"),Later->GetFont().Size,24.f);
	for(float Invalid:{0.f,2.1f,std::numeric_limits<float>::quiet_NaN()})
	{
		TestFalse(TEXT("Invalid scale rejects before widget changes"),Projection.Apply(Tree.Get(),Invalid));
		TestEqual(TEXT("Rejected scale preserves font"),First->GetFont().Size,34.f);
	}
	TestFalse(TEXT("No tree safely rejects"),Projection.Apply(nullptr,1));
	Projection.Apply(Tree.Get(),1); TestEqual(TEXT("Decrease restores authored font"),First->GetFont().Size,17.f);
	TestEqual(TEXT("Dynamic label restores its independent authored font"),Later->GetFont().Size,12.f);
	TestEqual(TEXT("Editable font restores without compounding"),Spin->GetFont().Size,16.f);
	return !HasAnyErrors();
}
#endif
