// Copyright Roman Empire Game. All Rights Reserved.
#include "UnitLifecycleProbe.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Units/UnitNavigationResult.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Building/BuildingTypes.h"
#include "Components/ActorComponent.h"
#include "Components/Button.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Core/GameplayMode.h"
#include "Core/SelectionOperation.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Faction/FactionData.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMemory.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Layout/ArrangedWidget.h"
#include "Layout/Geometry.h"
#include "Layout/WidgetPath.h"
#include "Tests/AutomationEditorCommon.h"
#include "Slate/SceneViewport.h"
#include "Units/UnitActivity.h"
#include "Units/UnitCombatState.h"
#include "Units/UnitOrderState.h"
#include "Units/UnitSelectionPresentation.h"
#include "Units/UnitVitalState.h"
#include "Units/UnitTypes.h"
#include "Units/RomanUnitRegistry.h"
#include "UObject/Class.h"
#include "UObject/GarbageCollection.h"
#include "UObject/ScriptDelegates.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWidget.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanEmpireUnitTests
{
	// Exercise the real reflected API without leaking runtime-private include paths.
	struct FCall
	{
		FAutomationTestBase& Test;
		UObject* Target;
		UFunction* Function;
		FStructOnScope Params;
		bool bValid;
		FCall(FAutomationTestBase& InTest, UObject* InTarget, FName Name, int32 Count)
			: Test(InTest), Target(InTarget), Function(Target ? Target->FindFunction(Name) : nullptr), Params(Function),
			bValid(Test.TestTrue(*FString::Printf(TEXT("Function %s has expected parameter count"), *Name.ToString()),
				IsValid(Target) && Function && Function->NumParms == Count)) {}
		template<typename P> P* Property(FName Name, bool bReturn = false)
		{
			P* Result = bValid ? FindFProperty<P>(Function, Name) : nullptr;
			bValid = Test.TestTrue(*FString::Printf(TEXT("Parameter %s has expected type and direction"), *Name.ToString()),
				Result && Result->HasAnyPropertyFlags(CPF_Parm) && Result->HasAnyPropertyFlags(CPF_ReturnParm) == bReturn) && bValid;
			return bValid ? Result : nullptr;
		}
		template<typename P, typename T> void Set(FName Name, T Value)
		{
			if (P* Prop = Property<P>(Name)) Prop->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
		}
		template<typename P, typename T> T Read(T Default = T())
		{
			P* Prop = Property<P>(TEXT("ReturnValue"), true);
			return Prop ? Prop->GetPropertyValue_InContainer(Params.GetStructMemory()) : Default;
		}
		void Execute() { if (bValid) Target->ProcessEvent(Function, Params.GetStructMemory()); }
	};
	bool Bool(FAutomationTestBase& Test, UObject* Unit, FName Name)
	{
		FCall Call(Test, Unit, Name, 1); Call.Execute(); return Call.Read<FBoolProperty, bool>();
	}
	int32 Int(FAutomationTestBase& Test, UObject* Unit, FName Name)
	{
		FCall Call(Test, Unit, Name, 1); Call.Execute(); return Call.Read<FIntProperty, int32>();
	}
	float Stamina(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("GetCurrentStamina"), 1); Call.Execute(); return Call.Read<FFloatProperty, float>();
	}
	float Recovery(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("GetAttackRecoveryRemaining"), 1); Call.Execute(); return Call.Read<FFloatProperty, float>();
	}
	EUnitOrderType Order(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("GetOrderType"), 1);
		FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("ReturnValue"), true);
		if (!Test.TestTrue(TEXT("Order query uses the public enum"), Prop && Prop->GetEnum() == StaticEnum<EUnitOrderType>())) return EUnitOrderType::None;
		Call.Execute();
		return static_cast<EUnitOrderType>(Prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory())));
	}
	EUnitActivity Activity(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("GetActivity"), 1);
		FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("ReturnValue"), true);
		if (!Test.TestTrue(TEXT("Activity query uses the public enum"), Prop && Prop->GetEnum() == StaticEnum<EUnitActivity>())) return EUnitActivity::Unavailable;
		Call.Execute();
		return static_cast<EUnitActivity>(Prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory())));
	}
	EUnitNavigationResult NavigationResult(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("GetNavigationResult"), 1);
		FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("ReturnValue"), true);
		if (!Test.TestTrue(TEXT("Navigation query uses public enum"), Prop && Prop->GetEnum() == StaticEnum<EUnitNavigationResult>())) return EUnitNavigationResult::None;
		Call.Execute();
		return static_cast<EUnitNavigationResult>(Prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory())));
	}
	EUnitLifeState Life(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("GetLifeState"), 1);
		FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("ReturnValue"), true);
		if (!Test.TestTrue(TEXT("Life query uses the public enum"), Prop && Prop->GetEnum() == StaticEnum<EUnitLifeState>())) return EUnitLifeState::Uninitialized;
		Call.Execute();
		return static_cast<EUnitLifeState>(Prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory())));
	}
	void Command(FAutomationTestBase& Test, UObject* Unit, FName Name) { FCall Call(Test, Unit, Name, 0); Call.Execute(); }
	void SetBool(FAutomationTestBase& Test, UObject* Unit, FName Name, FName Param, bool Value)
	{
		FCall Call(Test, Unit, Name, 1); Call.Set<FBoolProperty>(Param, Value); Call.Execute();
	}
	void Health(FAutomationTestBase& Test, UObject* Unit, int32 Value)
	{
		FCall Call(Test, Unit, TEXT("SetCurrentHealth"), 1); Call.Set<FIntProperty>(TEXT("NewHealth"), Value); Call.Execute();
	}
	void Damage(FAutomationTestBase& Test, UObject* Unit, float Value, AActor* Source = nullptr)
	{
		FCall Call(Test, Unit, TEXT("TakeCombatDamage"), 3);
		Call.Set<FFloatProperty>(TEXT("Damage"), Value);
		Call.Set<FBoolProperty>(TEXT("bIsRanged"), false);
		if (FObjectPropertyBase* Property = Call.Property<FObjectPropertyBase>(TEXT("DamageSource")))
			Property->SetObjectPropertyValue_InContainer(Call.Params.GetStructMemory(), Source);
		Call.Execute();
	}
	void Move(FAutomationTestBase& Test, UObject* Unit, const FVector& Destination = FVector(45000.0, 40000.0, 100.0))
	{
		FCall Call(Test, Unit, TEXT("CommandMoveTo"), 1);
		FStructProperty* Prop = Call.Property<FStructProperty>(TEXT("Destination"));
		if (!Test.TestTrue(TEXT("Move takes FVector"), Prop && Prop->Struct == TBaseStructure<FVector>::Get())) return;
		Prop->CopyCompleteValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Destination);
		Call.Execute();
	}
	void Attack(FAutomationTestBase& Test, UObject* Unit, AActor* Target)
	{
		FCall Call(Test, Unit, TEXT("CommandAttack"), 1);
		FObjectPropertyBase* Prop = Call.Property<FObjectPropertyBase>(TEXT("Target"));
		if (Prop) Prop->SetObjectPropertyValue_InContainer(Call.Params.GetStructMemory(), Target);
		Call.Execute();
	}
	bool Initialize(FAutomationTestBase& Test, UObject* Unit, FName ID, EFactionID Faction)
	{
		FCall Call(Test, Unit, TEXT("InitializeUnit"), 3);
		Call.Set<FNameProperty>(TEXT("DefinitionID"), ID);
		FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("NewOwner"));
		if (!Test.TestTrue(TEXT("Initialization faction type"), Prop && Prop->GetEnum() == StaticEnum<EFactionID>())) return false;
		Prop->GetUnderlyingProperty()->SetIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), static_cast<int64>(Faction));
		Call.Execute(); return Call.Read<FBoolProperty, bool>();
	}
	void SetOwner(FAutomationTestBase& Test, UObject* Unit, EFactionID Faction)
	{
		FCall Call(Test, Unit, TEXT("SetOwnerFaction"), 1);
		FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("NewOwner"));
		if (!Test.TestTrue(TEXT("Owner setter uses faction enum"), Prop && Prop->GetEnum() == StaticEnum<EFactionID>())) return;
		Prop->GetUnderlyingProperty()->SetIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), static_cast<int64>(Faction));
		Call.Execute();
	}
	void Dodge(FAutomationTestBase& Test, UObject* Unit)
	{
		FCall Call(Test, Unit, TEXT("PerformDodge"), 1);
		FStructProperty* Prop = Call.Property<FStructProperty>(TEXT("Direction"));
		if (!Test.TestTrue(TEXT("Dodge takes FVector2D"), Prop && Prop->Struct == TBaseStructure<FVector2D>::Get())) return;
		const FVector2D Direction(1.0, 0.0);
		Prop->CopyCompleteValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Direction);
		Call.Execute();
	}
	struct FInitializationSnapshot
	{
		FName DefinitionID;
		EFactionID Owner = EFactionID::None;
		FUnitData Definition;
		EUnitLifeState State = EUnitLifeState::Uninitialized;
		int32 Health = 0, MaxHealth = 0, Morale = 0;
		float CurrentStamina = 0.0f;
		FInitializationSnapshot(FAutomationTestBase& Test, UObject* Unit)
		{
			FCall IDCall(Test, Unit, TEXT("GetUnitDefinitionID"), 1); IDCall.Execute();
			DefinitionID = IDCall.Read<FNameProperty, FName>();
			FCall OwnerCall(Test, Unit, TEXT("GetOwnerFaction"), 1);
			FEnumProperty* OwnerProp = OwnerCall.Property<FEnumProperty>(TEXT("ReturnValue"), true);
			if (Test.TestTrue(TEXT("Owner query uses faction enum"), OwnerProp && OwnerProp->GetEnum() == StaticEnum<EFactionID>()))
			{
				OwnerCall.Execute();
				Owner = static_cast<EFactionID>(OwnerProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
					OwnerProp->ContainerPtrToValuePtr<void>(OwnerCall.Params.GetStructMemory())));
			}
			FCall DataCall(Test, Unit, TEXT("GetUnitData"), 1);
			FStructProperty* DataProp = DataCall.Property<FStructProperty>(TEXT("ReturnValue"), true);
			if (Test.TestTrue(TEXT("Definition query uses public unit struct"), DataProp && DataProp->Struct == FUnitData::StaticStruct()))
			{
				DataCall.Execute();
				DataProp->CopyCompleteValue(&Definition, DataProp->ContainerPtrToValuePtr<void>(DataCall.Params.GetStructMemory()));
			}
			State = Life(Test, Unit);
			Health = Int(Test, Unit, TEXT("GetCurrentHealth"));
			MaxHealth = Int(Test, Unit, TEXT("GetMaxHealth"));
			Morale = Int(Test, Unit, TEXT("GetCurrentMorale"));
			CurrentStamina = Stamina(Test, Unit);
		}
		void CheckUnchanged(FAutomationTestBase& Test, UObject* Unit) const
		{
			const FInitializationSnapshot After(Test, Unit);
			Test.TestEqual(TEXT("Snapshot definition ID unchanged"), After.DefinitionID, DefinitionID);
			Test.TestEqual(TEXT("Snapshot owner unchanged"), After.Owner, Owner);
			Test.TestTrue(TEXT("Snapshot complete definition unchanged"), FUnitData::StaticStruct()->CompareScriptStruct(&Definition, &After.Definition, 0));
			Test.TestEqual(TEXT("Snapshot lifecycle unchanged"), After.State, State);
			Test.TestEqual(TEXT("Snapshot health unchanged"), After.Health, Health);
			Test.TestEqual(TEXT("Snapshot maximum health unchanged"), After.MaxHealth, MaxHealth);
			Test.TestEqual(TEXT("Snapshot morale unchanged"), After.Morale, Morale);
			Test.TestEqual(TEXT("Snapshot stamina unchanged"), After.CurrentStamina, CurrentStamina);
		}
		void CheckAuthored(FAutomationTestBase& Test, const FUnitData& Authored, EFactionID Faction) const
		{
			Test.TestEqual(TEXT("Authored stable ID applied"), DefinitionID, Authored.UnitID);
			Test.TestEqual(TEXT("Requested faction applied"), Owner, Faction);
			Test.TestTrue(TEXT("Complete authored definition applied"), FUnitData::StaticStruct()->CompareScriptStruct(&Definition, &Authored, 0));
			Test.TestEqual(TEXT("Initialized state is alive"), State, EUnitLifeState::Alive);
			Test.TestEqual(TEXT("Authored health applied"), Health, Authored.BaseStats.MaxHealth);
			Test.TestEqual(TEXT("Authored maximum health applied"), MaxHealth, Authored.BaseStats.MaxHealth);
			Test.TestEqual(TEXT("Authored morale applied"), Morale, Authored.BaseStats.Morale);
			Test.TestEqual(TEXT("Authored stamina applied"), CurrentStamina, Authored.BaseStats.Stamina);
		}
	};
	bool BindProbe(FAutomationTestBase& Test, UObject* Unit, FName Event, UUnitLifecycleProbe* Probe, FName Handler)
	{
		FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(Unit->GetClass(), Event);
		UFunction* Function = Probe->FindFunction(Handler);
		if (!Test.TestTrue(TEXT("Probe signature matches actual unit event"), Property && Function &&
			Property->SignatureFunction && Property->SignatureFunction->IsSignatureCompatibleWith(Function))) return false;
		FScriptDelegate Delegate;
		Delegate.BindUFunction(Probe, Handler);
		Property->AddDelegate(Delegate, Unit);
		return true;
	}
	UObject* Component(FAutomationTestBase& Test, UObject* Unit, FName Field)
	{
		FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Unit->GetClass(), Field);
		if (!Test.TestNotNull(*FString::Printf(TEXT("Reflected component %s exists"), *Field.ToString()), Prop)) return nullptr;
		return Prop->GetObjectPropertyValue_InContainer(Unit);
	}
	template<typename T> T StoredState(FAutomationTestBase& Test, UObject* Unit, FName Field)
	{
		// ProcessEvent intentionally rejects destroyed actors. Inspect their retained
		// reflected storage synchronously before GC instead of reading default params.
		FStructProperty* Prop = FindFProperty<FStructProperty>(Unit->GetClass(), Field);
		T Result;
		if (Test.TestTrue(*FString::Printf(TEXT("Actor retains typed %s storage"), *Field.ToString()), Prop && Prop->Struct == T::StaticStruct()))
		{
			Prop->CopyCompleteValue(&Result, Prop->ContainerPtrToValuePtr<void>(Unit));
		}
		return Result;
	}
	FUnitVitalState StoredVitals(FAutomationTestBase& Test, UObject* Unit)
	{
		return StoredState<FUnitVitalState>(Test, Unit, TEXT("Vitals"));
	}
	void CheckTargetReset(FAutomationTestBase& Test, const FUnitOrderState& Orders)
	{
		FWeakObjectProperty* TargetProp = FindFProperty<FWeakObjectProperty>(FUnitOrderState::StaticStruct(), TEXT("Target"));
		if (Test.TestNotNull(TEXT("Order target is a weak reflected reference"), TargetProp))
			Test.TestTrue(TEXT("Attack target reset, not merely invalid"), TargetProp->GetPropertyValue_InContainer(&Orders).IsExplicitlyNull());
	}
	bool StoredBool(FAutomationTestBase& Test, UObject* Unit, FName Field)
	{
		FBoolProperty* Prop = FindFProperty<FBoolProperty>(Unit->GetClass(), Field);
		return Test.TestNotNull(*Field.ToString(), Prop) && Prop->GetPropertyValue_InContainer(Unit);
	}
	void CheckComponents(FAutomationTestBase& Test, AActor* Unit)
	{
		const TPair<FName, FName> Names[] = {
			{TEXT("VisualMesh"), TEXT("VisualMesh")}, {TEXT("SelectionDecal"), TEXT("SelectionDecal")},
			{TEXT("SelectionRingMesh"), TEXT("SelectionRing")}, {TEXT("HeadMesh"), TEXT("HeadMesh")},
			{TEXT("WeaponMesh"), TEXT("WeaponMesh")}, {TEXT("ShieldMesh"), TEXT("ShieldMesh")},
			{TEXT("FPSCameraArm"), TEXT("FPSCameraArm")}, {TEXT("FPSCamera"), TEXT("FPSCamera")},
			{TEXT("RightArmMesh"), TEXT("RightArm")}, {TEXT("LeftArmMesh"), TEXT("LeftArm")},
			{TEXT("RightLegMesh"), TEXT("RightLeg")}, {TEXT("LeftLegMesh"), TEXT("LeftLeg")},
			{TEXT("RightHandMesh"), TEXT("RightHand")}, {TEXT("LeftHandMesh"), TEXT("LeftHand")},
			{TEXT("RightFootMesh"), TEXT("RightFoot")}, {TEXT("LeftFootMesh"), TEXT("LeftFoot")}
		};
		for (const auto& Pair : Names)
		{
			USceneComponent* Part = Cast<USceneComponent>(Component(Test, Unit, Pair.Key));
			if (!Test.TestNotNull(*Pair.Key.ToString(), Part)) continue;
			Test.TestTrue(TEXT("Component retains its constructor identity"), Part == Unit->GetDefaultSubobjectByName(Pair.Value));
			Test.TestTrue(TEXT("Component belongs to this actor"), Part->GetOwner() == Unit);
			USceneComponent* Parent = Pair.Key == TEXT("FPSCamera")
				? Cast<USceneComponent>(Component(Test, Unit, TEXT("FPSCameraArm"))) : Unit->GetRootComponent();
			Test.TestTrue(TEXT("Component attachment retained"), Part->GetAttachParent() == Parent);
		}
	}
	void CheckQuiescent(FAutomationTestBase& Test, AActor* Unit)
	{
		if (IsValid(Unit)) Test.TestFalse(TEXT("Unit cannot accept commands"), Bool(Test, Unit, TEXT("CanAcceptCommands")));
		else Test.TestEqual(TEXT("Destroyed unit retains terminal state"), StoredVitals(Test, Unit).GetLifeState(), EUnitLifeState::Ended);
		const FUnitOrderState Orders = StoredState<FUnitOrderState>(Test, Unit, TEXT("Orders"));
		const FUnitCombatState Combat = StoredState<FUnitCombatState>(Test, Unit, TEXT("Combat"));
		const FUnitSelectionPresentation Selection = StoredState<FUnitSelectionPresentation>(Test, Unit, TEXT("SelectionPresentation"));
		Test.TestFalse(TEXT("Selection cleared"), Selection.IsSelected());
		Test.TestFalse(TEXT("Possession flag cleared"), StoredBool(Test, Unit, TEXT("bIsPossessedByPlayer")));
		Test.TestFalse(TEXT("Blocking cleared"), Combat.IsBlocking());
		Test.TestFalse(TEXT("Special action cleared"), StoredBool(Test, Unit, TEXT("bInTestudo")));
		Test.TestFalse(TEXT("Move command cleared"), Orders.HasMoveCommand());
		Test.TestEqual(TEXT("Active order cleared"), Orders.GetOrderType(), EUnitOrderType::None);
		Test.TestTrue(TEXT("Order destination cleared"), Orders.GetDestination().IsZero());
		Test.TestFalse(TEXT("Attack animation cleared"), Combat.IsAttackInProgress());
		Test.TestEqual(TEXT("Attack cooldown cleared"), Combat.GetRecoveryRemaining(), 0.0f);
		UStaticMeshComponent* Ring = Cast<UStaticMeshComponent>(Component(Test, Unit, TEXT("SelectionRingMesh")));
		if (Test.TestNotNull(TEXT("Corpse selection ring exists"), Ring)) Test.TestTrue(TEXT("Corpse selection ring fully hidden"), !Ring->IsVisible() && Ring->bHiddenInGame);
		CheckTargetReset(Test, Orders);
		Test.TestFalse(TEXT("Corpse collision disabled"), Unit->GetActorEnableCollision());
		ACharacter* Character = Cast<ACharacter>(Unit);
		if (Test.TestNotNull(TEXT("Unit is a character"), Character))
		{
			Test.TestEqual(TEXT("Movement disabled"), Character->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
			Test.TestTrue(TEXT("Velocity cleared"), Character->GetVelocity().IsNearlyZero());
			Test.TestTrue(TEXT("Pending dodge launch cleared"), Character->GetCharacterMovement()->PendingLaunchVelocity.IsNearlyZero());
		}
		UCameraComponent* Camera = Cast<UCameraComponent>(Component(Test, Unit, TEXT("FPSCamera")));
		if (Camera) Test.TestFalse(TEXT("Camera inactive"), Camera->IsActive());
	}

	void ObjectCommand(FAutomationTestBase& Test, UObject* Target, FName Function, FName Param, UObject* Value);

	class FExerciseNavigation : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseNavigation(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 90.0)
			{
				Test.AddError(FString::Printf(TEXT("Navigation PIE timeout in phase %d; unit %s"), Phase,
					Unit.IsValid() ? *Unit->GetActorLocation().ToString() : TEXT("not spawned")));
				return true;
			}
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Nav) return false;
			if (Phase == 0)
			{
				// Keep bootstrap troops from interfering; do not disable world, movement,
				// navigation or AI ticks. All geometry here is transient PIE-only.
				for (TActorIterator<ACharacter> It(World); It; ++It)
					if (It->FindFunction(TEXT("CommandHold"))) Command(Test, *It, TEXT("CommandHold"));
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				AStaticMeshActor* Wall = World->SpawnActor<AStaticMeshActor>(FVector(0, -3500, 240), FRotator::ZeroRotator, Params);
				if (!Test.TestNotNull(TEXT("Transient navigation obstacle"), Wall)) return true;
				Obstacle = Wall;
				UStaticMeshComponent* Mesh = Wall->GetStaticMeshComponent();
				Mesh->SetMobility(EComponentMobility::Movable);
				Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
				Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Mesh->SetCollisionResponseToAllChannels(ECR_Block);
				Wall->SetActorScale3D(FVector(3, 10, 5));
				Phase = 1;
				return false;
			}
			if (Phase == 1)
			{
				if (Nav->IsNavigationBuildInProgress()) return false;
				FNavLocation StartPoint, EndPoint;
				if (!Nav->ProjectPointToNavigation(FVector(-1200, -3500, 0), StartPoint, FVector(100, 100, 300)) ||
					!Nav->ProjectPointToNavigation(FVector(1200, -3500, 0), EndPoint, FVector(100, 100, 300))) return false;
				UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, StartPoint.Location, EndPoint.Location);
				if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() < 3) return false;
				const ANavigationData* Data = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
				if (!Test.TestNotNull(TEXT("Runtime NavMesh exists"), Data)) return true;
				Test.TestEqual(TEXT("Navigation radius matches unit capsule"), Data->GetConfig().AgentRadius, 42.0f);
				Test.TestEqual(TEXT("Navigation height matches unit capsule"), Data->GetConfig().AgentHeight, 192.0f);
				Test.AddInfo(FString::Printf(TEXT("Full obstacle route has %d corners"), Path->PathPoints.Num()));
				Start = StartPoint.Location + FVector(0, 0, 100);
				Goal = EndPoint.Location;
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.ObjectFlags |= RF_Transient;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
				ACharacter* Spawned = World->SpawnActor<ACharacter>(Class, FTransform(Start), Params);
				if (!Test.TestNotNull(TEXT("Navigation fixture unit"), Spawned)) return true;
				Unit = Spawned;
				if (!Initialize(Test, Spawned, TEXT("Legionary"), EFactionID::Rome)) return true;
				Spawned->FinishSpawning(Spawned->GetActorTransform());
				AI = Cast<AAIController>(Spawned->GetController());
				if (!Test.TestTrue(TEXT("Unit owns tactical AI controller"), AI.IsValid() && AI->GetClass()->GetFName() == TEXT("RomanUnitAIController"))) return true;
				Move(Test, Spawned, Goal);
				Phase = 2;
				return false;
			}
			if (!Test.TestTrue(TEXT("Navigation fixture remains alive"), Unit.IsValid() && AI.IsValid())) return true;
			ACharacter* Actor = Unit.Get();
			if (Phase == 2)
			{
				const FVector Location = Actor->GetActorLocation();
				bMovedWithVelocity |= Actor->GetVelocity().Size2D() > 1.0;
				MaxDetour = FMath::Max(MaxDetour, FMath::Abs(Location.Y - Start.Y));
				if (FMath::Abs(Location.X) < 150 && FMath::Abs(Location.Y + 3500) < 500)
				{
					Test.AddError(TEXT("Character crossed blocking obstacle")); return true;
				}
				const EUnitNavigationResult Result = NavigationResult(Test, Actor);
				if (Result == EUnitNavigationResult::Unreachable) { Test.AddError(TEXT("Valid obstacle route failed during execution")); return true; }
				if (Result != EUnitNavigationResult::Arrived) return false;
				Test.TestTrue(TEXT("Actual CharacterMovement produced velocity"), bMovedWithVelocity);
				Test.TestTrue(TEXT("Unit walked around obstacle"), MaxDetour > 500);
				Test.TestTrue(TEXT("Unit reached requested goal"), FVector::Dist2D(Location, Goal) < 60);
				Test.TestEqual(TEXT("Arrival consumes Move order"), Order(Test, Actor), EUnitOrderType::None);
				Move(Test, Actor, Start);
				Phase = 3;
				return false;
			}
			if (Phase == 3)
			{
				if (NavigationResult(Test, Actor) != EUnitNavigationResult::Moving) return false;
				const FVector ReplacementGoal = Goal + FVector(0, -700, 0);
				UPathFollowingComponent* Following = AI->GetPathFollowingComponent();
				bool bObservedCommittedHold = false;
				const FDelegateHandle Handle = Following->OnRequestFinished.AddLambda(
					[&](FAIRequestID, const FPathFollowingResult&)
					{
						bObservedCommittedHold = Order(Test, Actor) == EUnitOrderType::Hold;
						Move(Test, Actor, ReplacementGoal);
					});
				Command(Test, Actor, TEXT("CommandHold"));
				Following->OnRequestFinished.Remove(Handle);
				Test.TestTrue(TEXT("Abort observer sees committed Hold"), bObservedCommittedHold);
				Test.TestEqual(TEXT("Hold cannot overwrite observer replacement"), Order(Test, Actor), EUnitOrderType::Move);
				Test.TestEqual(TEXT("Observer replacement destination survives"), StoredState<FUnitOrderState>(Test, Actor, TEXT("Orders")).GetDestination(), ReplacementGoal);
				Move(Test, Actor, Goal + FVector(0, -700, 0));
				Test.TestEqual(TEXT("Abort completion cannot erase replacement Move"), Order(Test, Actor), EUnitOrderType::Move);
				Command(Test, Actor, TEXT("CommandHold"));
				Test.TestEqual(TEXT("Hold cancels path following"), AI->GetMoveStatus(), EPathFollowingStatus::Idle);
				Test.TestTrue(TEXT("Hold cancels movement velocity"), Actor->GetVelocity().IsNearlyZero());
				Move(Test, Actor, FVector(100000, -3500, 0));
				Phase = 4;
				return false;
			}
			if (Phase == 4)
			{
				if (NavigationResult(Test, Actor) != EUnitNavigationResult::Unreachable) return false;
				Test.TestEqual(TEXT("Unreachable goal leaves Hold without retry"), Order(Test, Actor), EUnitOrderType::Hold);
				Test.TestEqual(TEXT("Failed route leaves idle path follower"), AI->GetMoveStatus(), EPathFollowingStatus::Idle);
				Test.TestTrue(TEXT("Unreachable goal did not teleport unit"), Actor->GetActorLocation().X < 2000);
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.ObjectFlags |= RF_Transient;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ACharacter* Spawned = World->SpawnActor<ACharacter>(Actor->GetClass(), FTransform(Goal + FVector(0, -1300, 100)), Params);
				if (!Test.TestNotNull(TEXT("Pursuit target spawned"), Spawned)) return true;
				Enemy = Spawned;
				if (!Initialize(Test, Spawned, TEXT("Legionary"), EFactionID::Carthage)) return true;
				Spawned->FinishSpawning(Spawned->GetActorTransform());
				Command(Test, Spawned, TEXT("CommandHold"));
				Attack(Test, Actor, Spawned);
				Phase = 5;
				return false;
			}
			if (!Test.TestTrue(TEXT("Pursuit target remains alive"), Enemy.IsValid())) return true;
			if (Phase == 5)
			{
				if (NavigationResult(Test, Actor) != EUnitNavigationResult::Moving) return false;
				Test.TestTrue(TEXT("Path submission retains attack target"), StoredState<FUnitOrderState>(Test, Actor, TEXT("Orders")).GetTarget() == Enemy.Get());
				PursuitRevision = StoredState<FUnitOrderState>(Test, Actor, TEXT("Orders")).GetRevision();
				Enemy->SetActorLocation(Enemy->GetActorLocation() + FVector(700, 0, 0));
				Phase = 6;
				return false;
			}
			if (Phase == 6)
			{
				if (NavigationResult(Test, Actor) != EUnitNavigationResult::Arrived) return false;
				const FUnitOrderState Orders = StoredState<FUnitOrderState>(Test, Actor, TEXT("Orders"));
				Test.TestTrue(TEXT("Pursuit reaches moved target, not its old location"),
					FVector::Dist2D(Actor->GetActorLocation(), Enemy->GetActorLocation()) <= StoredState<FUnitData>(Test, Actor, TEXT("UnitData")).AttackRange);
				Test.TestEqual(TEXT("Moving target keeps original Attack revision"), Orders.GetRevision(), PursuitRevision);
				Test.TestEqual(TEXT("Pursuit follows current target location"), Orders.GetDestination(), Enemy->GetActorLocation());
				Test.TestTrue(TEXT("Moving pursuit retains target identity"), Orders.GetTarget() == Enemy.Get());
				Test.TestEqual(TEXT("Arrival in attack range stops path following"), AI->GetMoveStatus(), EPathFollowingStatus::Idle);
				Move(Test, Actor, Goal);
				Phase = 8;
				return false;
			}
			if (Phase == 8)
			{
				if (NavigationResult(Test, Actor) != EUnitNavigationResult::Moving) return false;
				APlayerController* Player = World->GetFirstPlayerController();
				APawn* Camera = Player->GetPawn();
				for (int32 Cycle = 0; Cycle < 2; ++Cycle)
				{
					ObjectCommand(Test, Player, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Actor);
					Test.TestTrue(TEXT("Real player possession replaces AI"), Actor->GetController() == Player);
					Test.TestNull(TEXT("Cached tactical controller is parked"), AI->GetPawn());
					Test.TestEqual(TEXT("Possession cancels path"), AI->GetMoveStatus(), EPathFollowingStatus::Idle);
					Command(Test, Player, TEXT("ExitFirstPersonMode"));
					Test.TestTrue(TEXT("Exit reuses the same tactical controller"), Actor->GetController() == AI.Get());
					Test.TestTrue(TEXT("Exit restores actual tactical camera"), Player->GetPawn() == Camera);
				}
				Move(Test, Actor, Goal);
				Phase = 7;
				return false;
			}
			if (NavigationResult(Test, Actor) != EUnitNavigationResult::Moving) return false;
			Health(Test, Actor, 0);
			Test.TestEqual(TEXT("Death cancels active path"), AI->GetMoveStatus(), EPathFollowingStatus::Idle);
			Test.TestTrue(TEXT("Death clears movement velocity"), Actor->GetVelocity().IsNearlyZero());
			Actor->Destroy();
			Test.TestFalse(TEXT("EndPlay destroys cached tactical controller"), AI.IsValid());
			Enemy->Destroy();
			if (Obstacle.IsValid()) Obstacle->Destroy();
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		int32 Phase = 0;
		TWeakObjectPtr<ACharacter> Unit;
		TWeakObjectPtr<ACharacter> Enemy;
		TWeakObjectPtr<AAIController> AI;
		TWeakObjectPtr<AStaticMeshActor> Obstacle;
		FVector Start = FVector::ZeroVector;
		FVector Goal = FVector::ZeroVector;
		double MaxDetour = 0;
		bool bMovedWithVelocity = false;
		uint64 PursuitRevision = 0;
	};

	class FExerciseDisconnectedNavigation : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseDisconnectedNavigation(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 120.0)
			{
				Test.AddError(FString::Printf(TEXT("Disconnected navigation PIE timed out in phase %d; unit %s"), Phase,
					Unit.IsValid() ? *Unit->GetActorLocation().ToString() : TEXT("not spawned")));
				return true;
			}
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Nav) return false;
			if (Phase == 0)
			{
				for (TActorIterator<ACharacter> It(World); It; ++It)
					if (It->FindFunction(TEXT("CommandHold"))) Command(Test, *It, TEXT("CommandHold"));
				UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				if (!Test.TestNotNull(TEXT("Wall fixture mesh"), Cube)) return true;
				auto SpawnWall = [&](const FVector& Center, const FVector& Scale)
				{
					FActorSpawnParameters Params;
					Params.ObjectFlags |= RF_Transient;
					AStaticMeshActor* Wall = World->SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator, Params);
					if (!Wall) return static_cast<AStaticMeshActor*>(nullptr);
					UStaticMeshComponent* Mesh = Wall->GetStaticMeshComponent();
					Mesh->SetMobility(EComponentMobility::Movable);
					Mesh->SetStaticMesh(Cube);
					Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					Mesh->SetCollisionResponseToAllChannels(ECR_Block);
					Wall->SetActorScale3D(Scale);
					Walls.Add(Wall);
					return Wall;
				};
				// The two long sections exceed the NavMesh bounds. The 400 cm opening
				// between them is navigable for a 42 cm-radius agent only after the gate goes.
				if (!Test.TestNotNull(TEXT("South wall"), SpawnWall(FVector(6000, -5100, 240), FVector(3, 28, 5))) ||
					!Test.TestNotNull(TEXT("North wall"), SpawnWall(FVector(6000, 1600, 240), FVector(3, 98, 5))) ||
					!Test.TestNotNull(TEXT("Passage gate"), SpawnWall(FVector(6000, -3500, 240), FVector(3, 5, 5)))) return true;
				Gate = Walls.Last();
				Phase = 1;
				PhaseStartedAt = World->GetTimeSeconds();
				return false;
			}
			if (Phase == 1)
			{
				if (World->GetTimeSeconds() - PhaseStartedAt < 1.0f || Nav->IsNavigationBuildInProgress()) return false;
				FNavLocation StartPoint, EndPoint;
				if (!Nav->ProjectPointToNavigation(FVector(5000, -3500, 0), StartPoint, FVector(100, 100, 300)) ||
					!Nav->ProjectPointToNavigation(FVector(7000, -3500, 0), EndPoint, FVector(100, 100, 300))) return false;
				Start = StartPoint.Location;
				Goal = EndPoint.Location;
				UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Start, Goal);
				if (Path && Path->IsValid() && !Path->IsPartial()) return false;
				UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
				if (!Test.TestNotNull(TEXT("Disconnected fixture class"), Class)) return true;
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.ObjectFlags |= RF_Transient;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ACharacter* Spawned = World->SpawnActor<ACharacter>(Class, FTransform(Start + FVector(0, 0, 100)), Params);
				if (!Test.TestNotNull(TEXT("Disconnected fixture unit"), Spawned)) return true;
				Unit = Spawned;
				if (!Initialize(Test, Spawned, TEXT("Legionary"), EFactionID::Rome)) return true;
				Spawned->FinishSpawning(Spawned->GetActorTransform());
				AI = Cast<AAIController>(Spawned->GetController());
				if (!Test.TestNotNull(TEXT("Disconnected fixture AI"), AI.Get())) return true;
				Move(Test, Spawned, Goal);
				Phase = 2;
				return false;
			}
			if (!Test.TestTrue(TEXT("Disconnected fixture remains alive"), Unit.IsValid() && AI.IsValid())) return true;
			ACharacter* Actor = Unit.Get();
			if (Phase == 2)
			{
				if (NavigationResult(Test, Actor) != EUnitNavigationResult::Unreachable) return false;
				Test.TestEqual(TEXT("Disconnected goal settles in Hold"), Order(Test, Actor), EUnitOrderType::Hold);
				Test.TestEqual(TEXT("Disconnected route leaves path follower idle"), AI->GetMoveStatus(), EPathFollowingStatus::Idle);
				Test.TestTrue(TEXT("Disconnected goal cannot teleport through gate"), Actor->GetActorLocation().X < 5500);
				if (!Test.TestTrue(TEXT("Gate survives until opening"), Gate.IsValid())) return true;
				Gate->Destroy();
				Phase = 3;
				PhaseStartedAt = World->GetTimeSeconds();
				return false;
			}
			if (Phase == 3)
			{
				if (World->GetTimeSeconds() - PhaseStartedAt < 1.0f || Nav->IsNavigationBuildInProgress()) return false;
				UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Actor->GetActorLocation(), Goal);
				if (!Path || !Path->IsValid() || Path->IsPartial()) return false;
				Move(Test, Actor, Goal);
				Phase = 4;
				return false;
			}
			const FVector Location = Actor->GetActorLocation();
			bMovedWithVelocity |= Actor->GetVelocity().Size2D() > 1.0;
			if (FMath::Abs(Location.X - 6000) < 175 && FMath::Abs(Location.Y + 3500) > 180)
			{
				Test.AddError(TEXT("Unit crossed a solid wall instead of the narrow opening"));
				return true;
			}
			const EUnitNavigationResult Result = NavigationResult(Test, Actor);
			if (Result == EUnitNavigationResult::Unreachable)
			{
				Test.AddError(TEXT("Opened passage failed during execution"));
				return true;
			}
			if (Result != EUnitNavigationResult::Arrived) return false;
			Test.TestTrue(TEXT("Opened passage used CharacterMovement"), bMovedWithVelocity);
			Test.TestTrue(TEXT("Unit reached far side through narrow opening"), Location.X > 6500 && FVector::Dist2D(Location, Goal) < 60);
			Test.TestEqual(TEXT("Arrival consumes opened-passage Move"), Order(Test, Actor), EUnitOrderType::None);
			Actor->Destroy();
			for (const TWeakObjectPtr<AStaticMeshActor>& Wall : Walls) if (Wall.IsValid()) Wall->Destroy();
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		float PhaseStartedAt = 0;
		int32 Phase = 0;
		TWeakObjectPtr<ACharacter> Unit;
		TWeakObjectPtr<AAIController> AI;
		TArray<TWeakObjectPtr<AStaticMeshActor>> Walls;
		TWeakObjectPtr<AStaticMeshActor> Gate;
		FVector Start = FVector::ZeroVector;
		FVector Goal = FVector::ZeroVector;
		bool bMovedWithVelocity = false;
	};

	class FExerciseUnitOrders : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseUnitOrders(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Unit order/combat PIE test timed out")); return true; }
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestNotNull(TEXT("Order test uses real Legionary class"), Class)) return true;
			// Every early return releases only this scenario's actors. Component ticks
			// are disabled too: these checks advance the coordinator explicitly, once.
			struct FFixtures
			{
				TArray<TWeakObjectPtr<ACharacter>> Actors;
				~FFixtures() { for (const auto& Actor : Actors) if (Actor.IsValid()) Actor->Destroy(); }
			} Fixtures;
			auto SetAttackSpeed = [&](ACharacter* Actor, float Value)
			{
				FStructProperty* Prop = FindFProperty<FStructProperty>(Class, TEXT("UnitData"));
				if (!Test.TestTrue(TEXT("Fixture config uses typed instance definition"), Prop && Prop->Struct == FUnitData::StaticStruct())) return false;
				// Instance-only fixture setup. The canonical asset is never modified.
				FUnitData* Data = Prop->ContainerPtrToValuePtr<FUnitData>(Actor);
				Data->BaseStats.AttackSpeed = Value;
				Data->BaseStats.MeleeAttack = 1; // Stable, nonlethal hits against real defense.
				return true;
			};
			auto Spawn = [&](EFactionID Faction, const FVector& Location)
			{
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.ObjectFlags |= RF_Transient;
				ACharacter* Actor = World->SpawnActor<ACharacter>(Class, FTransform(Location), Params);
				if (!Test.TestNotNull(TEXT("Order fixture spawned"), Actor)) return static_cast<ACharacter*>(nullptr);
				Fixtures.Actors.Add(Actor);
				Test.TestEqual(TEXT("Uninitialized activity is unavailable"), Activity(Test, Actor), EUnitActivity::Unavailable);
				if (!Initialize(Test, Actor, TEXT("Legionary"), Faction) || !SetAttackSpeed(Actor, 2.0f)) return static_cast<ACharacter*>(nullptr);
				Test.TestEqual(TEXT("Initialized deferred activity remains unavailable"), Activity(Test, Actor), EUnitActivity::Unavailable);
				Actor->FinishSpawning(Actor->GetActorTransform());
				Actor->SetActorTickEnabled(false);
				Actor->GetCharacterMovement()->SetComponentTickEnabled(false);
				Actor->SetActorEnableCollision(false);
				Test.TestEqual(TEXT("Begun unit derives idle activity"), Activity(Test, Actor), EUnitActivity::Idle);
				return Actor;
			};
			auto CheckTarget = [&](ACharacter* Actor, AActor* Target)
			{
				const FUnitOrderState Orders = StoredState<FUnitOrderState>(Test, Actor, TEXT("Orders"));
				Test.TestTrue(TEXT("Order owner retains expected target"), Orders.GetTarget() == Target);
				FCall Query(Test, Actor, TEXT("GetAttackTarget"), 1); Query.Execute();
				FObjectPropertyBase* Prop = Query.Property<FObjectPropertyBase>(TEXT("ReturnValue"), true);
				if (Prop) Test.TestTrue(TEXT("Public target projects order owner"), Prop->GetObjectPropertyValue_InContainer(Query.Params.GetStructMemory()) == Target);
			};
			const FVector Origin(60000.0, 0.0, 400.0);
			ACharacter* Unit = Spawn(EFactionID::Rome, Origin);
			ACharacter* Enemy = Spawn(EFactionID::Carthage, Origin + FVector(1000.0, 0.0, 0.0));
			if (!Unit || !Enemy) return true;
			Attack(Test, Unit, Enemy);
			Test.TestEqual(TEXT("Attack command owns explicit attack order"), Order(Test, Unit), EUnitOrderType::Attack);
			Test.TestEqual(TEXT("Pursuit derives commanded activity"), Activity(Test, Unit), EUnitActivity::Commanded);
			Test.TestTrue(TEXT("Attack starts pursuit"), Bool(Test, Unit, TEXT("HasMoveCommand")));
			CheckTarget(Unit, Enemy);
			Enemy->SetActorLocation(Origin + FVector(1200.0, 200.0, 0.0));
			Unit->Tick(0.125f);
			CheckTarget(Unit, nullptr);
			Test.TestEqual(TEXT("Off-nav pursuit settles in Hold"), Order(Test, Unit), EUnitOrderType::Hold);
			Test.TestEqual(TEXT("Off-nav pursuit reports failure"), NavigationResult(Test, Unit), EUnitNavigationResult::Unreachable);
			Test.TestEqual(TEXT("Off-nav pursuit never bypasses navigation by teleporting"), Unit->GetActorLocation(), Origin);
			Move(Test, Unit, Origin + FVector(0.0, 1000.0, 0.0));
			Test.TestEqual(TEXT("Move replaces attack order"), Order(Test, Unit), EUnitOrderType::Move);
			CheckTarget(Unit, nullptr);
			CheckTargetReset(Test, StoredState<FUnitOrderState>(Test, Unit, TEXT("Orders")));
			Command(Test, Unit, TEXT("CommandStop"));
			Test.TestEqual(TEXT("Stop clears current order"), Order(Test, Unit), EUnitOrderType::None);
			Test.TestEqual(TEXT("Stop derives idle activity"), Activity(Test, Unit), EUnitActivity::Idle);
			Attack(Test, Unit, Enemy);
			Command(Test, Unit, TEXT("CommandHold"));
			Test.TestEqual(TEXT("Hold replaces pursuit"), Order(Test, Unit), EUnitOrderType::Hold);
			Test.TestEqual(TEXT("Hold derives commanded activity"), Activity(Test, Unit), EUnitActivity::Commanded);
			CheckTargetReset(Test, StoredState<FUnitOrderState>(Test, Unit, TEXT("Orders")));
			const FVector HeldPosition = Unit->GetActorLocation();
			const int32 HeldEnemyHealth = Int(Test, Enemy, TEXT("GetCurrentHealth"));
			for (int32 Index = 0; Index < 3; ++Index) Unit->Tick(0.125f);
			Test.TestEqual(TEXT("Hold persists with nearby enemy"), Order(Test, Unit), EUnitOrderType::Hold);
			Test.TestEqual(TEXT("Hold does not move"), Unit->GetActorLocation(), HeldPosition);
			Test.TestEqual(TEXT("Hold does not auto-acquire or attack"), Int(Test, Enemy, TEXT("GetCurrentHealth")), HeldEnemyHealth);
			Test.TestFalse(TEXT("Hold has no movement command"), Bool(Test, Unit, TEXT("HasMoveCommand")));
			CheckTarget(Unit, nullptr);
			Attack(Test, Unit, nullptr);
			Attack(Test, Unit, Unit);
			Move(Test, Unit, FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0));
			Test.TestEqual(TEXT("Invalid commands preserve Hold"), Order(Test, Unit), EUnitOrderType::Hold);
			Attack(Test, Unit, Enemy);
			Health(Test, Enemy, 0);
			Test.TestEqual(TEXT("Death projects terminal activity"), Activity(Test, Enemy), EUnitActivity::Dead);
			Unit->Tick(0.125f);
			Test.TestEqual(TEXT("Dead target retires attack order"), Order(Test, Unit), EUnitOrderType::None);
			Test.TestEqual(TEXT("Retired pursuit cannot move toward dead target"), Unit->GetActorLocation(), HeldPosition);
			CheckTargetReset(Test, StoredState<FUnitOrderState>(Test, Unit, TEXT("Orders")));
			Enemy->Destroy();
			ACharacter* DestroyedEnemy = Spawn(EFactionID::Carthage, Origin + FVector(1500.0, 0.0, 0.0));
			if (!DestroyedEnemy) return true;
			Attack(Test, Unit, DestroyedEnemy);
			DestroyedEnemy->Destroy();
			Unit->Tick(0.125f);
			Test.TestEqual(TEXT("Destroyed target retires attack order"), Order(Test, Unit), EUnitOrderType::None);
			Test.TestEqual(TEXT("Expired weak target cannot drive movement"), Unit->GetActorLocation(), HeldPosition);
			CheckTargetReset(Test, StoredState<FUnitOrderState>(Test, Unit, TEXT("Orders")));

			const FVector CombatOrigin(100000.0, 0.0, 400.0);
			ACharacter* Fighter = Spawn(EFactionID::Rome, CombatOrigin);
			ACharacter* Defender = Spawn(EFactionID::Carthage, CombatOrigin + FVector(1.0, 0.0, 0.0));
			if (!Fighter || !Defender) return true;
			const int32 BeforeHealth = Int(Test, Defender, TEXT("GetCurrentHealth"));
			Attack(Test, Fighter, Defender);
			Fighter->Tick(0.125f);
			Test.TestEqual(TEXT("AI windup does not damage immediately"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth);
			Test.TestEqual(TEXT("AI windup starts full recovery without charging current frame twice"), Recovery(Test, Fighter), 0.5f);
			Test.TestTrue(TEXT("AI windup reserves exactly one impact"), StoredState<FUnitCombatState>(Test, Fighter, TEXT("Combat")).HasPendingImpact());
			Fighter->Tick(0.125f);
			Test.TestEqual(TEXT("AI first window applies one hit"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 1);
			Test.TestEqual(TEXT("Impact consumes elapsed recovery once"), Recovery(Test, Fighter), 0.375f);
			Test.TestFalse(TEXT("First impact cannot repeat"), StoredState<FUnitCombatState>(Test, Fighter, TEXT("Combat")).HasPendingImpact());
			Test.TestEqual(TEXT("AI recovery derives attacking activity"), Activity(Test, Fighter), EUnitActivity::Attacking);
			Test.TestTrue(TEXT("AI attack marks combat in progress"), Bool(Test, Fighter, TEXT("IsAttacking")));
			Test.TestFalse(TEXT("In-range attack pauses movement"), Bool(Test, Fighter, TEXT("HasMoveCommand")));
			CheckTarget(Fighter, Defender);
			for (int32 Index = 1; Index <= 2; ++Index)
			{
				Fighter->Tick(0.125f);
				Test.TestEqual(TEXT("AI recovery advances exactly once per explicit tick"), Recovery(Test, Fighter), 0.375f - Index * 0.125f);
				Test.TestEqual(TEXT("AI cannot strike again before complete recovery"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 1);
			}
			Fighter->Tick(0.125f);
			Test.TestEqual(TEXT("Second windup also waits before impact"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 1);
			Test.TestEqual(TEXT("Second windup starts one full recovery"), Recovery(Test, Fighter), 0.5f);
			Fighter->Tick(0.125f);
			Test.TestEqual(TEXT("Second AI window applies one additional hit"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 2);

			ACharacter* CancelAttacker = Spawn(EFactionID::Rome, FVector(120000.0, 0.0, 400.0));
			ACharacter* CancelTarget = Spawn(EFactionID::Carthage, FVector(120001.0, 0.0, 400.0));
			if (!CancelAttacker || !CancelTarget) return true;
			const int32 CancelTargetHealth = Int(Test, CancelTarget, TEXT("GetCurrentHealth"));
			Attack(Test, CancelAttacker, CancelTarget);
			CancelAttacker->Tick(0.125f);
			Command(Test, CancelAttacker, TEXT("CommandHold"));
			CancelAttacker->Tick(0.125f);
			Test.TestEqual(TEXT("Hold during windup prevents damage"), Int(Test, CancelTarget, TEXT("GetCurrentHealth")), CancelTargetHealth);
			Test.TestFalse(TEXT("Hold consumes pending impact"), StoredState<FUnitCombatState>(Test, CancelAttacker, TEXT("Combat")).HasPendingImpact());
			Test.TestEqual(TEXT("Hold does not refund recovery"), Recovery(Test, CancelAttacker), 0.375f);
			Damage(Test, CancelAttacker, 10.0f, Fighter);
			Test.TestEqual(TEXT("Same-faction damage source is rejected"), Int(Test, CancelAttacker, TEXT("GetCurrentHealth")), Int(Test, CancelAttacker, TEXT("GetMaxHealth")));
			Damage(Test, CancelAttacker, 10.0f, CancelAttacker);
			Test.TestEqual(TEXT("Self damage source is rejected"), Int(Test, CancelAttacker, TEXT("GetCurrentHealth")), Int(Test, CancelAttacker, TEXT("GetMaxHealth")));
			Damage(Test, CancelTarget, 1.0f, Fighter);
			Test.TestEqual(TEXT("Enemy damage source uses single damage entry"), Int(Test, CancelTarget, TEXT("GetCurrentHealth")), CancelTargetHealth - 1);

			ACharacter* DodgedAttacker = Spawn(EFactionID::Rome, FVector(130000.0, 0.0, 400.0));
			ACharacter* DodgedTarget = Spawn(EFactionID::Carthage, FVector(130001.0, 0.0, 400.0));
			if (!DodgedAttacker || !DodgedTarget) return true;
			const int32 DodgedHealth = Int(Test, DodgedTarget, TEXT("GetCurrentHealth"));
			Attack(Test, DodgedAttacker, DodgedTarget);
			DodgedAttacker->Tick(0.125f);
			DodgedTarget->SetActorLocation(FVector(132000.0, 0.0, 400.0));
			DodgedAttacker->Tick(0.125f);
			Test.TestEqual(TEXT("Target leaving melee range before impact avoids damage"), Int(Test, DodgedTarget, TEXT("GetCurrentHealth")), DodgedHealth);
			Test.TestFalse(TEXT("Missed impact is consumed once"), StoredState<FUnitCombatState>(Test, DodgedAttacker, TEXT("Combat")).HasPendingImpact());

			ACharacter* PlayerUnit = Spawn(EFactionID::Rome, FVector(140000.0, 0.0, 400.0));
			if (!PlayerUnit) return true;
			const float UnpossessedStamina = Stamina(Test, PlayerUnit);
			Command(Test, PlayerUnit, TEXT("PerformAttack"));
			Test.TestEqual(TEXT("Direct attack requires possession"), Stamina(Test, PlayerUnit), UnpossessedStamina);
			Test.TestEqual(TEXT("Unpossessed direct attack cannot reserve impact"), Recovery(Test, PlayerUnit), 0.0f);
			SetBool(Test, PlayerUnit, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), true);
			TStrongObjectPtr<UUnitLifecycleProbe> AttackProbe(NewObject<UUnitLifecycleProbe>());
			AttackProbe->Test = &Test;
			AttackProbe->ObservedUnit = PlayerUnit;
			AttackProbe->bReattackOnVitals = true;
			if (!BindProbe(Test, PlayerUnit, TEXT("OnUnitVitalsChanged"), AttackProbe.Get(), TEXT("HandleVitals"))) return true;
			const float BeforeStamina = Stamina(Test, PlayerUnit);
			for (const float InvalidSpeed : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::min() * 0.125f})
			{
				SetAttackSpeed(PlayerUnit, InvalidSpeed);
				Command(Test, PlayerUnit, TEXT("PerformAttack"));
				Test.TestEqual(TEXT("Invalid attack speed cannot consume stamina"), Stamina(Test, PlayerUnit), BeforeStamina);
				Test.TestEqual(TEXT("Invalid attack speed cannot start recovery"), Recovery(Test, PlayerUnit), 0.0f);
			}
			Test.TestEqual(TEXT("Rejected attacks emit no vitals notification"), AttackProbe->VitalsEvents, 0);
			SetAttackSpeed(PlayerUnit, 2.0f);
			Command(Test, PlayerUnit, TEXT("PerformAttack"));
			Test.TestTrue(TEXT("Attack observer attempted nested attack"), AttackProbe->bHandledVitalsReattack);
			Test.TestEqual(TEXT("FPS attack spends stamina exactly once including nested request"), Stamina(Test, PlayerUnit), BeforeStamina - 10.0f);
			Test.TestEqual(TEXT("Only committed attack emits vitals notification"), AttackProbe->VitalsEvents, 1);
			Test.TestEqual(TEXT("FPS duration derives inverse speed once, not squared"), Recovery(Test, PlayerUnit), 0.5f);
			Command(Test, PlayerUnit, TEXT("PerformAttack"));
			Test.TestEqual(TEXT("Repeated attack cannot spend stamina during recovery"), Stamina(Test, PlayerUnit), BeforeStamina - 10.0f);
			PlayerUnit->Tick(0.125f);
			Test.TestEqual(TEXT("FPS recovery advances once"), Recovery(Test, PlayerUnit), 0.375f);
			Test.TestEqual(TEXT("Attack recovery suppresses stamina regeneration"), Stamina(Test, PlayerUnit), BeforeStamina - 10.0f);
			PlayerUnit->Tick(0.375f);
			Test.TestEqual(TEXT("FPS recovery completes at exact duration"), Recovery(Test, PlayerUnit), 0.0f);
			Test.TestFalse(TEXT("Completed recovery clears attacking projection"), Bool(Test, PlayerUnit, TEXT("IsAttacking")));
			Test.TestEqual(TEXT("Completed recovery derives idle activity"), Activity(Test, PlayerUnit), EUnitActivity::Idle);
			SetAttackSpeed(PlayerUnit, 4.0f);
			const float BeforeNextAttack = Stamina(Test, PlayerUnit);
			Command(Test, PlayerUnit, TEXT("PerformAttack"));
			Test.TestEqual(TEXT("Next attack derives current instance speed once"), Recovery(Test, PlayerUnit), 0.25f);
			Test.TestEqual(TEXT("Next legal attack spends once"), Stamina(Test, PlayerUnit), BeforeNextAttack - 10.0f);
			PlayerUnit->Tick(0.25f);
			ACharacter* DirectVictim = Spawn(EFactionID::Carthage, PlayerUnit->GetActorLocation() + FVector(80, 0, 0));
			if (!DirectVictim) return true;
			PlayerUnit->SetActorEnableCollision(true);
			DirectVictim->SetActorEnableCollision(true);
			const int32 DirectHealth = Int(Test, DirectVictim, TEXT("GetCurrentHealth"));
			Command(Test, PlayerUnit, TEXT("PerformAttack"));
			Test.TestEqual(TEXT("Direct attack does not trace at input time"), Int(Test, DirectVictim, TEXT("GetCurrentHealth")), DirectHealth);
			PlayerUnit->Tick(0.03125f);
			Test.TestEqual(TEXT("Direct hit waits for its window"), Int(Test, DirectVictim, TEXT("GetCurrentHealth")), DirectHealth);
			PlayerUnit->Tick(0.03125f);
			Test.TestEqual(TEXT("Direct window applies exactly one validated hit"), Int(Test, DirectVictim, TEXT("GetCurrentHealth")), DirectHealth - 1);
			PlayerUnit->Tick(0.03125f);
			Test.TestEqual(TEXT("Direct hit cannot repeat inside recovery"), Int(Test, DirectVictim, TEXT("GetCurrentHealth")), DirectHealth - 1);

			int32 ScenarioIndex = 0;
			for (const EUnitProbeDamageAction Action : {EUnitProbeDamageAction::MoveAttacker, EUnitProbeDamageAction::HoldAttacker,
				EUnitProbeDamageAction::KillAttacker, EUnitProbeDamageAction::DestroyAttacker, EUnitProbeDamageAction::KillTarget, EUnitProbeDamageAction::DestroyTarget})
			{
				const FVector Location(180000.0 + 20000.0 * ScenarioIndex++, 0.0, 400.0);
				ACharacter* Attacker = Spawn(EFactionID::Rome, Location);
				ACharacter* Target = Spawn(EFactionID::Carthage, Location + FVector(1.0, 0.0, 0.0));
				if (!Attacker || !Target) return true;
				TStrongObjectPtr<UUnitLifecycleProbe> DamageProbe(NewObject<UUnitLifecycleProbe>());
				DamageProbe->Test = &Test;
				DamageProbe->ObservedUnit = Target;
				DamageProbe->ActionUnit = Attacker;
				DamageProbe->DamageAction = Action;
				if (!BindProbe(Test, Target, TEXT("OnUnitDamaged"), DamageProbe.Get(), TEXT("HandleDamage"))) return true;
				const float AttackerStamina = Stamina(Test, Attacker);
				Attack(Test, Attacker, Target);
				Attacker->Tick(0.125f);
				Test.TestEqual(TEXT("Damage observer is not called during windup"), DamageProbe->DamageEvents, 0);
				Attacker->Tick(0.125f);
				Test.TestEqual(TEXT("AI hit notifies target once despite nested direct attack"), DamageProbe->DamageEvents, 1);
				Test.TestEqual(TEXT("Damage observer sees committed elapsed recovery"), DamageProbe->RecoveryAtDamage, 0.375f);
				Test.TestEqual(TEXT("Damage callback cannot start another attack or spend stamina"), StoredVitals(Test, Attacker).GetStamina(), AttackerStamina);
				if (Action == EUnitProbeDamageAction::KillAttacker || Action == EUnitProbeDamageAction::DestroyAttacker)
				{
					Test.TestEqual(TEXT("Nested attacker termination persists"), StoredVitals(Test, Attacker).GetLifeState(),
						Action == EUnitProbeDamageAction::KillAttacker ? EUnitLifeState::Dead : EUnitLifeState::Ended);
					CheckQuiescent(Test, Attacker);
				}
				else
				{
					const EUnitOrderType Expected = Action == EUnitProbeDamageAction::MoveAttacker ? EUnitOrderType::Move :
						Action == EUnitProbeDamageAction::HoldAttacker ? EUnitOrderType::Hold : EUnitOrderType::None;
					Test.TestEqual(TEXT("Damage callback replacement/cancellation survives outer AI attack"), Order(Test, Attacker), Expected);
					CheckTargetReset(Test, StoredState<FUnitOrderState>(Test, Attacker, TEXT("Orders")));
					Test.TestEqual(TEXT("Nested order change cannot charge recovery twice"), Recovery(Test, Attacker), 0.375f);
					Test.TestEqual(TEXT("Outer attack cannot perform stale target movement after callback"), Attacker->GetActorLocation(), Location);
				}
			}

			for (const bool bKill : {false, true})
			{
				ACharacter* Possessed = Spawn(EFactionID::Rome, FVector(400000.0, bKill ? 20000.0 : 0.0, 400.0));
				if (!Possessed) return true;
				UCameraComponent* Camera = Cast<UCameraComponent>(Component(Test, Possessed, TEXT("FPSCamera")));
				if (!Test.TestNotNull(TEXT("Activation reentry uses actual FPS camera"), Camera)) return true;
				const float NormalSpeed = Possessed->GetCharacterMovement()->MaxWalkSpeed;
				TStrongObjectPtr<UUnitLifecycleProbe> CameraProbe(NewObject<UUnitLifecycleProbe>());
				CameraProbe->Test = &Test;
				CameraProbe->ObservedUnit = Possessed;
				CameraProbe->bKillOnCameraActivation = bKill;
				Camera->OnComponentActivated.AddDynamic(CameraProbe.Get(), &UUnitLifecycleProbe::HandleCameraActivated);
				SetBool(Test, Possessed, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), true);
				Test.TestEqual(TEXT("Camera observer handles one real activation"), CameraProbe->CameraActivationEvents, 1);
				Test.TestFalse(TEXT("Nested activation exit leaves camera inactive"), Camera->IsActive());
				Test.TestFalse(TEXT("Outer activation cannot restore stale possession flag"), Bool(Test, Possessed, TEXT("IsPossessedByPlayer")));
				if (bKill)
				{
					Test.TestEqual(TEXT("Camera activation observer can commit death"), Life(Test, Possessed), EUnitLifeState::Dead);
					CheckQuiescent(Test, Possessed);
				}
				else
				{
					Test.TestEqual(TEXT("Outer possession cannot overwrite callback-restored movement speed"), Possessed->GetCharacterMovement()->MaxWalkSpeed, NormalSpeed);
					Test.TestEqual(TEXT("Outer possession cannot cancel callback's replacement move"), Order(Test, Possessed), EUnitOrderType::Move);
					Test.TestTrue(TEXT("Callback replacement movement remains active"), Bool(Test, Possessed, TEXT("HasMoveCommand")));
				}
			}
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
	};

	class FExerciseUnits : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseUnits(FAutomationTestBase* InTest) : Test(*InTest) {}
		virtual bool Update() override
		{
			if (StartedAt == 0.0) StartedAt = FPlatformTime::Seconds();
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Unit lifecycle PIE test timed out")); return true; }
			if (Stage == 1)
			{
				if (!World || World != OwnedWorld.Get()) { Test.AddError(TEXT("Owned unit PIE world ended early")); return true; }
				if (World->GetTimeSeconds() - DeathTime < 3.3f) return false;
				Test.TestFalse(TEXT("Low-altitude corpse expires on time"), Low.IsValid());
				Test.TestFalse(TEXT("High-altitude corpse expires on same deadline"), High.IsValid());
				Test.TestFalse(TEXT("Invalid spawn has bounded cleanup"), Invalid.IsValid());
				Test.TestEqual(TEXT("Lifespan does not deliver another death"), Probe->DeathEvents, 1);
				for (const auto& NestedProbe : NestedProbes)
				{
					Test.TestFalse(TEXT("Nested termination leaves no live unit"), NestedProbe->ObservedUnit.IsValid());
					Test.TestEqual(TEXT("Nested termination has no delayed death callbacks"), NestedProbe->DeathEvents, NestedProbe->bDestroyDuringVitals ? 0 : 1);
					Test.TestEqual(TEXT("Nested termination has no delayed vitals callbacks"), NestedProbe->VitalsEvents, NestedProbe->bDestroyDuringVitals ? 1 : 2);
				}
				return true;
			}
			ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!World || !State || !State->IsGameplayInitialized()) return false;
			OwnedWorld = World;
			UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestNotNull(TEXT("Real Legionary class"), Class)) return true;
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(World);
			const URomanUnitDefinition* Definition = Registry ? Registry->FindUnit(TEXT("Legionary")) : nullptr;
			if (!Test.TestNotNull(TEXT("Canonical Legionary definition"), Definition)) return true;
			if (!Test.TestTrue(TEXT("Authored Legionary supports both encounter factions"),
				Definition->AvailableToFactions.Contains(TEXT("Rome")) && Definition->AvailableToFactions.Contains(TEXT("Carthage")))) return true;
			CheckComponents(Test, Class->GetDefaultObject<AActor>());
			auto Spawn = [&](float Z)
			{
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.ObjectFlags |= RF_Transient;
				return World->SpawnActor<ACharacter>(Class, FTransform(FVector(40000.0, 40000.0, Z)), Params);
			};
			ACharacter* Unit = Spawn(-500.0f);
			if (!Test.TestNotNull(TEXT("Deferred fixture"), Unit)) return true;
			Low = Unit;
			CheckComponents(Test, Unit);
			Test.TestEqual(TEXT("Deferred unit has explicit uninitialized state"), Life(Test, Unit), EUnitLifeState::Uninitialized);
			const FInitializationSnapshot Uninitialized(Test, Unit);
			Command(Test, Unit, TEXT("PerformAttack"));
			Command(Test, Unit, TEXT("StartBlocking"));
			SetBool(Test, Unit, TEXT("SetSelected"), TEXT("bNewSelected"), true);
			SetBool(Test, Unit, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), true);
			Test.TestFalse(TEXT("Uninitialized cannot be selected"), Bool(Test, Unit, TEXT("IsSelected")));
			Test.TestFalse(TEXT("Uninitialized cannot be possessed"), Bool(Test, Unit, TEXT("IsPossessedByPlayer")));
			Test.TestFalse(TEXT("Invalid definition rejected atomically"), Initialize(Test, Unit, TEXT("MissingUnit"), EFactionID::Rome));
			Test.TestEqual(TEXT("Invalid initialization preserves state"), Life(Test, Unit), EUnitLifeState::Uninitialized);
			Uninitialized.CheckUnchanged(Test, Unit);
			Test.TestFalse(TEXT("Invalid faction rejected atomically"), Initialize(Test, Unit, TEXT("Legionary"), EFactionID::None));
			Uninitialized.CheckUnchanged(Test, Unit);
			Test.TestTrue(TEXT("Initialize valid deferred spawn"), Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Rome));
			const FInitializationSnapshot Initialized(Test, Unit);
			Initialized.CheckAuthored(Test, Definition->Definition, EFactionID::Rome);
			const int32 InitialHealth = Int(Test, Unit, TEXT("GetCurrentHealth"));
			Test.TestTrue(TEXT("Deferred initialization applies health before BeginPlay"), InitialHealth > 0);
			Test.TestFalse(TEXT("Initialized deferred actor cannot act before BeginPlay"), Bool(Test, Unit, TEXT("CanAcceptCommands")));
			Health(Test, Unit, 1);
			Test.TestEqual(TEXT("Deferred health command rejected"), Int(Test, Unit, TEXT("GetCurrentHealth")), InitialHealth);
			Test.TestFalse(TEXT("Second initialization with different valid faction rejected"), Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Carthage));
			Initialized.CheckUnchanged(Test, Unit);
			Unit->FinishSpawning(Unit->GetActorTransform());
			Unit->SetActorTickEnabled(false);
			Initialized.CheckUnchanged(Test, Unit);
			Test.TestTrue(TEXT("Begun unit accepts commands"), Bool(Test, Unit, TEXT("CanAcceptCommands")));
			Health(Test, Unit, InitialHealth - 1);
			const FInitializationSnapshot Damaged(Test, Unit);
			Test.TestFalse(TEXT("Reinitialize cannot heal or transfer live unit"), Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Carthage));
			Damaged.CheckUnchanged(Test, Unit);
			Test.TestEqual(TEXT("Health remains damaged"), Int(Test, Unit, TEXT("GetCurrentHealth")), InitialHealth - 1);
			Damage(Test, Unit, 0.0f);
			Damage(Test, Unit, -10.0f);
			Damage(Test, Unit, std::numeric_limits<float>::quiet_NaN());
			Damage(Test, Unit, std::numeric_limits<float>::infinity());
			Damage(Test, Unit, -std::numeric_limits<float>::infinity());
			Test.TestEqual(TEXT("Invalid damage preserves health"), Int(Test, Unit, TEXT("GetCurrentHealth")), InitialHealth - 1);
			Damaged.CheckUnchanged(Test, Unit);
			ACharacter* BeginPlayUnit = Spawn(8000.0f);
			if (!Test.TestNotNull(TEXT("BeginPlay initialization fixture"), BeginPlayUnit)) return true;
			SetOwner(Test, BeginPlayUnit, EFactionID::Rome);
			Test.TestFalse(TEXT("Configuring owner alone does not initialize"), Bool(Test, BeginPlayUnit, TEXT("IsInitialized")));
			BeginPlayUnit->FinishSpawning(BeginPlayUnit->GetActorTransform());
			FInitializationSnapshot(Test, BeginPlayUnit).CheckAuthored(Test, Definition->Definition, EFactionID::Rome);
			Test.TestTrue(TEXT("Normal BeginPlay initialization accepts commands"), Bool(Test, BeginPlayUnit, TEXT("CanAcceptCommands")));
			BeginPlayUnit->Destroy();
			SetBool(Test, Unit, TEXT("SetSelected"), TEXT("bNewSelected"), true);
			UStaticMeshComponent* Ring = Cast<UStaticMeshComponent>(Component(Test, Unit, TEXT("SelectionRingMesh")));
			if (Test.TestNotNull(TEXT("Selection ring exists"), Ring))
			{
				Test.TestTrue(TEXT("Selected ring visible and not hidden in game"), Ring->IsVisible() && !Ring->bHiddenInGame);
				UObject* Material = Ring->GetMaterial(0);
				SetBool(Test, Unit, TEXT("SetSelected"), TEXT("bNewSelected"), false);
				Test.TestTrue(TEXT("Deselected ring hidden"), !Ring->IsVisible() && Ring->bHiddenInGame);
				SetBool(Test, Unit, TEXT("SetSelected"), TEXT("bNewSelected"), true);
				Test.TestTrue(TEXT("Repeated selection reuses material"), Material && Ring->GetMaterial(0) == Material);
			}
			SetBool(Test, Unit, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), true);
			UCameraComponent* Camera = Cast<UCameraComponent>(Component(Test, Unit, TEXT("FPSCamera")));
			if (Test.TestNotNull(TEXT("FPS camera exists"), Camera)) Test.TestTrue(TEXT("Possession activates actual camera"), Camera->IsActive());
			SetBool(Test, Unit, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), false);
			if (Camera) Test.TestFalse(TEXT("Normal possession exit deactivates actual camera"), Camera->IsActive());
			Test.TestFalse(TEXT("Normal exit clears possession flag"), Bool(Test, Unit, TEXT("IsPossessedByPlayer")));
			Test.TestEqual(TEXT("Normal exit restores authored speed"), Unit->GetCharacterMovement()->MaxWalkSpeed, Definition->Definition.BaseStats.Speed);
			SetBool(Test, Unit, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), true);
			if (Camera) Test.TestTrue(TEXT("Camera supports another possession"), Camera->IsActive());
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			CheckComponents(Test, Unit);
			Command(Test, Unit, TEXT("PerformAttack"));
			Unit->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Unit->GetCharacterMovement()->Activate();
			Dodge(Test, Unit);
			Test.TestFalse(TEXT("Live dodge establishes pending launch before death"), Unit->GetCharacterMovement()->PendingLaunchVelocity.IsNearlyZero());
			Command(Test, Unit, TEXT("StartBlocking"));
			Command(Test, Unit, TEXT("ActivateTestudo"));
			ACharacter* Enemy = Spawn(10000.0f);
			if (!Test.TestNotNull(TEXT("Enemy fixture"), Enemy)) return true;
			Test.TestTrue(TEXT("Enemy definition and faction valid"), Initialize(Test, Enemy, TEXT("Legionary"), EFactionID::Carthage));
			Enemy->FinishSpawning(Enemy->GetActorTransform());
			Enemy->SetActorTickEnabled(false);
			Move(Test, Unit);
			Test.TestTrue(TEXT("Living unit accepts move order"), Bool(Test, Unit, TEXT("HasMoveCommand")));
			Attack(Test, Unit, Enemy);
			FCall AttackQuery(Test, Unit, TEXT("GetAttackTarget"), 1); AttackQuery.Execute();
			FObjectPropertyBase* AttackProp = AttackQuery.Property<FObjectPropertyBase>(TEXT("ReturnValue"), true);
			if (AttackProp) Test.TestTrue(TEXT("Attack order preserves target while establishing movement"),
				AttackProp->GetObjectPropertyValue_InContainer(AttackQuery.Params.GetStructMemory()) == Enemy);
			Unit->GetCharacterMovement()->Velocity = FVector(100.0, 20.0, 0.0);
			Probe.Reset(NewObject<UUnitLifecycleProbe>());
			Probe->Test = &Test;
			Probe->ObservedUnit = Unit;
			FMulticastDelegateProperty* Death = FindFProperty<FMulticastDelegateProperty>(Class, TEXT("OnUnitDied"));
			if (!Test.TestNotNull(TEXT("Real death event"), Death)) return true;
			if (!Test.TestTrue(TEXT("Death observer signature is compatible"), Death->SignatureFunction &&
				Death->SignatureFunction->IsSignatureCompatibleWith(Probe->FindFunction(TEXT("HandleDeath"))))) return true;
			FScriptDelegate Delegate;
			Delegate.BindUFunction(Probe.Get(), TEXT("HandleDeath"));
			Death->AddDelegate(Delegate, Unit);
			Health(Test, Unit, 0);
			Test.TestEqual(TEXT("Exactly one death event"), Probe->DeathEvents, 1);
			CheckQuiescent(Test, Unit);
			Move(Test, Unit);
			Attack(Test, Unit, Enemy);
			Damage(Test, Unit, MAX_flt);
			CheckQuiescent(Test, Unit);
			Test.TestEqual(TEXT("Post-death damage cannot deliver another death"), Probe->DeathEvents, 1);
			Enemy->Destroy();
			Test.TestTrue(TEXT("Corpse lifespan scheduled before return"), Unit->GetLifeSpan() > 2.5f && Unit->GetLifeSpan() <= 3.0f);
			ACharacter* HighUnit = Spawn(20000.0f);
			if (!Test.TestNotNull(TEXT("High-altitude fixture"), HighUnit)) return true;
			High = HighUnit;
			Initialize(Test, HighUnit, TEXT("Legionary"), EFactionID::Rome);
			HighUnit->FinishSpawning(HighUnit->GetActorTransform());
			Damage(Test, HighUnit, MAX_flt);
			Test.TestEqual(TEXT("Large finite damage commits death without integer overflow"), Life(Test, HighUnit), EUnitLifeState::Dead);
			Unit->Tick(0.1f);
			HighUnit->Tick(0.1f);
			Test.TestTrue(TEXT("Low corpse is not destroyed by absolute Z threshold"), IsValid(Unit));
			Test.TestTrue(TEXT("High corpse remains until its deadline"), IsValid(HighUnit));
			ACharacter* InvalidUnit = Spawn(10000.0f);
			if (!Test.TestNotNull(TEXT("Invalid spawn fixture"), InvalidUnit)) return true;
			Invalid = InvalidUnit;
			Test.AddExpectedError(TEXT("has no valid content definition"), EAutomationExpectedErrorFlags::Contains, 1);
			InvalidUnit->FinishSpawning(InvalidUnit->GetActorTransform());
			Test.TestEqual(TEXT("Missing owner produces explicit failure state"), Life(Test, InvalidUnit), EUnitLifeState::InitializationFailed);
			Test.TestFalse(TEXT("Invalid spawn collision disabled"), InvalidUnit->GetActorEnableCollision());
			Test.TestFalse(TEXT("Invalid spawn tick disabled"), InvalidUnit->IsActorTickEnabled());
			Test.TestFalse(TEXT("Failed spawn cannot initialize late"), Initialize(Test, InvalidUnit, TEXT("Legionary"), EFactionID::Rome));
			ACharacter* Early = Spawn(5000.0f);
			if (!Test.TestNotNull(TEXT("Early destruction fixture"), Early)) return true;
			Initialize(Test, Early, TEXT("Legionary"), EFactionID::Rome);
			Early->FinishSpawning(Early->GetActorTransform());
			Move(Test, Early);
			Death->AddDelegate(Delegate, Early);
			Early->SetLifeSpan(2.0f);
			Early->Destroy();
			Test.TestEqual(TEXT("Early destruction commits Ended"), StoredVitals(Test, Early).GetLifeState(), EUnitLifeState::Ended);
			CheckQuiescent(Test, Early);
			Test.TestEqual(TEXT("Early destruction clears scheduled lifespan"), Early->GetLifeSpan(), 0.0f);
			Test.TestEqual(TEXT("Destroy is not a combat death event"), Probe->DeathEvents, 1);
			for (const bool bDestroy : {false, true})
			{
				ACharacter* Nested = Spawn(12000.0f);
				if (!Test.TestNotNull(TEXT("Nested vitals fixture"), Nested)) return true;
				Test.TestTrue(TEXT("Nested fixture initializes"), Initialize(Test, Nested, TEXT("Legionary"), EFactionID::Rome));
				Nested->FinishSpawning(Nested->GetActorTransform());
				Nested->SetActorTickEnabled(false);
				TStrongObjectPtr<UUnitLifecycleProbe> NestedProbe(NewObject<UUnitLifecycleProbe>());
				NestedProbe->Test = &Test;
				NestedProbe->ObservedUnit = Nested;
				NestedProbe->bTerminateOnVitals = true;
				NestedProbe->bDestroyDuringVitals = bDestroy;
				if (!BindProbe(Test, Nested, TEXT("OnUnitDied"), NestedProbe.Get(), TEXT("HandleDeath")) ||
					!BindProbe(Test, Nested, TEXT("OnUnitVitalsChanged"), NestedProbe.Get(), TEXT("HandleVitals"))) return true;
				const float BeforeStamina = Stamina(Test, Nested);
				Nested->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Nested->GetCharacterMovement()->Activate();
				Dodge(Test, Nested);
				Test.TestTrue(TEXT("Vitals observer performed nested termination"), NestedProbe->bHandledVitalsAction);
				const FUnitVitalState AfterNested = StoredVitals(Test, Nested);
				Test.TestEqual(TEXT("Dodge spends once before observer termination"), AfterNested.GetStamina(), BeforeStamina - 20.0f);
				Test.TestEqual(TEXT("Nested callback commits expected terminal state"), AfterNested.GetLifeState(), bDestroy ? EUnitLifeState::Ended : EUnitLifeState::Dead);
				Test.TestEqual(TEXT("Destroy is not damage and kill notifies once"), NestedProbe->DeathEvents, bDestroy ? 0 : 1);
				Test.TestEqual(TEXT("No vitals notification after EndPlay"), NestedProbe->VitalsEvents, bDestroy ? 1 : 2);
				CheckQuiescent(Test, Nested);
				Test.TestTrue(TEXT("Dodge cannot queue launch after observer termination"), Nested->GetCharacterMovement()->PendingLaunchVelocity.IsNearlyZero());
				if (bDestroy) Test.TestEqual(TEXT("Nested destruction leaves no timer"), Nested->GetLifeSpan(), 0.0f);
				NestedProbes.Add(MoveTemp(NestedProbe));
			}
			DeathTime = World->GetTimeSeconds();
			Stage = 1;
			return false;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		float DeathTime = 0.0f;
		int32 Stage = 0;
		TWeakObjectPtr<UWorld> OwnedWorld;
		TWeakObjectPtr<AActor> Low, High, Invalid;
		TStrongObjectPtr<UUnitLifecycleProbe> Probe;
		TArray<TStrongObjectPtr<UUnitLifecycleProbe>> NestedProbes;
	};
	void ObjectCommand(FAutomationTestBase& Test, UObject* Target, FName Function, FName Param, UObject* Value)
	{
		FCall Call(Test, Target, Function, 1);
		if (FObjectPropertyBase* Prop = Call.Property<FObjectPropertyBase>(Param))
			Prop->SetObjectPropertyValue_InContainer(Call.Params.GetStructMemory(), Value);
		Call.Execute();
	}
	UObject* ObjectQuery(FAutomationTestBase& Test, UObject* Target, FName Function)
	{
		FCall Call(Test, Target, Function, 1);
		FObjectPropertyBase* Prop = Call.Property<FObjectPropertyBase>(TEXT("ReturnValue"), true);
		Call.Execute();
		return Prop ? Prop->GetObjectPropertyValue_InContainer(Call.Params.GetStructMemory()) : nullptr;
	}
	void SelectMany(FAutomationTestBase& Test, UObject* Controller, const TArray<AActor*>& Units)
	{
		FCall Call(Test, Controller, TEXT("SelectUnits"), 1);
		FArrayProperty* Prop = Call.Property<FArrayProperty>(TEXT("Units"));
		FObjectPropertyBase* Inner = Prop ? CastField<FObjectPropertyBase>(Prop->Inner) : nullptr;
		if (!Test.TestNotNull(TEXT("Selection takes an object array"), Inner)) return;
		FScriptArrayHelper Array(Prop, Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()));
		for (AActor* Unit : Units) Inner->SetObjectPropertyValue(Array.GetRawPtr(Array.AddValue()), Unit);
		Call.Execute();
	}
	TArray<AActor*> SelectedActors(FAutomationTestBase& Test, UObject* Controller)
	{
		FCall Call(Test, Controller, TEXT("GetSelectedUnits"), 1);
		FArrayProperty* Prop = Call.Property<FArrayProperty>(TEXT("ReturnValue"), true);
		FObjectPropertyBase* Inner = Prop ? CastField<FObjectPropertyBase>(Prop->Inner) : nullptr;
		TArray<AActor*> Result;
		if (!Test.TestNotNull(TEXT("Selection query returns an object array"), Inner)) return Result;
		Call.Execute();
		FScriptArrayHelper Array(Prop, Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()));
		for (int32 Index = 0; Index < Array.Num(); ++Index) Result.Add(Cast<AActor>(Inner->GetObjectPropertyValue(Array.GetRawPtr(Index))));
		return Result;
	}
	bool Eligible(FAutomationTestBase& Test, UObject* Controller, AActor* Unit)
	{
		FCall Call(Test, Controller, TEXT("IsPlayerUnitEligible"), 2);
		if (FObjectPropertyBase* Prop = Call.Property<FObjectPropertyBase>(TEXT("Unit")))
			Prop->SetObjectPropertyValue_InContainer(Call.Params.GetStructMemory(), Unit);
		Call.Execute();
		return Call.Read<FBoolProperty, bool>();
	}
	bool HasListener(FAutomationTestBase& Test, UObject* Source, FName Event, UObject* Listener, FName Handler)
	{
		FMulticastDelegateProperty* Prop = FindFProperty<FMulticastDelegateProperty>(Source->GetClass(), Event);
		if (!Test.TestNotNull(TEXT("Ownership event exists"), Prop)) return false;
		const FMulticastScriptDelegate* Delegate = Prop->GetMulticastDelegate(Prop->ContainerPtrToValuePtr<void>(Source));
		return Delegate && Delegate->Contains(Listener, Handler);
	}
	void ApplyControllerProbe(UUnitLifecycleProbe* Probe)
	{
		if (!Probe->Test || Probe->bHandledControllerAction || !Probe->ObservedUnit.IsValid() || !Probe->ObservedController.IsValid()) return;
		Probe->bHandledControllerAction = true;
		switch (Probe->ControllerAction)
		{
		case EUnitControllerProbeAction::ChangeOwner: SetOwner(*Probe->Test, Probe->ObservedUnit.Get(), EFactionID::Carthage); break;
		case EUnitControllerProbeAction::Kill: Health(*Probe->Test, Probe->ObservedUnit.Get(), 0); break;
		case EUnitControllerProbeAction::Destroy: Probe->ObservedUnit->Destroy(); break;
		case EUnitControllerProbeAction::Exit: Command(*Probe->Test, Probe->ObservedController.Get(), TEXT("ExitFirstPersonMode")); break;
		case EUnitControllerProbeAction::SelectReplacement:
			ObjectCommand(*Probe->Test, Probe->ObservedController.Get(), TEXT("SelectUnit"), TEXT("Unit"), Probe->ReplacementUnit.Get()); break;
		}
	}
	class FExerciseOwnership : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseOwnership(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Ownership PIE startup timed out")); return true; }
			ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			FNavLocation MoveGoal;
			if (!Nav || Nav->IsNavigationBuildInProgress() ||
				!Nav->ProjectPointToNavigation(FVector(-1200, -3500, 0), MoveGoal, FVector(100, 100, 250))) return false;
			APlayerController* Controller = World->GetFirstPlayerController();
			if (!Controller || !Bool(Test, Controller, TEXT("CanIssueTacticalCommands"))) return false;
			APawn* OriginalCamera = Controller->GetPawn();
			UUserWidget* Root = Cast<UUserWidget>(ObjectQuery(Test, Controller, TEXT("GetMainWidget")));
			UClass* UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestTrue(TEXT("Ownership fixture has real controller, camera, UMG and unit class"), OriginalCamera && Root && UnitClass)) return true;
			int32 SpawnIndex = 0;
			auto Spawn = [&](EFactionID Faction, bool bDeferred = false, UWorld* SpawnWorld = nullptr)
			{
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.ObjectFlags |= RF_Transient;
				ACharacter* Unit = (SpawnWorld ? SpawnWorld : World)->SpawnActor<ACharacter>(UnitClass,
					FTransform(FVector(150000.0 + 2000.0 * SpawnIndex++, 100000.0, 300.0)), Params);
				if (!Test.TestNotNull(TEXT("Transient ownership unit spawned"), Unit)) return Unit;
				Test.TestTrue(TEXT("Ownership fixture initialization succeeded"), Initialize(Test, Unit, TEXT("Legionary"), Faction));
				if (!bDeferred) Unit->FinishSpawning(Unit->GetActorTransform());
				Unit->SetActorTickEnabled(false);
				for (UActorComponent* Component : Unit->GetComponents()) if (Component) Component->SetComponentTickEnabled(false);
				return Unit;
			};
			ACharacter* Owned = Spawn(EFactionID::Rome);
			ACharacter* Other = Spawn(EFactionID::Rome);
			ACharacter* Enemy = Spawn(EFactionID::Carthage);
			ACharacter* Dead = Spawn(EFactionID::Rome);
			ACharacter* Deferred = Spawn(EFactionID::Rome, true);
			if (!Owned || !Other || !Enemy || !Dead || !Deferred) return true;
			Health(Test, Dead, 0);
			Test.TestTrue(TEXT("Live owned unit eligible"), Eligible(Test, Controller, Owned));
			if (!Test.TestFalse(TEXT("Enemy unit ineligible"), Eligible(Test, Controller, Enemy))) return true;
			Test.TestFalse(TEXT("Null unit ineligible"), Eligible(Test, Controller, nullptr));
			Test.TestFalse(TEXT("Dead unit ineligible"), Eligible(Test, Controller, Dead));
			Test.TestFalse(TEXT("Initialized deferred unit ineligible before BeginPlay"), Eligible(Test, Controller, Deferred));
			ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Deferred);
			Test.TestTrue(TEXT("Deferred possession rejected"), Controller->GetPawn() == OriginalCamera);
			ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Enemy);
			Test.TestTrue(TEXT("Enemy possession rejected"), Controller->GetPawn() == OriginalCamera);

			UWorld* ForeignWorld = UWorld::CreateWorld(EWorldType::Game, false);
			if (!Test.TestNotNull(TEXT("Foreign world created"), ForeignWorld)) return true;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(ForeignWorld);
			ON_SCOPE_EXIT
			{
				ForeignWorld->DestroyWorld(false);
				GEngine->DestroyWorldContext(ForeignWorld);
				// Enhanced Input enumerates world subsystems through TObjectIterator.
				// Collect the deinitialized auxiliary subsystem before Slate next dispatches input.
				CollectGarbage(RF_NoFlags);
			};
			ForeignWorld->SetGameInstance(World->GetGameInstance());
			ForeignWorld->InitializeActorsForPlay(FURL());
			ACharacter* Foreign = Spawn(EFactionID::Rome, false, ForeignWorld);
			if (!Foreign) return true;
			Foreign->DispatchBeginPlay();
			Test.TestTrue(TEXT("Foreign unit is otherwise command-ready"), Bool(Test, Foreign, TEXT("CanAcceptCommands")));
			Test.TestFalse(TEXT("Foreign-world unit ineligible"), Eligible(Test, Controller, Foreign));
			SelectMany(Test, Controller, { Owned, Enemy, nullptr, Other, Owned, Dead, Deferred, Foreign });
			Test.TestTrue(TEXT("Mixed selection retains only unique eligible owned units"), SelectedActors(Test, Controller) == TArray<AActor*>({ Owned, Other }));
			Test.TestTrue(TEXT("Owned selection ring active"), Bool(Test, Owned, TEXT("IsSelected")));
			Test.TestFalse(TEXT("Enemy selection ring inactive"), Bool(Test, Enemy, TEXT("IsSelected")));
			auto PlayerMove = [&]()
			{
				FCall Call(Test, Controller, TEXT("CommandSelectedUnitsMove"), 1);
				FStructProperty* Prop = Call.Property<FStructProperty>(TEXT("Destination"));
				if (!Test.TestTrue(TEXT("Player move accepts FVector"), Prop && Prop->Struct == TBaseStructure<FVector>::Get())) return;
				const FVector Destination = MoveGoal.Location; // Multi-selection needs a navigable formation footprint.
				Prop->CopyCompleteValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Destination);
				Call.Execute();
			};
			PlayerMove();
			Test.TestEqual(TEXT("Player move reaches owned selected unit"), Order(Test, Owned), EUnitOrderType::Move);
			Test.TestEqual(TEXT("Player move does not reach enemy"), Order(Test, Enemy), EUnitOrderType::None);
			ObjectCommand(Test, Controller, TEXT("CommandSelectedUnitsAttack"), TEXT("Target"), Enemy);
			Test.TestEqual(TEXT("Player attack orders reach owned selection"), Order(Test, Owned), EUnitOrderType::Attack);
			Test.TestEqual(TEXT("Rejected enemy receives no player order"), Order(Test, Enemy), EUnitOrderType::None);
			Attack(Test, Enemy, Owned);
			Test.TestEqual(TEXT("Enemy AI actor command remains legal"), Order(Test, Enemy), EUnitOrderType::Attack);
			SetOwner(Test, Owned, EFactionID::Carthage);
			Test.TestTrue(TEXT("Ownership change synchronously prunes selection"), SelectedActors(Test, Controller) == TArray<AActor*>({ Other }));
			Test.TestFalse(TEXT("Ownership change clears the former ring"), Bool(Test, Owned, TEXT("IsSelected")));
			Command(Test, Owned, TEXT("CommandHold"));
			PlayerMove();
			Test.TestEqual(TEXT("Player move cannot command former selection"), Order(Test, Owned), EUnitOrderType::Hold);
			ObjectCommand(Test, Controller, TEXT("CommandSelectedUnitsAttack"), TEXT("Target"), Enemy);
			Test.TestEqual(TEXT("Former selection receives no subsequent player order"), Order(Test, Owned), EUnitOrderType::Hold);
			SetOwner(Test, Owned, EFactionID::None);
			Test.TestFalse(TEXT("Unowned unit is ineligible"), Eligible(Test, Controller, Owned));
			SetOwner(Test, Owned, EFactionID::Rome);
			ObjectCommand(Test, Controller, TEXT("SelectUnit"), TEXT("Unit"), Owned);
			ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Owned);
			Test.TestTrue(TEXT("Owned possession transfers actual Pawn"), Controller->GetPawn() == Owned);
			PlayerMove();
			Test.TestEqual(TEXT("Tactical order facade rejects possession mode"), Order(Test, Owned), EUnitOrderType::None);
			UTextBlock* PossessionStatus = Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("PossessionText")));
			if (!Test.TestNotNull(TEXT("Possession status text exists"), PossessionStatus)) return true;
			Test.TestTrue(TEXT("Possession HUD initially shows lowered guard"),
				PossessionStatus->GetText().ToString().Contains(TEXT("Guard DOWN")));
			Command(Test, Owned, TEXT("StartBlocking"));
			Test.TestTrue(TEXT("Shield block activates on the controlled soldier"), Bool(Test, Owned, TEXT("IsBlocking")));
			Test.TestTrue(TEXT("Guard activation updates the possession HUD synchronously"),
				PossessionStatus->GetText().ToString().Contains(TEXT("Guard UP")));
			Command(Test, Owned, TEXT("StopBlocking"));
			Test.TestTrue(TEXT("Guard release updates the possession HUD synchronously"),
				PossessionStatus->GetText().ToString().Contains(TEXT("Guard DOWN")));
			Command(Test, Owned, TEXT("StartBlocking"));
			SetOwner(Test, Owned, EFactionID::Carthage);
			CheckReturned(Controller, OriginalCamera, Root);
			Test.TestFalse(TEXT("Ownership loss clears unit blocking"), Bool(Test, Owned, TEXT("IsBlocking")));
			Test.TestFalse(TEXT("Ownership loss clears unit possession"), Bool(Test, Owned, TEXT("IsPossessedByPlayer")));
			Test.TestFalse(TEXT("Ownership loss unbinds selection observer"), HasListener(Test, Owned, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandleSelectedUnitOwnerChanged")));
			Test.TestFalse(TEXT("Ownership loss unbinds possession observer"), HasListener(Test, Owned, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandlePossessedUnitOwnerChanged")));

			UObject* Factions = ObjectQuery(Test, World->GetAuthGameMode(), TEXT("GetFactionManager"));
			auto SetPlayerFaction = [&](EFactionID Faction)
			{
				FCall Call(Test, Factions, TEXT("SetPlayerFaction"), 1);
				if (FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("NewFaction")))
					Prop->GetUnderlyingProperty()->SetIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), static_cast<int64>(Faction));
				Call.Execute();
			};
			ObjectCommand(Test, Controller, TEXT("SelectUnit"), TEXT("Unit"), Other);
			ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Other);
			SetPlayerFaction(EFactionID::Carthage);
			CheckReturned(Controller, OriginalCamera, Root);
			Test.TestTrue(TEXT("Eligibility follows canonical player faction"), Eligible(Test, Controller, Enemy));
			SetPlayerFaction(EFactionID::Rome);

			for (EUnitControllerProbeAction Action : { EUnitControllerProbeAction::ChangeOwner, EUnitControllerProbeAction::Kill,
				EUnitControllerProbeAction::Destroy, EUnitControllerProbeAction::Exit })
			{
				ACharacter* Unit = Spawn(EFactionID::Rome);
				if (!Unit) return true;
				TStrongObjectPtr<UUnitLifecycleProbe> Probe(NewObject<UUnitLifecycleProbe>());
				Probe->Test = &Test; Probe->ObservedUnit = Unit; Probe->ObservedController = Controller; Probe->ControllerAction = Action;
				UCameraComponent* Camera = Cast<UCameraComponent>(Component(Test, Unit, TEXT("FPSCamera")));
				if (!Test.TestNotNull(TEXT("Real unit camera"), Camera)) return true;
				Camera->OnComponentActivated.AddDynamic(Probe.Get(), &UUnitLifecycleProbe::HandleControllerCameraActivated);
				ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Unit);
				Test.TestTrue(TEXT("Actual camera callback interrupted controller entry"), Probe->bHandledControllerAction);
				CheckReturned(Controller, OriginalCamera, Root);
				Test.TestFalse(TEXT("Interrupted entry leaves no stored possession flag"), StoredBool(Test, Unit, TEXT("bIsPossessedByPlayer")));
			}
			TStrongObjectPtr<UUnitLifecycleProbe> ExitProbe(NewObject<UUnitLifecycleProbe>());
			ExitProbe->Test = &Test; ExitProbe->ObservedUnit = Other; ExitProbe->ObservedController = Controller;
			UCameraComponent* ExitCamera = Cast<UCameraComponent>(Component(Test, Other, TEXT("FPSCamera")));
			if (!ExitCamera) return true;
			ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Other);
			ExitCamera->OnComponentDeactivated.AddDynamic(ExitProbe.Get(), &UUnitLifecycleProbe::HandleControllerCameraDeactivated);
			Command(Test, Controller, TEXT("ExitFirstPersonMode"));
			Test.TestTrue(TEXT("Real deactivation attempted recursive exit and entry"), ExitProbe->bHandledControllerAction);
			CheckReturned(Controller, OriginalCamera, Root);
			Command(Test, Controller, TEXT("ExitFirstPersonMode"));
			ObjectCommand(Test, Controller, TEXT("SelectUnit"), TEXT("Unit"), Other);
			const TArray<AActor*> SelectionBeforeCycles = SelectedActors(Test, Controller);
			Test.TestTrue(TEXT("Repeated possession starts from an owned selection"),
				SelectionBeforeCycles == TArray<AActor*>({ Other }));
			const FTransform CameraBeforeCycles = OriginalCamera->GetActorTransform();
			APlayerCameraManager* CameraManager = Controller->PlayerCameraManager;
			if (!Test.TestNotNull(TEXT("Repeated possession has the live camera manager"), CameraManager)) return true;
			const float TacticalPitchMin = CameraManager->ViewPitchMin;
			const float TacticalPitchMax = CameraManager->ViewPitchMax;
			AAIController* OriginalAI = Cast<AAIController>(Other->GetController());
			Test.TestNotNull(TEXT("Repeated possession starts with tactical AI"), OriginalAI);
			for (int32 Cycle = 0; Cycle < 3; ++Cycle)
			{
				ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Other);
				Test.TestTrue(TEXT("Repeated entry possesses the same living unit"), Controller->GetPawn() == Other);
				Test.TestEqual(TEXT("Possession limits downward view pitch"), CameraManager->ViewPitchMin, -80.0f);
				Test.TestEqual(TEXT("Possession limits upward view pitch"), CameraManager->ViewPitchMax, 80.0f);
				Command(Test, Controller, TEXT("ExitFirstPersonMode"));
				Test.TestEqual(TEXT("Repeated exit restores tactical minimum pitch"), CameraManager->ViewPitchMin, TacticalPitchMin);
				Test.TestEqual(TEXT("Repeated exit restores tactical maximum pitch"), CameraManager->ViewPitchMax, TacticalPitchMax);
				Test.TestTrue(TEXT("Repeated exit restores the tactical pawn"), Controller->GetPawn() == OriginalCamera);
				Test.TestFalse(TEXT("Repeated exit leaves possession mode"), Bool(Test, Controller, TEXT("IsInFirstPersonMode")));
				Test.TestTrue(TEXT("Repeated exit preserves the tactical camera transform"),
					OriginalCamera->GetActorTransform().Equals(CameraBeforeCycles));
				Test.TestTrue(TEXT("Repeated exit preserves the selected owned unit"),
					SelectedActors(Test, Controller) == SelectionBeforeCycles);
				Test.TestTrue(TEXT("Repeated exit restores the same tactical AI controller"),
					OriginalAI && Other->GetController() == OriginalAI);
			}
			Command(Test, Controller, TEXT("ClearSelection"));

			SetOwner(Test, Owned, EFactionID::Rome);
			TStrongObjectPtr<UUnitLifecycleProbe> SelectionProbe(NewObject<UUnitLifecycleProbe>());
			SelectionProbe->Test = &Test; SelectionProbe->ObservedUnit = Owned; SelectionProbe->ObservedController = Controller;
			SelectionProbe->ReplacementUnit = Other; SelectionProbe->ControllerAction = EUnitControllerProbeAction::SelectReplacement;
			if (!BindProbe(Test, Controller, TEXT("OnSelectionChanged"), SelectionProbe.Get(), TEXT("HandleControllerSelection"))) return true;
			// Register the real UMG observer after the mutating observer. Otherwise
			// it can run before the nested change and a stale-payload regression is masked.
			FMulticastDelegateProperty* SelectionEvent = FindFProperty<FMulticastDelegateProperty>(Controller->GetClass(), TEXT("OnSelectionChanged"));
			UFunction* UIHandler = Root->FindFunction(TEXT("HandleSelectionChanged"));
			if (!Test.TestTrue(TEXT("Production selection observer has the real event signature"), SelectionEvent && UIHandler &&
				SelectionEvent->SignatureFunction->IsSignatureCompatibleWith(UIHandler))) return true;
			FScriptDelegate UIDelegate;
			UIDelegate.BindUFunction(Root, TEXT("HandleSelectionChanged"));
			SelectionEvent->RemoveDelegate(UIDelegate, Controller);
			SelectionEvent->AddDelegate(UIDelegate, Controller);
			ObjectCommand(Test, Controller, TEXT("SelectUnit"), TEXT("Unit"), Owned);
			Test.TestTrue(TEXT("Selection observer committed its replacement"), SelectedActors(Test, Controller) == TArray<AActor*>({ Other }));
			FObjectPropertyBase* Observed = FindFProperty<FObjectPropertyBase>(Root->GetClass(), TEXT("ObservedUnit"));
			Test.TestTrue(TEXT("UMG observes current selection, not stale outer payload"), Observed && Observed->GetObjectPropertyValue_InContainer(Root) == Other);
			SelectionProbe->ControllerAction = EUnitControllerProbeAction::ChangeOwner; SelectionProbe->bHandledControllerAction = false;
			ObjectCommand(Test, Controller, TEXT("SelectUnit"), TEXT("Unit"), Owned);
			Test.TestTrue(TEXT("Owner change inside selection notification removes the unit"), SelectedActors(Test, Controller).IsEmpty());
			Test.TestFalse(TEXT("Nested ownership change leaves no ring"), Bool(Test, Owned, TEXT("IsSelected")));
			Command(Test, Controller, TEXT("ClearSelection"));
			Deferred->Destroy();
			ObjectCommand(Test, Controller, TEXT("SelectUnit"), TEXT("Unit"), Other);
			ObjectCommand(Test, Controller, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Other);
			Test.TestTrue(TEXT("Live controller observes selected ownership"), HasListener(Test, Other, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandleSelectedUnitOwnerChanged")));
			Test.TestTrue(TEXT("Live controller observes possessed ownership"), HasListener(Test, Other, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandlePossessedUnitOwnerChanged")));
			Test.TestTrue(TEXT("Live controller observes player faction"), HasListener(Test, State, TEXT("OnPlayerFactionChanged"), Controller, TEXT("HandlePlayerFactionChanged")));
			Controller->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			Test.TestFalse(TEXT("Controller teardown releases unit possession"), Bool(Test, Other, TEXT("IsPossessedByPlayer")));
			Test.TestFalse(TEXT("Controller teardown releases selected ownership observer"), HasListener(Test, Other, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandleSelectedUnitOwnerChanged")));
			Test.TestFalse(TEXT("Controller teardown releases possessed ownership observer"), HasListener(Test, Other, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandlePossessedUnitOwnerChanged")));
			Test.TestFalse(TEXT("Controller teardown releases public faction observer"), HasListener(Test, State, TEXT("OnPlayerFactionChanged"), Controller, TEXT("HandlePlayerFactionChanged")));
			return true;
		}
	private:
		void CheckReturned(APlayerController* Controller, APawn* Camera, UUserWidget* Root)
		{
			Test.TestTrue(TEXT("Original actual camera restored"), Controller->GetPawn() == Camera);
			Test.TestFalse(TEXT("Controller no longer in possession mode"), Bool(Test, Controller, TEXT("IsInFirstPersonMode")));
			Test.TestTrue(TEXT("Canonical selection is empty"), SelectedActors(Test, Controller).IsEmpty());
			Test.TestTrue(TEXT("Tactical command eligibility restored"), Bool(Test, Controller, TEXT("CanIssueTacticalCommands")));
			Test.TestEqual(TEXT("Crosshair collapsed after ownership cleanup"), Root->GetWidgetFromName(TEXT("CrosshairText"))->GetVisibility(), ESlateVisibility::Collapsed);
			Test.TestEqual(TEXT("Possession HUD collapsed after ownership cleanup"), Root->GetWidgetFromName(TEXT("PossessionText"))->GetVisibility(), ESlateVisibility::Collapsed);
		}
		FAutomationTestBase& Test;
		double StartedAt;
	};
	void ModifySelection(FAutomationTestBase& Test, UObject* Controller, const TArray<AActor*>& Units, EUnitSelectionOperation Operation)
	{
		FCall Call(Test, Controller, TEXT("ModifyUnitSelection"), 2);
		FArrayProperty* Prop = Call.Property<FArrayProperty>(TEXT("Units"));
		FObjectPropertyBase* Inner = Prop ? CastField<FObjectPropertyBase>(Prop->Inner) : nullptr;
		FEnumProperty* Enum = Call.Property<FEnumProperty>(TEXT("Operation"));
		if (!Test.TestTrue(TEXT("Modifier command has unit array and operation enum"), Inner && Enum)) return;
		FScriptArrayHelper Array(Prop, Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()));
		for (AActor* Unit : Units) Inner->SetObjectPropertyValue(Array.GetRawPtr(Array.AddValue()), Unit);
		Enum->GetUnderlyingProperty()->SetIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), static_cast<int64>(Operation));
		Call.Execute();
	}
	void ClickSelection(FAutomationTestBase& Test, UObject* Controller, AActor* Actor, bool bAdditive = false)
	{
		FCall Call(Test, Controller, TEXT("SelectActorFromInput"), 2);
		if (FObjectPropertyBase* Prop = Call.Property<FObjectPropertyBase>(TEXT("Actor")))
			Prop->SetObjectPropertyValue_InContainer(Call.Params.GetStructMemory(), Actor);
		Call.Set<FBoolProperty>(TEXT("bAdditive"), bAdditive);
		Call.Execute();
	}
	class FExerciseAdditiveSelection : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseAdditiveSelection(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 40.0) { Test.AddError(TEXT("Additive selection PIE timed out")); return true; }
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			APlayerController* Controller = World->GetFirstPlayerController();
			if (!Controller || !Bool(Test, Controller, TEXT("CanIssueTacticalCommands"))) return false;
			UUserWidget* Root = Cast<UUserWidget>(ObjectQuery(Test, Controller, TEXT("GetMainWidget")));
			UTextBlock* Feedback = Root ? Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("InteractionFeedbackText"))) : nullptr;
			if (!Test.TestTrue(TEXT("Additive fixture has production UMG feedback"), Root && Feedback)) return true;
			if (bWaitingForFeedback)
			{
				if (World->GetTimeSeconds() - FeedbackStarted < 3.2f) return false;
				Test.TestTrue(TEXT("Feedback expires through real world timer"), Feedback->GetText().IsEmpty());
				Test.TestEqual(TEXT("Expired feedback collapses"), Feedback->GetVisibility(), ESlateVisibility::Collapsed);
				ClickSelection(Test, Controller, Enemy.Get());
				Root->RemoveFromParent();
				Test.TestFalse(TEXT("Widget destruction unbinds feedback observer"), HasListener(Test, Controller,
					TEXT("OnInteractionFeedback"), Root, TEXT("HandleInteractionFeedback")));
				Test.TestTrue(TEXT("Widget destruction clears pending feedback"), Feedback->GetText().IsEmpty());
				ClickSelection(Test, Controller, Enemy.Get());
				Test.TestTrue(TEXT("Detached widget receives no later feedback"), Feedback->GetText().IsEmpty());
				return true;
			}
			UClass* UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestNotNull(TEXT("Additive unit class exists"), UnitClass)) return true;
			int32 Index = 0;
			auto Spawn = [&](EFactionID Owner)
			{
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.ObjectFlags |= RF_Transient;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ACharacter* Unit = World->SpawnActor<ACharacter>(UnitClass, FTransform(FVector(190000 + 2000 * Index++, 170000, 300)), Params);
				if (!Test.TestNotNull(TEXT("Additive transient unit spawned"), Unit)) return Unit;
				Test.TestTrue(TEXT("Additive fixture initialized"), Initialize(Test, Unit, TEXT("Legionary"), Owner));
				Unit->FinishSpawning(Unit->GetActorTransform());
				Unit->SetActorTickEnabled(false);
				for (UActorComponent* Part : Unit->GetComponents()) if (Part) Part->SetComponentTickEnabled(false);
				return Unit;
			};
			ACharacter* First = Spawn(EFactionID::Rome);
			ACharacter* Second = Spawn(EFactionID::Rome);
			Enemy = Spawn(EFactionID::Carthage);
			if (!First || !Second || !Enemy.IsValid()) return true;
			ModifySelection(Test, Controller, { First, First, Enemy.Get(), nullptr }, EUnitSelectionOperation::Replace);
			Test.TestTrue(TEXT("Replace filters and deduplicates"), SelectedActors(Test, Controller) == TArray<AActor*>({ First }));
			ModifySelection(Test, Controller, { First, Second, Second }, EUnitSelectionOperation::Add);
			Test.TestTrue(TEXT("Add appends eligible unique units"), SelectedActors(Test, Controller) == TArray<AActor*>({ First, Second }));
			ModifySelection(Test, Controller, { First, First }, EUnitSelectionOperation::Toggle);
			Test.TestTrue(TEXT("Duplicate toggle removes once"), SelectedActors(Test, Controller) == TArray<AActor*>({ Second }));
			Test.TestFalse(TEXT("Toggle removal clears presentation"), Bool(Test, First, TEXT("IsSelected")));
			Test.TestFalse(TEXT("Toggle removal releases ownership observer"), HasListener(Test, First, TEXT("OnOwnerFactionChanged"), Controller, TEXT("HandleSelectedUnitOwnerChanged")));
			for (EUnitSelectionOperation Operation : { EUnitSelectionOperation::Add, EUnitSelectionOperation::Toggle })
			{
				ModifySelection(Test, Controller, { Enemy.Get(), nullptr }, Operation);
				ModifySelection(Test, Controller, {}, Operation);
				Test.TestTrue(TEXT("No eligible modifier input preserves selection"), SelectedActors(Test, Controller) == TArray<AActor*>({ Second }));
			}
			ClickSelection(Test, Controller, First, true);
			Test.TestTrue(TEXT("Additive click toggles unselected unit on"), SelectedActors(Test, Controller) == TArray<AActor*>({ Second, First }));
			ClickSelection(Test, Controller, nullptr, true);
			ClickSelection(Test, Controller, Controller->GetPawn(), true);
			Test.TestEqual(TEXT("Additive empty and terrain clicks preserve selection"), SelectedActors(Test, Controller).Num(), 2);
			ClickSelection(Test, Controller, Enemy.Get());
			Test.TestEqual(TEXT("Rejected enemy click preserves selection"), SelectedActors(Test, Controller).Num(), 2);
			Test.TestFalse(TEXT("Rejected click produces visible message"), Feedback->GetText().IsEmpty());
			Test.TestTrue(TEXT("Feedback is drawn"), Feedback->IsVisible());
			ClickSelection(Test, Controller, First);
			Test.TestTrue(TEXT("Ordinary unit click replaces selection"), SelectedActors(Test, Controller) == TArray<AActor*>({ First }));
			Test.TestTrue(TEXT("Valid selection clears rejection feedback"), Feedback->GetText().IsEmpty());
			ClickSelection(Test, Controller, First, true);
			Test.TestTrue(TEXT("Additive click toggles selected unit off"), SelectedActors(Test, Controller).IsEmpty());
			ClickSelection(Test, Controller, First);
			ClickSelection(Test, Controller, nullptr);
			Test.TestTrue(TEXT("Plain empty click clears selection"), SelectedActors(Test, Controller).IsEmpty());
			ClickSelection(Test, Controller, First);
			auto SetMode = [&](EGameplayMode Mode)
			{
				FCall Call(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
				if (FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("NewMode")))
					Prop->GetUnderlyingProperty()->SetIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), static_cast<int64>(Mode));
				Call.Execute();
			};
			SetMode(EGameplayMode::Strategic);
			const TArray<AActor*> StrategicSelection = SelectedActors(Test, Controller);
			ModifySelection(Test, Controller, { Second }, EUnitSelectionOperation::Add);
			ClickSelection(Test, Controller, Second, true);
			Test.TestTrue(TEXT("Selection commands reject strategic mode"), SelectedActors(Test, Controller) == StrategicSelection);
			SetMode(EGameplayMode::Tactical);
			ClickSelection(Test, Controller, First);
			ModifySelection(Test, Controller, { Second }, EUnitSelectionOperation::Add);
			SetOwner(Test, Second, EFactionID::Carthage);
			Test.TestTrue(TEXT("Ownership cleanup prunes additive member"), SelectedActors(Test, Controller) == TArray<AActor*>({ First }));
			Test.TestFalse(TEXT("Ownership cleanup clears added member ring"), Bool(Test, Second, TEXT("IsSelected")));
			ModifySelection(Test, Controller, {}, EUnitSelectionOperation::Replace);
			Test.TestTrue(TEXT("Empty replace clears"), SelectedActors(Test, Controller).IsEmpty());
			TStrongObjectPtr<UUnitLifecycleProbe> Probe(NewObject<UUnitLifecycleProbe>());
			Probe->Test = &Test; Probe->ObservedController = Controller; Probe->ObservedUnit = First;
			Probe->ControllerAction = EUnitControllerProbeAction::ChangeOwner;
			if (!BindProbe(Test, Controller, TEXT("OnSelectionChanged"), Probe.Get(), TEXT("HandleControllerSelection"))) return true;
			ModifySelection(Test, Controller, { First }, EUnitSelectionOperation::Add);
			Test.TestTrue(TEXT("Add notification tolerates nested ownership loss"), Probe->bHandledControllerAction && SelectedActors(Test, Controller).IsEmpty());
			Test.TestFalse(TEXT("Nested ownership loss leaves no selected ring"), Bool(Test, First, TEXT("IsSelected")));
			ClickSelection(Test, Controller, Enemy.Get(), true);
			FeedbackStarted = World->GetTimeSeconds();
			bWaitingForFeedback = true;
			return false;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		float FeedbackStarted = 0;
		bool bWaitingForFeedback = false;
		TWeakObjectPtr<ACharacter> Enemy;
	};
	bool SnapshotContains(const TArray<AUnitBase*>& Snapshot, const AActor* Actor)
	{
		return Snapshot.ContainsByPredicate([Actor](const AUnitBase* Unit)
		{
			return static_cast<const void*>(Unit) == static_cast<const void*>(Actor);
		});
	}
	// NullRHI automation may have no sized viewport. Supply dimensions only;
	// projection still uses the real local player, camera and engine matrix code.
	class FRegistryProjectionViewport final : public FSceneViewport
	{
	public:
		FRegistryProjectionViewport() : FSceneViewport(TSharedPtr<SViewport>()) {}
		virtual FIntPoint GetSizeXY() const override { return FIntPoint(1920, 1080); }
	};
	class FExerciseUnitRegistry : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseUnitRegistry(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Registry PIE startup timed out")); return true; }
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			URomanUnitRegistrySubsystem* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>();
			UClass* UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestTrue(TEXT("Real PIE live registry and unit class exist"), Registry && UnitClass)) return true;
			Test.TestNull(TEXT("Editor world has no live gameplay registry"), GEditor->GetEditorWorldContext().World()->GetSubsystem<URomanUnitRegistrySubsystem>());
			const int32 Baseline = Registry->GetRegisteredUnitCount();
			Test.TestTrue(TEXT("Bootstrap units register without a world scan"), Baseline > 0);
			Test.TestFalse(TEXT("Registry rejects null membership"), Registry->ContainsUnit(nullptr));
			Test.TestFalse(TEXT("Registry rejects non-unit actors"), Registry->ContainsUnit(World->GetFirstPlayerController()));
			const FVector Origin(250000.0, 200000.0, 300.0);
			auto Spawn = [&](EFactionID Faction, double Offset, bool bFinish = true, UWorld* SpawnWorld = nullptr)
			{
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true;
				Params.ObjectFlags |= RF_Transient;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ACharacter* Unit = (SpawnWorld ? SpawnWorld : World)->SpawnActor<ACharacter>(UnitClass, FTransform(Origin + FVector(Offset, 0, 0)), Params);
				if (!Test.TestNotNull(TEXT("Registry transient fixture spawned"), Unit)) return Unit;
				Test.TestTrue(TEXT("Registry fixture initialization succeeded"), Initialize(Test, Unit, TEXT("Legionary"), Faction));
				if (bFinish) Unit->FinishSpawning(Unit->GetActorTransform());
				Unit->SetActorTickEnabled(false);
				for (UActorComponent* Part : Unit->GetComponents()) if (Part) Part->SetComponentTickEnabled(false);
				return Unit;
			};
			ACharacter* Deferred = Spawn(EFactionID::Rome, 3000, false);
			if (!Deferred) return true;
			Test.TestFalse(TEXT("Initialized deferred actor not registered before BeginPlay"), Registry->ContainsUnit(Deferred));
			SetOwner(Test, Deferred, EFactionID::Carthage);
			Test.TestEqual(TEXT("Deferred ownership changes do not register early"), Registry->GetRegisteredUnitCount(), Baseline);
			Deferred->FinishSpawning(Deferred->GetActorTransform());
			Test.TestTrue(TEXT("BeginPlay registers initialized actor"), Registry->ContainsUnit(Deferred));
			Test.TestTrue(TEXT("Registration uses latest deferred faction"), SnapshotContains(Registry->GetUnitsForFaction(EFactionID::Carthage), Deferred));
			Test.TestEqual(TEXT("BeginPlay registers exactly once"), Registry->GetRegisteredUnitCount(), Baseline + 1);
			SetOwner(Test, Deferred, EFactionID::Carthage);
			Test.TestEqual(TEXT("Unchanged faction cannot duplicate a registration"), Registry->GetRegisteredUnitCount(), Baseline + 1);
			Deferred->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
			Test.TestEqual(TEXT("EndPlay removes a still-valid actor"), Registry->GetRegisteredUnitCount(), Baseline);
			Test.TestFalse(TEXT("Ended actor absent from registry"), Registry->ContainsUnit(Deferred));
			SetOwner(Test, Deferred, EFactionID::Rome);
			Deferred->Destroy();
			Test.TestEqual(TEXT("Repeated cleanup is idempotent"), Registry->GetRegisteredUnitCount(), Baseline);
			FActorSpawnParameters InvalidParams;
			InvalidParams.bDeferConstruction = true;
			InvalidParams.ObjectFlags |= RF_Transient;
			InvalidParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Invalid = World->SpawnActor<ACharacter>(UnitClass, FTransform(Origin + FVector(6000, 0, 0)), InvalidParams);
			if (!Test.TestNotNull(TEXT("Invalid definition fixture exists"), Invalid)) return true;
			Test.TestFalse(TEXT("Unknown definition rejected"), Initialize(Test, Invalid, TEXT("Missing_RegistryFixture"), EFactionID::Rome));
			Test.AddExpectedError(TEXT("has no valid content definition"), EAutomationExpectedErrorFlags::Contains, 1);
			Invalid->FinishSpawning(Invalid->GetActorTransform());
			Test.TestEqual(TEXT("Failed initialization never enters registry"), Registry->GetRegisteredUnitCount(), Baseline);
			Test.TestFalse(TEXT("Failed actor not a live member"), Registry->ContainsUnit(Invalid));
			Invalid->Destroy();

			ACharacter* Source = Spawn(EFactionID::Rome, 0);
			ACharacter* Near = Spawn(EFactionID::Carthage, 500);
			ACharacter* Far = Spawn(EFactionID::Carthage, 900);
			if (!Source || !Near || !Far) return true;
			const auto SameActor = [](const AUnitBase* Unit, const AActor* Actor) { return static_cast<const void*>(Unit) == static_cast<const void*>(Actor); };
			Test.TestTrue(TEXT("Nearest enemy uses faction index"), SameActor(Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000), Near));
			Test.TestNull(TEXT("Radius boundary stays strict"), Registry->FindNearestEnemy(Origin, EFactionID::Rome, 500));
			Test.TestNull(TEXT("Invalid radius rejected"), Registry->FindNearestEnemy(Origin, EFactionID::Rome, std::numeric_limits<double>::infinity()));
			Test.TestNull(TEXT("Negative radius rejected"), Registry->FindNearestEnemy(Origin, EFactionID::Rome, -1));
			Test.TestNull(TEXT("Non-finite origin rejected"), Registry->FindNearestEnemy(FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), EFactionID::Rome, 1000));
			// This isolated index fixture has no NavMesh. Acquire within melee range;
			// the navigation scenario separately exercises pursuit over real paths.
			Source->SetActorLocation(Near->GetActorLocation() - FVector(50, 0, 0));
			Source->Tick(0.01f);
			Test.TestTrue(TEXT("Actual idle Tick acquires the registry enemy"), ObjectQuery(Test, Source, TEXT("GetAttackTarget")) == Near);
			Source->SetActorLocation(Origin);
			Command(Test, Source, TEXT("CommandHold"));
			Source->Tick(0.01f);
			Test.TestNull(TEXT("Hold still prevents auto-acquisition"), ObjectQuery(Test, Source, TEXT("GetAttackTarget")));
			const TArray<AUnitBase*> OldSnapshot = Registry->GetUnitsForFaction(EFactionID::Carthage);
			SetOwner(Test, Near, EFactionID::Rome);
			Test.TestTrue(TEXT("Snapshot is independent of later index changes"), SnapshotContains(OldSnapshot, Near));
			Test.TestFalse(TEXT("Changed owner removed from old bucket"), SnapshotContains(Registry->GetUnitsForFaction(EFactionID::Carthage), Near));
			Test.TestTrue(TEXT("Changed owner appears in new bucket"), SnapshotContains(Registry->GetUnitsForFaction(EFactionID::Rome), Near));
			APlayerController* Controller = World->GetFirstPlayerController();
			ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
			UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient : nullptr;
			APawn* CameraPawn = Controller ? Controller->GetPawn() : nullptr;
			if (!Test.TestTrue(TEXT("Rectangle fixture has actual local player and camera"), ViewportClient && CameraPawn && Controller->PlayerCameraManager)) return true;
			FSceneViewport* OriginalViewport = ViewportClient->Viewport;
			const FIntPoint OriginalSize = OriginalViewport ? OriginalViewport->GetSizeXY() : FIntPoint::ZeroValue;
			Test.AddInfo(FString::Printf(TEXT("Original rectangle viewport: %d x %d"), OriginalSize.X, OriginalSize.Y));
			TUniquePtr<FRegistryProjectionViewport> ProjectionViewport;
			if (OriginalSize.X == 0 || OriginalSize.Y == 0)
			{
				ProjectionViewport = MakeUnique<FRegistryProjectionViewport>();
				ViewportClient->Viewport = ProjectionViewport.Get();
			}
			const FTransform OriginalCameraTransform = CameraPawn->GetActorTransform();
			USpringArmComponent* CameraArm = CameraPawn->FindComponentByClass<USpringArmComponent>();
			ON_SCOPE_EXIT
			{
				ViewportClient->Viewport = OriginalViewport;
				CameraPawn->SetActorTransform(OriginalCameraTransform);
				if (CameraArm) CameraArm->TickComponent(0.0f, LEVELTICK_All, nullptr);
				Controller->PlayerCameraManager->UpdateCamera(0.01f);
			};
			CameraPawn->SetActorLocation(FVector(Origin.X, Origin.Y, CameraPawn->GetActorLocation().Z));
			if (CameraArm) CameraArm->TickComponent(0.0f, LEVELTICK_All, nullptr);
			Controller->PlayerCameraManager->UpdateCamera(0.01f);
			FVector2D SourceScreen = FVector2D::ZeroVector, NearScreen = FVector2D::ZeroVector, FarScreen = FVector2D::ZeroVector;
			if (!Test.TestTrue(TEXT("Real controller projects rectangle fixture"), Controller &&
				Controller->ProjectWorldLocationToScreen(Source->GetActorLocation(), SourceScreen) &&
				Controller->ProjectWorldLocationToScreen(Near->GetActorLocation(), NearScreen) &&
				Controller->ProjectWorldLocationToScreen(Far->GetActorLocation(), FarScreen))) return true;
			const FVector2D RectStart(FMath::Min3(SourceScreen.X, NearScreen.X, FarScreen.X) - 2,
				FMath::Min3(SourceScreen.Y, NearScreen.Y, FarScreen.Y) - 2);
			const FVector2D RectEnd(FMath::Max3(SourceScreen.X, NearScreen.X, FarScreen.X) + 2,
				FMath::Max3(SourceScreen.Y, NearScreen.Y, FarScreen.Y) + 2);
			auto SelectRect = [&](const FVector2D& Start, const FVector2D& End, bool bAdditive = false)
			{
				FCall Call(Test, Controller, TEXT("SelectUnitsInScreenRect"), 3);
				Call.Set<FBoolProperty>(TEXT("bAdditive"), bAdditive);
				for (const auto& Pair : { TPair<FName, FVector2D>(TEXT("Start"), Start), TPair<FName, FVector2D>(TEXT("End"), End) })
				{
					FStructProperty* Prop = Call.Property<FStructProperty>(Pair.Key);
					if (!Test.TestTrue(TEXT("Rectangle command takes FVector2D"), Prop && Prop->Struct == TBaseStructure<FVector2D>::Get())) return;
					Prop->CopyCompleteValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Pair.Value);
				}
				Call.Execute();
			};
			SelectRect(RectEnd, RectStart); // Reverse drag is normalized by production code.
			const TArray<AActor*> BoxSelection = SelectedActors(Test, Controller);
			Test.TestTrue(TEXT("Real screen rectangle selects indexed owned actors"), BoxSelection.Contains(Source) && BoxSelection.Contains(Near));
			Test.TestFalse(TEXT("Real screen rectangle excludes enemy inside bounds"), BoxSelection.Contains(Far));
			SelectMany(Test, Controller, { Near });
			SelectRect(SourceScreen - FVector2D(1, 1), SourceScreen + FVector2D(1, 1), true);
			Test.TestTrue(TEXT("Additive rectangle retains prior unit and adds projected unit"),
				SelectedActors(Test, Controller) == TArray<AActor*>({ Near, Source }));
			SelectRect(SourceScreen - FVector2D(1, 1), SourceScreen + FVector2D(1, 1), true);
			Test.TestEqual(TEXT("Repeated additive rectangle does not duplicate or toggle units"), SelectedActors(Test, Controller).Num(), 2);
			const FVector2D EmptyStart(-10000, -10000), EmptyEnd(-9999, -9999);
			SelectRect(EmptyStart, EmptyEnd, true);
			Test.TestEqual(TEXT("Empty additive rectangle preserves selection"), SelectedActors(Test, Controller).Num(), 2);
			SelectRect(EmptyStart, EmptyEnd);
			Test.TestTrue(TEXT("Empty replacement rectangle clears selection"), SelectedActors(Test, Controller).IsEmpty());
			SelectRect(RectStart, RectEnd);
			SelectRect(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0), RectEnd);
			Test.TestTrue(TEXT("Invalid rectangle leaves selection unchanged"), SelectedActors(Test, Controller) == BoxSelection);
			SetOwner(Test, Near, EFactionID::Carthage);
			SelectRect(RectStart, RectEnd);
			Test.TestFalse(TEXT("Rectangle query observes ownership reindex"), SelectedActors(Test, Controller).Contains(Near));
			SetOwner(Test, Near, EFactionID::Rome);
			Command(Test, Controller, TEXT("ClearSelection"));
			Test.TestTrue(TEXT("Nearest search follows owner change"), SameActor(Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000), Far));
			SetOwner(Test, Far, EFactionID::None);
			Test.TestTrue(TEXT("Unowned living actor remains indexed"), SnapshotContains(Registry->GetUnitsForFaction(EFactionID::None), Far));
			Test.TestNull(TEXT("Unowned actor is not auto-acquired"), Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000));
			SetOwner(Test, Near, EFactionID::Carthage);
			SetOwner(Test, Far, EFactionID::Carthage);
			Far->SetActorLocation(Origin + FVector(-500, 0, 0));
			ACharacter* TieWinner = Near->GetFName().LexicalLess(Far->GetFName()) ? Near : Far;
			Test.TestTrue(TEXT("Equal distance uses explicit lexical tie-break"), SameActor(Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000), TieWinner));
			Far->SetActorLocation(Origin + FVector(900, 0, 0));
			Health(Test, Near, 0);
			Test.TestEqual(TEXT("Death immediately removes indexed record"), Registry->GetRegisteredUnitCount(), Baseline + 2);
			Test.TestTrue(TEXT("Nearest search excludes corpses"), SameActor(Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000), Far));
			TWeakObjectPtr<AActor> WeakFar(Far);
			Far->Destroy();
			Test.TestEqual(TEXT("Destroy immediately removes indexed record"), Registry->GetRegisteredUnitCount(), Baseline + 1);
			CollectGarbage(RF_NoFlags);
			// Threadsafe serial/index check deliberately ignores garbage flags: pending
			// destruction alone must not masquerade as actual collection.
			Test.TestFalse(TEXT("Registry does not retain destroyed actors across GC"), WeakFar.IsValid(true, true));
			Test.TestNull(TEXT("No stale target after destruction and GC"), Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000));

			for (EUnitControllerProbeAction Action : { EUnitControllerProbeAction::ChangeOwner, EUnitControllerProbeAction::Kill, EUnitControllerProbeAction::Destroy })
			{
				ACharacter* Nested = Spawn(EFactionID::Rome, 1600);
				if (!Nested) return true;
				TStrongObjectPtr<UUnitLifecycleProbe> Probe(NewObject<UUnitLifecycleProbe>());
				Probe->Test = &Test; Probe->ObservedUnit = Nested; Probe->RegistryAction = Action;
				Probe->RegistryExpectedLiveCount = Registry->GetRegisteredUnitCount();
				if (!BindProbe(Test, Nested, TEXT("OnOwnerFactionChanged"), Probe.Get(), TEXT("HandleRegistryOwner")) ||
					!BindProbe(Test, Nested, TEXT("OnUnitDied"), Probe.Get(), TEXT("HandleRegistryDeath"))) return true;
				SetOwner(Test, Nested, EFactionID::Carthage);
				Test.TestEqual(TEXT("Real owner callbacks executed"), Probe->RegistryOwnerEvents, Action == EUnitControllerProbeAction::ChangeOwner ? 2 : 1);
				Test.TestEqual(TEXT("Only combat death broadcasts death"), Probe->RegistryDeathEvents, Action == EUnitControllerProbeAction::Kill ? 1 : 0);
				if (Action == EUnitControllerProbeAction::ChangeOwner)
				{
					Test.TestTrue(TEXT("Nested owner wins without stale outer reindex"), SnapshotContains(Registry->GetUnitsForFaction(EFactionID::Rome), Nested));
					Test.TestFalse(TEXT("Superseded faction has no nested actor"), SnapshotContains(Registry->GetUnitsForFaction(EFactionID::Carthage), Nested));
				}
				else Test.TestEqual(TEXT("Nested termination cannot re-register actor"), Registry->GetRegisteredUnitCount(), Probe->RegistryExpectedLiveCount - 1);
				if (IsValid(Nested)) Nested->Destroy();
			}

			UWorld* ForeignWorld = UWorld::CreateWorld(EWorldType::Game, false);
			if (!Test.TestNotNull(TEXT("Second world exists"), ForeignWorld)) return true;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(ForeignWorld);
			URomanUnitRegistrySubsystem* ForeignRegistry = ForeignWorld->GetSubsystem<URomanUnitRegistrySubsystem>();
			ON_SCOPE_EXIT
			{
				ForeignWorld->DestroyWorld(false);
				if (ForeignRegistry)
				{
					Test.TestEqual(TEXT("World teardown clears registry records"), ForeignRegistry->GetRegisteredUnitCount(), 0);
					Test.TestTrue(TEXT("World teardown clears faction index"), ForeignRegistry->GetUnitsForFaction(EFactionID::Carthage).IsEmpty());
				}
				GEngine->DestroyWorldContext(ForeignWorld);
				CollectGarbage(RF_NoFlags);
			};
			ForeignWorld->SetGameInstance(World->GetGameInstance());
			ForeignWorld->InitializeActorsForPlay(FURL());
			if (!Test.TestNotNull(TEXT("Second world has its own registry"), ForeignRegistry)) return true;
			Test.TestTrue(TEXT("Registry instance is world scoped"), ForeignRegistry != Registry);
			Test.TestEqual(TEXT("Second registry starts empty"), ForeignRegistry->GetRegisteredUnitCount(), 0);
			ACharacter* Foreign = Spawn(EFactionID::Carthage, 1, true, ForeignWorld);
			if (!Foreign) return true;
			Foreign->DispatchBeginPlay();
			Test.TestTrue(TEXT("Foreign actor registers in its world"), ForeignRegistry->ContainsUnit(Foreign));
			Test.TestFalse(TEXT("Foreign actor absent from main registry"), Registry->ContainsUnit(Foreign));
			Test.TestFalse(TEXT("Main actor absent from foreign registry"), ForeignRegistry->ContainsUnit(Source));
			Test.TestNull(TEXT("Main nearest search excludes closer foreign actor"), Registry->FindNearestEnemy(Origin, EFactionID::Rome, 1000));
			Source->Destroy();
			Test.TestEqual(TEXT("Main fixture cleanup restores baseline"), Registry->GetRegisteredUnitCount(), Baseline);
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
	};
	FGuid CreateSquad(FAutomationTestBase& Test, URomanUnitRegistrySubsystem* Registry, const TArray<AActor*>& Members)
	{
		FCall Call(Test, Registry, TEXT("CreateSquad"), 2);
		FArrayProperty* Property = Call.Property<FArrayProperty>(TEXT("Members"));
		FObjectPropertyBase* Inner = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
		FStructProperty* Result = Call.Property<FStructProperty>(TEXT("ReturnValue"), true);
		if (!Test.TestTrue(TEXT("Squad reflected input/output types"), Inner && Result && Result->Struct == TBaseStructure<FGuid>::Get())) return {};
		FScriptArrayHelper Array(Property, Property->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()));
		Array.Resize(Members.Num());
		for (int32 Index = 0; Index < Members.Num(); ++Index) Inner->SetObjectPropertyValue(Array.GetRawPtr(Index), Members[Index]);
		Call.Execute();
		return *Result->ContainerPtrToValuePtr<FGuid>(Call.Params.GetStructMemory());
	}
	FGuid UnitSquad(FAutomationTestBase& Test, URomanUnitRegistrySubsystem* Registry, AActor* Unit)
	{
		FCall Call(Test, Registry, TEXT("GetUnitSquad"), 2);
		if (FObjectPropertyBase* Property = Call.Property<FObjectPropertyBase>(TEXT("Unit")))
			Property->SetObjectPropertyValue_InContainer(Call.Params.GetStructMemory(), Unit);
		FStructProperty* Result = Call.Property<FStructProperty>(TEXT("ReturnValue"), true);
		if (!Test.TestTrue(TEXT("Squad handle type"), Result && Result->Struct == TBaseStructure<FGuid>::Get())) return {};
		Call.Execute();
		return *Result->ContainerPtrToValuePtr<FGuid>(Call.Params.GetStructMemory());
	}
	class FExerciseFormation : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseFormation(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 90)
			{
				Test.AddError(FString::Printf(TEXT("Formation PIE timed out in phase %d"), Phase)); return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			APlayerController* Player = World->GetFirstPlayerController();
			if (!Player || !Bool(Test, Player, TEXT("CanIssueTacticalCommands"))) return false;
			auto PlayerMove = [&](const FVector& Destination)
			{
				FCall Call(Test, Player, TEXT("CommandSelectedUnitsMove"), 1);
				if (FStructProperty* Property = Call.Property<FStructProperty>(TEXT("Destination")))
					Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Destination);
				Call.Execute();
			};
			if (Phase == 0)
			{
				UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				FNavLocation Point;
				if (!Nav || Nav->IsNavigationBuildInProgress() ||
					!Nav->ProjectPointToNavigation(FVector(1000, -3500, 0), Point, FVector(100, 100, 250))) return false;
				for (TActorIterator<ACharacter> It(World); It; ++It)
					if (It->FindFunction(TEXT("CommandHold"))) Command(Test, *It, TEXT("CommandHold"));
				UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
				for (int32 Index = 0; Index < 4; ++Index)
				{
					FActorSpawnParameters Params;
					Params.bDeferConstruction = true;
					Params.ObjectFlags |= RF_Transient;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					ACharacter* Unit = World->SpawnActor<ACharacter>(Class, FTransform(FVector(-1400, -3740 + 240 * Index, 100)), Params);
					if (!Test.TestNotNull(TEXT("Formation member spawned"), Unit)) return true;
					Members.Add(Unit);
					if (!Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Rome)) return true;
					Unit->FinishSpawning(Unit->GetActorTransform());
					Command(Test, Unit, TEXT("CommandHold"));
				}
				URomanUnitRegistrySubsystem* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>();
				Squad = CreateSquad(Test, Registry, { Members[0].Get(), Members[1].Get(), Members[2].Get(), Members[3].Get() });
				Test.TestTrue(TEXT("Real squad created for partial-selection test"), Squad.IsValid());
				SelectMany(Test, Player, { Members[0].Get(), Members[1].Get(), Members[2].Get() });
				PlayerMove(Point.Location);
				Test.TestEqual(TEXT("Unselected squad member receives no move"), Order(Test, Members[3].Get()), EUnitOrderType::Hold);
				TArray<FVector> Destinations;
				for (int32 Index = 0; Index < 3; ++Index)
				{
					Test.TestEqual(TEXT("Selected member receives Move"), Order(Test, Members[Index].Get()), EUnitOrderType::Move);
					Destinations.Add(StoredState<FUnitOrderState>(Test, Members[Index].Get(), TEXT("Orders")).GetDestination());
				}
				for (int32 A = 0; A < 3; ++A)
					for (int32 B = A + 1; B < 3; ++B)
						Test.TestTrue(TEXT("Projected formation slots remain separated"), FVector::Dist2D(Destinations[A], Destinations[B]) > 100);
				PlayerMove(FVector(100000, 0, 0));
				for (int32 Index = 0; Index < 3; ++Index)
					Test.TestEqual(TEXT("Invalid footprint preserves every prior destination"), StoredState<FUnitOrderState>(Test, Members[Index].Get(), TEXT("Orders")).GetDestination(), Destinations[Index]);
				UUserWidget* Root = Cast<UUserWidget>(ObjectQuery(Test, Player, TEXT("GetMainWidget")));
				UTextBlock* Feedback = Root ? Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("InteractionFeedbackText"))) : nullptr;
				Test.TestTrue(TEXT("Invalid formation reports user-visible feedback"), Feedback && !Feedback->GetText().IsEmpty());
				Phase = 1;
				return false;
			}
			for (const auto& Unit : Members)
				if (!Test.TestTrue(TEXT("Formation member stays valid"), Unit.IsValid())) return true;
			if (Phase == 1)
			{
				for (int32 Index = 0; Index < 3; ++Index)
					if (NavigationResult(Test, Members[Index].Get()) != EUnitNavigationResult::Moving) return false;
				AAIController* AI = Cast<AAIController>(Members[0]->GetController());
				if (!Test.TestNotNull(TEXT("Formation member has live path follower"), AI)) return true;
				bool bNested = false;
				const FDelegateHandle Handle = AI->GetPathFollowingComponent()->OnRequestFinished.AddLambda(
					[&](FAIRequestID, const FPathFollowingResult&)
					{
						if (bNested) return;
						bNested = true;
						PlayerMove(FVector(1200, -4500, 0));
						for (int32 Index = 0; Index < 3; ++Index)
							Goals.Add(StoredState<FUnitOrderState>(Test, Members[Index].Get(), TEXT("Orders")).GetDestination());
					});
				PlayerMove(FVector(-1000, -4000, 0));
				AI->GetPathFollowingComponent()->OnRequestFinished.Remove(Handle);
				Test.TestTrue(TEXT("Real path abort issues a nested group order"), bNested);
				UUserWidget* Root = Cast<UUserWidget>(ObjectQuery(Test, Player, TEXT("GetMainWidget")));
				UTextBlock* Feedback = Root ? Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("InteractionFeedbackText"))) : nullptr;
				Test.TestTrue(TEXT("Valid formation clears obsolete feedback"), Feedback && Feedback->GetText().IsEmpty());
				if (!Test.TestEqual(TEXT("Nested order has every member"), Goals.Num(), 3)) return true;
				for (int32 Index = 0; Index < 3; ++Index)
					Test.TestEqual(TEXT("Outer batch cannot overwrite newer formation"), StoredState<FUnitOrderState>(Test, Members[Index].Get(), TEXT("Orders")).GetDestination(), Goals[Index]);
				Arrived.Init(false, 3);
				Phase = 2;
				return false;
			}
			bool bAllArrived = true;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				if (Arrived[Index]) continue;
				const EUnitNavigationResult Result = NavigationResult(Test, Members[Index].Get());
				if (Result == EUnitNavigationResult::Unreachable)
				{
					Test.AddError(FString::Printf(TEXT("Formation member %d failed its real path"), Index)); return true;
				}
				if (Result != EUnitNavigationResult::Arrived) { bAllArrived = false; continue; }
				Test.TestTrue(TEXT("Member physically reaches its own assigned slot"), FVector::Dist2D(Members[Index]->GetActorLocation(), Goals[Index]) < 60);
				Arrived[Index] = true;
				Command(Test, Members[Index].Get(), TEXT("CommandHold"));
			}
			if (!bAllArrived) return false;
			URomanUnitRegistrySubsystem* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>();
			for (const auto& Unit : Members)
				Test.TestEqual(TEXT("Formation movement does not replace squad identity"), UnitSquad(Test, Registry, Unit.Get()), Squad);
			Test.TestEqual(TEXT("Unselected squad member stays on Hold"), Order(Test, Members[3].Get()), EUnitOrderType::Hold);
			SelectMany(Test, Player, { Members[0].Get() });
			const FVector SingleGoal(1200, -5000, 0);
			PlayerMove(SingleGoal);
			Test.TestEqual(TEXT("Single selection keeps exact destination without offset"), StoredState<FUnitOrderState>(Test, Members[0].Get(), TEXT("Orders")).GetDestination(), SingleGoal);
			Command(Test, Player, TEXT("ClearSelection"));
			for (const auto& Unit : Members) Unit->Destroy();
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		int32 Phase = 0;
		FGuid Squad;
		TArray<TWeakObjectPtr<ACharacter>> Members;
		TArray<FVector> Goals;
		TArray<bool> Arrived;
	};

	class FExerciseGroupCongestion : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseGroupCongestion(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 120.0)
			{
				Test.AddError(FString::Printf(TEXT("Opposing formation routes timed out; %d of 6 arrived"), ArrivedCount));
				return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			APlayerController* Player = World->GetFirstPlayerController();
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Player || !Nav || !Bool(Test, Player, TEXT("CanIssueTacticalCommands")) || Nav->IsNavigationBuildInProgress()) return false;
			auto PlayerMove = [&](const FVector& Destination)
			{
				FCall Call(Test, Player, TEXT("CommandSelectedUnitsMove"), 1);
				if (FStructProperty* Property = Call.Property<FStructProperty>(TEXT("Destination")))
					Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Destination);
				Call.Execute();
			};
			if (Phase == 0)
			{
				FNavLocation Left, Right;
				if (!Nav->ProjectPointToNavigation(FVector(-1500, -3500, 0), Left, FVector(100, 100, 250)) ||
					!Nav->ProjectPointToNavigation(FVector(1500, -3500, 0), Right, FVector(100, 100, 250))) return false;
				for (TActorIterator<ACharacter> It(World); It; ++It)
					if (It->FindFunction(TEXT("CommandHold"))) Command(Test, *It, TEXT("CommandHold"));
				UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
				if (!Test.TestNotNull(TEXT("Congestion fixture class"), Class)) return true;
				for (int32 Index = 0; Index < 6; ++Index)
				{
					const bool bLeft = Index < 3;
					const FVector Location(bLeft ? -1500.0 : 1500.0, -3900.0 + 400.0 * (Index % 3), 100.0);
					FActorSpawnParameters Params;
					Params.bDeferConstruction = true;
					Params.ObjectFlags |= RF_Transient;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					ACharacter* Unit = World->SpawnActor<ACharacter>(Class, FTransform(Location), Params);
					if (!Test.TestNotNull(TEXT("Congestion member spawned"), Unit)) return true;
					Members.Add(Unit);
					if (!Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Rome)) return true;
					Unit->FinishSpawning(Unit->GetActorTransform());
					Command(Test, Unit, TEXT("CommandHold"));
				}
				SelectMany(Test, Player, { Members[0].Get(), Members[1].Get(), Members[2].Get() });
				PlayerMove(Right.Location);
				SelectMany(Test, Player, { Members[3].Get(), Members[4].Get(), Members[5].Get() });
				PlayerMove(Left.Location);
				for (int32 Index = 0; Index < 6; ++Index)
				{
					if (!Test.TestEqual(TEXT("Both groups receive simultaneous Move orders"), Order(Test, Members[Index].Get()), EUnitOrderType::Move)) return true;
					Goals.Add(StoredState<FUnitOrderState>(Test, Members[Index].Get(), TEXT("Orders")).GetDestination());
				}
				Arrived.Init(false, 6);
				Phase = 1;
				return false;
			}
			for (const auto& Unit : Members)
				if (!Test.TestTrue(TEXT("Congestion member remains valid"), Unit.IsValid())) return true;
			for (int32 A = 0; A < Members.Num(); ++A)
				for (int32 B = A + 1; B < Members.Num(); ++B)
				{
					const double Separation = FVector::Dist2D(Members[A]->GetActorLocation(), Members[B]->GetActorLocation());
					if (A < 3 && B >= 3) MinOpposingDistance = FMath::Min(MinOpposingDistance, Separation);
					if (Separation < 65.0)
					{
						Test.AddError(FString::Printf(TEXT("Congested units %d and %d overlap"), A, B));
						return true;
					}
				}
			bool bAnyMoving = false;
			for (int32 Index = 0; Index < Members.Num(); ++Index)
			{
				if (Arrived[Index]) continue;
				ACharacter* Unit = Members[Index].Get();
				bMovedWithVelocity |= Unit->GetVelocity().Size2D() > 1.0;
				const EUnitNavigationResult Result = NavigationResult(Test, Unit);
				if (Result == EUnitNavigationResult::Unreachable)
				{
					Test.AddError(FString::Printf(TEXT("Congestion member %d lost its route"), Index));
					return true;
				}
				if (Result != EUnitNavigationResult::Arrived) { bAnyMoving = true; continue; }
				Test.TestTrue(TEXT("Congestion member reaches its assigned slot"), FVector::Dist2D(Unit->GetActorLocation(), Goals[Index]) < 60);
				Arrived[Index] = true;
				++ArrivedCount;
				Command(Test, Unit, TEXT("CommandHold"));
			}
			if (bAnyMoving) return false;
			Test.TestEqual(TEXT("All opposing group members arrived"), ArrivedCount, 6);
			Test.TestTrue(TEXT("Congested traffic used CharacterMovement"), bMovedWithVelocity);
			Test.TestTrue(TEXT("Opposing groups actually shared a congested corridor"), MinOpposingDistance < 300.0);
			Test.AddInfo(FString::Printf(TEXT("Closest opposing units: %.1f cm"), MinOpposingDistance));
			Command(Test, Player, TEXT("ClearSelection"));
			for (const auto& Unit : Members) Unit->Destroy();
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		int32 Phase = 0;
		int32 ArrivedCount = 0;
		bool bMovedWithVelocity = false;
		double MinOpposingDistance = TNumericLimits<double>::Max();
		TArray<TWeakObjectPtr<ACharacter>> Members;
		TArray<FVector> Goals;
		TArray<bool> Arrived;
	};

	class FExerciseUnitLoadProfile : public IAutomationLatentCommand
	{
	public:
		FExerciseUnitLoadProfile(FAutomationTestBase* InTest, int32 InCount)
			: Test(*InTest), Count(InCount), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 120.0)
			{
				StopCapture();
				Test.AddError(FString::Printf(TEXT("%d-unit profile timed out in phase %d"), Count, Phase));
				return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			APlayerController* Player = World->GetFirstPlayerController();
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Player || !Nav || !Bool(Test, Player, TEXT("CanIssueTacticalCommands")) || Nav->IsNavigationBuildInProgress()) return false;
			if (Phase == 0)
			{
				FNavLocation Goal;
				if (!Nav->ProjectPointToNavigation(FVector(2500, -3500, 0), Goal, FVector(100, 100, 250))) return false;
				for (TActorIterator<ACharacter> It(World); It; ++It)
					if (It->FindFunction(TEXT("CommandHold"))) Command(Test, *It, TEXT("CommandHold"));
				UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
				if (!Test.TestNotNull(TEXT("Load fixture class"), Class)) return true;
				TArray<AActor*> Selection;
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const FVector Location(-4000.0 + 240.0 * (Index / 10), -4800.0 + 240.0 * (Index % 10), 100.0);
					FActorSpawnParameters Params;
					Params.bDeferConstruction = true;
					Params.ObjectFlags |= RF_Transient;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					ACharacter* Unit = World->SpawnActor<ACharacter>(Class, FTransform(Location), Params);
					if (!Test.TestNotNull(TEXT("Load fixture unit"), Unit)) return true;
					Members.Add(Unit);
					if (!Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Rome)) return true;
					Unit->FinishSpawning(Unit->GetActorTransform());
					Command(Test, Unit, TEXT("CommandHold"));
					Selection.Add(Unit);
				}
				// Put the active traffic inside the tactical viewport for rendered runs.
				if (APawn* CameraPawn = Player->GetPawn())
				{
					CameraPawn->SetActorLocation(FVector(-3000, -3500, 0));
					FCall Zoom(Test, CameraPawn, TEXT("SetZoomLevel"), 1);
					if (FFloatProperty* Property = Zoom.Property<FFloatProperty>(TEXT("NewZoomLevel")))
						Property->SetPropertyValue_InContainer(Zoom.Params.GetStructMemory(), 0.52f);
					Zoom.Execute();
				}
				SelectMany(Test, Player, Selection);
				FCall Call(Test, Player, TEXT("CommandSelectedUnitsMove"), 1);
				if (FStructProperty* Property = Call.Property<FStructProperty>(TEXT("Destination")))
					Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Goal.Location);
				Call.Execute();
				int32 MovingCount = 0;
				for (const auto& Unit : Members)
					MovingCount += Order(Test, Unit.Get()) == EUnitOrderType::Move ? 1 : 0;
				if (!Test.TestEqual(TEXT("Load fixture orders all real units"), MovingCount, Count)) return true;
				Phase = 1;
				return false;
			}
			if (Phase == 1)
			{
				if (++FrameCount < 30) return false; // Warm up paths, RVO and render-independent world ticks.
				int32 ViewWidth = 0, ViewHeight = 0;
				Player->GetViewportSize(ViewWidth, ViewHeight);
				for (const auto& Unit : Members)
				{
					FVector2D Screen;
					if (Unit.IsValid() && Player->ProjectWorldLocationToScreen(Unit->GetActorLocation(), Screen) &&
						Screen.X >= 0 && Screen.X < ViewWidth && Screen.Y >= 0 && Screen.Y < ViewHeight) ++VisibleAtCapture;
				}
				FrameCount = 0;
				CaptureStartedAt = FPlatformTime::Seconds();
				UsedPhysicalStart = FPlatformMemory::GetStats().UsedPhysical;
				if (GEngine)
				{
					GEngine->Exec(World, *FString::Printf(TEXT("Trace.Bookmark M3Load%dStart"), Count));
					GEngine->Exec(World, TEXT("CsvProfile Start"));
					bCaptureActive = true;
				}
				Phase = 2;
				return false;
			}
			if (++FrameCount < 240) return false;
			StopCapture();
			int32 LiveCount = 0;
			int32 FailedCount = 0;
			for (const auto& Unit : Members)
			{
				if (Unit.IsValid())
				{
					++LiveCount;
					FailedCount += NavigationResult(Test, Unit.Get()) == EUnitNavigationResult::Unreachable ? 1 : 0;
				}
			}
			Test.TestEqual(TEXT("Load fixture retains all units"), LiveCount, Count);
			Test.TestEqual(TEXT("Active load routes stay navigable"), FailedCount, 0);
			Test.AddInfo(FString::Printf(TEXT("M3Load%d: 240 sampled PIE frames, %.3f wall seconds, %d/%d units initially inside viewport, process used physical memory %.1f -> %.1f MiB; editor PIE measurement, not a packaged-player baseline; GPU validity depends on launch RHI"),
				Count, FPlatformTime::Seconds() - CaptureStartedAt,
				VisibleAtCapture, Count,
				double(UsedPhysicalStart) / 1048576.0, double(FPlatformMemory::GetStats().UsedPhysical) / 1048576.0));
			Command(Test, Player, TEXT("ClearSelection"));
			for (const auto& Unit : Members) if (Unit.IsValid()) Unit->Destroy();
			return true;
		}
	private:
		void StopCapture()
		{
			if (!bCaptureActive || !GEngine) return;
			if (UWorld* World = GEditor ? GEditor->PlayWorld : nullptr)
			{
				GEngine->Exec(World, TEXT("CsvProfile Stop"));
				GEngine->Exec(World, *FString::Printf(TEXT("Trace.Bookmark M3Load%dEnd"), Count));
			}
			bCaptureActive = false;
		}
		FAutomationTestBase& Test;
		int32 Count;
		double StartedAt;
		double CaptureStartedAt = 0;
		uint64 UsedPhysicalStart = 0;
		int32 FrameCount = 0;
		int32 Phase = 0;
		int32 VisibleAtCapture = 0;
		bool bCaptureActive = false;
		TArray<TWeakObjectPtr<ACharacter>> Members;
	};

	bool ClickUnitCommandThroughSlate(FAutomationTestBase& Test, UUserWidget* Root, UButton* Button)
	{
		if (!Test.TestTrue(TEXT("Slate pointer router initialized"), FSlateApplication::IsInitialized() && Root && Button)) return false;
		Root->ForceLayoutPrepass();
		const TSharedPtr<SWidget> SlateButton = Button->GetCachedWidget();
		if (!Test.TestTrue(TEXT("Visible command materializes a native SButton"),
			SlateButton.IsValid() && SlateButton->GetType() == FName(TEXT("SButton")))) return false;
		FSlateApplication& Slate = FSlateApplication::Get();
		FWidgetPath Path;
		if (!Test.TestTrue(TEXT("Command button has a live viewport path"),
			Slate.GeneratePathToWidgetUnchecked(SlateButton.ToSharedRef(), Path) && Path.IsValid())) return false;
		for (int32 Index = 0; Index < Path.Widgets.Num() - 1; ++Index)
			if (!Test.TestTrue(TEXT("Command ancestors permit child pointer input"),
				Path.Widgets[Index].Widget->GetVisibility().AreChildrenHitTestVisible())) return false;
		const FGeometry& Geometry = Path.Widgets.Last().Geometry;
		if (!Test.TestTrue(TEXT("Command button has a positive arranged area"),
			Geometry.GetLocalSize().X > 0 && Geometry.GetLocalSize().Y > 0)) return false;
		const FVector2D Position = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
		TArray<FWidgetAndPointer> PointerWidgets;
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
			PointerWidgets.Emplace(Path.Widgets[Index], TOptional<FVirtualPointerPosition>());
		const FWidgetPath PointerPath{MakeArrayView(PointerWidgets)};
		const TSharedRef<FSlateVirtualUserHandle> VirtualUser = Slate.FindOrCreateVirtualUser(26);
		const TSharedPtr<FSlateUser> User = Slate.GetUser(VirtualUser->GetUserIndex());
		if (!Test.TestTrue(TEXT("Command pointer uses an isolated virtual user"), User.IsValid() && User->IsVirtualUser())) return false;
		const uint32 UserIndex = static_cast<uint32>(VirtualUser->GetUserIndex());
		const TSet<FKey> NoButtons;
		TSet<FKey> PressedButtons;
		PressedButtons.Add(EKeys::LeftMouseButton);
		const FModifierKeysState Modifiers;
		const FPointerEvent Move(UserIndex, 0, Position, Position, NoButtons, FKey(), 0.0f, Modifiers);
		const FPointerEvent Down(UserIndex, 0, Position, Position, PressedButtons, EKeys::LeftMouseButton, 0.0f, Modifiers);
		const FPointerEvent Up(UserIndex, 0, Position, Position, NoButtons, EKeys::LeftMouseButton, 0.0f, Modifiers);
		Slate.RoutePointerMoveEvent(PointerPath, Move, false);
		const FReply DownReply = Slate.RoutePointerDownEvent(PointerPath, Down);
		const FReply UpReply = Slate.RoutePointerUpEvent(PointerPath, Up);
		Slate.RoutePointerMoveEvent(FWidgetPath(), Move, false);
		User->ReleaseAllCapture();
		return Test.TestTrue(TEXT("Command SButton consumes press and release before gameplay"),
			DownReply.IsEventHandled() && DownReply.GetHandler() == SlateButton &&
			UpReply.IsEventHandled() && UpReply.GetHandler() == SlateButton);
	}

	class FExerciseSelectedCommands : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseSelectedCommands(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedAt > 90)
			{
				Test.AddError(FString::Printf(TEXT("Stop/Hold PIE timed out in phase %d"), Phase)); return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			APlayerController* Player = World->GetFirstPlayerController();
			if (!Player) return false;
			auto PlayerMove = [&]()
			{
				FCall Call(Test, Player, TEXT("CommandSelectedUnitsMove"), 1);
				if (FStructProperty* Prop = Call.Property<FStructProperty>(TEXT("Destination")))
				{
					const FVector Goal(1200, -3500, 0);
					Prop->CopyCompleteValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), &Goal);
				}
				Call.Execute();
			};
			if (Phase == 0)
			{
				UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				FNavLocation Point;
				if (!Nav || Nav->IsNavigationBuildInProgress() ||
					!Nav->ProjectPointToNavigation(FVector(1200, -3500, 0), Point, FVector(100, 100, 250))) return false;
				for (TActorIterator<ACharacter> It(World); It; ++It)
					if (It->FindFunction(TEXT("CommandHold"))) Command(Test, *It, TEXT("CommandHold"));
				UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
				for (int32 Index = 0; Index < 2; ++Index)
				{
					FActorSpawnParameters Params;
					Params.bDeferConstruction = true;
					Params.ObjectFlags |= RF_Transient;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					ACharacter* Unit = World->SpawnActor<ACharacter>(Class,
						FTransform(FVector(-1200, -3500 + Index * 300, 100)), Params);
					if (!Test.TestNotNull(TEXT("Command fixture spawned"), Unit)) return true;
					Units.Add(Unit);
					if (!Initialize(Test, Unit, TEXT("Legionary"), EFactionID::Rome)) return true;
					Unit->FinishSpawning(Unit->GetActorTransform());
					Command(Test, Unit, TEXT("CommandHold"));
				}
				Root = Cast<UUserWidget>(ObjectQuery(Test, Player, TEXT("GetMainWidget")));
				Stop = Root.IsValid() ? Cast<UButton>(Root->GetWidgetFromName(TEXT("StopButton"))) : nullptr;
				Hold = Root.IsValid() ? Cast<UButton>(Root->GetWidgetFromName(TEXT("HoldButton"))) : nullptr;
				if (!Test.TestTrue(TEXT("Native Stop and Hold controls exist"), Stop.IsValid() && Hold.IsValid())) return true;
				Test.TestFalse(TEXT("Command controls hidden without unit selection"), Stop->IsVisible() || Hold->IsVisible());
				SelectMany(Test, Player, { Units[0].Get(), Units[1].Get() });
				Test.TestTrue(TEXT("Tactical selection exposes enabled commands"), Stop->IsVisible() && Hold->IsVisible() &&
					Stop->GetIsEnabled() && Hold->GetIsEnabled());
				PlayerMove();
				Phase = 1;
				return false;
			}
			for (const auto& Unit : Units)
				if (!Test.TestTrue(TEXT("Command fixture remains alive"), Unit.IsValid())) return true;
			if (Phase == 1)
			{
				for (const auto& Unit : Units)
					if (NavigationResult(Test, Unit.Get()) != EUnitNavigationResult::Moving) return false;
				AAIController* AI = Cast<AAIController>(Units[0]->GetController());
				if (!Test.TestNotNull(TEXT("Selected unit has active path following"), AI)) return true;
				bool bNested = false;
				const FDelegateHandle Handle = AI->GetPathFollowingComponent()->OnRequestFinished.AddLambda(
					[&](FAIRequestID, const FPathFollowingResult&)
					{
						if (bNested) return;
						bNested = true;
						Command(Test, Player, TEXT("CommandSelectedUnitsHold"));
					});
				Command(Test, Player, TEXT("CommandSelectedUnitsStop"));
				AI->GetPathFollowingComponent()->OnRequestFinished.Remove(Handle);
				Test.TestTrue(TEXT("Stopping an active route invoked the nested order"), bNested);
				for (const auto& Unit : Units)
					Test.TestEqual(TEXT("New Hold supersedes outer Stop for the whole selection"), Order(Test, Unit.Get()), EUnitOrderType::Hold);
				PlayerMove();
				Phase = 2;
				return false;
			}
			if (Phase == 2)
			{
				for (const auto& Unit : Units)
					if (NavigationResult(Test, Unit.Get()) != EUnitNavigationResult::Moving) return false;
				if (!ClickUnitCommandThroughSlate(Test, Root.Get(), Stop.Get())) return true;
				for (const auto& Unit : Units)
				{
					Test.TestEqual(TEXT("Stop click clears tactical order"), Order(Test, Unit.Get()), EUnitOrderType::None);
					Test.TestEqual(TEXT("Stop click clears navigation result"), NavigationResult(Test, Unit.Get()), EUnitNavigationResult::None);
					AAIController* AI = Cast<AAIController>(Unit->GetController());
					Test.TestTrue(TEXT("Stop click idles path following"), AI && AI->GetMoveStatus() == EPathFollowingStatus::Idle);
				}
				PlayerMove();
				Phase = 3;
				return false;
			}
			if (Phase == 3)
			{
				for (const auto& Unit : Units)
					if (NavigationResult(Test, Unit.Get()) != EUnitNavigationResult::Moving) return false;
				if (!ClickUnitCommandThroughSlate(Test, Root.Get(), Hold.Get())) return true;
				for (const auto& Unit : Units)
				{
					Test.TestEqual(TEXT("Hold click persists commanded order"), Order(Test, Unit.Get()), EUnitOrderType::Hold);
					AAIController* AI = Cast<AAIController>(Unit->GetController());
					Test.TestTrue(TEXT("Hold click idles path following"), AI && AI->GetMoveStatus() == EPathFollowingStatus::Idle);
				}
				Phase = 4;
				return false;
			}
			if (Phase == 4)
			{
				for (const auto& Unit : Units)
					Test.TestEqual(TEXT("Hold still persists after world update"), Order(Test, Unit.Get()), EUnitOrderType::Hold);
				auto SetMode = [&](EGameplayMode Mode)
				{
					FCall Call(Test, World->GetAuthGameMode(), TEXT("SetGameplayMode"), 1);
					if (FEnumProperty* Prop = Call.Property<FEnumProperty>(TEXT("NewMode")))
						Prop->GetUnderlyingProperty()->SetIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Call.Params.GetStructMemory()), static_cast<int64>(Mode));
					Call.Execute();
				};
				SetMode(EGameplayMode::Strategic);
				Test.TestFalse(TEXT("Strategic mode hides tactical unit controls"), Stop->IsVisible() || Hold->IsVisible());
				Command(Test, Player, TEXT("CommandSelectedUnitsStop"));
				FCall(Test, Root.Get(), TEXT("HandleStopClicked"), 0).Execute();
				for (const auto& Unit : Units)
					Test.TestEqual(TEXT("Strategic mode rejects native and stale UI commands"), Order(Test, Unit.Get()), EUnitOrderType::Hold);
				SetMode(EGameplayMode::Tactical);
				Test.TestTrue(TEXT("Tactical return restores unit controls"), Stop->IsVisible() && Hold->IsVisible());
				ObjectCommand(Test, Player, TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), Units[0].Get());
				Test.TestFalse(TEXT("Possession hides tactical command buttons"), Stop->IsVisible() || Hold->IsVisible());
				FCall(Test, Root.Get(), TEXT("HandleStopClicked"), 0).Execute();
				FCall(Test, Root.Get(), TEXT("HandleHoldClicked"), 0).Execute();
				Test.TestEqual(TEXT("Stale UI callbacks cannot command possessed unit"), Order(Test, Units[0].Get()), EUnitOrderType::None);
				Command(Test, Player, TEXT("ExitFirstPersonMode"));
				SelectMany(Test, Player, { Units[0].Get(), Units[1].Get() });
				Command(Test, Player, TEXT("CommandSelectedUnitsHold"));
				SetOwner(Test, Units[1].Get(), EFactionID::Carthage);
				Test.TestTrue(TEXT("Owner loss keeps commands for remaining selection"), Stop->IsVisible() && Hold->IsVisible());
				SetOwner(Test, Units[0].Get(), EFactionID::Carthage);
				Test.TestFalse(TEXT("Owner loss hides controls when selection empties"), Stop->IsVisible() || Hold->IsVisible());
				FCall(Test, Root.Get(), TEXT("HandleStopClicked"), 0).Execute();
				Test.TestEqual(TEXT("Stale Stop callback cannot touch former unit"), Order(Test, Units[0].Get()), EUnitOrderType::Hold);
				for (const auto& Unit : Units) Unit->Destroy();
				return true;
			}
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
		int32 Phase = 0;
		TWeakObjectPtr<UUserWidget> Root;
		TWeakObjectPtr<UButton> Stop;
		TWeakObjectPtr<UButton> Hold;
		TArray<TWeakObjectPtr<ACharacter>> Units;
	};

	class FExerciseSquadRegistry : public IAutomationLatentCommand
	{
	public:
		explicit FExerciseSquadRegistry(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (FPlatformTime::Seconds() - StartedAt > 30.0) { Test.AddError(TEXT("Squad PIE startup timed out")); return true; }
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!State || !State->IsGameplayInitialized()) return false;
			URomanUnitRegistrySubsystem* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>();
			UClass* UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestTrue(TEXT("Squad PIE dependencies ready"), Registry && UnitClass)) return true;
			Test.TestEqual(TEXT("Bootstrap creates two tactical squads"), Registry->GetSquadCount(), 2);
			for (EFactionID Faction : { EFactionID::Rome, EFactionID::Carthage })
			{
				const TArray<AUnitBase*> Units = Registry->GetUnitsForFaction(Faction);
				if (!Test.TestFalse(TEXT("Bootstrap faction populated"), Units.IsEmpty())) return true;
				const FGuid Squad = Registry->GetUnitSquad(Units[0]);
				Test.TestTrue(TEXT("Bootstrap squad handle valid"), Squad.IsValid());
				Test.TestEqual(TEXT("Bootstrap squad contains whole initial force"), Registry->GetSquadMembers(Squad).Num(), Units.Num());
				for (AUnitBase* Unit : Units) Test.TestEqual(TEXT("Bootstrap membership consistent"), Registry->GetUnitSquad(Unit), Squad);
			}
			const int32 Baseline = Registry->GetSquadCount();
			TArray<TWeakObjectPtr<AActor>> Fixtures;
			ON_SCOPE_EXIT { for (const TWeakObjectPtr<AActor>& Actor : Fixtures) if (Actor.IsValid()) Actor->Destroy(); };
			auto Spawn = [&](EFactionID Faction, bool bFinish = true, UWorld* InWorld = nullptr)
			{
				FActorSpawnParameters Params;
				Params.bDeferConstruction = true; Params.ObjectFlags |= RF_Transient;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ACharacter* Unit = (InWorld ? InWorld : World)->SpawnActor<ACharacter>(UnitClass,
					FTransform(FVector(400000.0 + Fixtures.Num() * 500.0, 400000.0, 300.0)), Params);
				if (!Test.TestNotNull(TEXT("Squad fixture spawned"), Unit)) return Unit;
				Fixtures.Add(Unit);
				Initialize(Test, Unit, TEXT("Legionary"), Faction);
				if (bFinish) Unit->FinishSpawning(Unit->GetActorTransform());
				Unit->SetActorTickEnabled(false);
				for (UActorComponent* Part : Unit->GetComponents()) if (Part) Part->SetComponentTickEnabled(false);
				return Unit;
			};
			ACharacter* First = Spawn(EFactionID::Rome);
			ACharacter* Second = Spawn(EFactionID::Rome);
			ACharacter* Enemy = Spawn(EFactionID::Carthage);
			ACharacter* Deferred = Spawn(EFactionID::Rome, false);
			if (!First || !Second || !Enemy || !Deferred) return true;
			Test.TestFalse(TEXT("Empty group rejected"), CreateSquad(Test, Registry, {}).IsValid());
			Test.TestFalse(TEXT("Invalid mixed request rejected atomically"), CreateSquad(Test, Registry, {First, Enemy}).IsValid());
			Test.TestFalse(TEXT("Null mixed request rejected atomically"), CreateSquad(Test, Registry, {First, nullptr}).IsValid());
			Test.TestFalse(TEXT("Deferred unit rejected"), CreateSquad(Test, Registry, {First, Deferred}).IsValid());
			Test.TestFalse(TEXT("Invalid creation did not claim first member"), UnitSquad(Test, Registry, First).IsValid());
			SetOwner(Test, Enemy, EFactionID::None);
			Test.TestFalse(TEXT("Unowned unit rejected"), CreateSquad(Test, Registry, {Enemy}).IsValid());
			SetOwner(Test, Enemy, EFactionID::Carthage);
			const FGuid Squad = CreateSquad(Test, Registry, {Second, First, Second});
			Test.TestTrue(TEXT("Valid group created"), Squad.IsValid());
			const TArray<AUnitBase*> Ordered = Registry->GetSquadMembers(Squad);
			Test.TestTrue(TEXT("Membership deduplicated in caller order"), Ordered.Num() == 2 &&
				static_cast<const void*>(Ordered[0]) == Second && static_cast<const void*>(Ordered[1]) == First);
			Test.TestEqual(TEXT("Squad derives canonical faction"), Registry->GetSquadFaction(Squad), EFactionID::Rome);
			Test.TestFalse(TEXT("Existing membership cannot be silently reassigned"), CreateSquad(Test, Registry, {First}).IsValid());
			Deferred->FinishSpawning(Deferred->GetActorTransform());
			Test.TestFalse(TEXT("A later grouped member rejects the whole request"), CreateSquad(Test, Registry, {Deferred, First}).IsValid());
			Test.TestFalse(TEXT("Atomic rejection did not claim an earlier ungrouped member"), UnitSquad(Test, Registry, Deferred).IsValid());
			SetOwner(Test, First, EFactionID::Rome);
			Test.TestEqual(TEXT("No-op ownership retains squad"), UnitSquad(Test, Registry, First), Squad);
			Command(Test, First, TEXT("CommandHold"));
			Test.TestTrue(TEXT("Disband succeeds"), Registry->DisbandSquad(Squad));
			Test.TestFalse(TEXT("Disband is not repeat success"), Registry->DisbandSquad(Squad));
			Test.TestEqual(TEXT("Disband does not change order"), Order(Test, First), EUnitOrderType::Hold);
			Test.TestTrue(TEXT("Disband does not remove live unit"), Registry->ContainsUnit(First));
			Test.TestFalse(TEXT("Disband clears reverse lookup"), UnitSquad(Test, Registry, First).IsValid());
			const FGuid Next = CreateSquad(Test, Registry, {First, Second});
			Test.TestTrue(TEXT("New group does not reuse previous handle"), Next.IsValid() && Next != Squad);
			TStrongObjectPtr<UUnitLifecycleProbe> Probe(NewObject<UUnitLifecycleProbe>());
			Probe->Test = &Test; Probe->ObservedUnit = First; Probe->SquadObserved = Next;
			BindProbe(Test, First, TEXT("OnOwnerFactionChanged"), Probe.Get(), TEXT("HandleSquadOwner"));
			SetOwner(Test, First, EFactionID::Carthage);
			Test.TestEqual(TEXT("Owner callback creates exactly one new group after detach"), Probe->SquadOwnerEvents, 1);
			const FGuid Reentered = UnitSquad(Test, Registry, First);
			Test.TestTrue(TEXT("Reentrant regroup is committed"), Reentered.IsValid() && Reentered != Next);
			Test.TestEqual(TEXT("Old group retains remaining member"), Registry->GetSquadMembers(Next).Num(), 1);
			Probe->SquadObserved = Reentered;
			BindProbe(Test, First, TEXT("OnUnitDied"), Probe.Get(), TEXT("HandleSquadDeath"));
			Health(Test, First, 0);
			Test.TestEqual(TEXT("Death observer saw retired singleton"), Probe->SquadDeathEvents, 1);
			Test.TestFalse(TEXT("Dead member cannot recreate group"), CreateSquad(Test, Registry, {First}).IsValid());
			TWeakObjectPtr<ACharacter> WeakSecond(Second);
			Second->Destroy();
			Test.TestTrue(TEXT("EndPlay empties final old group"), Registry->GetSquadMembers(Next).IsEmpty());
			Test.TestEqual(TEXT("Ended group has no faction"), Registry->GetSquadFaction(Next), EFactionID::None);
			CollectGarbage(RF_NoFlags);
			Test.TestFalse(TEXT("Squad registry does not retain destroyed member through GC"), WeakSecond.IsValid(true, true));
			Test.TestEqual(TEXT("Fixture squad cleanup restores baseline"), Registry->GetSquadCount(), Baseline);
			const TArray<AUnitBase*> BeforeTraining = Registry->GetUnitsForFaction(EFactionID::Rome);
			UClass* BarracksClass = LoadClass<AActor>(nullptr, TEXT("/Script/RomanEmpireGame.Barracks"));
			if (!Test.TestNotNull(TEXT("Training fixture class"), BarracksClass)) return true;
			FActorSpawnParameters BuildingParams;
			BuildingParams.ObjectFlags |= RF_Transient;
			BuildingParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Barracks = World->SpawnActor<AActor>(BarracksClass, FVector(450000.0, 450000.0, 300.0), FRotator::ZeroRotator, BuildingParams);
			if (!Test.TestNotNull(TEXT("Training fixture spawned"), Barracks)) return true;
			Fixtures.Add(Barracks); Barracks->SetActorTickEnabled(false);
			SetOwner(Test, Barracks, EFactionID::Rome);
			Command(Test, Barracks, TEXT("StartConstruction"));
			FCall BuildingData(Test, Barracks, TEXT("GetBuildingData"), 1);
			FStructProperty* Definition = BuildingData.Property<FStructProperty>(TEXT("ReturnValue"), true);
			if (!Test.TestTrue(TEXT("Building data type"), Definition && Definition->Struct == FBuildingData::StaticStruct())) return true;
			BuildingData.Execute();
			Barracks->Tick(Definition->ContainerPtrToValuePtr<FBuildingData>(BuildingData.Params.GetStructMemory())->ConstructionTime + 0.1f);
			FCall Train(Test, Barracks, TEXT("TrainUnit"), 2);
			Train.Set<FNameProperty>(TEXT("UnitID"), FName(TEXT("Legionary"))); Train.Execute();
			if (!Test.TestTrue(TEXT("Training starts through canonical producer"), Train.Read<FBoolProperty, bool>())) return true;
			FCall Remaining(Test, Barracks, TEXT("GetTrainingTimeRemaining"), 1); Remaining.Execute();
			Barracks->Tick(Remaining.Read<FFloatProperty, float>() + 0.1f);
			const TArray<AUnitBase*> AfterTraining = Registry->GetUnitsForFaction(EFactionID::Rome);
			Test.TestEqual(TEXT("Training registers exactly one new unit"), AfterTraining.Num(), BeforeTraining.Num() + 1);
			for (AUnitBase* Unit : AfterTraining) if (!BeforeTraining.Contains(Unit))
			{
				const FGuid RecruitedSquad = Registry->GetUnitSquad(Unit);
				Test.TestTrue(TEXT("Trained unit receives tactical membership"), RecruitedSquad.IsValid());
				Test.TestEqual(TEXT("Recruit starts as a singleton"), Registry->GetSquadMembers(RecruitedSquad).Num(), 1);
			}

			UWorld* OtherWorld = UWorld::CreateWorld(EWorldType::Game, false);
			if (!Test.TestNotNull(TEXT("Isolation world created"), OtherWorld)) return true;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(OtherWorld);
			OtherWorld->SetGameInstance(World->GetGameInstance()); OtherWorld->InitializeActorsForPlay(FURL());
			URomanUnitRegistrySubsystem* OtherRegistry = OtherWorld->GetSubsystem<URomanUnitRegistrySubsystem>();
			bool bOtherWorldDestroyed = false;
			ON_SCOPE_EXIT { if (!bOtherWorldDestroyed) OtherWorld->DestroyWorld(false); GEngine->DestroyWorldContext(OtherWorld); CollectGarbage(RF_NoFlags); };
			ACharacter* Foreign = Spawn(EFactionID::Rome, true, OtherWorld);
			if (!OtherRegistry || !Foreign) return true;
			Foreign->DispatchBeginPlay();
			Test.TestFalse(TEXT("Foreign world unit rejected"), CreateSquad(Test, Registry, {Foreign}).IsValid());
			const FGuid ForeignSquad = CreateSquad(Test, OtherRegistry, {Foreign});
			Test.TestTrue(TEXT("Own-world group created"), ForeignSquad.IsValid());
			Test.TestTrue(TEXT("Squad handles cannot resolve in another world"), Registry->GetSquadMembers(ForeignSquad).IsEmpty());
			OtherWorld->DestroyWorld(false);
			bOtherWorldDestroyed = true;
			Test.TestEqual(TEXT("Deinitialize clears group count"), OtherRegistry->GetSquadCount(), 0);
			Test.TestTrue(TEXT("Deinitialize clears members"), OtherRegistry->GetSquadMembers(ForeignSquad).IsEmpty());
			Test.TestFalse(TEXT("Deinitialized registry rejects writes"), CreateSquad(Test, OtherRegistry, {Foreign}).IsValid());
			return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
	};
	class FWaitForUnitPIEEnd : public IAutomationLatentCommand
	{
	public:
		explicit FWaitForUnitPIEEnd(FAutomationTestBase* InTest) : Test(*InTest), StartedAt(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld) return true;
			if (FPlatformTime::Seconds() - StartedAt < 60.0) return false;
			Test.AddError(TEXT("Unit test PIE teardown timed out")); return true;
		}
	private:
		FAutomationTestBase& Test;
		double StartedAt;
	};
}

