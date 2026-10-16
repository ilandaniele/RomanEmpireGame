// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UI/BuildingMenuWidget.h"
#include "RomanTerritoryButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRomanTerritoryClicked, FName, TerritoryID);

/** Mouse HUD entry carrying only a stable presentation key, never live ownership. */
UCLASS()
class ROMANEMPIREGAME_API URomanTerritoryButton : public URomanHUDButton
{
	GENERATED_BODY()
public:
	void InitializeTerritoryButton(FName ID);
	UFUNCTION(BlueprintPure, Category = "UI|Territories")
	FName GetTerritoryID() const { return TerritoryID; }
	UPROPERTY(BlueprintAssignable, Category = "UI|Territories")
	FOnRomanTerritoryClicked OnTerritoryClicked;
private:
	UPROPERTY(Transient)
	FName TerritoryID = NAME_None;
	UFUNCTION()
	void HandleClicked();
};
