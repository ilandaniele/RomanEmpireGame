// Copyright Roman Empire Game. All Rights Reserved.
#include "RomanFunctionalScenario.h"

#include "Editor.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace
{
	/** Local assertion sink, immediately unregistered: never enumerated or run
	 * recursively by automation. FunctionalTest remains the outcome owner.
	 */
	class FScenarioAssertions final : public FAutomationTestBase
	{
	public:
		FScenarioAssertions() : FAutomationTestBase(TEXT("RomanFunctionalSink_") + FGuid::NewGuid().ToString(), false)
		{
			FAutomationTestFramework::Get().UnregisterAutomationTest(GetTestName());
		}
		virtual EAutomationTestFlags GetTestFlags() const override { return EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter; }
		virtual FString GetBeautifiedTestName() const override { return GetTestName(); }
		virtual uint32 GetRequiredDeviceNum() const override { return 1; }
		virtual void AddError(const FString& Error, int32 StackOffset = 0) override { Errors.Add(Error); }
		virtual void GetTests(TArray<FString>& Names, TArray<FString>& Commands) const override {}
		virtual bool RunTest(const FString& Parameters) override { return false; }
		TArray<FString> Errors;
	};
}

struct FRomanFunctionalExecution
{
	// Order matters: command destructor may still reference Assertions.
	FScenarioAssertions Assertions;
	TSharedPtr<IAutomationLatentCommand> Command;
	FTSTicker::FDelegateHandle Ticker;
	~FRomanFunctionalExecution() { FTSTicker::GetCoreTicker().RemoveTicker(Ticker); }
};
#else
struct FRomanFunctionalExecution {};
#endif

ARomanFunctionalScenario::ARomanFunctionalScenario()
{
	PrimaryActorTick.bCanEverTick = true;
	SetTimeLimit(120.0f, EFunctionalTestResult::Failed);
}

ARomanFunctionalScenario::~ARomanFunctionalScenario() = default;

void ARomanFunctionalScenario::StartTest()
{
	Super::StartTest();
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
	if (SchemaVersion != 1 || !GEditor || GetWorld() != GEditor->PlayWorld)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Owned schema1 fixture requires an intentional fresh PIE session."));
		return;
	}
	Execution = MakeShared<FRomanFunctionalExecution>();
	Execution->Command = CreateRomanFunctionalScenario(Scenario, Execution->Assertions);
	if (!Execution->Command.IsValid())
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Unknown functional scenario."));
		return;
	}
	// These existing commands include explicit GC and Slate dispatch. Run on
	// the engine's core ticker, outside actor/world tick traversal, just like
	// native automation, never recursively inside a gameplay actor tick.
	Execution->Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
		[this](float) { AdvanceScenario(); return true; }));
#else
	FinishTest(EFunctionalTestResult::Failed, TEXT("Development automation support required."));
#endif
}

void ARomanFunctionalScenario::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ARomanFunctionalScenario::AdvanceScenario()
{
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
	if (!IsRunning() || !Execution || !Execution->Command.IsValid()) return;
	if (!GEditor || GetWorld() != GEditor->PlayWorld)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Scenario world changed unexpectedly."));
		return;
	}
	// Keep the command alive across callbacks/GC; execute only once per world tick.
	const TSharedPtr<FRomanFunctionalExecution> Running = Execution;
	const TSharedPtr<IAutomationLatentCommand> Command = Running->Command;
	const bool Complete = Command->Update();
	if (!Running->Assertions.Errors.IsEmpty())
	{
		FinishTest(EFunctionalTestResult::Failed, FString::Join(Running->Assertions.Errors, TEXT("\n")));
	}
	else if (Complete)
	{
		FinishTest(EFunctionalTestResult::Succeeded, TEXT("Canonical integration scenario completed with zero assertion errors."));
	}
#endif
}

void ARomanFunctionalScenario::CleanUp()
{
	Execution.Reset();
	Super::CleanUp();
}

void ARomanFunctionalScenario::FinishTest(EFunctionalTestResult TestResult, const FString& Message)
{
	// Stop the scoped ticker on success, assertion failure, timeout and abort.
	// AdvanceScenario keeps a local shared execution alive across callbacks.
	Execution.Reset();
	Super::FinishTest(TestResult, Message);
}

void ARomanFunctionalScenario::EndPlay(const EEndPlayReason::Type Reason)
{
	Execution.Reset();
	Super::EndPlay(Reason);
}
