// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "RomanFunctionalScenario.generated.h"

class FAutomationTestBase;
class IAutomationLatentCommand;
struct FRomanFunctionalExecution;

/** Editor-only adapters to the existing canonical integration scenarios.
 * Each dedicated map owns exactly one adapter and a fresh PIE world.
 */
UCLASS(NotBlueprintable)
class ARomanFunctionalScenario : public AFunctionalTest
{
	GENERATED_BODY()
public:
	ARomanFunctionalScenario();
	virtual ~ARomanFunctionalScenario() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void FinishTest(EFunctionalTestResult TestResult, const FString& Message) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void CleanUp() override;

	UPROPERTY(VisibleAnywhere, Category="Roman|Tests")
	int32 SchemaVersion = 1;
	/** Stable fixture discriminator, not gameplay data. */
	UPROPERTY(VisibleAnywhere, Category="Roman|Tests")
	int32 Scenario = 0;

protected:
	virtual void StartTest() override;
private:
	void AdvanceScenario();
	TSharedPtr<FRomanFunctionalExecution> Execution;
};

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
TSharedPtr<IAutomationLatentCommand> CreateRomanFunctionalScenario(int32 Scenario, FAutomationTestBase& Assertions);
#endif