void UUnitLifecycleProbe::HandleSquadOwner(AUnitBase* Unit)
{
	if (!Test || !ObservedUnit.IsValid()) return;
	URomanUnitRegistrySubsystem* Registry = ObservedUnit->GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>();
	if (!Test->TestNotNull(TEXT("Squad owner callback registry"), Registry)) return;
	++SquadOwnerEvents;
	Test->TestFalse(TEXT("Owner observer sees prior membership removed"), Registry->GetUnitSquad(Unit).IsValid());
	Test->TestEqual(TEXT("Owner observer sees old group's remaining member"), Registry->GetSquadMembers(SquadObserved).Num(), 1);
	Test->TestTrue(TEXT("Owner observer can regroup committed new ownership"), Registry->CreateSquad({ Unit }).IsValid());
}

void UUnitLifecycleProbe::HandleSquadDeath(AUnitBase* Unit)
{
	if (!Test || !ObservedUnit.IsValid()) return;
	URomanUnitRegistrySubsystem* Registry = ObservedUnit->GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>();
	if (!Test->TestNotNull(TEXT("Squad death callback registry"), Registry)) return;
	++SquadDeathEvents;
	Test->TestFalse(TEXT("Death observer sees reverse membership cleared"), Registry->GetUnitSquad(Unit).IsValid());
	Test->TestTrue(TEXT("Death observer sees last-member group retired"), Registry->GetSquadMembers(SquadObserved).IsEmpty());
	Test->TestFalse(TEXT("Death observer cannot recreate group with dying actor"), Registry->CreateSquad({ Unit }).IsValid());
}

