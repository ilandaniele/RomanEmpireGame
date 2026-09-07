// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Building/BuildingTypes.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
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
#include "UObject/ScriptDelegates.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StructOnScope.h"
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

#endif
