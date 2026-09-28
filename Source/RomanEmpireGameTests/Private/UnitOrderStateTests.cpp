// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Units/UnitOrderState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RHIFeatureLevel.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include <initializer_list>
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace UnitOrderStateTests
{
	/** Isolated world with its own context; never changes the editor's current map or PIE. */
	class FScopedWorld
	{
	public:
		FScopedWorld()
		{
			const FName WorldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("UnitOrderStateTestWorld"));
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).CreatePhysicsScene(false).RequiresHitProxies(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).EnableTraceCollision(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}

		~FScopedWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}

		FScopedWorld(const FScopedWorld&) = delete;
		FScopedWorld& operator=(const FScopedWorld&) = delete;

		UWorld* Get() const { return World; }

	private:
		UWorld* World = nullptr; // CreateWorld roots it; DestroyWorld removes that root.
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitOrderStateTest, "RomanEmpireGame.Units.OrderState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnitOrderStateTest::RunTest(const FString& Parameters)
{
	using namespace UnitOrderStateTests;
	FUnitOrderState State;
	const FVector FirstDestination(300.0, -40.0, 20.0);
	const FVector SecondDestination(-200.0, 600.0, -15.0);
	FVector NotFinite = FirstDestination;
	NotFinite.X = std::numeric_limits<double>::quiet_NaN();
	FVector Infinite = FirstDestination;
	Infinite.Z = std::numeric_limits<double>::infinity();

	TestEqual(TEXT("Default order is None"), State.GetOrderType(), EUnitOrderType::None);
	TestEqual(TEXT("Default destination is zero"), State.GetDestination(), FVector::ZeroVector);
	TestNull(TEXT("Default target is null"), State.GetTarget());
	TestFalse(TEXT("Default has no movement"), State.HasMoveCommand());
	TestFalse(TEXT("Empty order has no stale target to clear"), State.ClearInvalidTarget());
	TestFalse(TEXT("Pursuit requires Attack"), State.UpdatePursuitDestination(FirstDestination));
	TestFalse(TEXT("NaN move rejected"), State.IssueMove(NotFinite));
	TestFalse(TEXT("Infinite move rejected"), State.IssueMove(Infinite));
	TestEqual(TEXT("Rejected move preserves empty state"), State.GetOrderType(), EUnitOrderType::None);
	TestEqual(TEXT("Rejected commands do not create revisions"), State.GetRevision(), uint64(0));
	TestTrue(TEXT("Finite move accepted"), State.IssueMove(FirstDestination));
	const uint64 MoveRevision = State.GetRevision();
	TestTrue(TEXT("Accepted command advances revision"), MoveRevision > 0);
	TestEqual(TEXT("Move type"), State.GetOrderType(), EUnitOrderType::Move);
	TestEqual(TEXT("Move destination"), State.GetDestination(), FirstDestination);
	TestTrue(TEXT("Move is active"), State.HasMoveCommand());
	TestFalse(TEXT("Pursuit cannot overwrite Move"), State.UpdatePursuitDestination(SecondDestination));
	TestFalse(TEXT("Null attack rejects atomically"), State.IssueAttack(nullptr, SecondDestination));
	TestEqual(TEXT("Invalid attack preserves Move"), State.GetOrderType(), EUnitOrderType::Move);
	TestEqual(TEXT("Invalid attack preserves destination"), State.GetDestination(), FirstDestination);
	TestEqual(TEXT("Rejected replacement preserves revision"), State.GetRevision(), MoveRevision);
	State.StopMovement();
	TestEqual(TEXT("Completed move becomes None"), State.GetOrderType(), EUnitOrderType::None);
	TestEqual(TEXT("Completed move clears destination"), State.GetDestination(), FVector::ZeroVector);
	TestFalse(TEXT("Completed move cannot continue"), State.HasMoveCommand());

	FScopedWorld Fixture;
	UWorld* World = Fixture.Get();
	if (!TestNotNull(TEXT("Owned transient world"), World)) return false;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	AActor* FirstTarget = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AActor* SecondTarget = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	if (!TestNotNull(TEXT("First real spawned target"), FirstTarget) ||
		!TestNotNull(TEXT("Second real spawned target"), SecondTarget)) return false;

	TestTrue(TEXT("Attack accepted"), State.IssueAttack(FirstTarget, FirstDestination));
	const uint64 AttackRevision = State.GetRevision();
	TestTrue(TEXT("Attack advances revision"), AttackRevision > MoveRevision);
	TestEqual(TEXT("Attack type"), State.GetOrderType(), EUnitOrderType::Attack);
	TestTrue(TEXT("Attack starts pursuit"), State.HasMoveCommand());
	TestEqual(TEXT("Attack retains target"), State.GetTarget(), FirstTarget);
	TestFalse(TEXT("Live attack cannot be cleared as stale"), State.ClearInvalidTarget());
	TestFalse(TEXT("NaN attack rejected"), State.IssueAttack(SecondTarget, NotFinite));
	TestFalse(TEXT("Infinite attack rejected"), State.IssueAttack(SecondTarget, Infinite));
	TestFalse(TEXT("NaN pursuit rejected"), State.UpdatePursuitDestination(NotFinite));
	TestFalse(TEXT("Infinite pursuit rejected"), State.UpdatePursuitDestination(Infinite));
	TestFalse(TEXT("NaN replacement move rejected"), State.IssueMove(NotFinite));
	TestEqual(TEXT("Invalid replacements retain Attack"), State.GetOrderType(), EUnitOrderType::Attack);
	TestEqual(TEXT("Invalid replacements retain target"), State.GetTarget(), FirstTarget);
	TestEqual(TEXT("Invalid replacements retain destination"), State.GetDestination(), FirstDestination);
	TestTrue(TEXT("Invalid replacements retain pursuit"), State.HasMoveCommand());
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		for (double InvalidValue : { std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity() })
		{
			FVector InvalidDestination = FirstDestination;
			InvalidDestination[Axis] = InvalidValue;
			TestFalse(TEXT("Every non-finite move coordinate is rejected"), State.IssueMove(InvalidDestination));
			TestFalse(TEXT("Every non-finite attack coordinate is rejected"), State.IssueAttack(SecondTarget, InvalidDestination));
			TestFalse(TEXT("Every non-finite pursuit coordinate is rejected"), State.UpdatePursuitDestination(InvalidDestination));
			TestEqual(TEXT("Coordinate rejection retains Attack"), State.GetOrderType(), EUnitOrderType::Attack);
			TestEqual(TEXT("Coordinate rejection retains target"), State.GetTarget(), FirstTarget);
			TestEqual(TEXT("Coordinate rejection retains destination"), State.GetDestination(), FirstDestination);
			TestTrue(TEXT("Coordinate rejection retains pursuit"), State.HasMoveCommand());
		}
	}
	TestTrue(TEXT("Pursuit destination updates"), State.UpdatePursuitDestination(SecondDestination));
	TestEqual(TEXT("Pursuit keeps Attack type"), State.GetOrderType(), EUnitOrderType::Attack);
	TestEqual(TEXT("Pursuit keeps original target"), State.GetTarget(), FirstTarget);
	TestEqual(TEXT("Pursuit has new destination"), State.GetDestination(), SecondDestination);
	TestEqual(TEXT("Pursuit tracking preserves command revision"), State.GetRevision(), AttackRevision);
	State.StopMovement();
	State.StopMovement();
	TestFalse(TEXT("Attack pursuit can stop repeatedly"), State.HasMoveCommand());
	TestEqual(TEXT("Stopped pursuit remains Attack"), State.GetOrderType(), EUnitOrderType::Attack);
	TestEqual(TEXT("Stopped pursuit retains target"), State.GetTarget(), FirstTarget);
	TestEqual(TEXT("Stopped pursuit retains last destination"), State.GetDestination(), SecondDestination);
	TestTrue(TEXT("Pursuit resumes without replacing target"), State.UpdatePursuitDestination(FirstDestination));
	TestTrue(TEXT("Resumed pursuit moves"), State.HasMoveCommand());
	TestTrue(TEXT("Attack replaces target"), State.IssueAttack(SecondTarget, SecondDestination));
	TestTrue(TEXT("Retarget invalidates old completion revision"), State.GetRevision() > AttackRevision);
	TestEqual(TEXT("Replaced attack target"), State.GetTarget(), SecondTarget);
	TestTrue(TEXT("Move replaces Attack"), State.IssueMove(FirstDestination));
	TestNull(TEXT("Move clears attack target"), State.GetTarget());
	TestTrue(TEXT("Replacement Move moves"), State.HasMoveCommand());
	State.Hold();
	State.StopMovement();
	TestEqual(TEXT("Hold persists through StopMovement"), State.GetOrderType(), EUnitOrderType::Hold);
	TestFalse(TEXT("Hold has no motion"), State.HasMoveCommand());
	TestNull(TEXT("Hold has no target"), State.GetTarget());
	TestEqual(TEXT("Hold has no destination"), State.GetDestination(), FVector::ZeroVector);
	TestFalse(TEXT("Hold cannot accidentally resume pursuit"), State.UpdatePursuitDestination(FirstDestination));
	TestFalse(TEXT("Hold is not a stale Attack"), State.ClearInvalidTarget());
	TestTrue(TEXT("Attack explicitly replaces Hold"), State.IssueAttack(FirstTarget, FirstDestination));
	State.Hold();
	TestNull(TEXT("Hold clears a live attack target"), State.GetTarget());
	TestFalse(TEXT("Hold cancels a live pursuit"), State.HasMoveCommand());
	State.Clear();
	State.Clear();
	TestEqual(TEXT("Clear is idempotent"), State.GetOrderType(), EUnitOrderType::None);
	TestEqual(TEXT("Clear destination"), State.GetDestination(), FVector::ZeroVector);
	TestNull(TEXT("Clear target"), State.GetTarget());
	TestFalse(TEXT("Clear motion"), State.HasMoveCommand());

	TestTrue(TEXT("Attack observes target lifecycle"), State.IssueAttack(FirstTarget, FirstDestination));
	TestTrue(TEXT("Real target destruction succeeds"), FirstTarget->Destroy());
	TestNull(TEXT("Destroyed target is immediately hidden"), State.GetTarget());
	TestFalse(TEXT("Destroyed target cannot continue pursuit"), State.HasMoveCommand());
	TestFalse(TEXT("Destroyed target cannot resume pursuit"), State.UpdatePursuitDestination(SecondDestination));
	TestEqual(TEXT("Rejected resume preserves last destination"), State.GetDestination(), FirstDestination);
	TestTrue(TEXT("Stale Attack clears explicitly"), State.ClearInvalidTarget());
	TestEqual(TEXT("Stale Attack becomes None"), State.GetOrderType(), EUnitOrderType::None);
	TestEqual(TEXT("Stale Attack clears destination"), State.GetDestination(), FVector::ZeroVector);
	TestFalse(TEXT("Stale Attack clears only once"), State.ClearInvalidTarget());
	TestTrue(TEXT("Move fixture before rejected destroyed target"), State.IssueMove(SecondDestination));
	TestFalse(TEXT("Destroyed target cannot receive new Attack"), State.IssueAttack(FirstTarget, FirstDestination));
	TestEqual(TEXT("Destroyed-target rejection preserves Move"), State.GetOrderType(), EUnitOrderType::Move);
	TestEqual(TEXT("Destroyed-target rejection preserves destination"), State.GetDestination(), SecondDestination);

	TestTrue(TEXT("GC lifecycle attack accepted"), State.IssueAttack(SecondTarget, SecondDestination));
	TWeakObjectPtr<AActor> WeakTarget(SecondTarget);
	TestTrue(TEXT("GC target destruction succeeds"), SecondTarget->Destroy());
	FirstTarget = nullptr;
	SecondTarget = nullptr;
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	TestFalse(TEXT("Order does not retain destroyed actor through GC"), WeakTarget.IsValid(true));
	TestNull(TEXT("Collected target query is safe"), State.GetTarget());
	TestFalse(TEXT("Collected target has no motion"), State.HasMoveCommand());
	TestTrue(TEXT("Collected target can be cleared"), State.ClearInvalidTarget());
	TestEqual(TEXT("Collected target leaves empty order"), State.GetOrderType(), EUnitOrderType::None);
	return true;
}
#endif