void UUnitLifecycleProbe::HandleDeath(AUnitBase* Unit)
{
	++DeathEvents;
	if (!Test || DeathEvents != 1 || !ObservedUnit.IsValid()) return;
	using namespace RomanEmpireUnitTests;
	AActor* Actor = ObservedUnit.Get();
	Test->TestTrue(TEXT("Death payload identifies the observed unit"),
		static_cast<const void*>(Unit) == static_cast<const void*>(Actor));
	Test->TestEqual(TEXT("Observer sees committed death"), Life(*Test, Actor), EUnitLifeState::Dead);
	CheckQuiescent(*Test, Actor);
	Test->TestTrue(TEXT("Observer sees scheduled cleanup"), Actor->GetLifeSpan() > 0.0f);
	const float BeforeStamina = Stamina(*Test, Actor);
	Health(*Test, Actor, 999);
	Damage(*Test, Actor, 999.0f);
	Move(*Test, Actor);
	Command(*Test, Actor, TEXT("PerformAttack"));
	Command(*Test, Actor, TEXT("ThrowPilum"));
	Command(*Test, Actor, TEXT("StartBlocking"));
	Command(*Test, Actor, TEXT("ActivateTestudo"));
	SetBool(*Test, Actor, TEXT("SetSelected"), TEXT("bNewSelected"), true);
	SetBool(*Test, Actor, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), true);
	Test->TestFalse(TEXT("Death callback cannot reinitialize"), Initialize(*Test, Actor, TEXT("Legionary"), EFactionID::Rome));
	Test->TestEqual(TEXT("Death callback cannot heal"), Int(*Test, Actor, TEXT("GetCurrentHealth")), 0);
	Test->TestEqual(TEXT("Death callback cannot consume stamina"), Stamina(*Test, Actor), BeforeStamina);
	CheckQuiescent(*Test, Actor);
}

