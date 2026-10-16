// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Building/ProductionEvent.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "TrainingRefundProbe.generated.h"

class FAutomationTestBase;
class ABarracks;
UCLASS()
class UTrainingRefundProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<UObject> Producer;
	FName UnitID;
	int32 Notifications = 0;
	int32 FailedOrders = 0;
	int32 CompletedOrders = 0;
	int32 PendingRefundEvents = 0;
	UFUNCTION()
	void OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance);
	UFUNCTION()
	void OnProductionChanged(ABarracks* Building, FProductionEvent Event);
};
