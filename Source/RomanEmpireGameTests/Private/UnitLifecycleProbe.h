// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnitLifecycleProbe.generated.h"

class AUnitBase;
class AActor;
class FAutomationTestBase;

UCLASS()
class UUnitLifecycleProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<AActor> ObservedUnit;
	int32 DeathEvents = 0;
	int32 VitalsEvents = 0;
	bool bTerminateOnVitals = false;
	bool bDestroyDuringVitals = false;
	bool bHandledVitalsAction = false;
	UFUNCTION()
	void HandleDeath(AUnitBase* Unit);
	UFUNCTION()
	void HandleVitals(AUnitBase* Unit);
};
