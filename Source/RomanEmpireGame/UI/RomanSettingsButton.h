// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UI/BuildingMenuWidget.h"
#include "RomanSettingsButton.generated.h"
/** Reuses existing high-contrast HUD styling, with constructor-time keyboard focus enabled. */
UCLASS()
class URomanSettingsButton : public URomanHUDButton
{
	GENERATED_BODY()
public:
	URomanSettingsButton(const FObjectInitializer& Initializer):Super(Initializer) { InitIsFocusable(true); }
};
