// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UI/BuildingMenuWidget.h"
#include "RomanArmyButton.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRomanArmyClicked, FGuid, ArmyID);
/** Carries only a stable inspection key, not live inventory/faction/placement. */
UCLASS()
class ROMANEMPIREGAME_API URomanArmyButton : public URomanHUDButton
{
	GENERATED_BODY()
public:
	void InitializeArmyButton(FGuid ID);
	UFUNCTION(BlueprintPure, Category = "UI|Armies") FGuid GetArmyID() const { return ArmyID; }
	UPROPERTY(BlueprintAssignable, Category = "UI|Armies") FOnRomanArmyClicked OnArmyClicked;
private:
	UPROPERTY(Transient) FGuid ArmyID;
	UFUNCTION() void HandleClicked();
};
