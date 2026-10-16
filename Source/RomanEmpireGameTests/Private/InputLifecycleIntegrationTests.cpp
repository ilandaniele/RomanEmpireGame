// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Building/BuildingTypes.h"
#include "Components/Button.h"
#include "Components/InputComponent.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Core/GameplayMode.h"
#include "Editor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputDeveloperSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Faction/FactionData.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/PlatformTime.h"
#include "Input/RomanInputConfig.h"
#include "Input/RomanInputRemapping.h"
#include "InputActionValue.h"
#include "UI/RomanPlayerSettingsSubsystem.h"
#include "PlayerPreferencesStorageProbe.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace RomanInputPIETests
{
	// The game controller and actors are runtime-private. Exercise reflected public
	// commands, with checked parameter storage, rather than importing private headers.
	class FCall
	{
	public:
		FCall(FAutomationTestBase& InTest, UObject* InTarget, FName Name, int32 Count)
			: Test(InTest), Target(InTarget), Function(InTarget ? InTarget->FindFunction(Name) : nullptr), Params(Function)
		{
			bValid = Test.TestNotNull(*FString::Printf(TEXT("Input test API %s exists"), *Name.ToString()), Function);
			if (bValid) bValid = Test.TestEqual(TEXT("Input test API parameter count matches"), static_cast<int32>(Function->NumParms), Count);
		}

		template <typename T>
		void SetEnum(FName Name, T Value)
		{
			FEnumProperty* Property = Get<FEnumProperty>(Name, false);
			if (Property && Test.TestTrue(TEXT("Input enum has the expected type"), Property->GetEnum() == StaticEnum<T>()))
			{
				Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
			}
			else bValid = false;
		}

		void SetObject(FName Name, UObject* Value)
		{
			FObjectPropertyBase* Property = Get<FObjectPropertyBase>(Name, false);
			if (Property && Test.TestTrue(TEXT("Input object is compatible"), !Value || Value->IsA(Property->PropertyClass)))
				Property->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Value);
			else bValid = false;
		}

		void SetInt(FName Name, int32 Value)
		{
			if (FIntProperty* Property = Get<FIntProperty>(Name, false)) Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
		}

		void SetName(FName Name, FName Value)
		{
			if (FNameProperty* Property = Get<FNameProperty>(Name, false)) Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
		}

		bool Execute()
		{
			if (bValid) Target->ProcessEvent(Function, Params.GetStructMemory());
			return bValid;
		}

		bool ReadBool()
		{
			FBoolProperty* Property = Get<FBoolProperty>(TEXT("ReturnValue"), true);
			return Property && Property->GetPropertyValue_InContainer(Params.GetStructMemory());
		}

		int32 ReadInt()
		{
			FIntProperty* Property = Get<FIntProperty>(TEXT("ReturnValue"), true);
			return Property ? Property->GetPropertyValue_InContainer(Params.GetStructMemory()) : 0;
		}

		FName ReadName()
		{
			FNameProperty* Property = Get<FNameProperty>(TEXT("ReturnValue"), true);
			return Property ? Property->GetPropertyValue_InContainer(Params.GetStructMemory()) : NAME_None;
		}

		UObject* ReadObject()
		{
			FObjectPropertyBase* Property = Get<FObjectPropertyBase>(TEXT("ReturnValue"), true);
			return Property ? Property->GetObjectPropertyValue_InContainer(Params.GetStructMemory()) : nullptr;
		}

		template <typename T>
		T ReadEnum()
		{
			FEnumProperty* Property = Get<FEnumProperty>(TEXT("ReturnValue"), true);
			if (Property && Test.TestTrue(TEXT("Return enum has the expected type"), Property->GetEnum() == StaticEnum<T>()))
				return static_cast<T>(Property->GetUnderlyingProperty()->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory())));
			return static_cast<T>(0);
		}

		template <typename T>
		T ReadStruct()
		{
			T Value;
			FStructProperty* Property = Get<FStructProperty>(TEXT("ReturnValue"), true);
			if (Property && Test.TestTrue(TEXT("Return struct has the expected type"), Property->Struct == T::StaticStruct()))
				Property->CopyCompleteValue(&Value, Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()));
			return Value;
		}

	private:
		FAutomationTestBase& Test;
		UObject* Target;
		UFunction* Function;
		FStructOnScope Params;
		bool bValid = false;

		template <typename T>
		T* Get(FName Name, bool bReturn)
		{
			if (!bValid) return nullptr;
			T* Property = FindFProperty<T>(Function, Name);
			bValid = Test.TestTrue(TEXT("Reflected parameter type and direction match"), Property &&
				Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAnyPropertyFlags(CPF_ReturnParm) == bReturn);
			return bValid ? Property : nullptr;
		}
	};

	UObject* Object(FAutomationTestBase& Test, UObject* Target, FName Name)
	{
		FCall Call(Test, Target, Name, 1);
		return Call.Execute() ? Call.ReadObject() : nullptr;
	}

	bool Boolean(FAutomationTestBase& Test, UObject* Target, FName Name)
	{
		FCall Call(Test, Target, Name, 1);
		return Call.Execute() && Call.ReadBool();
	}

	int32 Integer(FAutomationTestBase& Test, UObject* Target, FName Name)
	{
		FCall Call(Test, Target, Name, 1);
		return Call.Execute() ? Call.ReadInt() : 0;
	}

	class FExercisePhysicalKeys : public IAutomationLatentCommand
	{
	public:
		explicit FExercisePhysicalKeys(FAutomationTestBase* InTest) : Test(*InTest) {}
		virtual ~FExercisePhysicalKeys() override { Cleanup(); }

		virtual bool Update() override
		{
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - StartedAt > 45.0)
			{
				Test.AddError(TEXT("Input PIE fixture exceeded its 45-second deadline"));
				Cleanup();
				return true;
			}
			if (Stage == 0)
			{
				UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
				if (!World || !World->HasBegunPlay() || !World->GetFirstPlayerController()) return false;
				PlayWorld = World;
				if (!Initialize(World)) { Cleanup(); return true; }
				SetMode(EGameplayMode::Tactical);
				++Stage;
				return false;
			}
			if (!GEditor || !PlayWorld.IsValid() || GEditor->PlayWorld != PlayWorld.Get() || !Controller.IsValid())
			{
				Test.AddError(TEXT("Owned PIE world disappeared during input lifecycle assertions"));
				Cleanup();
				return true;
			}

			switch (Stage)
			{
			case 1:
				CheckContexts(EGameplayMode::Tactical);
				CheckShiftPressAndRelease();
				Test.TestTrue(TEXT("Ready Tactical permits commands"), Boolean(Test, Controller.Get(), TEXT("CanIssueTacticalCommands")));
				Test.TestTrue(TEXT("Ready Tactical permits one turn request"), Boolean(Test, Controller.Get(), TEXT("CanEndTurn")));
				Test.TestTrue(TEXT("Selected completed producer is initially trainable in Tactical"), TrainButton->GetIsEnabled());
				Test.TestFalse(TEXT("Quick slot cannot open construction while the menu is closed"), QuickSlot(1));
				Tap(EKeys::One);
				Test.TestFalse(TEXT("Physical 1 also respects the closed-menu gate"), Boolean(Test, Placement, TEXT("IsPlacing")));
				Tap(EKeys::B);
				Test.TestTrue(TEXT("Physical B opens the Tactical construction menu"), Boolean(Test, Root, TEXT("IsBuildingMenuVisible")));
				Key(EKeys::LeftShift, IE_Pressed);
				Test.TestFalse(TEXT("Construction menu suppresses the selection modifier"), SelectionModifier());
				Tap(EKeys::One);
				Test.TestTrue(TEXT("Physical 1 starts the authored first building preview"), Boolean(Test, Placement, TEXT("IsPlacing")));
				Test.TestFalse(TEXT("Placement suppresses the selection modifier"), SelectionModifier());
				Key(EKeys::LeftShift, IE_Released);
				Test.TestFalse(TEXT("Choosing a quick slot closes the menu"), Boolean(Test, Root, TEXT("IsBuildingMenuVisible")));
				{
					FCall Call(Test, Placement, TEXT("GetCurrentBuildingID"), 1);
					Call.Execute();
					BuildingID = Call.ReadName();
					Test.TestFalse(TEXT("Preview retains a stable building identity"), BuildingID.IsNone());
				}
				Tap(EKeys::Escape);
				Test.TestFalse(TEXT("Shared Escape cancels preview first"), Boolean(Test, Placement, TEXT("IsPlacing")));
				Tap(EKeys::B);
				Test.TestTrue(TEXT("Construction menu can reopen"), Boolean(Test, Root, TEXT("IsBuildingMenuVisible")));
				SetMode(EGameplayMode::Strategic);
				Test.TestFalse(TEXT("Mode change closes the construction menu synchronously"), Boolean(Test, Root, TEXT("IsBuildingMenuVisible")));
				break;

			case 2:
				CheckContexts(EGameplayMode::Strategic);
				Test.TestFalse(TEXT("Strategic rejects tactical commands"), Boolean(Test, Controller.Get(), TEXT("CanIssueTacticalCommands")));
				Test.TestFalse(TEXT("Strategic disables the selected producer training control"), TrainButton->GetIsEnabled());
				Test.TestFalse(TEXT("Strategic rejects quick slots even through the public API"), QuickSlot(1));
				Tap(EKeys::B);
				Tap(EKeys::One);
				Test.TestFalse(TEXT("Strategic physical B/1 cannot activate construction"), Boolean(Test, Placement, TEXT("IsPlacing")));
				Test.TestFalse(TEXT("Strategic physical B does not open a tactical menu"), Boolean(Test, Root, TEXT("IsBuildingMenuVisible")));
				{
					FCall Call(Test, Placement, TEXT("StartPlacement"), 1);
					Call.SetName(TEXT("BuildingID"), BuildingID);
					Call.Execute();
					Test.TestFalse(TEXT("Direct placement component API also enforces mode eligibility"), Boolean(Test, Placement, TEXT("IsPlacing")));
					Test.TestFalse(TEXT("Ineligible direct confirmation is rejected"), Boolean(Test, Placement, TEXT("ConfirmPlacement")));
				}
				{
					const int32 Before = Integer(Test, Campaign, TEXT("GetCurrentTurn"));
					Key(EKeys::T, IE_Pressed);
					Test.TestEqual(TEXT("Strategic physical T advances exactly one canonical turn"), Integer(Test, Campaign, TEXT("GetCurrentTurn")), Before + 1);
					Pump();
					Test.TestEqual(TEXT("Holding T does not repeat the turn"), Integer(Test, Campaign, TEXT("GetCurrentTurn")), Before + 1);
					Key(EKeys::T, IE_Released);
				}
				SetMode(EGameplayMode::Tactical);
				break;

			case 3:
				CheckContexts(EGameplayMode::Tactical);
				SelectSoldier();
				Key(EKeys::W, IE_Pressed);
				Key(EKeys::F, IE_Pressed);
				Test.TestTrue(TEXT("Physical F enters the selected living player soldier"), Controller->GetPawn() == Soldier);
				break;

			case 4:
				CheckContexts(EGameplayMode::Possessed);
				Soldier->ConsumeMovementInputVector();
				Pump();
				Test.TestTrue(TEXT("Held camera W does not move the newly possessed soldier"),
					Soldier->ConsumeMovementInputVector().IsNearlyZero());
				Key(EKeys::W, IE_Released);
				Key(EKeys::W, IE_Pressed);
				Test.TestFalse(TEXT("Released and repressed W moves the soldier"),
					Soldier->ConsumeMovementInputVector().IsNearlyZero());
				Test.TestTrue(TEXT("Held F is ignored by the newly activated exit action"), Controller->GetPawn() == Soldier);
				Test.TestFalse(TEXT("Possession rejects tactical command APIs"), Boolean(Test, Controller.Get(), TEXT("CanIssueTacticalCommands")));
				Test.TestFalse(TEXT("Possession rejects turn eligibility"), Boolean(Test, Controller.Get(), TEXT("CanEndTurn")));
				{
					const int32 Before = Integer(Test, Campaign, TEXT("GetCurrentTurn"));
					Tap(EKeys::T);
					Test.TestFalse(TEXT("Possession also rejects explicit public turn requests"), Boolean(Test, Controller.Get(), TEXT("RequestEndTurn")));
					Test.TestEqual(TEXT("Possession T/API attempts leave the canonical turn unchanged"), Integer(Test, Campaign, TEXT("GetCurrentTurn")), Before);
				}
				Key(EKeys::RightMouseButton, IE_Pressed);
				Test.TestTrue(TEXT("Possession RMB begins blocking through Enhanced Input"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Key(EKeys::RightMouseButton, IE_Released);
				Test.TestFalse(TEXT("RMB release ends blocking through the Completed binding"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Key(EKeys::RightMouseButton, IE_Pressed);
				Test.TestTrue(TEXT("A fresh RMB press starts a second block"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Key(EKeys::F, IE_Released);
				Key(EKeys::F, IE_Pressed);
				Test.TestTrue(TEXT("Fresh F exits possession and restores the camera"), Controller->GetPawn() == Camera);
				Test.TestFalse(TEXT("Exiting while RMB is held cancels the soldier block immediately"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				break;

			case 5:
				CheckContexts(EGameplayMode::Tactical);
				{
					const FVector Before = Camera->GetActorLocation();
					Pump();
					Test.TestTrue(TEXT("Held soldier W does not pan the restored camera"),
						Camera->GetActorLocation().Equals(Before));
					Key(EKeys::W, IE_Released);
					Key(EKeys::W, IE_Pressed);
					Test.TestFalse(TEXT("Released and repressed W pans the restored camera"),
						Camera->GetActorLocation().Equals(Before));
					Key(EKeys::W, IE_Released);
				}
				Test.TestTrue(TEXT("Held exit F cannot immediately re-enter the soldier"), Controller->GetPawn() == Camera);
				Test.TestFalse(TEXT("Held RMB does not leave a stale soldier block"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Key(EKeys::RightMouseButton, IE_Released);
				Key(EKeys::F, IE_Released);
				SelectSoldier();
				Key(EKeys::F, IE_Pressed);
				break;

			case 6:
				CheckContexts(EGameplayMode::Possessed);
				Key(EKeys::F, IE_Released);
				Key(EKeys::RightMouseButton, IE_Pressed);
				Test.TestTrue(TEXT("Block is active before the viewport flush path"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Test.TestTrue(TEXT("Controller requests key flushing on viewport focus changes"), Controller->ShouldFlushKeysWhenViewportFocusChanges());
				Controller->FlushPressedKeys();
				Test.TestFalse(TEXT("Viewport key flush synchronously clears held block"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Test.TestFalse(TEXT("Viewport key flush clears partial box selection"), Boolean(Test, Controller.Get(), TEXT("IsBoxSelecting")));
				Key(EKeys::RightMouseButton, IE_Released);
				Key(EKeys::RightMouseButton, IE_Pressed);
				Test.TestTrue(TEXT("A fresh RMB can block after focus flush"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				SetMode(EGameplayMode::Strategic);
				Test.TestTrue(TEXT("External canonical mode change restores the camera pawn"), Controller->GetPawn() == Camera);
				Test.TestFalse(TEXT("External canonical mode change clears held block"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				break;

			case 7:
				CheckContexts(EGameplayMode::Strategic);
				Key(EKeys::RightMouseButton, IE_Released);
				SetMode(EGameplayMode::Tactical);
				break;

			case 8:
				CheckContexts(EGameplayMode::Tactical);
				SelectSoldier();
				Key(EKeys::F, IE_Pressed);
				break;

			case 9:
				CheckContexts(EGameplayMode::Possessed);
				Key(EKeys::F, IE_Released);
				Key(EKeys::RightMouseButton, IE_Pressed);
				Test.TestTrue(TEXT("Block is active before possessed-unit death"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				{
					FCall Call(Test, Soldier, TEXT("SetCurrentHealth"), 1);
					Call.SetInt(TEXT("NewHealth"), 0);
					Call.Execute();
				}
				Test.TestFalse(TEXT("Canonical death clears the dead soldier block"), Boolean(Test, Soldier, TEXT("IsBlocking")));
				Test.TestTrue(TEXT("Canonical death restores the camera"), Controller->GetPawn() == Camera);
				break;

			case 10:
				CheckContexts(EGameplayMode::Tactical);
				Key(EKeys::RightMouseButton, IE_Released);
				Test.TestFalse(TEXT("Death does not leave a phantom possession mode"), Boolean(Test, Controller.Get(), TEXT("IsInFirstPersonMode")));
				Key(EKeys::LeftShift, IE_Pressed);
				Test.TestTrue(TEXT("Shift is active before a mode boundary"), SelectionModifier());
				SetMode(EGameplayMode::Strategic);
				Test.TestFalse(TEXT("Mode boundary synchronously rejects the held selection modifier"), SelectionModifier());
				break;

			case 11:
				CheckContexts(EGameplayMode::Strategic);
				Pump();
				Test.TestFalse(TEXT("Strategic does not evaluate Tactical Shift"), SelectionModifier());
				SetMode(EGameplayMode::Tactical);
				break;

			case 12:
				CheckContexts(EGameplayMode::Tactical);
				Pump();
				Test.TestFalse(TEXT("Held Shift is ignored when Tactical is reactivated"), SelectionModifier());
				Key(EKeys::LeftShift, IE_Released);
				Key(EKeys::LeftShift, IE_Pressed);
				Test.TestTrue(TEXT("Fresh Shift activates after the mode boundary"), SelectionModifier());
				Controller->FlushPressedKeys();
				Test.TestFalse(TEXT("Viewport focus flush synchronously clears the selection modifier"), SelectionModifier());
				Pump();
				Test.TestFalse(TEXT("Modifier remains cleared after flushed input is processed"), SelectionModifier());
				Key(EKeys::LeftShift, IE_Released);
				Key(EKeys::RightShift, IE_Pressed);
				Test.TestTrue(TEXT("Fresh other Shift activates after focus flush"), SelectionModifier());
				Key(EKeys::RightShift, IE_Released);
				Test.TestFalse(TEXT("Fresh Shift release clears the modifier after focus flush"), SelectionModifier());
				break;

			default:
				Test.TestTrue(TEXT("Mode changes preserve a separately owned input context"), Subsystem->HasMappingContext(ForeignContext.Get()));
				Cleanup();
				return true;
			}
			++Stage;
			return false; // Let the real subsystem perform its deferred mapping rebuild.
		}

	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		int32 Stage = 0;
		TWeakObjectPtr<UWorld> PlayWorld;
		TWeakObjectPtr<APlayerController> Controller;
		UEnhancedPlayerInput* PlayerInput = nullptr;
		UEnhancedInputLocalPlayerSubsystem* Subsystem = nullptr;
		URomanInputConfig* Config = nullptr;
		UUserWidget* Root = nullptr;
		UButton* TrainButton = nullptr;
		UObject* Placement = nullptr;
		UObject* Campaign = nullptr;
		APawn* Soldier = nullptr;
		APawn* Camera = nullptr;
		TStrongObjectPtr<UInputMappingContext> ForeignContext;
		FName BuildingID;
		bool bWasControllerTickEnabled = false;
		bool bOwnsTickOverride = false;

		bool Initialize(UWorld* World)
		{
			Controller = World->GetFirstPlayerController();
			PlayerInput = Cast<UEnhancedPlayerInput>(Controller->PlayerInput);
			Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
			Config = Cast<URomanInputConfig>(Object(Test, Controller.Get(), TEXT("GetInputConfiguration")));
			Root = Cast<UUserWidget>(Object(Test, Controller.Get(), TEXT("GetMainWidget")));
			Campaign = Object(Test, World->GetAuthGameMode(), TEXT("GetCampaignManager"));
			Camera = Controller->GetPawn();
			FObjectPropertyBase* PlacementProperty = FindFProperty<FObjectPropertyBase>(Controller->GetClass(), TEXT("BuildingPlacementComponent"));
			Placement = PlacementProperty ? PlacementProperty->GetObjectPropertyValue_InContainer(Controller.Get()) : nullptr;
			if (!Test.TestTrue(TEXT("Normal PIE startup supplies authored Enhanced Input and gameplay/UI objects"),
				PlayerInput && Subsystem && Config && Root && Placement && Campaign && Camera &&
				Cast<UEnhancedInputComponent>(Controller->InputComponent))) return false;
			Test.TestEqual(TEXT("Controller loaded the committed input asset, not transient generated mappings"),
				Config->GetPathName(), FString(TEXT("/Game/Input/DA_RomanInputConfig.DA_RomanInputConfig")));
			TrainButton = Cast<UButton>(Root->GetWidgetFromName(TEXT("TrainButton")));
			if (!Test.TestNotNull(TEXT("Native production UI exposes its training control"), TrainButton)) return false;
			UObject* FactionManager = Object(Test, World->GetAuthGameMode(), TEXT("GetFactionManager"));
			FCall FactionCall(Test, FactionManager, TEXT("GetPlayerFaction"), 1);
			FactionCall.Execute();
			const EFactionID PlayerFaction = FactionCall.ReadEnum<EFactionID>();
			UClass* UnitClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.UnitBase")).TryLoadClass<APawn>();
			if (!Test.TestNotNull(TEXT("Native unit class resolves"), UnitClass)) return false;
			for (TActorIterator<APawn> It(World, UnitClass); It; ++It)
			{
				FCall Owner(Test, *It, TEXT("GetOwnerFaction"), 1);
				Owner.Execute();
				if (Owner.ReadEnum<EFactionID>() == PlayerFaction && Boolean(Test, *It, TEXT("IsAlive")))
				{
					Soldier = *It;
					break;
				}
			}
			if (!Test.TestNotNull(TEXT("Normal startup supplies a living player-owned soldier"), Soldier)) return false;

			UClass* BarracksClass = FSoftClassPath(TEXT("/Script/RomanEmpireGame.Barracks")).TryLoadClass<AActor>();
			if (!Test.TestNotNull(TEXT("Native barracks class resolves"), BarracksClass)) return false;
			FActorSpawnParameters Spawn;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Producer = World->SpawnActor<AActor>(BarracksClass, FVector(50000, 50000, 300), FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Transient PIE training fixture spawns"), Producer)) return false;
			FCall Owner(Test, Producer, TEXT("SetOwnerFaction"), 1);
			Owner.SetEnum(TEXT("NewOwner"), PlayerFaction);
			Owner.Execute();
			FCall Data(Test, Producer, TEXT("GetBuildingData"), 1);
			Data.Execute();
			FCall(Test, Producer, TEXT("StartConstruction"), 0).Execute();
			Producer->Tick(Data.ReadStruct<FBuildingData>().ConstructionTime + 0.1f);
			Producer->SetActorTickEnabled(false);
			FCall Select(Test, Controller.Get(), TEXT("SelectBuilding"), 1);
			Select.SetObject(TEXT("Building"), Producer);
			Select.Execute();

			// Own only this test controller's tick. Process its real Enhanced Input stack
			// deterministically below; do not send OS keys or call action delegates.
			bWasControllerTickEnabled = Controller->IsActorTickEnabled();
			Controller->SetActorTickEnabled(false);
			bOwnsTickOverride = true;
			ForeignContext.Reset(NewObject<UInputMappingContext>());
			Subsystem->AddMappingContext(ForeignContext.Get(), -100);
			return true;
		}

		void SetMode(EGameplayMode Mode)
		{
			FCall Call(Test, PlayWorld->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
			Call.SetEnum(TEXT("NewMode"), Mode);
			Call.Execute();
		}

		void CheckContexts(EGameplayMode Mode)
		{
			Test.TestTrue(TEXT("Shared cancellation context stays active"), Subsystem->HasMappingContext(Config->SharedContext));
			int32 ActiveModes = 0;
			for (UInputMappingContext* Context : { Config->StrategicContext.Get(), Config->TacticalContext.Get(), Config->PossessionContext.Get() })
			{
				if (Subsystem->HasMappingContext(Context)) ++ActiveModes;
			}
			Test.TestEqual(TEXT("Exactly one gameplay mode context is active"), ActiveModes, 1);
			Test.TestTrue(TEXT("The canonical mode's context is active"), Subsystem->HasMappingContext(Config->GetContextForMode(Mode)));
			Test.TestEqual(TEXT("Controller exposes the same active context"),
				Object(Test, Controller.Get(), TEXT("GetActiveGameplayContext")), static_cast<UObject*>(Config->GetContextForMode(Mode)));
			Test.TestTrue(TEXT("Unrelated context remains registered during mode transitions"), Subsystem->HasMappingContext(ForeignContext.Get()));
		}

		void SelectSoldier()
		{
			FCall Call(Test, Controller.Get(), TEXT("SelectUnit"), 1);
			Call.SetObject(TEXT("Unit"), Soldier);
			Call.Execute();
		}

		bool QuickSlot(int32 Slot)
		{
			FCall Call(Test, Controller.Get(), TEXT("StartBuildingPlacementByQuickSlot"), 2);
			Call.SetInt(TEXT("QuickSlot"), Slot);
			return Call.Execute() && Call.ReadBool();
		}

		void Pump()
		{
			TArray<UInputComponent*> Stack = { Controller->InputComponent };
			PlayerInput->ProcessInputStack(Stack, 1.0f / 60.0f, false);
		}

		void Key(const FKey& KeyCode, EInputEvent Event)
		{
			const FInputDeviceId Device = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(Controller->GetPlatformUserId());
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(KeyCode, Event, Event == IE_Released ? 0.0f : 1.0f, -1, Device));
			Pump();
		}

		void Tap(const FKey& KeyCode)
		{
			Key(KeyCode, IE_Pressed);
			Key(KeyCode, IE_Released);
		}

		bool SelectionModifier()
		{
			return Boolean(Test, Controller.Get(), TEXT("IsSelectionModifierActive"));
		}

		void CheckShiftPressAndRelease()
		{
			Test.TestFalse(TEXT("Selection modifier starts inactive"), SelectionModifier());
			for (const FKey& ShiftKey : { EKeys::LeftShift, EKeys::RightShift })
			{
				Key(ShiftKey, IE_Pressed);
				Test.TestTrue(TEXT("Each physical Shift activates the selection modifier"), SelectionModifier());
				Key(ShiftKey, IE_Released);
				Test.TestFalse(TEXT("Each physical Shift release clears the selection modifier"), SelectionModifier());
			}
			Key(EKeys::LeftShift, IE_Pressed);
			Key(EKeys::RightShift, IE_Pressed);
			Key(EKeys::LeftShift, IE_Released);
			Test.TestTrue(TEXT("Releasing left Shift preserves held right Shift"), SelectionModifier());
			Key(EKeys::LeftShift, IE_Pressed);
			Key(EKeys::RightShift, IE_Released);
			Test.TestTrue(TEXT("Releasing right Shift preserves held left Shift"), SelectionModifier());
			Key(EKeys::LeftShift, IE_Released);
			Test.TestFalse(TEXT("Releasing both Shifts clears the modifier"), SelectionModifier());
		}

		void Cleanup()
		{
			if (Controller.IsValid() && bOwnsTickOverride)
			{
				Controller->FlushPressedKeys();
				if (Subsystem && ForeignContext.IsValid()) Subsystem->RemoveMappingContext(ForeignContext.Get());
				Controller->SetActorTickEnabled(bWasControllerTickEnabled);
			}
			bOwnsTickOverride = false;
			ForeignContext.Reset();
		}
	};

	class FExercisePreferenceInputs : public IAutomationLatentCommand
	{
		FAutomationTestBase& Test;
		TWeakObjectPtr<APlayerController> Controller;
		TWeakObjectPtr<APawn> Camera, Soldier;
		TWeakObjectPtr<URomanPlayerSettingsSubsystem> Settings;
		TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> Input;
		TStrongObjectPtr<URomanInputConfig> Config;
		FRomanPlayerPreferences Before;
		FTransform CameraTransform;
		bool bRestore = false, bTickEnabled = false, bEntered = false;
		int32 Stage = 0;
		double Started = FPlatformTime::Seconds();
		void Pump(const UInputAction* Action, FVector2D Value)
		{
			Input->InjectInputForAction(Action,FInputActionValue(Value),{},{});
			CastChecked<UEnhancedPlayerInput>(Controller->PlayerInput)->ProcessInputStack({Controller->InputComponent},1.f/60,false);
		}
		FRotator Look()
		{
			Controller->SetControlRotation(FRotator::ZeroRotator); Controller->RotationInput=FRotator::ZeroRotator;
			Pump(Config->Look,FVector2D(.1,.2)); Controller->UpdateRotation(1.f/60);
			return Controller->GetControlRotation().GetNormalized();
		}
	public:
		explicit FExercisePreferenceInputs(FAutomationTestBase* InTest):Test(*InTest){}
		virtual ~FExercisePreferenceInputs() override
		{
			if (!bRestore) return;
			if (Controller.IsValid())
			{
				if (bEntered) { FCall Exit(Test,Controller.Get(),TEXT("ExitFirstPersonMode"),0); Exit.Execute(); }
				Controller->FlushPressedKeys(); Controller->SetActorTickEnabled(bTickEnabled);
			}
			if (Camera.IsValid()) Camera->SetActorTransform(CameraTransform);
			if (Settings.IsValid()) Settings->TryApply(Before,false);
		}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds()-Started>30) { Test.AddError(TEXT("Preference input PIE timed out")); return true; }
			UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
			if (Stage==0)
			{
				if (!World || !World->HasBegunPlay() || !World->GetFirstPlayerController()) return false;
				Controller=World->GetFirstPlayerController();
				if (!Boolean(Test,Controller.Get(),TEXT("CanIssueTacticalCommands"))) return false;
				Camera=Controller->GetPawn(); Settings=World->GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>();
				Input=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
				Config.Reset(LoadObject<URomanInputConfig>(nullptr,TEXT("/Game/Input/DA_RomanInputConfig")));
				if (!Test.TestTrue(TEXT("Real camera/settings/authored input/player pipeline"),Camera.IsValid() && Settings.IsValid() && Input.IsValid() && Config.IsValid() && Cast<UEnhancedPlayerInput>(Controller->PlayerInput))) return true;
				Before=Settings->GetPreferences(); CameraTransform=Camera->GetActorTransform(); bTickEnabled=Controller->IsActorTickEnabled(); bRestore=true;
				Controller->SetActorTickEnabled(false); Controller->FlushPressedKeys(); ++Stage; return false;
			}
			if (!Test.TestTrue(TEXT("Owned preference input fixtures alive"),Controller.IsValid() && Camera.IsValid() && Settings.IsValid() && Input.IsValid())) return true;
			if (Stage==1)
			{
				FRomanPlayerPreferences Value=Before; Value.PanSensitivity=1; Value.LookSensitivity=1; Value.bInvertLookY=false; Settings->TryApply(Value,false);
				const FVector Start=Camera->GetActorLocation(); Pump(Config->CameraPan,FVector2D(.1,0)); const FVector Base=Camera->GetActorLocation()-Start;
				Test.TestTrue(TEXT("Real EnhancedInput pans native RTS camera"),Base.SizeSquared()>1);
				Camera->SetActorLocation(Start); Value.PanSensitivity=2; Settings->TryApply(Value,false); Pump(Config->CameraPan,FVector2D(.1,0));
				Test.TestTrue(TEXT("Live pan preference doubles actual displacement"),(Camera->GetActorLocation()-Start).Equals(Base*2,.001));
				UClass* UnitClass=FSoftClassPath(TEXT("/Script/RomanEmpireGame.UnitBase")).TryLoadClass<APawn>();
				if (!Test.TestNotNull(TEXT("Native unit class for preference input test"),UnitClass)) return true;
				for (TActorIterator<APawn> It(World,UnitClass); It; ++It)
				{
					FCall Owner(Test,*It,TEXT("GetOwnerFaction"),1); Owner.Execute();
					if (Owner.ReadEnum<EFactionID>()==EFactionID::Rome && Boolean(Test,*It,TEXT("IsAlive"))) { Soldier=*It; break; }
				}
				if (!Test.TestTrue(TEXT("Actual eligible bootstrap soldier"),Soldier.IsValid())) return true;
				FCall Enter(Test,Controller.Get(),TEXT("EnterFirstPersonMode"),1); Enter.SetObject(TEXT("UnitToPossess"),Soldier.Get()); Enter.Execute();
				bEntered=Controller->GetPawn()==Soldier.Get(); Test.TestTrue(TEXT("Actual possession before look injection"),bEntered); ++Stage; return false;
			}
			if (!bEntered) return true;
			FRomanPlayerPreferences Value=Before; Value.LookSensitivity=1; Value.bInvertLookY=false; Settings->TryApply(Value,false);
			const FRotator Base=Look(); Test.TestTrue(TEXT("Real input moves both FPS look axes"),FMath::Abs(Base.Yaw)>.001 && FMath::Abs(Base.Pitch)>.001);
			Value.LookSensitivity=2; Settings->TryApply(Value,false); const FRotator Doubled=Look();
			Test.TestTrue(TEXT("Live look preference doubles actual yaw/pitch"),FMath::IsNearlyEqual(Doubled.Yaw,Base.Yaw*2,.001) && FMath::IsNearlyEqual(Doubled.Pitch,Base.Pitch*2,.001));
			Value.bInvertLookY=true; Settings->TryApply(Value,false); const FRotator Inverted=Look();
			Test.TestTrue(TEXT("Invert changes pitch only, not yaw"),FMath::IsNearlyEqual(Inverted.Yaw,Doubled.Yaw,.001) && FMath::IsNearlyEqual(Inverted.Pitch,-Doubled.Pitch,.001));
			return true;
		}
	};

	class FExerciseSettingsMenu : public IAutomationLatentCommand
	{
		FAutomationTestBase& Test;
		TWeakObjectPtr<APlayerController> Controller;
		double Started=FPlatformTime::Seconds();
	public:
		explicit FExerciseSettingsMenu(FAutomationTestBase* InTest):Test(*InTest){}
		virtual ~FExerciseSettingsMenu() override { if(Controller.IsValid()) { FCall Close(Test,Controller.Get(),TEXT("CloseSettings"),0); Close.Execute(); Controller->FlushPressedKeys(); } }
		virtual bool Update() override
		{
			UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
			if(!World || !World->HasBegunPlay() || !World->GetFirstPlayerController()) { if(FPlatformTime::Seconds()-Started>30) { Test.AddError(TEXT("Settings PIE startup timed out")); return true; } return false; }
			Controller=World->GetFirstPlayerController();
			auto* Settings=World->GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>();
			auto* Root=Cast<UUserWidget>(Object(Test,Controller.Get(),TEXT("GetMainWidget")));
			auto* Open=Root?Cast<UButton>(Root->GetWidgetFromName(TEXT("OpenSettingsButton"))):nullptr;
			if(!Test.TestTrue(TEXT("Actual reachable native settings affordance"),Settings && Open)) return true;
			const auto Before=Settings->GetPreferences();
			const auto Device=IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(Controller->GetPlatformUserId());
			const auto Key=[&](FKey Code,EInputEvent Event) { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Code,Event,Event==IE_Released?0.f:1.f,-1,Device)); };
			APawn* Camera=Controller->GetPawn(); const FTransform CameraBefore=Camera->GetActorTransform();
			Key(EKeys::W,IE_Pressed);
			CastChecked<UEnhancedPlayerInput>(Controller->PlayerInput)->ProcessInputStack({Controller->InputComponent},1.f/60,false);
			Test.TestTrue(TEXT("Held-key fixture establishes an actually evaluated physical press"),Controller->IsInputKeyDown(EKeys::W));
			Open->OnClicked.Broadcast();
			Test.TestTrue(TEXT("Actual HUD click opens settings"),Boolean(Test,Controller.Get(),TEXT("IsSettingsOpen")));
			Test.TestTrue(TEXT("Offline menu pauses canonical world"),Controller->IsPaused());
			Test.TestFalse(TEXT("Paused menu gates tactical commands"),Boolean(Test,Controller.Get(),TEXT("CanIssueTacticalCommands")));
			Test.TestFalse(TEXT("Paused menu gates turn requests"),Boolean(Test,Controller.Get(),TEXT("CanEndTurn")));
			auto* Menu=Cast<UUserWidget>(Object(Test,Controller.Get(),TEXT("GetSettingsWidget")));
			auto* Master=Menu?Cast<USpinBox>(Menu->GetWidgetFromName(TEXT("MasterVolumeInput"))):nullptr;
			auto* Resume=Menu?Cast<UButton>(Menu->GetWidgetFromName(TEXT("ResumeSettingsButton"))):nullptr;
			auto* Defaults=Menu?Cast<UButton>(Menu->GetWidgetFromName(TEXT("DefaultsSettingsButton"))):nullptr;
			if(!Test.TestTrue(TEXT("Actual editable keyboard-navigable controls"),Master && Resume && Defaults && Resume->GetIsFocusable())) return true;
			Test.TestEqual(TEXT("Form initializes from canonical preferences"),Master->GetValue(),Before.MasterVolume);
			Master->SetValue(Before.MasterVolume==.2f?.3f:.2f);
			Test.TestTrue(TEXT("Editing never mutates canonical values"),Settings->GetPreferences()==Before);
			Defaults->OnClicked.Broadcast(); Test.TestEqual(TEXT("Defaults only updates detached form"),Master->GetValue(),1.f);
			Test.TestTrue(TEXT("Defaults has no user-file/gameplay side effect"),Settings->GetPreferences()==Before);
			Resume->OnClicked.Broadcast();
			Test.TestFalse(TEXT("Real Resume removes modal"),Boolean(Test,Controller.Get(),TEXT("IsSettingsOpen")));
			Test.TestFalse(TEXT("Owned pause released on Resume"),Controller->IsPaused());
			Test.TestTrue(TEXT("Cancel discards all drafts"),Settings->GetPreferences()==Before);
			auto* Input=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
			auto* Config=LoadObject<URomanInputConfig>(nullptr,TEXT("/Game/Input/DA_RomanInputConfig"));
			const auto* UserSettings=Input->GetUserSettings();
			const auto* Profile=UserSettings?UserSettings->GetActiveKeyProfile():nullptr;
			int32 AuthoredMappings=0,MappableMappings=0;
			for(const UInputMappingContext* Context:{Config->StrategicContext.Get(),Config->TacticalContext.Get(),Config->PossessionContext.Get(),Config->SharedContext.Get()})
				for(const FEnhancedActionKeyMapping& Mapping:Context->GetMappings()) { ++AuthoredMappings; if(Mapping.IsPlayerMappable()) ++MappableMappings; }
			Test.AddInfo(FString::Printf(TEXT("Readonly remap bootstrap: actual local userSettings=%d activeProfile=%d rows=%d authoredMappable=%d/%d"),UserSettings!=nullptr,Profile!=nullptr,Profile?Profile->GetPlayerMappingRows().Num():0,MappableMappings,AuthoredMappings));
			Test.TestTrue(TEXT("Real user profile has all29 registered discrete bindings"),Profile && Profile->GetPlayerMappingRows().Num()==29 && MappableMappings==29 && AuthoredMappings==33);
			const auto Pan=[&] { Input->InjectInputForAction(Config->CameraPan,FInputActionValue(FVector2D(.1,0)),{},{}); CastChecked<UEnhancedPlayerInput>(Controller->PlayerInput)->ProcessInputStack({Controller->InputComponent},1.f/60,false); };
			const FVector ResumePosition=Camera->GetActorLocation(); Pan();
			Test.TestTrue(TEXT("Movement held before modal stays latched on resume"),Camera->GetActorLocation().Equals(ResumePosition,.001));
			Key(EKeys::W,IE_Released); Pan();
			Test.TestTrue(TEXT("Actual release restores movement handlers"),(Camera->GetActorLocation()-ResumePosition).SizeSquared()>1);
			Camera->SetActorTransform(CameraBefore);
			Key(EKeys::F10,IE_Pressed); Key(EKeys::F10,IE_Released);
			Test.TestTrue(TEXT("Actual reserved physical F10 opens modal"),Boolean(Test,Controller.Get(),TEXT("IsSettingsOpen")));
			Test.TestEqual(TEXT("Reopen replaces cancelled draft"),Master->GetValue(),Before.MasterVolume);
			FCall Close(Test,Controller.Get(),TEXT("CloseSettings"),0); Close.Execute();
			Test.TestFalse(TEXT("Repeated close idempotently releases owned pause"),Controller->IsPaused());
			Close.Execute(); Controller->FlushPressedKeys();
			UClass* UnitClass=FSoftClassPath(TEXT("/Script/RomanEmpireGame.UnitBase")).TryLoadClass<APawn>();
			APawn* Soldier=nullptr;
			if(UnitClass) for(TActorIterator<APawn> It(World,UnitClass);It;++It)
			{
				FCall Owner(Test,*It,TEXT("GetOwnerFaction"),1); Owner.Execute();
				if(Owner.ReadEnum<EFactionID>()==EFactionID::Rome && Boolean(Test,*It,TEXT("IsAlive"))) { Soldier=*It; break; }
			}
			if(!Test.TestNotNull(TEXT("Actual soldier for possessed settings flow"),Soldier)) return true;
			FCall Enter(Test,Controller.Get(),TEXT("EnterFirstPersonMode"),1); Enter.SetObject(TEXT("UnitToPossess"),Soldier); Enter.Execute();
			Test.TestTrue(TEXT("Native possession retains original pawn before menu"),Controller->GetPawn()==Soldier);
			Key(EKeys::F10,IE_Pressed); Key(EKeys::F10,IE_Released);
			Test.TestTrue(TEXT("F10 opens settings in FPS without exiting possession"),Boolean(Test,Controller.Get(),TEXT("IsSettingsOpen")) && Controller->GetPawn()==Soldier && Controller->IsPaused());
			Resume->OnClicked.Broadcast();
			Test.TestTrue(TEXT("FPS resume restores game-only cursor and same pawn"),Controller->GetPawn()==Soldier && !Controller->bShowMouseCursor && !Controller->IsPaused());
			FCall Exit(Test,Controller.Get(),TEXT("ExitFirstPersonMode"),0); Exit.Execute();
			Test.TestTrue(TEXT("Ordinary possession exit still restores camera"),Controller->GetPawn()==Camera);
			Controller->FlushPressedKeys();
			// Actual Apply route and real IO, never the personal preferences destination.
			FRomanPreferencesStorageProbe Storage(*Settings);
			const FString FixturePath=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/PreferencesMenu"),FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json")));
			ON_SCOPE_EXIT { Settings->TryApply(Before,false); IFileManager::Get().Delete(*FixturePath,false,true); IFileManager::Get().Delete(*(FixturePath+TEXT(".previous")),false,true); IFileManager::Get().Delete(*(FixturePath+TEXT(".rejected")),false,true); };
			Storage.SetPath(FixturePath); Open->OnClicked.Broadcast();
			auto* Apply=Cast<UButton>(Menu->GetWidgetFromName(TEXT("ApplySettingsButton")));
			auto* Status=Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("SettingsStatus")));
			if(!Test.TestTrue(TEXT("Actual native Apply/status route"),Apply && Status)) return true;
			const float DesiredMaster=Before.MasterVolume==.25f?.35f:.25f;
			Master->SetValue(DesiredMaster); Apply->OnClicked.Broadcast();
			Test.TestEqual(TEXT("Actual Apply commits edited form value"),Settings->GetPreferences().MasterVolume,DesiredMaster);
			FRomanPlayerPreferences Persisted;
			Test.TestTrue(TEXT("Actual UI Apply persists exact detached canonical values"),FRomanPreferencesFile::Read(FixturePath,Persisted) && Persisted==Settings->GetPreferences());
			Test.TestTrue(TEXT("Successful actual persist reports saved"),Status->GetText().ToString().Contains(TEXT("Saved and applied")));
			const auto Committed=Settings->GetPreferences();
			Storage.SetPath(FPaths::Combine(FPaths::ProjectDir(),TEXT("PreferencesMenuMustNotCreate.json")));
			Master->SetValue(DesiredMaster+.1f); Apply->OnClicked.Broadcast();
			Test.TestTrue(TEXT("Failed actual Apply preserves every canonical preference"),Settings->GetPreferences()==Committed);
			Test.TestTrue(TEXT("Actual IO failure visible to player"),Status->GetText().ToString().Contains(TEXT("Could not save/apply")));
			Resume->OnClicked.Broadcast();
			return true;
		}
	};

	struct FRemapSlotFixture
	{
		FString Slot=TEXT("RomanMenuRemap_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
		FString Before=GetDefault<UEnhancedInputDeveloperSettings>()->InputSettingsSaveSlotName;
		FRemapSlotFixture() {GetMutableDefault<UEnhancedInputDeveloperSettings>()->InputSettingsSaveSlotName=Slot;}
		~FRemapSlotFixture() {GetMutableDefault<UEnhancedInputDeveloperSettings>()->InputSettingsSaveSlotName=Before; UGameplayStatics::DeleteGameInSlot(Slot,0);}
	};
	class FExerciseRemappingMenu : public IAutomationLatentCommand
	{
		FAutomationTestBase& Test; TSharedRef<FRemapSlotFixture> Slot;
		TWeakObjectPtr<APlayerController> Controller; double Started=FPlatformTime::Seconds();
	public:
		FExerciseRemappingMenu(FAutomationTestBase* InTest,TSharedRef<FRemapSlotFixture> InSlot):Test(*InTest),Slot(InSlot){}
		virtual ~FExerciseRemappingMenu() override {if(Controller.IsValid()) {FCall Close(Test,Controller.Get(),TEXT("CloseSettings"),0); Close.Execute(); Controller->FlushPressedKeys();}}
		virtual bool Update() override
		{
			UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
			if(!World || !World->HasBegunPlay() || !World->GetFirstPlayerController()) {if(FPlatformTime::Seconds()-Started>30){Test.AddError(TEXT("Remapping PIE startup timed out"));return true;} return false;}
			Controller=World->GetFirstPlayerController();
			if(!Boolean(Test,Controller.Get(),TEXT("CanIssueTacticalCommands"))) {if(FPlatformTime::Seconds()-Started>30){Test.AddError(TEXT("Remapping canonical startup timed out"));return true;} return false;}
			auto* Input=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
			auto* PlayerInput=Cast<UEnhancedPlayerInput>(Controller->PlayerInput); auto* Settings=Input?Input->GetUserSettings():nullptr;
			auto* Config=LoadObject<URomanInputConfig>(nullptr,TEXT("/Game/Input/DA_RomanInputConfig"));
			if(!Test.TestTrue(TEXT("Actual local player engine settings/profile/input stack"),PlayerInput && Settings && Config && Settings->GetActiveKeyProfile())) return true;
			Test.TestEqual(TEXT("Canonical engine persistence redirected only to owned GUID fixture"),PlayerInput->GetUserSettingsSaveFileName(),Slot->Slot);
			Test.TestFalse(TEXT("Never overwrite a preexisting personal input file"),UGameplayStatics::DoesSaveGameExist(Slot->Slot,0));
			const auto Device=IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(Controller->GetPlatformUserId());
			const auto Physical=[&](FKey Key,EInputEvent Event){Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key,Event,Event==IE_Released?0.f:1.f,-1,Device));};
			const auto Pump=[&] {PlayerInput->ProcessInputStack({Controller->InputComponent},1.f/60,false);};
			APawn* Camera=Controller->GetPawn(); const FTransform Before=Camera->GetActorTransform(); const bool TickBefore=Controller->IsActorTickEnabled(); Controller->SetActorTickEnabled(false);
			ON_SCOPE_EXIT {Camera->SetActorTransform(Before); Controller->SetActorTickEnabled(TickBefore); Controller->FlushPressedKeys(); GetMutableDefault<UEnhancedInputDeveloperSettings>()->InputSettingsSaveSlotName=Slot->Slot;};
			const auto Open=[&]{return Boolean(Test,Controller.Get(),TEXT("OpenSettings"));};
			if(!Test.TestTrue(TEXT("Actual controller opens offline remapping menu"),Open())) return true;
			auto* Menu=Cast<UUserWidget>(Object(Test,Controller.Get(),TEXT("GetSettingsWidget")));
			auto* Move=Menu?Cast<UButton>(Menu->GetWidgetFromName(TEXT("Binding_Roman.v1.Tactical.Move_forward"))):nullptr;
			auto* SaveKeys=Menu?Cast<UButton>(Menu->GetWidgetFromName(TEXT("SaveBindingsButton"))):nullptr;
			auto* Defaults=Menu?Cast<UButton>(Menu->GetWidgetFromName(TEXT("DefaultsBindingsButton"))):nullptr;
			auto* Resume=Menu?Cast<UButton>(Menu->GetWidgetFromName(TEXT("ResumeSettingsButton"))):nullptr;
			auto* Status=Menu?Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("SettingsStatus"))):nullptr;
			if(!Test.TestTrue(TEXT("29 real native rows plus save/default/resume/status controls"),Move && SaveKeys && Defaults && Resume && Status && RomanInputRemapping::GetBindings(Config,Settings).Num()==29)) return true;
			const auto SlateKey=[&](FKey Key)
			{
				FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
				FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
			};
			const auto Current=[&]
			{
				const auto Values=RomanInputRemapping::GetBindings(Config,Settings); const auto* Row=Values.FindByPredicate([](const FRomanKeyBinding& Value){return Value.Name==FName(TEXT("Roman.v1.Tactical.Move_forward"));}); return Row?Row->Current:FKey();
			};
			Move->OnClicked.Broadcast(); SlateKey(EKeys::U);
			Test.TestEqual(TEXT("Actual Slate key down/up capture commits canonical U binding"),Current(),EKeys::U);
			Test.TestTrue(TEXT("Actual row reacts to engine settings event"),CastChecked<UTextBlock>(Move->GetContent())->GetText().ToString().Contains(TEXT("[U]")));
			auto* Root=Cast<UUserWidget>(Object(Test,Controller.Get(),TEXT("GetMainWidget")));
			auto* Controls=Root?Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ControlsText"))):nullptr;
			Test.TestTrue(TEXT("HUD help follows actual remap event without per-frame polling"),Controls && Controls->GetText().ToString().Contains(TEXT("U /")));
			Move->OnClicked.Broadcast(); SlateKey(EKeys::D); Test.TestEqual(TEXT("Actual conflicting capture preserves U"),Current(),EKeys::U);
			Test.TestTrue(TEXT("Conflict feedback visible"),Status->GetText().ToString().Contains(TEXT("already used")));
			Move->OnClicked.Broadcast(); SlateKey(EKeys::Escape); Test.TestTrue(TEXT("Escape cancels capture without closing paused menu"),Boolean(Test,Controller.Get(),TEXT("IsSettingsOpen")) && Controller->IsPaused());
			SaveKeys->OnClicked.Broadcast(); Test.TestTrue(TEXT("Actual native Save reports successful engine persist"),Status->GetText().ToString().Contains(TEXT("Bindings saved")));
			TStrongObjectPtr<UEnhancedInputUserSettings> Reload(Cast<UEnhancedInputUserSettings>(UGameplayStatics::LoadGameFromSlot(Slot->Slot,0)));
			if(!Test.TestNotNull(TEXT("Actual native UI save reloads"),Reload.Get())) return true;
			RomanInputRemapping::RegisterContexts(Config,Reload.Get()); const auto Rows=RomanInputRemapping::GetBindings(Config,Reload.Get());
			Test.TestTrue(TEXT("Actual native UI persist retains U across actual file load"),Rows.ContainsByPredicate([](const FRomanKeyBinding& Value){return Value.Name==FName(TEXT("Roman.v1.Tactical.Move_forward")) && Value.Current==EKeys::U;}));
			GetMutableDefault<UEnhancedInputDeveloperSettings>()->InputSettingsSaveSlotName=TEXT("../MustNotCreateRomanRemap"); SaveKeys->OnClicked.Broadcast();
			Test.TestTrue(TEXT("Actual native persistence rejection is honest; binding still active"),Status->GetText().ToString().Contains(TEXT("Could not save bindings")) && Current()==EKeys::U);
			GetMutableDefault<UEnhancedInputDeveloperSettings>()->InputSettingsSaveSlotName=Slot->Slot;
			Resume->OnClicked.Broadcast();
			// This synchronous fixture has not yielded another engine frame. Flush the real
			// deferred subsystem rebuild here, outside action dispatch, before asserting its view.
			FModifyContextOptions Immediate; Immediate.bForceImmediately=true;
			Input->RequestRebuildControlMappings(Immediate,EInputMappingRebuildType::RebuildWithFlush); Pump();
			Test.TestTrue(TEXT("Rebuilt real EnhancedInput mapping uses U with original action/modifiers"),PlayerInput->GetEnhancedActionMappingsView().ContainsByPredicate([&](const FEnhancedActionKeyMapping& Mapping){return Mapping.Action==Config->CameraPan && Mapping.Key==EKeys::U;}));
			const FVector Start=Camera->GetActorLocation(); Physical(EKeys::W,IE_Pressed); Pump(); Physical(EKeys::W,IE_Released); Pump();
			Test.TestTrue(TEXT("Original W no longer moves after remap"),Camera->GetActorLocation().Equals(Start,.01));
			Physical(EKeys::U,IE_Pressed); Pump(); Physical(EKeys::U,IE_Released); Pump();
			Test.TestTrue(TEXT("Actual new physical U moves real RTS camera"),!Camera->GetActorLocation().Equals(Start,.01));
			Open();
			FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::U,FModifierKeysState(),0,false,0,0)); Resume->OnClicked.Broadcast();
			const FVector HeldStart=Camera->GetActorLocation(); Input->InjectInputForAction(Config->CameraPan,FInputActionValue(FVector2D(.1,0)),{},{}); Pump();
			Test.TestTrue(TEXT("Remapped movement pressed in menu remains latched across resume"),Camera->GetActorLocation().Equals(HeldStart,.01));
			Physical(EKeys::U,IE_Released); Input->InjectInputForAction(Config->CameraPan,FInputActionValue(FVector2D(.1,0)),{},{}); Pump();
			Test.TestTrue(TEXT("Only physical release clears remapped movement latch"),!Camera->GetActorLocation().Equals(HeldStart,.01));
			Open(); Move=Cast<UButton>(Menu->GetWidgetFromName(TEXT("Binding_Roman.v1.Tactical.Move_forward")));
			Move->OnClicked.Broadcast(); SlateKey(EKeys::SpaceBar); Resume->SetUserFocus(Controller.Get());
			SlateKey(EKeys::SpaceBar); // SButton consumes key-up while invoking Resume.
			Test.TestFalse(TEXT("Keyboard Space activates real Resume while also mapped to movement"),Boolean(Test,Controller.Get(),TEXT("IsSettingsOpen")));
			const FVector ConsumedReleaseStart=Camera->GetActorLocation(); Input->InjectInputForAction(Config->CameraPan,FInputActionValue(FVector2D(.1,0)),{},{}); Pump();
			Test.TestTrue(TEXT("Release consumed by a Slate child does not leave a false movement latch"),!Camera->GetActorLocation().Equals(ConsumedReleaseStart,.01));
			Open(); Defaults->OnClicked.Broadcast(); Test.TestEqual(TEXT("Actual default button restores W"),Current(),EKeys::W);
			Test.TestTrue(TEXT("Actual defaults save reports engine result"),Status->GetText().ToString().Contains(TEXT("Bindings saved")));
			Resume->OnClicked.Broadcast();
			UClass* UnitClass=FSoftClassPath(TEXT("/Script/RomanEmpireGame.UnitBase")).TryLoadClass<APawn>(); APawn* Soldier=nullptr;
			if(UnitClass) for(TActorIterator<APawn> It(World,UnitClass);It;++It)
			{
				FCall Owner(Test,*It,TEXT("GetOwnerFaction"),1); Owner.Execute();
				if(Owner.ReadEnum<EFactionID>()==EFactionID::Rome && Boolean(Test,*It,TEXT("IsAlive"))) {Soldier=*It; break;}
			}
			if(!Test.TestNotNull(TEXT("Actual owned soldier for FPS remapping"),Soldier)) return true;
			FCall Enter(Test,Controller.Get(),TEXT("EnterFirstPersonMode"),1); Enter.SetObject(TEXT("UnitToPossess"),Soldier); Enter.Execute();
			if(!Test.TestTrue(TEXT("Actual FPS possession before remapping"),Controller->GetPawn()==Soldier)) return true;
			Open();
			auto* SoldierMove=Cast<UButton>(Menu->GetWidgetFromName(TEXT("Binding_Roman.v1.Possession.Move_forward")));
			auto* ExitKey=Cast<UButton>(Menu->GetWidgetFromName(TEXT("Binding_Roman.v1.Possession.Exit_possession")));
			if(!Test.TestTrue(TEXT("Independent authored FPS move/exit rows available"),SoldierMove && ExitKey)) return true;
			SoldierMove->OnClicked.Broadcast(); SlateKey(EKeys::K); ExitKey->OnClicked.Broadcast(); SlateKey(EKeys::G);
			Resume->OnClicked.Broadcast(); Input->RequestRebuildControlMappings(Immediate,EInputMappingRebuildType::RebuildWithFlush); Pump();
			Soldier->ConsumeMovementInputVector(); Physical(EKeys::K,IE_Pressed); Pump();
			Test.TestTrue(TEXT("New physical FPS K requests actual shared soldier movement"),Soldier->ConsumeMovementInputVector().SizeSquared()>0);
			Physical(EKeys::K,IE_Released); Pump(); Physical(EKeys::G,IE_Pressed); Pump(); Physical(EKeys::G,IE_Released); Pump();
			Test.TestTrue(TEXT("New physical FPS G restores original RTS pawn through canonical possession exit"),Controller->GetPawn()==Camera);
			Open(); Defaults->OnClicked.Broadcast(); Resume->OnClicked.Broadcast(); return true;
		}
	};
	class FWaitForInputPIEEnd : public IAutomationLatentCommand
	{
	public:
		explicit FWaitForInputPIEEnd(FAutomationTestBase* InTest) : Test(*InTest) {}
		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld) return true;
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - StartedAt < 30.0) return false;
			Test.AddError(TEXT("Input integration test PIE did not tear down within 30 seconds"));
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanPhysicalInputLifecycleTest,
	"RomanEmpireGame.Input.PhysicalKeysAndModeLifecyclePIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRomanPhysicalInputLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestNotNull(TEXT("Input integration requires an editor"), GEditor)) return false;
	if (!TestNull(TEXT("Input test owns its PIE session"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed L_Testbed loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanInputPIETests::FExercisePhysicalKeys(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanInputPIETests::FWaitForInputPIEEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanRemappingMenuTest,"RomanEmpireGame.Input.RemappingMenuPIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanRemappingMenuTest::RunTest(const FString& Parameters)
{
	if(!TestNotNull(TEXT("Editor exists"),GEditor) || !TestNull(TEXT("Own isolated remapping PIE session"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	auto Slot=MakeShared<RomanInputPIETests::FRemapSlotFixture>();
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanInputPIETests::FExerciseRemappingMenu(this,Slot));
	AddCommand(new FEndPlayMapCommand()); AddCommand(new RomanInputPIETests::FWaitForInputPIEEnd(this)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanPreferenceInputTest,"RomanEmpireGame.Preferences.InputProjectionPIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanPreferenceInputTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor"),GEditor) || !TestNull(TEXT("Own fresh PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanInputPIETests::FExercisePreferenceInputs(this));
	AddCommand(new FEndPlayMapCommand()); AddCommand(new RomanInputPIETests::FWaitForInputPIEEnd(this)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanSettingsMenuTest,"RomanEmpireGame.Preferences.SettingsMenuPIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanSettingsMenuTest::RunTest(const FString& Parameters)
{
	if(!TestNotNull(TEXT("Editor"),GEditor) || !TestNull(TEXT("Own fresh PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanInputPIETests::FExerciseSettingsMenu(this));
	AddCommand(new FEndPlayMapCommand()); AddCommand(new RomanInputPIETests::FWaitForInputPIEEnd(this)); return true;
}
#endif
