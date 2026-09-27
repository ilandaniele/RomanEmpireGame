// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BuildingSelectionProbe.generated.h"

class ABuildingBase;
class AUnitBase;
class FAutomationTestBase;

/** Changes ownership inside the real selection notification, before the UI observer. */
UCLASS()
class UBuildingSelectionProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<UObject> Target;
	bool bArmed = false;
	int32 Interventions = 0;
	UFUNCTION()
	void OnSelection(const TArray<AUnitBase*>& Units, ABuildingBase* Building);
};
