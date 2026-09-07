// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Building/BuildingTypes.h"
#include "Components/Button.h"
#include "Components/InputComponent.h"
#include "Core/GameplayMode.h"
#include "Editor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Faction/FactionData.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/PlatformTime.h"
#include "Input/RomanInputConfig.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
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
				Test.TestTrue(TEXT("Ready Tactical permits commands"), Boolean(Test, Controller.Get(), TEXT("CanIssueTacticalCommands")));
				Test.TestTrue(TEXT("Ready Tactical permits one turn request"), Boolean(Test, Controller.Get(), TEXT("CanEndTurn")));
				Test.TestTrue(TEXT("Selected completed producer is initially trainable in Tactical"), TrainButton->GetIsEnabled());
				Test.TestFalse(TEXT("Quick slot cannot open construction while the menu is closed"), QuickSlot(1));
				Tap(EKeys::One);
				Test.TestFalse(TEXT("Physical 1 also respects the closed-menu gate"), Boolean(Test, Placement, TEXT("IsPlacing")));
				Tap(EKeys::B);
				Test.TestTrue(TEXT("Physical B opens the Tactical construction menu"), Boolean(Test, Root, TEXT("IsBuildingMenuVisible")));
				Tap(EKeys::One);
				Test.TestTrue(TEXT("Physical 1 starts the authored first building preview"), Boolean(Test, Placement, TEXT("IsPlacing")));
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

			default:
				CheckContexts(EGameplayMode::Tactical);
				Key(EKeys::RightMouseButton, IE_Released);
				Test.TestFalse(TEXT("Death does not leave a phantom possession mode"), Boolean(Test, Controller.Get(), TEXT("IsInFirstPersonMode")));
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

#endif
