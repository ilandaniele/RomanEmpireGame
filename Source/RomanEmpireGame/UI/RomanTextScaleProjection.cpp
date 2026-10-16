// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanTextScaleProjection.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Fonts/SlateFontInfo.h"

bool FRomanTextScaleProjection::Apply(UWidgetTree* Tree, float Scale)
{
	if (!Tree || !FMath::IsFinite(Scale) || Scale < 1 || Scale > 2) return false;
	for (auto It=BaseSizes.CreateIterator();It;++It) if (!It.Key().IsValid()) It.RemoveCurrent();
	Tree->ForEachWidgetAndDescendants([this,Scale](UWidget* Widget)
	{
		if (UTextBlock* Text=Cast<UTextBlock>(Widget))
		{
			FSlateFontInfo Font=Text->GetFont();
			const float Base=BaseSizes.FindOrAdd(Text,Font.Size);
			const float Desired=Base*Scale;
			if (!FMath::IsNearlyEqual(Font.Size,Desired)) { Font.Size=Desired; Text->SetFont(Font); }
		}
		else if (USpinBox* Spin=Cast<USpinBox>(Widget))
		{
			FSlateFontInfo Font=Spin->GetFont();
			const float Base=BaseSizes.FindOrAdd(Spin,Font.Size);
			const float Desired=Base*Scale;
			if (!FMath::IsNearlyEqual(Font.Size,Desired)) { Font.Size=Desired; Spin->SetFont(Font); }
		}
	});
	return true;
}
