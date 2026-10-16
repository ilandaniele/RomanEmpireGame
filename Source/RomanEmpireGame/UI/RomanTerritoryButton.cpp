// Copyright Roman Empire Game. All Rights Reserved.
#include "RomanTerritoryButton.h"

void URomanTerritoryButton::InitializeTerritoryButton(FName ID)
{
	TerritoryID = ID;
	OnClicked.AddUniqueDynamic(this, &URomanTerritoryButton::HandleClicked);
}

void URomanTerritoryButton::HandleClicked()
{
	if (GetIsEnabled() && !TerritoryID.IsNone()) OnTerritoryClicked.Broadcast(TerritoryID);
}
