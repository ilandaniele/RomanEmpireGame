// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "UI/RomanSettingsButton.h"
#include "UI/RomanSettingsWidget.h"
#include "RomanKeyBindingButton.generated.h"
/** Event routing only; the ID is immutable and never stores a player's binding. */
UCLASS()
class URomanKeyBindingButton : public URomanSettingsButton
{
	GENERATED_BODY()
public:
	void InitializeBinding(URomanSettingsWidget* InMenu,FName InName)
	{
		Menu=InMenu; MappingName=InName; OnClicked.AddUniqueDynamic(this,&ThisClass::BeginCapture);
	}
	FName GetMappingName() const {return MappingName;}
private:
	TWeakObjectPtr<URomanSettingsWidget> Menu;
	FName MappingName;
	UFUNCTION() void BeginCapture() {if(Menu.IsValid()) Menu->BeginBindingCapture(MappingName,this);}
};
