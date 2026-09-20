// Copyright Roman Empire Game. All Rights Reserved.
#include "UnitLifecycleProbe.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Faction/FactionData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Units/UnitActivity.h"
#include "Units/UnitCombatState.h"
#include "Units/UnitOrderState.h"
#include "Units/UnitSelectionPresentation.h"
#include "Units/UnitVitalState.h"
#include "Units/UnitTypes.h"
#include "UObject/Class.h"
#include "UObject/GarbageCollection.h"
#include "UObject/ScriptDelegates.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
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
	void Damage(FAutomationTestBase& Test, UObject* Unit, float Value)
	{
		FCall Call(Test, Unit, TEXT("TakeCombatDamage"), 3);
		Call.Set<FFloatProperty>(TEXT("Damage"), Value);
		Call.Set<FBoolProperty>(TEXT("bIsRanged"), false);
		// DamageSource is initialized to null by FStructOnScope.
		Call.Property<FObjectPropertyBase>(TEXT("DamageSource"));
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
			CheckTarget(Unit, Enemy);
			Test.TestEqual(TEXT("Pursuit updates destination without replacing attack"), Order(Test, Unit), EUnitOrderType::Attack);
			Test.TestEqual(TEXT("Pursuit follows target's current location"), StoredState<FUnitOrderState>(Test, Unit, TEXT("Orders")).GetDestination(), Enemy->GetActorLocation());
			Test.TestFalse(TEXT("Pursuit actually advances position"), Unit->GetActorLocation().Equals(Origin));
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
			Test.TestEqual(TEXT("AI first hit deals actual damage once"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 1);
			Test.TestEqual(TEXT("AI hit starts full recovery without charging current frame twice"), Recovery(Test, Fighter), 0.5f);
			Test.TestEqual(TEXT("AI recovery derives attacking activity"), Activity(Test, Fighter), EUnitActivity::Attacking);
			Test.TestTrue(TEXT("AI attack marks combat in progress"), Bool(Test, Fighter, TEXT("IsAttacking")));
			Test.TestFalse(TEXT("In-range attack pauses movement"), Bool(Test, Fighter, TEXT("HasMoveCommand")));
			CheckTarget(Fighter, Defender);
			for (int32 Index = 1; Index <= 3; ++Index)
			{
				Fighter->Tick(0.125f);
				Test.TestEqual(TEXT("AI recovery advances exactly once per explicit tick"), Recovery(Test, Fighter), 0.5f - Index * 0.125f);
				Test.TestEqual(TEXT("AI cannot strike again before complete recovery"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 1);
			}
			Fighter->Tick(0.125f);
			Test.TestEqual(TEXT("AI strikes once when recovery reaches zero"), Int(Test, Defender, TEXT("GetCurrentHealth")), BeforeHealth - 2);
			Test.TestEqual(TEXT("Second AI strike starts one full recovery"), Recovery(Test, Fighter), 0.5f);

			ACharacter* PlayerUnit = Spawn(EFactionID::Rome, FVector(140000.0, 0.0, 400.0));
			if (!PlayerUnit) return true;
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
				Test.TestEqual(TEXT("AI hit notifies target once despite nested direct attack"), DamageProbe->DamageEvents, 1);
				Test.TestEqual(TEXT("Damage observer sees already-committed full recovery"), DamageProbe->RecoveryAtDamage, 0.5f);
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
					Test.TestEqual(TEXT("Nested order change cannot charge recovery twice"), Recovery(Test, Attacker), 0.5f);
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
#endif
