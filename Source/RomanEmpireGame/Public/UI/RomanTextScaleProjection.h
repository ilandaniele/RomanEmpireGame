// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
class UWidgetTree;
class UWidget;

/** Per-root layout font projection, not render transforms or another settings owner.
 * Retain across detach/reattach so fonts never multiply cumulatively. Weak keys do not own widgets. */
struct ROMANEMPIREGAME_API FRomanTextScaleProjection
{
	bool Apply(UWidgetTree* Tree, float Scale);
private:
	TMap<TWeakObjectPtr<UWidget>,float> BaseSizes;
};
