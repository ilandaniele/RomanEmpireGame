// Copyright Roman Empire Game. All Rights Reserved.

#include "NestedObjectiveEventProbe.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Class.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace RomanEmpireNestedObjectiveTests
{
	// Runtime-private manager/controller types stay behind their reflected commands.
	UObject* ReadObject(FAutomationTestBase& Test, UObject* Target, FName Name)
	{
		UFunction* Function = Target ? Target->FindFunction(Name) : nullptr;
		FObjectPropertyBase* Return = Function
			? FindFProperty<FObjectPropertyBase>(Function, TEXT("ReturnValue")) : nullptr;
		if (!Test.TestTrue(TEXT("Object query exposes exactly one object return parameter"),
			Function && Function->NumParms == 1 && Return && Return->HasAllPropertyFlags(CPF_Parm | CPF_ReturnParm)))
		{
			return nullptr;
		}
		FStructOnScope Parameters(Function);
		Target->ProcessEvent(Function, Parameters.GetStructMemory());
		return Return->GetObjectPropertyValue_InContainer(Parameters.GetStructMemory());
	}

	bool ApplyGold(FAutomationTestBase& Test, UObject* Manager, EFactionID Faction, int32 Gold)
	{
		UFunction* Function = Manager ? Manager->FindFunction(TEXT("ApplyFactionResourceDelta")) : nullptr;
		FEnumProperty* FactionProperty = Function ? FindFProperty<FEnumProperty>(Function, TEXT("FactionID")) : nullptr;
		FStructProperty* DeltaProperty = Function ? FindFProperty<FStructProperty>(Function, TEXT("Delta")) : nullptr;
		FStructProperty* ReturnProperty = Function ? FindFProperty<FStructProperty>(Function, TEXT("ReturnValue")) : nullptr;
		if (!Test.TestTrue(TEXT("Canonical wallet command has its typed faction/delta/result contract"),
			Function && Function->NumParms == 3 && FactionProperty && DeltaProperty && ReturnProperty &&
			FactionProperty->GetEnum() == StaticEnum<EFactionID>() &&
			FactionProperty->HasAnyPropertyFlags(CPF_Parm) && !FactionProperty->HasAnyPropertyFlags(CPF_ReturnParm) &&
			DeltaProperty->Struct == FFactionResourceDelta::StaticStruct() &&
			DeltaProperty->HasAnyPropertyFlags(CPF_Parm) && !DeltaProperty->HasAnyPropertyFlags(CPF_ReturnParm) &&
			ReturnProperty->Struct == FFactionResourceTransactionResult::StaticStruct() &&
			ReturnProperty->HasAllPropertyFlags(CPF_Parm | CPF_ReturnParm)))
		{
			return false;
		}
		FStructOnScope Parameters(Function);
		FactionProperty->GetUnderlyingProperty()->SetIntPropertyValue(
			FactionProperty->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), static_cast<int64>(Faction));
		FFactionResourceDelta Delta;
		Delta.Gold = Gold;
		DeltaProperty->CopyCompleteValue(DeltaProperty->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &Delta);
		Manager->ProcessEvent(Function, Parameters.GetStructMemory());
		FFactionResourceTransactionResult Result;
		ReturnProperty->CopyCompleteValue(&Result, ReturnProperty->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()));
		return Test.TestTrue(TEXT("Real canonical wallet transaction succeeds"), Result.WasSuccessful());
	}

	FString ObjectiveLabel(const FCampaignObjectiveState& State)
	{
		return FString::Printf(TEXT("OBJECTIVES\nConquest: %d / %d territories   |   Economy: %d / %d aurei"),
			State.ControlledTerritories, State.RequiredTerritories, State.CurrentGold, State.RequiredGold);
	}

	class FExerciseNestedObjectives : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseNestedObjectives(FAutomationTestBase* InTest) : Test(*InTest) {}
		virtual ~FExerciseNestedObjectives() override { UnbindProbe(); }

		virtual bool Update() override
		{
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (Stage > 0 && (!World || World != OwnedWorld.Get() || !State.IsValid() || !Root.IsValid()))
			{
				Test.AddError(TEXT("The owned PIE world or UI ended before nested-event assertions"));
				return true;
			}
			if (Stage == 0)
			{
				ARomanEmpireGameState* CurrentState = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
				APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
				if (!World || !World->HasBegunPlay() || !CurrentState || !CurrentState->IsGameplayInitialized() || !Controller)
				{
					if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
					Test.AddError(TEXT("Nested-objective test did not reach ready PIE gameplay within 30 seconds"));
					return true;
				}
				OwnedWorld = World;
				State = CurrentState;
				Root = Cast<UUserWidget>(ReadObject(Test, Controller, TEXT("GetMainWidget")));
				Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetFactionManager"));
				if (!Test.TestTrue(TEXT("Normal startup supplies the production UMG and wallet owner"), Root.IsValid() && Manager.IsValid())) return true;
				Root->RemoveFromParent();
				Stage = 1;
				return false; // Let normal NativeDestruct release the root's observer first.
			}
			if (Stage == 1)
			{
				if (!Test.TestFalse(TEXT("Detached production UMG has released its objective observer"),
					State->OnCampaignObjectivesChanged.Contains(Root.Get(), TEXT("HandleObjectivesChanged")))) return true;
				Probe.Reset(NewObject<UNestedObjectiveEventProbe>());
				Probe->Test = &Test;
				Probe->FactionManager = Manager;
				Probe->PlayerFaction = State->GetPlayerFaction();
				State->OnCampaignObjectivesChanged.AddUniqueDynamic(Probe.Get(), &UNestedObjectiveEventProbe::HandleObjectives);
				Root->AddToViewport();
				Stage = 2;
				return false;
			}

			// UE 5.8 script multicast delegates visit this copied invocation list in
			// registration order. Reattachment places the real UI after our mutator.
			const TArray<UObject*> Listeners = State->OnCampaignObjectivesChanged.GetAllObjects();
			const int32 ProbeIndex = Listeners.IndexOfByKey(Probe.Get());
			const int32 RootIndex = Listeners.IndexOfByKey(Root.Get());
			if (!Test.TestTrue(TEXT("The nested mutator deterministically precedes the actual UMG observer"),
				ProbeIndex != INDEX_NONE && RootIndex != INDEX_NONE && ProbeIndex < RootIndex)) return true;
			UTextBlock* ObjectiveText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ObjectiveText")));
			if (!Test.TestNotNull(TEXT("The production objective label exists"), ObjectiveText)) return true;
			const FCampaignObjectiveState Before = State->GetObjectiveState();
			Test.TestEqual(TEXT("Reattached objective UI initially agrees with GameState"),
				ObjectiveText->GetText().ToString(), ObjectiveLabel(Before));
			constexpr int32 OuterGold = 17;
			ApplyGold(Test, Manager.Get(), State->GetPlayerFaction(), OuterGold);
			const FCampaignObjectiveState After = State->GetObjectiveState();
			Test.TestEqual(TEXT("Exactly one transaction is committed from inside the notification"), Probe->NestedTransactions, 1);
			if (Test.TestEqual(TEXT("The real public event is delivered once per committed transaction"), Probe->ObservedGold.Num(), 2))
			{
				Test.TestEqual(TEXT("Outer event captures only the first transaction"), Probe->ObservedGold[0], Before.CurrentGold + OuterGold);
				Test.TestEqual(TEXT("Nested event captures both transactions"), Probe->ObservedGold[1], Before.CurrentGold + OuterGold + Probe->NestedGold);
			}
			Test.TestEqual(TEXT("Canonical objective progress contains both real wallet transactions"),
				After.CurrentGold, Before.CurrentGold + OuterGold + Probe->NestedGold);
			// No tick or forced refresh is permitted here: the stale outer payload runs
			// last, so the old implementation displays Before + OuterGold and fails.
			Test.TestEqual(TEXT("Late outer notification cannot roll UMG objectives back behind GameState"),
				ObjectiveText->GetText().ToString(), ObjectiveLabel(After));
			UnbindProbe();
			Test.TestFalse(TEXT("Regression observer is removed before restoring the wallet"),
				State->OnCampaignObjectivesChanged.GetAllObjects().Contains(Probe.Get()));
			ApplyGold(Test, Manager.Get(), State->GetPlayerFaction(), Before.CurrentGold - After.CurrentGold);
			Test.TestEqual(TEXT("Wallet is restored without another nested mutation"), State->GetObjectiveState().CurrentGold, Before.CurrentGold);
			Test.TestEqual(TEXT("Normal UMG observation remains active after probe cleanup"),
				ObjectiveText->GetText().ToString(), ObjectiveLabel(State->GetObjectiveState()));
			return true;
		}

	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		int32 Stage = 0;
		TWeakObjectPtr<UWorld> OwnedWorld;
		TWeakObjectPtr<ARomanEmpireGameState> State;
		TWeakObjectPtr<UUserWidget> Root;
		TWeakObjectPtr<UObject> Manager;
		TStrongObjectPtr<UNestedObjectiveEventProbe> Probe;

		void UnbindProbe()
		{
			if (State.IsValid() && Probe.IsValid())
			{
				State->OnCampaignObjectivesChanged.RemoveDynamic(Probe.Get(), &UNestedObjectiveEventProbe::HandleObjectives);
			}
		}
	};

	class FWaitForNestedObjectivePIEEnd : public IAutomationLatentCommand
	{
	public:
		explicit FWaitForNestedObjectivePIEEnd(FAutomationTestBase* InTest) : Test(*InTest) {}
		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld) return true;
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
			Test.AddError(TEXT("Nested-objective test's owned PIE session did not end within 30 seconds"));
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
	};
}

void UNestedObjectiveEventProbe::HandleObjectives(FCampaignObjectiveState State)
{
	ObservedGold.Add(State.CurrentGold);
	if (!Test || ObservedGold.Num() != 1) return; // Bounded even if the command fails.
	if (RomanEmpireNestedObjectiveTests::ApplyGold(*Test, FactionManager.Get(), PlayerFaction, NestedGold))
	{
		++NestedTransactions;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNestedObjectiveEventIntegrationTest,
	"RomanEmpireGame.UI.NestedObjectiveTransactionsPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNestedObjectiveEventIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestNotNull(TEXT("Nested-objective test requires an editor"), GEditor) ||
		!TestNull(TEXT("Nested-objective test owns a fresh PIE session"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loads before PIE"), EditorWorld && EditorWorld->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireNestedObjectiveTests::FExerciseNestedObjectives(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireNestedObjectiveTests::FWaitForNestedObjectivePIEEnd(this));
	return true;
}

#endif
