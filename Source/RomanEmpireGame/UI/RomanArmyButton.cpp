// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanArmyButton.h"
void URomanArmyButton::InitializeArmyButton(FGuid ID)
{
	ArmyID = ID; OnClicked.AddUniqueDynamic(this, &URomanArmyButton::HandleClicked);
}
void URomanArmyButton::HandleClicked()
{
	if (GetIsEnabled() && ArmyID.IsValid()) OnArmyClicked.Broadcast(ArmyID);
}
