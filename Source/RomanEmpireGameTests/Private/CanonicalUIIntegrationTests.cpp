// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "VisualRegression.h"
#include "PlacementTransactionProbe.h"
#include "BuildingSelectionProbe.h"
#include "TerritoryProjectionProbe.h"
#include "TrainingRefundProbe.h"
#include "CampaignArmyProbe.h"
#include "CampaignArmyActionsProbe.h"
#include "CampaignAIExecutionProbe.h"
#include "CampaignEncounterProbe.h"
#include "CampaignReinforcementProbe.h"
#include "CampaignTravelProbe.h"
#include "RomanCampaignFunctional.h"
#include "CampaignSaveProbe.h"
#include "Campaign/CampaignSaveSubsystem.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Campaign/CampaignAIPlanning.h"
#include "UI/MinimapPresentation.h"
#include "UI/RomanPlayerSettingsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Building/TrainingReservation.h"
#include "Templates/UnrealTemplate.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Battle/TacticalObjective.h"
#include "Building/BuildingTypes.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/CampaignTurn.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ActorComponent.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/StaticMeshComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Core/GameplayMode.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Engine/UserInterfaceSettings.h"
#include "Widgets/SWindow.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Faction/FactionData.h"
#include "Faction/DiplomacyState.h"
#include "Units/UnitOrderState.h"
#include "Units/RomanUnitRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Layout/ArrangedWidget.h"
#include "Layout/Geometry.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Class.h"
#include "UObject/GarbageCollection.h"
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

		void SetGuid(FName Name, const FGuid& Value)
		{
			FStructProperty* Property = GetProperty<FStructProperty>(Name, false);
			if (Property && Validate(Property->Struct == TBaseStructure<FGuid>::Get(), TEXT("Input is a GUID")))
				Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &Value);
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

		void SetText(FName Name, const FText& Value)
		{
			if (FTextProperty* Property = GetProperty<FTextProperty>(Name, false))
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

		FGuid ReadGuid()
		{
			FStructProperty* Property = GetProperty<FStructProperty>(TEXT("ReturnValue"), true);
			if (Property && Validate(Property->Struct == TBaseStructure<FGuid>::Get(), TEXT("Return struct has GUID type")))
				return *Property->ContainerPtrToValuePtr<FGuid>(Parameters.GetStructMemory());
			return FGuid();
		}

		FName ReadName()
		{
			FNameProperty* Property = GetProperty<FNameProperty>(TEXT("ReturnValue"), true);
			return Property ? Property->GetPropertyValue_InContainer(Parameters.GetStructMemory()) : FName(NAME_None);
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

		template <typename T>
		TArray<T> ReadEnumArray()
		{
			TArray<T> Result;
			FArrayProperty* Property = GetProperty<FArrayProperty>(TEXT("ReturnValue"), true);
			FEnumProperty* Inner = Property ? CastField<FEnumProperty>(Property->Inner) : nullptr;
			if (Property && Validate(Inner && Inner->GetEnum() == StaticEnum<T>(), TEXT("Returned array has the expected enum type")))
			{
				Property->CopyCompleteValue(&Result, Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()));
			}
			return Result;
		}

		template <typename T>
		TArray<T> ReadStructArray()
		{
			TArray<T> Result;
			FArrayProperty* Property = GetProperty<FArrayProperty>(TEXT("ReturnValue"), true);
			FStructProperty* Inner = Property ? CastField<FStructProperty>(Property->Inner) : nullptr;
			if (Property && Validate(Inner && Inner->Struct == T::StaticStruct(), TEXT("Returned array contains expected struct")))
				Property->CopyCompleteValue(&Result, Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()));
			return Result;
		}

		TArray<FName> ReadNameArray()
		{
			TArray<FName> Result;
			FArrayProperty* Property = GetProperty<FArrayProperty>(TEXT("ReturnValue"), true);
			if (Property && Validate(CastField<FNameProperty>(Property->Inner) != nullptr,
				TEXT("Returned array contains stable names")))
			{
				Property->CopyCompleteValue(&Result, Property->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()));
			}
			return Result;
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

	FFactionResourceAmounts ReadResources(FAutomationTestBase& Test, UObject* Manager, EFactionID Faction = EFactionID::Rome)
	{
		FCall Call(Test, Manager, TEXT("GetFactionResources"), 2);
		Call.SetEnum(TEXT("FactionID"), Faction);
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
		if (!Test.TestNotNull(TEXT("HUD button exists for pointer routing"), Button)) return false;
		if (!Test.TestTrue(TEXT("Slate is initialized for native pointer routing"), FSlateApplication::IsInitialized())) return false;
		Root->ForceLayoutPrepass();
		const TSharedPtr<SWidget> SlateButton = Button->GetCachedWidget();
		if (!Test.TestTrue(TEXT("The HUD UButton owns a materialized SButton"),
			SlateButton.IsValid() && SlateButton->GetType() == FName(TEXT("SButton")))) return false;

		FSlateApplication& Slate = FSlateApplication::Get();
		FWidgetPath Path;
		if (!Test.TestTrue(TEXT("The visible HUD button has a path through the live PIE window"),
			Slate.GeneratePathToWidgetUnchecked(SlateButton.ToSharedRef(), Path) && Path.IsValid())) return false;
		for (int32 Index = 0; Index < Path.Widgets.Num() - 1; ++Index)
		{
			if (!Test.TestTrue(TEXT("Every ancestor permits pointer interaction with descendants"),
				Path.Widgets[Index].Widget->GetVisibility().AreChildrenHitTestVisible())) return false;
		}
		const FGeometry& Geometry = Path.Widgets.Last().Geometry;
		if (Geometry.GetLocalSize().X <= 0 || Geometry.GetLocalSize().Y <= 0)
		{
			Test.AddInfo(FString::Printf(TEXT("Zero layout for %s"), *Button->GetName()));
			for (int32 I=0; I<Path.Widgets.Num(); ++I)
				Test.AddInfo(FString::Printf(TEXT("  %s : %s"), *Path.Widgets[I].Widget->GetType().ToString(), *Path.Widgets[I].Geometry.GetLocalSize().ToString()));
		}
		if (!Test.TestTrue(TEXT("The arranged HUD button has a positive clickable area"),
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

	class FStartUIWindowPIE : public IAutomationLatentCommand
	{
	public:
		FStartUIWindowPIE(int32 InWidth = 1280, int32 InHeight = 720) : Width(InWidth), Height(InHeight) {}
		virtual bool Update() override
		{
			// A docked viewport is zero-sized under NullRHI. Use an actual PIE
			// window, not synthetic per-button geometry or a detached widget tree.
			FRequestPlaySessionParams Params;
			auto* Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
			Settings->NewWindowWidth = Width;
			Settings->NewWindowHeight = Height;
			Settings->CenterNewWindow = true; // Do not clamp an offscreen test to the desktop work area.
			Params.EditorPlaySettings = Settings;
			const TSharedRef<SWindow> Window = SNew(SWindow)
				.Title(FText::FromString(TEXT("Roman UI automation")))
				.ClientSize(FVector2D(Width,Height)).ScreenPosition(FVector2D::ZeroVector)
				.CreateTitleBar(false).UseOSWindowBorder(false).SizingRule(ESizingRule::FixedSize)
				.LayoutBorder(FMargin(0)).UserResizeBorder(FMargin(0))
				.AutoCenter(EAutoCenter::None).SaneWindowPlacement(false)
				.AdjustInitialSizeAndPositionForDPIScale(false);
			FSlateApplication::Get().AddWindow(Window);
			Params.CustomPIEWindow = Window;
			GEditor->RequestPlaySession(Params);
			return true;
		}
	private:
		int32 Width, Height;
	};

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
				const FFactionResourceAmounts BeforeDetachedCancel = ReadResources(Test, FactionManager);
				FCall(Test, Root, TEXT("HandleCancelTrainingClicked"), 0).Execute();
				CheckTrainingCharge(TEXT("Detached cancellation callback"), BeforeDetachedCancel, FFactionResourceAmounts());
				Test.TestFalse(TEXT("Detached cancel control is disabled"),
					CastChecked<UButton>(Root->GetWidgetFromName(TEXT("CancelTrainingButton")))->GetIsEnabled());
				const FString DetachedText = ResourceText->GetText().ToString();
				AddGold(Test, FactionManager, 19);
				CheckGameStateProjection();
				Test.TestEqual(TEXT("Detached UMG does not receive wallet events"), ResourceText->GetText().ToString(), DetachedText);
				const FString DetachedDetail = TerritoryDetail()->GetText().ToString();
				SetMode(EGameplayMode::Strategic);
				ChangeOwner(TEXT("Gaul"), EFactionID::Rome);
				Test.TestTrue(TEXT("Detached founding commits without UI listener"), FoundSettlement(TEXT("Egypt"), TEXT("Alexandria Test")).WasSuccessful());
				Test.TestFalse(TEXT("Detached territorial facade rejects selection"), SelectTerritory(TEXT("Gaul")));
				Test.TestEqual(TEXT("Detached territorial panel receives no mode/ownership update"),
					TerritoryDetail()->GetText().ToString(), DetachedDetail);
				Root->AddToViewport();
				Stage = 2;
				return false;
			}
			Test.TestTrue(TEXT("The same root reattaches to the viewport"), Root->IsInViewport());
			CheckBindings(1);
			CheckGameStateProjection();
			Test.TestTrue(TEXT("Reattached panel catches up to current Strategic mode"), Root->GetWidgetFromName(TEXT("StrategicPanel"))->IsVisible());
			FCall EmptySelection(Test, Root, TEXT("GetSelectedTerritoryID"), 1);
			EmptySelection.Execute();
			Test.TestTrue(TEXT("Detach discarded local inspection selection"), EmptySelection.ReadName().IsNone());
			ClickThroughSlate(Test, Root, TerritoryButton(TEXT("Gaul")));
			CheckStrategicDetail(TEXT("Gaul"));
			Test.TestTrue(TEXT("Reattached UI sees changed dormant-owner metadata and participation"),
				TerritoryDetail()->GetText().ToString().Contains(TEXT("Active participant")));
			ChangeOwner(TEXT("Gaul"), EFactionID::Gaul);
			CheckStrategicDetail(TEXT("Gaul"));
			ClickThroughSlate(Test, Root, TerritoryButton(TEXT("Egypt")));
			CheckStrategicDetail(TEXT("Egypt"));
			Test.TestTrue(TEXT("Reattached inspector catches live founding missed while detached"),
				TerritoryDetail()->GetText().ToString().Contains(TEXT("Alexandria Test")));
			SetMode(EGameplayMode::Tactical);
			Test.TestEqual(TEXT("Reattached root catches up from the authoritative wallet"),
				ResourceText->GetText().ToString(), ResourceLabel(ReadResources(Test, FactionManager)));
			const FFactionResourceAmounts BeforeQueuedOrder = ReadResources(Test, FactionManager);
			ClickThroughSlate(Test, Root, TrainButton);
			CheckTrainingCharge(TEXT("Queued Slate TRAIN after reattachment"), BeforeQueuedOrder, TrainingCost);
			Test.TestEqual(TEXT("One click after reattachment adds exactly one pending order"),
				ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 1);
			Test.TestTrue(TEXT("Production event refreshes pending orders in UMG"),
				ProductionText->GetText().ToString().Contains(TEXT("1 pending")));
			if (!ExerciseTrainingTransactions()) return true;
			ExerciseCancellationRefunds();
			ExerciseFailedTrainingRefunds();
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
			CheckGameStateProjection();
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
			CheckGameStateProjection();
			Test.TestTrue(TEXT("Owned GameState can end independently of its authorities"), GameState->Destroy());
			CheckRelayContract(0);
			Test.TestFalse(TEXT("Ended GameState is not gameplay-ready"), GameState->IsGameplayInitialized());
			Test.TestEqual(TEXT("Ended GameState does not retain a copied turn"), GameState->GetCurrentTurn(), 0);
			CheckUnavailableTerritories(GameState);
			Test.TestFalse(TEXT("Territory selection rejects an ended public state"), SelectTerritory(TEXT("Rome")));
			Test.TestFalse(TEXT("Ended-state guard collapses the strategic panel"), Root->GetWidgetFromName(TEXT("StrategicPanel"))->IsVisible());
			const int32 BeforeEndedEvent = TerritoryProbe->PublicOwners.Num();
			ChangeOwner(TEXT("Rome"), EFactionID::Carthage);
			ChangeOwner(TEXT("Rome"), EFactionID::Rome);
			Test.TestEqual(TEXT("Ended territorial relay forwards no subsequent ownership events"),
				TerritoryProbe->PublicOwners.Num(), BeforeEndedEvent);
			const FFactionResourceAmounts EmptyWallet;
			const FFactionResourceAmounts EndedWallet = GameState->GetFactionResources(EFactionID::Rome);
			Test.TestTrue(TEXT("Ended GameState does not retain a copied wallet"),
				FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&EndedWallet, &EmptyWallet, 0));
			const FString EndedText = ResourceText->GetText().ToString();
			AddGold(Test, FactionManager, 5);
			Test.TestEqual(TEXT("Detached state relay forwards no later authority event"), ResourceText->GetText().ToString(), EndedText);
			return true;
		}

	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		int32 Stage = 0;
		TWeakObjectPtr<UWorld> PlayWorld;
		APlayerController* Controller = nullptr;
		ARomanEmpireGameState* GameState = nullptr;
		UObject* FactionManager = nullptr;
		UObject* CampaignManager = nullptr;
		UObject* WorldMapManager = nullptr;
		UTacticalBattleSubsystem* TacticalBattle = nullptr;
		AActor* Producer = nullptr;
		UUserWidget* Root = nullptr;
		UTextBlock* ResourceText = nullptr;
		UTextBlock* PhaseText = nullptr;
		UTextBlock* ProductionText = nullptr;
		UTextBlock* BattleText = nullptr;
		UTextBlock* OutcomeText = nullptr;
		UButton* TrainButton = nullptr;
		FName TrainingUnitID;
		FFactionResourceAmounts TrainingCost;
		float TrainingDuration = 0.0f;

		void CheckTrainingCharge(const TCHAR* Context, const FFactionResourceAmounts& Before,
			const FFactionResourceAmounts& ExpectedCost)
		{
			const FFactionResourceAmounts After = ReadResources(Test, FactionManager);
			const TPair<const TCHAR*, int32 FFactionResourceAmounts::*> Fields[] = {
				{ TEXT("Gold"), &FFactionResourceAmounts::Gold }, { TEXT("Food"), &FFactionResourceAmounts::Food },
				{ TEXT("Iron"), &FFactionResourceAmounts::Iron }, { TEXT("Wood"), &FFactionResourceAmounts::Wood },
				{ TEXT("Stone"), &FFactionResourceAmounts::Stone }, { TEXT("Population"), &FFactionResourceAmounts::Population }
			};
			for (const auto& Field : Fields)
			{
				Test.TestEqual(*FString::Printf(TEXT("%s charges exactly the defined %s amount"), Context, Field.Key),
					Before.*(Field.Value) - After.*(Field.Value), ExpectedCost.*(Field.Value));
			}
			Test.TestEqual(TEXT("Training transaction leaves UMG on the canonical wallet"),
				ResourceText->GetText().ToString(), ResourceLabel(After));
		}

		bool ExerciseTrainingTransactions()
		{
			const FFactionResourceAmounts NoCharge;
			const FFactionResourceAmounts BeforeCompletion = ReadResources(Test, FactionManager);
			Producer->Tick(TrainingDuration + 0.1f);
			Test.TestTrue(TEXT("Completion promotes the prepaid queued order"), ReadBool(Test, Producer, TEXT("IsTraining")));
			Test.TestEqual(TEXT("Promotion consumes one pending order"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 0);
			CheckTrainingCharge(TEXT("Completion and queue promotion"), BeforeCompletion, NoCharge);
			if (TacticalBattle)
			{
				const FTacticalBattleResult Result = TacticalBattle->GetResult();
				Test.TestEqual(TEXT("Trained Legionary enters the one tactical result owner"), Result.Squads.Num(), 3);
				if (Result.Squads.Num() == 3)
				{
					Test.TestEqual(TEXT("Trained unit is a Roman singleton reinforcement"), Result.Squads[2].Faction, EFactionID::Rome);
					Test.TestEqual(TEXT("Trained squad starts with one survivor"), Result.Squads[2].Survivors, 1);
				}
				const FString BattleLabel = BattleText ? BattleText->GetText().ToString() : TEXT("<missing>");
				int32 RomanSurvivors = 0;
				int32 RomanRoster = 0;
				for (const FTacticalBattleSquadResult& Squad : Result.Squads)
				{
					if (Squad.Faction == EFactionID::Rome)
					{
						RomanSurvivors += Squad.Survivors;
						RomanRoster += Squad.InitialUnits;
					}
				}
				Test.TestTrue(*FString::Printf(TEXT("Battle UMG receives training without a UI tick: %s"), *BattleLabel),
					BattleLabel.Contains(FString::Printf(TEXT("Allies %d/%d"), RomanSurvivors, RomanRoster)) && RomanRoster == 16);
			}
			UClass* UnitClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.UnitBase")).TryLoadClass<AActor>();
			if (!Test.TestNotNull(TEXT("The live unit class is available for casualty verification"), UnitClass)) return false;
			AActor* EnemyCasualty = nullptr;
			for (TActorIterator<AActor> It(Producer->GetWorld(), UnitClass); It; ++It)
			{
				FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1);
				Owner.Execute();
				if (Owner.ReadEnum<EFactionID>() == EFactionID::Carthage)
				{
					EnemyCasualty = *It;
					break;
				}
			}
			if (!Test.TestNotNull(TEXT("A known defender is available for the battle death event"), EnemyCasualty)) return false;
			FCall LethalDamage(Test, EnemyCasualty, TEXT("TakeCombatDamage"), 3);
			LethalDamage.SetFloat(TEXT("Damage"), 100000.0f);
			LethalDamage.SetObject(TEXT("DamageSource"), nullptr);
			LethalDamage.Execute();
			if (TacticalBattle)
			{
				const FTacticalBattleResult AfterDeath = TacticalBattle->GetResult();
				Test.TestEqual(TEXT("Live death enters the tactical casualty ledger"), AfterDeath.Squads[1].Casualties, 1);
				Test.TestEqual(TEXT("Live death reduces defender survivors once"), AfterDeath.Squads[1].Survivors, 9);
				Test.TestEqual(TEXT("A single defender loss does not end the objective battle"),
					AfterDeath.Outcome, ETacticalBattleOutcome::InProgress);
				Test.TestTrue(TEXT("Battle UMG receives one defender casualty without a UI tick"),
					BattleText && BattleText->GetText().ToString().Contains(TEXT("Foes 9/10")));
			}

			const int32 GoldBeforeDrain = ReadResources(Test, FactionManager).Gold;
			AddGold(Test, FactionManager, -GoldBeforeDrain);
			const FFactionResourceAmounts BeforeRejectedOrder = ReadResources(Test, FactionManager);
			Test.AddExpectedMessage(TEXT("Cannot train unit: resource transaction failed"),
				ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			FCall Rejected(Test, Producer, TEXT("TrainUnit"), 2);
			Rejected.SetName(TEXT("UnitID"), TrainingUnitID);
			Rejected.Execute();
			Test.TestFalse(TEXT("Unfunded training is rejected by the real producer"), Rejected.ReadBool());
			CheckTrainingCharge(TEXT("Insufficient-funds rejection"), BeforeRejectedOrder, NoCharge);
			Test.TestEqual(TEXT("Unfunded order never enters the queue"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 0);
			AddGold(Test, FactionManager, GoldBeforeDrain);

			const FIntProperty* QueueCapacity = FindFProperty<FIntProperty>(Producer->GetClass(), TEXT("MaxQueueSize"));
			if (!Test.TestNotNull(TEXT("Producer exposes its authored queue capacity"), QueueCapacity)) return false;
			const int32 Capacity = QueueCapacity->GetPropertyValue_InContainer(Producer);
			if (!Test.TestTrue(TEXT("Queue fixture has a bounded positive capacity"), Capacity > 0 && Capacity <= 16)) return false;
			FFactionResourceDelta Funding;
			Funding.Gold = TrainingCost.Gold * (Capacity + 1);
			Funding.Food = TrainingCost.Food * (Capacity + 1);
			Funding.Iron = TrainingCost.Iron * (Capacity + 1);
			Funding.Wood = TrainingCost.Wood * (Capacity + 1);
			Funding.Stone = TrainingCost.Stone * (Capacity + 1);
			Funding.Population = TrainingCost.Population * (Capacity + 1);
			FCall Fund(Test, FactionManager, TEXT("ApplyFactionResourceDelta"), 3);
			Fund.SetEnum(TEXT("FactionID"), EFactionID::Rome);
			Fund.SetStruct(TEXT("Delta"), Funding);
			Fund.Execute();
			if (!Test.TestTrue(TEXT("Canonical wallet funds the bounded queue fixture"),
				Fund.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful())) return false;
			for (int32 Index = 0; Index < Capacity; ++Index)
			{
				const FFactionResourceAmounts BeforeOrder = ReadResources(Test, FactionManager);
				FCall Accepted(Test, Producer, TEXT("TrainUnit"), 2);
				Accepted.SetName(TEXT("UnitID"), TrainingUnitID);
				Accepted.Execute();
				Test.TestTrue(TEXT("Funded order is accepted before queue capacity"), Accepted.ReadBool());
				CheckTrainingCharge(TEXT("Funded queue order"), BeforeOrder, TrainingCost);
				Test.TestEqual(TEXT("Each accepted order adds one queue entry"),
					ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), Index + 1);
			}
			const FFactionResourceAmounts BeforeFullQueue = ReadResources(Test, FactionManager);
			Test.AddExpectedMessage(TEXT("Training queue is full"),
				ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
			FCall FullQueue(Test, Producer, TEXT("TrainUnit"), 2);
			FullQueue.SetName(TEXT("UnitID"), TrainingUnitID);
			FullQueue.Execute();
			Test.TestFalse(TEXT("Funded order is rejected at queue capacity"), FullQueue.ReadBool());
			CheckTrainingCharge(TEXT("Full-queue rejection"), BeforeFullQueue, NoCharge);
			Test.TestEqual(TEXT("Rejected order does not overflow the queue"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), Capacity);
			CheckGameStateProjection();
			return true;
		}

		void ExerciseCancellationRefunds()
		{
			UButton* Cancel = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("CancelTrainingButton")));
			Test.TestTrue(TEXT("Owned active producer exposes cancellation control"), Cancel->IsVisible() && Cancel->GetIsEnabled());
			Test.TestFalse(TEXT("Cancel button preserves gameplay keyboard focus"), Cancel->GetIsFocusable());
			Test.TestTrue(TEXT("Controller reads active eligible cancellation"), ReadBool(Test, Controller, TEXT("CanCancelSelectedTraining")));
			if (FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureTrainingUI"))) CaptureStrategicUI(TEXT("ActiveTraining"));
			const FFactionResourceAmounts BeforeModeGuard = ReadResources(Test, FactionManager);
			SetMode(EGameplayMode::Strategic);
			Test.TestFalse(TEXT("Strategic mode hides cancel control"), Cancel->IsVisible());
			Test.TestFalse(TEXT("Strategic controller cancellation rejects"), ReadBool(Test, Controller, TEXT("CancelSelectedTraining")));
			FCall(Test, Root, TEXT("HandleCancelTrainingClicked"), 0).Execute();
			CheckTrainingCharge(TEXT("Strategic cancellation guard"), BeforeModeGuard, FFactionResourceAmounts());
			SetMode(EGameplayMode::Tactical);
			const int32 BeforeSlateQueue = ReadInt(Test, Producer, TEXT("GetPendingOrderCount"));
			const FFactionResourceAmounts BeforeSlate = ReadResources(Test, FactionManager);
			ClickThroughSlate(Test, Root, Cancel);
			FFactionResourceAmounts RefundExpectation = TrainingCost;
			RefundExpectation.Gold *= -1; RefundExpectation.Food *= -1; RefundExpectation.Iron *= -1;
			RefundExpectation.Wood *= -1; RefundExpectation.Stone *= -1; RefundExpectation.Population *= -1;
			CheckTrainingCharge(TEXT("Slate CANCEL + REFUND"), BeforeSlate, RefundExpectation);
			Test.TestEqual(TEXT("One Slate cancel promotes exactly one prepaid order"),
				ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), BeforeSlateQueue - 1);
			Test.TestTrue(TEXT("Queued promotion keeps cancel enabled without UI tick"), Cancel->IsVisible() && Cancel->GetIsEnabled());
			Test.TestTrue(TEXT("Cancellation event refreshes pending queue without a UI tick"),
				ProductionText->GetText().ToString().Contains(FString::Printf(TEXT("%d pending"), BeforeSlateQueue - 1)));
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(Producer);
			URomanUnitDefinition* Asset = const_cast<URomanUnitDefinition*>(Registry ? Registry->FindUnit(TrainingUnitID) : nullptr);
			if (!Test.TestNotNull(TEXT("Refund test has original unit asset"), Asset)) return;
			TGuardValue<FFactionResourceAmounts> PriceGuard(Asset->Definition.TrainingCost, FFactionResourceAmounts());
			const int32 InitialPending = ReadInt(Test, Producer, TEXT("GetPendingOrderCount"));
			FCall Owner(Test, Producer, TEXT("SetOwnerFaction"), 1);
			Owner.SetEnum(TEXT("NewOwner"), EFactionID::Carthage);
			Owner.Execute();
			const FFactionResourceAmounts CarthageBefore = ReadResources(Test, FactionManager, EFactionID::Carthage);
			Test.TestFalse(TEXT("Ownership loss hides cancellation control"), Cancel->IsVisible());
			Test.TestFalse(TEXT("Controller cannot cancel after ownership loss"), ReadBool(Test, Controller, TEXT("CancelSelectedTraining")));
			FCall Receipt(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1);
			Receipt.Execute();
			const FTrainingReservation OriginalReceipt = Receipt.ReadStruct<FTrainingReservation>();
			Test.TestEqual(TEXT("Receipt retains original payer after ownership change"), OriginalReceipt.PayingFaction, EFactionID::Rome);
			Test.TestTrue(TEXT("Receipt retains all six original costs after catalog edit"),
				FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&OriginalReceipt.PaidCost, &TrainingCost, 0));
			// Force a canonical refund overflow, then restore the wallet and retry.
			const int32 PreviousGold = ReadResources(Test, FactionManager).Gold;
			AddGold(Test, FactionManager, MAX_int32 - PreviousGold);
			const int32 BeforePending = ReadInt(Test, Producer, TEXT("GetPendingOrderCount"));
			Test.TestFalse(TEXT("Refund overflow rejects cancellation"), ReadBool(Test, Producer, TEXT("TryCancelTraining")));
			Test.TestTrue(TEXT("Refund failure preserves active order"), ReadBool(Test, Producer, TEXT("IsTraining")));
			Test.TestEqual(TEXT("Refund failure preserves queue"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), BeforePending);
			Test.TestEqual(TEXT("Refund failure preserves max wallet"), ReadResources(Test, FactionManager).Gold, MAX_int32);
			FCall Retained(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1);
			Retained.Execute();
			const FTrainingReservation RetainedReceipt = Retained.ReadStruct<FTrainingReservation>();
			Test.TestTrue(TEXT("Refund failure preserves exact receipt for retry"), FTrainingReservation::StaticStruct()->CompareScriptStruct(&OriginalReceipt, &RetainedReceipt, 0));
			AddGold(Test, FactionManager, PreviousGold - MAX_int32);
			TStrongObjectPtr<UTrainingRefundProbe> Probe(NewObject<UTrainingRefundProbe>());
			Probe->Test = &Test; Probe->Producer = Producer; Probe->UnitID = TrainingUnitID;
			FMulticastDelegateProperty* Event = FindFProperty<FMulticastDelegateProperty>(FactionManager->GetClass(), TEXT("OnFactionResourcesChanged"));
			FScriptDelegate Nested;
			Nested.BindUFunction(Probe.Get(), TEXT("OnResourcesChanged"));
			Event->AddDelegate(Nested, FactionManager);
			for (int32 Index = 0; Index <= InitialPending; ++Index)
			{
				FFactionResourceAmounts Expected = ReadResources(Test, FactionManager);
				Expected.Gold += TrainingCost.Gold; Expected.Food += TrainingCost.Food;
				Expected.Iron += TrainingCost.Iron; Expected.Wood += TrainingCost.Wood;
				Expected.Stone += TrainingCost.Stone; Expected.Population += TrainingCost.Population;
				Test.TestTrue(TEXT("Cancel refunds active order and promotes prepaid next"), ReadBool(Test, Producer, TEXT("TryCancelTraining")));
				const FFactionResourceAmounts Actual = ReadResources(Test, FactionManager);
				Test.TestTrue(TEXT("Cancel refunds all six original paid fields exactly once"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Actual, &Expected, 0));
				Test.TestEqual(TEXT("Cancellation promotes queue without charging again"),
					ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), FMath::Max(0, InitialPending - Index - 1));
			}
			Event->RemoveDelegate(Nested, FactionManager);
			Test.TestEqual(TEXT("Exactly one wallet notification per nonzero refund"), Probe->Notifications, InitialPending + 1);
			Test.TestFalse(TEXT("All cancelled orders leave producer idle"), ReadBool(Test, Producer, TEXT("IsTraining")));
			const FFactionResourceAmounts BeforeIdle = ReadResources(Test, FactionManager);
			Test.TestFalse(TEXT("Idle cancellation cannot double-refund"), ReadBool(Test, Producer, TEXT("TryCancelTraining")));
			CheckTrainingCharge(TEXT("No active cancellation"), BeforeIdle, FFactionResourceAmounts());
			const FFactionResourceAmounts CarthageAfter = ReadResources(Test, FactionManager, EFactionID::Carthage);
			Test.TestTrue(TEXT("Refund returns to original payer, never captured building's new owner"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&CarthageBefore, &CarthageAfter, 0));
			Owner.SetEnum(TEXT("NewOwner"), EFactionID::Rome);
			Owner.Execute();
			FCall Free(Test, Producer, TEXT("TrainUnit"), 2);
			Free.SetName(TEXT("UnitID"), TrainingUnitID);
			Free.Execute();
			Test.TestTrue(TEXT("Zero-cost order accepts normally"), Free.ReadBool());
			Test.TestTrue(TEXT("Zero-cost active cancellation succeeds"), ReadBool(Test, Producer, TEXT("TryCancelTraining")));
			CheckTrainingCharge(TEXT("Zero-cost cancellation"), BeforeIdle, FFactionResourceAmounts());
			FCall Select(Test, Controller, TEXT("SelectBuilding"), 1);
			Select.SetObject(TEXT("Building"), Producer);
			Select.Execute();
			Test.TestFalse(TEXT("Idle producer hides cancellation control"), Cancel->IsVisible());
			Test.TestFalse(TEXT("Idle controller cancellation rejects"), ReadBool(Test, Controller, TEXT("CancelSelectedTraining")));
			if (FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureTrainingUI"))) CaptureStrategicUI(TEXT("IdleTraining"));
		}

		void ExerciseFailedTrainingRefunds()
		{
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(Producer);
			URomanUnitDefinition* Asset = const_cast<URomanUnitDefinition*>(Registry ? Registry->FindUnit(TrainingUnitID) : nullptr);
			if (!Test.TestNotNull(TEXT("Failure refund fixture has the canonical asset"), Asset)) return;
			FFactionResourceAmounts Paid;
			Paid.Gold = 7; Paid.Food = 5; Paid.Iron = 3; Paid.Wood = 2; Paid.Stone = 1; Paid.Population = 1;
			TGuardValue<FFactionResourceAmounts> PriceGuard(Asset->Definition.TrainingCost, Paid);
			const FFactionResourceAmounts BeforeOrders = ReadResources(Test, FactionManager);
			for (int32 Index = 0; Index < 2; ++Index)
			{
				FCall Train(Test, Producer, TEXT("TrainUnit"), 2);
				Train.SetName(TEXT("UnitID"), TrainingUnitID);
				Train.Execute();
				if (!Test.TestTrue(TEXT("Failure fixture accepts paid active and queued orders"), Train.ReadBool())) return;
			}
			Asset->Definition.TrainingCost = FFactionResourceAmounts();
			FCall Owner(Test, Producer, TEXT("SetOwnerFaction"), 1);
			Owner.SetEnum(TEXT("NewOwner"), EFactionID::Carthage);
			Owner.Execute();
			const FFactionResourceAmounts CarthageBefore = ReadResources(Test, FactionManager, EFactionID::Carthage);
			TStrongObjectPtr<UTrainingRefundProbe> Probe(NewObject<UTrainingRefundProbe>());
			Probe->Test = &Test; Probe->Producer = Producer; Probe->UnitID = TrainingUnitID;
			FMulticastDelegateProperty* WalletEvent = FindFProperty<FMulticastDelegateProperty>(FactionManager->GetClass(), TEXT("OnFactionResourcesChanged"));
			FMulticastDelegateProperty* ProductionEvent = FindFProperty<FMulticastDelegateProperty>(Producer->GetClass(), TEXT("OnProductionChanged"));
			if (!Test.TestTrue(TEXT("Failure fixture has real wallet/production events"), WalletEvent && ProductionEvent)) return;
			FScriptDelegate WalletDelegate, ProductionDelegate;
			WalletDelegate.BindUFunction(Probe.Get(), TEXT("OnResourcesChanged"));
			ProductionDelegate.BindUFunction(Probe.Get(), TEXT("OnProductionChanged"));
			ProductionEvent->AddDelegate(ProductionDelegate, Producer);
			ON_SCOPE_EXIT
			{
				WalletEvent->RemoveDelegate(WalletDelegate, FactionManager);
				ProductionEvent->RemoveDelegate(ProductionDelegate, Producer);
				Owner.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Owner.Execute();
			};
			URomanUnitRegistrySubsystem* Units = Producer->GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>();
			if (!Test.TestNotNull(TEXT("Failure fixture uses live unit registry"), Units)) return;
			const int32 UnitCount = Units->GetRegisteredUnitCount();
			const int32 GoldAfterSpend = ReadResources(Test, FactionManager).Gold;
			AddGold(Test, FactionManager, MAX_int32 - GoldAfterSpend);
			{
				TGuardValue<TSoftClassPtr<AUnitBase>> ClassGuard(Asset->UnitClass, TSoftClassPtr<AUnitBase>());
				Test.AddExpectedMessage(TEXT("factory class is unavailable"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);
				Producer->Tick(TrainingDuration + 0.1f);
			}
			FCall Original(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1); Original.Execute();
			const FTrainingReservation FailedReceipt = Original.ReadStruct<FTrainingReservation>();
			Test.TestTrue(TEXT("Blocked failed-spawn refund retains original receipt"),
				FailedReceipt.PayingFaction == EFactionID::Rome && FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&FailedReceipt.PaidCost, &Paid, 0));
			Test.TestTrue(TEXT("Failed spawn waits for refund instead of discarding order"), ReadBool(Test, Producer, TEXT("IsAwaitingTrainingRefund")));
			Test.TestEqual(TEXT("Failed spawn publishes one pending-refund invalidation"), Probe->PendingRefundEvents, 1);
			Producer->Tick(TrainingDuration + 0.1f); // Class restored: must NOT spawn on credit retry.
			Test.TestEqual(TEXT("Blocked failed-spawn refund preserves queue"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 1);
			Test.TestEqual(TEXT("No terminal event before refund succeeds"), Probe->FailedOrders, 0);
			Test.TestEqual(TEXT("Blocked credit retries do not spam pending-refund events"), Probe->PendingRefundEvents, 1);
			Test.TestEqual(TEXT("Failure retry never spawns despite restored factory"), Units->GetRegisteredUnitCount(), UnitCount);
			FCall Retained(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1); Retained.Execute();
			const FTrainingReservation RetryReceipt = Retained.ReadStruct<FTrainingReservation>();
			Test.TestTrue(TEXT("Blocked retry retains exact detached receipt"), FTrainingReservation::StaticStruct()->CompareScriptStruct(&FailedReceipt, &RetryReceipt, 0));
			AddGold(Test, FactionManager, GoldAfterSpend - MAX_int32);
			Owner.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Owner.Execute();
			FCall Select(Test, Controller, TEXT("SelectBuilding"), 1);
			Select.SetObject(TEXT("Building"), Producer); Select.Execute();
			Test.TestTrue(TEXT("Selected retained failure projects refund status without UI tick"), ProductionText->GetText().ToString().Contains(TEXT("refund pending")));
			WalletEvent->AddDelegate(WalletDelegate, FactionManager);
			{
				TGuardValue<float> DurationGuard(Asset->Definition.TrainingTime, 0.0f);
				Test.AddExpectedMessage(TEXT("Queued production definition"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
				Producer->Tick(0.1f);
			}
			Test.TestEqual(TEXT("Spawn failure refunds once and publishes one Failed"), Probe->FailedOrders, 1);
			Test.TestEqual(TEXT("Invalid queue promotion consumes one pending slot, retaining its receipt"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 0);
			Test.TestTrue(TEXT("Invalid queued definition also waits for refund"), ReadBool(Test, Producer, TEXT("IsAwaitingTrainingRefund")));
			Producer->Tick(0.1f); // Definition restored: the failed queued order still refunds, not spawns.
			Test.TestEqual(TEXT("Each accepted failed order produces one Failed event"), Probe->FailedOrders, 2);
			Test.TestEqual(TEXT("Failure refunds notify the original wallet exactly once each"), Probe->Notifications, 2);
			Test.TestEqual(TEXT("Failed orders never publish Completed"), Probe->CompletedOrders, 0);
			Test.TestFalse(TEXT("Refunded failed queue leaves producer idle"), ReadBool(Test, Producer, TEXT("IsTraining")));
			Test.TestTrue(TEXT("Failure completion refreshes idle UI without tick"), ProductionText->GetText().ToString().Contains(TEXT("idle")));
			const FFactionResourceAmounts After = ReadResources(Test, FactionManager);
			Test.TestTrue(TEXT("Failed active and queued orders restore all six original amounts"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&BeforeOrders, &After, 0));
			const FFactionResourceAmounts CarthageAfter = ReadResources(Test, FactionManager, EFactionID::Carthage);
			Test.TestTrue(TEXT("Capture never redirects failed-order refunds to the new owner"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&CarthageBefore, &CarthageAfter, 0));
			Producer->Tick(TrainingDuration + 0.1f);
			Test.TestEqual(TEXT("Idle tick cannot repeat the failure refund"), Probe->Notifications, 2);
			Test.TestEqual(TEXT("Failure paths never register new units"), Units->GetRegisteredUnitCount(), UnitCount);
		}

		void CheckBindings(int32 Expected)
		{
			Test.TestEqual(TEXT("Exactly the expected number of root GameState resource subscriptions exist"),
				ListenerCount(Test, GameState, TEXT("OnFactionResourcesChanged"), Root), Expected);
			Test.TestEqual(TEXT("Exactly the expected number of root GameState turn subscriptions exist"),
				ListenerCount(Test, GameState, TEXT("OnTurnProcessed"), Root), Expected);
			Test.TestEqual(TEXT("Root no longer observes the wallet authority directly"),
				ListenerCount(Test, FactionManager, TEXT("OnFactionResourcesChanged"), Root), 0);
			Test.TestEqual(TEXT("Root no longer observes the turn authority directly"),
				ListenerCount(Test, CampaignManager, TEXT("OnTurnProcessed"), Root), 0);
			Test.TestEqual(TEXT("GameState owns one wallet relay independent of UMG attachment"),
				ListenerCount(Test, FactionManager, TEXT("OnFactionResourcesChanged"), GameState), 1);
			Test.TestEqual(TEXT("GameState owns one turn relay independent of UMG attachment"),
				ListenerCount(Test, CampaignManager, TEXT("OnTurnProcessed"), GameState), 1);
			Test.TestEqual(TEXT("GameState owns one territorial relay independent of UMG attachment"),
				ListenerCount(Test, WorldMapManager, TEXT("OnTerritoryOwnerChanged"), GameState), 1);
			Test.TestEqual(TEXT("Root owns the expected number of public territory subscriptions"),
				ListenerCount(Test, GameState, TEXT("OnTerritoryOwnerChanged"), Root), Expected);
			Test.TestEqual(TEXT("Root never observes private territorial authority directly"),
				ListenerCount(Test, WorldMapManager, TEXT("OnTerritoryOwnerChanged"), Root), 0);
			Test.TestEqual(TEXT("Root has exactly the expected public settlement subscriptions"),
				ListenerCount(Test, GameState, TEXT("OnTerritorySettlementChanged"), Root), Expected);
			Test.TestEqual(TEXT("Root never observes settlement authority directly"),
				ListenerCount(Test, WorldMapManager, TEXT("OnTerritorySettlementChanged"), Root), 0);
			Test.TestEqual(TEXT("Exactly the expected number of root battle subscriptions exist"),
				ListenerCount(Test, GameState, TEXT("OnTacticalBattleChanged"), Root), Expected);
			Test.TestEqual(TEXT("GameState owns one battle relay independent of UMG attachment"),
				ListenerCount(Test, TacticalBattle, TEXT("OnTacticalBattleChanged"), GameState), 1);
			Test.TestEqual(TEXT("Root never observes the tactical authority directly"),
				ListenerCount(Test, TacticalBattle, TEXT("OnTacticalBattleChanged"), Root), 0);
			Test.TestEqual(TEXT("Exactly the expected number of root selection subscriptions exist"),
				ListenerCount(Test, Controller, TEXT("OnSelectionChanged"), Root), Expected);
			Test.TestEqual(TEXT("Exactly the expected number of root production subscriptions exist"),
				ListenerCount(Test, Producer, TEXT("OnProductionChanged"), Root), Expected);
		}

		void AdvanceTurn()
		{
			const int32 PreviousTurn = ReadInt(Test, CampaignManager, TEXT("GetCurrentTurn"));
			TMap<EFactionID, FFactionResourceAmounts> ExpectedWallets;
			for (EFactionID Faction : { EFactionID::Rome, EFactionID::Carthage })
			{
				FFactionResourceAmounts Expected = GameState->GetFactionResources(Faction);
				for (const FTerritoryPresentationSnapshot& Row : GameState->GetTerritorySnapshots())
				{
					if (Row.OwnerFaction != Faction) continue;
					const FFactionResourceDelta& Delta = Row.EffectiveResourceProduction;
					Expected.Gold += Delta.Gold; Expected.Food += Delta.Food; Expected.Iron += Delta.Iron;
					Expected.Wood += Delta.Wood; Expected.Stone += Delta.Stone; Expected.Population += Delta.Population;
				}
				ExpectedWallets.Add(Faction, Expected);
			}
			FCall Call(Test, CampaignManager, TEXT("ProcessTurn"), 1);
			Call.Execute();
			Test.TestTrue(TEXT("Canonical campaign pipeline processes the requested turn"), Call.ReadStruct<FCampaignTurnResult>().WasSuccessful());
			const int32 CurrentTurn = ReadInt(Test, CampaignManager, TEXT("GetCurrentTurn"));
			Test.TestEqual(TEXT("Only one canonical turn advances"), CurrentTurn, PreviousTurn + 1);
			for (const auto& Pair : ExpectedWallets)
			{
				const FFactionResourceAmounts Actual = GameState->GetFactionResources(Pair.Key);
				Test.TestTrue(TEXT("Each turn credits all six effective production fields exactly once"),
					FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Actual, &Pair.Value, 0));
			}
			Test.TestEqual(TEXT("UMG settles on the canonical mode and turn after the complete pipeline"),
				PhaseText->GetText().ToString(), FString::Printf(TEXT("Tactical   |   Turn %d"), CurrentTurn));
			CheckGameStateProjection();
		}

		void CheckGameStateProjection()
		{
			CheckTerritoryProjection();
			// Query the exported native contract directly; compare against each owner
			// after mutations, without ticking a UI or refreshing a copied snapshot.
			AGameModeBase* GameMode = PlayWorld->GetAuthGameMode();
			Test.TestTrue(TEXT("Public GameState is initialized during normal PIE gameplay"), GameState->IsGameplayInitialized());
			Test.TestEqual(TEXT("GameState readiness delegates to GameMode"),
				GameState->IsGameplayInitialized(), ReadBool(Test, GameMode, TEXT("IsGameplayInitialized")));
			FCall Mode(Test, GameMode, TEXT("GetCurrentGameplayMode"), 1);
			Mode.Execute();
			Test.TestEqual(TEXT("GameState exposes the canonical gameplay mode"),
				GameState->GetCurrentGameplayMode(), Mode.ReadEnum<EGameplayMode>());
			Test.TestEqual(TEXT("GameState exposes the sole campaign turn"),
				GameState->GetCurrentTurn(), ReadInt(Test, CampaignManager, TEXT("GetCurrentTurn")));
			FCall PlayerFaction(Test, FactionManager, TEXT("GetPlayerFaction"), 1);
			PlayerFaction.Execute();
			Test.TestEqual(TEXT("GameState exposes the canonical player faction"),
				GameState->GetPlayerFaction(), PlayerFaction.ReadEnum<EFactionID>());
			for (EFactionID Faction : { EFactionID::None, EFactionID::Rome, EFactionID::Carthage,
				EFactionID::Gaul, EFactionID::Greece, EFactionID::Egypt, EFactionID::Britannia })
			{
				const FFactionResourceAmounts Expected = ReadResources(Test, FactionManager, Faction);
				const FFactionResourceAmounts Actual = GameState->GetFactionResources(Faction);
				Test.TestTrue(TEXT("GameState projects every faction wallet field from its authority"),
					FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Actual, &Expected, 0));
				FCall Territories(Test, WorldMapManager, TEXT("GetFactionTerritoryCount"), 2);
				Territories.SetEnum(TEXT("FactionID"), Faction);
				Territories.Execute();
				Test.TestEqual(TEXT("GameState projects the WorldMapManager territory count"),
					GameState->GetFactionTerritoryCount(Faction), Territories.ReadInt());
			}
			FCall Objectives(Test, CampaignManager, TEXT("GetObjectiveState"), 1);
			Objectives.Execute();
			const FCampaignObjectiveState ExpectedObjectives = Objectives.ReadStruct<FCampaignObjectiveState>();
			const FCampaignObjectiveState ActualObjectives = GameState->GetObjectiveState();
			Test.TestTrue(TEXT("GameState projects all objective progress, thresholds, and terminal outcome"),
				FCampaignObjectiveState::StaticStruct()->CompareScriptStruct(&ActualObjectives, &ExpectedObjectives, 0));
			Test.TestEqual(TEXT("GameState exposes the canonical campaign-active status"),
				GameState->IsCampaignActive(), ReadBool(Test, CampaignManager, TEXT("IsCampaignActive")));
		}

		void CheckRelayContract(int32 ExpectedListeners = 1)
		{
			const TPair<UObject*, FName> Relays[] = {
				{ PlayWorld->GetAuthGameMode(), TEXT("OnGameplayInitialized") },
				{ PlayWorld->GetAuthGameMode(), TEXT("OnGameplayModeChanged") },
				{ FactionManager, TEXT("OnFactionResourcesChanged") },
				{ FactionManager, TEXT("OnPlayerFactionChanged") },
				{ WorldMapManager, TEXT("OnTerritoryOwnerChanged") },
				{ WorldMapManager, TEXT("OnTerritorySettlementChanged") },
				{ CampaignManager, TEXT("OnTurnStepStarted") },
				{ CampaignManager, TEXT("OnTurnProcessed") },
				{ CampaignManager, TEXT("OnCampaignStarted") },
				{ CampaignManager, TEXT("OnCampaignObjectivesChanged") },
				{ CampaignManager, TEXT("OnVictory") },
				{ CampaignManager, TEXT("OnDefeat") }
			};
			for (const TPair<UObject*, FName>& Relay : Relays)
			{
				Test.TestEqual(*FString::Printf(TEXT("GameState owns the expected number of %s relay subscriptions"), *Relay.Value.ToString()),
					ListenerCount(Test, Relay.Key, Relay.Value, GameState), ExpectedListeners);
				FMulticastDelegateProperty* Source = FindFProperty<FMulticastDelegateProperty>(Relay.Key->GetClass(), Relay.Value);
				FMulticastDelegateProperty* Exposed = FindFProperty<FMulticastDelegateProperty>(GameState->GetClass(), Relay.Value);
				Test.TestTrue(*FString::Printf(TEXT("GameState relay %s preserves its authority's event signature"), *Relay.Value.ToString()),
					Source && Exposed && Source->SignatureFunction && Exposed->SignatureFunction &&
					Source->SignatureFunction->IsSignatureCompatibleWith(Exposed->SignatureFunction));
			}
		}

		TStrongObjectPtr<UTerritoryProjectionProbe> TerritoryProbe;

		FTerritoryOwnershipChangeResult ChangeOwner(FName ID, EFactionID Owner)
		{
			FCall Call(Test, WorldMapManager, TEXT("ChangeTerritoryOwner"), 3);
			Call.SetName(TEXT("TerritoryID"), ID);
			Call.SetEnum(TEXT("NewOwner"), Owner);
			Call.Execute();
			return Call.ReadStruct<FTerritoryOwnershipChangeResult>();
		}

		FSettlementFoundingResult FoundSettlement(FName ID, const TCHAR* Name)
		{
			FCall Call(Test, WorldMapManager, TEXT("FoundTerritorySettlement"), 3);
			Call.SetName(TEXT("TerritoryID"), ID);
			Call.SetText(TEXT("Name"), FText::FromString(Name));
			Call.Execute();
			return Call.ReadStruct<FSettlementFoundingResult>();
		}

		void CheckUnavailableTerritories(ARomanEmpireGameState* State)
		{
			Test.TestTrue(TEXT("Unready/ended state exposes no territory catalog"), State->GetTerritorySnapshots().IsEmpty());
			FTerritoryPresentationSnapshot Snapshot;
			Snapshot.TerritoryID = TEXT("Stale");
			Snapshot.OwnerFaction = EFactionID::Rome;
			Snapshot.AdjacentTerritoryIDs.Add(TEXT("StaleNeighbor"));
			Test.TestFalse(TEXT("Unready/ended state cannot read a known territory"), State->TryGetTerritorySnapshot(TEXT("Rome"), Snapshot));
			const FTerritoryPresentationSnapshot Empty;
			Test.TestTrue(TEXT("Unavailable lookup resets every output field"),
				FTerritoryPresentationSnapshot::StaticStruct()->CompareScriptStruct(&Snapshot, &Empty, 0));
		}

		void CheckTerritoryProjection()
		{
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(GameState);
			if (!Test.TestTrue(TEXT("Territory projection has a ready catalog"), Registry && Registry->IsReady())) return;
			const TArray<FTerritoryPresentationSnapshot> Rows = GameState->GetTerritorySnapshots();
			FCall IDs(Test, WorldMapManager, TEXT("GetTerritoryIDs"), 1);
			IDs.Execute();
			const TArray<FName> CanonicalIDs = IDs.ReadNameArray();
			Test.TestEqual(TEXT("Public projection covers all nine registered territories"), Rows.Num(), 9);
			TArray<FName> ActualIDs;
			for (const FTerritoryPresentationSnapshot& Row : Rows)
			{
				ActualIDs.Add(Row.TerritoryID);
				const URomanTerritoryDefinition* Asset = Registry->FindTerritory(Row.TerritoryID);
				if (!Test.TestNotNull(TEXT("Every public territory has immutable metadata"), Asset)) continue;
				FCall Definition(Test, WorldMapManager, TEXT("GetTerritoryDefinition"), 2);
				Definition.SetName(TEXT("TerritoryID"), Row.TerritoryID);
				Definition.Execute();
				const FTerritoryDefinition Data = Definition.ReadStruct<FTerritoryDefinition>();
				Test.TestEqual(TEXT("Territory label is canonical"), Row.DisplayName.ToString(), Data.DisplayName.ToString());
				Test.TestEqual(TEXT("Territory description is canonical"), Row.Description.ToString(), Data.Description.ToString());
				Test.TestEqual(TEXT("Terrain is canonical"), Row.TerrainType, Data.TerrainType);
				Test.TestEqual(TEXT("Map location is authored, not an actor transform"), Row.MapLocation, Asset->MapLocation);
				Test.TestEqual(TEXT("Bonus resource is canonical"), Row.BonusResource, Data.BonusResource);
				Test.TestTrue(TEXT("All six base production values are canonical"),
					FFactionResourceDelta::StaticStruct()->CompareScriptStruct(&Row.BaseResourceProduction, &Data.ResourceProduction, 0));
				FFactionResourceDelta Effective = Row.BaseResourceProduction;
				Effective.Gold += Row.Settlement.ProductionBonus.Gold;
				Effective.Food += Row.Settlement.ProductionBonus.Food;
				Effective.Iron += Row.Settlement.ProductionBonus.Iron;
				Effective.Wood += Row.Settlement.ProductionBonus.Wood;
				Effective.Stone += Row.Settlement.ProductionBonus.Stone;
				Effective.Population += Row.Settlement.ProductionBonus.Population;
				Test.TestTrue(TEXT("All six effective production values are base plus live bonus"),
					FFactionResourceDelta::StaticStruct()->CompareScriptStruct(&Row.EffectiveResourceProduction, &Effective, 0));
				FCall Owner(Test, WorldMapManager, TEXT("GetTerritoryOwner"), 2);
				Owner.SetName(TEXT("TerritoryID"), Row.TerritoryID);
				Owner.Execute();
				Test.TestEqual(TEXT("Snapshot uses current ownership, not starting owner"), Row.OwnerFaction, Owner.ReadEnum<EFactionID>());
				FCall Neighbors(Test, WorldMapManager, TEXT("GetAdjacentTerritoryIDs"), 2);
				Neighbors.SetName(TEXT("TerritoryID"), Row.TerritoryID);
				Neighbors.Execute();
				Test.TestTrue(TEXT("Public neighbors use canonical sorted ID graph"), Row.AdjacentTerritoryIDs == Neighbors.ReadNameArray());
				const URomanFactionDefinition* Faction = Registry->FindFaction(Row.OwnerFaction);
				Test.TestEqual(TEXT("Owner label comes from catalog, even for dormant Gaul"), Row.OwnerDisplayName.ToString(),
					Faction ? Faction->Definition.DisplayName.ToString() : FString());
				Test.TestEqual(TEXT("Owner color comes from catalog"), Row.OwnerColor,
					Faction ? Faction->Definition.FactionColor : FLinearColor::White);
				Test.TestEqual(TEXT("Slice participation is distinct from ownership"), Row.bOwnerActive,
					Row.OwnerFaction == EFactionID::Rome || Row.OwnerFaction == EFactionID::Carthage);
				FTerritoryPresentationSnapshot Single;
				Test.TestTrue(TEXT("Single-ID read succeeds"), GameState->TryGetTerritorySnapshot(Row.TerritoryID, Single));
				Test.TestTrue(TEXT("Single-ID read matches detached list value"),
					FTerritoryPresentationSnapshot::StaticStruct()->CompareScriptStruct(&Row, &Single, 0));
			}
			Test.TestTrue(TEXT("Catalog enumerates canonical IDs in lexical order"), ActualIDs == CanonicalIDs);
			TArray<FName> Sorted = ActualIDs;
			Sorted.Sort(FNameLexicalLess());
			Test.TestTrue(TEXT("Public ordering is deterministic"), ActualIDs == Sorted);
			for (FName Unknown : { FName(NAME_None), FName(TEXT("Missing")) })
			{
				FTerritoryPresentationSnapshot UnknownRow = Rows.IsEmpty() ? FTerritoryPresentationSnapshot() : Rows[0];
				const FTerritoryPresentationSnapshot Empty;
				Test.TestFalse(TEXT("Unknown ID is not confused with unowned territory"), GameState->TryGetTerritorySnapshot(Unknown, UnknownRow));
				Test.TestTrue(TEXT("Unknown ID clears previous output"),
					FTerritoryPresentationSnapshot::StaticStruct()->CompareScriptStruct(&UnknownRow, &Empty, 0));
			}
		}

		void ExerciseTerritorialEvents()
		{
			// A valid, populated authority from another world must not be projected,
			// even when it shares the same immutable GameInstance content catalog.
			UWorld* ForeignWorld = UWorld::CreateWorld(EWorldType::Game, false);
			if (Test.TestNotNull(TEXT("Foreign territory world exists"), ForeignWorld))
			{
				GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(ForeignWorld);
				ForeignWorld->SetGameInstance(PlayWorld->GetGameInstance());
				ForeignWorld->InitializeActorsForPlay(FURL());
				FActorSpawnParameters Parameters;
				Parameters.ObjectFlags |= RF_Transient;
				Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* ForeignMap = ForeignWorld->SpawnActor<AActor>(WorldMapManager->GetClass(),
					FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
				FWeakObjectProperty* MapProperty = FindFProperty<FWeakObjectProperty>(GameState->GetClass(), TEXT("WorldMap"));
				if (Test.TestTrue(TEXT("Foreign authority and reflected weak link exist"), ForeignMap && MapProperty))
				{
					ForeignMap->DispatchBeginPlay();
					FCall IDs(Test, ForeignMap, TEXT("GetTerritoryIDs"), 1);
					IDs.Execute();
					Test.TestEqual(TEXT("Foreign authority is populated, not an empty placeholder"), IDs.ReadNameArray().Num(), 9);
					MapProperty->SetObjectPropertyValue_InContainer(GameState, ForeignMap);
					CheckUnavailableTerritories(GameState);
					MapProperty->SetObjectPropertyValue_InContainer(GameState, WorldMapManager);
				}
				ForeignWorld->DestroyWorld(false);
				GEngine->DestroyWorldContext(ForeignWorld);
				CollectGarbage(RF_NoFlags);
			}
			ARomanEmpireGameState* Unready = PlayWorld->SpawnActorDeferred<ARomanEmpireGameState>(
				ARomanEmpireGameState::StaticClass(), FTransform::Identity, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Test.TestNotNull(TEXT("Deferred state exists before BeginPlay"), Unready))
			{
				CheckUnavailableTerritories(Unready);
				Unready->Destroy();
			}
			TerritoryProbe.Reset(NewObject<UTerritoryProjectionProbe>());
			TerritoryProbe->Test = &Test;
			TerritoryProbe->State = GameState;
			TerritoryProbe->WorldMap = WorldMapManager;
			GameState->OnTerritoryOwnerChanged.AddDynamic(TerritoryProbe.Get(), &UTerritoryProjectionProbe::OnPublicOwnerChanged);
			FMulticastDelegateProperty* Event = FindFProperty<FMulticastDelegateProperty>(WorldMapManager->GetClass(), TEXT("OnTerritoryOwnerChanged"));
			if (!Test.TestNotNull(TEXT("Ownership source event is reflected"), Event)) return;
			FScriptDelegate Relay, Nested;
			Relay.BindUFunction(GameState, TEXT("HandleTerritoryOwnerChanged"));
			Nested.BindUFunction(TerritoryProbe.Get(), TEXT("OnSourceOwnerChanged"));
			// Force an earlier listener to overtake the outer transaction payload.
			Event->RemoveDelegate(Relay, WorldMapManager);
			Event->AddDelegate(Nested, WorldMapManager);
			Event->AddDelegate(Relay, WorldMapManager);
			Test.TestTrue(TEXT("No-op ownership transaction succeeds"), ChangeOwner(TEXT("Rome"), EFactionID::Rome).WasSuccessful());
			Test.TestFalse(TEXT("Unknown ownership transaction rejects"), ChangeOwner(TEXT("Missing"), EFactionID::Rome).WasSuccessful());
			Test.TestFalse(TEXT("Invalid owner rejects"), ChangeOwner(TEXT("Rome"), static_cast<EFactionID>(255)).WasSuccessful());
			Test.TestEqual(TEXT("No-op/rejected transactions publish no public events"), TerritoryProbe->PublicOwners.Num(), 0);
			FTerritoryPresentationSnapshot Before;
			GameState->TryGetTerritorySnapshot(TEXT("Rome"), Before);
			Test.TestTrue(TEXT("Valid owner change commits"), ChangeOwner(TEXT("Rome"), EFactionID::Carthage).bOwnershipChanged);
			Test.TestEqual(TEXT("One change gives exactly one public notification"), TerritoryProbe->PublicOwners.Num(), 1);
			Test.TestEqual(TEXT("Returned snapshot is not mutated by gameplay"), Before.OwnerFaction, EFactionID::Rome);
			CheckTerritoryProjection();
			Before.DisplayName = FText::FromString(TEXT("Caller edited"));
			Before.AdjacentTerritoryIDs.Reset();
			FTerritoryPresentationSnapshot Fresh;
			GameState->TryGetTerritorySnapshot(TEXT("Rome"), Fresh);
			Test.TestTrue(TEXT("Caller edits cannot modify canonical metadata/graph"),
				Fresh.DisplayName.ToString() != Before.DisplayName.ToString() && !Fresh.AdjacentTerritoryIDs.IsEmpty());
			ChangeOwner(TEXT("Rome"), EFactionID::Rome);
			TerritoryProbe->PublicOwners.Reset();
			TerritoryProbe->SourceNotifications = 0;
			TerritoryProbe->bNestOnce = true;
			ChangeOwner(TEXT("Rome"), EFactionID::Carthage);
			Test.TestEqual(TEXT("Nested ownership commits exactly twice"), TerritoryProbe->SourceNotifications, 2);
			Test.TestTrue(TEXT("Both public notifications publish current owner, not stale outer payload"),
				TerritoryProbe->PublicOwners == TArray<EFactionID>({ EFactionID::Rome, EFactionID::Rome }));
			Event->RemoveDelegate(Nested, WorldMapManager);
			CheckTerritoryProjection();
		}

		void SetMode(EGameplayMode Mode)
		{
			FCall Call(Test, PlayWorld->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
			Call.SetEnum(TEXT("NewMode"), Mode);
			Call.Execute();
			Test.TestEqual(TEXT("UI fixture uses canonical gameplay mode"), GameState->GetCurrentGameplayMode(), Mode);
		}

		bool SelectTerritory(FName ID)
		{
			FCall Call(Test, Root, TEXT("SelectTerritory"), 2);
			Call.SetName(TEXT("TerritoryID"), ID);
			Call.Execute();
			return Call.ReadBool();
		}

		UTextBlock* TerritoryDetail()
		{
			return CastChecked<UTextBlock>(Root->GetWidgetFromName(TEXT("TerritoryDetailText")));
		}

		UButton* TerritoryButton(FName ID)
		{
			// UObject names are deliberately unique across rebuilt views; stable IDs
			// identify controls, not the transient object's generated suffix.
			TArray<UWidget*> Widgets;
			Root->WidgetTree->GetAllWidgets(Widgets);
			for (UWidget* Widget : Widgets)
			{
				UButton* Button = Cast<UButton>(Widget);
				if (!Button || !Button->FindFunction(TEXT("GetTerritoryID"))) continue;
				FCall Key(Test, Button, TEXT("GetTerritoryID"), 1);
				Key.Execute();
				if (Key.ReadName() == ID) return Button;
			}
			Test.AddError(FString::Printf(TEXT("Territorial control missing for stable ID %s"), *ID.ToString()));
			return nullptr;
		}

		void CheckStrategicDetail(FName ID)
		{
			FTerritoryPresentationSnapshot Row;
			if (!Test.TestTrue(TEXT("Inspected territory is publicly readable"), GameState->TryGetTerritorySnapshot(ID, Row))) return;
			FCall Selected(Test, Root, TEXT("GetSelectedTerritoryID"), 1);
			Selected.Execute();
			Test.TestEqual(TEXT("UI carries only the selected stable key"), Selected.ReadName(), ID);
			const FString Detail = TerritoryDetail()->GetText().ToString();
			Test.TestTrue(TEXT("Detail shows canonical name and ID"), Detail.Contains(Row.DisplayName.ToString()) && Detail.Contains(ID.ToString()));
			if (Row.OwnerFaction != EFactionID::None)
			{
				Test.TestTrue(TEXT("Detail shows current catalog owner"), Detail.Contains(Row.OwnerDisplayName.ToString()));
			}
			Test.TestTrue(TEXT("Detail explicitly labels production as base, not effective"), Detail.Contains(TEXT("Base production (not effective yield)")));
			const FFactionResourceDelta& Production = Row.BaseResourceProduction;
			const TPair<const TCHAR*, int32> Fields[] = { { TEXT("Gold"), Production.Gold }, { TEXT("Food"), Production.Food },
				{ TEXT("Iron"), Production.Iron }, { TEXT("Wood"), Production.Wood }, { TEXT("Stone"), Production.Stone },
				{ TEXT("Population"), Production.Population } };
			for (const auto& Field : Fields)
			{
				Test.TestTrue(TEXT("Detail projects each of six canonical base production fields"),
					Detail.Contains(FString::Printf(TEXT("%s %s"), Field.Key, *FText::AsNumber(Field.Value).ToString())));
			}
			for (FName Neighbor : Row.AdjacentTerritoryIDs)
			{
				Test.TestTrue(TEXT("Detail includes canonical neighbor IDs"), Detail.Contains(Neighbor.ToString()));
			}
			Test.TestTrue(TEXT("Detail projects terrain"), Detail.Contains(StaticEnum<ETerrainType>()->GetDisplayNameTextByValue(static_cast<int64>(Row.TerrainType)).ToString()));
			Test.TestTrue(TEXT("Detail labels effective production separately"), Detail.Contains(TEXT("Effective production per turn")));
			Test.TestTrue(TEXT("Detail includes live resident population, not population yield"),
				Detail.Contains(FString::Printf(TEXT("Residents: %s"), *FText::AsNumber(Row.Settlement.Population).ToString())));
			if (Row.Settlement.bHasSettlement)
			{
				Test.TestTrue(TEXT("Detail includes current live settlement name"), Detail.Contains(Row.Settlement.SettlementName.ToString()));
			}
			else Test.TestTrue(TEXT("Unfounded inspector is explicit"), Detail.Contains(TEXT("Settlement: None")));
		}

		void CaptureStrategicUI(const TCHAR* Name)
		{
			// Opt-in only with a real renderer; normal NullRHI regression stays unchanged.
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureStrategicUI"))) return;
			Root->ForceLayoutPrepass();
			TArray<FColor> Pixels;
			FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Real Slate renderer captures strategic UI"),
				FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			if (!Test.TestTrue(TEXT("Strategic capture has valid pixel dimensions"),
				Size.X > 0 && Size.Y > 0 && Pixels.Num() == Size.X * Size.Y)) return;
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(),
				FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureTrainingUI")) ? TEXT("Artifacts/C008_Visual") :
				(FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureSettlementUI")) ? TEXT("Artifacts/C006_Visual") : TEXT("Artifacts/C005_Visual")));
			IFileManager::Get().MakeDirectory(*Directory, true);
			const FString Path = FPaths::Combine(Directory, FString(Name) + TEXT(".png"));
			Test.TestTrue(TEXT("Strategic capture is saved as an artifact"), FFileHelper::SaveArrayToFile(PNG, *Path));
		}

		void ExerciseStrategicUI()
		{
			Test.TestFalse(TEXT("Strategic panel starts hidden in Tactical mode"), Root->GetWidgetFromName(TEXT("StrategicPanel"))->IsVisible());
			Test.TestFalse(TEXT("Tactical mode cannot invoke territorial inspection facade"), SelectTerritory(TEXT("Rome")));
			SetMode(EGameplayMode::Strategic);
			Test.TestTrue(TEXT("Canonical Strategic mode displays the territory panel"), Root->GetWidgetFromName(TEXT("StrategicPanel"))->IsVisible());
			const TArray<FTerritoryPresentationSnapshot> Rows = GameState->GetTerritorySnapshots();
			Test.TestEqual(TEXT("Strategic UI has all nine data-backed controls"), Rows.Num(), 9);
			Test.TestEqual(TEXT("Native grid actually contains nine controls"),
				CastChecked<UPanelWidget>(Root->GetWidgetFromName(TEXT("TerritoryGrid")))->GetChildrenCount(), Rows.Num());
			for (const FTerritoryPresentationSnapshot& Row : Rows)
			{
				UButton* Button = TerritoryButton(Row.TerritoryID);
				const URomanTerritoryDefinition* Asset = URomanContentRegistrySubsystem::Get(GameState)->FindTerritory(Row.TerritoryID);
				const bool bStartingSettlement = Asset && !Asset->StartingSettlementName.IsEmpty();
				Test.TestEqual(TEXT("Bootstrap initializes authored settlement exactly once"), Row.Settlement.bHasSettlement, bStartingSettlement);
				Test.TestEqual(TEXT("Bootstrap settlement population is 100 or zero"), Row.Settlement.Population, bStartingSettlement ? 100 : 0);
				Test.TestEqual(TEXT("Bootstrap settlement name from asset"), Row.Settlement.SettlementName.ToString(),
					bStartingSettlement ? Asset->StartingSettlementName.ToString() : FString());
				if (!Button) return;
				FCall Key(Test, Button, TEXT("GetTerritoryID"), 1);
				Key.Execute();
				Test.TestEqual(TEXT("Each card carries its stable identity"), Key.ReadName(), Row.TerritoryID);
				Test.TestTrue(TEXT("Dormant and unowned controls remain inspectable"), Button->GetIsEnabled());
				Test.TestFalse(TEXT("Territory cards preserve gameplay keyboard focus"), Button->GetIsFocusable());
				const FString Label = CastChecked<UTextBlock>(Button->GetContent())->GetText().ToString();
				Test.TestTrue(TEXT("Card reads canonical territory label"), Label.Contains(Row.DisplayName.ToString()));
				if (Row.OwnerFaction != EFactionID::None) Test.TestTrue(TEXT("Card reads catalog owner"), Label.Contains(Row.OwnerDisplayName.ToString()));
				Test.TestTrue(TEXT("Card projects active/dormant/unowned status"), Label.Contains(Row.bOwnerActive ? TEXT("Active") :
					(Row.OwnerFaction == EFactionID::None ? TEXT("No owner") : TEXT("Dormant"))));
				Test.TestTrue(TEXT("Responsive region label wraps inside its scrolling grid"), CastChecked<UTextBlock>(Button->GetContent())->GetAutoWrapText());
			}
			const int32 TurnBefore = GameState->GetCurrentTurn();
			const FFactionResourceAmounts WalletBefore = GameState->GetFactionResources(EFactionID::Rome);
			ClickThroughSlate(Test, Root, TerritoryButton(TEXT("Gaul")));
			CheckStrategicDetail(TEXT("Gaul"));
			Test.TestTrue(TEXT("Dormant Gaul detail remains explicit"), TerritoryDetail()->GetText().ToString().Contains(TEXT("Dormant faction")));
			CaptureStrategicUI(TEXT("DormantGaul"));
			ClickThroughSlate(Test, Root, TerritoryButton(TEXT("Rome")));
			CheckStrategicDetail(TEXT("Rome"));
			CaptureStrategicUI(TEXT("ActiveRome"));
			ExerciseSettlementUI();
			SelectTerritory(TEXT("Rome"));
			FMulticastDelegateProperty* PublicEvent = FindFProperty<FMulticastDelegateProperty>(GameState->GetClass(), TEXT("OnTerritoryOwnerChanged"));
			FScriptDelegate UIListener;
			UIListener.BindUFunction(Root, TEXT("HandleTerritoryOwnerChanged"));
			PublicEvent->RemoveDelegate(UIListener, GameState);
			PublicEvent->AddDelegate(UIListener, GameState); // Adversarial public observer now precedes UMG.
			TerritoryProbe->bNestPublicOnce = true;
			ChangeOwner(TEXT("Rome"), EFactionID::Carthage);
			CheckStrategicDetail(TEXT("Rome"));
			Test.TestFalse(TEXT("Nested public events cannot revive the stale Carthage owner in UMG"),
				TerritoryDetail()->GetText().ToString().Contains(TEXT("Carthage")));
			ChangeOwner(TEXT("Rome"), EFactionID::Carthage);
			CheckStrategicDetail(TEXT("Rome"));
			ChangeOwner(TEXT("Rome"), EFactionID::Rome);
			for (FName ID : { FName(TEXT("Missing")), FName(NAME_None) })
			{
				Test.TestFalse(TEXT("Unknown ID rejects and clears stale local inspection"), SelectTerritory(ID));
				FCall Selected(Test, Root, TEXT("GetSelectedTerritoryID"), 1);
				Selected.Execute();
				Test.TestTrue(TEXT("Unknown ID leaves no stale selected region"), Selected.ReadName().IsNone());
			}
			Test.TestEqual(TEXT("Inspection and ownership callbacks never advance turns"), GameState->GetCurrentTurn(), TurnBefore);
			const FFactionResourceAmounts WalletAfter = GameState->GetFactionResources(EFactionID::Rome);
			Test.TestTrue(TEXT("Read-only inspection does not spend or produce resources"),
				FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&WalletBefore, &WalletAfter, 0));
			Test.TestTrue(TEXT("A valid local region can be selected again"), SelectTerritory(TEXT("Rome")));
			SetMode(EGameplayMode::Tactical);
			Test.TestFalse(TEXT("Leaving Strategic collapses the panel"), Root->GetWidgetFromName(TEXT("StrategicPanel"))->IsVisible());
			FCall Cleared(Test, Root, TEXT("GetSelectedTerritoryID"), 1);
			Cleared.Execute();
			Test.TestTrue(TEXT("Mode exit clears local territorial selection"), Cleared.ReadName().IsNone());
		}

		void ExerciseSettlementUI()
		{
			TerritoryProbe->SettlementNotifications = 0;
			GameState->OnTerritorySettlementChanged.AddDynamic(TerritoryProbe.Get(), &UTerritoryProjectionProbe::OnPublicSettlementChanged);
			FMulticastDelegateProperty* Event = FindFProperty<FMulticastDelegateProperty>(GameState->GetClass(), TEXT("OnTerritorySettlementChanged"));
			FScriptDelegate UIListener;
			UIListener.BindUFunction(Root, TEXT("HandleTerritorySettlementChanged"));
			Event->RemoveDelegate(UIListener, GameState);
			Event->AddDelegate(UIListener, GameState);
			SelectTerritory(TEXT("Sicily"));
			FTerritoryPresentationSnapshot Before;
			GameState->TryGetTerritorySnapshot(TEXT("Sicily"), Before);
			Test.TestFalse(TEXT("Founding fixture initially unfounded"), Before.Settlement.bHasSettlement);
			const FFactionResourceAmounts Wallet = GameState->GetFactionResources(EFactionID::Rome);
			const int32 Turn = GameState->GetCurrentTurn();
			Test.TestFalse(TEXT("Unknown settlement request rejects"), FoundSettlement(TEXT("Missing"), TEXT("Invalid")).WasSuccessful());
			Test.TestEqual(TEXT("Whitespace name rejects"), FoundSettlement(TEXT("Sicily"), TEXT(" \t ")).Failure, ESettlementFoundingFailure::InvalidName);
			Test.TestEqual(TEXT("Rejected founding sends no notification"), TerritoryProbe->SettlementNotifications, 0);
			TerritoryProbe->bNestSettlementOnce = true;
			Test.TestTrue(TEXT("Stable-ID founding succeeds"), FoundSettlement(TEXT("Sicily"), TEXT("Syracuse Test")).WasSuccessful());
			Test.TestEqual(TEXT("Nested founding gives exactly two committed events"), TerritoryProbe->SettlementNotifications, 2);
			Test.TestFalse(TEXT("Old snapshot remains unfounded"), Before.Settlement.bHasSettlement);
			CheckStrategicDetail(TEXT("Sicily"));
			Test.TestEqual(TEXT("Duplicate founding rejects"), FoundSettlement(TEXT("Sicily"), TEXT("Overwrite")).Failure, ESettlementFoundingFailure::AlreadyFounded);
			if (FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureSettlementUI"))) CaptureStrategicUI(TEXT("FoundedSicily"));
			Test.TestEqual(TEXT("Duplicate sends no extra event"), TerritoryProbe->SettlementNotifications, 2);
			Test.TestEqual(TEXT("Founding does not advance turn"), GameState->GetCurrentTurn(), Turn);
			const FFactionResourceAmounts AfterWallet = GameState->GetFactionResources(EFactionID::Rome);
			Test.TestTrue(TEXT("Legacy free founding does not debit wallet"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &AfterWallet, 0));
			FCall ActorLookup(Test, WorldMapManager, TEXT("GetTerritoryByID"), 2);
			ActorLookup.SetName(TEXT("TerritoryID"), TEXT("Sicily"));
			ActorLookup.Execute();
			AActor* Actor = Cast<AActor>(ActorLookup.ReadObject());
			if (Test.TestNotNull(TEXT("Settlement actor wrapper exists"), Actor))
			{
				Test.TestTrue(TEXT("Actor delegates has-settlement"), ReadBool(Test, Actor, TEXT("HasSettlement")));
				Test.TestEqual(TEXT("Actor delegates live residents"), ReadInt(Test, Actor, TEXT("GetPopulation")), 100);
				FCall Production(Test, Actor, TEXT("GetResourceProduction"), 1);
				Production.Execute();
				FTerritoryPresentationSnapshot Live;
				GameState->TryGetTerritorySnapshot(TEXT("Sicily"), Live);
				const FFactionResourceDelta ActorProduction = Production.ReadStruct<FFactionResourceDelta>();
				Test.TestTrue(TEXT("Actor delegates effective production"), FFactionResourceDelta::StaticStruct()->CompareScriptStruct(&ActorProduction, &Live.EffectiveResourceProduction, 0));
				Actor->Destroy();
				FTerritoryPresentationSnapshot After;
				GameState->TryGetTerritorySnapshot(TEXT("Sicily"), Before);
				Test.TestTrue(TEXT("Actor destruction does not erase campaign settlement"), Before.Settlement.bHasSettlement);
				ChangeOwner(TEXT("Sicily"), EFactionID::Rome);
				GameState->TryGetTerritorySnapshot(TEXT("Sicily"), After);
				Test.TestTrue(TEXT("Settlement survives ownership change without actor"),
					FTerritorySettlementState::StaticStruct()->CompareScriptStruct(&Before.Settlement, &After.Settlement, 0));
				// Subsequent normal turns must credit this actor-independent yield.
			}
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
			APlayerCameraManager* CameraManager = Controller->PlayerCameraManager;
			UWidget* Crosshair = Root->GetWidgetFromName(TEXT("CrosshairText"));
			UTextBlock* PossessionText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("PossessionText")));
			if (!Test.TestTrue(TEXT("Normal bootstrap supplies a living Rome soldier, camera, and possession controls"),
				Soldier && CameraPawn && CameraManager && Crosshair && PossessionText)) return false;
			const float OriginalViewPitchMin = CameraManager->ViewPitchMin;
			const float OriginalViewPitchMax = CameraManager->ViewPitchMax;

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
			Test.TestEqual(TEXT("Possession applies its downward pitch limit"), CameraManager->ViewPitchMin, -80.0f);
			Test.TestEqual(TEXT("Possession applies its upward pitch limit"), CameraManager->ViewPitchMax, 80.0f);
			Test.TestTrue(TEXT("Possession changes the authoritative controller mode"), ReadBool(Test, Controller, TEXT("IsInFirstPersonMode")));
			Test.TestTrue(TEXT("Possession events display crosshair and soldier vitals"), Crosshair->IsVisible() && PossessionText->IsVisible());
			Test.TestFalse(TEXT("Possessed mode never shows the strategic panel"), Root->GetWidgetFromName(TEXT("StrategicPanel"))->IsVisible());
			Test.TestTrue(TEXT("Possession UI reads the same damaged health"), PossessionText->GetText().ToString().Contains(HealthLabel));
			Test.TestFalse(TEXT("Possession mode header remains single-line after its text grows"), PhaseText->GetAutoWrapText());
			Test.TestTrue(TEXT("Header follows the actual possessed mode"), PhaseText->GetText().ToString().StartsWith(TEXT("Possessed   |   Turn ")));
			CheckGameStateProjection();

			FCall KillCall(Test, Soldier, TEXT("SetCurrentHealth"), 1);
			KillCall.SetInt(TEXT("NewHealth"), 0);
			KillCall.Execute();
			Test.TestFalse(TEXT("Canonical health transition kills the possessed soldier"), ReadBool(Test, Soldier, TEXT("IsAlive")));
			Test.TestTrue(TEXT("Possessed soldier death restores the original camera pawn"), Controller->GetPawn() == CameraPawn);
			Test.TestEqual(TEXT("Death restores the tactical minimum pitch"), CameraManager->ViewPitchMin, OriginalViewPitchMin);
			Test.TestEqual(TEXT("Death restores the tactical maximum pitch"), CameraManager->ViewPitchMax, OriginalViewPitchMax);
			Test.TestFalse(TEXT("Death exits the controller's possession mode"), ReadBool(Test, Controller, TEXT("IsInFirstPersonMode")));
			Test.TestFalse(TEXT("Restored tactical header remains single-line"), PhaseText->GetAutoWrapText());
			Test.TestTrue(TEXT("Header follows the restored tactical mode"), PhaseText->GetText().ToString().StartsWith(TEXT("Tactical   |   Turn ")));
			FCall SelectedCall(Test, Controller, TEXT("GetSelectedUnits"), 1);
			SelectedCall.Execute();
			Test.TestEqual(TEXT("Death removes the soldier from canonical selection"), SelectedCall.ReadObjectArrayCount(), 0);
			Test.TestEqual(TEXT("Death event collapses the possession crosshair"), Crosshair->GetVisibility(), ESlateVisibility::Collapsed);
			Test.TestEqual(TEXT("Death event collapses soldier vitals"), PossessionText->GetVisibility(), ESlateVisibility::Collapsed);
			Test.TestEqual(TEXT("Death leaves no stale unit selection panel"),
				Root->GetWidgetFromName(TEXT("SelectionPanel"))->GetVisibility(), ESlateVisibility::Collapsed);
			CheckGameStateProjection();
			return true;
		}

		void ExerciseActiveSliceFactions()
		{
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(Controller);
			if (Test.TestNotNull(TEXT("Authored content registry remains ready"), Registry))
			{
				Test.TestEqual(TEXT("Catalog still contains three authored factions"), Registry->GetAllFactions().Num(), 3);
				Test.TestNotNull(TEXT("Inactive Gaul's definition is preserved"), Registry->FindFaction(EFactionID::Gaul));
			}
			FCall Active(Test, FactionManager, TEXT("GetActiveFactions"), 1);
			Active.Execute();
			Test.TestTrue(TEXT("Exactly Rome and Carthage participate in deterministic order"),
				Active.ReadEnumArray<EFactionID>() == TArray<EFactionID>({ EFactionID::Rome, EFactionID::Carthage }));
			FCall AI(Test, FactionManager, TEXT("GetAIFactions"), 1);
			AI.Execute();
			Test.TestTrue(TEXT("Only Carthage takes AI stages when Rome is player"),
				AI.ReadEnumArray<EFactionID>() == TArray<EFactionID>({ EFactionID::Carthage }));
			for (EFactionID Faction : { EFactionID::None, EFactionID::Rome, EFactionID::Carthage,
				EFactionID::Gaul, EFactionID::Greece, EFactionID::Egypt, EFactionID::Britannia, static_cast<EFactionID>(255) })
			{
				FCall Query(Test, FactionManager, TEXT("IsFactionActive"), 2);
				Query.SetEnum(TEXT("FactionID"), Faction);
				Query.Execute();
				Test.TestEqual(TEXT("Active roster excludes dormant, absent and invalid factions"), Query.ReadBool(),
					Faction == EFactionID::Rome || Faction == EFactionID::Carthage);
				if (Faction == EFactionID::Rome || Faction == EFactionID::Carthage) continue;
				FCall SetPlayer(Test, FactionManager, TEXT("SetPlayerFaction"), 1);
				SetPlayer.SetEnum(TEXT("NewFaction"), Faction);
				SetPlayer.Execute();
				Test.TestTrue(TEXT("Inactive faction cannot become player or alter GameState projection"), GameState->GetPlayerFaction() == EFactionID::Rome);
				FFactionResourceDelta Delta;
				Delta.Gold = 17;
				FCall Add(Test, FactionManager, TEXT("ApplyFactionResourceDelta"), 3);
				Add.SetEnum(TEXT("FactionID"), Faction);
				Add.SetStruct(TEXT("Delta"), Delta);
				Add.Execute();
				Test.TestTrue(TEXT("Inactive faction has no mutable wallet"),
					Add.ReadStruct<FFactionResourceTransactionResult>().Failure == EResourceTransactionFailure::InvalidFaction);
				FCall Spend(Test, FactionManager, TEXT("TrySpendFactionResources"), 3);
				Spend.SetEnum(TEXT("FactionID"), Faction);
				Spend.SetStruct(TEXT("Cost"), FFactionResourceAmounts());
				Spend.Execute();
				Test.TestTrue(TEXT("Even zero-cost transactions reject nonparticipants"),
					Spend.ReadStruct<FFactionResourceTransactionResult>().Failure == EResourceTransactionFailure::InvalidFaction);
				Test.TestTrue(TEXT("Inactive faction balance stays empty"), ReadResources(Test, FactionManager, Faction).IsZero());
				FCall Relation(Test, FactionManager, TEXT("GetDiplomaticStatus"), 3);
				Relation.SetEnum(TEXT("Faction1"), EFactionID::Rome);
				Relation.SetEnum(TEXT("Faction2"), Faction);
				Relation.Execute();
				Test.TestTrue(TEXT("Inactive relation query is invalid, not implicit neutral"), Relation.ReadEnum<EDiplomaticStatus>() == EDiplomaticStatus::Invalid);
				FCall Diplomacy(Test, FactionManager, TEXT("SetDiplomaticStatus"), 4);
				Diplomacy.SetEnum(TEXT("Faction1"), Faction);
				Diplomacy.SetEnum(TEXT("Faction2"), EFactionID::Rome);
				Diplomacy.SetEnum(TEXT("NewStatus"), EDiplomaticStatus::War);
				Diplomacy.Execute();
				Test.TestTrue(TEXT("Inactive diplomacy mutation rejects with typed failure"),
					Diplomacy.ReadStruct<FDiplomaticChangeResult>().Failure == EDiplomaticChangeFailure::InvalidFaction);
			}
			FCall SetPlayer(Test, FactionManager, TEXT("SetPlayerFaction"), 1);
			SetPlayer.SetEnum(TEXT("NewFaction"), EFactionID::Carthage);
			SetPlayer.Execute();
			Test.TestTrue(TEXT("Other active participant can become player"), GameState->GetPlayerFaction() == EFactionID::Carthage);
			AI.Execute();
			Test.TestTrue(TEXT("AI roster derives from the current player without a second owner"),
				AI.ReadEnumArray<EFactionID>() == TArray<EFactionID>({ EFactionID::Rome }));
			SetPlayer.SetEnum(TEXT("NewFaction"), EFactionID::Rome);
			SetPlayer.Execute();
			FCall War(Test, FactionManager, TEXT("AreAtWar"), 3);
			War.SetEnum(TEXT("Faction1"), EFactionID::Rome);
			War.SetEnum(TEXT("Faction2"), EFactionID::Carthage);
			War.Execute();
			Test.TestTrue(TEXT("Rejected requests preserve the slice's initial Rome-Carthage war"), War.ReadBool());
			FCall Owner(Test, WorldMapManager, TEXT("GetTerritoryOwner"), 2);
			Owner.SetName(TEXT("TerritoryID"), TEXT("Gaul"));
			Owner.Execute();
			Test.TestTrue(TEXT("Inactive polity retains authored territory ownership"), Owner.ReadEnum<EFactionID>() == EFactionID::Gaul);
		}

		bool ExerciseStableAdjacency()
		{
			FCall Neighbors(Test, WorldMapManager, TEXT("GetAdjacentTerritoryIDs"), 2);
			Neighbors.SetName(TEXT("TerritoryID"), TEXT("Rome"));
			Neighbors.Execute();
			const TArray<FName> IDs = Neighbors.ReadNameArray();
			if (!Test.TestTrue(TEXT("Authored Rome has campaign connections"), !IDs.IsEmpty())) return false;
			auto Resolve = [this](FName ID)
			{
				FCall Call(Test, WorldMapManager, TEXT("GetTerritoryByID"), 2);
				Call.SetName(TEXT("TerritoryID"), ID);
				Call.Execute();
				return Cast<AActor>(Call.ReadObject());
			};
			AActor* Rome = Resolve(TEXT("Rome"));
			AActor* Neighbor = Resolve(IDs[0]);
			if (!Test.TestTrue(TEXT("Canonical IDs resolve to live campaign actors"), Rome && Neighbor)) return false;
			auto Adjacent = [this](AActor* First, AActor* Second)
			{
				FCall Call(Test, WorldMapManager, TEXT("AreTerritoriesAdjacent"), 3);
				Call.SetObject(TEXT("Territory1"), First);
				Call.SetObject(TEXT("Territory2"), Second);
				Call.Execute();
				return Call.ReadBool();
			};
			Test.TestTrue(TEXT("Actor wrapper projects the authored connection"), Adjacent(Rome, Neighbor));
			Test.TestFalse(TEXT("Actor wrapper rejects self"), Adjacent(Rome, Rome));
			Test.TestFalse(TEXT("Actor wrapper rejects null"), Adjacent(Rome, nullptr));
			const FVector OriginalLocation = Neighbor->GetActorLocation();
			FTerritoryPresentationSnapshot BeforeMove;
			Test.TestTrue(TEXT("Neighbor has a public snapshot before presentation moves"),
				GameState->TryGetTerritorySnapshot(IDs[0], BeforeMove));
			Neighbor->SetActorLocation(Rome->GetActorLocation() + FVector(1000000.0, 0.0, 0.0));
			Test.TestTrue(TEXT("Moving presentation cannot change campaign adjacency"), Adjacent(Rome, Neighbor));
			FCall Actors(Test, WorldMapManager, TEXT("GetAdjacentTerritories"), 2);
			Actors.SetObject(TEXT("Territory"), Rome);
			Actors.Execute();
			Test.TestEqual(TEXT("Moved neighbor remains in canonical actor projection"), Actors.ReadObjectArrayCount(), IDs.Num());
			Neighbors.Execute();
			Test.TestTrue(TEXT("Stable neighbor list survives actor movement"), Neighbors.ReadNameArray() == IDs);
			FTerritoryPresentationSnapshot AfterMove;
			Test.TestTrue(TEXT("Moved actor retains public territory metadata"), GameState->TryGetTerritorySnapshot(IDs[0], AfterMove));
			Test.TestTrue(TEXT("Moving a visual actor cannot alter any public territory field"),
				FTerritoryPresentationSnapshot::StaticStruct()->CompareScriptStruct(&BeforeMove, &AfterMove, 0));
			Neighbor->SetActorLocation(OriginalLocation);
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
			BattleText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("BattleText")));
			OutcomeText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("OutcomeText")));
			TrainButton = Cast<UButton>(Root->GetWidgetFromName(TEXT("TrainButton")));
			UTextBlock* DiagnosticText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("DiagnosticText")));
			UTextBlock* SelectionText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("SelectionText")));
			UTextBlock* ObjectiveText = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ObjectiveText")));
			if (!Test.TestTrue(TEXT("All production UI controls materialize as the expected UMG types"),
				ResourceText && PhaseText && ProductionText && BattleText && OutcomeText && TrainButton && DiagnosticText && SelectionText && ObjectiveText &&
				Root->GetWidgetFromName(TEXT("BuildingMenu")) && Root->GetWidgetFromName(TEXT("OutcomePanel")))) return false;
			Test.TestTrue(TEXT("Flexible resource header can wrap long six-currency balances"), ResourceText->GetAutoWrapText());
			Test.TestFalse(TEXT("Separate mode/turn column preserves single-line label"), PhaseText->GetAutoWrapText());

			AGameModeBase* GameMode = World->GetAuthGameMode();
			GameState = World->GetGameState<ARomanEmpireGameState>();
			FactionManager = ReadObject(Test, GameMode, TEXT("GetFactionManager"));
			CampaignManager = ReadObject(Test, GameMode, TEXT("GetCampaignManager"));
			WorldMapManager = ReadObject(Test, GameMode, TEXT("GetWorldMapManager"));
			if (!Test.TestTrue(TEXT("Normal startup supplies the public GameState and canonical managers"),
				GameState && FactionManager && CampaignManager && WorldMapManager)) return false;
			if (!ExerciseStableAdjacency()) return false;
			ExerciseActiveSliceFactions();
			ExerciseTerritorialEvents();
			ExerciseStrategicUI();
			TacticalBattle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (!Test.TestNotNull(TEXT("Normal startup supplies one tactical battle owner"), TacticalBattle)) return false;
			const FTacticalBattleResult InitialBattle = TacticalBattle->GetResult();
			Test.TestEqual(TEXT("Known prototype forces create two result squads"), InitialBattle.Squads.Num(), 2);
			if (InitialBattle.Squads.Num() == 2)
			{
				Test.TestEqual(TEXT("Initial Roman formation has 15 survivors"), InitialBattle.Squads[0].Survivors, 15);
				Test.TestEqual(TEXT("Initial Carthaginian formation has 10 survivors"), InitialBattle.Squads[1].Survivors, 10);
			}
			Test.TestEqual(TEXT("Initial objective has its stable ID"), InitialBattle.ObjectiveID, FName(TEXT("CarthaginianCamp")));
			Test.TestTrue(TEXT("Battle UMG initially projects both known forces"),
				BattleText->GetText().ToString().Contains(TEXT("Allies 15/15")) &&
				BattleText->GetText().ToString().Contains(TEXT("Foes 10/10")));
			CheckRelayContract();
			FCall ModeCall(Test, GameMode, TEXT("SetGameplayMode"), 1);
			ModeCall.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical);
			ModeCall.Execute();
			CheckGameStateProjection();
			Test.TestEqual(TEXT("Startup UI reads the real wallet"), ResourceText->GetText().ToString(), ResourceLabel(ReadResources(Test, FactionManager)));
			AddGold(Test, FactionManager, 137);
			CheckGameStateProjection();
			const FFactionResourceAmounts AfterDelta = ReadResources(Test, FactionManager);
			Test.TestEqual(TEXT("Resource event updates visible text synchronously without a UI tick"),
				ResourceText->GetText().ToString(), ResourceLabel(AfterDelta));
			Test.TestTrue(TEXT("Diagnostic text observes the same canonical gold"),
				DiagnosticText->GetText().ToString().Contains(FString::Printf(TEXT("Gold=%d"), AfterDelta.Gold)));
			Test.TestTrue(TEXT("Objective text observes the same economic progress"),
				ObjectiveText->GetText().ToString().Contains(FString::Printf(TEXT("Economy: %d /"), AfterDelta.Gold)));
			UClass* SoldierClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.Legionary")).TryLoadClass<AActor>();
			if (!Test.TestNotNull(TEXT("Native soldier class exists for dormant-faction guard"), SoldierClass)) return false;
			const FTransform DormantTransform(FVector(4000.0, 4000.0, 150.0));
			AActor* Dormant = World->SpawnActorDeferred<AActor>(SoldierClass, DormantTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Test.TestNotNull(TEXT("Dormant guard soldier can spawn deferred"), Dormant)) return false;
			FCall Initialize(Test, Dormant, TEXT("InitializeUnit"), 3);
			Initialize.SetName(TEXT("DefinitionID"), TEXT("Legionary"));
			Initialize.SetEnum(TEXT("NewOwner"), EFactionID::Rome);
			Initialize.Execute();
			if (!Test.TestTrue(TEXT("Guard fixture uses normal authored soldier initialization"), Initialize.ReadBool()))
			{
				Dormant->Destroy();
				return false;
			}
			Dormant->FinishSpawning(DormantTransform);
			FCall SetDormant(Test, Dormant, TEXT("SetOwnerFaction"), 1);
			SetDormant.SetEnum(TEXT("NewOwner"), EFactionID::Gaul);
			SetDormant.Execute();
			FCall(Test, Dormant, TEXT("CommandHold"), 0).Execute();
			AdvanceTurn();
			FCall DormantOrder(Test, Dormant, TEXT("GetOrderType"), 1);
			DormantOrder.Execute();
			Test.TestTrue(TEXT("Campaign AI cannot overwrite dormant faction HOLD"), DormantOrder.ReadEnum<EUnitOrderType>() == EUnitOrderType::Hold);
			Dormant->Destroy();
			Test.TestTrue(TEXT("Turn skips inactive Gaul production without creating a wallet"),
				GameState->GetFactionResources(EFactionID::Gaul).IsZero());
			if (!ExerciseUnitSelectionAndPossession(World, SelectionText)) return false;

			UClass* BarracksClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.Barracks")).TryLoadClass<AActor>();
			if (!Test.TestNotNull(TEXT("Native production class is available"), BarracksClass)) return false;
			AActor* InitialBarracks = nullptr;
			int32 InitialBarracksCount = 0;
			for (TActorIterator<AActor> It(World, BarracksClass); It; ++It)
			{
				++InitialBarracksCount;
				if (FVector::Dist2D(It->GetActorLocation(), FVector(-600.0, -800.0, 0.0)) < 1.0)
				{
					InitialBarracks = *It;
				}
			}
			if (!Test.TestEqual(TEXT("Scenario starts with one real barracks"), InitialBarracksCount, 1) ||
				!Test.TestNotNull(TEXT("Real barracks replaces the camp prop at its original site"), InitialBarracks)) return false;
			Test.TestTrue(TEXT("Initial barracks is complete and ready for production"), ReadBool(Test, InitialBarracks, TEXT("IsComplete")));
			FCall InitialOwner(Test, InitialBarracks, TEXT("GetOwnerFaction"), 1);
			InitialOwner.Execute();
			Test.TestEqual(TEXT("Initial barracks belongs to the player faction"), InitialOwner.ReadEnum<EFactionID>(), EFactionID::Rome);
			FCall SelectInitial(Test, Controller, TEXT("SelectBuilding"), 1);
			SelectInitial.SetObject(TEXT("Building"), InitialBarracks);
			SelectInitial.Execute();
			Test.TestEqual(TEXT("The camp's real barracks is selectable without a world-trace shortcut"),
				ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(InitialBarracks));
			Test.TestTrue(TEXT("Initial barracks exposes the functional UMG TRAIN control"), TrainButton->IsVisible());
			int32 DecorativeBarracksCount = 0;
			for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
			{
				if (FVector::Dist(It->GetActorLocation(), FVector(-600.0, -800.0, 150.0)) < 1.0) ++DecorativeBarracksCount;
			}
			Test.TestEqual(TEXT("No decorative barracks mesh remains at the original camp site"), DecorativeBarracksCount, 0);
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
			if (!Test.TestFalse(TEXT("The selected producer defines trainable stable IDs"), Data.TrainableUnits.IsEmpty())) return false;
			TrainingUnitID = Data.TrainableUnits[0];
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(World);
			const URomanUnitDefinition* UnitDefinition = Registry ? Registry->FindUnit(TrainingUnitID) : nullptr;
			if (!Test.TestNotNull(TEXT("TRAIN uses the canonical unit definition"), UnitDefinition)) return false;
			TrainingCost = UnitDefinition->Definition.TrainingCost;
			TrainingDuration = UnitDefinition->Definition.TrainingTime;
			if (!Test.TestTrue(TEXT("Training fixture has a positive gold cost and duration"),
				TrainingCost.Gold > 0 && TrainingDuration > 0.0f)) return false;
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
			const FFactionResourceAmounts BeforeActiveOrder = ReadResources(Test, FactionManager);
			ClickThroughSlate(Test, Root, TrainButton);
			CheckTrainingCharge(TEXT("Initial Slate TRAIN"), BeforeActiveOrder, TrainingCost);
			Test.TestTrue(TEXT("UMG click starts training on the selected producer"), ReadBool(Test, Producer, TEXT("IsTraining")));
			Test.TestFalse(TEXT("UMG click leaves the other producer idle"), ReadBool(Test, OtherProducer, TEXT("IsTraining")));
			Test.TestEqual(TEXT("One initial click accepts exactly one order"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 0);
			Test.TestEqual(TEXT("The UMG click preserves the canonical building selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Producer));
			Test.TestTrue(TEXT("Production event refreshes active training in UMG"), ProductionText->GetText().ToString().Contains(TEXT("active")));
			Test.TestEqual(TEXT("Training cost is reflected from the updated canonical wallet"), ResourceText->GetText().ToString(), ResourceLabel(ReadResources(Test, FactionManager)));
			CheckGameStateProjection();
			return true;
		}
	};

	class FExerciseCampaignArmies : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseCampaignArmies(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Campaign armies PIE timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UObject* Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetCampaignManager"));
			const TArray<FCampaignArmy> Initial = State->GetCampaignArmies();
			if (!Test.TestTrue(TEXT("Live bootstrap has exactly two campaign armies"), Initial.Num() == 2)) return true;
			const UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (!Test.TestNotNull(TEXT("Army inventory materializes an actual tactical encounter"), Battle)) return true;
			const FTacticalBattleResult Result = Battle->GetResult();
			for (const FCampaignArmy& Army : Initial)
			{
				Test.TestTrue(TEXT("Campaign identity is valid"), Army.ArmyID.IsValid());
				FCampaignArmy Lookup;
				Test.TestTrue(TEXT("Public lookup returns canonical army"), State->TryGetCampaignArmy(Army.ArmyID, Lookup));
				Test.TestTrue(TEXT("Public lookup and array agree"), FCampaignArmy::StaticStruct()->CompareScriptStruct(&Army, &Lookup, 0));
				FTerritoryPresentationSnapshot Home;
				Test.TestTrue(TEXT("Army has an authored stable territory"), State->TryGetTerritorySnapshot(Army.TerritoryID, Home));
				Test.TestEqual(TEXT("Initial placement uses owned territory"), Home.OwnerFaction, Army.Faction);
				const FTacticalBattleSquadResult* Squad = Result.Squads.FindByPredicate([&](const FTacticalBattleSquadResult& Value) { return Value.Faction == Army.Faction; });
				if (!Test.TestNotNull(TEXT("Canonical force has actual faction squad"), Squad)) return true;
				Test.TestEqual(TEXT("Tactical quantity comes from canonical inventory"), Squad->InitialUnits, FCampaignArmyState::GetUnitCount(Army));
				for (const FTacticalBattleSquadResult& Other : Result.Squads)
					Test.TestTrue(TEXT("No local squad handle masquerades as persistent army ID"), Army.ArmyID != Other.SquadID);
			}
			Test.TestTrue(TEXT("Campaign identities are distinct"), Initial[0].ArmyID != Initial[1].ArmyID);
			TArray<FCampaignArmy> Copies = Initial;
			Copies[0].TerritoryID = TEXT("Other"); Copies[0].Units[0].Count = 999;
			Test.TestTrue(TEXT("Mutating detached array cannot edit authority"), SameArmies(Initial, State->GetCampaignArmies()));
			FCampaignArmy Stale = Initial[0];
			Test.TestFalse(TEXT("Unknown public identity rejects"), State->TryGetCampaignArmy(FGuid(), Stale));
			Test.TestFalse(TEXT("Unknown read clears prior identity"), Stale.ArmyID.IsValid());
			AActor* Enemy = nullptr;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->FindFunction(TEXT("CommandHold"))) continue;
				FCall(Test, *It, TEXT("CommandHold"), 0).Execute();
				FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute();
				if (Owner.ReadEnum<EFactionID>() == EFactionID::Carthage) Enemy = *It;
			}
			if (!Test.TestNotNull(TEXT("Actual casualty fixture exists"), Enemy)) return true;
			FCall Kill(Test, Enemy, TEXT("SetCurrentHealth"), 1); Kill.SetInt(TEXT("NewHealth"), 0); Kill.Execute();
			Test.TestTrue(TEXT("Encounter casualty does not prematurely commit strategic inventory"), SameArmies(Initial, State->GetCampaignArmies()));
			UObject* Map = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetWorldMapManager"));
			FCall Territory(Test, Map, TEXT("GetTerritoryByID"), 2); Territory.SetName(TEXT("TerritoryID"), Initial[0].TerritoryID); Territory.Execute();
			AActor* TerritoryActor = Cast<AActor>(Territory.ReadObject());
			if (Test.TestNotNull(TEXT("Army territory presentation actor exists"), TerritoryActor)) TerritoryActor->Destroy();
			Test.TestTrue(TEXT("Army values survive presentation actor destruction"), SameArmies(Initial, State->GetCampaignArmies()));
			Test.TestEqual(TEXT("Exactly one canonical army relay"), ListenerCount(Test, Manager, TEXT("OnCampaignArmiesChanged"), State), 1);
			TStrongObjectPtr<UCampaignArmyProbe> Probe(NewObject<UCampaignArmyProbe>()); Probe->State = State;
			State->OnCampaignArmiesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignArmyProbe::OnArmiesChanged);
			FCall Restart(Test, Manager, TEXT("StartNewCampaign"), 1); Restart.SetEnum(TEXT("PlayerFaction"), EFactionID::Rome); Restart.Execute();
			const TArray<FCampaignArmy> Fresh = State->GetCampaignArmies();
			Test.TestEqual(TEXT("New campaign replaces whole strategic roster"), Fresh.Num(), 2);
			Test.TestEqual(TEXT("Restart publishes one army invalidation"), Probe->Notifications, 1);
			Test.TestTrue(TEXT("Observer queries complete committed roster"), SameArmies(Fresh, Probe->LastObserved));
			for (const FCampaignArmy& Army : Fresh)
				for (const FCampaignArmy& Old : Initial) Test.TestTrue(TEXT("New campaign has new identities"), Army.ArmyID != Old.ArmyID);
			State->OnCampaignArmiesChanged.RemoveDynamic(Probe.Get(), &UCampaignArmyProbe::OnArmiesChanged);
			State->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			Test.TestEqual(TEXT("Read-model teardown removes army relay"), ListenerCount(Test, Manager, TEXT("OnCampaignArmiesChanged"), State), 0);
			Test.TestTrue(TEXT("Ended read model exposes no stale armies"), State->GetCampaignArmies().IsEmpty());
			Stale = Initial[0];
			Test.TestFalse(TEXT("Ended read model rejects identity lookup"), State->TryGetCampaignArmy(Stale.ArmyID, Stale));
			Test.TestTrue(TEXT("Ended read clears stale units"), Stale.Units.IsEmpty());
			FCall Canonical(Test, Manager, TEXT("GetCampaignArmies"), 1); Canonical.Execute();
			Test.TestTrue(TEXT("Read-model teardown does not erase canonical owner"), SameArmies(Fresh, Canonical.ReadStructArray<FCampaignArmy>()));
			CastChecked<AActor>(Manager)->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			FCall Ended(Test, Manager, TEXT("GetCampaignArmies"), 1); Ended.Execute();
			Test.TestTrue(TEXT("Ended owner exposes no roster"), Ended.ReadStructArray<FCampaignArmy>().IsEmpty());
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		static bool SameArmies(const TArray<FCampaignArmy>& A, const TArray<FCampaignArmy>& B)
		{
			if (A.Num() != B.Num()) return false;
			for (int32 Index = 0; Index < A.Num(); ++Index)
				if (!FCampaignArmy::StaticStruct()->CompareScriptStruct(&A[Index], &B[Index], 0)) return false;
			return true;
		}
	};

	struct FStrategicBootstrapScope
	{
		UClass* ModeClass = nullptr;
		FBoolProperty* Property = nullptr;
		FDelegateHandle WorldInitialized, ActorSpawned;
		TWeakObjectPtr<UWorld> FixtureWorld;
		explicit FStrategicBootstrapScope(UClass* Class)
		{
			ModeClass = Class;
			Property = Class ? FindFProperty<FBoolProperty>(Class, TEXT("bStartWithPrototypeEncounter")) : nullptr;
			WorldInitialized = FWorldDelegates::OnPostWorldInitialization.AddLambda([this](UWorld* World, const UWorld::InitializationValues)
			{
				if (!World || World->WorldType != EWorldType::PIE || !ModeClass || !Property) return;
				FixtureWorld = World;
				ActorSpawned = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([this](AActor* Actor)
				{
					if (Actor && Actor->IsA(ModeClass)) Property->SetPropertyValue_InContainer(Actor, false);
				}));
			});
		}
		~FStrategicBootstrapScope()
		{
			FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitialized);
			if (FixtureWorld.IsValid()) FixtureWorld->RemoveOnActorSpawnedHandler(ActorSpawned);
		}
	};

	class FExerciseCampaignEncounter : public IAutomationLatentCommand
	{
	public:
		FExerciseCampaignEncounter(FAutomationTestBase* InTest, bool bInVictory, TSharedPtr<FStrategicBootstrapScope> InScope, bool bInReinforcements = false)
			: Test(*InTest), bVictory(bInVictory), bReinforcements(bInReinforcements), Scope(MoveTemp(InScope)), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Campaign encounter PIE timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UObject* Mode = World->GetAuthGameMode();
			UObject* Manager = ReadObject(Test, Mode, TEXT("GetCampaignManager"));
			UObject* Map = ReadObject(Test, Mode, TEXT("GetWorldMapManager"));
			UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (!Manager || !Map || !Battle) return true;
			const auto Move = [&](FGuid Army, FName Target)
			{
				FCall Call(Test, Manager, TEXT("MoveCampaignArmy"), 3); Call.SetGuid(TEXT("ArmyID"), Army); Call.SetName(TEXT("TargetTerritoryID"), Target); Call.Execute();
				return Call.ReadStruct<FCampaignArmyMutationResult>().Failure;
			};
			const auto Begin = [&](FGuid RequestedArmy = FGuid())
			{
				FCall Call(Test, Manager, TEXT("BeginCampaignEncounter"), 3); Call.SetGuid(TEXT("AttackingArmyID"), RequestedArmy.IsValid() ? RequestedArmy : Attacker); Call.SetName(TEXT("TargetTerritoryID"), TEXT("Africa")); Call.Execute();
				return Call.ReadEnum<ECampaignEncounterFailure>();
			};
			const auto Apply = [&]()
			{
				FCall Call(Test, Manager, TEXT("ApplyCurrentEncounterResult"), 1); Call.Execute(); return Call.ReadEnum<ECampaignEncounterFailure>();
			};
			if (Stage == 0)
			{
				Test.TestFalse(TEXT("Strategic bootstrap has no sandbox battle to overwrite"), Battle->HasEncounter());
				Test.TestEqual(TEXT("Strategic bootstrap starts in strategic mode"), State->GetCurrentGameplayMode(), EGameplayMode::Strategic);
				Test.TestFalse(TEXT("No prepared plan before command"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
				const auto Initial = State->GetCampaignArmies();
				for (const FCampaignArmy& Army : Initial)
				{
					if (Army.Faction == EFactionID::Rome) Attacker = Army.ArmyID;
					else if (Army.Faction == EFactionID::Carthage) Defender = Army.ArmyID;
				}
				if (!Test.TestTrue(TEXT("Actual canonical army handles exist"), Attacker.IsValid() && Defender.IsValid())) return true;
				Test.TestEqual(TEXT("No invented result before deployment"), Apply(), ECampaignEncounterFailure::NoEncounter);
				Test.TestEqual(TEXT("Unknown movement rejects"), Move(FGuid(), TEXT("Sicily")), ECampaignArmyFailure::UnknownArmy);
				Test.TestEqual(TEXT("Enemy movement rejects"), Move(Defender, TEXT("Sicily")), ECampaignArmyFailure::NotOwned);
				Test.TestEqual(TEXT("Self movement is not adjacency"), Move(Attacker, TEXT("Rome")), ECampaignArmyFailure::NotAdjacent);
				Test.TestEqual(TEXT("Hostile destination cannot use peaceful movement"), Move(Attacker, TEXT("Africa")), ECampaignArmyFailure::NotOwned);
				FCall Tactical(Test, Mode, TEXT("SetGameplayMode"), 1); Tactical.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); Tactical.Execute();
				Test.TestEqual(TEXT("Wrong-mode movement rejects"), Move(Attacker, TEXT("Sicily")), ECampaignArmyFailure::WrongMode);
				Test.TestEqual(TEXT("Wrong-mode deployment rejects"), Begin(), ECampaignEncounterFailure::WrongMode);
				FCall Strategic(Test, Mode, TEXT("SetGameplayMode"), 1); Strategic.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); Strategic.Execute();
				Test.TestEqual(TEXT("Cannot deploy an enemy-owned army"), Begin(Defender), ECampaignEncounterFailure::InvalidAttacker);
				FTerritoryPresentationSnapshot Sicily; State->TryGetTerritorySnapshot(TEXT("Sicily"), Sicily);
				Test.TestEqual(TEXT("Slice authors Sicily as unowned; no invented initial owner"), Sicily.OwnerFaction, EFactionID::None);
				FCall OwnDestination(Test, Map, TEXT("ChangeTerritoryOwner"), 3); OwnDestination.SetName(TEXT("TerritoryID"), TEXT("Sicily"));
				OwnDestination.SetEnum(TEXT("NewOwner"), EFactionID::Rome); OwnDestination.Execute();
				Test.TestTrue(TEXT("Controlled own-destination fixture commits canonical ownership"), OwnDestination.ReadStruct<FTerritoryOwnershipChangeResult>().WasSuccessful());
				Test.TestEqual(TEXT("Own adjacent army movement succeeds"), Move(Attacker, TEXT("Sicily")), ECampaignArmyFailure::None);
				FCampaignArmy Moved; State->TryGetCampaignArmy(Attacker, Moved);
				Test.TestEqual(TEXT("Move keeps identity and places army on Sicily"), Moved.TerritoryID, FName(TEXT("Sicily")));
				UObject* Factions = ReadObject(Test, Mode, TEXT("GetFactionManager"));
				const auto SetWar = [&](EDiplomaticStatus Status)
				{
					FCall Change(Test, Factions, TEXT("SetDiplomaticStatus"), 4); Change.SetEnum(TEXT("Faction1"), EFactionID::Rome);
					Change.SetEnum(TEXT("Faction2"), EFactionID::Carthage); Change.SetEnum(TEXT("NewStatus"), Status); Change.Execute();
				};
				SetWar(EDiplomaticStatus::Neutral); Test.TestEqual(TEXT("Neutral parties cannot initiate battle"), Begin(), ECampaignEncounterFailure::NotAtWar); SetWar(EDiplomaticStatus::War);
				URomanUnitDefinition* UnitDefinition = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
				if (!Test.TestNotNull(TEXT("Deployment validates real unit catalog"), UnitDefinition)) return true;
				const auto SavedClass = UnitDefinition->UnitClass; UnitDefinition->UnitClass.Reset();
				Test.TestEqual(TEXT("Unavailable class rejects before any actors or binding commit"), Begin(), ECampaignEncounterFailure::InvalidContent);
				UnitDefinition->UnitClass = SavedClass;
				Test.TestFalse(TEXT("Failed deployment leaves no plan"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
				Test.TestFalse(TEXT("Failed deployment leaves no tactical result"), Battle->HasEncounter());
				// Fail after the first actual actor materializes, not merely before staging.
				bool bInvalidatedClass = false;
				const FDelegateHandle SpawnHook = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
				{
					if (!bInvalidatedClass && Actor && Actor->FindFunction(TEXT("GetUnitDefinitionID")))
					{ bInvalidatedClass = true; UnitDefinition->UnitClass.Reset(); }
				}));
				Test.AddExpectedError(TEXT("Cannot spawn unit definition Legionary"), EAutomationExpectedErrorFlags::Contains, 1);
				const ECampaignEncounterFailure Interrupted = Begin();
				World->RemoveOnActorSpawnedHandler(SpawnHook); UnitDefinition->UnitClass = SavedClass;
				Test.TestTrue(TEXT("Partial-spawn fixture exercised the real factory"), bInvalidatedClass);
				Test.TestEqual(TEXT("Partial materialization fails atomically"), Interrupted, ECampaignEncounterFailure::MutationFailed);
				Test.TestFalse(TEXT("Failed partial spawn publishes no pending plan"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
				Test.TestFalse(TEXT("Failed partial spawn starts no evaluator"), Battle->HasEncounter());
				URomanUnitRegistrySubsystem* Units = World->GetSubsystem<URomanUnitRegistrySubsystem>();
				Test.TestEqual(TEXT("Failed staging retires spawned actors from live registry"), Units->GetRegisteredUnitCount(), 0);
				Test.TestEqual(TEXT("Failed staging leaves no orphan squads"), Units->GetSquadCount(), 0);
				State->TryGetCampaignArmy(Attacker, Moved);
				Test.TestEqual(TEXT("Failed staging preserves strategic composition"), FCampaignArmyState::GetUnitCount(Moved), 15);
				Test.TestEqual(TEXT("Failed staging preserves strategic location"), Moved.TerritoryID, FName(TEXT("Sicily")));
				Probe.Reset(NewObject<UCampaignEncounterProbe>()); Probe->Test = &Test; Probe->State = State; Probe->Manager = Manager; Probe->Map = Map;
				Probe->ArmyID = Attacker; Probe->bVictory = bVictory; Probe->ExpectedSurvivors = bVictory ? 14 : 0;
				State->OnTacticalBattleChanged.AddUniqueDynamic(Probe.Get(), &UCampaignEncounterProbe::OnTacticalChanged);
				if (!Test.TestEqual(TEXT("Actually adjacent campaign encounter launches"), Begin(), ECampaignEncounterFailure::None)) return true;
				PlanID = State->GetCampaignEncounterPlan().EncounterID;
				Test.TestTrue(TEXT("First tactical observer already sees prepared strategic mapping"), Probe->bSawPreparedPlan);
				Test.TestEqual(TEXT("Subsystem and strategic plan share exact encounter identity"), Battle->GetResult().EncounterID, PlanID);
				Test.TestEqual(TEXT("Live deployment has two content-bound squads"), Battle->GetResult().Squads.Num(), 2);
				Test.TestEqual(TEXT("Deployment switches to tactical"), State->GetCurrentGameplayMode(), EGameplayMode::Tactical);
				Test.TestEqual(TEXT("In-progress result cannot commit"), Apply(), ECampaignEncounterFailure::InvalidResult);
				Test.TestEqual(TEXT("Pending encounter blocks strategic movement"), Move(Attacker, TEXT("Rome")), ECampaignArmyFailure::EncounterPending);
				FCall Turn(Test, Manager, TEXT("ProcessTurn"), 1); Turn.Execute();
				Test.TestEqual(TEXT("Pending encounter blocks pipeline before economy"), Turn.ReadStruct<FCampaignTurnResult>().Failure, ECampaignTurnFailure::EncounterPending);
				if (bReinforcements)
				{
					ExerciseReinforcements(World, State, Manager);
					Probe->ExpectedSurvivors = bVictory ? 15 : 0;
				}
				BeforeTurn = State->GetCurrentTurn(); BeforeWallet = State->GetFactionResources(EFactionID::Rome);
				State->OnCampaignArmiesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignEncounterProbe::OnArmiesChanged);
				State->OnTerritoryOwnerChanged.AddUniqueDynamic(Probe.Get(), &UCampaignEncounterProbe::OnTerritoryChanged);
				State->OnCampaignEncounterChanged.AddUniqueDynamic(Probe.Get(), &UCampaignEncounterProbe::OnEncounterChanged);
				bool bKilledOneRoman = false;
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (!It->FindFunction(TEXT("GetUnitDefinitionID"))) continue;
					FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute();
					if (Owner.ReadEnum<EFactionID>() == EFactionID::Rome)
					{
						if (!bVictory || !bKilledOneRoman) { FCall Kill(Test, *It, TEXT("SetCurrentHealth"), 1); Kill.SetInt(TEXT("NewHealth"), 0); Kill.Execute(); bKilledOneRoman = true; }
						else Occupant = *It;
					}
					else if (bVictory) { FCall Kill(Test, *It, TEXT("SetCurrentHealth"), 1); Kill.SetInt(TEXT("NewHealth"), 0); Kill.Execute(); }
				}
				if (bVictory)
				{
					Test.TestFalse(TEXT("Enemy kills alone still need physical occupation"), Battle->IsResolved());
					if (!Test.TestTrue(TEXT("Physical occupant survives"), Occupant.IsValid())) return true;
					Occupant->SetActorLocation(FVector(3000.0, 0.0, 120.0), false, nullptr, ETeleportType::TeleportPhysics);
				}
				Stage = 1; return false; // Wait for the real objective timer/capture or terminal death.
			}
			if (!Battle->IsResolved()) return false;
			FCampaignArmy Before; Test.TestTrue(TEXT("Tactical result has not yet decremented strategic inventory"), State->TryGetCampaignArmy(Attacker, Before));
			Test.TestEqual(TEXT("Strategic inventory waits for mapped casualties"), FCampaignArmyState::GetUnitCount(Before), bReinforcements ? 16 : 15);
			Test.TestEqual(TEXT("Canonical victory/defeat commits"), Apply(), ECampaignEncounterFailure::None);
			Test.TestTrue(TEXT("Every actor/public army/ownership/encounter observer sees complete state"), Probe->bAllReadsCommitted);
			Test.TestEqual(TEXT("One army invalidation"), Probe->ArmyEvents, 1);
			Test.TestEqual(TEXT("Capture publishes exactly one territory invalidation; defeat none"), Probe->TerritoryEvents, bVictory ? 1 : 0);
			Test.TestEqual(TEXT("One consumed encounter invalidation"), Probe->EncounterEvents, 1);
			Test.TestEqual(TEXT("Battle application does not advance turns"), State->GetCurrentTurn(), BeforeTurn);
			const FFactionResourceAmounts AfterWallet = State->GetFactionResources(EFactionID::Rome);
			Test.TestTrue(TEXT("No synthetic economy credit during application"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&BeforeWallet, &AfterWallet, 0));
			Test.TestEqual(TEXT("Replay cannot mutate live owners"), Apply(), ECampaignEncounterFailure::AlreadyApplied);
			Test.TestEqual(TEXT("Replay emits no second army event"), Probe->ArmyEvents, 1);
			Test.TestTrue(TEXT("Public consumed identity query"), State->HasAppliedCampaignEncounter(PlanID));
			State->OnCampaignArmiesChanged.RemoveDynamic(Probe.Get(), &UCampaignEncounterProbe::OnArmiesChanged);
			State->OnTerritoryOwnerChanged.RemoveDynamic(Probe.Get(), &UCampaignEncounterProbe::OnTerritoryChanged);
			State->OnCampaignEncounterChanged.RemoveDynamic(Probe.Get(), &UCampaignEncounterProbe::OnEncounterChanged);
			State->OnTacticalBattleChanged.RemoveDynamic(Probe.Get(), &UCampaignEncounterProbe::OnTacticalChanged);
			Test.TestEqual(TEXT("Exactly one public encounter relay"), ListenerCount(Test, Manager, TEXT("OnCampaignEncounterChanged"), State), 1);
			FCall Reset(Test, Manager, TEXT("StartNewCampaign"), 1); Reset.SetEnum(TEXT("PlayerFaction"), EFactionID::Rome); Reset.Execute();
			Test.TestFalse(TEXT("New campaign removes old encounter identity"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
			Test.TestFalse(TEXT("New campaign clears consumed identities"), State->HasAppliedCampaignEncounter(PlanID));
			Test.TestEqual(TEXT("Old world result cannot apply to new campaign"), Apply(), ECampaignEncounterFailure::NoEncounter);
			State->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			Test.TestEqual(TEXT("Read-model teardown detaches encounter relay"), ListenerCount(Test, Manager, TEXT("OnCampaignEncounterChanged"), State), 0);
			Test.TestFalse(TEXT("Ended read model has no stale plan"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
			CastChecked<AActor>(Manager)->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			FCall Ended(Test, Manager, TEXT("GetCampaignEncounterPlan"), 1); Ended.Execute();
			Test.TestFalse(TEXT("Ended owner releases coordinator"), Ended.ReadStruct<FCampaignEncounterPlan>().EncounterID.IsValid());
			return true;
		}
	private:
		FAutomationTestBase& Test;
		bool bVictory;
		bool bReinforcements;
		TSharedPtr<FStrategicBootstrapScope> Scope;
		double StartedAt;
		int32 Stage = 0, BeforeTurn = 0;
		FGuid Attacker, Defender, PlanID;
		FFactionResourceAmounts BeforeWallet;
		TWeakObjectPtr<AActor> Occupant;
		TStrongObjectPtr<UCampaignEncounterProbe> Probe;
		void ExerciseReinforcements(UWorld* World, ARomanEmpireGameState* State, UObject* Manager)
		{
			AActor* Producer = nullptr;
			for (TActorIterator<AActor> It(World); It; ++It) if (It->FindFunction(TEXT("TrainUnit"))) { Producer = *It; break; }
			if (!Test.TestNotNull(TEXT("Paid enrollment uses actual barracks"), Producer)) return;
			UObject* Mode = World->GetAuthGameMode();
			UObject* Factions = ReadObject(Test, Mode, TEXT("GetFactionManager"));
			URomanUnitRegistrySubsystem* Units = World->GetSubsystem<URomanUnitRegistrySubsystem>();
			URomanUnitDefinition* Asset = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
			const FFactionResourceAmounts WalletBefore = State->GetFactionResources(EFactionID::Rome);
			const FFactionResourceAmounts Paid = Asset->Definition.TrainingCost;
			const auto Train = [&]() { FCall Call(Test, Producer, TEXT("TrainUnit"), 2); Call.SetName(TEXT("UnitID"), TEXT("Legionary")); Call.Execute(); return Call.ReadBool(); };
			const auto Receipt = [&]() { FCall Call(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1); Call.Execute(); return Call.ReadStruct<FTrainingReservation>(); };
			FCall WrongMode(Test, Mode, TEXT("SetGameplayMode"), 1); WrongMode.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); WrongMode.Execute();
			Test.TestFalse(TEXT("Mapped training rejects wrong mode before charge"), Train());
			Test.TestEqual(TEXT("Wrong-mode rejection preserves wallet"), State->GetFactionResources(EFactionID::Rome).Gold, WalletBefore.Gold);
			FCall TacticalMode(Test, Mode, TEXT("SetGameplayMode"), 1); TacticalMode.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); TacticalMode.Execute();
			Test.TestTrue(TEXT("First mapped paid order accepted"), Train());
			const FTrainingReservation First = Receipt();
			Test.TestTrue(TEXT("Paid receipt has identity and frozen campaign destination"), First.ReceiptID.IsValid() && First.EncounterID == PlanID && First.CampaignArmyID == Attacker);
			Test.TestTrue(TEXT("Second mapped order prepays independently"), Train());
			const FFactionResourceAmounts Charged = State->GetFactionResources(EFactionID::Rome);
			Test.TestEqual(TEXT("Exactly two original gold costs charged"), Charged.Gold, WalletBefore.Gold - 2 * Paid.Gold);
			TStrongObjectPtr<UCampaignReinforcementProbe> Enrollment(NewObject<UCampaignReinforcementProbe>());
			Enrollment->Test = &Test; Enrollment->State = State; Enrollment->Manager = Manager; Enrollment->Producer = Producer;
			Enrollment->ArmyID = Attacker; Enrollment->EncounterID = PlanID;
			State->OnTacticalBattleChanged.AddUniqueDynamic(Enrollment.Get(), &UCampaignReinforcementProbe::OnTacticalChanged);
			State->OnCampaignArmiesChanged.AddUniqueDynamic(Enrollment.Get(), &UCampaignReinforcementProbe::OnArmiesChanged);
			State->OnCampaignEncounterChanged.AddUniqueDynamic(Enrollment.Get(), &UCampaignReinforcementProbe::OnEncounterChanged);
			Producer->Tick(Asset->Definition.TrainingTime + 0.1f);
			Test.TestTrue(TEXT("All enrollment observers read full tactical/strategic commit"), Enrollment->bAllReadsCommitted);
			Test.TestEqual(TEXT("One tactical invalidation on enrollment"), Enrollment->TacticalEvents, 1);
			Test.TestEqual(TEXT("One army invalidation on enrollment"), Enrollment->ArmyEvents, 1);
			Test.TestEqual(TEXT("One encounter invalidation on enrollment"), Enrollment->EncounterEvents, 1);
			const FTrainingReservation Second = Receipt();
			Test.TestTrue(TEXT("Prepaid promotion retains destination and distinct receipt"), Second.ReceiptID.IsValid() && Second.ReceiptID != First.ReceiptID && Second.EncounterID == PlanID && Second.CampaignArmyID == Attacker);
			Test.TestEqual(TEXT("Completed training does not recharge"), State->GetFactionResources(EFactionID::Rome).Gold, Charged.Gold);
			// Invalidate mode after an actor exists; fail enrollment, destroy staging and retain receipt if credit overflows.
			AddGold(Test, Factions, MAX_int32 - Charged.Gold);
			int32 SpawnAttempts = 0;
			const FDelegateHandle Hook = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
			{
				if (Actor && Actor->FindFunction(TEXT("GetUnitDefinitionID")))
				{ ++SpawnAttempts; FCall Change(Test, Mode, TEXT("SetGameplayMode"), 1); Change.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); Change.Execute(); }
			}));
			Producer->Tick(Asset->Definition.TrainingTime + 0.1f);
			Test.TestEqual(TEXT("Enrollment failure occurs after real deferred spawn"), SpawnAttempts, 1);
			Test.TestTrue(TEXT("Blocked refund retains original receipt"), ReadBool(Test, Producer, TEXT("IsAwaitingTrainingRefund")) && Receipt().ReceiptID == Second.ReceiptID);
			Test.TestEqual(TEXT("Failed enrollment removes staged unit"), Units->GetRegisteredUnitCount(), 26);
			Test.TestEqual(TEXT("Failed enrollment removes orphan singleton squad"), Units->GetSquadCount(), 3);
			Test.TestEqual(TEXT("Failed enrollment emits no extra roster publication"), Enrollment->TacticalEvents, 1);
			Test.TestEqual(TEXT("Failed enrollment emits no extra inventory publication"), Enrollment->ArmyEvents, 1);
			Test.TestEqual(TEXT("Failed enrollment emits no extra encounter publication"), Enrollment->EncounterEvents, 1);
			FCall RestoreMode(Test, Mode, TEXT("SetGameplayMode"), 1); RestoreMode.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); RestoreMode.Execute();
			AddGold(Test, Factions, Charged.Gold - MAX_int32);
			{
				TGuardValue<FFactionResourceAmounts> PriceGuard(Asset->Definition.TrainingCost, FFactionResourceAmounts());
				Producer->Tick(1000.0f); Producer->Tick(1000.0f);
			}
			World->RemoveOnActorSpawnedHandler(Hook);
			Test.TestEqual(TEXT("Refund retry never respawns paid order"), SpawnAttempts, 1);
			Test.TestFalse(TEXT("Successful original refund retires failed order"), ReadBool(Test, Producer, TEXT("IsTraining")));
			FFactionResourceAmounts Expected = WalletBefore;
			Expected.Gold -= Paid.Gold; Expected.Food -= Paid.Food; Expected.Iron -= Paid.Iron;
			Expected.Wood -= Paid.Wood; Expected.Stone -= Paid.Stone; Expected.Population -= Paid.Population;
			const FFactionResourceAmounts Final = State->GetFactionResources(EFactionID::Rome);
			Test.TestTrue(TEXT("Failed enrollment refunds all six captured costs despite catalog edit"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Expected, &Final, 0));
			const FFactionResourceAmounts EnemyBefore = State->GetFactionResources(EFactionID::Carthage);
			Test.TestTrue(TEXT("Another original-owner order accepted"), Train());
			FCall Captured(Test, Producer, TEXT("SetOwnerFaction"), 1); Captured.SetEnum(TEXT("NewOwner"), EFactionID::Carthage); Captured.Execute();
			Producer->Tick(Asset->Definition.TrainingTime + 0.1f);
			Test.TestFalse(TEXT("Ownership change cannot complete original-owner enrollment"), ReadBool(Test, Producer, TEXT("IsTraining")));
			const FFactionResourceAmounts OwnerRefund = State->GetFactionResources(EFactionID::Rome);
			const FFactionResourceAmounts EnemyAfter = State->GetFactionResources(EFactionID::Carthage);
			Test.TestTrue(TEXT("Ownership mismatch refunds original payer all captured costs"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Final, &OwnerRefund, 0));
			Test.TestTrue(TEXT("New producer owner receives no original payer credit"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&EnemyBefore, &EnemyAfter, 0));
			Test.TestEqual(TEXT("Captured order cannot mint a tactical unit"), Units->GetRegisteredUnitCount(), 26);
			FCall RestoreOwner(Test, Producer, TEXT("SetOwnerFaction"), 1); RestoreOwner.SetEnum(TEXT("NewOwner"), EFactionID::Rome); RestoreOwner.Execute();
			State->OnTacticalBattleChanged.RemoveDynamic(Enrollment.Get(), &UCampaignReinforcementProbe::OnTacticalChanged);
			State->OnCampaignArmiesChanged.RemoveDynamic(Enrollment.Get(), &UCampaignReinforcementProbe::OnArmiesChanged);
			State->OnCampaignEncounterChanged.RemoveDynamic(Enrollment.Get(), &UCampaignReinforcementProbe::OnEncounterChanged);
		}
	};

	class FExerciseCampaignArmyActions : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseCampaignArmyActions(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignArmyActions() { if (Definition.IsValid()) Definition->Definition.TrainingCost = OriginalCost; }
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 35.0) { Test.AddError(TEXT("Army actions session entry timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			auto* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			UObject* Mode = World->GetAuthGameMode();
			UObject* Manager = ReadObject(Test, Mode, TEXT("GetCampaignManager"));
			UObject* Factions = ReadObject(Test, Mode, TEXT("GetFactionManager"));
			if (bBattleQueued)
			{
				if (World == SourceWorld.Get() || Session->HasPendingTransfer()) return false;
				FCampaignArmy Army; Test.TestTrue(TEXT("Purchased and assigned persistent ID travels"), State->TryGetCampaignArmy(TravelArmyID, Army));
				Test.TestEqual(TEXT("All 23 strategic quantities survive real selected-map deployment"), FCampaignArmyState::GetUnitCount(Army), 23);
				Test.TestEqual(TEXT("Only tactical destination materializes purchased forces"), World->GetSubsystem<URomanUnitRegistrySubsystem>()->GetRegisteredUnitCount(), 33);
				const auto Wallet = State->GetFactionResources(EFactionID::Rome);
				Test.TestTrue(TEXT("Travel preserves exact six-cost debit"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &TravelWallet, 0));
				Test.TestEqual(TEXT("Mapped tactical session rejects strategic recruitment"), RecruitFailureInCurrentWorld(Manager), ECampaignArmyFailure::WrongMode);
				return true;
			}
			const auto Quote = [&](FName Region, FName Unit, int32 Count, FGuid Destination = FGuid())
			{
				FCall Call(Test, Manager, TEXT("GetCampaignRecruitmentQuote"), 5);
				Call.SetName(TEXT("TerritoryID"), Region); Call.SetName(TEXT("UnitID"), Unit); Call.SetInt(TEXT("Count"), Count); Call.SetGuid(TEXT("DestinationArmyID"), Destination); Call.Execute();
				return Call.ReadStruct<FCampaignRecruitmentQuote>();
			};
			const auto Recruit = [&](int32 Count, FGuid Destination = FGuid())
			{
				FCall Call(Test, Manager, TEXT("RecruitCampaignUnits"), 5);
				Call.SetName(TEXT("TerritoryID"), TEXT("Rome")); Call.SetName(TEXT("UnitID"), TEXT("Legionary")); Call.SetInt(TEXT("Count"), Count); Call.SetGuid(TEXT("DestinationArmyID"), Destination); Call.Execute();
				return Call.ReadStruct<FCampaignArmyMutationResult>();
			};
			const auto Assign = [&](FGuid From, FGuid To, int32 Count)
			{
				FCall Call(Test, Manager, TEXT("AssignCampaignArmyUnits"), 5);
				Call.SetGuid(TEXT("SourceArmyID"), From); Call.SetGuid(TEXT("DestinationArmyID"), To); Call.SetName(TEXT("UnitID"), TEXT("Legionary")); Call.SetInt(TEXT("Count"), Count); Call.Execute();
				return Call.ReadStruct<FCampaignArmyMutationResult>();
			};
			if (!bEntered)
			{
				Test.TestEqual(TEXT("Sandbox cannot purchase strategic quantities"), Quote(TEXT("Rome"), TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::Unavailable);
				SourceWorld = World;
				Test.TestEqual(TEXT("Actions fixture enters actual strategic world"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::None);
				Test.TestEqual(TEXT("Pending entry freezes purchases without debit"), Recruit(1).Failure, ECampaignArmyFailure::MutationInProgress);
				bEntered = true; return false;
			}
			if (World == SourceWorld.Get() || Session->HasPendingTransfer()) return false;
			FGuid Source, Enemy;
			for (const auto& Army : State->GetCampaignArmies()) { if (Army.Faction == EFactionID::Rome) Source = Army.ArmyID; else if (Army.Faction == EFactionID::Carthage) Enemy = Army.ArmyID; }
			if (!Test.TestTrue(TEXT("Fixture has both canonical armies"), Source.IsValid() && Enemy.IsValid())) return true;
			Definition = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
			if (!Test.TestTrue(TEXT("Loaded authored unit is available"), Definition.IsValid())) return true;
			OriginalCost = Definition->Definition.TrainingCost;
			FFactionResourceAmounts Six; Six.Gold = 1; Six.Food = 2; Six.Iron = 3; Six.Wood = 4; Six.Stone = 5; Six.Population = 6;
			Definition->Definition.TrainingCost = Six;
			const auto InitialWallet = State->GetFactionResources(EFactionID::Rome);
			const auto InitialArmies = State->GetCampaignArmies();
			const auto Unchanged = [&]()
			{
				const auto Wallet = State->GetFactionResources(EFactionID::Rome); const auto Armies = State->GetCampaignArmies();
				Test.TestTrue(TEXT("Rejected action preserves all six wallet fields"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &InitialWallet, 0));
				Test.TestEqual(TEXT("Rejected action preserves roster size"), Armies.Num(), InitialArmies.Num());
				for (int32 I = 0; I < FMath::Min(Armies.Num(), InitialArmies.Num()); ++I)
					Test.TestTrue(TEXT("Rejected action preserves complete army values"), FCampaignArmy::StaticStruct()->CompareScriptStruct(&Armies[I], &InitialArmies[I], 0));
			};
			Test.TestEqual(TEXT("Zero recruit rejects"), Recruit(0, Source).Failure, ECampaignArmyFailure::InvalidUnitCount);
			Test.TestEqual(TEXT("Negative recruit rejects"), Recruit(-1, Source).Failure, ECampaignArmyFailure::InvalidUnitCount);
			Test.TestEqual(TEXT("Unknown content quote rejects"), Quote(TEXT("Rome"), TEXT("MissingUnit"), 1).Failure, ECampaignArmyFailure::InvalidContent);
			Test.TestEqual(TEXT("Unknown region rejects"), Quote(TEXT("MissingRegion"), TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::InvalidTerritoryID);
			Test.TestEqual(TEXT("Hostile settlement cannot recruit"), Quote(TEXT("Africa"), TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::NotOwned);
			Test.TestEqual(TEXT("Unknown destination cannot debit"), Recruit(1, FGuid(9, 0, 0, 1)).Failure, ECampaignArmyFailure::UnknownArmy);
			Test.TestEqual(TEXT("Cannot recruit into hostile army"), Recruit(1, Enemy).Failure, ECampaignArmyFailure::NotOwned);
			Test.TestEqual(TEXT("Self assignment rejects"), Assign(Source, Source, 1).Failure, ECampaignArmyFailure::SameArmy);
			Test.TestEqual(TEXT("Cannot transfer to hostile army"), Assign(Source, Enemy, 1).Failure, ECampaignArmyFailure::NotOwned);
			Definition->Definition.TrainingCost.Gold = MAX_int32;
			Test.TestEqual(TEXT("Six-cost scaling overflow rejects"), Recruit(2).Failure, ECampaignArmyFailure::CostOverflow);
			Test.TestEqual(TEXT("Unaffordable purchase rejects without debit"), Recruit(1).Failure, ECampaignArmyFailure::InsufficientResources);
			Definition->Definition.TrainingCost.Gold = -1;
			Test.TestEqual(TEXT("Invalid authored cost fails closed"), Recruit(1).Failure, ECampaignArmyFailure::InvalidContent);
			Definition->Definition.TrainingCost = Six;
			Test.TestTrue(TEXT("Fresh-army quote available without reserving an ID"), Quote(TEXT("Rome"), TEXT("Legionary"), 2).IsAvailable());
			const auto Factory = Definition->UnitClass; Definition->UnitClass.Reset();
			Test.TestEqual(TEXT("Missing factory quote rejects without loading"), Recruit(1).Failure, ECampaignArmyFailure::InvalidContent);
			Definition->UnitClass = Factory;
			const auto Availability = Definition->AvailableToFactions; Definition->AvailableToFactions.Reset();
			Test.TestEqual(TEXT("Unavailable faction content rejects"), Recruit(1).Failure, ECampaignArmyFailure::InvalidContent);
			Definition->AvailableToFactions = Availability;
			UObject* Map = ReadObject(Test, Mode, TEXT("GetWorldMapManager"));
			FCall Owner(Test, Map, TEXT("ChangeTerritoryOwner"), 3); Owner.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Owner.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Owner.Execute();
			Test.TestEqual(TEXT("Owned region without settlement cannot recruit"), Quote(TEXT("Sicily"), TEXT("Legionary"), 1).Failure, ECampaignArmyFailure::SettlementRequired);
			Unchanged();
			FCall Tactical(Test, Mode, TEXT("SetGameplayMode"), 1); Tactical.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); Tactical.Execute();
			Test.TestEqual(TEXT("Session tactical zoom cannot recruit strategic inventory"), Recruit(1).Failure, ECampaignArmyFailure::WrongMode);
			Test.TestEqual(TEXT("Session tactical zoom cannot assign strategic inventory"), Assign(Source, Source, 1).Failure, ECampaignArmyFailure::WrongMode);
			FCall Strategic(Test, Mode, TEXT("SetGameplayMode"), 1); Strategic.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); Strategic.Execute();
			Unchanged();
			TStrongObjectPtr<UCampaignArmyActionsProbe> Probe(NewObject<UCampaignArmyActionsProbe>());
			Probe->Test = &Test; Probe->State = State; Probe->Manager = Manager; Probe->Factions = Factions; Probe->SourceArmyID = Source;
			Probe->ExpectedWallet = InitialWallet; Probe->ExpectedTotal = 15; Probe->ExpectedArmies = 1;
			State->OnCampaignArmiesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnArmiesChanged);
			State->OnFactionResourcesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnResourcesChanged);
			const auto ExpectRecruit = [&](int32 Count)
			{
				Test.TestTrue(TEXT("Fixture expected wallet debit valid"), RomanEmpireEconomy::TrySpendResources(Probe->ExpectedWallet, Quote(TEXT("Rome"), TEXT("Legionary"), Count).Cost).WasSuccessful());
				Probe->ExpectedTotal += Count;
			};
			ExpectRecruit(2); Probe->ExpectedArmies = 2;
			const auto NewArmy = Recruit(2);
			Test.TestTrue(TEXT("Paid fresh army commits with stable ID"), NewArmy.WasSuccessful() && NewArmy.ArmyID.IsValid() && NewArmy.ArmyID != Source && NewArmy.ArmyID != Enemy);
			ExpectRecruit(3);
			Test.TestTrue(TEXT("Paid recruit into chosen existing army"), Recruit(3, Source).WasSuccessful());
			State->OnCampaignArmiesChanged.RemoveDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnArmiesChanged);
			State->OnFactionResourcesChanged.RemoveDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnResourcesChanged);
			const auto Move = [&](FName Region)
			{
				FCall Call(Test, Manager, TEXT("MoveCampaignArmy"), 3); Call.SetGuid(TEXT("ArmyID"), NewArmy.ArmyID); Call.SetName(TEXT("TargetTerritoryID"), Region); Call.Execute();
				return Call.ReadStruct<FCampaignArmyMutationResult>().WasSuccessful();
			};
			Test.TestTrue(TEXT("Fixture relocates second army to adjacent owned region"), Move(TEXT("Sicily")));
			Test.TestEqual(TEXT("Runtime assignment forbids remote transfer"), Assign(Source, NewArmy.ArmyID, 1).Failure, ECampaignArmyFailure::NotCoLocated);
			Test.TestEqual(TEXT("Recruitment cannot silently relocate chosen army"), Recruit(1, NewArmy.ArmyID).Failure, ECampaignArmyFailure::NotCoLocated);
			Test.TestTrue(TEXT("Fixture returns second army"), Move(TEXT("Rome")));
			Test.TestEqual(TEXT("Runtime over-assignment does not debit or discard"), Assign(Source, NewArmy.ArmyID, 19).Failure, ECampaignArmyFailure::InsufficientUnits);
			State->OnCampaignArmiesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnArmiesChanged);
			State->OnFactionResourcesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnResourcesChanged);
			Test.TestTrue(TEXT("Co-located assignment conserves all 20 units"), Assign(Source, NewArmy.ArmyID, 5).WasSuccessful());
			Probe->ExpectedArmies = 1;
			Test.TestTrue(TEXT("Full assignment retires emptied source"), Assign(NewArmy.ArmyID, Source, 7).WasSuccessful());
			FCampaignArmy Retired; Test.TestFalse(TEXT("Retired identity does not become an empty save row"), State->TryGetCampaignArmy(NewArmy.ArmyID, Retired));
			Definition->Definition.TrainingCost = FFactionResourceAmounts(); Probe->ExpectedTotal += 3; Probe->ExpectedArmies = 2;
			const auto FreeArmy = Recruit(3);
			Test.TestTrue(TEXT("Explicit zero-cost definition still commits inventory"), FreeArmy.WasSuccessful());
			Probe->ExpectedArmies = 1;
			Test.TestTrue(TEXT("Free inventory assigns without charging again"), Assign(FreeArmy.ArmyID, Source, 3).WasSuccessful());
			Test.TestTrue(TEXT("Every resource/army observer sees both domains committed"), Probe->bAllCommitted);
			Test.TestEqual(TEXT("Exactly one army event per accepted action"), Probe->ArmyEvents, 6);
			Test.TestEqual(TEXT("Only the two nonzero purchases publish resource events"), Probe->ResourceEvents, 2);
			State->OnCampaignArmiesChanged.RemoveDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnArmiesChanged);
			State->OnFactionResourcesChanged.RemoveDynamic(Probe.Get(), &UCampaignArmyActionsProbe::OnResourcesChanged);
			Test.TestEqual(TEXT("Strategic purchases spawn no tactical actors"), World->GetSubsystem<URomanUnitRegistrySubsystem>()->GetRegisteredUnitCount(), 0);
			const auto WalletAfter = State->GetFactionResources(EFactionID::Rome);
			Test.TestTrue(TEXT("All six exact authored costs persist"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&WalletAfter, &Probe->ExpectedWallet, 0));
			Definition->Definition.TrainingCost = OriginalCost;
			TravelWallet = WalletAfter; TravelArmyID = Source; SourceWorld = World;
			Test.TestEqual(TEXT("Purchased forces enter actual selected tactical map"), Session->BeginTacticalEncounter(Source, TEXT("Africa")), ECampaignSessionFailure::None);
			bBattleQueued = true; return false;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		bool bEntered = false, bBattleQueued = false;
		FGuid TravelArmyID;
		FFactionResourceAmounts TravelWallet;
		TWeakObjectPtr<UWorld> SourceWorld;
		TWeakObjectPtr<URomanUnitDefinition> Definition;
		FFactionResourceAmounts OriginalCost;
		ECampaignArmyFailure RecruitFailureInCurrentWorld(UObject* Manager)
		{
			FCall Call(Test, Manager, TEXT("RecruitCampaignUnits"), 5); Call.SetName(TEXT("TerritoryID"), TEXT("Rome")); Call.SetName(TEXT("UnitID"), TEXT("Legionary")); Call.SetInt(TEXT("Count"), 1); Call.Execute();
			return Call.ReadStruct<FCampaignArmyMutationResult>().Failure;
		}
	};

	class FExerciseCampaignAIQuery : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseCampaignAIQuery(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignAIQuery() { if (Definition.IsValid()) Definition->Definition.TrainingCost = OriginalCost; }
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 35.0) { Test.AddError(TEXT("Canonical AI query journey timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			auto* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			UObject* Mode = World->GetAuthGameMode();
			UObject* Campaign = ReadObject(Test, Mode, TEXT("GetCampaignManager"));
			const auto Query = [&]() { FCall Call(Test, Campaign, TEXT("GetCampaignAIProposal"), 1); Call.Execute(); return Call.ReadStruct<FCampaignAIProposal>(); };
			if (Stage == 0)
			{
				Test.TestEqual(TEXT("Sandbox query cannot propose campaign actions"), Query().Action, ECampaignAIAction::None);
				PreviousWorld = World;
				Test.TestEqual(TEXT("Enter real strategic session"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::None);
				++Stage; return false;
			}
			if (Stage == 1)
			{
				if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
				Definition = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
				if (!Test.TestTrue(TEXT("Canonical current definition exists"), Definition.IsValid())) return true;
				OriginalCost = Definition->Definition.TrainingCost; Definition->Definition.TrainingCost = {};
				const auto Before = State->GetCampaignArmies(); const auto Wallet = State->GetFactionResources(EFactionID::Carthage);
				const int32 Turn = State->GetCurrentTurn();
				auto Proposal = Query(); const auto Again = Query();
				Test.TestEqual(TEXT("Current threatened Carthage proposes bounded recruitment"), Proposal.Action, ECampaignAIAction::Recruit);
				Test.TestEqual(TEXT("Current content ID comes from actual catalog"), Proposal.UnitID, FName(TEXT("Legionary")));
				Test.TestEqual(TEXT("Current garrison request is bounded"), Proposal.Count, 2);
				Test.TestEqual(TEXT("Current settlement, not actor location"), Proposal.TerritoryID, FName(TEXT("Africa")));
				Test.TestTrue(TEXT("Repeated world query is deterministic"), FCampaignAIProposal::StaticStruct()->CompareScriptStruct(&Proposal, &Again, 0));
				FCampaignArmyState BeforeState, AfterState; BeforeState.ReplaceFromSnapshot(Before); AfterState.ReplaceFromSnapshot(State->GetCampaignArmies());
				Test.TestTrue(TEXT("Query cannot recruit or alter armies"), FCampaignArmyState::StaticStruct()->CompareScriptStruct(&BeforeState, &AfterState, 0));
				const auto AfterWallet = State->GetFactionResources(EFactionID::Carthage);
				Test.TestTrue(TEXT("Query cannot debit any wallet field"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &AfterWallet, 0));
				Test.TestEqual(TEXT("Query cannot advance turn"), State->GetCurrentTurn(), Turn);
				UObject* Factions = ReadObject(Test, Mode, TEXT("GetFactionManager"));
				FFactionResourceDelta Credit; Credit.Gold = Credit.Food = Credit.Iron = Credit.Wood = Credit.Stone = Credit.Population = 12;
				FCall Add(Test, Factions, TEXT("ApplyFactionResourceDelta"), 3); Add.SetEnum(TEXT("FactionID"), EFactionID::Carthage); Add.SetStruct(TEXT("Delta"), Credit); Add.Execute();
				if (!Test.TestTrue(TEXT("Controlled wallet fixture credits canonical authority"), Add.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful())) return true;
				FFactionResourceAmounts Six; Six.Gold = 1; Six.Food = 2; Six.Iron = 3; Six.Wood = 4; Six.Stone = 5; Six.Population = 6;
				Definition->Definition.TrainingCost = Six; Proposal = Query();
				Test.TestEqual(TEXT("Current authored prices are reread, not cached"), Proposal.Action, ECampaignAIAction::Recruit);
				Test.TestEqual(TEXT("World quote population exact"), Proposal.Cost.Population, 12);
				Test.TestEqual(TEXT("World quote stone exact"), Proposal.Cost.Stone, 10);
				FFactionResourceAmounts Drain; Drain.Population = State->GetFactionResources(EFactionID::Carthage).Population;
				FCall Spend(Test, Factions, TEXT("TrySpendFactionResources"), 3); Spend.SetEnum(TEXT("FactionID"), EFactionID::Carthage); Spend.SetStruct(TEXT("Cost"), Drain); Spend.Execute();
				Test.TestTrue(TEXT("Canonical affordability fixture debit accepted"), Spend.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful());
				Test.TestEqual(TEXT("Unpayable current wallet cannot propose purchase"), Query().Action, ECampaignAIAction::None);
				FFactionResourceDelta Restore; Restore.Population = Drain.Population;
				FCall Refund(Test, Factions, TEXT("ApplyFactionResourceDelta"), 3); Refund.SetEnum(TEXT("FactionID"), EFactionID::Carthage); Refund.SetStruct(TEXT("Delta"), Restore); Refund.Execute();
				Test.TestTrue(TEXT("Controlled population debit restored"), Refund.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful());
				FFactionResourceAmounts RemoveCredit; RemoveCredit.Gold = RemoveCredit.Food = RemoveCredit.Iron = RemoveCredit.Wood = RemoveCredit.Stone = RemoveCredit.Population = 12;
				FCall Remove(Test, Factions, TEXT("TrySpendFactionResources"), 3); Remove.SetEnum(TEXT("FactionID"), EFactionID::Carthage); Remove.SetStruct(TEXT("Cost"), RemoveCredit); Remove.Execute();
				Test.TestTrue(TEXT("All temporary wallet credit restored"), Remove.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful());
				Definition->Definition.TrainingCost = OriginalCost;
				FCall Tactical(Test, Mode, TEXT("SetGameplayMode"), 1); Tactical.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); Tactical.Execute();
				Test.TestEqual(TEXT("Wrong mode cannot propose strategic AI"), Query().Action, ECampaignAIAction::None);
				FCall Strategic(Test, Mode, TEXT("SetGameplayMode"), 1); Strategic.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); Strategic.Execute();
				FGuid PlayerArmy; for (const auto& Army : State->GetCampaignArmies()) if (Army.Faction == EFactionID::Rome) PlayerArmy = Army.ArmyID;
				PreviousWorld = World;
				Test.TestEqual(TEXT("Actual selected-map tactical travel accepted"), Session->BeginTacticalEncounter(PlayerArmy, TEXT("Africa")), ECampaignSessionFailure::None);
				Test.TestEqual(TEXT("Pending transfer cannot propose strategic AI"), Query().Action, ECampaignAIAction::None);
				++Stage; return false;
			}
			if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
			Test.TestEqual(TEXT("Actual tactical destination cannot propose campaign AI"), Query().Action, ECampaignAIAction::None);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		int32 Stage = 0;
		TWeakObjectPtr<UWorld> PreviousWorld;
		TWeakObjectPtr<URomanUnitDefinition> Definition;
		FFactionResourceAmounts OriginalCost;
	};

	class FExerciseCampaignAIExecution : public IAutomationLatentCommand
	{
	public:
		FExerciseCampaignAIExecution(FAutomationTestBase* InTest, FString InCase) : Test(*InTest), Case(MoveTemp(InCase)), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignAIExecution() { if (Definition.IsValid()) Definition->Definition.TrainingCost = OriginalCost; if (Target.IsValid()) Target->TacticalMap = OriginalMap; }
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 85.0) { Test.AddError(TEXT("AI defense journey timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			auto* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			UObject* Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetCampaignManager"));
			auto* Root = Cast<UUserWidget>(ReadObject(Test, World->GetFirstPlayerController(), TEXT("GetMainWidget")));
			if (!Test.TestNotNull(TEXT("Defense uses real HUD"), Root)) return true;
			const auto Click = [&](const TCHAR* Name) { return ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(Name))); };
			const auto Turn = [&]() { FCall Call(Test, Manager, TEXT("ProcessTurn"), 1); Call.Execute(); return Call.ReadStruct<FCampaignTurnResult>(); };
			const auto CheckWallets = [&]() { for (const auto& Pair : Wallets) { const auto Actual = State->GetFactionResources(Pair.Key);
				Test.TestTrue(TEXT("All six balances exact"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Actual, &Pair.Value, 0)); } };
			if (Stage == 0) { PreviousWorld = World; Test.TestEqual(TEXT("Enter real campaign"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::None); Stage = 1; return false; }
			if (Stage == 1)
			{
				if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
				Definition = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
				Target = const_cast<URomanTerritoryDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindTerritory(TEXT("Rome")));
				if (!Test.TestTrue(TEXT("Current content available"), Definition.IsValid() && Target.IsValid())) return true;
				OriginalCost = Definition->Definition.TrainingCost; OriginalMap = Target->TacticalMap;
				Price.Gold = 1; Price.Food = 2; Price.Iron = 3; Price.Wood = 4; Price.Stone = 5; Price.Population = 6; Definition->Definition.TrainingCost = Price;
				FFactionResourceDelta Credit; Credit.Gold = Credit.Food = Credit.Iron = Credit.Wood = Credit.Stone = Credit.Population = 100;
				FCall Add(Test, ReadObject(Test, World->GetAuthGameMode(), TEXT("GetFactionManager")), TEXT("ApplyFactionResourceDelta"), 3);
				Add.SetEnum(TEXT("FactionID"), EFactionID::Carthage); Add.SetStruct(TEXT("Delta"), Credit); Add.Execute();
				if (!Test.TestTrue(TEXT("Controlled wallet funded"), Add.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful())) return true;
				for (const auto& Army : State->GetCampaignArmies()) (Army.Faction == EFactionID::Rome ? RomanArmy : AIArmy) = Army.ArmyID;
				if (Case == TEXT("Relief"))
				{
					UObject* Map = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetWorldMapManager"));
					FCall Change(Test, Map, TEXT("ChangeTerritoryOwner"), 3); Change.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Change.SetEnum(TEXT("NewOwner"), EFactionID::Carthage); Change.Execute();
					if (!Test.TestTrue(TEXT("Relief fixture owns adjacent source"), Change.ReadStruct<FTerritoryOwnershipChangeResult>().WasSuccessful())) return true;
					FStructProperty* Property = FindFProperty<FStructProperty>(Manager->GetClass(), TEXT("ArmyState"));
					if (!Test.TestTrue(TEXT("Test-only canonical roster fixture has expected type"), Property && Property->Struct == FCampaignArmyState::StaticStruct())) return true;
					FCampaignArmy Relief; Relief.ArmyID = FGuid(30, 0, 0, 1); Relief.Faction = EFactionID::Carthage; Relief.TerritoryID = TEXT("Sicily");
					FCampaignArmyUnitStack Stack; Stack.UnitID = TEXT("Legionary"); Stack.Count = 5; Relief.Units.Add(Stack);
					Test.TestTrue(TEXT("Controlled relief fixture inserted into existing owner"), Property->ContainerPtrToValuePtr<FCampaignArmyState>(Manager)->RegisterArmy(Relief).WasSuccessful());
					FCampaignArmy Retained = Relief; Retained.ArmyID = FGuid(31, 0, 0, 1); Retained.Units[0].Count = 15;
					Test.TestTrue(TEXT("Relief source retains enough against its own Roman threat"), Property->ContainerPtrToValuePtr<FCampaignArmyState>(Manager)->RegisterArmy(Retained).WasSuccessful());
				}
				for (EFactionID Faction : { EFactionID::Rome, EFactionID::Carthage }) Wallets.Add(Faction, State->GetFactionResources(Faction));
				Probe = TStrongObjectPtr<UCampaignAIExecutionProbe>(NewObject<UCampaignAIExecutionProbe>());
				Probe->Test = &Test; Probe->State = State; Probe->Manager = Manager; Probe->Session = Session; Probe->AIArmy = AIArmy; Probe->Target = Target; Probe->bBreakMap = Case == TEXT("RetryVictory");
				State->OnTurnStepStarted.AddDynamic(Probe.Get(), &UCampaignAIExecutionProbe::OnStep);
				State->OnFactionResourcesChanged.AddDynamic(Probe.Get(), &UCampaignAIExecutionProbe::OnResources);
				State->OnCampaignEncounterChanged.AddDynamic(Probe.Get(), &UCampaignAIExecutionProbe::OnEncounter);
				Session->OnSessionChanged.AddDynamic(Probe.Get(), &UCampaignAIExecutionProbe::OnSession);
				Stage = 2; return false;
			}
			if (Stage == 2)
			{
				for (const auto& Row : State->GetTerritorySnapshots()) if (auto* Wallet = Wallets.Find(Row.OwnerFaction))
				{ const auto& D = Row.EffectiveResourceProduction; Wallet->Gold += D.Gold; Wallet->Food += D.Food; Wallet->Iron += D.Iron; Wallet->Wood += D.Wood; Wallet->Stone += D.Stone; Wallet->Population += D.Population; }
				if (Case == TEXT("Relief"))
				{
					const auto Result = Turn(); Test.TestTrue(TEXT("Relief canonical turn succeeds"), Result.WasSuccessful());
					Test.TestEqual(TEXT("Relief executes one whole pipeline"), Result.CompletedSteps.Num(), 9); CheckWallets();
					FCampaignArmy Army; Test.TestTrue(TEXT("Relief identity retained"), State->TryGetCampaignArmy(FGuid(30, 0, 0, 1), Army));
					Test.TestEqual(TEXT("Relief relocates to threatened owned Africa"), Army.TerritoryID, FName(TEXT("Africa")));
					Test.TestEqual(TEXT("Relief never mints troops"), FCampaignArmyState::GetUnitCount(Army), 5);
					State->TryGetCampaignArmy(AIArmy, Army); Test.TestEqual(TEXT("Existing AI garrison preserved"), FCampaignArmyState::GetUnitCount(Army), 10);
					Test.TestTrue(TEXT("Relief source army identity retained"), State->TryGetCampaignArmy(FGuid(31, 0, 0, 1), Army));
					Test.TestEqual(TEXT("Relief cannot abandon threatened source"), Army.TerritoryID, FName(TEXT("Sicily"))); Test.TestEqual(TEXT("Source defense preserved"), FCampaignArmyState::GetUnitCount(Army), 15);
					Test.TestEqual(TEXT("Relief charges no recruitment"), Probe->RecruitmentEvents, 0); Test.TestFalse(TEXT("Relief queues no battle"), Session->HasPendingTransfer());
					Test.TestEqual(TEXT("Relief spawns no tactical actors"), World->GetSubsystem<URomanUnitRegistrySubsystem>()->GetRegisteredUnitCount(), 0); return true;
				}
				const int32 Recruits = Turns < 2 ? 2 : Turns == 2 ? 1 : 0;
				if (Recruits > 0) { auto& W = Wallets.FindChecked(EFactionID::Carthage); W.Gold -= Recruits * Price.Gold; W.Food -= Recruits * Price.Food; W.Iron -= Recruits * Price.Iron; W.Wood -= Recruits * Price.Wood; W.Stone -= Recruits * Price.Stone; W.Population -= Recruits * Price.Population; }
				Probe->ExpectedWallet = Wallets.FindChecked(EFactionID::Carthage); Probe->ExpectedCount = 10 + FMath::Min((Turns + 1) * 2, 5);
				const auto Result = Turn(); ++Turns;
				Test.TestEqual(TEXT("All nine stages complete"), Result.CompletedSteps.Num(), 9); Test.TestEqual(TEXT("Turn advances once"), State->GetCurrentTurn(), Turns + 1); CheckWallets();
				FCampaignArmy Army; Test.TestTrue(TEXT("AI identity preserved"), State->TryGetCampaignArmy(AIArmy, Army)); Test.TestEqual(TEXT("Bounded recruitment exact"), FCampaignArmyState::GetUnitCount(Army), Probe->ExpectedCount);
				Test.TestEqual(TEXT("No strategic tactical actors"), World->GetSubsystem<URomanUnitRegistrySubsystem>()->GetRegisteredUnitCount(), 0);
				if (Turns < 4) { Test.TestTrue(TEXT("Recruit turn succeeds"), Result.WasSuccessful()); return false; }
				Encounter = State->GetCampaignEncounterPlan().EncounterID; Test.TestTrue(TEXT("Accepted identity retained"), Encounter.IsValid() && Encounter == Probe->AcceptedEncounter);
				Test.TestEqual(TEXT("Three atomic purchase events"), Probe->RecruitmentEvents, 3);
				if (Case == TEXT("RetryVictory")) { Test.TestEqual(TEXT("Post-turn dispatch failure"), Result.Failure, ECampaignTurnFailure::AITacticalTravelFailed); Test.TestFalse(TEXT("Broken map queues no travel"), Session->HasPendingTransfer()); Stage = 3; return false; }
				Test.TestTrue(TEXT("Fourth turn queues real attack"), Result.WasSuccessful() && Session->HasPendingTransfer()); Definition->Definition.TrainingCost = OriginalCost; PreviousWorld = World; Stage = 4; return false;
			}
			if (Stage == 3)
			{
				auto* Button = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("EndTurnButton")));
				Test.TestTrue(TEXT("Retry enabled"), Button->GetIsEnabled()); Test.TestEqual(TEXT("Retry label"), CastChecked<UTextBlock>(Button->GetChildAt(0))->GetText().ToString(), FString(TEXT("RETRY [T]"))); Capture(Root, TEXT("Retry"));
				const auto Retry = Turn(); Test.TestTrue(TEXT("Explicit retry"), Retry.bWasAITacticalTravelRetry); Test.TestEqual(TEXT("Retry repeats no stages"), Retry.CompletedSteps.Num(), 0);
				Test.TestEqual(TEXT("Broken retry fails"), Retry.Failure, ECampaignTurnFailure::AITacticalTravelFailed); Test.TestEqual(TEXT("Retry preserves turn"), State->GetCurrentTurn(), 5); Test.TestEqual(TEXT("Retry preserves identity"), State->GetCampaignEncounterPlan().EncounterID, Encounter); CheckWallets();
				Target->TacticalMap = OriginalMap; Definition->Definition.TrainingCost = OriginalCost; PreviousWorld = World; if (!Click(TEXT("EndTurnButton"))) return true;
				Test.TestTrue(TEXT("Pointer retry schedules repaired map"), Session->HasPendingTransfer()); Test.TestEqual(TEXT("Repaired retry preserves turn"), State->GetCurrentTurn(), 5); CheckWallets(); Stage = 4; return false;
			}
			auto* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (Stage == 4)
			{
				if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
				Test.TestEqual(TEXT("Incoming turn restored"), State->GetCurrentTurn(), 5); Test.TestEqual(TEXT("Actual encounter identity"), Battle->GetResult().EncounterID, Encounter);
				Session->OnSessionChanged.RemoveDynamic(Probe.Get(), &UCampaignAIExecutionProbe::OnSession);
				Test.TestEqual(TEXT("AI attacks"), Battle->GetAttackingFaction(), EFactionID::Carthage); Test.TestEqual(TEXT("Human defends"), Battle->GetDefendingFaction(), EFactionID::Rome);
				APawn* Camera = World->GetFirstPlayerController()->GetPawn();
				if (Test.TestNotNull(TEXT("Human defense camera exists"), Camera))
					Test.TestTrue(TEXT("Camera starts at defended objective"), FVector::DistSquared2D(Camera->GetActorLocation(), Battle->GetObjectiveActor()->GetActorLocation()) < 1.0f);
				for (TActorIterator<AActor> It(World); It; ++It) if (It->FindFunction(TEXT("CommandHold")))
				{ FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute(); if (Owner.ReadEnum<EFactionID>() == EFactionID::Rome) { Romans.Add(*It); FCall(Test, *It, TEXT("CommandHold"), 0).Execute(); } else { Enemies.Add(*It); InitialLocations.Add(It->GetActorLocation()); } }
				if (!Test.TestEqual(TEXT("15 defenders materialized"), Romans.Num(), 15) || !Test.TestEqual(TEXT("15 purchased attackers materialized"), Enemies.Num(), 15)) return true;
				CheckWallets(); Capture(Root, TEXT("Defense")); Stage = 5; return false;
			}
			if (Stage == 5)
			{
				for (auto Roman : Romans) if (Roman.IsValid()) FCall(Test, Roman.Get(), TEXT("CommandHold"), 0).Execute();
				bool bMoved = false, bAttacking = false;
				for (int32 Index = 0; Index < Enemies.Num(); ++Index) if (Enemies[Index].IsValid()) { bMoved |= FVector::DistSquared(Enemies[Index]->GetActorLocation(), InitialLocations[Index]) > 10000.0f;
					FCall Order(Test, Enemies[Index].Get(), TEXT("GetOrderType"), 1); Order.Execute(); bAttacking |= Order.ReadEnum<EUnitOrderType>() == EUnitOrderType::Attack; }
				if (!bMoved || !bAttacking) return false;
				Test.TestTrue(TEXT("Actual AI command and NavMesh movement observed"), bMoved && bAttacking);
				if (Case == TEXT("Defeat") || Case == TEXT("Draw")) { for (auto Roman : Romans) Kill(Roman.Get()); if (Case == TEXT("Draw")) for (auto Enemy : Enemies) Kill(Enemy.Get()); else { Kill(Enemies[0].Get()); Kill(Enemies[1].Get()); } }
				else { Kill(Romans[0].Get()); Kill(Romans[1].Get()); for (auto Enemy : Enemies) Kill(Enemy.Get()); }
				Stage = 6; return false;
			}
			if (Stage == 6)
			{
				if (!Battle->IsResolved()) return false;
				if (TerminalAt == 0.0) { TerminalAt = FPlatformTime::Seconds(); return false; }
				if (FPlatformTime::Seconds() - TerminalAt < 0.75) return false;
				for (auto Unit : Case == TEXT("Defeat") ? Enemies : Romans) if (Unit.IsValid() && ReadBool(Test, Unit.Get(), TEXT("IsAlive")))
				{ FCall Order(Test, Unit.Get(), TEXT("GetOrderType"), 1); Order.Execute(); Test.TestEqual(TEXT("AI pulse cannot restore terminal orders"), Order.ReadEnum<EUnitOrderType>(), EUnitOrderType::None); Test.TestFalse(TEXT("Terminal unit Tick stays disabled"), Unit->IsActorTickEnabled()); }
				Test.TestEqual(TEXT("Human-relative outcome"), Battle->GetResult().Outcome, Case == TEXT("Defeat") ? ETacticalBattleOutcome::PlayerDefeat : Case == TEXT("Draw") ? ETacticalBattleOutcome::Draw : ETacticalBattleOutcome::PlayerVictory);
				Test.TestEqual(TEXT("Only physical AI occupation captures"), !Battle->GetResult().CapturedObjectiveID.IsNone(), Case == TEXT("Defeat"));
				const auto* Outcome = CastChecked<UTextBlock>(Root->GetWidgetFromName(TEXT("OutcomeText")));
				Test.TestTrue(TEXT("Human-readable current tactical outcome"), Outcome->GetText().ToString().Contains(Case == TEXT("Defeat") ? TEXT("Your camp was captured") : Case == TEXT("Draw") ? TEXT("TACTICAL DRAW") : TEXT("TACTICAL VICTORY")));
				Capture(Root, TEXT("Outcome")); PreviousWorld = World;
				if (!Click(TEXT("ContinueCampaignButton"))) return true; Test.TestTrue(TEXT("Continue consumes once"), State->HasAppliedCampaignEncounter(Encounter)); Stage = 7; return false;
			}
			if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
			CheckWallets(); Test.TestEqual(TEXT("Return repeats no turn"), State->GetCurrentTurn(), 5); Test.TestFalse(TEXT("Return clears AI queue"), ReadBool(Test, Manager, TEXT("IsAwaitingAITacticalTravel")));
			Test.TestFalse(TEXT("Return clears plan"), State->GetCampaignEncounterPlan().EncounterID.IsValid()); Test.TestTrue(TEXT("Replay ledger persists"), State->HasAppliedCampaignEncounter(Encounter));
			Test.TestEqual(TEXT("No tactical actors on return"), World->GetSubsystem<URomanUnitRegistrySubsystem>()->GetRegisteredUnitCount(), 0);
			FCampaignArmy Army; const bool bWin = Case != TEXT("Defeat") && Case != TEXT("Draw"); Test.TestEqual(TEXT("Roman identity or retirement"), State->TryGetCampaignArmy(RomanArmy, Army), bWin);
			if (bWin) Test.TestEqual(TEXT("Roman survivors exact"), FCampaignArmyState::GetUnitCount(Army), 13);
			Test.TestEqual(TEXT("AI identity or retirement"), State->TryGetCampaignArmy(AIArmy, Army), Case == TEXT("Defeat"));
			if (Case == TEXT("Defeat")) { Test.TestEqual(TEXT("AI survivors exact"), FCampaignArmyState::GetUnitCount(Army), 13); Test.TestEqual(TEXT("AI original army relocates"), Army.TerritoryID, FName(TEXT("Rome"))); }
			FTerritoryPresentationSnapshot Rome; State->TryGetTerritorySnapshot(TEXT("Rome"), Rome); Test.TestEqual(TEXT("Only occupation changes owner"), Rome.OwnerFaction, Case == TEXT("Defeat") ? EFactionID::Carthage : EFactionID::Rome); return true;
		}
	private:
		FAutomationTestBase& Test; FString Case; double StartedAt, TerminalAt = 0.0; int32 Stage = 0, Turns = 0;
		TWeakObjectPtr<UWorld> PreviousWorld; TWeakObjectPtr<URomanUnitDefinition> Definition; TWeakObjectPtr<URomanTerritoryDefinition> Target; TStrongObjectPtr<UCampaignAIExecutionProbe> Probe;
		FFactionResourceAmounts OriginalCost, Price; TSoftObjectPtr<UWorld> OriginalMap; TMap<EFactionID, FFactionResourceAmounts> Wallets; FGuid RomanArmy, AIArmy, Encounter;
		TArray<TWeakObjectPtr<AActor>> Romans, Enemies; TArray<FVector> InitialLocations;
		void Kill(AActor* Actor) { if (IsValid(Actor)) { FCall Call(Test, Actor, TEXT("SetCurrentHealth"), 1); Call.SetInt(TEXT("NewHealth"), 0); Call.Execute(); } }
		void Capture(UUserWidget* Root, const TCHAR* Name)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureCampaignAI"))) return;
			TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue; if (!Test.TestTrue(TEXT("Defense HUD renders"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/C015E3B_Visual")); IFileManager::Get().MakeDirectory(*Directory, true);
			Test.TestTrue(TEXT("Defense capture saved"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory, Case + Name + TEXT(".png"))));
		}
	};

	class FExerciseCampaignArmyUI : public IAutomationLatentCommand
	{
	public:
		FExerciseCampaignArmyUI(FAutomationTestBase* InTest, bool bInVictory, bool bInPersistence = false)
			: Test(*InTest), bVictory(bInVictory), bPersistence(bInPersistence), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignArmyUI()
		{
			if (Definition.IsValid()) Definition->Definition.TrainingCost = OriginalCost;
			RemoveContentFixture();
		}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 80.0) { Test.AddError(TEXT("Recruit/assign UI journey timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			if (LayoutWorld.Get() != World || LayoutStage != Stage) { LayoutWorld = World; LayoutStage = Stage; LayoutAt = FPlatformTime::Seconds(); return false; }
			if (FPlatformTime::Seconds() - LayoutAt < 0.15) return false;
			auto* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			UObject* Controller = World->GetFirstPlayerController();
			auto* Root = Cast<UUserWidget>(ReadObject(Test, Controller, TEXT("GetMainWidget")));
			if (!Test.TestNotNull(TEXT("Army action journey uses production native root"), Root)) return true;
			UObject* Factions = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetFactionManager"));
			const auto Click = [&](const TCHAR* Name) { return ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(Name))); };
			const auto Selected = [&]() { FCall Call(Test, Root, TEXT("GetSelectedCampaignArmyID"), 1); Call.Execute(); return Call.ReadGuid(); };
			const auto Target = [&]() { FCall Call(Test, Root, TEXT("GetAssignmentTargetArmyID"), 1); Call.Execute(); return Call.ReadGuid(); };
			const auto Count = [&]() { return ReadInt(Test, Root, TEXT("GetCampaignActionCount")); };
			const auto SetCount = [&](int32 Value) { FCall Call(Test, Root, TEXT("SetCampaignActionCount"), 2); Call.SetInt(TEXT("Count"), Value); Call.Execute(); return Call.ReadBool(); };
			const auto Enabled = [&](const TCHAR* Name) { return CastChecked<UButton>(Root->GetWidgetFromName(Name))->GetIsEnabled(); };
			const auto Total = [&]() { int32 Value = 0; for (const auto& Army : State->GetCampaignArmies()) if (Army.Faction == EFactionID::Rome) Value += FCampaignArmyState::GetUnitCount(Army); return Value; };
			const auto Resources = [&](FFactionResourceAmounts Cost, bool bCredit)
			{
				FCall Call(Test, Factions, bCredit ? TEXT("ApplyFactionResourceDelta") : TEXT("TrySpendFactionResources"), 3); Call.SetEnum(TEXT("FactionID"), EFactionID::Rome);
				if (bCredit) { FFactionResourceDelta Delta; Delta.Population = Cost.Population; Call.SetStruct(TEXT("Delta"), Delta); }
				else Call.SetStruct(TEXT("Cost"), Cost);
				Call.Execute(); return Call.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful();
			};
			if (Stage == 0)
			{
				Test.TestFalse(TEXT("Sandbox does not expose strategic purchases"), Root->GetWidgetFromName(TEXT("CampaignArmyActionsPanel"))->IsVisible());
				PreviousWorld = World; if (!Click(TEXT("EnterCampaignButton"))) return true; ++Stage; return false;
			}
			if (Stage == 1)
			{
				if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
				for (const auto& Army : State->GetCampaignArmies()) if (Army.Faction == EFactionID::Rome) OriginalArmy = Army.ArmyID;
				Definition = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
				if (!Test.TestTrue(TEXT("Loaded unit fixture exists"), Definition.IsValid())) return true;
				OriginalCost = Definition->Definition.TrainingCost;
				FFactionResourceAmounts Six; Six.Gold = 1; Six.Food = 2; Six.Iron = 3; Six.Wood = 4; Six.Stone = 5; Six.Population = 6;
				Definition->Definition.TrainingCost = Six; ExpectedWallet = State->GetFactionResources(EFactionID::Rome);
				// Production has one unit definition. Add a transient duplicate to the real
				// reflected catalog solely to exercise cycling; remove it before any purchase.
				RegistryFixture = URomanContentRegistrySubsystem::Get(World);
				const auto* UnitMap = FindFProperty<FMapProperty>(RegistryFixture->GetClass(), TEXT("UnitsByID"));
				if (!Test.TestNotNull(TEXT("Reflected catalog fixture map exists"), UnitMap)) return true;
				OtherDefinition = DuplicateObject<URomanUnitDefinition>(Definition.Get(), RegistryFixture.Get(), TEXT("ArmyUIFixtureDefinition"));
				OtherDefinition->Definition.UnitID = TEXT("ArmyUIFixture");
				UnitMap->ContainerPtrToValuePtr<TMap<FName, TObjectPtr<URomanUnitDefinition>>>(RegistryFixture.Get())->Add(OtherDefinition->Definition.UnitID, OtherDefinition.Get());
				FFactionResourceAmounts Four = Six; Four.Gold *= 4; Four.Food *= 4; Four.Iron *= 4; Four.Wood *= 4; Four.Stone *= 4; Four.Population *= 4;
				Test.TestTrue(TEXT("Fixture computes exact four-unit debit"), RomanEmpireEconomy::TrySpendResources(ExpectedWallet, Four).WasSuccessful());
				if (!ClickRegion(Root, TEXT("Rome")) || !ClickArmy(Root, OriginalArmy)) return true;
				FCall Choices(Test, Controller, TEXT("GetCampaignUnitChoices"), 1); Choices.Execute(); const auto UnitIDs = Choices.ReadNameArray();
				Test.TestTrue(TEXT("Transient catalog fixture exposes a real alternate unit choice"), UnitIDs.Num() > 1);
				if (UnitIDs.Num() > 1)
				{
					if (!Click(TEXT("CampaignUnitChoiceButton"))) return true;
					FCall Unit(Test, Root, TEXT("GetCampaignActionUnitID"), 1); Unit.Execute(); Test.TestTrue(TEXT("Physical unit cycle chooses a different current content ID"), Unit.ReadName() != TEXT("Legionary"));
					for (int32 I = 0; I < UnitIDs.Num(); ++I) { FCall Read(Test, Root, TEXT("GetCampaignActionUnitID"), 1); Read.Execute(); if (Read.ReadName() == TEXT("Legionary")) break; if (!Click(TEXT("CampaignUnitChoiceButton"))) return true; }
				}
				RemoveContentFixture();
				Test.TestFalse(TEXT("Quantity minimum prevents underflow"), Enabled(TEXT("CampaignCountDecreaseButton")));
				if (!Click(TEXT("CampaignCountIncreaseButton"))) return true;
				Test.TestEqual(TEXT("Physical count increment selects two"), Count(), 2);
				for (const TCHAR* Name : {TEXT("CampaignUnitChoiceButton"), TEXT("CampaignCountIncreaseButton"), TEXT("RecruitNewArmyButton"), TEXT("RecruitSelectedArmyButton"), TEXT("AssignArmyUnitsButton")})
					Test.TestFalse(TEXT("Army HUD controls preserve gameplay keyboard focus"), CastChecked<UButton>(Root->GetWidgetFromName(Name))->GetCachedWidget()->SupportsKeyboardFocus());
				if (!Click(TEXT("RecruitNewArmyButton"))) return true;
				NewArmy = Selected(); Test.TestTrue(TEXT("Routed purchase selects actual new persistent army"), NewArmy.IsValid() && NewArmy != OriginalArmy);
				Test.TestEqual(TEXT("One routed purchase adds exactly two"), Total(), 17); ++Stage; return false;
			}
			if (Stage == 2)
			{
				if (!Click(TEXT("CampaignCountDecreaseButton"))) return true;
				Test.TestEqual(TEXT("Physical decrement restores one"), Count(), 1);
				Test.TestTrue(TEXT("Post-event quotes keep selected purchase enabled"), Enabled(TEXT("RecruitSelectedArmyButton")));
				if (!Click(TEXT("RecruitSelectedArmyButton"))) return true;
				Test.TestEqual(TEXT("Chosen army reinforcement adds exactly one"), Total(), 18); ++Stage; return false;
			}
			if (Stage == 3) { if (!Click(TEXT("RecruitNewArmyButton"))) return true; Test.TestEqual(TEXT("Second new army adds exactly one"), Total(), 19); ++Stage; return false; }
			if (Stage == 4)
			{
				if (!ClickArmy(Root, NewArmy)) return true;
				const FGuid FirstTarget = Target(); Test.TestTrue(TEXT("Two explicit co-located destinations allow cycling"), Enabled(TEXT("AssignmentTargetButton")));
				if (!Click(TEXT("AssignmentTargetButton"))) return true;
				Test.TestTrue(TEXT("Pointer target cycle picks a different owned stable ID"), Target().IsValid() && Target() != FirstTarget && Target() != NewArmy);
				if (!Click(TEXT("AssignArmyUnitsButton"))) return true;
				Test.TestEqual(TEXT("Partial assignment cannot mint or charge troops"), Total(), 19); ++Stage; return false;
			}
			if (Stage == 5)
			{
				if (!Click(TEXT("CampaignCountIncreaseButton")) || !Click(TEXT("AssignArmyUnitsButton"))) return true;
				FCampaignArmy Army; Test.TestFalse(TEXT("Full pointer assignment retires depleted source"), State->TryGetCampaignArmy(NewArmy, Army));
				Test.TestTrue(TEXT("Retired selection resolves to surviving destination"), Selected().IsValid() && Selected() != NewArmy);
				Test.TestEqual(TEXT("Complete assignment conserves all nineteen"), Total(), 19); ++Stage; return false;
			}
			if (Stage == 6)
			{
				Test.TestTrue(TEXT("Batch limit selectable"), SetCount(100)); Test.TestFalse(TEXT("Maximum prevents increment overflow"), Enabled(TEXT("CampaignCountIncreaseButton")));
				Test.TestFalse(TEXT("Out-of-range count rejects without clamping"), SetCount(101) || SetCount(0)); Test.TestEqual(TEXT("Rejected count preserves choice"), Count(), 100);
				Test.TestFalse(TEXT("Unaffordable batch disables both purchases"), Enabled(TEXT("RecruitNewArmyButton")) || Enabled(TEXT("RecruitSelectedArmyButton")));
				FCall(Test, Root, TEXT("HandleRecruitNewArmyClicked"), 0).Execute(); Test.TestEqual(TEXT("Disabled callback cannot mint inventory"), Total(), 19);
				Test.TestTrue(TEXT("Restore single-unit choice"), SetCount(1));
				FFactionResourceAmounts Population; Population.Population = State->GetFactionResources(EFactionID::Rome).Population;
				Test.TestTrue(TEXT("Fixture drains an actually required currency"), Resources(Population, false));
				Test.TestFalse(TEXT("Resource event disables stale funded purchase"), Enabled(TEXT("RecruitNewArmyButton")));
				Capture(Root, TEXT("NoResources"));
				Test.TestTrue(TEXT("Fixture restores exact resource amount"), Resources(Population, true));
				Test.TestTrue(TEXT("Resource event alone re-enables valid purchase"), Enabled(TEXT("RecruitNewArmyButton")));
				const auto Wallet = State->GetFactionResources(EFactionID::Rome); Test.TestTrue(TEXT("Purchases debit all six exactly, assignments charge nothing"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &ExpectedWallet, 0));
				Root->RemoveFromParent(); ++Stage; return false;
			}
			if (Stage == 7)
			{
				Test.TestEqual(TEXT("Detached root removes army subscription"), ListenerCount(Test, State, TEXT("OnCampaignArmiesChanged"), Root), 0);
				Test.TestFalse(TEXT("Detached choices cannot be edited"), SetCount(2));
				for (const TCHAR* Name : {TEXT("HandleRecruitNewArmyClicked"), TEXT("HandleRecruitSelectedArmyClicked"), TEXT("HandleAssignArmyUnitsClicked")}) FCall(Test, Root, Name, 0).Execute();
				Test.TestEqual(TEXT("Detached callbacks cannot mutate inventories"), Total(), 19); Root->AddToViewport(); ++Stage; return false;
			}
			if (Stage == 8)
			{
				Test.TestEqual(TEXT("Reattachment binds one army invalidation"), ListenerCount(Test, State, TEXT("OnCampaignArmiesChanged"), Root), 1);
				if (!ClickRegion(Root, TEXT("Africa"))) return true;
				Test.TestFalse(TEXT("Hostile selection cannot recruit"), Enabled(TEXT("RecruitNewArmyButton")));
				FCall Tactical(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1); Tactical.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); Tactical.Execute();
				Test.TestFalse(TEXT("Strategic zoom mode exit collapses army actions"), Root->GetWidgetFromName(TEXT("CampaignArmyActionsPanel"))->IsVisible());
				Test.TestFalse(TEXT("Mode exit cannot edit action choices"), SetCount(2));
				FCall(Test, Root, TEXT("HandleRecruitNewArmyClicked"), 0).Execute(); Test.TestEqual(TEXT("Mode exit cannot mutate quantities"), Total(), 19);
				FCall Strategic(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1); Strategic.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); Strategic.Execute(); ++Stage; return false;
			}
			if (Stage == 9)
			{
				if (bPersistence)
				{
					auto* Save = World->GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>();
					for (auto Slot : {ECampaignSaveSlot::Manual, ECampaignSaveSlot::Autosave}) for (int32 I = 0; I < 2; ++I)
					{
						const FString Name = Save->GetStorageSlotName(Slot, I);
						if (!Test.TestTrue(TEXT("Combined journey touches ONLY isolated test slots"), Name.StartsWith(TEXT("RomanEmpire_Automation_")))) return true;
						UGameplayStatics::DeleteGameInSlot(Name, 0);
					}
					if (!Click(TEXT("AutosaveToggleButton")) || !Click(TEXT("SaveCampaignButton"))) return true;
					Test.TestFalse(TEXT("Combined journey keeps manual slot independent from autosave"), Save->GetSettings().bAutosaveEnabled);
					PipelineProbe = TStrongObjectPtr<UCampaignSaveProbe>(NewObject<UCampaignSaveProbe>());
					PipelineProbe->Test = &Test; PipelineProbe->Save = Save;
					State->OnTurnStepStarted.AddUniqueDynamic(PipelineProbe.Get(), &UCampaignSaveProbe::OnStep);
					if (!Click(TEXT("EndTurnButton"))) return true;
					Test.TestEqual(TEXT("Combined native journey advances before battle"), State->GetCurrentTurn(), 2);
					Test.TestEqual(TEXT("Production and AI execute one canonical nine-stage turn"), PipelineProbe->TurnStages, 9);
					ExpectedWallet = State->GetFactionResources(EFactionID::Rome);
				}
				if (!ClickRegion(Root, TEXT("Rome")) || !ClickArmy(Root, OriginalArmy)) return true;
				Capture(Root, TEXT("Ready"));
				FCampaignArmy Army; State->TryGetCampaignArmy(OriginalArmy, Army); ExpectedDeployment = FCampaignArmyState::GetUnitCount(Army);
				Definition->Definition.TrainingCost = OriginalCost;
				if (!ClickRegion(Root, TEXT("Africa")) || !Click(TEXT("StartBattleButton"))) return true;
				Test.TestFalse(TEXT("Travel disables recruitment synchronously"), Enabled(TEXT("RecruitNewArmyButton")));
				PreviousWorld = World; ++Stage; return false;
			}
			auto* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (Stage == 10)
			{
				if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
				int32 InitialRomans = 0; for (const auto& Squad : Battle->GetResult().Squads) if (Squad.Faction == EFactionID::Rome) InitialRomans += Squad.InitialUnits;
				Test.TestEqual(TEXT("Only chosen strategic army deploys, not the other owned army"), InitialRomans, ExpectedDeployment);
				Test.TestEqual(TEXT("Other strategic army's inventory survives the tactical world"), Total(), 19);
				if (bPersistence) JourneyEncounter = State->GetCampaignEncounterPlan().EncounterID;
				TArray<AActor*> Romans, Enemies;
				for (TActorIterator<AActor> It(World); It; ++It) if (It->FindFunction(TEXT("CommandHold")))
				{
					FCall(Test, *It, TEXT("CommandHold"), 0).Execute(); FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute();
					(Owner.ReadEnum<EFactionID>() == EFactionID::Rome ? Romans : Enemies).Add(*It);
				}
				if (!Test.TestEqual(TEXT("Actual factory count matches purchased/assigned chosen army"), Romans.Num(), ExpectedDeployment) || Romans.IsEmpty()) return true;
				if (bVictory) { Kill(Romans.Last()); Romans[0]->SetActorLocation(Battle->GetObjectiveActor()->GetActorLocation() + FVector(0,550,96), false, nullptr, ETeleportType::TeleportPhysics); for (AActor* Enemy : Enemies) Kill(Enemy); }
				else for (AActor* Roman : Romans) Kill(Roman);
				++Stage; return false;
			}
			if (Stage == 11)
			{
				if (!Battle->IsResolved()) return false;
				Capture(Root, TEXT("Outcome")); if (!Click(TEXT("ContinueCampaignButton"))) return true;
				PreviousWorld = World; ++Stage; return false;
			}
			if (World == PreviousWorld.Get() || Session->HasPendingTransfer()) return false;
			if (bPersistence && Stage == 13 && Session->GetPhase() == ECampaignTransferPhase::TacticalDeployment)
			{
				// After the human defeat, the canonical AI can attack its weakened neighbor.
				// Finish the actual defense and Continue to a safe load boundary; do not alter AI policy.
				if (!Battle->IsResolved())
				{
					for (TActorIterator<AActor> It(World); It; ++It) if (It->FindFunction(TEXT("CommandHold")))
					{ FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute(); if (Owner.ReadEnum<EFactionID>() == EFactionID::Carthage) Kill(*It); }
					return false;
				}
				if (!Click(TEXT("ContinueCampaignButton"))) return true;
				PreviousWorld = World; return false;
			}
			Test.TestEqual(TEXT("Real Continue returns to campaign"), State->GetCurrentGameplayMode(), EGameplayMode::Strategic);
			if (bPersistence && Stage == 14)
			{
				auto* Save = World->GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>();
				Test.TestEqual(TEXT("Combined save/advance/load restores post-battle turn"), State->GetCurrentTurn(), SavedJourney.Campaign.Turn.CurrentTurn);
				Test.TestEqual(TEXT("Post-battle save captures live restored authority"), Save->SaveCampaign(), ECampaignSaveFailure::None);
				FCampaignSaveRecord Actual; if (!ReadJourneySave(Save, Actual)) return true;
				Test.TestTrue(TEXT("Complete native first-playable journey restores EVERY canonical post-battle value/IDs/RNG/ledger"), FCampaignTransferSnapshot::StaticStruct()->CompareScriptStruct(&Actual.Campaign, &SavedJourney.Campaign, 0));
				return true;
			}
			if (bPersistence && Stage == 13)
			{
				if (!Click(TEXT("LoadCampaignButton")) || !Click(TEXT("ConfirmSaveActionButton"))) return true;
				PreviousWorld = World; Stage = 14; return false;
			}
			Test.TestEqual(TEXT("Actual casualties conserve surviving armies across return"), Total(), bVictory ? 18 : 19 - ExpectedDeployment);
			FCampaignArmy Original; Test.TestEqual(TEXT("Retirement belongs only to the actual deployed army"), State->TryGetCampaignArmy(OriginalArmy, Original), bVictory);
			const auto Wallet = State->GetFactionResources(EFactionID::Rome); Test.TestTrue(TEXT("UI purchases persist after actual outcome/return, without duplicate spend/refund"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &ExpectedWallet, 0));
			if (bPersistence)
			{
				auto* Save = World->GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>();
				if (!Click(TEXT("SaveCampaignButton")) || !Click(TEXT("ConfirmSaveActionButton"))) return true;
				if (!ReadJourneySave(Save, SavedJourney)) return true;
				Test.TestTrue(TEXT("Saved post-battle ledger retains consumed encounter exactly once"), SavedJourney.Campaign.Encounter.AppliedEncounterIDs.Contains(JourneyEncounter));
				Test.TestEqual(TEXT("Ownership after actual battle agrees with canonical territory model"), State->GetFactionTerritoryCount(EFactionID::Rome), bVictory ? 2 : 1);
				State->OnTurnStepStarted.AddUniqueDynamic(PipelineProbe.Get(), &UCampaignSaveProbe::OnStep);
				if (!Click(TEXT("EndTurnButton"))) return true;
				Test.TestEqual(TEXT("Post-battle advance executes production/AI once, not replaying the prior encounter"), PipelineProbe->TurnStages, 18);
				Test.TestEqual(TEXT("Post-battle advance changes turn before load"), State->GetCurrentTurn(), 3);
				Stage = 13; return false;
			}
			return true;
		}
	private:
		FAutomationTestBase& Test;
		bool bVictory, bPersistence;
		double StartedAt, LayoutAt = 0;
		int32 Stage = 0, LayoutStage = -1, ExpectedDeployment = 0;
		FGuid OriginalArmy, NewArmy;
		FGuid JourneyEncounter; FCampaignSaveRecord SavedJourney; TStrongObjectPtr<UCampaignSaveProbe> PipelineProbe;
		bool ReadJourneySave(UCampaignSaveSubsystem* Save, FCampaignSaveRecord& Out)
		{
			bool Found = false;
			for (int32 I = 0; I < 2; ++I)
			{
				const FString Name = Save->GetStorageSlotName(ECampaignSaveSlot::Manual, I); TArray<uint8> Bytes; FCampaignSaveRecord Record;
				if (UGameplayStatics::DoesSaveGameExist(Name, 0) && UGameplayStatics::LoadDataFromSlot(Bytes, Name, 0) && RomanCampaignSave::Decode(Bytes, Record) == ECampaignSaveFailure::None && (!Found || Record.Generation > Out.Generation))
				{ Found = true; Out = MoveTemp(Record); }
			}
			return Test.TestTrue(TEXT("Combined journey reads actual versioned disk save"), Found);
		}
		TWeakObjectPtr<UWorld> PreviousWorld, LayoutWorld;
		TWeakObjectPtr<URomanUnitDefinition> Definition, OtherDefinition;
		TWeakObjectPtr<URomanContentRegistrySubsystem> RegistryFixture;
		void RemoveContentFixture()
		{
			if (RegistryFixture.IsValid() && OtherDefinition.IsValid())
				if (const auto* UnitMap = FindFProperty<FMapProperty>(RegistryFixture->GetClass(), TEXT("UnitsByID")))
					UnitMap->ContainerPtrToValuePtr<TMap<FName, TObjectPtr<URomanUnitDefinition>>>(RegistryFixture.Get())->Remove(OtherDefinition->Definition.UnitID);
			RegistryFixture.Reset(); OtherDefinition.Reset();
		}
		FFactionResourceAmounts OriginalCost, ExpectedWallet;
		void Kill(AActor* Actor) { FCall Call(Test, Actor, TEXT("SetCurrentHealth"), 1); Call.SetInt(TEXT("NewHealth"), 0); Call.Execute(); }
		bool ClickArmy(UUserWidget* Root, FGuid ID)
		{
			for (UWidget* Widget : CastChecked<UScrollBox>(Root->GetWidgetFromName(TEXT("CampaignArmyList")))->GetAllChildren())
			{ FCall Key(Test, Widget, TEXT("GetArmyID"), 1); Key.Execute(); if (Key.ReadGuid() == ID) return ClickThroughSlate(Test, Root, Cast<UButton>(Widget)); }
			Test.AddError(TEXT("Actual army card missing")); return false;
		}
		bool ClickRegion(UUserWidget* Root, FName ID)
		{
			for (UWidget* Widget : CastChecked<UPanelWidget>(Root->GetWidgetFromName(TEXT("TerritoryGrid")))->GetAllChildren())
			{ FCall Key(Test, Widget, TEXT("GetTerritoryID"), 1); Key.Execute(); if (Key.ReadName() == ID) return ClickThroughSlate(Test, Root, Cast<UButton>(Widget)); }
			Test.AddError(TEXT("Actual region card missing")); return false;
		}
		void Capture(UUserWidget* Root, const TCHAR* Name)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureCampaignArmyUI"))) return;
			TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Actual army action controls render"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/C015E2_Visual")); IFileManager::Get().MakeDirectory(*Directory, true);
			Test.TestTrue(TEXT("Army action capture saved"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory, FString(bVictory ? TEXT("Victory") : TEXT("Defeat")) + Name + TEXT(".png"))));
		}
	};

	class FExerciseCampaignTravel : public IAutomationLatentCommand
	{
	public:
		FExerciseCampaignTravel(FAutomationTestBase* InTest, bool bInVictory, bool bInUI = false, bool bInCampaignWin = false)
			: Test(*InTest), bVictory(bInVictory), bUI(bInUI), bCampaignWin(bInCampaignWin), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignTravel() { FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitialized); }
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 60.0) { Test.AddError(TEXT("Actual campaign session travel timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UCampaignSessionSubsystem* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			UObject* Mode = World->GetAuthGameMode();
			UObject* Manager = ReadObject(Test, Mode, TEXT("GetCampaignManager"));
			UObject* Map = ReadObject(Test, Mode, TEXT("GetWorldMapManager"));
			UObject* Factions = ReadObject(Test, Mode, TEXT("GetFactionManager"));
			UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			UUserWidget* Root = Cast<UUserWidget>(ReadObject(Test, World->GetFirstPlayerController(), TEXT("GetMainWidget")));
			if (bUI && !Test.TestNotNull(TEXT("Real campaign controls are in the production root"), Root)) return true;
			if (bUI && LayoutWorld.Get() != World) { LayoutWorld = World; LayoutAt = FPlatformTime::Seconds(); return false; }
			if (bUI && FPlatformTime::Seconds() - LayoutAt < 0.2) return false;
			if (!Test.TestNotNull(TEXT("Automatic GameInstance session subsystem exists"), Session) || !Manager || !Map || !Factions || !Battle) return true;
			AActor* Producer = nullptr;
			for (TActorIterator<AActor> It(World); It; ++It) if (It->FindFunction(TEXT("TrainUnit"))) Producer = *It;
			if (!Test.TestNotNull(TEXT("Real world has a producer"), Producer)) return true;
			const auto Train = [&]() { FCall Call(Test, Producer, TEXT("TrainUnit"), 2); Call.SetName(TEXT("UnitID"), TEXT("Legionary")); Call.Execute(); return Call.ReadBool(); };
			const auto Credit = [&](int32 Gold) { FFactionResourceDelta Delta; Delta.Gold = Gold;
				FCall Call(Test, Factions, TEXT("ApplyFactionResourceDelta"), 3); Call.SetEnum(TEXT("FactionID"), EFactionID::Rome);
				Call.SetStruct(TEXT("Delta"), Delta); Call.Execute(); return Call.ReadStruct<FFactionResourceTransactionResult>().WasSuccessful(); };
			const auto WalletMatches = [&]() { const auto Wallet = State->GetFactionResources(EFactionID::Rome);
				return FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &ExpectedWallet, 0); };
			if (Stage == 0)
			{
				if (!bEntryPrepared)
				{
				SessionIdentity = Session; PreviousWorld = World; InitialArmies = State->GetCampaignArmies();
				for (const auto& Army : InitialArmies) if (Army.Faction == EFactionID::Rome) Attacker = Army.ArmyID;
				FCall Own(Test, Map, TEXT("ChangeTerritoryOwner"), 3); Own.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Own.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Own.Execute();
				FCall Found(Test, Map, TEXT("FoundTerritorySettlement"), 3); Found.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Found.SetText(TEXT("Name"), FText::FromString(TEXT("Travel City"))); Found.Execute();
				Test.TestTrue(TEXT("Fixture founds an actual canonical city"), Found.ReadStruct<FSettlementFoundingResult>().WasSuccessful());
				ExpectedWallet = State->GetFactionResources(EFactionID::Rome);
				if (!Test.TestTrue(TEXT("Actual active and queued prepaid orders exist"), Train() && Train())) return true;
				FCall Receipt(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1); Receipt.Execute(); const auto BeforeReceipt = Receipt.ReadStruct<FTrainingReservation>();
				const int32 AfterPayment = State->GetFactionResources(EFactionID::Rome).Gold;
				Test.TestTrue(TEXT("Credit fixture reaches representable wallet maximum"), Credit(MAX_int32 - AfterPayment));
				Test.TestEqual(TEXT("Combined refund overflow blocks actual travel"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::TransferRejected);
				Test.TestEqual(TEXT("Blocked travel reports exact refund failure"), Session->GetLastTransferFailure(), ECampaignTransferFailure::RefundRejected);
				Test.TestFalse(TEXT("No capsule/travel installed on failed preflight"), Session->HasPendingTransfer());
				Test.TestEqual(TEXT("Failure preserves maximum wallet"), State->GetFactionResources(EFactionID::Rome).Gold, MAX_int32);
				FCall Unchanged(Test, Producer, TEXT("GetCurrentTrainingReservation"), 1); Unchanged.Execute();
				Test.TestEqual(TEXT("Failure retains original paid receipt"), Unchanged.ReadStruct<FTrainingReservation>().ReceiptID, BeforeReceipt.ReceiptID);
				Test.TestEqual(TEXT("Failure retains queued receipt"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 1);
				Test.TestTrue(TEXT("Wallet can be repaired without lost receipt"), Credit(AfterPayment - MAX_int32));
				Probe.Reset(NewObject<UCampaignTravelProbe>()); Probe->Test = &Test; Probe->State = State; Probe->Session = Session;
				Probe->Producer = Producer; Probe->Factions = Factions; Probe->Map = Map; Probe->ExpectedWallet = ExpectedWallet;
				Probe->ArmyID = Attacker;
				WorldInitialized = FWorldDelegates::OnPostWorldInitialization.AddLambda([this](UWorld* Incoming, const UWorld::InitializationValues)
				{
					if (!Incoming || Incoming->WorldType != EWorldType::PIE) return;
					Incoming->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([WeakProbe = TWeakObjectPtr<UCampaignTravelProbe>(Probe.Get())](AActor* Actor)
					{
						if (ARomanEmpireGameState* IncomingState = Cast<ARomanEmpireGameState>(Actor); IncomingState && WeakProbe.IsValid())
						{
							WeakProbe->State = IncomingState;
							IncomingState->OnGameplayInitialized.AddUniqueDynamic(WeakProbe.Get(), &UCampaignTravelProbe::OnReady);
						}
					}));
				});
				State->OnFactionResourcesChanged.AddUniqueDynamic(Probe.Get(), &UCampaignTravelProbe::OnRefund);
				bEntryPrepared = true;
				if (bUI) { Root->RemoveFromParent(); return false; }
				}
				if (bUI)
				{
					if (!bEntryReattached)
					{
					Test.TestEqual(TEXT("Detached session UI removes GameInstance listener"), ListenerCount(Test, Session, TEXT("OnSessionChanged"), Root), 0);
					FCall(Test, Root, TEXT("HandleEnterCampaignClicked"), 0).Execute();
					Test.TestFalse(TEXT("Detached entry cannot schedule travel"), Session->HasPendingTransfer());
					Root->AddToViewport();
					bEntryReattached = true; return false;
					}
					Test.TestEqual(TEXT("Reattached session UI binds once"), ListenerCount(Test, Session, TEXT("OnSessionChanged"), Root), 1);
					if (!ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(TEXT("EnterCampaignButton"))))) return true;
					Test.TestTrue(TEXT("Routed entry control schedules real travel"), Session->HasPendingTransfer());
					Test.TestFalse(TEXT("Entry disabled immediately while travel pending"), CastChecked<UButton>(Root->GetWidgetFromName(TEXT("EnterCampaignButton")))->GetIsEnabled());
					Test.TestFalse(TEXT("Pending controller cannot issue tactical commands or turns"), ReadBool(Test, World->GetFirstPlayerController(), TEXT("CanIssueTacticalCommands")) || ReadBool(Test, World->GetFirstPlayerController(), TEXT("CanEndTurn")));
				}
				else Test.TestEqual(TEXT("Strategic entry schedules real travel"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::None);
				State->OnFactionResourcesChanged.RemoveDynamic(Probe.Get(), &UCampaignTravelProbe::OnRefund);
				Test.TestEqual(TEXT("One committed wallet notification for both receipts"), Probe->RefundNotifications, 1);
				Test.TestEqual(TEXT("Repeated entry cannot schedule another world"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::Busy);
				Stage = 1; return false;
			}
			if (Session->HasPendingTransfer() || World == PreviousWorld.Get()) return false;
			Test.TestTrue(TEXT("Same GameInstance coordinator survives actual new world"), Session == SessionIdentity.Get() && Session->HasSession());
			FTerritoryPresentationSnapshot City;
			Test.TestTrue(TEXT("Transferred city exists before ready reads"), State->TryGetTerritorySnapshot(TEXT("Sicily"), City));
			Test.TestEqual(TEXT("Transferred owner is not reset to authored neutral"), City.OwnerFaction, EFactionID::Rome);
			Test.TestEqual(TEXT("Transferred city name survives"), City.Settlement.SettlementName.ToString(), FString(TEXT("Travel City")));
			if (Stage == 1)
			{
				Test.TestFalse(TEXT("Strategic entry does not reseed sandbox actors/battle"), Battle->HasEncounter());
				Test.TestEqual(TEXT("Actual entry is strategic"), State->GetCurrentGameplayMode(), EGameplayMode::Strategic);
				Test.TestTrue(TEXT("Entry refunds every original currency"), WalletMatches());
				Test.TestEqual(TEXT("Entry preserves stable army IDs"), State->GetCampaignArmies()[0].ArmyID, InitialArmies[0].ArmyID);
				FCall Turn(Test, Manager, TEXT("ProcessTurn"), 1); Turn.Execute();
				Test.TestTrue(TEXT("Strategic entry accepts actual turn"), Turn.ReadStruct<FCampaignTurnResult>().WasSuccessful());
				ExpectedTurn = State->GetCurrentTurn(); ExpectedWallet = State->GetFactionResources(EFactionID::Rome);
				Probe->ExpectedTurn = ExpectedTurn;
				const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(Session);
				const URomanTerritoryDefinition* Territory = Registry->FindTerritory(TEXT("Africa"));
				TargetMap = Territory->TacticalMap.ToSoftObjectPath();
				ExpectedDefenders = 0;
				for (const auto& Army : State->GetCampaignArmies()) if (Army.Faction == EFactionID::Carthage && Army.TerritoryID == TEXT("Africa")) ExpectedDefenders += FCampaignArmyState::GetUnitCount(Army);
				Test.TestEqual(TEXT("Completed canonical AI turn recruits two before player attack"), ExpectedDefenders, 12);
				Test.TestEqual(TEXT("Invalid army never creates a travel capsule"), Session->BeginTacticalEncounter(FGuid::NewGuid(), TEXT("Africa")), ECampaignSessionFailure::EncounterRejected);
				if (bCampaignWin)
					FindFProperty<FIntProperty>(Manager->GetClass(), TEXT("ConquestVictoryTerritories"))->SetPropertyValue_InContainer(Manager, State->GetFactionTerritoryCount(EFactionID::Rome) + 1);
				if (bUI)
				{
					if (!ExerciseArmyUI(Root, State, Factions)) return true;
					CaptureUI(Root, bCampaignWin ? TEXT("GlobalWinStrategy") : bVictory ? TEXT("VictoryStrategy") : TEXT("DefeatStrategy"));
					if (!ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(TEXT("StartBattleButton"))))) return true;
					Test.TestTrue(TEXT("Routed selected army/region battle schedules travel"), Session->HasPendingTransfer());
				}
				else Test.TestEqual(TEXT("Selected-map encounter queues actual travel"), Session->BeginTacticalEncounter(Attacker, TEXT("Africa")), ECampaignSessionFailure::None);
				Test.TestEqual(TEXT("Travel rejects a second command"), Session->BeginTacticalEncounter(Attacker, TEXT("Africa")), ECampaignSessionFailure::Busy);
				PreviousWorld = World; Stage = 2; return false;
			}
			if (Stage == 2)
			{
				Test.TestEqual(TEXT("Engine loaded selected territory's actual map"), UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()), TargetMap.GetLongPackageName());
				Test.TestEqual(TEXT("New battle world is tactical, not strategic camera feedback"), State->GetCurrentGameplayMode(), EGameplayMode::Tactical);
				Test.TestEqual(TEXT("Turn carried into battle"), State->GetCurrentTurn(), ExpectedTurn);
				Test.TestTrue(TEXT("Every wallet field carried into battle"), WalletMatches());
				EncounterID = State->GetCampaignEncounterPlan().EncounterID;
				Test.TestEqual(TEXT("Canonical actual battle uses transferred encounter ID"), Battle->GetResult().EncounterID, EncounterID);
				Test.TestEqual(TEXT("Return cannot consume an unresolved result"), Session->ReturnToStrategicCampaign(), ECampaignSessionFailure::Unresolved);
				if (!Test.TestTrue(TEXT("Battle accepts active and queued paid receipts"), Train() && Train())) return true;
				TArray<AActor*> Romans, Enemies;
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (!It->FindFunction(TEXT("CommandHold"))) continue;
					FCall(Test, *It, TEXT("CommandHold"), 0).Execute();
					FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute();
					(Owner.ReadEnum<EFactionID>() == EFactionID::Rome ? Romans : Enemies).Add(*It);
				}
				Test.TestEqual(TEXT("New local world deploys fifteen canonical Romans"), Romans.Num(), 15);
				Test.TestEqual(TEXT("New local world deploys current canonical defenders"), Enemies.Num(), ExpectedDefenders);
				if (bVictory)
				{
					Kill(Romans.Last());
					Romans[0]->SetActorLocation(Battle->GetObjectiveActor()->GetActorLocation() + FVector(0, 550, 96), false, nullptr, ETeleportType::TeleportPhysics);
					for (AActor* Enemy : Enemies) Kill(Enemy);
				}
				else for (AActor* Roman : Romans) Kill(Roman);
				Stage = 3; return false;
			}
			if (Stage == 3)
			{
				if (!Battle->IsResolved()) return false;
				Test.TestEqual(TEXT("Actual physical encounter has expected terminal"), Battle->GetResult().Outcome, bVictory ? ETacticalBattleOutcome::PlayerVictory : ETacticalBattleOutcome::PlayerDefeat);
				Test.TestFalse(TEXT("Mapped terminal cannot sandbox-restart and lose campaign"), ReadBool(Test, Mode, TEXT("CanRestartEncounter")));
				Probe->bExpectedRetired = !bVictory;
				if (bCampaignWin)
				{
					FCall Apply(Test, Manager, TEXT("ApplyCurrentEncounterResult"), 1); Apply.Execute();
					Test.TestEqual(TEXT("Actual capture resolves campaign threshold"), State->GetObjectiveState().Outcome, ECampaignOutcome::Victory);
					Test.TestFalse(TEXT("Global victory closes active campaign before Continue"), State->IsCampaignActive());
				}
				if (bUI)
				{
					if (!bContinueDetached) { Root->RemoveFromParent(); bContinueDetached = true; return false; }
					if (!bContinueReattached)
					{
					FCall(Test, Root, TEXT("HandleContinueCampaignClicked"), 0).Execute();
					Test.TestFalse(TEXT("Detached Continue cannot apply or travel"), Session->HasPendingTransfer());
					Root->AddToViewport();
					bContinueReattached = true; return false;
					}
					Test.TestEqual(TEXT("Reattached root binds one session listener"), ListenerCount(Test, Session, TEXT("OnSessionChanged"), Root), 1);
					auto* Continue = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("ContinueCampaignButton")));
					Test.TestTrue(TEXT("Real Continue enabled, including inactive global winner"), Continue->GetIsEnabled());
					Test.TestEqual(TEXT("Mapped result hides sandbox Restart"), Root->GetWidgetFromName(TEXT("RestartEncounterButton"))->GetVisibility(), ESlateVisibility::Collapsed);
					CaptureUI(Root, bCampaignWin ? TEXT("GlobalWinOutcome") : bVictory ? TEXT("VictoryOutcome") : TEXT("DefeatOutcome"));
					if (!ClickThroughSlate(Test, Root, Continue)) return true;
					Test.TestFalse(TEXT("Continue disables synchronously while return pending"), Continue->GetIsEnabled());
				}
				else Test.TestEqual(TEXT("Actual result application and return schedules travel"), Session->ReturnToStrategicCampaign(), ECampaignSessionFailure::None);
				Test.TestTrue(TEXT("Consumed encounter is canonical before teardown"), State->HasAppliedCampaignEncounter(EncounterID));
				Test.TestFalse(TEXT("All terminal paid production cleared before teardown"), ReadBool(Test, Producer, TEXT("IsTraining")));
				Test.TestEqual(TEXT("No paid queue abandoned at teardown"), ReadInt(Test, Producer, TEXT("GetPendingOrderCount")), 0);
				Test.TestEqual(TEXT("Duplicate return is rejected"), Session->ReturnToStrategicCampaign(), ECampaignSessionFailure::Busy);
				PreviousWorld = World; Stage = 4; return false;
			}
			Test.TestEqual(TEXT("Returned actual world is strategic"), State->GetCurrentGameplayMode(), EGameplayMode::Strategic);
			Test.TestFalse(TEXT("Return does not revive a stale or sandbox battle"), Battle->HasEncounter());
			Test.TestFalse(TEXT("Returned snapshot has no pending plan/local squads"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
			Test.TestTrue(TEXT("Consumed encounter ID survives return"), State->HasAppliedCampaignEncounter(EncounterID));
			Test.TestTrue(TEXT("Terminal active/queued costs are completely refunded"), WalletMatches());
			FCampaignArmy Army;
			Test.TestEqual(TEXT("Army retirement matches actual casualties, not reseeding"), State->TryGetCampaignArmy(Attacker, Army), bVictory);
			if (bVictory)
			{
				Test.TestEqual(TEXT("Victory relocates stable attacker to selected territory"), Army.TerritoryID, FName(TEXT("Africa")));
				Test.TestEqual(TEXT("One real casualty persists after return"), Army.Units[0].Count, 14);
			}
			FTerritoryPresentationSnapshot Target; State->TryGetTerritorySnapshot(TEXT("Africa"), Target);
			Test.TestEqual(TEXT("Ownership follows actual outcome"), Target.OwnerFaction, bVictory ? EFactionID::Rome : EFactionID::Carthage);
			if (bCampaignWin) Test.AddExpectedMessagePlain(TEXT("stopped at ValidateRequest (CampaignInactive)"), ELogVerbosity::Warning);
			FCall Turn(Test, Manager, TEXT("ProcessTurn"), 1); Turn.Execute();
			if (bCampaignWin)
			{
				Test.TestFalse(TEXT("Returned global victory stays inactive"), State->IsCampaignActive());
				Test.TestFalse(TEXT("Finished campaign does not resume turns"), Turn.ReadStruct<FCampaignTurnResult>().WasSuccessful());
				Test.TestEqual(TEXT("Finished campaign preserves terminal turn"), State->GetCurrentTurn(), ExpectedTurn);
			}
			else
			{
				Test.TestTrue(TEXT("Returned campaign resumes turn, no stale EncounterResolved guard"), Turn.ReadStruct<FCampaignTurnResult>().WasSuccessful());
				Test.TestEqual(TEXT("Returned campaign advances exactly once"), State->GetCurrentTurn(), ExpectedTurn + 1);
			}
			Test.TestEqual(TEXT("Every new world published one complete first readiness"), Probe->ReadyNotifications, 3);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		bool bVictory, bUI, bCampaignWin;
		bool bEntryPrepared = false, bEntryReattached = false, bContinueDetached = false, bContinueReattached = false;
		double StartedAt;
		int32 Stage = 0, ExpectedTurn = 0;
		FGuid Attacker, EncounterID;
		int32 ExpectedDefenders = 0;
		FSoftObjectPath TargetMap;
		FFactionResourceAmounts ExpectedWallet;
		TArray<FCampaignArmy> InitialArmies;
		TWeakObjectPtr<UWorld> PreviousWorld;
		TWeakObjectPtr<UCampaignSessionSubsystem> SessionIdentity;
		TStrongObjectPtr<UCampaignTravelProbe> Probe;
		FDelegateHandle WorldInitialized;
		TWeakObjectPtr<UWorld> LayoutWorld;
		double LayoutAt = 0.0;
		void CaptureUI(UUserWidget* Root, const TCHAR* Name)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureCampaignUI"))) return;
			TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Actual campaign controls render to pixels"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/C015D3_Visual")); IFileManager::Get().MakeDirectory(*Directory, true);
			Test.TestTrue(TEXT("Campaign capture written"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory, FString(Name) + TEXT(".png"))));
		}
		bool ExerciseArmyUI(UUserWidget* Root, ARomanEmpireGameState* State, UObject* Factions)
		{
			UWorld* World = State->GetWorld();
			UObject* Mode = World->GetAuthGameMode();
			FCall Tactical(Test, Mode, TEXT("SetGameplayMode"), 1); Tactical.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical); Tactical.Execute();
			Test.TestFalse(TEXT("Strategic session camera zoom cannot enable sandbox commands"), ReadBool(Test, World->GetFirstPlayerController(), TEXT("CanIssueTacticalCommands")));
			const FFactionResourceAmounts Before = State->GetFactionResources(State->GetPlayerFaction());
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->FindFunction(TEXT("TrainUnit"))) continue;
				FCall Train(Test, *It, TEXT("TrainUnit"), 2); Train.SetName(TEXT("UnitID"), TEXT("Legionary")); Train.Execute();
				Test.TestFalse(TEXT("Strategic session rejects direct sandbox training"), Train.ReadBool());
				Test.TestEqual(TEXT("Rejected training queues no untracked receipt"), ReadInt(Test, *It, TEXT("GetPendingOrderCount")), 0);
			}
			Test.TestEqual(TEXT("Rejected session training keeps canonical gold"), State->GetFactionResources(State->GetPlayerFaction()).Gold, Before.Gold);
			FCall Strategic(Test, Mode, TEXT("SetGameplayMode"), 1); Strategic.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic); Strategic.Execute();
			FCall DirectBattle(Test, ReadObject(Test, Mode, TEXT("GetCampaignManager")), TEXT("BeginCampaignEncounter"), 3);
			DirectBattle.SetGuid(TEXT("AttackingArmyID"), Attacker); DirectBattle.SetName(TEXT("TargetTerritoryID"), TEXT("Africa")); DirectBattle.Execute();
			Test.TestEqual(TEXT("Live session rejects direct arena materialization"), DirectBattle.ReadEnum<ECampaignEncounterFailure>(), ECampaignEncounterFailure::WrongMode);
			Test.TestFalse(TEXT("Rejected direct arena creates no battle"), World->GetSubsystem<UTacticalBattleSubsystem>()->HasEncounter());
			const auto RegionButton = [&](FName ID) -> UButton*
			{
				const auto* Grid = CastChecked<UPanelWidget>(Root->GetWidgetFromName(TEXT("TerritoryGrid")));
				for (UWidget* Widget : Grid->GetAllChildren()) { FCall Key(Test, Widget, TEXT("GetTerritoryID"), 1); Key.Execute(); if (Key.ReadName() == ID) return Cast<UButton>(Widget); }
				return nullptr;
			};
			UButton* ArmyButton = nullptr;
			for (UWidget* Widget : CastChecked<UScrollBox>(Root->GetWidgetFromName(TEXT("CampaignArmyList")))->GetAllChildren())
			{
				FCall Key(Test, Widget, TEXT("GetArmyID"), 1); Key.Execute();
				if (Key.ReadGuid() == Attacker) { ArmyButton = Cast<UButton>(Widget); break; }
			}
			if (!ArmyButton)
			{
				FString Names; Root->WidgetTree->ForEachWidget([&Names](UWidget* Widget) { if (Widget->GetName().StartsWith(TEXT("Army"))) Names += Widget->GetName() + TEXT(" "); });
				Test.AddError(FString::Printf(TEXT("Army lookup %s unavailable; children=%d viewport=%d visible=%d widgets=%s"), *Attacker.ToString(EGuidFormats::Digits), CastChecked<UScrollBox>(Root->GetWidgetFromName(TEXT("CampaignArmyList")))->GetChildrenCount(), Root->IsInViewport(), Root->IsVisible(), *Names));
				return false;
			}
			if (!ClickThroughSlate(Test, Root, ArmyButton)) return false;
			Test.TestFalse(TEXT("Army HUD entry never takes keyboard focus"), ArmyButton->GetCachedWidget()->SupportsKeyboardFocus());
			FCampaignArmy Army; State->TryGetCampaignArmy(Attacker, Army);
			FTerritoryPresentationSnapshot Source; State->TryGetTerritorySnapshot(Army.TerritoryID, Source);
			FName Friendly;
			for (const FName ID : Source.AdjacentTerritoryIDs) { FTerritoryPresentationSnapshot Neighbor; State->TryGetTerritorySnapshot(ID, Neighbor); if (Neighbor.OwnerFaction == EFactionID::Rome) { Friendly = ID; break; } }
			if (!Test.TestFalse(TEXT("Fixture has adjacent owned movement destination"), Friendly.IsNone())) return false;
			if (!ClickThroughSlate(Test, Root, RegionButton(Friendly))) return false;
			if (!ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(TEXT("MoveArmyButton"))))) return false;
			State->TryGetCampaignArmy(Attacker, Army); Test.TestEqual(TEXT("Routed MOVE updates canonical stable army"), Army.TerritoryID, Friendly);
			if (!ClickThroughSlate(Test, Root, RegionButton(Source.TerritoryID))) return false;
			if (!ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(TEXT("MoveArmyButton"))))) return false;
			if (!ClickThroughSlate(Test, Root, RegionButton(TEXT("Africa")))) return false;
			auto* BattleButton = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("StartBattleButton")));
			Test.TestTrue(TEXT("Actual selected target enables guarded battle"), BattleButton->GetIsEnabled());
			FCall Neutral(Test, Factions, TEXT("SetDiplomaticStatus"), 4); Neutral.SetEnum(TEXT("Faction1"), EFactionID::Rome); Neutral.SetEnum(TEXT("Faction2"), EFactionID::Carthage); Neutral.SetEnum(TEXT("NewStatus"), EDiplomaticStatus::Neutral); Neutral.Execute();
			Test.TestFalse(TEXT("Diplomacy event disables stale attack eligibility"), BattleButton->GetIsEnabled());
			FCall War(Test, Factions, TEXT("SetDiplomaticStatus"), 4); War.SetEnum(TEXT("Faction1"), EFactionID::Rome); War.SetEnum(TEXT("Faction2"), EFactionID::Carthage); War.SetEnum(TEXT("NewStatus"), EDiplomaticStatus::War); War.Execute();
			Test.TestTrue(TEXT("Diplomacy event re-enables valid attack"), BattleButton->GetIsEnabled());
			return true;
		}
		void Kill(AActor* Actor) { FCall Call(Test, Actor, TEXT("SetCurrentHealth"), 1); Call.SetInt(TEXT("NewHealth"), 0); Call.Execute(); }
	};

	class FExerciseCampaignSave : public IAutomationLatentCommand
	{
	public:
		FExerciseCampaignSave(FAutomationTestBase* InTest, FString InCase) : Test(*InTest), Case(MoveTemp(InCase)), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignSave()
		{
			FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitialized);
			FWorldDelegates::OnPostWorldInitialization.Remove(FailingWorldInitialized);
			if (Target.IsValid()) Target->TacticalMap = OriginalMap;
			if (Definition.IsValid()) Definition->UnitClass = OriginalClass;
		}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 50.0) { Test.AddError(TEXT("Save/load journey timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (Stage == 7 && State && World != PreviousWorld.Get())
			{
				auto* FailedSession = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
				if (FailedSession->GetLastFailure() != ECampaignSessionFailure::RestoreFailed) return false;
				if (LayoutWorld.Get() != World) { LayoutWorld = World; LayoutAt = FPlatformTime::Seconds(); return false; }
				if (FPlatformTime::Seconds() - LayoutAt < 0.15) return false;
				auto* FailedSave = World->GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>();
				Test.TestTrue(TEXT("Failed saved load retains pending capsule"), FailedSession->HasPendingTransfer());
				Test.TestFalse(TEXT("Failed saved load never publishes default readiness"), State->IsGameplayInitialized());
				Test.TestEqual(TEXT("First readiness not emitted for quarantined save"), Probe->ReadyNotifications, 0);
				Test.TestTrue(TEXT("Saved preferences do not leak before successful restoration"), FailedSave->GetSettings().bAutosaveEnabled);
				Test.TestFalse(TEXT("Quarantine cannot save defaults"), FailedSave->CanSaveCampaign());
				Test.TestEqual(TEXT("Restore fails explicitly on current missing factory"), FailedSession->GetLastTransferFailure(), ECampaignTransferFailure::InvalidContent);
				UUserWidget* FailedRoot = Cast<UUserWidget>(ReadObject(Test, World->GetFirstPlayerController(), TEXT("GetMainWidget")));
				if (!Test.TestNotNull(TEXT("Quarantine has native recovery UI"), FailedRoot)) return true;
				Test.TestFalse(TEXT("Unsafe saved-load UI remains collapsed until readiness"), FailedRoot->GetWidgetFromName(TEXT("CampaignSavePanel"))->IsVisible());
				CaptureSaveUI(FailedRoot, TEXT("Quarantine"));
				FWorldDelegates::OnPostWorldInitialization.Remove(FailingWorldInitialized); FailingWorldInitialized.Reset();
				Definition->UnitClass = OriginalClass;
				PreviousWorld = World;
				if (!ClickThroughSlate(Test, FailedRoot, Cast<UButton>(FailedRoot->GetWidgetFromName(TEXT("RetryCampaignButton"))))) return true;
				Stage = 2; return false;
			}
			if (!State || !State->IsGameplayInitialized()) return false;
			auto* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			auto* Save = World->GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>();
			UObject* Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetCampaignManager"));
			const auto Turn = [&]() { FCall Call(Test, Manager, TEXT("ProcessTurn"), 1); Call.Execute(); return Call.ReadStruct<FCampaignTurnResult>(); };
			const bool bUI = Case == TEXT("UI") || Case == TEXT("Resume") || Case == TEXT("Recovery");
			UUserWidget* Root = bUI ? Cast<UUserWidget>(ReadObject(Test, World->GetFirstPlayerController(), TEXT("GetMainWidget"))) : nullptr;
			if (bUI)
			{
				if (!Test.TestNotNull(TEXT("Save UI is the production root"), Root)) return true;
				if (LayoutWorld.Get() != World || LayoutStage != Stage) { LayoutWorld = World; LayoutStage = Stage; LayoutAt = FPlatformTime::Seconds(); return false; }
				if (FPlatformTime::Seconds() - LayoutAt < 0.15) return false;
			}
			const auto Click = [&](const TCHAR* Name) { return ClickThroughSlate(Test, Root, Cast<UButton>(Root->GetWidgetFromName(Name))); };
			if (Stage == 0)
			{
				if (!Test.TestNotNull(TEXT("Persistence subsystem exists"), Save)) return true;
				for (auto Slot : {ECampaignSaveSlot::Manual, ECampaignSaveSlot::Autosave}) for (int32 I = 0; I < 2; ++I)
				{
					const FString Name = Save->GetStorageSlotName(Slot, I);
					if (!Test.TestTrue(TEXT("Tests exclusively use isolated automation slots"), Name.StartsWith(TEXT("RomanEmpire_Automation_")))) return true;
					UGameplayStatics::DeleteGameInSlot(Name, 0); // Exact generated test slots only.
				}
				Test.TestFalse(TEXT("Sandbox cannot save an actual session"), Save->CanSaveCampaign());
				Test.TestEqual(TEXT("Sandbox save fails closed"), Save->SaveCampaign(), ECampaignSaveFailure::UnsafeBoundary);
				Test.TestEqual(TEXT("Absent load returns explicit not found"), Save->LoadCampaign(), ECampaignSaveFailure::NotFound);
				Test.TestFalse(TEXT("Failed load never queues travel"), Session->HasPendingTransfer());
				PreviousWorld = World; Identity = Save;
				Test.TestEqual(TEXT("Save fixture enters actual campaign"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::None);
				Test.TestEqual(TEXT("Pending source cannot save"), Save->SaveCampaign(), ECampaignSaveFailure::UnsafeBoundary);
				Stage = 1; return false;
			}
			if (Session->HasPendingTransfer() || World == PreviousWorld.Get()) return false;
			Test.TestTrue(TEXT("Persistence identity survives actual world travel"), Identity.Get() == Save);
			if (Stage == 5 || Stage == 6)
			{
				if (Stage == 5) Test.TestTrue(TEXT("Save controls reenable after direct turn/operation guards release without UI Tick"), CastChecked<UButton>(Root->GetWidgetFromName(TEXT("SaveCampaignButton")))->GetIsEnabled());
				else
				{
					Test.TestFalse(TEXT("Resume fixture is an actual ready sandbox, not an active session"), Session->HasSession());
					Test.TestFalse(TEXT("Initial sandbox cannot overwrite actual campaign save"), CastChecked<UButton>(Root->GetWidgetFromName(TEXT("SaveCampaignButton")))->GetIsEnabled());
					Test.TestTrue(TEXT("Initial sandbox offers resume from existing disk save"), CastChecked<UButton>(Root->GetWidgetFromName(TEXT("LoadCampaignButton")))->GetIsEnabled());
				}
				if (!Click(TEXT("LoadCampaignButton"))) return true;
				Test.TestFalse(TEXT("Load requires confirmation, never abandons source on first click"), Session->HasPendingTransfer());
				if (!Click(TEXT("CancelSaveActionButton"))) return true;
				Test.TestFalse(TEXT("Cancelling confirmation retains source campaign"), Session->HasPendingTransfer());
				if (!Click(TEXT("LoadCampaignButton"))) return true;
				CaptureSaveUI(Root, TEXT("ConfirmLoad"));
				OldUIRoot = TStrongObjectPtr<UUserWidget>(Root);
				BindReady(); PreviousWorld = World;
				if (Case == TEXT("Recovery"))
				{
					Definition = const_cast<URomanUnitDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindUnit(TEXT("Legionary")));
					if (!Test.TestTrue(TEXT("Recovery fixture has current content"), Definition.IsValid())) return true;
					OriginalClass = Definition->UnitClass;
					FailingWorldInitialized = FWorldDelegates::OnPostWorldInitialization.AddLambda([this](UWorld* Incoming, const UWorld::InitializationValues)
					{ if (Incoming && Incoming->WorldType == EWorldType::PIE && Definition.IsValid()) Definition->UnitClass.Reset(); });
				}
				if (!Click(TEXT("ConfirmSaveActionButton"))) return true;
				Test.TestTrue(TEXT("Confirmed physical load schedules actual reload"), Session->HasPendingTransfer());
				Test.TestFalse(TEXT("Pending load disables physical save control"), CastChecked<UButton>(Root->GetWidgetFromName(TEXT("SaveCampaignButton")))->GetIsEnabled());
				Stage = Case == TEXT("Recovery") ? 7 : 2; return false;
			}
			if (Stage == 3)
			{
				Test.TestEqual(TEXT("Loaded queued intent dispatches the same actual tactical encounter"), State->GetTacticalBattleResult().EncounterID, Expected.Campaign.Encounter.PendingPlan.EncounterID);
				Test.TestEqual(TEXT("Queued retry restores human defender role"), State->GetCampaignEncounterPlan().PlayerFaction, EFactionID::Rome);
				Test.TestEqual(TEXT("Tactical save is forbidden"), Save->SaveCampaign(), ECampaignSaveFailure::UnsafeBoundary);
				Test.TestEqual(TEXT("Tactical load is forbidden even with a valid disk file"), Save->LoadCampaign(), ECampaignSaveFailure::UnsafeBoundary);
				return true;
			}
			if (Stage == 1)
			{
				if (bUI && !bUIDetached)
				{
					Test.TestEqual(TEXT("Save UI binds one persistence listener"), ListenerCount(Test, Save, TEXT("OnSaveStateChanged"), Root), 1);
					Root->RemoveFromParent();
					Test.TestEqual(TEXT("Detach removes persistent GameInstance listener"), ListenerCount(Test, Save, TEXT("OnSaveStateChanged"), Root), 0);
					const bool Before = Save->GetSettings().bAutosaveEnabled;
					FCall(Test, Root, TEXT("HandleSaveCampaignClicked"), 0).Execute(); FCall(Test, Root, TEXT("HandleAutosaveToggleClicked"), 0).Execute();
					Test.TestEqual(TEXT("Detached callback cannot alter preferences"), Save->GetSettings().bAutosaveEnabled, Before);
					Test.TestFalse(TEXT("Detached callback cannot write disk"), Save->CanLoadCampaign());
					Root->AddToViewport(); bUIDetached = true; LayoutAt = FPlatformTime::Seconds(); return false;
				}
				Probe = TStrongObjectPtr<UCampaignSaveProbe>(NewObject<UCampaignSaveProbe>());
				Probe->Test = &Test; Probe->Save = Save;
				Save->OnSaveStateChanged.AddUniqueDynamic(Probe.Get(), &UCampaignSaveProbe::OnSave);
				Session->OnSessionChanged.AddUniqueDynamic(Probe.Get(), &UCampaignSaveProbe::OnSession);
				State->OnTurnStepStarted.AddUniqueDynamic(Probe.Get(), &UCampaignSaveProbe::OnStep);
				Test.TestTrue(TEXT("Strategic session has safe readonly boundary"), Save->CanSaveCampaign());
				if (Case == TEXT("QueuedAI"))
				{
					Target = const_cast<URomanTerritoryDefinition*>(URomanContentRegistrySubsystem::Get(World)->FindTerritory(TEXT("Rome")));
					if (!Test.TestTrue(TEXT("Queued save fixture has current target metadata"), Target.IsValid())) return true;
					OriginalMap = Target->TacticalMap; Probe->Target = Target; Probe->bBreakMap = true;
					FStructProperty* Property = FindFProperty<FStructProperty>(Manager->GetClass(), TEXT("ArmyState"));
					if (!Test.TestTrue(TEXT("Controlled fixture uses existing canonical army owner"), Property && Property->Struct == FCampaignArmyState::StaticStruct())) return true;
					auto* Roster = Property->ContainerPtrToValuePtr<FCampaignArmyState>(Manager);
					auto Armies = Roster->GetArmies();
					for (auto& Army : Armies) if (Army.Faction == EFactionID::Carthage) Army.Units[0].Count = 15;
					Test.TestTrue(TEXT("Fixture strengthens canonical enemy"), Roster->ReplaceFromSnapshot(Armies).WasSuccessful());
				}
				if (Case == TEXT("RoundTrip") || bUI)
				{
					UObject* Map = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetWorldMapManager"));
					FCall Own(Test, Map, TEXT("ChangeTerritoryOwner"), 3); Own.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Own.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Own.Execute();
					FCall Found(Test, Map, TEXT("FoundTerritorySettlement"), 3); Found.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Found.SetText(TEXT("Name"), FText::FromString(TEXT("Disk City"))); Found.Execute();
					if (bUI)
					{
						Test.TestEqual(TEXT("Reattach binds persistence listener exactly once"), ListenerCount(Test, Save, TEXT("OnSaveStateChanged"), Root), 1);
						if (!Click(TEXT("AutosaveToggleButton"))) return true;
						Test.TestFalse(TEXT("Physical toggle changes canonical preference"), Save->GetSettings().bAutosaveEnabled);
					}
					else Test.TestTrue(TEXT("Saved preferences can disable autosave"), Save->SetAutosaveEnabled(false));
				}
				if (bUI) { if (!Click(TEXT("SaveCampaignButton"))) return true; Test.TestEqual(TEXT("Physical first save writes fixed slot"), Save->GetLastFailure(), ECampaignSaveFailure::None); }
				else Test.TestEqual(TEXT("Manual save writes fixed platform slot"), Save->SaveCampaign(), ECampaignSaveFailure::None);
				if (!ReadLatest(Save, ECampaignSaveSlot::Manual, Expected, LatestIndex)) return true;
				Test.TestEqual(TEXT("First generation is one"), Expected.Generation, int64(1));
				if (bUI)
				{
					if (!Click(TEXT("SaveCampaignButton"))) return true;
					FCampaignSaveRecord Unchanged; int32 Index = -1; ReadLatest(Save, ECampaignSaveSlot::Manual, Unchanged, Index);
					Test.TestEqual(TEXT("Overwrite requires confirmation and does no early I/O"), Unchanged.Generation, int64(1));
					if (!Click(TEXT("CancelSaveActionButton")) || !Click(TEXT("SaveCampaignButton"))) return true;
					CaptureSaveUI(Root, TEXT("ConfirmOverwrite"));
					if (!Click(TEXT("ConfirmSaveActionButton"))) return true;
					if (!ReadLatest(Save, ECampaignSaveSlot::Manual, Expected, LatestIndex)) return true;
					Test.TestEqual(TEXT("Confirmed physical overwrite rotates generation"), Expected.Generation, int64(2));
				}
				const auto Result = Turn();
				Test.TestEqual(TEXT("Committed turn still completes all nine stages"), Result.CompletedSteps.Num(), 9);
				if (Case == TEXT("QueuedAI"))
				{
					Test.TestEqual(TEXT("Controlled post-turn dispatch failure preserves accepted intent"), Result.Failure, ECampaignTurnFailure::AITacticalTravelFailed);
					Target->TacticalMap = OriginalMap; Probe->bBreakMap = false;
				}
				else Test.TestTrue(TEXT("Autosave cannot duplicate turn stages"), Result.WasSuccessful());
				Test.TestEqual(TEXT("Exactly nine stage events"), Probe->TurnStages, 9);
				Test.TestEqual(TEXT("Current turn advanced exactly once"), State->GetCurrentTurn(), Expected.Campaign.Turn.CurrentTurn + 1);
				if (Case == TEXT("RoundTrip") || bUI)
				{
					Test.TestFalse(TEXT("Disabled autosave writes no slot"), UGameplayStatics::DoesSaveGameExist(Save->GetStorageSlotName(ECampaignSaveSlot::Autosave, 0), 0));
					Save->SetAutosaveEnabled(true); // Loaded saved false must replace this preference before readiness.
					if (bUI)
					{
						if (Case == TEXT("Resume")) { PreviousWorld = World; Stage = 6; UGameplayStatics::OpenLevel(World, TEXT("/Game/Maps/L_Testbed")); }
						else Stage = 5;
						return false;
					}
				}
				else
				{
					FCampaignSaveRecord Auto; int32 AutoIndex = -1;
					if (!ReadLatest(Save, ECampaignSaveSlot::Autosave, Auto, AutoIndex)) return true;
					Test.TestEqual(TEXT("Autosave captures post-increment turn"), Auto.Campaign.Turn.CurrentTurn, State->GetCurrentTurn());
					Test.TestEqual(TEXT("Autosave captures request marker"), Auto.Campaign.Turn.LastAutosaveRequestedTurn, State->GetCurrentTurn());
					const auto Armies = State->GetCampaignArmies();
					Test.TestTrue(TEXT("Autosave captures committed AI quantities and stable IDs"), SameArmies(Auto.Campaign.Armies, Armies));
					Test.TestEqual(TEXT("Autosave status is success"), Save->GetLastFailure(), ECampaignSaveFailure::None);
					if (Case == TEXT("IOFailure")) return ExerciseIO(Save, AutoIndex, Turn);
				}
				if (Case == TEXT("Backup") || Case == TEXT("Guards") || Case == TEXT("StorageConflicts"))
				{
					Test.TestEqual(TEXT("Second save rotates without overwriting backup"), Save->SaveCampaign(), ECampaignSaveFailure::None);
					FCampaignSaveRecord Latest; int32 Index = -1; if (!ReadLatest(Save, ECampaignSaveSlot::Manual, Latest, Index)) return true;
					Test.TestEqual(TEXT("Second generation increments"), Latest.Generation, int64(2));
					Test.TestTrue(TEXT("Both generation files exist"), UGameplayStatics::DoesSaveGameExist(Save->GetStorageSlotName(ECampaignSaveSlot::Manual, 0), 0) && UGameplayStatics::DoesSaveGameExist(Save->GetStorageSlotName(ECampaignSaveSlot::Manual, 1), 0));
					if (Case == TEXT("Guards")) return ExerciseGuards(Save, Session, State, Latest, Index);
					if (Case == TEXT("StorageConflicts")) return ExerciseStorageConflicts(Save, Session, Latest);
					TArray<uint8> Corrupt; UGameplayStatics::LoadDataFromSlot(Corrupt, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0);
					Corrupt.Last() ^= 0x55; Test.TestTrue(TEXT("Test corrupts only newest isolated generation"), UGameplayStatics::SaveDataToSlot(Corrupt, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0));
				}
				if (Case == TEXT("Autosave") || Case == TEXT("QueuedAI"))
				{ if (!ReadLatest(Save, ECampaignSaveSlot::Autosave, Expected, LatestIndex)) return true; Test.TestEqual(TEXT("Autosave is independent from manual turn one"), Save->GetLastSavedTurn(), 2); }
				if (Case == TEXT("QueuedAI")) Test.TestTrue(TEXT("Disk autosave retains accepted post-turn intent"), Expected.Campaign.Turn.bAwaitingAITacticalTravel && Expected.Campaign.Encounter.PendingPlan.EncounterID.IsValid());
				BindReady();
				PreviousWorld = World;
				const auto Slot = Case == TEXT("Autosave") || Case == TEXT("QueuedAI") ? ECampaignSaveSlot::Autosave : ECampaignSaveSlot::Manual;
				Test.TestEqual(TEXT("Validated load schedules actual strategic reload"), Save->LoadCampaign(Slot), ECampaignSaveFailure::None);
				Test.TestEqual(TEXT("Backup recovery surfaced explicitly"), Save->WasBackupRecovered(), Case == TEXT("Backup"));
				Test.TestTrue(TEXT("Load is staged, never resets live source in place"), Session->HasPendingTransfer());
				Test.TestEqual(TEXT("Repeated pending load is rejected"), Save->LoadCampaign(Slot), ECampaignSaveFailure::Busy);
				Stage = 2; return false;
			}
			Test.TestEqual(TEXT("First readiness validated restored fields and preferences"), Probe->ReadyNotifications, 1);
			Test.TestTrue(TEXT("Loaded session is strategic and ready"), Session->HasSession() && Session->GetPhase() == ECampaignTransferPhase::Strategic);
			if (bUI)
			{
				Test.TestEqual(TEXT("Old world root loses persistent listener during real load"), ListenerCount(Test, Save, TEXT("OnSaveStateChanged"), OldUIRoot.Get()), 0);
				Test.TestEqual(TEXT("New world root owns exactly one persistence listener"), ListenerCount(Test, Save, TEXT("OnSaveStateChanged"), Root), 1);
				const bool Before = Save->GetSettings().bAutosaveEnabled;
				FCall(Test, OldUIRoot.Get(), TEXT("HandleAutosaveToggleClicked"), 0).Execute(); FCall(Test, OldUIRoot.Get(), TEXT("HandleLoadCampaignClicked"), 0).Execute();
				Test.TestEqual(TEXT("Stale-world callback cannot change loaded preference"), Save->GetSettings().bAutosaveEnabled, Before);
				Test.TestFalse(TEXT("Stale-world callback cannot queue another load"), Session->HasPendingTransfer());
				Test.TestEqual(TEXT("Loaded preference is visible in native control"), CastChecked<UTextBlock>(CastChecked<UButton>(Root->GetWidgetFromName(TEXT("AutosaveToggleButton")))->GetContent())->GetText().ToString(), FString(TEXT("AUTO OFF")));
				CaptureSaveUI(Root, TEXT("Loaded"));
			}
			Test.TestEqual(TEXT("Loaded canonical campaign can be saved again"), Save->SaveCampaign(), ECampaignSaveFailure::None);
			FCampaignSaveRecord Actual; int32 Index = -1; if (!ReadLatest(Save, ECampaignSaveSlot::Manual, Actual, Index)) return true;
			Test.TestTrue(TEXT("Disk save/advance/load restores EVERY campaign value, RNG seeds, ledgers and stable IDs exactly"), FCampaignTransferSnapshot::StaticStruct()->CompareScriptStruct(&Actual.Campaign, &Expected.Campaign, 0));
			Test.TestEqual(TEXT("Loaded autosave preference is exact"), Actual.Settings.bAutosaveEnabled, Expected.Settings.bAutosaveEnabled);
			Test.TestTrue(TEXT("Notifications are emitted and prevent nested persistence"), Probe->SaveNotifications > 3);
			if (Case == TEXT("QueuedAI"))
			{
				PreviousWorld = World;
				const auto Retry = Turn();
				Test.TestTrue(TEXT("Loaded intent retries dispatch, never another turn"), Retry.WasSuccessful() && Retry.bWasAITacticalTravelRetry && Retry.CompletedSteps.IsEmpty());
				Test.TestEqual(TEXT("Load/retry has no production, spending or stage replay"), Probe->TurnStages, 9);
				Stage = 3; return false;
			}
			return true;
		}
	private:
		void BindReady()
		{
			Probe->Expected = Expected;
			WorldInitialized = FWorldDelegates::OnPostWorldInitialization.AddLambda([this](UWorld* Incoming, const UWorld::InitializationValues)
			{
				if (!Incoming || Incoming->WorldType != EWorldType::PIE) return;
				Incoming->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([Weak = TWeakObjectPtr<UCampaignSaveProbe>(Probe.Get())](AActor* Actor)
				{
					if (auto* IncomingState = Cast<ARomanEmpireGameState>(Actor); IncomingState && Weak.IsValid())
					{ Weak->State = IncomingState; IncomingState->OnGameplayInitialized.AddUniqueDynamic(Weak.Get(), &UCampaignSaveProbe::OnReady); }
				}));
			});
		}
		void CaptureSaveUI(UUserWidget* Root, const TCHAR* Name)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureCampaignSave"))) return;
			TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Actual save controls render"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/C016C_Visual")); IFileManager::Get().MakeDirectory(*Directory, true);
			Test.TestTrue(TEXT("Save UI capture written"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory, Case + Name + TEXT(".png"))));
		}
		static bool SameArmies(const TArray<FCampaignArmy>& A, const TArray<FCampaignArmy>& B)
		{
			if (A.Num() != B.Num()) return false;
			for (int32 I = 0; I < A.Num(); ++I) if (!FCampaignArmy::StaticStruct()->CompareScriptStruct(&A[I], &B[I], 0)) return false;
			return true;
		}
		bool ReadLatest(UCampaignSaveSubsystem* Save, ECampaignSaveSlot Slot, FCampaignSaveRecord& Out, int32& Index)
		{
			bool Found = false;
			for (int32 I = 0; I < 2; ++I)
			{
				TArray<uint8> Bytes; FCampaignSaveRecord Record;
				if (UGameplayStatics::DoesSaveGameExist(Save->GetStorageSlotName(Slot, I), 0) && UGameplayStatics::LoadDataFromSlot(Bytes, Save->GetStorageSlotName(Slot, I), 0) && RomanCampaignSave::Decode(Bytes, Record) == ECampaignSaveFailure::None && (!Found || Record.Generation > Out.Generation))
				{ Found = true; Out = MoveTemp(Record); Index = I; }
			}
			return Test.TestTrue(TEXT("Platform disk file decodes with current bounded envelope"), Found);
		}
		bool ExerciseGuards(UCampaignSaveSubsystem* Save, UCampaignSessionSubsystem* Session, ARomanEmpireGameState* State, FCampaignSaveRecord Latest, int32 Index)
		{
			const int32 Turn = State->GetCurrentTurn(); const auto Armies = State->GetCampaignArmies(); const auto Wallet = State->GetFactionResources(EFactionID::Rome);
			const auto Reject = [&](ECampaignSaveFailure Failure)
			{
				Test.TestEqual(TEXT("Invalid disk record is explicitly rejected"), Save->LoadCampaign(), Failure);
				Test.TestFalse(TEXT("Rejected load never freezes or queues source"), Session->HasPendingTransfer());
				Test.TestEqual(TEXT("Rejected load leaves turn unchanged"), State->GetCurrentTurn(), Turn);
				Test.TestTrue(TEXT("Rejected load leaves canonical army state unchanged"), SameArmies(State->GetCampaignArmies(), Armies));
				const auto Current = State->GetFactionResources(EFactionID::Rome); Test.TestTrue(TEXT("Rejected load leaves six-currency wallet unchanged"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &Current, 0));
			};
			TArray<uint8> Bytes; RomanCampaignSave::Encode(Latest, Bytes); Bytes[4] = 2;
			UGameplayStatics::SaveDataToSlot(Bytes, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0);
			Reject(ECampaignSaveFailure::UnsupportedVersion); // Older good backup MUST NOT hide a newer schema.
			Latest.Campaign.Armies[0].Units[0].UnitID = TEXT("MissingSavedDefinition"); RomanCampaignSave::Encode(Latest, Bytes);
			UGameplayStatics::SaveDataToSlot(Bytes, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0);
			Reject(ECampaignSaveFailure::InvalidContent);
			Latest = Expected; Latest.Generation = 2; Latest.Campaign.DestinationMap = FSoftObjectPath(TEXT("/Game/Maps/L_MissingSaveWorld.L_MissingSaveWorld")); RomanCampaignSave::Encode(Latest, Bytes);
			UGameplayStatics::SaveDataToSlot(Bytes, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0);
			Reject(ECampaignSaveFailure::LoadRejected);
			Bytes.Last() ^= 0x55; for (int32 I = 0; I < 2; ++I) UGameplayStatics::SaveDataToSlot(Bytes, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, I), 0);
			Reject(ECampaignSaveFailure::CorruptData);
			RomanCampaignSave::Encode(Expected, Bytes); UGameplayStatics::SaveDataToSlot(Bytes, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, 0), 0);
			Test.TestEqual(TEXT("Corrupt inactive generation can be repaired without losing good backup"), Save->SaveCampaign(), ECampaignSaveFailure::None);
			return true;
		}
		bool ExerciseStorageConflicts(UCampaignSaveSubsystem* Save, UCampaignSessionSubsystem* Session, FCampaignSaveRecord Latest)
		{
			// Use the fixture's process/GUID-isolated real platform slots, never production saves.
			// Capture all saved domains through the public API in the OTHER isolated slot.
			// This also proves successful operations still work after each rejection, without
			// exposing runtime-private manager headers or writing either conflicted file.
			const auto CaptureCampaign = [&](FCampaignTransferSnapshot& Out)
			{
				if (!Test.TestEqual(TEXT("Independent slot can capture the live campaign"),
					Save->SaveCampaign(ECampaignSaveSlot::Autosave), ECampaignSaveFailure::None)) return false;
				FCampaignSaveRecord Record; int32 Index = -1;
				if (!ReadLatest(Save, ECampaignSaveSlot::Autosave, Record, Index)) return false;
				Out = MoveTemp(Record.Campaign); return true;
			};
			FCampaignTransferSnapshot Before;
			if (!CaptureCampaign(Before)) return true;
			const auto ReadSlot = [&](int32 Index, TArray<uint8>& Out)
			{
				return Test.TestTrue(TEXT("Read isolated platform generation"), UGameplayStatics::LoadDataFromSlot(
					Out, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0));
			};
			const auto WriteSlot = [&](int32 Index, const TArray<uint8>& Bytes)
			{
				return Test.TestTrue(TEXT("Write isolated platform fixture generation"), UGameplayStatics::SaveDataToSlot(
					Bytes, Save->GetStorageSlotName(ECampaignSaveSlot::Manual, Index), 0));
			};
			const auto RejectWithoutMutation = [&](ECampaignSaveFailure Failure, bool bCheckLoad = true)
			{
				TArray<uint8> Original[2];
				if (!ReadSlot(0, Original[0]) || !ReadSlot(1, Original[1])) return;
				if (bCheckLoad) Test.TestEqual(TEXT("Ambiguous/incompatible load is rejected"), Save->LoadCampaign(), Failure);
				Test.TestEqual(TEXT("Save refuses to overwrite unsafe generations"), Save->SaveCampaign(), Failure);
				Test.TestEqual(TEXT("Rejection is observable"), Save->GetLastFailure(), Failure);
				Test.TestFalse(TEXT("Rejected storage cannot queue world travel"), Session->HasPendingTransfer());
				Test.TestFalse(TEXT("Rejection never claims successful backup recovery"), Save->WasBackupRecovered());
				Test.TestTrue(TEXT("Rejected operation releases its busy guard"), Save->CanSaveCampaign());
				for (int32 Index = 0; Index < 2; ++Index)
				{
					TArray<uint8> After;
					if (ReadSlot(Index, After)) Test.TestTrue(TEXT("Rejected save/load preserves BOTH generation files byte-for-byte"), After == Original[Index]);
				}
				FCampaignTransferSnapshot After;
				if (CaptureCampaign(After))
					Test.TestTrue(TEXT("Rejected disk operation preserves EVERY canonical campaign value"),
						FCampaignTransferSnapshot::StaticStruct()->CompareScriptStruct(&Before, &After, 0));
			};
			TArray<uint8> Original, Conflicting;
			if (!Test.TestEqual(TEXT("Encode valid generation fixture"), RomanCampaignSave::Encode(Latest, Original), ECampaignSaveFailure::None)) return true;
			auto Other = Latest; Other.Settings.bAutosaveEnabled = !Other.Settings.bAutosaveEnabled;
			if (!Test.TestEqual(TEXT("Encode same-generation different valid record"), RomanCampaignSave::Encode(Other, Conflicting), ECampaignSaveFailure::None)) return true;
			Test.TestTrue(TEXT("Conflict fixture has genuinely different valid bytes"), Original != Conflicting);
			if (!WriteSlot(0, Original) || !WriteSlot(1, Conflicting)) return true;
			RejectWithoutMutation(ECampaignSaveFailure::CorruptData);
			// Matching duplicate generations are safe; rejecting all equal numbers is not a fix.
			if (!WriteSlot(1, Original)) return true;
			Test.TestEqual(TEXT("Identical duplicate generations allow the next save"), Save->SaveCampaign(), ECampaignSaveFailure::None);
			TArray<uint8> Retained, Advanced; FCampaignSaveRecord Next;
			if (ReadSlot(0, Retained)) Test.TestTrue(TEXT("Successful rotation retains the previous good generation"), Retained == Original);
			if (ReadSlot(1, Advanced) && Test.TestEqual(TEXT("Advanced platform generation decodes"), RomanCampaignSave::Decode(Advanced, Next), ECampaignSaveFailure::None))
				Test.TestEqual(TEXT("Duplicate generation advances exactly once"), Next.Generation, Latest.Generation + 1);
			// Either physical slot can hold an unsupported schema; a valid neighbor must not hide it.
			for (int32 UnsupportedIndex = 0; UnsupportedIndex < 2; ++UnsupportedIndex)
			{
				for (uint8 Version : {uint8(0), uint8(RomanCampaignSave::CurrentVersion + 1)})
				{
					auto Unsupported = Original; Unsupported[4] = Version;
					if (!WriteSlot(0, Original) || !WriteSlot(1, Original) || !WriteSlot(UnsupportedIndex, Unsupported)) return true;
					RejectWithoutMutation(ECampaignSaveFailure::UnsupportedVersion);
				}
			}
			Latest.Generation = MAX_int64 - 1;
			if (!Test.TestEqual(TEXT("Last readable generation encodes"), RomanCampaignSave::Encode(Latest, Original), ECampaignSaveFailure::None)) return true;
			if (!WriteSlot(0, Original) || !WriteSlot(1, Original)) return true;
			RejectWithoutMutation(ECampaignSaveFailure::InvalidCampaign, false); // Do not queue a valid load in this no-travel guard fixture.
			return true;
		}
		bool ExerciseIO(UCampaignSaveSubsystem* Save, int32 GoodIndex, TFunctionRef<FCampaignTurnResult()> Turn)
		{
			TArray<uint8> Original; UGameplayStatics::LoadDataFromSlot(Original, Save->GetStorageSlotName(ECampaignSaveSlot::Autosave, GoodIndex), 0);
			const FString Name = Save->GetStorageSlotName(ECampaignSaveSlot::Autosave, 1 - GoodIndex);
			if (!Test.TestTrue(TEXT("I/O fixture target is isolated"), Name.StartsWith(TEXT("RomanEmpire_Automation_")))) return true;
			const FString Blocker = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), Name + TEXT(".sav"));
			if (!Test.TestTrue(TEXT("Empty test directory blocks ONLY the inactive test slot"), IFileManager::Get().MakeDirectory(*Blocker, false))) return true;
			ON_SCOPE_EXIT { IFileManager::Get().DeleteDirectory(*Blocker, false, false); }; // Exact newly created empty test directory; no recursive deletion.
			const auto Result = Turn();
			Test.TestTrue(TEXT("Autosave I/O failure never rolls back or repeats committed turn"), Result.WasSuccessful() && Result.CompletedSteps.Num() == 9 && Result.CompletedTurn == 3);
			Test.TestEqual(TEXT("I/O failure is explicitly observable"), Save->GetLastFailure(), ECampaignSaveFailure::IOError);
			Test.TestEqual(TEXT("Failed write never publishes success turn"), Save->GetLastSavedTurn(), 2);
			TArray<uint8> Preserved; UGameplayStatics::LoadDataFromSlot(Preserved, Save->GetStorageSlotName(ECampaignSaveSlot::Autosave, GoodIndex), 0);
			Test.TestTrue(TEXT("Previous verified generation survives failed write byte-for-byte"), Preserved == Original);
			Test.TestEqual(TEXT("Two committed turns emitted exactly eighteen stages"), Probe->TurnStages, 18);
			return true;
		}
		FAutomationTestBase& Test; FString Case; double StartedAt; int32 Stage = 0, LatestIndex = -1;
		TWeakObjectPtr<UWorld> PreviousWorld; TWeakObjectPtr<UCampaignSaveSubsystem> Identity;
		TStrongObjectPtr<UCampaignSaveProbe> Probe; FCampaignSaveRecord Expected; FDelegateHandle WorldInitialized;
		TWeakObjectPtr<URomanTerritoryDefinition> Target; TSoftObjectPtr<UWorld> OriginalMap;
		TStrongObjectPtr<UUserWidget> OldUIRoot; bool bUIDetached = false;
		TWeakObjectPtr<UWorld> LayoutWorld; int32 LayoutStage = -1; double LayoutAt = 0;
		TWeakObjectPtr<URomanUnitDefinition> Definition; TSoftClassPtr<AUnitBase> OriginalClass; FDelegateHandle FailingWorldInitialized;
	};

	class FExerciseCampaignRecovery : public IAutomationLatentCommand
	{
	public:
		FExerciseCampaignRecovery(FAutomationTestBase* InTest, bool bInWrongNonce, bool bInUI = false)
			: Test(*InTest), bWrongNonce(bInWrongNonce), bUI(bInUI), StartedAt(FPlatformTime::Seconds()) {}
		~FExerciseCampaignRecovery()
		{
			FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitialized);
			if (Definition.IsValid()) Definition->UnitClass = OriginalClass;
		}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 50.0) { Test.AddError(TEXT("Campaign recovery travel timed out")); return true; }
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (!World || !World->GetGameInstance()) return false;
			UCampaignSessionSubsystem* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			ARomanEmpireGameState* State = World->GetGameState<ARomanEmpireGameState>();
			if (!Session || !State) return false;
			if (Stage == 2)
			{
				if (World == PreviousWorld.Get() || Session->GetLastFailure() != ECampaignSessionFailure::RestoreFailed) return false;
				Test.TestFalse(TEXT("Rejected incoming world never publishes default readiness"), State->IsGameplayInitialized());
				Test.TestTrue(TEXT("Rejected import retains the only campaign capsule"), Session->HasPendingTransfer());
				Test.TestEqual(TEXT("Rejected import reports its actual cause"), Session->GetLastTransferFailure(),
					bWrongNonce ? ECampaignTransferFailure::InvalidMapReference : ECampaignTransferFailure::InvalidContent);
				Test.TestTrue(TEXT("Quarantined public facade exposes no default armies"), State->GetCampaignArmies().IsEmpty());
				Test.TestFalse(TEXT("Quarantined world has no sandbox/mapped encounter"), World->GetSubsystem<UTacticalBattleSubsystem>()->HasEncounter());
				AActor* Producer = nullptr;
				for (TActorIterator<AActor> It(World); It; ++It) if (It->FindFunction(TEXT("TrainUnit"))) Producer = *It;
				FCall Train(Test, Producer, TEXT("TrainUnit"), 2); Train.SetName(TEXT("UnitID"), TEXT("Legionary")); Train.Execute();
				Test.TestFalse(TEXT("Quarantine cannot create abandoned paid orders"), Train.ReadBool());
				if (Definition.IsValid()) Definition->UnitClass = OriginalClass;
				FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitialized); WorldInitialized.Reset();
				if (bUI)
				{
					UUserWidget* Root = Cast<UUserWidget>(ReadObject(Test, World->GetFirstPlayerController(), TEXT("GetMainWidget")));
					if (!Root) return false;
					if (!bDetached) { Root->RemoveFromParent(); bDetached = true; return false; }
					if (!bReattached)
					{
					Test.TestEqual(TEXT("Not-ready detached HUD removes GameInstance listener"), ListenerCount(Test, Session, TEXT("OnSessionChanged"), Root), 0);
					FCall(Test, Root, TEXT("HandleRetryCampaignClicked"), 0).Execute();
					Test.TestEqual(TEXT("Detached retry cannot schedule another world"), Session->GetLastFailure(), ECampaignSessionFailure::RestoreFailed);
					Root->AddToViewport();
					bReattached = true; return false;
					}
					Test.TestEqual(TEXT("Not-ready reattachment has one session listener"), ListenerCount(Test, Session, TEXT("OnSessionChanged"), Root), 1);
					Test.TestTrue(TEXT("Recovery panel is visible without gameplay readiness"), Root->GetWidgetFromName(TEXT("CampaignRecoveryPanel"))->IsVisible());
					auto* Retry = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("RetryCampaignButton")));
					Test.TestTrue(TEXT("Recovery button can act without gameplay readiness"), Retry->GetIsEnabled());
					if (!ClickThroughSlate(Test, Root, Retry)) return true;
					Test.TestFalse(TEXT("Retry disabled while replacement world is queued"), Retry->GetIsEnabled());
				}
				else Test.TestEqual(TEXT("Retry uses retained capsule through actual OpenLevel"), Session->RetryPendingTransfer(), ECampaignSessionFailure::None);
				Test.TestEqual(TEXT("Repeated retry cannot schedule another world"), Session->RetryPendingTransfer(), ECampaignSessionFailure::WrongPhase);
				PreviousWorld = World; Stage = 3; return false;
			}
			if (!State->IsGameplayInitialized() || (Stage > 0 && (Session->HasPendingTransfer() || World == PreviousWorld.Get()))) return false;
			if (Stage == 0)
			{
				for (const auto& Army : State->GetCampaignArmies()) if (Army.Faction == EFactionID::Rome) Attacker = Army.ArmyID;
				Test.TestEqual(TEXT("Retry is unavailable outside failed transfer"), Session->RetryPendingTransfer(), ECampaignSessionFailure::WrongPhase);
				Test.TestEqual(TEXT("Recovery fixture enters actual strategic session"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::None);
				PreviousWorld = World; Stage = 1; return false;
			}
			if (Stage == 1)
			{
				const auto* Registry = URomanContentRegistrySubsystem::Get(Session);
				Definition = const_cast<URomanUnitDefinition*>(Registry->FindUnit(TEXT("Legionary")));
				OriginalClass = Definition->UnitClass;
				if (!bWrongNonce)
					WorldInitialized = FWorldDelegates::OnPostWorldInitialization.AddLambda([this](UWorld* Incoming, const UWorld::InitializationValues)
					{
						if (Incoming && Incoming->WorldType == EWorldType::PIE && Definition.IsValid()) Definition->UnitClass.Reset();
					});
				Test.TestEqual(TEXT("Prepared valid encounter begins travel"), Session->BeginTacticalEncounter(Attacker, TEXT("Africa")), ECampaignSessionFailure::None);
				if (bWrongNonce)
					UGameplayStatics::OpenLevel(World, TEXT("/Game/Maps/L_Testbed"), true, TEXT("RomanTransfer=not-the-issued-nonce"));
				PreviousWorld = World; Stage = 2; return false;
			}
			Test.TestTrue(TEXT("Recovered capsule consumed exactly once"), Session->HasSession() && !Session->HasPendingTransfer());
			Test.TestEqual(TEXT("Recovery loads intended tactical phase"), Session->GetPhase(), ECampaignTransferPhase::TacticalDeployment);
			FCampaignArmy Army;
			Test.TestTrue(TEXT("Recovery retains source stable army identity"), State->TryGetCampaignArmy(Attacker, Army));
			Test.TestEqual(TEXT("Recovery retains original counts, no discarded/default capsule"), Army.Units[0].Count, 15);
			const auto* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			Test.TestEqual(TEXT("Recovery materializes exact transferred plan"), Battle->GetResult().EncounterID, State->GetCampaignEncounterPlan().EncounterID);
			Test.TestEqual(TEXT("Recovery has actual two deployed squads"), Battle->GetResult().Squads.Num(), 2);
			Test.TestEqual(TEXT("Consumed capsule cannot be retried"), Session->RetryPendingTransfer(), ECampaignSessionFailure::WrongPhase);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		bool bWrongNonce, bUI;
		bool bDetached = false, bReattached = false;
		double StartedAt;
		int32 Stage = 0;
		FGuid Attacker;
		TWeakObjectPtr<UWorld> PreviousWorld;
		TWeakObjectPtr<URomanUnitDefinition> Definition;
		TSoftClassPtr<AUnitBase> OriginalClass;
		FDelegateHandle WorldInitialized;
	};

	class FExerciseTacticalTerminal : public IAutomationLatentCommand
	{
	public:
		FExerciseTacticalTerminal(FAutomationTestBase* InTest, bool bInVictory)
			: Test(*InTest), bVictory(bInVictory), StartedAt(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 40.0)
			{
				Test.AddError(TEXT("Tactical terminal/restart PIE timed out"));
				return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized() || !World->GetFirstPlayerController()) return false;
			APlayerController* Controller = World->GetFirstPlayerController();
			UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			UUserWidget* Root = Cast<UUserWidget>(ReadObject(Test, Controller, TEXT("GetMainWidget")));
			if (!Test.TestNotNull(TEXT("Terminal fixture has canonical battle"), Battle) ||
				!Test.TestNotNull(TEXT("Terminal fixture has native HUD"), Root)) return true;
			if (Stage == 0)
			{
				OriginalWorld = World;
				OriginalCamera = Controller->GetPawn();
				MinPitch = Controller->PlayerCameraManager->ViewPitchMin;
				MaxPitch = Controller->PlayerCameraManager->ViewPitchMax;
				Test.TestFalse(TEXT("Unresolved encounter cannot restart"), ReadBool(Test, Controller, TEXT("RestartEncounter")));
				TArray<AActor*> Romans, Enemies;
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (It->FindFunction(TEXT("GetOwnerFaction")) && It->FindFunction(TEXT("CommandHold")))
					{
						FCall(Test, *It, TEXT("CommandHold"), 0).Execute();
						FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1); Owner.Execute();
						if (Owner.ReadEnum<EFactionID>() == EFactionID::Rome) Romans.Add(*It);
						else Enemies.Add(*It);
					}
					if (It->FindFunction(TEXT("TrainUnit"))) Producer = *It;
				}
				if (!Test.TestTrue(TEXT("Actual testbed contains initial 15/10 forces and barracks"),
					Romans.Num() == 15 && Enemies.Num() == 10 && Producer.IsValid())) return true;
				FCall Train(Test, Producer.Get(), TEXT("TrainUnit"), 2);
				Train.SetName(TEXT("UnitID"), TEXT("Legionary")); Train.Execute();
				Test.TestTrue(TEXT("Paid training starts before tactical resolution"), Train.ReadBool());
				Soldier = Romans[0];
				Survivor = bVictory ? Romans[0] : Enemies[0];
				if (!bVictory)
					for (int32 Index = 1; Index < Romans.Num(); ++Index) Kill(Romans[Index]);
				FCall Select(Test, Controller, TEXT("SelectUnit"), 1);
				Select.SetObject(TEXT("Unit"), Soldier.Get()); Select.Execute();
				FCall Possess(Test, Controller, TEXT("EnterFirstPersonMode"), 1);
				Possess.SetObject(TEXT("UnitToPossess"), Soldier.Get()); Possess.Execute();
				Test.TestTrue(TEXT("Terminal occurs during actual soldier possession"), Controller->GetPawn() == Soldier.Get() && ReadBool(Test, Controller, TEXT("IsInFirstPersonMode")));
				FCall(Test, Soldier.Get(), TEXT("StartBlocking"), 0).Execute();
				Test.TestTrue(TEXT("Possessed soldier blocks before terminal cleanup"), ReadBool(Test, Soldier.Get(), TEXT("IsBlocking")));
				if (bVictory)
				{
					ATacticalObjective* Objective = Battle->GetObjectiveActor();
					if (!Test.TestNotNull(TEXT("Victory uses physical registered camp"), Objective)) return true;
					Soldier->SetActorLocation(Objective->GetActorLocation() + FVector(0, 550, 96), false, nullptr, ETeleportType::TeleportPhysics);
					Test.TestTrue(TEXT("Possessed capsule actually occupies capture box"), Objective->IsOverlappingActor(Soldier.Get()));
					for (AActor* Enemy : Enemies) Kill(Enemy);
				}
				else Kill(Soldier.Get());
				Stage = 1;
				return false;
			}
			if (Stage == 1)
			{
				if (!Battle->IsResolved()) return false;
				Test.TestEqual(TEXT("Canonical terminal outcome matches fixture"), Battle->GetResult().Outcome,
					bVictory ? ETacticalBattleOutcome::PlayerVictory : ETacticalBattleOutcome::PlayerDefeat);
				Test.TestTrue(TEXT("Terminal restores the original camera pawn"), Controller->GetPawn() == OriginalCamera.Get());
				Test.TestFalse(TEXT("Terminal leaves no possessed mode"), ReadBool(Test, Controller, TEXT("IsInFirstPersonMode")));
				Test.TestFalse(TEXT("Terminal releases block"), ReadBool(Test, Soldier.Get(), TEXT("IsBlocking")));
				Test.TestFalse(TEXT("Terminal clears soldier possession flag"), ReadBool(Test, Soldier.Get(), TEXT("IsPossessedByPlayer")));
				Test.TestEqual(TEXT("Terminal restores minimum camera pitch"), Controller->PlayerCameraManager->ViewPitchMin, MinPitch);
				Test.TestEqual(TEXT("Terminal restores maximum camera pitch"), Controller->PlayerCameraManager->ViewPitchMax, MaxPitch);
				Test.TestFalse(TEXT("Terminal rejects tactical commands"), ReadBool(Test, Controller, TEXT("CanIssueTacticalCommands")));
				Test.TestFalse(TEXT("Terminal rejects campaign turn shortcut"), ReadBool(Test, Controller, TEXT("RequestEndTurn")));
				FCall Turn(Test, World->GetAuthGameMode(), TEXT("EndTurn"), 1); Turn.Execute();
				const FCampaignTurnResult TurnResult = Turn.ReadStruct<FCampaignTurnResult>();
				Test.TestEqual(TEXT("Rules facade cannot bypass canonical terminal turn guard"), TurnResult.Failure, ECampaignTurnFailure::EncounterResolved);
				Test.TestEqual(TEXT("Rejected terminal turn does not advance"), TurnResult.CompletedTurn, TurnResult.StartingTurn);
				Test.TestTrue(TEXT("Rejected terminal turn performs no economy or AI steps"), TurnResult.CompletedSteps.IsEmpty());
				Test.TestTrue(TEXT("Terminal restores camera mode and its authored input context"), State->GetCurrentGameplayMode() != EGameplayMode::Possessed && ReadObject(Test, Controller, TEXT("GetActiveGameplayContext")) != nullptr);
				Test.TestEqual(TEXT("Controller has one canonical battle listener"), ListenerCount(Test, State, TEXT("OnTacticalBattleChanged"), Controller), 1);
				Test.TestEqual(TEXT("HUD has one canonical battle listener"), ListenerCount(Test, State, TEXT("OnTacticalBattleChanged"), Root), 1);
				const int32 Health = ReadInt(Test, Survivor.Get(), TEXT("GetCurrentHealth"));
				Kill(Survivor.Get());
				FCall Damage(Test, Survivor.Get(), TEXT("TakeCombatDamage"), 3);
				Damage.SetFloat(TEXT("Damage"), 10000.0f); Damage.SetObject(TEXT("DamageSource"), nullptr); Damage.SetBool(TEXT("bIsRanged"), false); Damage.Execute();
				Test.TestEqual(TEXT("Terminal rejects subsequent health and damage mutations"), ReadInt(Test, Survivor.Get(), TEXT("GetCurrentHealth")), Health);
				Test.TestTrue(TEXT("Surviving force stays alive, not a terminal casualty"), ReadBool(Test, Survivor.Get(), TEXT("IsAlive")));
				Test.TestFalse(TEXT("Surviving force cannot accept commands"), ReadBool(Test, Survivor.Get(), TEXT("CanAcceptCommands")));
				Test.TestFalse(TEXT("Tracked survivor tick is quiescent"), Survivor->IsActorTickEnabled());
				FCall Train(Test, Producer.Get(), TEXT("TrainUnit"), 2);
				Train.SetName(TEXT("UnitID"), TEXT("Legionary")); Train.Execute();
				Test.TestFalse(TEXT("Terminal cannot charge a new training order"), Train.ReadBool());
				Producer->Tick(1000.0f);
				Test.TestTrue(TEXT("Terminal production tick retains paid order without spawning"), ReadBool(Test, Producer.Get(), TEXT("IsTraining")));
				Test.TestEqual(TEXT("Terminal battle does not acquire production reinforcement"), Battle->GetResult().Squads.Num(), 2);
				FCall BuildingDamage(Test, Producer.Get(), TEXT("ApplyBuildingDamage"), 2);
				BuildingDamage.SetFloat(TEXT("Damage"), 100000.0f); BuildingDamage.SetObject(TEXT("DamageSource"), nullptr); BuildingDamage.Execute();
				Test.TestTrue(TEXT("Terminal protects the retained producer from combat destruction"), ReadBool(Test, Producer.Get(), TEXT("IsComplete")));
				UTextBlock* Outcome = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("OutcomeText")));
				Test.TestTrue(TEXT("Event-driven outcome shows current tactical terminal"), Outcome && Outcome->GetText().ToString().Contains(bVictory ? TEXT("TACTICAL VICTORY") : TEXT("TACTICAL DEFEAT")));
				Root->RemoveFromParent();
				Stage = 2;
				return false;
			}
			if (Stage == 2)
			{
				Test.TestEqual(TEXT("Detached HUD unbinds battle events"), ListenerCount(Test, State, TEXT("OnTacticalBattleChanged"), Root), 0);
				UButton* Restart = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("RestartEncounterButton")));
				Test.TestFalse(TEXT("Detached restart control is disabled"), Restart->GetIsEnabled());
				FCall(Test, Root, TEXT("HandleRestartEncounterClicked"), 0).Execute();
				Test.TestTrue(TEXT("Detached callback cannot schedule travel"), ReadBool(Test, Controller, TEXT("CanRestartEncounter")));
				Root->AddToViewport();
				ReattachedAt = FPlatformTime::Seconds();
				Stage = 3;
				return false;
			}
			if (Stage == 3)
			{
				if (FPlatformTime::Seconds() - ReattachedAt < 0.2) return false;
				UButton* Restart = CastChecked<UButton>(Root->GetWidgetFromName(TEXT("RestartEncounterButton")));
				Test.TestEqual(TEXT("Reattached HUD binds once and catches up"), ListenerCount(Test, State, TEXT("OnTacticalBattleChanged"), Root), 1);
				Test.TestTrue(TEXT("Current terminal result permits restart"), Restart->GetIsEnabled());
				Root->ForceLayoutPrepass();
				Test.TestFalse(TEXT("Restart is a mouse HUD button without keyboard focus"), Restart->GetCachedWidget()->SupportsKeyboardFocus());
				CaptureTerminal(Root);
				if (!ClickThroughSlate(Test, Root, Restart)) return true;
				Test.TestFalse(TEXT("Travel pending disables restart UI immediately"), Restart->GetIsEnabled());
				Test.TestFalse(TEXT("Repeated command cannot schedule second restart"), ReadBool(Test, Controller, TEXT("RestartEncounter")));
				Stage = 4;
				return false;
			}
			if (World == OriginalWorld.Get()) return false;
			Test.TestFalse(TEXT("Actual OpenLevel creates an unresolved fresh encounter"), Battle->IsResolved());
			const FTacticalBattleResult Fresh = Battle->GetResult();
			Test.TestEqual(TEXT("Fresh encounter has exactly two initial squads"), Fresh.Squads.Num(), 2);
			int32 Rome = 0, Carthage = 0;
			for (const FTacticalBattleSquadResult& Squad : Fresh.Squads)
			{
				if (Squad.Faction == EFactionID::Rome) Rome += Squad.Survivors;
				else if (Squad.Faction == EFactionID::Carthage) Carthage += Squad.Survivors;
				Test.TestEqual(TEXT("Reloaded squad has no carried casualty"), Squad.Casualties, 0);
			}
			Test.TestEqual(TEXT("Real restart restores fifteen Romans"), Rome, 15);
			Test.TestEqual(TEXT("Real restart restores ten Carthaginians"), Carthage, 10);
			Test.TestNotNull(TEXT("Real restart registers its new physical objective"), Battle->GetObjectiveActor());
			Test.TestTrue(TEXT("Fresh controller resumes tactical commands"), ReadBool(Test, Controller, TEXT("CanIssueTacticalCommands")));
			Test.TestEqual(TEXT("Fresh HUD hides terminal overlay"), Root->GetWidgetFromName(TEXT("OutcomePanel"))->GetVisibility(), ESlateVisibility::Collapsed);
			Test.TestEqual(TEXT("Fresh controller has one battle listener"), ListenerCount(Test, State, TEXT("OnTacticalBattleChanged"), Controller), 1);
			Test.TestEqual(TEXT("Fresh HUD has one battle listener"), ListenerCount(Test, State, TEXT("OnTacticalBattleChanged"), Root), 1);
			return true;
		}

	private:
		FAutomationTestBase& Test;
		bool bVictory;
		double StartedAt;
		double ReattachedAt = 0.0;
		int32 Stage = 0;
		float MinPitch = 0.0f, MaxPitch = 0.0f;
		TWeakObjectPtr<UWorld> OriginalWorld;
		TWeakObjectPtr<APawn> OriginalCamera;
		TWeakObjectPtr<AActor> Soldier, Survivor, Producer;
		void CaptureTerminal(UUserWidget* Root)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureTerminalUI"))) return;
			TArray<FColor> Pixels;
			FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Renderer captures actual terminal HUD"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			if (!Test.TestTrue(TEXT("Terminal capture has valid pixels"), Size.X > 0 && Size.Y > 0 && Pixels.Num() == Size.X * Size.Y)) return;
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/C013_Visual"));
			IFileManager::Get().MakeDirectory(*Directory, true);
			Test.TestTrue(TEXT("Terminal capture written"), FFileHelper::SaveArrayToFile(PNG,
				*FPaths::Combine(Directory, bVictory ? TEXT("Victory.png") : TEXT("Defeat.png"))));
		}
		void Kill(AActor* Actor)
		{
			FCall Call(Test, Actor, TEXT("SetCurrentHealth"), 1);
			Call.SetInt(TEXT("NewHealth"), 0); Call.Execute();
		}
	};

	class FExerciseMinimap : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseMinimap(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!World || !State || !State->IsGameplayInitialized())
			{
				if (FPlatformTime::Seconds()-StartedAt < 30) return false;
				Test.AddError(TEXT("Minimap fixture did not become ready")); return true;
			}
			if (Stage == 0)
			{
				Player = World->GetFirstPlayerController();
				Root.Reset(Cast<UUserWidget>(ReadObject(Test, Player.Get(), TEXT("GetMainWidget"))));
				if (!Test.TestNotNull(TEXT("Current native shell exists"), Root.Get())) return true;
				Map = Root->GetWidgetFromName(TEXT("TacticalMinimap"));
				RegionMap = Root->GetWidgetFromName(TEXT("StrategicMinimap"));
				if (!Test.TestNotNull(TEXT("Tactical minimap instantiated"), Map.Get()) ||
					!Test.TestNotNull(TEXT("Strategic minimap instantiated"), RegionMap.Get())) return true;
				const auto* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>();
				Test.TestTrue(TEXT("Only tactical presentation active"), ReadBool(Test, Map.Get(), TEXT("IsPresentationActive")) &&
					!ReadBool(Test, RegionMap.Get(), TEXT("IsPresentationActive")));
				const auto Markers = ReadMarkers(Map.Get());
				const int32 UnitCount = Markers.FilterByPredicate([](const auto& M) { return M.Kind == EMinimapMarkerKind::Unit; }).Num();
				Test.TestEqual(TEXT("All registered live units displayed once"), UnitCount, Registry->GetRegisteredUnitCount());
				Test.TestEqual(TEXT("Only registered objective displayed"), Markers.FilterByPredicate([](const auto& M) { return M.Kind == EMinimapMarkerKind::Objective; }).Num(), 1);
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (!Registry->ContainsUnit(*It)) continue;
					FCall(Test, *It, TEXT("CommandHold"), 0).Execute();
					It->SetActorTickEnabled(false);
					if (!UnitClass.IsValid())
					{
						UnitClass = It->GetClass();
						FCall Definition(Test, *It, TEXT("GetUnitDefinitionID"), 1); Definition.Execute(); UnitID = Definition.ReadName();
					}
				}
				const FTransform Transform(FVector(0,0,100));
				Fixture = World->SpawnActorDeferred<AActor>(UnitClass.Get(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (!Test.TestNotNull(TEXT("Actual initialized unit registers"), Fixture.Get())) return true;
				FCall Initialize(Test, Fixture.Get(), TEXT("InitializeUnit"), 3); Initialize.SetName(TEXT("DefinitionID"), UnitID);
				Initialize.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Initialize.Execute();
				Test.TestTrue(TEXT("Factory initialization succeeds"), Initialize.ReadBool());
				Fixture->FinishSpawning(Transform);
				Fixture->SetActorTickEnabled(false);
				FCall(Test, Fixture.Get(), TEXT("CommandHold"), 0).Execute();
				Test.TestTrue(TEXT("Spawn event adds marker immediately"), Has(Map.Get(), Fixture->GetFName()));
				FCall Owner(Test, Fixture.Get(), TEXT("SetOwnerFaction"), 1); Owner.SetEnum(TEXT("NewOwner"), EFactionID::Carthage); Owner.Execute();
				auto OwnedMarkers = ReadMarkers(Map.Get());
				const auto* Marker = OwnedMarkers.FindByPredicate([this](const auto& M) { return M.ID == Fixture->GetFName(); });
				Test.TestTrue(TEXT("Owner event re-queries committed ownership"), Marker && !Marker->bPlayerOwned);
				Test.TestEqual(TEXT("Reindex emits no duplicate marker"), OwnedMarkers.FilterByPredicate([this](const auto& M) { return M.ID == Fixture->GetFName(); }).Num(), 1);
				FCall RestoreOwner(Test, Fixture.Get(), TEXT("SetOwnerFaction"), 1); RestoreOwner.SetEnum(TEXT("NewOwner"), EFactionID::Rome); RestoreOwner.Execute();
				FCall Select(Test, Player.Get(), TEXT("SelectActorFromInput"), 2); Select.SetObject(TEXT("Actor"), Fixture.Get()); Select.SetBool(TEXT("bAdditive"), false); Select.Execute();
				Fixture->SetActorLocation(FVector(250,125,100));
				Stage = 1; StageAt = FPlatformTime::Seconds(); return false;
			}
			if (Stage == 1)
			{
				if (FPlatformTime::Seconds()-StageAt < 0.5) return false;
				auto MovingMarkers = ReadMarkers(Map.Get());
				const auto* Moving = MovingMarkers.FindByPredicate([this](const auto& M) { return M.ID == Fixture->GetFName(); });
				Test.TestTrue(TEXT("Indexed movement timer samples actual position"), Moving && Moving->Position.Equals(FVector2D(250,125), 0.001));
				Test.TestTrue(TEXT("Selected unit has a separate color-safe shape"), Moving && Moving->bSelected && Moving->bPlayerOwned);
				Capture(TEXT("Tactical"));
				const FName FixtureID = Fixture->GetFName();
				FCall Damage(Test, Fixture.Get(), TEXT("TakeCombatDamage"), 3); Damage.SetFloat(TEXT("Damage"), 100000.f); Damage.SetObject(TEXT("DamageSource"), nullptr); Damage.SetBool(TEXT("bIsRanged"), false); Damage.Execute();
				Test.TestFalse(TEXT("Dead marker removed synchronously before destruction"), Has(Map.Get(), FixtureID));
				Fixture->Destroy();
				SetMode(EGameplayMode::Strategic);
				Test.TestFalse(TEXT("Tactical map stops in strategic mode"), ReadBool(Test, Map.Get(), TEXT("IsPresentationActive")));
				const auto Regions = ReadMarkers(RegionMap.Get());
				Test.TestEqual(TEXT("Nine canonical regions"), Regions.Num(), State->GetTerritorySnapshots().Num());
				const auto Snapshots = State->GetTerritorySnapshots();
				for (const auto& Snapshot : Snapshots)
				{
					const auto* Marker = Regions.FindByPredicate([&](const auto& M) { return M.ID == Snapshot.TerritoryID; });
					Test.TestTrue(TEXT("Territory position and color are canonical"), Marker &&
						Marker->Position == FVector2D(Snapshot.MapLocation) && Marker->Color == Snapshot.OwnerColor);
				}
				UObject* Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetWorldMapManager"));
				FCall Change(Test, Manager, TEXT("ChangeTerritoryOwner"), 3);
				Change.SetName(TEXT("TerritoryID"), TEXT("Gaul")); Change.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Change.Execute();
				const auto Updated = ReadMarkers(RegionMap.Get());
				const auto* Gaul = Updated.FindByPredicate([](const auto& M) { return M.ID == FName(TEXT("Gaul")); });
				Test.TestTrue(TEXT("Ownership invalidation refreshes territory marker immediately"), Gaul && Gaul->bPlayerOwned);
				Root->RemoveFromParent();
				Test.TestEqual(TEXT("Detached map clears marker values"), ReadInt(Test, RegionMap.Get(), TEXT("GetMarkerCount")), 0);
				FCall Stale(Test, RegionMap.Get(), TEXT("SetPresentationActive"), 1); Stale.SetBool(TEXT("bActive"), true); Stale.Execute();
				Test.TestFalse(TEXT("Detached shell cannot reactivate stale child"), ReadBool(Test, RegionMap.Get(), TEXT("IsPresentationActive")));
				CollectGarbage(RF_NoFlags);
				Root->AddToViewport();
				Stage = 2; return false;
			}
			if (Stage == 2)
			{
				Test.TestEqual(TEXT("Same root catches up after real detach/GC/reattach"), ReadInt(Test, RegionMap.Get(), TEXT("GetMarkerCount")), 9);
				Test.TestEqual(TEXT("Active region map has one owner listener"), ListenerCount(Test, State, TEXT("OnTerritoryOwnerChanged"), RegionMap.Get()), 1);
				Capture(TEXT("Strategic"));
				SetMode(EGameplayMode::Tactical);
				Test.TestEqual(TEXT("Inactive region listener removed"), ListenerCount(Test, State, TEXT("OnTerritoryOwnerChanged"), RegionMap.Get()), 0);
				Test.TestEqual(TEXT("Tactical listener singular"), ListenerCount(Test, State, TEXT("OnTerritoryOwnerChanged"), Map.Get()), 1);
				ATacticalObjective* Objective = World->GetSubsystem<UTacticalBattleSubsystem>()->GetObjectiveActor();
				if (Test.TestNotNull(TEXT("Registered objective fixture exists"), Objective)) Objective->Destroy();
				Stage = 3; StageAt = FPlatformTime::Seconds(); return false;
			}
			if (Stage == 3)
			{
				if (FPlatformTime::Seconds()-StageAt < 0.5) return false;
				Test.TestEqual(TEXT("Destroyed objective does not survive movement resample"), ReadMarkers(Map.Get()).FilterByPredicate([](const auto& M) { return M.Kind == EMinimapMarkerKind::Objective; }).Num(), 0);
				Root->RemoveFromParent();
				Test.TestEqual(TEXT("Detached listeners removed"), ListenerCount(Test, State, TEXT("OnTerritoryOwnerChanged"), Map.Get()), 0);
				return true;
			}
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt, StageAt = 0;
		int32 Stage = 0;
		TWeakObjectPtr<APlayerController> Player;
		TStrongObjectPtr<UUserWidget> Root;
		TWeakObjectPtr<UWidget> Map, RegionMap;
		TWeakObjectPtr<AActor> Fixture;
		TWeakObjectPtr<UClass> UnitClass;
		FName UnitID;
		TArray<FMinimapMarker> ReadMarkers(UObject* Widget)
		{
			FCall Query(Test, Widget, TEXT("GetMarkerSnapshots"), 1); Query.Execute(); return Query.ReadStructArray<FMinimapMarker>();
		}
		bool Has(UObject* Widget, FName ID) { return ReadMarkers(Widget).ContainsByPredicate([ID](const auto& M) { return M.ID == ID; }); }
		void SetMode(EGameplayMode Mode)
		{
			FCall Command(Test, Player->GetWorld()->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
			Command.SetEnum(TEXT("NewMode"), Mode); Command.Execute();
		}
		void Capture(const TCHAR* Name)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureMinimap"))) return;
			TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Actual minimap shell renders"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			FString CaptureFolder = TEXT("Artifacts/C017A_Visual");
			FParse::Value(FCommandLine::Get(), TEXT("RomanMinimapCaptureDir="), CaptureFolder);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(), CaptureFolder); IFileManager::Get().MakeDirectory(*Directory, true);
			Test.TestTrue(TEXT("Capture written"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory, FString(Name) + TEXT(".png"))));
		}
	};

	class FExerciseResponsiveShell : public IAutomationLatentCommand
	{
	public:
		FExerciseResponsiveShell(FAutomationTestBase* InTest, int32 InWidth, int32 InHeight, float InScale)
			: Test(*InTest), Width(InWidth), Height(InHeight), Scale(InScale), StartedAt(FPlatformTime::Seconds()) {}
		virtual ~FExerciseResponsiveShell() override
		{
			if (bPlayerTickChanged && Player.IsValid()) Player->SetActorTickEnabled(bOriginalPlayerTick);
			if (bScaleChanged) GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = OriginalScale;
			if(Preferences.IsValid() && bPreferencesChanged) Preferences->TryApply(PreviousPreferences,false);
		}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized())
			{
				if (FPlatformTime::Seconds()-StartedAt < 30) return false;
				Test.AddError(TEXT("Responsive shell fixture did not become ready")); return true;
			}
			if (Stage == 0)
			{
				OriginalScale = GetDefault<UUserInterfaceSettings>()->ApplicationScale;
				if (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested())
					StableView = RomanVisualRegression::Stabilize(World);
				GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = Scale; bScaleChanged = true;
				Player = World->GetFirstPlayerController();
				Root.Reset(Cast<UUserWidget>(ReadObject(Test, Player.Get(), TEXT("GetMainWidget"))));
				if (!Test.TestNotNull(TEXT("Responsive production root exists"), Root.Get())) return true;
				if(FParse::Value(FCommandLine::Get(),TEXT("RomanUITextScale="),PreferenceScale))
				{
					Preferences=World->GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>();
					if(!Test.TestTrue(TEXT("Real preferences owner for accessibility render"),Preferences.IsValid())) return true;
					PreviousPreferences=Preferences->GetPreferences();
					auto Value = (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested()) ? FRomanPlayerPreferences{} : PreviousPreferences;
					Value.TextScale=PreferenceScale; // Visual fixtures use canonical transient prefs, never write personal settings.
					if(!Test.TestEqual(TEXT("Validated temporary accessibility size"),Preferences->TryApply(Value,false),ERomanPreferencesFailure::None)) return true;
					bPreferencesChanged=true;
				}
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					AActor* Unit = *It;
					if (!World->GetSubsystem<URomanUnitRegistrySubsystem>()->ContainsUnit(Unit)) continue;
					FCall(Test, Unit, TEXT("CommandHold"), 0).Execute(); Unit->SetActorTickEnabled(false);
					if (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested())
					{
						if (ACharacter* Character = Cast<ACharacter>(Unit))
						{
							Character->GetCharacterMovement()->StopMovementImmediately();
							Character->GetCharacterMovement()->DisableMovement();
							Character->GetCharacterMovement()->SetComponentTickEnabled(false);
							if (USkeletalMeshComponent* Mesh = Character->GetMesh())
							{
								Mesh->SetComponentTickEnabled(false);
								Mesh->InitAnim(true); Mesh->TickAnimation(.25f, false); Mesh->RefreshBoneTransforms();
							}
						}
					}
				}
				for (TActorIterator<AActor> It(World); It; ++It)
					if (It->FindFunction(TEXT("TrainUnit"))) { Producer = *It; break; }
				if (!Test.TestNotNull(TEXT("Actual barracks exists"), Producer.Get())) return true;
				FCall Select(Test, Player.Get(), TEXT("SelectBuilding"), 1); Select.SetObject(TEXT("Building"), Producer.Get()); Select.Execute();
				return Next();
			}
			if (Stage >= 2 && !bPlayerTickChanged && (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested()))
			{
				// Real keyboard/modal checks have completed. Preserve native Slate button
				// dispatch but exclude the host cursor's unrelated edge-scroll polling.
				bOriginalPlayerTick = Player->IsActorTickEnabled(); Player->SetActorTickEnabled(false); bPlayerTickChanged = true;
				if (Player->GetPawn()) Player->GetPawn()->SetActorLocation(FVector::ZeroVector);
			}
			if (Frames == 0) LayoutAt = FPlatformTime::Seconds();
			if (Frames++ < 3 || ((RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested()) && FPlatformTime::Seconds() - LayoutAt < .5)) return false;
			if(Stage==1 && bPreferencesChanged && !bSettingsChecked)
			{
				if(!bSettingsOpened)
				{
					if(!ClickThroughSlate(Test,Root.Get(),Cast<UButton>(Widget(TEXT("OpenSettingsButton"))))) return true;
					if(!Test.TestTrue(TEXT("Actual HUD pointer opens accessible settings"),ReadBool(Test,Player.Get(),TEXT("IsSettingsOpen")))) return true;
					bSettingsOpened=true; Frames=0; return false;
				}
				auto* Menu=Cast<UUserWidget>(ReadObject(Test,Player.Get(),TEXT("GetSettingsWidget")));
				if(!Test.TestNotNull(TEXT("Actual modal layout root"),Menu)) return true;
				Menu->ForceLayoutPrepass();
				auto* Resume=Cast<UButton>(Menu->GetWidgetFromName(TEXT("ResumeSettingsButton")));
				if(bBindingCaptureOpened)
				{
					CheckCenterInside(Menu->GetWidgetFromName(TEXT("SettingsStatus")),Menu->GetWidgetFromName(TEXT("SettingsFit")),TEXT("Binding capture/error feedback stays visible in fixed footer at 200% text"));
					CheckCenterInside(Resume,Menu->GetWidgetFromName(TEXT("SettingsFit")),TEXT("Resume stays visible alongside long capture feedback"));
					TArray<FColor> CapturePixels; FIntVector CaptureSize=FIntVector::ZeroValue;
					if(!Test.TestTrue(TEXT("Actual accessible binding capture renders"),FSlateApplication::Get().TakeScreenshot(Menu->TakeWidget(),CapturePixels,CaptureSize))) return true;
					if (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested())
					{
						Test.TestEqual(TEXT("Binding raster native width"), CaptureSize.X, Width); Test.TestEqual(TEXT("Binding raster native height"), CaptureSize.Y, Height);
						RomanVisualRegression::SaveUIAndCompare(Test, CapturePixels, Width, Height, FMath::RoundToInt(Scale*100), FMath::RoundToInt(PreferenceScale*100), TEXT("BindingCapture"));
					}
					else
					{
					TArray64<uint8> CapturePNG; FImageUtils::PNGCompressImageArray(CaptureSize.X,CaptureSize.Y,TArrayView64<const FColor>(CapturePixels.GetData(),CapturePixels.Num()),CapturePNG);
					Test.TestTrue(TEXT("Actual binding capture screenshot saved"),FFileHelper::SaveArrayToFile(CapturePNG,*FPaths::Combine(FPaths::ProjectDir(),TEXT("Artifacts/C017H_UIText"),FString::Printf(TEXT("%dx%d_%d_BindingCapture.png"),Width,Height,FMath::RoundToInt(Scale*100)))));
					}
					FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
					FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
					Test.TestTrue(TEXT("Real Escape cancels capture without closing the paused modal"),ReadBool(Test,Player.Get(),TEXT("IsSettingsOpen")) && Player->IsPaused());
					FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
					Test.TestFalse(TEXT("Second real Escape closes modal"),ReadBool(Test,Player.Get(),TEXT("IsSettingsOpen")));
					Test.TestFalse(TEXT("Actual keyboard resume releases pause"),Player->IsPaused());
					bSettingsChecked=true; Frames=0; return false;
				}
				if(!Test.TestTrue(TEXT("Actual controller places keyboard focus on Resume"),Resume && Resume->GetCachedWidget()==FSlateApplication::Get().GetUserFocusedWidget(0))) return true;
				CheckCenterInside(Resume,Menu->GetWidgetFromName(TEXT("SettingsFit")),TEXT("Focused Resume is visible inside fixed modal footer"));
				TArray<FColor> Pixels; FIntVector Size=FIntVector::ZeroValue;
				if(!Test.TestTrue(TEXT("Actual settings modal renders"),FSlateApplication::Get().TakeScreenshot(Menu->TakeWidget(),Pixels,Size))) return true;
				if (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested())
				{
					Test.TestEqual(TEXT("Settings raster native width"), Size.X, Width); Test.TestEqual(TEXT("Settings raster native height"), Size.Y, Height);
					RomanVisualRegression::SaveUIAndCompare(Test, Pixels, Width, Height, FMath::RoundToInt(Scale*100), FMath::RoundToInt(PreferenceScale*100), TEXT("Settings"));
				}
				else
				{
				TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,TArrayView64<const FColor>(Pixels.GetData(),Pixels.Num()),PNG);
				const FString Directory=FPaths::Combine(FPaths::ProjectDir(),TEXT("Artifacts/C017H_UIText")); IFileManager::Get().MakeDirectory(*Directory,true);
				Test.TestTrue(TEXT("Settings capture saved"),FFileHelper::SaveArrayToFile(PNG,*FPaths::Combine(Directory,FString::Printf(TEXT("%dx%d_%d_Settings.png"),Width,Height,FMath::RoundToInt(Scale*100)))));
				}
				const auto PreviousFocus=FSlateApplication::Get().GetUserFocusedWidget(0);
				FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Tab,FModifierKeysState(),0,false,0,0));
				Test.TestTrue(TEXT("Real Tab navigates within the focused settings form"),FSlateApplication::Get().GetUserFocusedWidget(0)!=PreviousFocus && Menu->HasUserFocusedDescendants(Player.Get()));
				auto* Binding=Cast<UButton>(Menu->GetWidgetFromName(TEXT("Binding_Roman.v1.Possession.Exit_possession")));
				if(!Test.TestNotNull(TEXT("Actual last authored binding row is keyboard focusable"),Binding)) return true;
				Binding->OnClicked.Broadcast(); // Capture only, no remap/save/personal profile mutation.
				bBindingCaptureOpened=true; Frames=0; return false;
			}
			if(Stage==1 && bPreferencesChanged && !bTrainingRevealed)
			{
				CastChecked<UScrollBox>(Widget(TEXT("ResponsiveSelectionScroll")))->ScrollWidgetIntoView(Widget(TEXT("TrainButton")),false,EDescendantScrollDestination::Center);
				bTrainingRevealed=true; Frames=0; return false;
			}
			if (Stage == 1)
			{
				CheckShell(); CheckContained(TEXT("TacticalMinimapFrame"));
				CheckContained(TEXT("SelectionPanel")); CheckContained(TEXT("TrainButton"));
				if (!ClickThroughSlate(Test, Root.Get(), Cast<UButton>(Widget(TEXT("TrainButton"))))) return true;
				Test.TestTrue(TEXT("Responsive pointer click starts canonical training"), ReadBool(Test, Producer.Get(), TEXT("IsTraining")));
				return Next();
			}
			if (Stage == 2)
			{
				if(bPreferencesChanged && !bCancelRevealed)
				{
					CastChecked<UScrollBox>(Widget(TEXT("ResponsiveSelectionScroll")))->ScrollWidgetIntoView(Widget(TEXT("CancelTrainingButton")),false,EDescendantScrollDestination::Center);
					bCancelRevealed=true; Frames=0; return false;
				}
				CheckContained(TEXT("CancelTrainingButton"));
				if (!ClickThroughSlate(Test, Root.Get(), Cast<UButton>(Widget(TEXT("CancelTrainingButton"))))) return true;
				Test.TestFalse(TEXT("Responsive pointer cancellation removes that active order"), ReadBool(Test, Producer.Get(), TEXT("IsTraining")));
				for (const TCHAR* Name : {TEXT("SaveCampaignButton"), TEXT("LoadCampaignButton"), TEXT("LoadAutosaveButton"), TEXT("AutosaveToggleButton")})
				{
					auto* Button = CastChecked<UButton>(Widget(Name)); auto* Label = CastChecked<UTextBlock>(Button->GetContent());
					Test.TestTrue(TEXT("Save/load action labels wrap instead of overpainting neighbours"), Label->GetAutoWrapText());
					const FGeometry LabelGeometry = Geometry(Label);
					Test.TestTrue(TEXT("Arranged save/load label fits actual horizontal allocation"), Label->GetDesiredSize().X <= LabelGeometry.GetLocalSize().X + 1.f);
				}
				Capture(TEXT("Tactical")); FCall(Test,Root.Get(),TEXT("ShowBuildingMenu"),0).Execute(); return Next();
			}
			if (Stage == 3)
			{
				Test.TestTrue(TEXT("Canonical building menu opens"),ReadBool(Test,Root.Get(),TEXT("IsBuildingMenuVisible")));
				CheckContained(TEXT("BuildMenuPanel")); Capture(TEXT("Construction"));
				FCall(Test,Root.Get(),TEXT("HideBuildingMenu"),0).Execute(); SetMode(EGameplayMode::Strategic); return Next();
			}
			if (Stage == 4)
			{
				CheckShell(); CheckContained(TEXT("StrategicPanel"));
				auto* ArmyList = Cast<UScrollBox>(Widget(TEXT("CampaignArmyList")));
				if (!Test.TestNotNull(TEXT("Actual strategic army list"), ArmyList)) return true;
				if (!Test.TestEqual(TEXT("Initial army buttons retain both canonical armies"), ArmyList->GetChildrenCount(), 2)) return true;
				for (int32 Index = 0; Index < 2; ++Index)
				{
					UWidget* Button = ArmyList->GetChildAt(Index); FCampaignArmy Army;
					FCall Identity(Test, Button, TEXT("GetArmyID"), 1); if (!Identity.Execute()) return true;
					if (!Test.TestTrue(TEXT("Army UI retains actual command-routing identity"), State->TryGetCampaignArmy(Identity.ReadGuid(), Army))) return true;
					Test.TestEqual(TEXT("Faction-stable order independent of random army GUIDs"), Army.Faction, Index == 0 ? EFactionID::Rome : EFactionID::Carthage);
				}
				TArray<UWidget*> Cards; Root->WidgetTree->GetAllWidgets(Cards);
				for (UWidget* Candidate : Cards)
				{
					if (!Candidate->FindFunction(TEXT("GetTerritoryID"))) continue;
					auto* Button = CastChecked<UButton>(Candidate);
					const FGeometry Card = Geometry(Button), Label = Geometry(Button->GetContent());
					Test.TestTrue(TEXT("Wrapped region text fits its actual card, not a fixed unclipped rectangle"),Label.GetLocalSize().X <= Card.GetLocalSize().X && Label.GetLocalSize().Y <= Card.GetLocalSize().Y);
				}
				auto* Scroll = CastChecked<UScrollBox>(Widget(TEXT("ResponsiveRegionScroll")));
				Scroll->ScrollWidgetIntoView(GaulCard(), false, EDescendantScrollDestination::Center);
				return Next();
			}
			if (Stage == 5)
			{
				auto* Card = GaulCard();
				if (!Test.TestNotNull(TEXT("Canonical Gaul card exists"), Card)) return true;
				CheckCenterInside(Card, Root.Get(), TEXT("Gaul center is inside the current viewport"));
				CheckCenterInside(Card, Widget(TEXT("ResponsiveRegionScroll")), TEXT("Scrolled region has a visible center, not just offscreen positive geometry"));
				if (!ClickThroughSlate(Test, Root.Get(), Card)) return true;
				FCall Selected(Test, Root.Get(), TEXT("GetSelectedTerritoryID"), 1); Selected.Execute();
				Test.TestEqual(TEXT("Responsive region pointer follows stable canonical ID"), Selected.ReadName(), FName(TEXT("Gaul")));
				CastChecked<UScrollBox>(Widget(TEXT("ResponsiveDetailScroll")))->ScrollWidgetIntoView(Widget(TEXT("StartBattleButton")), false, EDescendantScrollDestination::Center);
				return Next();
			}
			if (Stage == 6)
			{
				if(bPreferencesChanged && !bStrategicCommandRevealed)
				{
					// Selection has now had real frames to arrange newly-created/wrapped army controls.
					CastChecked<UScrollBox>(Widget(TEXT("ResponsiveDetailScroll")))->ScrollWidgetIntoView(Widget(TEXT("StartBattleButton")),false,EDescendantScrollDestination::Center);
					bStrategicCommandRevealed=true; Frames=0; return false;
				}
				if(bPreferencesChanged && !bCampaignCommandRevealed)
				{
					CastChecked<UScrollBox>(Widget(TEXT("ResponsiveCommandsScroll")))->ScrollWidgetIntoView(Widget(TEXT("EnterCampaignButton")),false,EDescendantScrollDestination::Center);
					bCampaignCommandRevealed=true; Frames=0; return false;
				}
				CheckCenterInside(Widget(TEXT("StartBattleButton")), Widget(TEXT("ResponsiveDetailScroll")), TEXT("Full strategic column scroll exposes army commands"));
				const FGeometry BattleGeometry = Geometry(Widget(TEXT("StartBattleButton")));
				const FGeometry DetailGeometry = Geometry(Widget(TEXT("ResponsiveDetailScroll")));
				const FVector2D BattleStart = DetailGeometry.AbsoluteToLocal(BattleGeometry.LocalToAbsolute(FVector2D::ZeroVector));
				const FVector2D BattleEnd = DetailGeometry.AbsoluteToLocal(BattleGeometry.LocalToAbsolute(BattleGeometry.GetLocalSize()));
				Test.TestTrue(*FString::Printf(TEXT("Entire strategic battle command fits actual scroll allocation (start%s end%s scroll%s)"),
					*BattleStart.ToString(), *BattleEnd.ToString(), *DetailGeometry.GetLocalSize().ToString()),
					BattleStart.X >= -1 && BattleStart.Y >= -1 &&
					BattleEnd.X <= DetailGeometry.GetLocalSize().X + 1 && BattleEnd.Y <= DetailGeometry.GetLocalSize().Y + 1);
				CheckContained(TEXT("StartBattleButton"));
				CheckContained(TEXT("EnterCampaignButton")); Capture(TEXT("Strategic"));
				SetMode(EGameplayMode::Tactical);
				AActor* VisualSoldier = nullptr;
				const bool bVisual = RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested();
				const FVector VisualPosition(-400, -200, 100);
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					AActor* Unit = *It;
					if (!World->GetSubsystem<URomanUnitRegistrySubsystem>()->ContainsUnit(Unit)) continue;
					FCall Owner(Test, Unit, TEXT("GetOwnerFaction"), 1); Owner.Execute();
					if (Owner.ReadEnum<EFactionID>() == EFactionID::Rome)
					{
						if (bVisual)
						{
							// Actor iteration/name order is not a cross-process visual identity.
							if (!VisualSoldier || FVector::DistSquared(Unit->GetActorLocation(), VisualPosition) < FVector::DistSquared(VisualSoldier->GetActorLocation(), VisualPosition)) VisualSoldier = Unit;
							continue;
						}
						FCall Possess(Test, Player.Get(), TEXT("EnterFirstPersonMode"), 1);
						Possess.SetObject(TEXT("UnitToPossess"), Unit); Possess.Execute(); break;
					}
				}
				if (bVisual)
				{
					if (!Test.TestNotNull(TEXT("Canonical live Roman soldier for UI raster"), VisualSoldier)) return true;
					VisualSoldier->SetActorLocationAndRotation(VisualPosition, FRotator::ZeroRotator);
					Player->SetControlRotation(FRotator::ZeroRotator);
					FCall Possess(Test, Player.Get(), TEXT("EnterFirstPersonMode"), 1);
					Possess.SetObject(TEXT("UnitToPossess"), VisualSoldier); Possess.Execute();
					if (ACharacter* Character = Cast<ACharacter>(VisualSoldier))
					{
						// Static mesh ticking is off: publish native possession's bone-hide
						// effect explicitly, as the existing character raster fixture does.
						USkeletalMeshComponent* Mesh = Character->GetMesh();
						Test.TestTrue(TEXT("Native local possession hides rigged head"), Mesh->IsBoneHiddenByName(TEXT("head")));
						Test.TestFalse(TEXT("Native possession keeps sword hand visible"), Mesh->IsBoneHiddenByName(TEXT("hand_r")));
						Mesh->TickAnimation(.01f, false); Mesh->RefreshBoneTransforms();
					}
				}
				return Next();
			}
			if (Stage == 7)
			{
				Test.TestTrue(TEXT("Actual possession enters canonical first-person mode"), ReadBool(Test, Player.Get(), TEXT("IsInFirstPersonMode")));
				Test.TestFalse(TEXT("Actual possession refresh collapses unavailable campaign save controls"), Widget(TEXT("CampaignSavePanel"))->IsVisible());
				Test.TestFalse(TEXT("FPS diagnostic readout cannot obscure native viewport-centre aim"), Widget(TEXT("DiagnosticPanel"))->IsVisible());
				const FGeometry AimGeometry = Geometry(Widget(TEXT("CrosshairText")));
				const FGeometry ViewGeometry = Geometry(Root.Get());
				const FVector2D AimCenter = ViewGeometry.AbsoluteToLocal(AimGeometry.LocalToAbsolute(AimGeometry.GetLocalSize() * .5f));
				Test.TestTrue(TEXT("Crosshair text fits its arranged allocation at the applied text scale"),
					Widget(TEXT("CrosshairText"))->GetDesiredSize().X <= AimGeometry.GetLocalSize().X + 1.f &&
					Widget(TEXT("CrosshairText"))->GetDesiredSize().Y <= AimGeometry.GetLocalSize().Y + 1.f);
				Test.TestTrue(TEXT("Crosshair allocation remains centred on actual first-person viewport"),
					AimCenter.Equals(ViewGeometry.GetLocalSize() * .5f, 1.f));
				CheckShell(); CheckContained(TEXT("PossessionPanel")); CheckContained(TEXT("PossessionText")); CheckContained(TEXT("CrosshairText")); Capture(TEXT("Possession"));
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (!World->GetSubsystem<URomanUnitRegistrySubsystem>()->ContainsUnit(*It)) continue;
					FCall Owner(Test,*It,TEXT("GetOwnerFaction"),1); Owner.Execute();
					if (Owner.ReadEnum<EFactionID>() != EFactionID::Carthage) continue;
					FCall Damage(Test,*It,TEXT("TakeCombatDamage"),3); Damage.SetFloat(TEXT("Damage"),100000.f);
					Damage.SetObject(TEXT("DamageSource"),nullptr); Damage.SetBool(TEXT("bIsRanged"),false); Damage.Execute();
				}
				ATacticalObjective* Objective = World->GetSubsystem<UTacticalBattleSubsystem>()->GetObjectiveActor();
				if (!Test.TestNotNull(TEXT("Actual objective is available for terminal layout"),Objective)) return true;
				Player->GetPawn()->SetActorLocation(Objective->GetActorLocation()+FVector(0,0,100));
				return Next();
			}
			if (Stage == 8)
			{
				if (!World->GetSubsystem<UTacticalBattleSubsystem>()->IsResolved())
				{
					if (FPlatformTime::Seconds()-StartedAt < 30) return false;
					Test.AddError(TEXT("Actual occupation did not resolve terminal layout fixture")); return true;
				}
				CheckContained(TEXT("OutcomePanel")); CheckContained(TEXT("ResponsiveOutcomeScroll"));
				CastChecked<UScrollBox>(Widget(TEXT("ResponsiveOutcomeScroll")))->ScrollWidgetIntoView(Widget(TEXT("RestartEncounterButton")),false,EDescendantScrollDestination::Center);
				return Next();
			}
			if (Stage == 9)
			{
				Test.TestTrue(TEXT("Diagnostic presentation restores after native exit from possession"), Widget(TEXT("DiagnosticPanel"))->IsVisible());
				Test.TestTrue(TEXT("Save presentation re-queries native tactical eligibility after possession"), Widget(TEXT("CampaignSavePanel"))->IsVisible());
				CheckCenterInside(Widget(TEXT("RestartEncounterButton")),Widget(TEXT("ResponsiveOutcomeScroll")),TEXT("Terminal result scroll exposes its restart command"));
				CheckContained(TEXT("RestartEncounterButton")); Capture(TEXT("Outcome"));
				return true;
			}
			return true;
		}
	private:
		FAutomationTestBase& Test;
		int32 Width, Height, Stage = 0, Frames = 0;
		float Scale, OriginalScale = 1;
		double LayoutAt = 0;
		TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> StableView;
		bool bScaleChanged = false;
		bool bPlayerTickChanged = false, bOriginalPlayerTick = false;
		float PreferenceScale=1;
		bool bPreferencesChanged=false,bSettingsOpened=false,bSettingsChecked=false,bBindingCaptureOpened=false;
		bool bTrainingRevealed=false,bCancelRevealed=false,bCampaignCommandRevealed=false;
		bool bStrategicCommandRevealed=false;
		TWeakObjectPtr<URomanPlayerSettingsSubsystem> Preferences;
		FRomanPlayerPreferences PreviousPreferences;
		double StartedAt;
		TStrongObjectPtr<UUserWidget> Root;
		TWeakObjectPtr<APlayerController> Player;
		TWeakObjectPtr<AActor> Producer;
		bool Next() { ++Stage; Frames = 0; return false; }
		UWidget* Widget(const TCHAR* Name) { return Root->GetWidgetFromName(Name); }
		UButton* GaulCard()
		{
			TArray<UWidget*> Widgets; Root->WidgetTree->GetAllWidgets(Widgets);
			for (UWidget* Candidate : Widgets)
			{
				if (!Candidate->FindFunction(TEXT("GetTerritoryID"))) continue;
				FCall ID(Test,Candidate,TEXT("GetTerritoryID"),1); ID.Execute();
				if (ID.ReadName()==FName(TEXT("Gaul"))) return Cast<UButton>(Candidate);
			}
			return nullptr;
		}
		FGeometry Geometry(UWidget* Value)
		{
			Root->ForceLayoutPrepass(); FWidgetPath Path;
			if (!Value || !Value->GetCachedWidget().IsValid() || !FSlateApplication::Get().GeneratePathToWidgetUnchecked(Value->GetCachedWidget().ToSharedRef(), Path) || !Path.IsValid())
			{
				Test.AddError(FString::Printf(TEXT("No arranged live-window path for %s"), Value ? *Value->GetName() : TEXT("missing widget"))); return FGeometry();
			}
			return Path.Widgets.Last().Geometry;
		}
		void CheckContained(const TCHAR* Name)
		{
			const FGeometry Child = Geometry(Widget(Name)), Parent = Geometry(Root.Get());
			const FVector2D Start = Parent.AbsoluteToLocal(Child.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D End = Parent.AbsoluteToLocal(Child.LocalToAbsolute(Child.GetLocalSize()));
			Test.TestTrue(*FString::Printf(TEXT("%s has usable on-screen bounds at %dx%d scale%.2f (start%s end%s root%s)"), Name, Width, Height, Scale, *Start.ToString(), *End.ToString(), *Parent.GetLocalSize().ToString()),
				Child.GetLocalSize().X > 1 && Child.GetLocalSize().Y > 1 && Start.X >= -1 && Start.Y >= -1 && End.X <= Parent.GetLocalSize().X+1 && End.Y <= Parent.GetLocalSize().Y+1);
		}
		void CheckCenterInside(UWidget* ChildWidget, UWidget* ParentWidget, const TCHAR* Label)
		{
			const FGeometry Child = Geometry(ChildWidget), Parent = Geometry(ParentWidget);
			const FVector2D Center = Parent.AbsoluteToLocal(Child.LocalToAbsolute(Child.GetLocalSize()*0.5));
			Test.TestTrue(Label, Center.X >= 0 && Center.Y >= 0 && Center.X <= Parent.GetLocalSize().X && Center.Y <= Parent.GetLocalSize().Y);
		}
		void CheckShell()
		{
			const FGeometry RootGeometry = Geometry(Root.Get());
			const FVector2D Pixels = RootGeometry.LocalToAbsolute(RootGeometry.GetLocalSize())-RootGeometry.LocalToAbsolute(FVector2D::ZeroVector);
			Test.TestEqual(TEXT("Real window has requested pixel width"), static_cast<int32>(FMath::RoundToInt(Pixels.X)), Width);
			Test.TestEqual(TEXT("Real window has requested pixel height"), static_cast<int32>(FMath::RoundToInt(Pixels.Y)), Height);
			for (const TCHAR* Name : {TEXT("TopBar"),TEXT("ResponsiveInformation"),TEXT("ResponsiveBody"),TEXT("FooterFit")}) CheckContained(Name);
			const FGeometry Top = Geometry(Widget(TEXT("TopBar"))), Info = Geometry(Widget(TEXT("ResponsiveInformation"))), Body = Geometry(Widget(TEXT("ResponsiveBody"))), Footer = Geometry(Widget(TEXT("FooterFit")));
			Test.TestTrue(TEXT("Header/information/body/footer do not overlap vertically"),
				Top.LocalToAbsolute(Top.GetLocalSize()).Y <= Info.LocalToAbsolute(FVector2D::ZeroVector).Y+1 &&
				Info.LocalToAbsolute(Info.GetLocalSize()).Y <= Body.LocalToAbsolute(FVector2D::ZeroVector).Y+1 &&
				Body.LocalToAbsolute(Body.GetLocalSize()).Y <= Footer.LocalToAbsolute(FVector2D::ZeroVector).Y+1);
		}
		void SetMode(EGameplayMode Mode)
		{
			FCall Command(Test, Player->GetWorld()->GetAuthGameMode(), TEXT("SetGameplayMode"), 1); Command.SetEnum(TEXT("NewMode"), Mode); Command.Execute();
		}
		void Capture(const TCHAR* Mode)
		{
			if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureResponsive"))) return;
			if ((RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested()) && Player.IsValid() && Player->GetPawn() && Player->PlayerCameraManager)
				Test.AddInfo(FString::Printf(TEXT("UICapturePose %s pawn=%s position=%s rotation=%s control=%s cameraPosition=%s cameraRotation=%s fov=%.6f"),
					Mode, *Player->GetPawn()->GetName(), *Player->GetPawn()->GetActorLocation().ToString(), *Player->GetPawn()->GetActorRotation().ToString(),
					*Player->GetControlRotation().ToString(), *Player->PlayerCameraManager->GetCameraLocation().ToString(),
					*Player->PlayerCameraManager->GetCameraRotation().ToString(), Player->PlayerCameraManager->GetFOVAngle()));
			TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
			if (!Test.TestTrue(TEXT("Actual responsive shell renders"), FSlateApplication::Get().TakeScreenshot(Root->TakeWidget(), Pixels, Size))) return;
			Test.TestEqual(TEXT("Captured width is target resolution"), Size.X, Width); Test.TestEqual(TEXT("Captured height is target resolution"), Size.Y, Height);
			if (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested())
			{
				RomanVisualRegression::SaveUIAndCompare(Test, Pixels, Width, Height, FMath::RoundToInt(Scale*100), FMath::RoundToInt(PreferenceScale*100), Mode);
				return; // New immutable artifact only; historical UI PNGs are never overwritten.
			}
			TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
			const FString Directory = FPaths::Combine(FPaths::ProjectDir(),bPreferencesChanged?TEXT("Artifacts/C017H_UIText"):TEXT("Artifacts/C017B_Responsive")); IFileManager::Get().MakeDirectory(*Directory,true);
			Test.TestTrue(TEXT("Responsive capture saved"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory,FString::Printf(TEXT("%dx%d_%d_%s.png"), Width, Height, FMath::RoundToInt(Scale*100), Mode))));
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

void UCampaignAIExecutionProbe::OnStep(int32 Turn, ECampaignTurnStep Step)
{
	(void)Turn;
	if (!Test || !Session.IsValid()) return;
	Test->TestFalse(TEXT("No tactical travel inside canonical stages"), Session->HasPendingTransfer());
	Steps.Add(Step); CurrentStep = Step;
	Test->TestEqual(TEXT("Canonical ordered pipeline"), Step, RomanEmpireCampaign::GetCanonicalTurnPipeline()[(Steps.Num() - 1) % 9]);
	if (bBreakMap && Step == ECampaignTurnStep::ConflictResolution && State->GetCampaignEncounterPlan().EncounterID.IsValid()) Target->TacticalMap.Reset();
}
void UCampaignAIExecutionProbe::OnResources(EFactionID Faction, FFactionResourceAmounts Balance)
{
	using namespace RomanEmpireUITests;
	if (!Test || Faction != EFactionID::Carthage || CurrentStep != ECampaignTurnStep::AIActions) return;
	++RecruitmentEvents;
	Test->TestTrue(TEXT("Purchase event sees all six committed currencies"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Balance, &ExpectedWallet, 0));
	FCampaignArmy Army; Test->TestTrue(TEXT("Purchase event sees original army"), State->TryGetCampaignArmy(AIArmy, Army));
	Test->TestEqual(TEXT("Purchase event sees committed quantity"), FCampaignArmyState::GetUnitCount(Army), ExpectedCount);
	FCall Nested(*Test, Manager.Get(), TEXT("ProcessTurn"), 1); Nested.Execute();
	Test->TestEqual(TEXT("Purchase cannot reenter turn"), Nested.ReadStruct<FCampaignTurnResult>().Failure, ECampaignTurnFailure::TurnAlreadyProcessing);
	UObject* Mode = Manager->GetWorld()->GetAuthGameMode();
	FFactionResourceAmounts Cost; Cost.Gold = 1;
	FCall Spend(*Test, ReadObject(*Test, Mode, TEXT("GetFactionManager")), TEXT("TrySpendFactionResources"), 3);
	Spend.SetEnum(TEXT("FactionID"), EFactionID::Carthage); Spend.SetStruct(TEXT("Cost"), Cost); Spend.Execute();
	Test->TestEqual(TEXT("AI purchase publication blocks nested wallet write"), Spend.ReadStruct<FFactionResourceTransactionResult>().Failure, EResourceTransactionFailure::MutationInProgress);
	FCall Change(*Test, ReadObject(*Test, Mode, TEXT("GetWorldMapManager")), TEXT("ChangeTerritoryOwner"), 3);
	Change.SetName(TEXT("TerritoryID"), TEXT("Africa")); Change.SetEnum(TEXT("NewOwner"), EFactionID::Rome); Change.Execute();
	Test->TestEqual(TEXT("AI purchase publication blocks nested map write"), Change.ReadStruct<FTerritoryOwnershipChangeResult>().Failure, ETerritoryOwnershipFailure::TransactionInProgress);
}
void UCampaignAIExecutionProbe::OnEncounter()
{
	using namespace RomanEmpireUITests;
	if (!Test || !State.IsValid()) return;
	const auto Plan = State->GetCampaignEncounterPlan(); if (!Plan.EncounterID.IsValid()) return;
	if (!AcceptedEncounter.IsValid()) AcceptedEncounter = Plan.EncounterID;
	Test->TestEqual(TEXT("Notifications preserve accepted identity"), Plan.EncounterID, AcceptedEncounter);
	Test->TestEqual(TEXT("Human remains Roma"), Plan.PlayerFaction, EFactionID::Rome);
	Test->TestEqual(TEXT("Accepted target is Roma"), Plan.TargetTerritoryID, FName(TEXT("Rome")));
	Test->TestTrue(TEXT("Accepted intent visible before travel"), ReadBool(*Test, Manager.Get(), TEXT("IsAwaitingAITacticalTravel")));
	FCall Nested(*Test, Manager.Get(), TEXT("ProcessTurn"), 1); Nested.Execute();
	Test->TestTrue(TEXT("Publication blocks nested turn/retry"), !Nested.ReadStruct<FCampaignTurnResult>().WasSuccessful());
}
void UCampaignAIExecutionProbe::OnSession()
{
	if (!Test || !Session.IsValid() || !Session->HasPendingTransfer()) return;
	Test->TestEqual(TEXT("Travel only after four complete pipelines"), Steps.Num(), 36);
	Test->TestEqual(TEXT("Travel sees incremented turn"), State->GetCurrentTurn(), 5);
}

void UCampaignTravelProbe::OnReady()
{
	if (!Test || !State.IsValid()) return;
	++ReadyNotifications;
	Test->TestTrue(TEXT("First readiness reads restored canonical authority"), State->IsGameplayInitialized());
	Test->TestEqual(TEXT("First readiness reads restored turn, never default turn"), State->GetCurrentTurn(), ExpectedTurn);
	FTerritoryPresentationSnapshot City;
	Test->TestTrue(TEXT("First readiness finds preserved city"), State->TryGetTerritorySnapshot(TEXT("Sicily"), City));
	Test->TestEqual(TEXT("First readiness has committed owner"), City.OwnerFaction, EFactionID::Rome);
	Test->TestEqual(TEXT("First readiness has committed city name"), City.Settlement.SettlementName.ToString(), FString(TEXT("Travel City")));
	FCampaignArmy Army;
	Test->TestEqual(TEXT("First readiness has stable army identity/retirement"), State->TryGetCampaignArmy(ArmyID, Army), !bExpectedRetired);
	if (Session.IsValid() && Session->GetPhase() == ECampaignTransferPhase::TacticalDeployment)
		Test->TestEqual(TEXT("First tactical readiness includes actual mapped battle"), State->GetTacticalBattleResult().EncounterID, State->GetCampaignEncounterPlan().EncounterID);
}

void UCampaignTravelProbe::OnRefund(EFactionID Faction, FFactionResourceAmounts Balance)
{
	using namespace RomanEmpireUITests;
	if (Faction != EFactionID::Rome || !Test) return;
	++RefundNotifications;
	Test->TestTrue(TEXT("Refund event sees complete six-currency credit"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Balance, &ExpectedWallet, 0));
	Test->TestFalse(TEXT("Refund event sees cleared active producer"), ReadBool(*Test, Producer.Get(), TEXT("IsTraining")));
	Test->TestEqual(TEXT("Refund event sees cleared paid queue"), ReadInt(*Test, Producer.Get(), TEXT("GetPendingOrderCount")), 0);
	Test->TestEqual(TEXT("Observer cannot reenter session operation"), Session->EnterStrategicCampaign(), ECampaignSessionFailure::Busy);
	FFactionResourceAmounts Cost; Cost.Gold = 1;
	FCall Spend(*Test, Factions.Get(), TEXT("TrySpendFactionResources"), 3); Spend.SetEnum(TEXT("FactionID"), EFactionID::Rome); Spend.SetStruct(TEXT("Cost"), Cost); Spend.Execute();
	Test->TestEqual(TEXT("Observer cannot spend captured wallet"), Spend.ReadStruct<FFactionResourceTransactionResult>().Failure, EResourceTransactionFailure::MutationInProgress);
	FCall Change(*Test, Map.Get(), TEXT("ChangeTerritoryOwner"), 3); Change.SetName(TEXT("TerritoryID"), TEXT("Sicily")); Change.SetEnum(TEXT("NewOwner"), EFactionID::Carthage); Change.Execute();
	Test->TestEqual(TEXT("Observer cannot change captured territory"), Change.ReadStruct<FTerritoryOwnershipChangeResult>().Failure, ETerritoryOwnershipFailure::TransactionInProgress);
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FResponsiveShellPIETest, "RomanEmpireGame.UI.ResponsiveShell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FResponsiveShellPIETest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const FIntPoint Size : {FIntPoint(1280,720),FIntPoint(1920,1080),FIntPoint(2560,1440)})
		for (int32 Scale : {100,125})
		{
			const FString Case = FString::Printf(TEXT("%dx%d_%d"),Size.X,Size.Y,Scale); Names.Add(Case); Commands.Add(Case);
		}
}

bool FResponsiveShellPIETest::RunTest(const FString& Parameters)
{
	if (RomanVisualRegression::UIRequested() || RomanVisualRegression::UICandidatesRequested())
	{
		float TextScale = 0;
		if (!TestTrue(TEXT("UI regression requires actual responsive raster capture"), FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureResponsive"))) ||
			!TestTrue(TEXT("Exactly one UI visual mode"), RomanVisualRegression::UIRequested() != RomanVisualRegression::UICandidatesRequested()) ||
			!TestTrue(TEXT("Explicit canonical text scale1 or2"), FParse::Value(FCommandLine::Get(), TEXT("RomanUITextScale="), TextScale) && (TextScale == 1.f || TextScale == 2.f))) return false;
	}
	TArray<FString> Parts; Parameters.ParseIntoArray(Parts,TEXT("_"));
	TArray<FString> Dimensions; if (Parts.Num()==2) Parts[0].ParseIntoArray(Dimensions,TEXT("x"));
	if (!TestEqual(TEXT("Responsive case has two dimensions"),Dimensions.Num(),2) ||
		!TestNotNull(TEXT("Responsive test requires editor"),GEditor) || !TestNull(TEXT("Responsive test owns its session"),GEditor->PlayWorld)) return false;
	const int32 Width = FCString::Atoi(*Dimensions[0]), Height = FCString::Atoi(*Dimensions[1]);
	const float Scale = FCString::Atof(*Parts[1])/100.f;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE(Width,Height));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseResponsiveShell(this,Width,Height,Scale));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapLifecyclePIETest, "RomanEmpireGame.UI.MinimapLifecyclePIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMinimapLifecyclePIETest::RunTest(const FString&)
{
	if (!TestTrue(TEXT("Minimap fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseMinimap(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTravelVictoryPIETest, "RomanEmpireGame.Campaign.SessionTravelVictoryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTravelVictoryPIETest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Travel fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignTravel(this, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTravelDefeatPIETest, "RomanEmpireGame.Campaign.SessionTravelDefeatPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTravelDefeatPIETest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Travel fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignTravel(this, false));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTravelContentRecoveryPIETest, "RomanEmpireGame.Campaign.SessionTravelContentRecoveryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTravelContentRecoveryPIETest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Recovery fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignRecovery(this, false));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTravelNonceRecoveryPIETest, "RomanEmpireGame.Campaign.SessionTravelNonceRecoveryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignTravelNonceRecoveryPIETest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Recovery fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignRecovery(this, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignSessionUIFlowsTest, "RomanEmpireGame.Campaign.SessionUIFlows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FCampaignSessionUIFlowsTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("Victory"), TEXT("Defeat"), TEXT("GlobalVictory")}) { Names.Add(Name); Commands.Add(Name); }
}
bool FCampaignSessionUIFlowsTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Campaign UI fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignTravel(this, Parameters != TEXT("Defeat"), true, Parameters == TEXT("GlobalVictory")));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignSessionUIRecoveryTest, "RomanEmpireGame.Campaign.SessionUIRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FCampaignSessionUIRecoveryTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("Content"), TEXT("Nonce")}) { Names.Add(Name); Commands.Add(Name); }
}
bool FCampaignSessionUIRecoveryTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Campaign recovery UI fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignRecovery(this, Parameters == TEXT("Nonce"), true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

void UCampaignSaveProbe::OnSave()
{
	if (!Test || !Save.IsValid()) return;
	++SaveNotifications;
	Test->TestFalse(TEXT("Publication prevents nested persistence eligibility"), Save->CanSaveCampaign() || Save->CanLoadCampaign());
	Test->TestEqual(TEXT("Save-state listener cannot recursively save"), Save->SaveCampaign(), ECampaignSaveFailure::Busy);
	Test->TestEqual(TEXT("Save-state listener cannot recursively load"), Save->LoadCampaign(), ECampaignSaveFailure::Busy);
}

void UCampaignSaveProbe::OnStep(int32 Turn, ECampaignTurnStep Step)
{
	(void)Turn; (void)Step;
	if (!Test || !Save.IsValid()) return;
	++TurnStages;
	if (bBreakMap && Target.IsValid() && Step == ECampaignTurnStep::ConflictResolution) Target->TacticalMap.Reset();
	Test->TestEqual(TEXT("Public save never bypasses the active turn guard"), Save->SaveCampaign(), ECampaignSaveFailure::UnsafeBoundary);
	Test->TestEqual(TEXT("Public load never bypasses the active turn guard"), Save->LoadCampaign(), ECampaignSaveFailure::UnsafeBoundary);
}

void UCampaignSaveProbe::OnSession()
{
	// Actual load broadcasts after source freeze; staging must remain rooted across observer GC.
	if (Test && Save.IsValid()) CollectGarbage(RF_NoFlags);
}

void UCampaignSaveProbe::OnReady()
{
	if (!Test || !State.IsValid() || !Save.IsValid()) return;
	++ReadyNotifications;
	Test->TestEqual(TEXT("First readiness exposes loaded turn"), State->GetCurrentTurn(), Expected.Campaign.Turn.CurrentTurn);
	Test->TestEqual(TEXT("First readiness exposes loaded preferences"), Save->GetSettings().bAutosaveEnabled, Expected.Settings.bAutosaveEnabled);
	for (const auto& Wallet : Expected.Campaign.Factions)
	{
		const auto Actual = State->GetFactionResources(Wallet.Faction);
		Test->TestTrue(TEXT("First readiness exposes all six saved currencies"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Actual, &Wallet.Resources, 0));
	}
	for (const auto& Army : Expected.Campaign.Armies)
	{
		FCampaignArmy Actual;
		Test->TestTrue(TEXT("First readiness exposes saved army ID and quantity"), State->TryGetCampaignArmy(Army.ArmyID, Actual) && FCampaignArmy::StaticStruct()->CompareScriptStruct(&Actual, &Army, 0));
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignSaveDiskTest, "RomanEmpireGame.Campaign.SaveDisk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FCampaignSaveDiskTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{ for (const TCHAR* Name : {TEXT("RoundTrip"), TEXT("Backup"), TEXT("Guards"), TEXT("StorageConflicts"), TEXT("Autosave"), TEXT("IOFailure"), TEXT("QueuedAI"), TEXT("UI"), TEXT("Resume"), TEXT("Recovery")}) { Names.Add(Name); Commands.Add(Name); } }
bool FCampaignSaveDiskTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Disk save fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignSave(this, Parameters));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignAIExecutionTest, "RomanEmpireGame.Campaign.AIExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FCampaignAIExecutionTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{ for (const TCHAR* Name : {TEXT("Victory"), TEXT("Defeat"), TEXT("Draw"), TEXT("RetryVictory"), TEXT("Relief")}) { Names.Add(Name); Commands.Add(Name); } }
bool FCampaignAIExecutionTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("AI execution fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignAIExecution(this, Parameters));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAIQueryTest, "RomanEmpireGame.Campaign.AIQueryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignAIQueryTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("AI query fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignAIQuery(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignArmyActionsUITest, "RomanEmpireGame.Campaign.ArmyActionsUI",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FCampaignArmyActionsUITest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{ for (const TCHAR* Name : {TEXT("Victory"), TEXT("Defeat")}) { Names.Add(Name); Commands.Add(Name); } }
bool FCampaignArmyActionsUITest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Army UI fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignArmyUI(this, Parameters == TEXT("Victory")));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCampaignFirstPlayableTest, "RomanEmpireGame.Campaign.FirstPlayable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FCampaignFirstPlayableTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{ for (const TCHAR* Name : {TEXT("Victory"), TEXT("Defeat")}) { Names.Add(Name); Commands.Add(Name); } }
bool FCampaignFirstPlayableTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("First-playable fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignArmyUI(this, Parameters == TEXT("Victory"), true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignArmyActionsPIETest, "RomanEmpireGame.Campaign.ArmyActionsPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignArmyActionsPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Army actions fixture owns PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignArmyActions(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

void UCampaignArmyActionsProbe::CheckCommittedAndReentry()
{
	using namespace RomanEmpireUITests;
	if (!Test || !State.IsValid() || !Manager.IsValid() || !Factions.IsValid()) { bAllCommitted = false; return; }
	const auto Wallet = State->GetFactionResources(EFactionID::Rome);
	int32 Total = 0, Count = 0;
	for (const auto& Army : State->GetCampaignArmies()) if (Army.Faction == EFactionID::Rome) { Total += FCampaignArmyState::GetUnitCount(Army); ++Count; }
	bAllCommitted &= Total == ExpectedTotal && Count == ExpectedArmies && FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &ExpectedWallet, 0);
	FCall Quote(*Test, Manager.Get(), TEXT("GetCampaignRecruitmentQuote"), 5);
	Quote.SetName(TEXT("TerritoryID"), TEXT("Rome")); Quote.SetName(TEXT("UnitID"), TEXT("Legionary")); Quote.SetInt(TEXT("Count"), 1); Quote.Execute();
	Test->TestTrue(TEXT("Publication query can refresh UI from fully committed state"), Quote.ReadStruct<FCampaignRecruitmentQuote>().IsAvailable());
	FCall Recruit(*Test, Manager.Get(), TEXT("RecruitCampaignUnits"), 5);
	Recruit.SetName(TEXT("TerritoryID"), TEXT("Rome")); Recruit.SetName(TEXT("UnitID"), TEXT("Legionary")); Recruit.SetInt(TEXT("Count"), 1); Recruit.Execute();
	Test->TestEqual(TEXT("Publication blocks nested purchase"), Recruit.ReadStruct<FCampaignArmyMutationResult>().Failure, ECampaignArmyFailure::MutationInProgress);
	FCall Assign(*Test, Manager.Get(), TEXT("AssignCampaignArmyUnits"), 5);
	Assign.SetGuid(TEXT("SourceArmyID"), SourceArmyID); Assign.SetGuid(TEXT("DestinationArmyID"), SourceArmyID); Assign.SetName(TEXT("UnitID"), TEXT("Legionary")); Assign.SetInt(TEXT("Count"), 1); Assign.Execute();
	Test->TestEqual(TEXT("Publication blocks nested assignment"), Assign.ReadStruct<FCampaignArmyMutationResult>().Failure, ECampaignArmyFailure::MutationInProgress);
	FFactionResourceDelta Credit; Credit.Gold = 1;
	FCall Resource(*Test, Factions.Get(), TEXT("ApplyFactionResourceDelta"), 3); Resource.SetEnum(TEXT("FactionID"), EFactionID::Rome); Resource.SetStruct(TEXT("Delta"), Credit); Resource.Execute();
	Test->TestEqual(TEXT("Publication blocks independent nested wallet mutation"), Resource.ReadStruct<FFactionResourceTransactionResult>().Failure, EResourceTransactionFailure::MutationInProgress);
	FCall Player(*Test, Factions.Get(), TEXT("SetPlayerFaction"), 1); Player.SetEnum(TEXT("NewFaction"), EFactionID::Carthage); Player.Execute();
	Test->TestEqual(TEXT("Publication cannot switch payer midway"), State->GetPlayerFaction(), EFactionID::Rome);
}
void UCampaignArmyActionsProbe::OnArmiesChanged() { ++ArmyEvents; CheckCommittedAndReentry(); }
void UCampaignArmyActionsProbe::OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts Balance)
{ (void)FactionID; (void)Balance; ++ResourceEvents; CheckCommittedAndReentry(); }

void UCampaignArmyProbe::OnArmiesChanged()
{
	++Notifications;
	LastObserved = State.IsValid() ? State->GetCampaignArmies() : TArray<FCampaignArmy>();
}

void UCampaignReinforcementProbe::CheckCommittedAndReentry()
{
	using namespace RomanEmpireUITests;
	if (!Test || !State.IsValid()) { bAllReadsCommitted = false; return; }
	FCampaignArmy Army;
	const FCampaignEncounterPlan Plan = State->GetCampaignEncounterPlan();
	const FCampaignArmy* Planned = Plan.Armies.FindByPredicate([&](const FCampaignArmy& Value) { return Value.ArmyID == ArmyID; });
	bAllReadsCommitted &= State->TryGetCampaignArmy(ArmyID, Army) && FCampaignArmyState::GetUnitCount(Army) == 16 &&
		Planned && FCampaignArmyState::GetUnitCount(*Planned) == 16 && Plan.EncounterID == EncounterID &&
		State->GetTacticalBattleResult().Squads.Num() == 3 && !State->HasAppliedCampaignEncounter(EncounterID);
	FCall Apply(*Test, Manager.Get(), TEXT("ApplyCurrentEncounterResult"), 1); Apply.Execute();
	Test->TestEqual(TEXT("Enrollment publication blocks nested result application"), Apply.ReadEnum<ECampaignEncounterFailure>(), ECampaignEncounterFailure::MutationInProgress);
	Test->TestFalse(TEXT("Enrollment cannot recursively cancel the completing producer"), ReadBool(*Test, Producer.Get(), TEXT("TryCancelTraining")));
	FCall Train(*Test, Producer.Get(), TEXT("TrainUnit"), 2); Train.SetName(TEXT("UnitID"), TEXT("Legionary")); Train.Execute();
	Test->TestFalse(TEXT("Enrollment cannot recursively charge another order"), Train.ReadBool());
}
void UCampaignReinforcementProbe::OnTacticalChanged(FTacticalBattleResult Result)
{ (void)Result; ++TacticalEvents; CheckCommittedAndReentry(); }
void UCampaignReinforcementProbe::OnArmiesChanged() { ++ArmyEvents; CheckCommittedAndReentry(); }
void UCampaignReinforcementProbe::OnEncounterChanged() { ++EncounterEvents; CheckCommittedAndReentry(); }

void UCampaignEncounterProbe::OnTacticalChanged(FTacticalBattleResult Result)
{
	if (!State.IsValid() || Result.Outcome != ETacticalBattleOutcome::InProgress || bSawPreparedPlan) return;
	const FCampaignEncounterPlan Plan = State->GetCampaignEncounterPlan();
	bSawPreparedPlan = Plan.EncounterID.IsValid() && Plan.EncounterID == Result.EncounterID && Plan.Armies.Num() == 2;
	EncounterID = Plan.EncounterID;
	if (Test && bSawPreparedPlan)
	{
		RomanEmpireUITests::FCall Nested(*Test, Manager.Get(), TEXT("ApplyCurrentEncounterResult"), 1); Nested.Execute();
		Test->TestEqual(TEXT("Begin notification cannot reenter campaign application"), Nested.ReadEnum<ECampaignEncounterFailure>(), ECampaignEncounterFailure::MutationInProgress);
	}
}

void UCampaignEncounterProbe::CheckCommittedAndReentry()
{
	if (!Test || !State.IsValid() || !Manager.IsValid() || !Map.IsValid()) { bAllReadsCommitted = false; return; }
	FTerritoryPresentationSnapshot Territory; State->TryGetTerritorySnapshot(TEXT("Africa"), Territory);
	FCampaignArmy Army;
	const bool bHasAttacker = State->TryGetCampaignArmy(ArmyID, Army);
	bAllReadsCommitted &= State->HasAppliedCampaignEncounter(EncounterID) && State->GetCampaignEncounterPlan().EncounterID == EncounterID &&
		Territory.OwnerFaction == (bVictory ? EFactionID::Rome : EFactionID::Carthage) &&
		(bVictory ? bHasAttacker && Army.TerritoryID == TEXT("Africa") && FCampaignArmyState::GetUnitCount(Army) == ExpectedSurvivors : !bHasAttacker);
	using RomanEmpireUITests::FCall;
	FCall Nested(*Test, Manager.Get(), TEXT("ApplyCurrentEncounterResult"), 1); Nested.Execute();
	Test->TestEqual(TEXT("Result notifications cannot reenter application"), Nested.ReadEnum<ECampaignEncounterFailure>(), ECampaignEncounterFailure::MutationInProgress);
	FCall Move(*Test, Manager.Get(), TEXT("MoveCampaignArmy"), 3); Move.SetGuid(TEXT("ArmyID"), ArmyID); Move.SetName(TEXT("TargetTerritoryID"), TEXT("Sicily")); Move.Execute();
	Test->TestEqual(TEXT("Result notifications cannot relocate committed army"), Move.ReadStruct<FCampaignArmyMutationResult>().Failure, ECampaignArmyFailure::MutationInProgress);
	FCall Owner(*Test, Map.Get(), TEXT("ChangeTerritoryOwner"), 3); Owner.SetName(TEXT("TerritoryID"), TEXT("Africa")); Owner.SetEnum(TEXT("NewOwner"), EFactionID::Gaul); Owner.Execute();
	Test->TestEqual(TEXT("Paired transaction blocks nested owner mutation"), Owner.ReadStruct<FTerritoryOwnershipChangeResult>().Failure, ETerritoryOwnershipFailure::TransactionInProgress);
	FCall Settlement(*Test, Map.Get(), TEXT("FoundTerritorySettlement"), 3); Settlement.SetName(TEXT("TerritoryID"), TEXT("Rome")); Settlement.SetText(TEXT("Name"), FText::FromString(TEXT("Nested"))); Settlement.Execute();
	Test->TestEqual(TEXT("Paired transaction blocks nested settlement mutation"), Settlement.ReadStruct<FSettlementFoundingResult>().Failure, ESettlementFoundingFailure::TransactionInProgress);
	FCall Turn(*Test, Manager.Get(), TEXT("ProcessTurn"), 1); Turn.Execute();
	Test->TestEqual(TEXT("Result notifications cannot process economy"), Turn.ReadStruct<FCampaignTurnResult>().Failure, ECampaignTurnFailure::EncounterPending);
	FCall Reset(*Test, Manager.Get(), TEXT("StartNewCampaign"), 1); Reset.SetEnum(TEXT("PlayerFaction"), EFactionID::Rome); Reset.Execute();
	Test->TestTrue(TEXT("Nested reset cannot erase consumed identity"), State->HasAppliedCampaignEncounter(EncounterID));
}

void UCampaignEncounterProbe::OnArmiesChanged() { ++ArmyEvents; CheckCommittedAndReentry(); }
void UCampaignEncounterProbe::OnTerritoryChanged(FName ID, EFactionID Before, EFactionID After)
{ (void)ID; (void)Before; (void)After; ++TerritoryEvents; CheckCommittedAndReentry(); }
void UCampaignEncounterProbe::OnEncounterChanged() { ++EncounterEvents; CheckCommittedAndReentry(); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterVictoryPIETest, "RomanEmpireGame.Campaign.EncounterVictoryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterVictoryPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Encounter fixture owns a fresh PIE session"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/RomanEmpireGame.RomanEmpireGameMode"));
	TSharedPtr<RomanEmpireUITests::FStrategicBootstrapScope> Scope = MakeShared<RomanEmpireUITests::FStrategicBootstrapScope>(Class);
	if (!TestNotNull(TEXT("Strategic bootstrap setting available"), Scope->Property)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignEncounter(this, true, Scope));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignEncounterDefeatPIETest, "RomanEmpireGame.Campaign.EncounterDefeatPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignEncounterDefeatPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Encounter fixture owns a fresh PIE session"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/RomanEmpireGame.RomanEmpireGameMode"));
	TSharedPtr<RomanEmpireUITests::FStrategicBootstrapScope> Scope = MakeShared<RomanEmpireUITests::FStrategicBootstrapScope>(Class);
	if (!TestNotNull(TEXT("Strategic bootstrap setting available"), Scope->Property)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignEncounter(this, false, Scope));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignReinforcementPIETest, "RomanEmpireGame.Campaign.EncounterReinforcementPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignReinforcementPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Reinforcement fixture owns fresh PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/RomanEmpireGame.RomanEmpireGameMode"));
	TSharedPtr<RomanEmpireUITests::FStrategicBootstrapScope> Scope = MakeShared<RomanEmpireUITests::FStrategicBootstrapScope>(Class);
	if (!TestNotNull(TEXT("Strategic bootstrap setting available"), Scope->Property)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignEncounter(this, true, Scope, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignReinforcementDefeatPIETest, "RomanEmpireGame.Campaign.EncounterReinforcementDefeatPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignReinforcementDefeatPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Reinforcement defeat fixture owns fresh PIE"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/RomanEmpireGame.RomanEmpireGameMode"));
	TSharedPtr<RomanEmpireUITests::FStrategicBootstrapScope> Scope = MakeShared<RomanEmpireUITests::FStrategicBootstrapScope>(Class);
	if (!TestNotNull(TEXT("Strategic bootstrap setting available"), Scope->Property)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignEncounter(this, false, Scope, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignArmyPIETest,
	"RomanEmpireGame.Campaign.ArmiesPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignArmyPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Army fixture owns a fresh PIE session"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCampaignArmies(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTacticalVictoryRestartPIETest,
	"RomanEmpireGame.Battle.TerminalVictoryRestartPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FTacticalVictoryRestartPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Terminal fixture owns a fresh PIE session"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseTacticalTerminal(this, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTacticalDefeatRestartPIETest,
	"RomanEmpireGame.Battle.TerminalDefeatRestartPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FTacticalDefeatRestartPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestTrue(TEXT("Terminal fixture owns a fresh PIE session"), GEditor && !GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseTacticalTerminal(this, false));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
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
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseCanonicalUI(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

void UTrainingRefundProbe::OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance)
{
	(void)FactionID; (void)NewBalance;
	++Notifications;
	if (!Test || !Producer.IsValid()) return;
	Test->TestFalse(TEXT("Nested refund callback cannot cancel/refund a second order"),
		RomanEmpireUITests::ReadBool(*Test, Producer.Get(), TEXT("TryCancelTraining")));
	RomanEmpireUITests::FCall Train(*Test, Producer.Get(), TEXT("TrainUnit"), 2);
	Train.SetName(TEXT("UnitID"), UnitID);
	Train.Execute();
	Test->TestFalse(TEXT("Nested refund callback cannot append an order"), Train.ReadBool());
	if (AActor* Actor = Cast<AActor>(Producer.Get())) Actor->Tick(1000.0f);
}

void UTrainingRefundProbe::OnProductionChanged(ABarracks* Building, FProductionEvent Event)
{
	(void)Building;
	if (Event.Type == EProductionEventType::Failed) ++FailedOrders;
	if (Event.Type == EProductionEventType::Completed) ++CompletedOrders;
	if (Event.Type == EProductionEventType::RefundPending) ++PendingRefundEvents;
}

void UTerritoryProjectionProbe::OnSourceOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner)
{
	(void)PreviousOwner;
	++SourceNotifications;
	if (!Test || !bNestOnce || TerritoryID != FName(TEXT("Rome")) || NewOwner != EFactionID::Carthage) return;
	bNestOnce = false;
	RomanEmpireUITests::FCall Change(*Test, WorldMap.Get(), TEXT("ChangeTerritoryOwner"), 3);
	Change.SetName(TEXT("TerritoryID"), TerritoryID);
	Change.SetEnum(TEXT("NewOwner"), EFactionID::Rome);
	Change.Execute();
	Test->TestTrue(TEXT("Earlier owner observer commits a nested restore"),
		Change.ReadStruct<FTerritoryOwnershipChangeResult>().bOwnershipChanged);
}

void UTerritoryProjectionProbe::OnPublicOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner)
{
	(void)PreviousOwner;
	PublicOwners.Add(NewOwner);
	FTerritoryPresentationSnapshot Current;
	if (!Test || !State.IsValid()) return;
	Test->TestTrue(TEXT("Public event arrives after readable committed ownership"), State->TryGetTerritorySnapshot(TerritoryID, Current));
	Test->TestEqual(TEXT("Public event owner matches a fresh canonical read"), Current.OwnerFaction, NewOwner);
	if (bNestPublicOnce && TerritoryID == FName(TEXT("Rome")) && NewOwner == EFactionID::Carthage)
	{
		bNestPublicOnce = false;
		RomanEmpireUITests::FCall Change(*Test, WorldMap.Get(), TEXT("ChangeTerritoryOwner"), 3);
		Change.SetName(TEXT("TerritoryID"), TerritoryID);
		Change.SetEnum(TEXT("NewOwner"), EFactionID::Rome);
		Change.Execute();
		Test->TestTrue(TEXT("Earlier public observer overtakes an ownership notification"),
			Change.ReadStruct<FTerritoryOwnershipChangeResult>().bOwnershipChanged);
	}
}

void UTerritoryProjectionProbe::OnPublicSettlementChanged(FName TerritoryID)
{
	++SettlementNotifications;
	if (!Test || !State.IsValid()) return;
	FTerritoryPresentationSnapshot Row;
	Test->TestTrue(TEXT("Settlement notification reads committed live state"), State->TryGetTerritorySnapshot(TerritoryID, Row) && Row.Settlement.bHasSettlement);
	if (bNestSettlementOnce && TerritoryID == FName(TEXT("Sicily")))
	{
		bNestSettlementOnce = false;
		RomanEmpireUITests::FCall Found(*Test, WorldMap.Get(), TEXT("FoundTerritorySettlement"), 3);
		Found.SetName(TEXT("TerritoryID"), TEXT("Greece"));
		Found.SetText(TEXT("Name"), FText::FromString(TEXT("Athens Test")));
		Found.Execute();
		Test->TestTrue(TEXT("Settlement observer commits nested second founding"), Found.ReadStruct<FSettlementFoundingResult>().WasSuccessful());
	}
}

void UPlacementTransactionProbe::OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance)
{
	(void)FactionID;
	(void)NewBalance;
	using namespace RomanEmpireUITests;
	++WalletNotifications;
	// Bound recursion ensures a broken implementation reports failure, not a stack overflow.
	if (!Test || WalletNotifications > 1) return;
	if (bDestroyDeferredDuringSpend)
	{
		UActorComponent* Component = Cast<UActorComponent>(Placement.Get());
		UWorld* World = Component ? Component->GetWorld() : nullptr;
		if (World && BuildingClass.IsValid())
		{
			for (TActorIterator<AActor> It(World, BuildingClass.Get()); It; ++It)
			{
				if (!It->HasActorBegunPlay() && It->Destroy())
				{
					++DestroyedDeferredCount;
					break;
				}
			}
		}
		return;
	}
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
			const URomanTerritoryDefinition* OwnedTerritory = nullptr;
			const URomanTerritoryDefinition* ForeignTerritory = nullptr;
			for (const URomanTerritoryDefinition* Territory : Registry->GetAllTerritories())
			{
				if (Territory->Definition.StartingOwner == EFactionID::Rome && !OwnedTerritory) OwnedTerritory = Territory;
				if (Territory->Definition.StartingOwner != EFactionID::Rome &&
					Territory->Definition.StartingOwner != EFactionID::None && !ForeignTerritory) ForeignTerritory = Territory;
			}
			if (!Test.TestTrue(TEXT("Authored owned and foreign territories exist"), OwnedTerritory && ForeignTerritory)) return;
			const FVector OwnedCenter = OwnedTerritory->MapLocation;
			Placement->SetComponentTickEnabled(false);
			for (int32 Scenario = 0; Scenario < 2; ++Scenario)
			{
				Test.PushContext(Scenario == 0 ? TEXT("Recursive observers") : TEXT("Cancel and restart during committed spend"));
				FCall Start(Test, Placement, TEXT("StartPlacement"), 1);
				Start.SetName(TEXT("BuildingID"), Expected.BuildingID);
				Start.Execute();
				FCall Move(Test, Placement, TEXT("UpdatePreview"), 1);
				Move.SetVector(TEXT("WorldLocation"), FVector(50000.0, 50000.0, -10.0));
				Move.Execute();
				Test.TestFalse(TEXT("Footprint outside ground cannot be placed"), ReadBool(Test, Placement, TEXT("CanPlace")));
				Move.SetVector(TEXT("WorldLocation"), FVector(-600.0, -800.0, -10.0));
				Move.Execute();
				Test.TestFalse(TEXT("Existing physical barracks blocks placement"), ReadBool(Test, Placement, TEXT("CanPlace")));
				if (Scenario == 0)
				{
					FVector ForeignSite = ForeignTerritory->MapLocation;
					ForeignSite.Z = -10.0;
					Move.SetVector(TEXT("WorldLocation"), ForeignSite);
					Move.Execute();
					Test.TestFalse(TEXT("Foreign territory rejects placement"), ReadBool(Test, Placement, TEXT("CanPlace")));
				}
				const FVector ValidSite(OwnedCenter.X + 3000.0 + Scenario * 1000.0, OwnedCenter.Y + 3000.0, -10.0);
				Move.SetVector(TEXT("WorldLocation"), ValidSite);
				Move.Execute();
				Test.TestTrue(TEXT("Grounded, clear preview can be committed"), ReadBool(Test, Placement, TEXT("CanPlace")));
				if (Scenario == 0)
				{
					const FFactionResourceAmounts BeforeBlocked = ReadResources(Test, Manager);
					UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
					FActorSpawnParameters Params;
					Params.ObjectFlags |= RF_Transient;
					AStaticMeshActor* Blocker = World->SpawnActor<AStaticMeshActor>(
						FVector(ValidSite.X, ValidSite.Y, 190.0), FRotator::ZeroRotator, Params);
					if (!Test.TestTrue(TEXT("Transient blocker fixture exists"), Blocker && Cube)) { Test.PopContext(); return; }
					UStaticMeshComponent* Mesh = Blocker->GetStaticMeshComponent();
					Mesh->SetMobility(EComponentMobility::Movable);
					Mesh->SetStaticMesh(Cube);
					Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					Mesh->SetCollisionResponseToAllChannels(ECR_Block);
					Blocker->SetActorScale3D(FVector(4.0));
					Test.AddExpectedMessage(TEXT("Cannot confirm placement: invalid site"),
						ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
					Test.TestFalse(TEXT("New blocker invalidates stale preview at confirmation"), ReadBool(Test, Placement, TEXT("ConfirmPlacement")));
					const FFactionResourceAmounts AfterBlocked = ReadResources(Test, Manager);
					Test.TestTrue(TEXT("Blocked confirmation never charges resources"),
						FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&BeforeBlocked, &AfterBlocked, 0));
					Test.TestTrue(TEXT("Blocked confirmation preserves retryable preview"), ReadBool(Test, Placement, TEXT("IsPlacing")));
					Blocker->Destroy();
				}
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
			Move.SetVector(TEXT("WorldLocation"), FVector(OwnedCenter.X + 3000.0, OwnedCenter.Y + 4000.0, -10.0));
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

			// An observer can retire a deferred actor during the committed spend.
			// That post-charge failure must refund every resource and publish no building.
			FFactionResourceDelta RefundFixtureFunding;
			RefundFixtureFunding.Gold = Expected.Cost.Gold;
			RefundFixtureFunding.Food = Expected.Cost.Food;
			RefundFixtureFunding.Iron = Expected.Cost.Iron;
			RefundFixtureFunding.Wood = Expected.Cost.Wood;
			RefundFixtureFunding.Stone = Expected.Cost.Stone;
			RefundFixtureFunding.Population = Expected.Cost.Population;
			FCall FundRefundFixture(Test, Manager, TEXT("ApplyFactionResourceDelta"), 3);
			FundRefundFixture.SetEnum(TEXT("FactionID"), EFactionID::Rome);
			FundRefundFixture.SetStruct(TEXT("Delta"), RefundFixtureFunding);
			FundRefundFixture.Execute();
			FCall FailStart(Test, Placement, TEXT("StartPlacement"), 1);
			FailStart.SetName(TEXT("BuildingID"), Expected.BuildingID);
			FailStart.Execute();
			FCall FailMove(Test, Placement, TEXT("UpdatePreview"), 1);
			FailMove.SetVector(TEXT("WorldLocation"), FVector(OwnedCenter.X + 4000.0, OwnedCenter.Y + 4000.0, -10.0));
			FailMove.Execute();
			if (!Test.TestTrue(TEXT("Post-charge failure fixture starts from a valid site"), ReadBool(Test, Placement, TEXT("CanPlace")))) return;
			const FFactionResourceAmounts BeforeSpawnFailure = ReadResources(Test, Manager);
			int32 BuildingsBeforeSpawnFailure = 0;
			for (TActorIterator<AActor> It(World, BuildingClass); It; ++It) ++BuildingsBeforeSpawnFailure;
			TStrongObjectPtr<UPlacementTransactionProbe> FailureProbe(NewObject<UPlacementTransactionProbe>());
			FailureProbe->Test = &Test;
			FailureProbe->Placement = Placement;
			FailureProbe->BuildingClass = BuildingClass;
			FailureProbe->bDestroyDeferredDuringSpend = true;
			FScopedProbeBinding FailureWalletBinding(Test, Manager, TEXT("OnFactionResourcesChanged"), FailureProbe.Get(), TEXT("OnResourcesChanged"));
			FScopedProbeBinding FailurePlacedBinding(Test, Placement, TEXT("OnBuildingPlaced"), FailureProbe.Get(), TEXT("OnPlaced"));
			if (!FailureWalletBinding.bValid || !FailurePlacedBinding.bValid) return;
			Test.AddExpectedMessage(TEXT("Building spawn failed; resources refunded"),
				ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);
			Test.TestFalse(TEXT("Destroyed deferred actor prevents placement completion"), ReadBool(Test, Placement, TEXT("ConfirmPlacement")));
			Test.TestEqual(TEXT("Failure observer destroyed exactly one deferred actor"), FailureProbe->DestroyedDeferredCount, 1);
			Test.TestEqual(TEXT("Spend and compensating refund each emit one wallet event"), FailureProbe->WalletNotifications, 2);
			Test.TestEqual(TEXT("Failed spawn emits no placed event"), FailureProbe->PlacedNotifications, 0);
			const FFactionResourceAmounts AfterSpawnFailure = ReadResources(Test, Manager);
			Test.TestTrue(TEXT("Post-charge failure refunds all six resource fields"),
				FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&BeforeSpawnFailure, &AfterSpawnFailure, 0));
			int32 BuildingsAfterSpawnFailure = 0;
			for (TActorIterator<AActor> It(World, BuildingClass); It; ++It) ++BuildingsAfterSpawnFailure;
			Test.TestEqual(TEXT("Failed spawn leaves no additional building"), BuildingsAfterSpawnFailure, BuildingsBeforeSpawnFailure);
			Test.TestTrue(TEXT("Failed spawn keeps the preview retryable"), ReadBool(Test, Placement, TEXT("IsPlacing")));
			FCall(Test, Placement, TEXT("CancelPlacement"), 0).Execute();
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
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExerciseDefinitionPlacement(this, Definition));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FRestoreDefinition(this, Definition));
	return true;
}

void UBuildingSelectionProbe::OnSelection(const TArray<AUnitBase*>& Units, ABuildingBase* Building)
{
	(void)Units;
	if (!Test || !bArmed || reinterpret_cast<UObject*>(Building) != Target.Get()) return;
	bArmed = false; // Nested clear-selection notification must not recurse.
	++Interventions;
	RomanEmpireUITests::FCall Owner(*Test, Target.Get(), TEXT("SetOwnerFaction"), 1);
	Owner.SetEnum(TEXT("NewOwner"), EFactionID::None);
	Owner.Execute();
}

namespace RomanEmpireUITests
{
	class FExercisePlayerBuildingSelection : public IAutomationLatentCommand
	{
	public:
		explicit FExercisePlayerBuildingSelection(FAutomationTestBase* InTest) : Test(*InTest) {}
		virtual bool Update() override
		{
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			if (!World || !World->HasBegunPlay() || !Controller)
			{
				if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
				Test.AddError(TEXT("Building selection regression did not reach PIE startup within 30 seconds"));
				return true;
			}
			if (!bTacticalRequested)
			{
				Controller->SetActorTickEnabled(false);
				FCall Mode(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
				Mode.SetEnum(TEXT("NewMode"), EGameplayMode::Tactical);
				Mode.Execute();
				bTacticalRequested = true;
				return false;
			}
			Exercise(World, Controller);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		bool bTacticalRequested = false;

		void Exercise(UWorld* World, APlayerController* Controller)
		{
			UUserWidget* Root = Cast<UUserWidget>(ReadObject(Test, Controller, TEXT("GetMainWidget")));
			UButton* Train = Root ? Cast<UButton>(Root->GetWidgetFromName(TEXT("TrainButton"))) : nullptr;
			UTextBlock* Selection = Root ? Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("SelectionText"))) : nullptr;
			UWidget* Panel = Root ? Root->GetWidgetFromName(TEXT("SelectionPanel")) : nullptr;
			UObject* Manager = ReadObject(Test, World->GetAuthGameMode(), TEXT("GetFactionManager"));
			UClass* BarracksClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.Barracks")).TryLoadClass<AActor>();
			if (!Test.TestTrue(TEXT("Normal controller, UI and content authority are ready"), Root && Train && Selection && Panel && Manager && BarracksClass)) return;
			FCall Player(Test, Manager, TEXT("GetPlayerFaction"), 1);
			Player.Execute();
			const EFactionID PlayerFaction = Player.ReadEnum<EFactionID>();
			if (!Test.TestEqual(TEXT("Committed testbed uses Rome as canonical player faction"), PlayerFaction, EFactionID::Rome)) return;
			const auto Owner = [&](AActor* Building, EFactionID Faction)
			{
				FCall Call(Test, Building, TEXT("SetOwnerFaction"), 1);
				Call.SetEnum(TEXT("NewOwner"), Faction);
				Call.Execute();
			};
			const auto Select = [&](AActor* Building)
			{
				FCall Call(Test, Controller, TEXT("SelectBuilding"), 1);
				Call.SetObject(TEXT("Building"), Building);
				Call.Execute();
			};
			const auto Eligible = [&](AActor* Building)
			{
				FCall Call(Test, Controller, TEXT("IsPlayerBuildingEligible"), 2);
				Call.SetObject(TEXT("Building"), Building);
				Call.Execute();
				return Call.ReadBool();
			};
			int32 SpawnIndex = 0;
			const auto Spawn = [&]() -> AActor*
			{
				FActorSpawnParameters Parameters;
				Parameters.ObjectFlags |= RF_Transient;
				Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Actor = World->SpawnActor<AActor>(BarracksClass,
					FVector(60000.0 + 1000.0 * SpawnIndex++, 60000.0, 300.0), FRotator::ZeroRotator, Parameters);
				if (Actor) Actor->SetActorTickEnabled(false);
				return Actor;
			};
			AActor* Own = Spawn();
			AActor* Other = Spawn();
			if (!Test.TestTrue(TEXT("Transient normal-BeginPlay building fixtures spawned"), Own && Other)) return;
			Owner(Own, PlayerFaction);
			FCall(Test, Other, TEXT("StartConstruction"), 0).Execute();
			const auto Rejected = [&](const TCHAR* Label, AActor* Building)
			{
				Test.PushContext(Label);
				Test.TestFalse(TEXT("Public player eligibility rejects building"), Eligible(Building));
				Select(Building);
				Test.TestNull(TEXT("Rejected building does not become canonical selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")));
				if (IsValid(Building)) Test.TestFalse(TEXT("Rejected building has no selected presentation"), ReadBool(Test, Building, TEXT("IsSelected")));
				Test.TestFalse(TEXT("Rejected building has no visible selection/TRAIN panel"), Panel->IsVisible());
				Test.PopContext();
			};
			Rejected(TEXT("Own placing building"), Own);
			Rejected(TEXT("No owner"), Other);
			Owner(Other, EFactionID::Carthage);
			Rejected(TEXT("Enemy constructing building"), Other);
			const FTransform DeferredTransform(FVector(65000.0, 60000.0, 300.0));
			AActor* Deferred = World->SpawnActorDeferred<AActor>(BarracksClass, DeferredTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Test.TestNotNull(TEXT("Deferred building fixture exists"), Deferred)) return;
			// Default owner is authored input to normal BeginPlay, not a forged readiness flag.
			FEnumProperty* OwnerProperty = FindFProperty<FEnumProperty>(Deferred->GetClass(), TEXT("OwnerFaction"));
			if (!Test.TestNotNull(TEXT("Deferred owner is reflected"), OwnerProperty)) { Deferred->Destroy(); return; }
			OwnerProperty->GetUnderlyingProperty()->SetIntPropertyValue(OwnerProperty->ContainerPtrToValuePtr<void>(Deferred), static_cast<int64>(PlayerFaction));
			FCall(Test, Deferred, TEXT("StartConstruction"), 0).Execute();
			Rejected(TEXT("Deferred actor before initialization and BeginPlay"), Deferred);
			Deferred->FinishSpawning(DeferredTransform);
			Deferred->SetActorTickEnabled(false);
			Test.TestTrue(TEXT("Normal BeginPlay makes deferred own construction selectable"), Eligible(Deferred));
			FCall(Test, Own, TEXT("StartConstruction"), 0).Execute();
			const auto Click = [&](AActor* Actor, bool bAdditive)
			{
				FCall Call(Test, Controller, TEXT("SelectActorFromInput"), 2);
				Call.SetObject(TEXT("Actor"), Actor);
				Call.SetBool(TEXT("bAdditive"), bAdditive);
				Call.Execute();
			};
			Click(Own, true);
			Test.TestEqual(TEXT("Additive building click selects an eligible building"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Own));
			Click(Own, true);
			Test.TestEqual(TEXT("Additive building click does not toggle buildings"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Own));
			Click(Other, false);
			Test.TestEqual(TEXT("Enemy building click preserves existing eligible selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Own));
			UTextBlock* Feedback = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("InteractionFeedbackText")));
			Test.TestTrue(TEXT("Enemy building click reports visible feedback"), Feedback && Feedback->IsVisible() && !Feedback->GetText().IsEmpty());
			Click(nullptr, true);
			Test.TestEqual(TEXT("Additive empty click preserves building selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Own));
			Click(nullptr, false);
			Test.TestNull(TEXT("Plain empty click clears building selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")));
			Select(Own);
			Test.TestEqual(TEXT("Own constructing building is canonical selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Own));
			Test.TestTrue(TEXT("Own constructing building sets selected presentation"), ReadBool(Test, Own, TEXT("IsSelected")));
			Test.TestFalse(TEXT("Construction has no TRAIN affordance"), Train->IsVisible());
			FCall Data(Test, Own, TEXT("GetBuildingData"), 1);
			Data.Execute();
			const FBuildingData Definition = Data.ReadStruct<FBuildingData>();
			Own->Tick(Definition.ConstructionTime + 0.1f);
			Test.TestTrue(TEXT("Own completed producer is eligible"), Eligible(Own));
			Test.TestTrue(TEXT("Completion event exposes TRAIN"), Train->IsVisible());
			const FFactionResourceAmounts BeforeTrain = ReadResources(Test, Manager);
			ClickThroughSlate(Test, Root, Train);
			Test.TestTrue(TEXT("Live Slate TRAIN starts the selected own producer"), ReadBool(Test, Own, TEXT("IsTraining")));
			Test.TestTrue(TEXT("Authorized training spends canonical resources"), ReadResources(Test, Manager).Gold < BeforeTrain.Gold);

			const auto Cleared = [&](const TCHAR* Label, AActor* Building)
			{
				Test.PushContext(Label);
				Test.TestNull(TEXT("Committed invalidation clears selection synchronously"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")));
				Test.TestFalse(TEXT("Invalidation clears selected presentation"), ReadBool(Test, Building, TEXT("IsSelected")));
				Test.TestFalse(TEXT("Invalidation removes selection/TRAIN panel synchronously"), Panel->IsVisible());
				const FFactionResourceAmounts Before = ReadResources(Test, Manager);
				const int32 PendingBefore = ReadInt(Test, Building, TEXT("GetPendingOrderCount"));
				const bool bTrainingBefore = ReadBool(Test, Building, TEXT("IsTraining"));
				// Simulate a previously queued UI callback after the visible affordance disappears.
				FCall(Test, Root, TEXT("HandleTrainClicked"), 0).Execute();
				const FFactionResourceAmounts After = ReadResources(Test, Manager);
				Test.TestTrue(TEXT("Stale TRAIN callback spends no wallet fields"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Before, &After, 0));
				Test.TestEqual(TEXT("Stale TRAIN callback preserves pending orders"), ReadInt(Test, Building, TEXT("GetPendingOrderCount")), PendingBefore);
				Test.TestEqual(TEXT("Stale TRAIN callback preserves active training"), ReadBool(Test, Building, TEXT("IsTraining")), bTrainingBefore);
				Test.PopContext();
			};
			Owner(Own, EFactionID::Carthage);
			Cleared(TEXT("Owner loss"), Own);
			Other->Tick(Definition.ConstructionTime + 0.1f);
			if (Test.TestFalse(TEXT("Authored producer has at least one trainable unit"), Definition.TrainableUnits.IsEmpty()))
			{
				FCall DirectTraining(Test, Other, TEXT("TrainUnit"), 2);
				DirectTraining.SetName(TEXT("UnitID"), Definition.TrainableUnits[0]);
				DirectTraining.Execute();
				Test.TestTrue(TEXT("Enemy simulation still accepts direct training independent of player selection"), DirectTraining.ReadBool());
			}
			Owner(Own, PlayerFaction);
			Select(Own);
			FCall PlayerLoss(Test, Manager, TEXT("SetPlayerFaction"), 1);
			PlayerLoss.SetEnum(TEXT("NewFaction"), EFactionID::Carthage);
			PlayerLoss.Execute();
			FCall ChangedPlayer(Test, Manager, TEXT("GetPlayerFaction"), 1);
			ChangedPlayer.Execute();
			Test.TestEqual(TEXT("Canonical player faction switch commits"), ChangedPlayer.ReadEnum<EFactionID>(), EFactionID::Carthage);
			Cleared(TEXT("Canonical player faction loss"), Own);
			Rejected(TEXT("Building no longer belongs to canonical player faction"), Own);
			FCall PlayerRestore(Test, Manager, TEXT("SetPlayerFaction"), 1);
			PlayerRestore.SetEnum(TEXT("NewFaction"), PlayerFaction);
			PlayerRestore.Execute();
			Select(Own);
			Select(Deferred);
			Owner(Own, EFactionID::None);
			Test.TestEqual(TEXT("Old building event preserves new canonical selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Deferred));
			Test.TestEqual(TEXT("Controller releases old building observer"), ListenerCount(Test, Own, TEXT("OnBuildingStateChanged"), Controller), 0);
			FCall Placing(Test, Deferred, TEXT("SetPlacementMode"), 2);
			Placing.SetBool(TEXT("bPlacing"), true);
			Placing.SetBool(TEXT("bValidPlacement"), true);
			Placing.Execute();
			Cleared(TEXT("Selected building returns to placement"), Deferred);

			// Reorder the real UI listener after the adversarial listener without changing its handler.
			TStrongObjectPtr<UBuildingSelectionProbe> Probe(NewObject<UBuildingSelectionProbe>());
			Probe->Test = &Test;
			Probe->Target = Own;
			Probe->bArmed = true;
			FMulticastDelegateProperty* Event = FindFProperty<FMulticastDelegateProperty>(Controller->GetClass(), TEXT("OnSelectionChanged"));
			FScriptDelegate Listener;
			Listener.BindUFunction(Probe.Get(), TEXT("OnSelection"));
			if (!Test.TestTrue(TEXT("Nested building probe matches selection signature"), Event && Event->SignatureFunction->IsSignatureCompatibleWith(Probe->FindFunction(TEXT("OnSelection"))))) return;
			FScriptDelegate UIListener;
			UIListener.BindUFunction(Root, TEXT("HandleSelectionChanged"));
			Event->RemoveDelegate(UIListener, Controller);
			Event->AddDelegate(Listener, Controller);
			Event->AddDelegate(UIListener, Controller);
			Owner(Own, PlayerFaction);
			Select(Own);
			Test.TestEqual(TEXT("Ownership changes inside one selection notification"), Probe->Interventions, 1);
			Cleared(TEXT("Nested selection owner loss uses final canonical state"), Own);
			Event->RemoveDelegate(Listener, Controller);

			// The auxiliary actor receives real content and BeginPlay, but belongs to another world.
			UWorld* ForeignWorld = UWorld::CreateWorld(EWorldType::Game, false);
			if (!Test.TestNotNull(TEXT("Foreign building world created"), ForeignWorld)) return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(ForeignWorld);
			ForeignWorld->SetGameInstance(World->GetGameInstance());
			ForeignWorld->InitializeActorsForPlay(FURL());
			FActorSpawnParameters ForeignParameters;
			ForeignParameters.ObjectFlags |= RF_Transient;
			ForeignParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Foreign = ForeignWorld->SpawnActor<AActor>(BarracksClass, FVector::ZeroVector, FRotator::ZeroRotator, ForeignParameters);
			if (Test.TestNotNull(TEXT("Foreign building spawned"), Foreign))
			{
				Owner(Foreign, PlayerFaction);
				Foreign->DispatchBeginPlay();
				FCall(Test, Foreign, TEXT("StartConstruction"), 0).Execute();
				Test.TestTrue(TEXT("Foreign building is independently selection-ready"), ReadBool(Test, Foreign, TEXT("IsSelectionReady")));
				Rejected(TEXT("Foreign-world own building"), Foreign);
			}
			ForeignWorld->DestroyWorld(false);
			GEngine->DestroyWorldContext(ForeignWorld);
			CollectGarbage(RF_NoFlags); // Remove deinitialized subsystems before later input tests.

			Owner(Own, PlayerFaction);
			Select(Own);
			Own->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			Test.TestTrue(TEXT("Routed EndPlay fixture remains a valid actor"), IsValid(Own));
			Cleared(TEXT("Still-valid actor EndPlay"), Own);
			Rejected(TEXT("Ended actor cannot be selected again"), Own);
			Owner(Other, PlayerFaction);
			Select(Other);
			FCall Damage(Test, Other, TEXT("ApplyBuildingDamage"), 2);
			Damage.SetFloat(TEXT("Damage"), 100000.0f);
			Damage.SetObject(TEXT("DamageSource"), nullptr);
			Damage.Execute();
			Test.TestNull(TEXT("Destroyed building immediately clears canonical selection"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")));
			Rejected(TEXT("Destroyed building"), Other);
			FCall(Test, Deferred, TEXT("StartConstruction"), 0).Execute();
			FCall Strategic(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
			Strategic.SetEnum(TEXT("NewMode"), EGameplayMode::Strategic);
			Strategic.Execute();
			Select(Deferred);
			Test.TestEqual(TEXT("Own building selection remains eligible in Strategic mode"), ReadObject(Test, Controller, TEXT("GetSelectedBuilding")), static_cast<UObject*>(Deferred));
			Owner(Deferred, EFactionID::None);
			Cleared(TEXT("Strategic ownership-loss cleanup"), Deferred);
			Owner(Deferred, PlayerFaction);
			Select(Deferred);
			Test.TestEqual(TEXT("Controller observes current building before shutdown"), ListenerCount(Test, Deferred, TEXT("OnBuildingStateChanged"), Controller), 1);
			Controller->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			Test.TestEqual(TEXT("Controller EndPlay removes building observer"), ListenerCount(Test, Deferred, TEXT("OnBuildingStateChanged"), Controller), 0);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerBuildingSelectionPIETest,
	"RomanEmpireGame.Buildings.PlayerSelectionPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerBuildingSelectionPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestNotNull(TEXT("Building selection test requires an editor"), GEditor) ||
		!TestNull(TEXT("Building selection test owns a new PIE session"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loads"), EditorWorld && EditorWorld->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FStartUIWindowPIE());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FExercisePlayerBuildingSelection(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUITests::FWaitForOwnedPIEToEnd(this));
	return true;
}

namespace RomanCampaignFunctional
{
	class FSourceGate final : public IAutomationLatentCommand
	{
	public:
		explicit FSourceGate(FAutomationTestBase& InAssertions) : Test(InAssertions), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized())
			{
				if (FPlatformTime::Seconds() - Started < 30.0) return false;
				Test.AddError(TEXT("Functional campaign source readiness timed out.")); return true;
			}
			UCampaignSessionSubsystem* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
			const UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			const URomanUnitRegistrySubsystem* Units = World->GetSubsystem<URomanUnitRegistrySubsystem>();
			if (!Test.TestTrue(TEXT("Real source session/battle/unit registry initialized"), Session && Battle && Units)) return true;
			Test.TestEqual(TEXT("Source is the canonical tactical sandbox"), State->GetCurrentGameplayMode(), EGameplayMode::Tactical);
			Test.TestTrue(TEXT("Source canonical campaign active"), State->IsCampaignActive());
			Test.TestFalse(TEXT("Gate does not preinstall a transfer or session"), Session->HasPendingTransfer() || Session->HasSession());
			Test.TestTrue(TEXT("Source physical sandbox battle exists"), Battle->HasEncounter() && IsValid(Battle->GetObjectiveActor()));
			Test.TestEqual(TEXT("Source registers actual bootstrap actors"), Units->GetRegisteredUnitCount(), 25);
			Test.TestNotNull(TEXT("Source has actual local controller"), World->GetFirstPlayerController());
			const TArray<FCampaignArmy> Armies = State->GetCampaignArmies();
			Test.TestFalse(TEXT("Source canonical persistent armies exist"), Armies.IsEmpty());
			TSet<FGuid> IDs;
			for (const FCampaignArmy& Army : Armies)
			{
				Test.TestTrue(TEXT("Source persistent army identity valid and unique"), Army.ArmyID.IsValid() && !IDs.Contains(Army.ArmyID));
				IDs.Add(Army.ArmyID);
			}
			return true; // Queries only: the existing coordinator owns all subsequent operations.
		}
	private:
		FAutomationTestBase& Test;
		double Started;
	};
}

TSharedPtr<IAutomationLatentCommand> CreateRomanFunctionalCampaignGate(FAutomationTestBase& Assertions)
{
	return MakeShared<RomanCampaignFunctional::FSourceGate>(Assertions);
}

TSharedPtr<IAutomationLatentCommand> CreateRomanFunctionalCampaignTravel(FAutomationTestBase& Assertions)
{
	// This is the unchanged session journey: real strategic/battle/return worlds,
	// receipt/refund preflight, physical result and canonical IDs/casualties/readiness.
	return MakeShared<RomanEmpireUITests::FExerciseCampaignTravel>(&Assertions, true);
}

#endif