void UUnitLifecycleProbe::HandleControllerCameraActivated(UActorComponent* Component, bool bReset)
{
	(void)Component; (void)bReset;
	RomanEmpireUnitTests::ApplyControllerProbe(this);
}

void UUnitLifecycleProbe::HandleControllerCameraDeactivated(UActorComponent* Component)
{
	(void)Component;
	if (!Test || bHandledControllerAction || !ObservedController.IsValid()) return;
	bHandledControllerAction = true;
	RomanEmpireUnitTests::Command(*Test, ObservedController.Get(), TEXT("ExitFirstPersonMode"));
	RomanEmpireUnitTests::ObjectCommand(*Test, ObservedController.Get(), TEXT("EnterFirstPersonMode"), TEXT("UnitToPossess"), ObservedUnit.Get());
}

void UUnitLifecycleProbe::HandleControllerSelection(const TArray<AUnitBase*>& Units, ABuildingBase* Building)
{
	(void)Units; (void)Building;
	RomanEmpireUnitTests::ApplyControllerProbe(this);
}

void UUnitLifecycleProbe::HandleRegistryOwner(AUnitBase* Unit)
{
	if (!Test || !ObservedUnit.IsValid()) return;
	AActor* Actor = ObservedUnit.Get();
	Test->TestTrue(TEXT("Registry owner payload identifies observed unit"), static_cast<const void*>(Unit) == static_cast<const void*>(Actor));
	URomanUnitRegistrySubsystem* Registry = Actor->GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>();
	if (!Test->TestNotNull(TEXT("Registry exists during owner notification"), Registry)) return;
	++RegistryOwnerEvents;
	const EFactionID Owner = RomanEmpireUnitTests::FInitializationSnapshot(*Test, Actor).Owner;
	Test->TestEqual(TEXT("Owner observer sees committed record count"), Registry->GetRegisteredUnitCount(), RegistryExpectedLiveCount);
	for (EFactionID Faction : { EFactionID::None, EFactionID::Rome, EFactionID::Carthage })
		Test->TestEqual(TEXT("Owner observer sees exactly the canonical faction bucket"),
			RomanEmpireUnitTests::SnapshotContains(Registry->GetUnitsForFaction(Faction), Actor), Faction == Owner);
	if (bRegistryReentered) return;
	bRegistryReentered = true;
	if (RegistryAction == EUnitControllerProbeAction::ChangeOwner) RomanEmpireUnitTests::SetOwner(*Test, Actor, EFactionID::Rome);
	else if (RegistryAction == EUnitControllerProbeAction::Kill) RomanEmpireUnitTests::Health(*Test, Actor, 0);
	else if (RegistryAction == EUnitControllerProbeAction::Destroy) Actor->Destroy();
}

