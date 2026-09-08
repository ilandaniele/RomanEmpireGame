// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "PlacementTransactionProbe.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Building/BuildingTypes.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ActorComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Core/GameplayMode.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Faction/FactionData.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Layout/ArrangedWidget.h"
#include "Layout/Geometry.h"
#include "Layout/WidgetPath.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/ScriptDelegates.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace RomanEmpireUITests
{
	// The tests depend only on public contracts and reflection, not runtime-private headers.
	// FStructOnScope initializes/destructs nontrivial UFUNCTION parameters (FText, arrays, etc.).
	class FCall
	{
	public:
		FCall(FAutomationTestBase& InTest, UObject* InTarget, FName Name, int32 ParameterCount)
			: Test(InTest), Target(InTarget), Function(InTarget ? InTarget->FindFunction(Name) : nullptr),
			  Parameters(Function)
		{
			bValid = Test.TestNotNull(*FString::Printf(TEXT("Reflected function %s exists"), *Name.ToString()), Function);
			if (bValid)
			{
				bValid = Test.TestEqual(TEXT("Reflected parameter count matches the requested API"),
					static_cast<int32>(Function->NumParms), ParameterCount);
			}
		}

		template <typename T>
		void SetStruct(FName Name, const T& Value)
		{
			FStructProperty* Property = GetProperty<FStructProperty>(Name, false);
			if (Property && Validate(Property->Struct == T::StaticStruct(), TEXT("Input struct has the expected type")))
			{
				Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &Value);
			}
		}

		template <typename T>
		void SetEnum(FName Name, T Value)
		{
			FEnumProperty* Property = GetProperty<FEnumProperty>(Name, false);
			if (Property && Validate(Property->GetEnum() == StaticEnum<T>(), TEXT("Input enum has the expected type")))
			{
				Property->GetUnderlyingProperty()->SetIntPropertyValue(
					Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), static_cast<int64>(Value));
			}
		}

		void SetObject(FName Name, UObject* Value)
		{
			FObjectPropertyBase* Property = GetProperty<FObjectPropertyBase>(Name, false);
			if (Property && Validate(!Value || Value->IsA(Property->PropertyClass), TEXT("Input object is compatible")))
			{
				Property->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Value);
			}
		}

		void SetInt(FName Name, int32 Value)
		{
			if (FIntProperty* Property = GetProperty<FIntProperty>(Name, false))
			{
				Property->SetPropertyValue_InContainer(Parameters.GetStructMemory(), Value);
			}
		}

		void SetName(FName Name, FName Value)
		{
			if (FNameProperty* Property = GetProperty<FNameProperty>(Name, false))
			{
				Property->SetPropertyValue_InContainer(Parameters.GetStructMemory(), Value);
			}
		}

		void SetVector(FName Name, const FVector& Value)
		{
			FStructProperty* Property = GetProperty<FStructProperty>(Name, false);
			if (Property && Validate(Property->Struct == TBaseStructure<FVector>::Get(), TEXT("Input is a reflected FVector")))
			{
				Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &Value);
			}
		}

		void SetFloat(FName Name, float Value)
		{
			if (FFloatProperty* Property = GetProperty<FFloatProperty>(Name, false))
			{
				Property->SetPropertyValue_InContainer(Parameters.GetStructMemory(), Value);
			}
		}

		void SetBool(FName Name, bool Value)
		{
			if (FBoolProperty* Property = GetProperty<FBoolProperty>(Name, false))
			{
				Property->SetPropertyValue_InContainer(Parameters.GetStructMemory(), Value);
			}
		}

		bool Execute()
		{
			if (bValid)
			{
				Target->ProcessEvent(Function, Parameters.GetStructMemory());
			}
			return bValid;
		}

		template <typename T>
		T ReadStruct()
		{
			T Result;
			FStructProperty* Property = GetProperty<FStructProperty>(TEXT("ReturnValue"), true);
			if (Property && Validate(Property->Struct == T::StaticStruct(), TEXT("Return struct has the expected type")))
			{
				Property->CopyCompleteValue(&Result, Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()));
			}
			return Result;
		}

		UObject* ReadObject()
		{
			FObjectPropertyBase* Property = GetProperty<FObjectPropertyBase>(TEXT("ReturnValue"), true);
			return Property ? Property->GetObjectPropertyValue_InContainer(Parameters.GetStructMemory()) : nullptr;
		}

		int32 ReadInt()
		{
			FIntProperty* Property = GetProperty<FIntProperty>(TEXT("ReturnValue"), true);
			return Property ? Property->GetPropertyValue_InContainer(Parameters.GetStructMemory()) : 0;
		}

		bool ReadBool()
		{
			FBoolProperty* Property = GetProperty<FBoolProperty>(TEXT("ReturnValue"), true);
			return Property && Property->GetPropertyValue_InContainer(Parameters.GetStructMemory());
		}

		template <typename T>
		T ReadEnum()
		{
			FEnumProperty* Property = GetProperty<FEnumProperty>(TEXT("ReturnValue"), true);
			if (Property && Validate(Property->GetEnum() == StaticEnum<T>(), TEXT("Return enum has the expected type")))
			{
				return static_cast<T>(Property->GetUnderlyingProperty()->GetSignedIntPropertyValue(
					Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory())));
			}
			return static_cast<T>(0);
		}

		int32 ReadObjectArrayCount()
		{
			FArrayProperty* Property = GetProperty<FArrayProperty>(TEXT("ReturnValue"), true);
			if (Property && Validate(CastField<FObjectPropertyBase>(Property->Inner) != nullptr,
				TEXT("Returned array contains object references")))
			{
				return FScriptArrayHelper(Property, Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory())).Num();
			}
			return INDEX_NONE;
		}

	private:
		FAutomationTestBase& Test;
		UObject* Target;
		UFunction* Function;
		FStructOnScope Parameters;
		bool bValid = false;

		bool Validate(bool bCondition, const TCHAR* Label)
		{
			bValid = Test.TestTrue(Label, bCondition) && bValid;
			return bValid;
		}

		template <typename T>
		T* GetProperty(FName Name, bool bReturn)
		{
			if (!bValid) return nullptr;
			T* Property = FindFProperty<T>(Function, Name);
			if (!Validate(Property && Property->HasAnyPropertyFlags(CPF_Parm) &&
				Property->HasAnyPropertyFlags(CPF_ReturnParm) == bReturn,
				*FString::Printf(TEXT("Parameter %s has the expected reflected type and direction"), *Name.ToString())))
			{
				return nullptr;
			}
			return Property;
		}
	};

	UObject* ReadObject(FAutomationTestBase& Test, UObject* Target, FName Function)
	{
		FCall Call(Test, Target, Function, 1);
		return Call.Execute() ? Call.ReadObject() : nullptr;
	}

	int32 ReadInt(FAutomationTestBase& Test, UObject* Target, FName Function)
	{
		FCall Call(Test, Target, Function, 1);
		return Call.Execute() ? Call.ReadInt() : 0;
	}

	bool ReadBool(FAutomationTestBase& Test, UObject* Target, FName Function)
	{
		FCall Call(Test, Target, Function, 1);
		return Call.Execute() && Call.ReadBool();
	}

	FFactionResourceAmounts ReadResources(FAutomationTestBase& Test, UObject* Manager)
	{
		FCall Call(Test, Manager, TEXT("GetFactionResources"), 2);
		Call.SetEnum(TEXT("FactionID"), EFactionID::Rome);
		Call.Execute();
		return Call.ReadStruct<FFactionResourceAmounts>();
	}

	void AddGold(FAutomationTestBase& Test, UObject* Manager, int32 Gold)
	{
		FFactionResourceDelta Delta;
		Delta.Gold = Gold;
		FCall Call(Test, Manager, TEXT("ApplyFactionResourceDelta"), 3);
		Call.SetEnum(TEXT("FactionID"), EFactionID::Rome);
		Call.SetStruct(TEXT("Delta"), Delta);
		Call.Execute();
		Test.TestTrue(TEXT("Canonical wallet accepts the test transaction"),
			Call.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful());
	}

	FString ResourceLabel(const FFactionResourceAmounts& Resources)
	{
		return FString::Printf(TEXT("Aurei %d   Grain %d   Iron %d   Wood %d   Stone %d   Population %d"),
			Resources.Gold, Resources.Food, Resources.Iron, Resources.Wood, Resources.Stone, Resources.Population);
	}

	int32 ListenerCount(FAutomationTestBase& Test, UObject* Source, FName Event, UObject* Listener)
	{
		FMulticastDelegateProperty* Property = Source ? FindFProperty<FMulticastDelegateProperty>(Source->GetClass(), Event) : nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("Event %s exists"), *Event.ToString()), Property)) return INDEX_NONE;
		const FMulticastScriptDelegate* Delegate = Property->GetMulticastDelegate(Property->ContainerPtrToValuePtr<void>(Source));
		if (!Delegate) return 0;
		int32 Count = 0;
		for (UObject* Object : Delegate->GetAllObjectsEvenIfUnreachable())
		{
			if (Object == Listener) ++Count;
		}
		return Count;
	}

	bool ClickThroughSlate(FAutomationTestBase& Test, UUserWidget* Root, UButton* Button)
	{
		if (!Test.TestTrue(TEXT("Slate is initialized for native pointer routing"), FSlateApplication::IsInitialized())) return false;
		Root->ForceLayoutPrepass();
		const TSharedPtr<SWidget> SlateButton = Button->GetCachedWidget();
		if (!Test.TestTrue(TEXT("The training UButton owns a materialized SButton"),
			SlateButton.IsValid() && SlateButton->GetType() == FName(TEXT("SButton")))) return false;

		FSlateApplication& Slate = FSlateApplication::Get();
		FWidgetPath Path;
		if (!Test.TestTrue(TEXT("The visible training button has a path through the live PIE window"),
			Slate.GeneratePathToWidgetUnchecked(SlateButton.ToSharedRef(), Path) && Path.IsValid())) return false;
		for (int32 Index = 0; Index < Path.Widgets.Num() - 1; ++Index)
		{
			if (!Test.TestTrue(TEXT("Every ancestor permits pointer interaction with descendants"),
				Path.Widgets[Index].Widget->GetVisibility().AreChildrenHitTestVisible())) return false;
		}
		const FGeometry& Geometry = Path.Widgets.Last().Geometry;
		if (!Test.TestTrue(TEXT("The arranged training button has a positive clickable area"),
			Geometry.GetLocalSize().X > 0.0f && Geometry.GetLocalSize().Y > 0.0f)) return false;
		const FVector2D Position = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
		// Generated navigation paths omit per-widget pointer entries. Rebuild the path
		// with explicit empty translations before sending it to UE 5.8's event router.
		TArray<FWidgetAndPointer> PointerWidgets;
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
		{
			PointerWidgets.Emplace(Path.Widgets[Index], TOptional<FVirtualPointerPosition>());
		}
		const FWidgetPath PointerPath{MakeArrayView(PointerWidgets)};

		// Virtual Slate users have no platform cursor. These are in-process pointer events,
		// following WidgetInteractionComponent's routing, not OS input or UMG delegate calls.
		const TSharedRef<FSlateVirtualUserHandle> VirtualUser = Slate.FindOrCreateVirtualUser(25);
		const TSharedPtr<FSlateUser> User = Slate.GetUser(VirtualUser->GetUserIndex());
		if (!Test.TestTrue(TEXT("The routing fixture has no physical cursor"), User.IsValid() && User->IsVirtualUser())) return false;
		const uint32 UserIndex = static_cast<uint32>(VirtualUser->GetUserIndex());
		const uint32 PointerIndex = 0;
		const TSet<FKey> NoButtons;
		TSet<FKey> PressedButtons;
		PressedButtons.Add(EKeys::LeftMouseButton);
		const FModifierKeysState Modifiers;
		const FPointerEvent Move(UserIndex, PointerIndex, Position, Position, NoButtons, FKey(), 0.0f, Modifiers);
		const FPointerEvent Down(UserIndex, PointerIndex, Position, Position, PressedButtons, EKeys::LeftMouseButton, 0.0f, Modifiers);
		const FPointerEvent Up(UserIndex, PointerIndex, Position, Position, NoButtons, EKeys::LeftMouseButton, 0.0f, Modifiers);
		Slate.RoutePointerMoveEvent(PointerPath, Move, false);
		const FReply DownReply = Slate.RoutePointerDownEvent(PointerPath, Down);
		const FReply UpReply = Slate.RoutePointerUpEvent(PointerPath, Up);
		Slate.RoutePointerMoveEvent(FWidgetPath(), Move, false);
		User->ReleaseAllCapture();
		const bool bDownHandled = Test.TestTrue(TEXT("SButton consumes the routed pointer press before gameplay ancestors"),
			DownReply.IsEventHandled() && DownReply.GetHandler() == SlateButton);
		const bool bUpHandled = Test.TestTrue(TEXT("SButton consumes the routed pointer release before gameplay ancestors"),
			UpReply.IsEventHandled() && UpReply.GetHandler() == SlateButton);
		return bDownHandled && bUpHandled;
	}

	class FExerciseCanonicalUI : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseCanonicalUI(FAutomationTestBase* InTest) : Test(*InTest) {}

		virtual bool Update() override
		{
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			if (Stage > 0 && (!GEditor || !PlayWorld.IsValid() || GEditor->PlayWorld != PlayWorld.Get()))
			{
				Test.AddError(TEXT("The owned PIE world ended before UI lifecycle assertions completed"));
				return true;
			}
			if (Stage == 0)
			{
				UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
				if (!World || !World->HasBegunPlay() || !World->GetFirstPlayerController())
				{
					if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
					Test.AddError(TEXT("L_Testbed PIE did not reach an initialized player world within 30 seconds"));
					return true;
				}
				PlayWorld = World;
				Controller = World->GetFirstPlayerController();
				Root = Cast<UUserWidget>(ReadObject(Test, Controller, TEXT("GetMainWidget")));
				if (!Test.TestNotNull(TEXT("Production controller creates its UMG root during normal startup"), Root)) return true;
				if (!ExerciseInitialState(World)) return true;
				Root->RemoveFromParent();
				Stage = 1;
				return false; // Let Slate release the removed widget before checking NativeDestruct.
			}
			if (Stage == 1)
			{
				Test.TestFalse(TEXT("Root is detached from the viewport"), Root->IsInViewport());
				CheckBindings(0);
				const FString DetachedText = ResourceText->GetText().ToString();
				AddGold(Test, FactionManager, 19);
				Test.TestEqual(TEXT("Detached UMG does not receive wallet events"), ResourceText->GetText().ToString(), DetachedText);
				Root->AddToViewport();
				Stage = 2;
				return false;
			}
			Test.TestTrue(TEXT("The same root reattaches to the viewport"), Root->IsInViewport());
			CheckBindings(1);
			Test.TestEqual(TEXT("Reattached root catches up from the authoritative wallet"),
				ResourceText->GetText().ToString(), ResourceLabel(ReadResources(Test, FactionManager)));
			ClickThroughSlate(Test, Root, TrainButton);
			Test.TestEqual(TEXT("One click after reattachment adds exactly one pending order"),
				ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 1);
			Test.TestTrue(TEXT("Production event refreshes pending orders in UMG"),
				ProductionText->GetText().ToString().Contains(TEXT("1 pending")));
			Test.TestTrue(TEXT("Selected production actor can be destroyed in the live world"), Producer->Destroy());
			Test.TestNull(TEXT("Destroyed building is removed from canonical selection"),
				ReadObject(Test, Controller, TEXT("GetSelectedBuilding")));
			Test.TestEqual(TEXT("Destroyed building collapses the UMG selection panel"),
				Root->GetWidgetFromName(TEXT("SelectionPanel"))->GetVisibility(), ESlateVisibility::Collapsed);

			// Meet the real economic condition through its wallet, then let the campaign evaluate it.
			FCall ObjectiveCall(Test, CampaignManager, TEXT("GetObjectiveState"), 1);
			ObjectiveCall.Execute();
			const FCampaignObjectiveState Before = ObjectiveCall.ReadStruct<FCampaignObjectiveState>();
			AddGold(Test, FactionManager, Before.RequiredGold + 1000);
			FIntProperty* MaxTurnsProperty = FindFProperty<FIntProperty>(CampaignManager->GetClass(), TEXT("MaxTurns"));
			if (!Test.TestNotNull(TEXT("Campaign exposes the authored integer turn limit"), MaxTurnsProperty)) return true;
			MaxTurnsProperty->SetPropertyValue_InContainer(CampaignManager,
				ReadInt(Test, CampaignManager, TEXT("GetCurrentTurn")));
			AdvanceTurn();
			FCall OutcomeCall(Test, CampaignManager, TEXT("GetObjectiveState"), 1);
			OutcomeCall.Execute();
			const FCampaignObjectiveState After = OutcomeCall.ReadStruct<FCampaignObjectiveState>();
			Test.TestEqual(TEXT("Economic victory on the final allowed turn survives turn-limit evaluation"), After.Outcome, ECampaignOutcome::Victory);
			Test.TestFalse(TEXT("A resolved campaign is inactive"), ReadBool(Test, CampaignManager, TEXT("IsCampaignActive")));
			Test.TestTrue(TEXT("UMG displays the campaign-owned economic victory"),
				OutcomeText->GetText().ToString().Contains(TEXT("Economic victory")));
			Test.TestTrue(TEXT("Victory panel is visible"), Root->GetWidgetFromName(TEXT("OutcomePanel"))->IsVisible());
			return true;
		}

	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		int32 Stage = 0;
		TWeakObjectPtr<UWorld> PlayWorld;
		APlayerController* Controller = nullptr;
		UObject* FactionManager = nullptr;
		UObject* CampaignManager = nullptr;
		AActor* Producer = nullptr;
		UUserWidget* Root = nullptr;
		UTextBlock* ResourceText = nullptr;
		UTextBlock* PhaseText = nullptr;
		UTextBlock* ProductionText = nullptr;
		UTextBlock* OutcomeText = nullptr;
		UButton* TrainButton = nullptr;

		void CheckBindings(int32 Expected)
		{
			Test.TestEqual(TEXT("Exactly the expected number of root resource subscriptions exist"),
				ListenerCount(Test, FactionManager, TEXT("OnFactionResourcesChanged"), Root), Expected);
			Test.TestEqual(TEXT("Exactly the expected number of root turn subscriptions exist"),
				ListenerCount(Test, CampaignManager, TEXT("OnTurnProcessed"), Root), Expected);
			Test.TestEqual(TEXT("Exactly the expected number of root selection subscriptions exist"),
				ListenerCount(Test, Controller, TEXT("OnSelectionChanged"), Root), Expected);
			Test.TestEqual(TEXT("Exactly the expected number of root production subscriptions exist"),
				ListenerCount(Test, Producer, TEXT("OnProductionChanged"), Root), Expected);
		}

		void AdvanceTurn()
		{
			const int32 PreviousTurn = ReadInt(Test, CampaignManager, TEXT("GetCurrentTurn"));
			FCall Call(Test, CampaignManager, TEXT("ProcessTurn"), 1);
			Call.Execute();
			Test.TestTrue(TEXT("Canonical campaign pipeline processes the requested turn"), Call.ReadStruct<FCampaignTurnResult>().WasSuccessful());
			const int32 CurrentTurn = ReadInt(Test, CampaignManager, TEXT("GetCurrentTurn"));
			Test.TestEqual(TEXT("Only one canonical turn advances"), CurrentTurn, PreviousTurn + 1);
			Test.TestEqual(TEXT("UMG settles on the canonical mode and turn after the complete pipeline"),
				PhaseText->GetText().ToString(), FString::Printf(TEXT("Tactical   |   Turn %d"), CurrentTurn));
		}

		bool ExerciseUnitSelectionAndPossession(UWorld* World, UTextBlock* SelectionText)
		{
			UClass* UnitClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.UnitBase")).TryLoadClass<APawn>();
			if (!Test.TestNotNull(TEXT("Native unit class resolves for the live soldier fixture"), UnitClass)) return false;
			APawn* Soldier = nullptr;
			for (TActorIterator<APawn> Iterator(World, UnitClass); Iterator; ++Iterator)
			{
				FCall OwnerCall(Test, *Iterator, TEXT("GetOwnerFaction"), 1);
				OwnerCall.Execute();
				if (OwnerCall.ReadEnum<EFactionID>() == EFactionID::Rome &&
					ReadBool(Test, *Iterator, TEXT("IsAlive")) && ReadInt(Test, *Iterator, TEXT("GetCurrentHealth")) > 20)
				{
					Soldier = *Iterator;
					break;
				}
			}
			APawn* CameraPawn = Controller->GetPawn();
			UWidget* Crosshair = Root->GetWidgetFromName(TEXT("CrosshairText"));
			UTextBlock* PossessionText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("PossessionText")));
			if (!Test.TestTrue(TEXT("Normal bootstrap supplies a living Rome soldier, camera, and possession controls"),
				Soldier && CameraPawn && Crosshair && PossessionText)) return false;

			FCall SelectCall(Test, Controller, TEXT("SelectUnit"), 1);
			SelectCall.SetObject(TEXT("Unit"), Soldier);
			SelectCall.Execute();
			Test.TestTrue(TEXT("Selecting a live soldier updates canonical unit selection"), ReadBool(Test, Soldier, TEXT("IsSelected")));
			const FString BeforeDamageText = SelectionText->GetText().ToString();
			const int32 HealthBefore = ReadInt(Test, Soldier, TEXT("GetCurrentHealth"));
			FCall DamageCall(Test, Soldier, TEXT("TakeCombatDamage"), 3);
			DamageCall.SetFloat(TEXT("Damage"), 12.0f);
			DamageCall.SetObject(TEXT("DamageSource"), nullptr);
			DamageCall.SetBool(TEXT("bIsRanged"), false);
			DamageCall.Execute();
			const int32 HealthAfter = ReadInt(Test, Soldier, TEXT("GetCurrentHealth"));
			Test.TestTrue(TEXT("Combat rules reduce the selected soldier's health without killing it"), HealthAfter > 0 && HealthAfter < HealthBefore);
			const FString HealthLabel = FString::Printf(TEXT("HP %d / %d"), HealthAfter, ReadInt(Test, Soldier, TEXT("GetMaxHealth")));
			Test.TestTrue(TEXT("Unit vitals event changes selected HP text synchronously"),
				SelectionText->GetText().ToString() != BeforeDamageText && SelectionText->GetText().ToString().Contains(HealthLabel));

			FCall PossessCall(Test, Controller, TEXT("EnterFirstPersonMode"), 1);
			PossessCall.SetObject(TEXT("UnitToPossess"), Soldier);
			PossessCall.Execute();
			Test.TestTrue(TEXT("Entering possession transfers the controller to the selected soldier"), Controller->GetPawn() == Soldier);
			Test.TestTrue(TEXT("Possession changes the authoritative controller mode"), ReadBool(Test, Controller, TEXT("IsInFirstPersonMode")));
			Test.TestTrue(TEXT("Possession events display crosshair and soldier vitals"), Crosshair->IsVisible() && PossessionText->IsVisible());
			Test.TestTrue(TEXT("Possession UI reads the same damaged health"), PossessionText->GetText().ToString().Contains(HealthLabel));

			FCall KillCall(Test, Soldier, TEXT("SetCurrentHealth"), 1);
			KillCall.SetInt(TEXT("NewHealth"), 0);
			KillCall.Execute();
			Test.TestFalse(TEXT("Canonical health transition kills the possessed soldier"), ReadBool(Test, Soldier, TEXT("IsAlive")));
			Test.TestTrue(TEXT("Possessed soldier death restores the original camera pawn"), Controller->GetPawn() == CameraPawn);
			Test.TestFalse(TEXT("Death exits the controller's possession mode"), ReadBool(Test, Controller, TEXT("IsInFirstPersonMode")));
			FCall SelectedCall(Test, Controller, TEXT("GetSelectedUnits"), 1);
			SelectedCall.Execute();
			Test.TestEqual(TEXT("Death removes the soldier from canonical selection"), SelectedCall.ReadObjectArrayCount(), 0);
			Test.TestEqual(TEXT("Death event collapses the possession crosshair"), Crosshair->GetVisibility(), ESlateVisibility::Collapsed);
			Test.TestEqual(TEXT("Death event collapses soldier vitals"), PossessionText->GetVisibility(), ESlateVisibility::Collapsed);
			Test.TestEqual(TEXT("Death leaves no stale unit selection panel"),
				Root->GetWidgetFromName(TEXT("SelectionPanel"))->GetVisibility(), ESlateVisibility::Collapsed);
			return true;
		}

		bool ExerciseInitialState(UWorld* World)
		{
			Test.TestTrue(TEXT("Production UMG is attached to the player viewport"), Root->IsInViewport());
			if (!Test.TestNotNull(TEXT("Native root has a UMG widget tree"), Root->WidgetTree.Get()) ||
				!Test.TestNotNull(TEXT("Native tree has a root widget"), Root->WidgetTree->RootWidget.Get())) return false;
			Test.TestTrue(TEXT("The native UMG root materializes into Slate"), Root->GetCachedWidget().IsValid());
			ResourceText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ResourceText")));
			PhaseText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("PhaseText")));
			ProductionText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ProductionText")));
			OutcomeText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("OutcomeText")));
			TrainButton = Cast<UButton>(Root->GetWidgetFromName(TEXT("TrainButton")));
			UTextBlock* DiagnosticText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("DiagnosticText")));
			UTextBlock* SelectionText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("SelectionText")));
			UTextBlock* ObjectiveText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ObjectiveText")));
			if (!Test.TestTrue(TEXT("All production UI controls materialize as the expected UMG types"),
				ResourceText && PhaseText && ProductionText && OutcomeText && TrainButton && DiagnosticText && SelectionText && ObjectiveText &&
				Root->GetWidgetFromName(TEXT("BuildingMenu")) && Root->GetWidgetFromName(TEXT("OutcomePanel")))) return false;

			AGameModeBase* GameMode = World->GetAuthGameMode();
			FactionManager = ReadObject(Test, GameMode, TEXT("GetFactionManager"));
			CampaignManager = ReadObject(Test, GameMode, TEXT("GetCampaignManager"));
			if (!Test.TestTrue(TEXT("Normal startup supplies the canonical managers"), FactionManager && CampaignManager)) return false;
			FCall ModeCall(Test, GameMode, TEXT("SetGameplayMode"), 1);
			ModeCall.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical);
			ModeCall.Execute();
			Test.TestEqual(TEXT("Startup UI reads the real wallet"), ResourceText->GetText().ToString(), ResourceLabel(ReadResources(Test, FactionManager)));
			AddGold(Test, FactionManager, 137);
			const FFactionResourceAmounts AfterDelta = ReadResources(Test, FactionManager);
			Test.TestEqual(TEXT("Resource event updates visible text synchronously without a UI tick"),
				ResourceText->GetText().ToString(), ResourceLabel(AfterDelta));
			Test.TestTrue(TEXT("Diagnostic text observes the same canonical gold"),
				DiagnosticText->GetText().ToString().Contains(FString::Printf(TEXT("Gold=%d"), AfterDelta.Gold)));
			Test.TestTrue(TEXT("Objective text observes the same economic progress"),
				ObjectiveText->GetText().ToString().Contains(FString::Printf(TEXT("Economy: %d /"), AfterDelta.Gold)));
			AdvanceTurn();
			if (!ExerciseUnitSelectionAndPossession(World, SelectionText)) return false;

			UClass* BarracksClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.Barracks")).TryLoadClass<AActor>();
			if (!Test.TestNotNull(TEXT("Native production class is available"), BarracksClass)) return false;
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Producer = World->SpawnActor<AActor>(BarracksClass, FVector(50000.0, 50000.0, 300.0), FRotator::ZeroRotator, SpawnParameters);
			AActor* OtherProducer = World->SpawnActor<AActor>(BarracksClass, FVector(51000.0, 50000.0, 300.0), FRotator::ZeroRotator, SpawnParameters);
			if (!Test.TestTrue(TEXT("Two independent production actors initialize in the live world"), Producer && OtherProducer)) return false;
			FCall OwnerCall(Test, Producer, TEXT("SetOwnerFaction"), 1);
			OwnerCall.SetEnum(TEXT("NewOwner"), EFactionID::Rome);
			OwnerCall.Execute();
			FCall DataCall(Test, Producer, TEXT("GetBuildingData"), 1);
			DataCall.Execute();
			const FBuildingData Data = DataCall.ReadStruct<FBuildingData>();
			FCall(Test, Producer, TEXT("StartConstruction"), 0).Execute();
			Producer->Tick(Data.ConstructionTime + 0.1f);
			Test.TestTrue(TEXT("Construction completes through the building simulation"), ReadBool(Test, Producer, TEXT("IsComplete")));
			// Queue assertions must not depend on rendering speed between latent frames.
			Producer->SetActorTickEnabled(false);
			FCall SelectCall(Test, Controller, TEXT("SelectBuilding"), 1);
			SelectCall.SetObject(TEXT("Building"), Producer);
			SelectCall.Execute();
			Test.TestTrue(TEXT("Selection event shows the selected building's display name"), SelectionText->GetText().ToString().Contains(Data.DisplayName.ToString()));
			Test.TestTrue(TEXT("The selected completed producer exposes its UMG training button"), TrainButton->IsVisible());
			CheckBindings(1);
			ClickThroughSlate(Test, Root, TrainButton);
			Test.TestTrue(TEXT("UMG click starts training on the selected producer"), ReadBool(Test, Producer, TEXT("IsTraining")));
			Test.TestFalse(TEXT("UMG click leaves the other producer idle"), ReadBool(Test, OtherProducer, TEXT("IsTraining")));
			Test.TestEqual(TEXT("One initial click accepts exactly one order"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 0);
			Test.TestEqual(TEXT("The UMG click preserves the canonical building selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Producer));
			Test.TestTrue(TEXT("Production event refreshes active training in UMG"), ProductionText->GetText().ToString().Contains(TEXT("active")));
			Test.TestEqual(TEXT("Training cost is reflected from the updated canonical wallet"), ResourceText->GetText().ToString(), ResourceLabel(ReadResources(Test, FactionManager)));
			return true;
		}
	};

	class FWaitForOwnedPIEToEnd : public IAutomationLatentCommand
	{
	public:
		explicit FWaitForOwnedPIEToEnd(FAutomationTestBase* InTest) : Test(*InTest) {}

		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld) return true;
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
			Test.AddError(TEXT("The UI integration test's PIE session did not tear down within 30 seconds"));
			return true;
		}

	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanonicalUIIntegrationTest,
	"RomanEmpireGame.UI.CanonicalEventDrivenPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCanonicalUIIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestNotNull(TEXT("PIE UI integration requires an editor"), GEditor)) return false;
	if (!TestNull(TEXT("Test owns its PIE session and does not reuse an existing one"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("The committed test map loads before PIE is requested"),
		EditorWorld && EditorWorld->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCanonicalUI(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

void UPlacementTransactionProbe::OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance)
{
	(void)FactionID;
	(void)NewBalance;
	using namespace RomanEmpireUITests;
	++WalletNotifications;
	// Bound recursion ensures a broken implementation reports failure, not a stack overflow.
	if (!Test || WalletNotifications > 1) return;
	Test->TestFalse(TEXT("Wallet observer cannot recursively confirm the same placement"),
		ReadBool(*Test, Placement.Get(), TEXT("ConfirmPlacement")));
	Test->TestFalse(TEXT("Placement is not actionable while committing"),
		ReadBool(*Test, Placement.Get(), TEXT("CanPlace")));
	if (bCancelDuringSpend) FCall(*Test, Placement.Get(), TEXT("CancelPlacement"), 0).Execute();
	FCall Start(*Test, Placement.Get(), TEXT("StartPlacement"), 1);
	Start.SetName(TEXT("BuildingID"), BuildingID);
	Start.Execute();
}

void UPlacementTransactionProbe::OnPlaced(ABuildingBase* Building)
{
	(void)Building;
	using namespace RomanEmpireUITests;
	++PlacedNotifications;
	if (!Test || PlacedNotifications > 1) return;
	Test->TestFalse(TEXT("Placed observer sees the preview already consumed"),
		ReadBool(*Test, Placement.Get(), TEXT("IsPlacing")));
	Test->TestFalse(TEXT("Placed observer cannot recursively confirm"),
		ReadBool(*Test, Placement.Get(), TEXT("ConfirmPlacement")));
}

namespace RomanEmpireUITests
{
	// Change a real loaded definition before normal PIE startup; never save it. Holding
	// the original and restoring at scope exit also protects abort/timeout paths.
	class FScopedBuildingDefinition
	{
	public:
		explicit FScopedBuildingDefinition(URomanBuildingDefinition* InAsset)
			: Asset(InAsset), Original(InAsset->Definition), bWasDirty(InAsset->GetPackage()->IsDirty())
		{
			Asset->Definition.DisplayName = FText::FromString(TEXT("Audit Barracks Variant"));
			Asset->Definition.Cost.Gold = 37;
			Asset->Definition.Cost.Food = 11;
			Asset->Definition.Cost.Iron = 13;
			Asset->Definition.Cost.Wood = 17;
			Asset->Definition.Cost.Stone = 19;
			Asset->Definition.Cost.Population = 2;
			Asset->Definition.ConstructionTime = 3.25f;
			Asset->Definition.MaxHealth = 2468;
		}
		~FScopedBuildingDefinition() { Restore(); }
		void Restore()
		{
			if (bRestored) return;
			Asset->Definition = Original;
			Asset->GetPackage()->SetDirtyFlag(bWasDirty);
			bRestored = true;
		}
		TStrongObjectPtr<URomanBuildingDefinition> Asset;
		FBuildingData Original;
		bool bWasDirty;
	private:
		bool bRestored = false;
	};

	class FScopedProbeBinding
	{
	public:
		FScopedProbeBinding(FAutomationTestBase& Test, UObject* InSource, FName Event,
			UPlacementTransactionProbe* Probe, FName Handler) : Source(InSource)
		{
			Property = InSource ? FindFProperty<FMulticastDelegateProperty>(InSource->GetClass(), Event) : nullptr;
			UFunction* Function = Probe->FindFunction(Handler);
			bValid = Test.TestTrue(TEXT("Regression probe matches the real event signature"),
				Property && Function && Property->SignatureFunction->IsSignatureCompatibleWith(Function));
			if (bValid)
			{
				Delegate.BindUFunction(Probe, Handler);
				Property->AddDelegate(Delegate, Source.Get());
			}
		}
		~FScopedProbeBinding()
		{
			if (bValid && Source.IsValid()) Property->RemoveDelegate(Delegate, Source.Get());
		}
		bool bValid = false;
	private:
		TWeakObjectPtr<UObject> Source;
		FMulticastDelegateProperty* Property = nullptr;
		FScriptDelegate Delegate;
	};

	class FExerciseDefinitionPlacement : public IAutomationLatentCommand
	{
	public:
		FExerciseDefinitionPlacement(FAutomationTestBase* InTest, TSharedRef<FScopedBuildingDefinition> InDefinition)
			: Test(*InTest), Definition(InDefinition) {}
		virtual bool Update() override
		{
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			if (!World || !World->HasBegunPlay() || !Controller)
			{
				if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
				Test.AddError(TEXT("Definition/placement test did not reach PIE startup within 30 seconds"));
				return true;
			}
			if (!bRequestedTactical)
			{
				Controller->SetActorTickEnabled(false); // No OS cursor updates between assertions.
				FCall Mode(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
				Mode.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical);
				Mode.Execute();
				bRequestedTactical = true;
				return false; // Enhanced Input's context rebuild is deferred to the next frame.
			}
			if (!Test.TestTrue(TEXT("Normal Tactical eligibility is active"),
				ReadBool(Test, Controller, TEXT("CanIssueTacticalCommands")))) return true;
			Exercise(World, Controller);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		TSharedRef<FScopedBuildingDefinition> Definition;
		double StartedAt = 0.0;
		bool bRequestedTactical = false;

		void Exercise(UWorld* World, APlayerController* Controller)
		{
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(World);
			const FBuildingData Expected = Definition->Asset->Definition;
			if (!Test.TestTrue(TEXT("Normal registry resolves the edited authored asset by its stable ID"),
				Registry && Registry->IsReady() && Registry->FindBuilding(Expected.BuildingID) == Definition->Asset.Get())) return;
			UUserWidget* Root = Cast<UUserWidget>(ReadObject(Test, Controller, TEXT("GetMainWidget")));
			UUserWidget* Menu = Root ? Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("BuildingMenu"))) : nullptr;
			UTextBlock* Label = Menu ? Cast<UTextBlock>(Menu->GetWidgetFromName(
				*FString::Printf(TEXT("BuildLabel_%s"), *Expected.BuildingID.ToString()))) : nullptr;
			if (!Test.TestNotNull(TEXT("Normal UMG startup creates the definition's menu label"), Label)) return;
			Test.TestEqual(TEXT("Edited name and all costs reach the production menu without runtime constants"),
				Label->GetText().ToString(), FString::Printf(TEXT("[%d] %s  |  %s"), Definition->Asset->QuickSlot,
					*Expected.DisplayName.ToString(), *RomanEmpireEconomy::DescribeResourceCost(Expected.Cost)));
			UObject* Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetFactionManager"));
			UClass* PlacementClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.BuildingPlacementComponent")).TryLoadClass<UActorComponent>();
			UActorComponent* Placement = PlacementClass ? Controller->GetComponentByClass(PlacementClass) : nullptr;
			UClass* BuildingClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.BuildingBase")).TryLoadClass<AActor>();
			if (!Test.TestTrue(TEXT("Real placement, building class and faction authority exist"), Placement && BuildingClass && Manager)) return;
			Placement->SetComponentTickEnabled(false);
			for (int32 Scenario = 0; Scenario < 2; ++Scenario)
			{
				Test.PushContext(Scenario == 0 ? TEXT("Recursive observers") : TEXT("Cancel and restart during committed spend"));
				FCall Start(Test, Placement, TEXT("StartPlacement"), 1);
				Start.SetName(TEXT("BuildingID"), Expected.BuildingID);
				Start.Execute();
				FCall Move(Test, Placement, TEXT("UpdatePreview"), 1);
				Move.SetVector(TEXT("WorldLocation"), FVector(50000.0 + Scenario * 2000.0, 50000.0, 300.0));
				Move.Execute();
				Test.TestTrue(TEXT("Preview can be committed through the public command"), ReadBool(Test, Placement, TEXT("CanPlace")));
				TSet<AActor*> BeforeActors;
				for (TActorIterator<AActor> It(World, BuildingClass); It; ++It) BeforeActors.Add(*It);
				const FFactionResourceAmounts Before = ReadResources(Test, Manager);
				TStrongObjectPtr<UPlacementTransactionProbe> Probe(NewObject<UPlacementTransactionProbe>());
				Probe->Test = &Test;
				Probe->Placement = Placement;
				Probe->BuildingID = Expected.BuildingID;
				Probe->bCancelDuringSpend = Scenario == 1;
				FScopedProbeBinding WalletBinding(Test, Manager, TEXT("OnFactionResourcesChanged"), Probe.Get(), TEXT("OnResourcesChanged"));
				FScopedProbeBinding PlacedBinding(Test, Placement, TEXT("OnBuildingPlaced"), Probe.Get(), TEXT("OnPlaced"));
				if (!WalletBinding.bValid || !PlacedBinding.bValid) { Test.PopContext(); return; }
				Test.TestTrue(TEXT("Outer placement command succeeds"), ReadBool(Test, Placement, TEXT("ConfirmPlacement")));
				Test.TestEqual(TEXT("Exactly one wallet event (no duplicate charge or compensating refund)"), Probe->WalletNotifications, 1);
				Test.TestEqual(TEXT("Exactly one building-placed event"), Probe->PlacedNotifications, 1);
				const FFactionResourceAmounts After = ReadResources(Test, Manager);
				Test.TestEqual(TEXT("Gold charged once"), Before.Gold - After.Gold, Expected.Cost.Gold);
				Test.TestEqual(TEXT("Food charged once"), Before.Food - After.Food, Expected.Cost.Food);
				Test.TestEqual(TEXT("Iron charged once"), Before.Iron - After.Iron, Expected.Cost.Iron);
				Test.TestEqual(TEXT("Wood charged once"), Before.Wood - After.Wood, Expected.Cost.Wood);
				Test.TestEqual(TEXT("Stone charged once"), Before.Stone - After.Stone, Expected.Cost.Stone);
				Test.TestEqual(TEXT("Population charged once"), Before.Population - After.Population, Expected.Cost.Population);
				Test.TestFalse(TEXT("No preview remains after event-issued restart attempt"), ReadBool(Test, Placement, TEXT("IsPlacing")));
				TArray<AActor*> Created;
				for (TActorIterator<AActor> It(World, BuildingClass); It; ++It)
				{
					if (!BeforeActors.Contains(*It)) Created.Add(*It);
				}
				if (Test.TestEqual(TEXT("Exactly one actual building was created"), Created.Num(), 1))
				{
					AActor* Building = Created[0];
					FCall Data(Test, Building, TEXT("GetBuildingData"), 1);
					Data.Execute();
					const FBuildingData Actual = Data.ReadStruct<FBuildingData>();
					Test.TestEqual(TEXT("Factory retains the authored ID"), Actual.BuildingID, Expected.BuildingID);
					Test.TestEqual(TEXT("Factory receives changed display name"), Actual.DisplayName.ToString(), Expected.DisplayName.ToString());
					Test.TestEqual(TEXT("Factory receives changed hit points"), Actual.MaxHealth, Expected.MaxHealth);
					Test.TestEqual(TEXT("Factory receives changed construction duration"), Actual.ConstructionTime, Expected.ConstructionTime);
					Building->Tick(Expected.ConstructionTime - 0.1f);
					Test.TestFalse(TEXT("Building stays incomplete before the edited duration"), ReadBool(Test, Building, TEXT("IsComplete")));
					Building->Tick(0.2f);
					Test.TestTrue(TEXT("Building completes after the edited duration"), ReadBool(Test, Building, TEXT("IsComplete")));
					FCall Select(Test, Controller, TEXT("SelectBuilding"), 1);
					Select.SetObject(TEXT("Building"), Building);
					Select.Execute();
					UTextBlock* Selection = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("SelectionText")));
					Test.TestTrue(TEXT("Runtime selection UI displays the changed authored name"),
						Selection && Selection->GetText().ToString().Contains(Expected.DisplayName.ToString()));
				}
				Test.PopContext();
			}

			// A rejection must not consume the preview, charge any currency, or leave a
			// deferred actor behind. The same command becomes retryable after funding.
			AddGold(Test, Manager, -ReadResources(Test, Manager).Gold);
			FCall Start(Test, Placement, TEXT("StartPlacement"), 1);
			Start.SetName(TEXT("BuildingID"), Expected.BuildingID);
			Start.Execute();
			FCall Move(Test, Placement, TEXT("UpdatePreview"), 1);
			Move.SetVector(TEXT("WorldLocation"), FVector(55000.0, 50000.0, 300.0));
			Move.Execute();
			int32 BeforeCount = 0;
			for (TActorIterator<AActor> It(World, BuildingClass); It; ++It) ++BeforeCount;
			const FFactionResourceAmounts BeforeRejection = ReadResources(Test, Manager);
			Test.AddExpectedMessage(TEXT("Cannot confirm placement: resource transaction failed"),
				ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			Test.TestFalse(TEXT("Unfunded placement is rejected"), ReadBool(Test, Placement, TEXT("ConfirmPlacement")));
			FCall Failure(Test, Placement, TEXT("GetLastTransactionFailure"), 1);
			Failure.Execute();
			Test.TestEqual(TEXT("Placement reports the canonical insufficient-gold failure"),
				Failure.ReadEnum<EResourceTransactionFailure>(), EResourceTransactionFailure::InsufficientGold);
			const FFactionResourceAmounts AfterRejection = ReadResources(Test, Manager);
			Test.TestTrue(TEXT("Rejected placement preserves every wallet field"),
				FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&BeforeRejection, &AfterRejection, 0));
			int32 AfterCount = 0;
			for (TActorIterator<AActor> It(World, BuildingClass); It; ++It) ++AfterCount;
			Test.TestEqual(TEXT("Rejected deferred spawn leaves no extra building"), AfterCount, BeforeCount);
			Test.TestTrue(TEXT("Rejected preview remains retryable"), ReadBool(Test, Placement, TEXT("IsPlacing")));
			AddGold(Test, Manager, Expected.Cost.Gold);
			Test.TestTrue(TEXT("Original preview succeeds after funding"), ReadBool(Test, Placement, TEXT("ConfirmPlacement")));
			Test.TestEqual(TEXT("Retry deducts exactly its newly funded gold"), ReadResources(Test, Manager).Gold, 0);
			Test.TestFalse(TEXT("Successful retry consumes the preview"), ReadBool(Test, Placement, TEXT("IsPlacing")));
		}
	};

	class FRestoreDefinition : public IAutomationLatentCommand
	{
	public:
		FRestoreDefinition(FAutomationTestBase* InTest, TSharedRef<FScopedBuildingDefinition> InDefinition)
			: Test(*InTest), Definition(InDefinition) {}
		virtual bool Update() override
		{
			Definition->Restore();
			Test.TestTrue(TEXT("Authored definition restored after PIE"), FBuildingData::StaticStruct()->CompareScriptStruct(
				&Definition->Asset->Definition, &Definition->Original, 0));
			Test.TestEqual(TEXT("Original package dirty state preserved; fixture never saves assets"),
				Definition->Asset->GetPackage()->IsDirty(), Definition->bWasDirty);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		TSharedRef<FScopedBuildingDefinition> Definition;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDefinitionPlacementIntegrationTest,
	"RomanEmpireGame.Content.DefinitionPropagationAndAtomicPlacementPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDefinitionPlacementIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestNotNull(TEXT("Definition integration requires an editor"), GEditor) ||
		!TestNull(TEXT("Definition test owns a new PIE session"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loads"), EditorWorld && EditorWorld->GetName() == TEXT("L_Testbed"))) return false;
	URomanBuildingDefinition* Asset = LoadObject<URomanBuildingDefinition>(nullptr,
		TEXT("/Game/Data/Buildings/PDA_Building_Barracks.PDA_Building_Barracks"));
	if (!TestNotNull(TEXT("Authored Barracks definition loads"), Asset)) return false;
	const TSharedRef<RomanEmpireUITests::FScopedBuildingDefinition> Definition = MakeShared<RomanEmpireUITests::FScopedBuildingDefinition>(Asset);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseDefinitionPlacement(this, Definition));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FRestoreDefinition(this, Definition));
	return true;
}

#endif