void UUnitLifecycleProbe::HandleRegistryDeath(AUnitBase* Unit)
{
	(void)Unit;
	if (!Test || !ObservedUnit.IsValid()) return;
	++RegistryDeathEvents;
	AActor* Actor = ObservedUnit.Get();
	URomanUnitRegistrySubsystem* Registry = Actor->GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>();
	if (!Test->TestNotNull(TEXT("Registry exists during death notification"), Registry)) return;
	Test->TestEqual(TEXT("Death observer sees record already removed"), Registry->GetRegisteredUnitCount(), RegistryExpectedLiveCount - 1);
	Test->TestFalse(TEXT("Death observer cannot find corpse in registry"), Registry->ContainsUnit(Actor));
}

void UUnitLifecycleProbe::HandleVitals(AUnitBase* Unit)
{
	++VitalsEvents;
	if (!Test || !ObservedUnit.IsValid()) return;
	AActor* Actor = ObservedUnit.Get();
	Test->TestTrue(TEXT("Vitals payload identifies the observed unit"),
		static_cast<const void*>(Unit) == static_cast<const void*>(Actor));
	if (bReattackOnVitals && !bHandledVitalsReattack)
	{
		bHandledVitalsReattack = true;
		RomanEmpireUnitTests::Command(*Test, Actor, TEXT("PerformAttack"));
	}
	if (!bTerminateOnVitals || bHandledVitalsAction) return;
	bHandledVitalsAction = true; // Bound recursion before issuing any command.
	if (bDestroyDuringVitals)
	{
		Actor->Destroy();
	}
	else
	{
		RomanEmpireUnitTests::Health(*Test, Actor, 0);
	}
}

void UUnitLifecycleProbe::HandleDamage(AUnitBase* Unit, float Damage)
{
	++DamageEvents;
	if (!Test || DamageEvents != 1 || !ObservedUnit.IsValid() || !ActionUnit.IsValid()) return;
	using namespace RomanEmpireUnitTests;
	AActor* Target = ObservedUnit.Get();
	AActor* Attacker = ActionUnit.Get();
	Test->TestTrue(TEXT("Damage payload identifies observed target"), static_cast<const void*>(Unit) == static_cast<const void*>(Target));
	Test->TestTrue(TEXT("Damage observer receives positive committed damage"), Damage > 0.0f);
	RecoveryAtDamage = Recovery(*Test, Attacker);
	Command(*Test, Attacker, TEXT("PerformAttack"));
	switch (DamageAction)
	{
	case EUnitProbeDamageAction::MoveAttacker: Move(*Test, Attacker, Attacker->GetActorLocation() + FVector(0.0, 1000.0, 0.0)); break;
	case EUnitProbeDamageAction::HoldAttacker: Command(*Test, Attacker, TEXT("CommandHold")); break;
	case EUnitProbeDamageAction::KillAttacker: Health(*Test, Attacker, 0); break;
	case EUnitProbeDamageAction::DestroyAttacker: Attacker->Destroy(); break;
	case EUnitProbeDamageAction::KillTarget: Health(*Test, Target, 0); break;
	case EUnitProbeDamageAction::DestroyTarget: Target->Destroy(); break;
	case EUnitProbeDamageAction::None: break;
	}
}

void UUnitLifecycleProbe::HandleCameraActivated(UActorComponent* Component, bool bReset)
{
	++CameraActivationEvents;
	if (!Test || CameraActivationEvents != 1 || !ObservedUnit.IsValid()) return;
	using namespace RomanEmpireUnitTests;
	AActor* Actor = ObservedUnit.Get();
	Test->TestTrue(TEXT("Activation callback receives the observed unit's camera"), Component && Component->GetOwner() == Actor);
	Test->TestTrue(TEXT("Activation observer sees committed possession"), Bool(*Test, Actor, TEXT("IsPossessedByPlayer")));
	if (bKillOnCameraActivation) Health(*Test, Actor, 0);
	else
	{
		SetBool(*Test, Actor, TEXT("SetPossessedByPlayer"), TEXT("bPossessed"), false);
		Move(*Test, Actor, Actor->GetActorLocation() + FVector(1000.0, 0.0, 0.0));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitFoundationTest, "RomanEmpireGame.Units.FoundationPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitFoundationTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseUnits(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitOrdersCombatTest, "RomanEmpireGame.Units.OrdersCombatPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitOrdersCombatTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseUnitOrders(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitNavigationTest, "RomanEmpireGame.Units.NavigationPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitNavigationTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseNavigation(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitLoad50Test, "RomanEmpireGame.Performance.Load50PIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitLoad50Test::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseUnitLoadProfile(this, 50));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitLoad100Test, "RomanEmpireGame.Performance.Load100PIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitLoad100Test::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseUnitLoadProfile(this, 100));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitGroupCongestionTest, "RomanEmpireGame.Units.GroupCongestionPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitGroupCongestionTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseGroupCongestion(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitDisconnectedNavigationTest, "RomanEmpireGame.Units.DisconnectedNavigationPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitDisconnectedNavigationTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseDisconnectedNavigation(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitOwnershipTest, "RomanEmpireGame.Units.PlayerOwnershipPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitOwnershipTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseOwnership(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitAdditiveSelectionTest, "RomanEmpireGame.Units.AdditiveSelectionPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitAdditiveSelectionTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseAdditiveSelection(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitRegistryTest, "RomanEmpireGame.Units.LiveRegistryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitRegistryTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseUnitRegistry(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSquadRegistryTest, "RomanEmpireGame.Units.SquadRegistryPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSquadRegistryTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Committed test map loaded"), World && World->GetName() == TEXT("L_Testbed"))) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseSquadRegistry(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelectedCommandsTest, "RomanEmpireGame.Units.SelectedCommandsPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSelectedCommandsTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseSelectedCommands(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitFormationTest, "RomanEmpireGame.Units.FormationPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUnitFormationTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FExerciseFormation(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanEmpireUnitTests::FWaitForUnitPIEEnd(this));
	return true;
}
#endif
