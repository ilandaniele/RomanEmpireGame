// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Faction/FactionData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Tests/AutomationEditorCommon.h"
#include "Units/RomanCharacterPresentation.h"
#include "Units/RomanSoldierAnimInstance.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanMaterialPolicy
{
struct FCall
{
	FAutomationTestBase& Test; UObject* Target; UFunction* Function; FStructOnScope Params;
	FCall(FAutomationTestBase& T, UObject* O, FName Name)
		: Test(T), Target(O), Function(O ? O->FindFunction(Name) : nullptr), Params(Function) { Test.TestNotNull(Name.ToString(), Function); }
	template<typename P, typename V> void Set(FName Name, V Value)
	{
		P* Property = Function ? FindFProperty<P>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Faction(EFactionID Value)
	{
		FEnumProperty* Property = Function ? FindFProperty<FEnumProperty>(Function, TEXT("NewOwner")) : nullptr;
		if (Test.TestNotNull(TEXT("Native faction argument"), Property))
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
	}
	void Run() { if (Function && IsValid(Target)) Target->ProcessEvent(Function, Params.GetStructMemory()); }
};
struct FReleased { TWeakObjectPtr<ACharacter> Units[2]; TWeakObjectPtr<UMaterialInstanceDynamic> MID; };
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FReleased> Released; double Started = FPlatformTime::Seconds();
	int32 PosePhase = 0;
	TWeakObjectPtr<URomanCharacterPresentation> ConfigForPoses;
	FQuat IdleSpine, IdleLeg, IdleGuardArm, GuardArm, BeforeAttack;
	FTransform EarlyDeath, FinalDeath;
	bool PoseStep()
	{
		ACharacter* Unit = Released->Units[0].Get();
		USkeletalMeshComponent* Mesh = Unit ? Unit->GetMesh() : nullptr;
		URomanSoldierAnimInstance* Anim = Mesh ? Cast<URomanSoldierAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
		if (!Test.TestTrue(TEXT("Live canonical actor/graph/config across frames"), Unit && Mesh && Anim && ConfigForPoses.IsValid())) return true;
		const auto Step = [&](float Delta) { Mesh->TickAnimation(Delta, false); Mesh->RefreshBoneTransforms(); };
		const auto Bone = [&](FName Name) { return Mesh->GetBoneQuaternion(Name, EBoneSpaces::ComponentSpace); };
		const auto Command = [&](FName Name) { FCall Call(Test, Unit, Name); Call.Run(); };
		switch (PosePhase)
		{
		case 1:
			Mesh->SetComponentTickEnabled(false); Step(.01f); IdleSpine = Bone(TEXT("spine")); break;
		case 2:
			Step(.4f); Test.TestFalse(TEXT("Actual native graph deforms idle"), Bone(TEXT("spine")).Equals(IdleSpine, .0001f));
			IdleLeg = Bone(TEXT("thigh_l")); Unit->GetCharacterMovement()->Velocity = FVector(150, 0, 0); break;
		case 3:
			Step(.25f); Test.TestTrue(TEXT("Movement derived from canonical velocity"), Anim->bMoving);
			Test.TestFalse(TEXT("Actual walking graph deforms leg"), Bone(TEXT("thigh_l")).Equals(IdleLeg, .01f));
			Unit->GetCharacterMovement()->Velocity = FVector::ZeroVector; IdleGuardArm = Bone(TEXT("forearm_l")); Command(TEXT("StartBlocking")); break;
		case 4:
			Step(.2f); Test.TestTrue(TEXT("Canonical guard reaches actual graph"), Anim->bBlocking); GuardArm = Bone(TEXT("forearm_l"));
			Test.TestFalse(TEXT("Actual guard graph raises shield arm"), GuardArm.Equals(IdleGuardArm, .01f)); break;
		case 5:
			Step(.6f); Test.TestTrue(TEXT("Guard remains held instead of falling back"), Bone(TEXT("forearm_l")).Equals(GuardArm, .005f));
			Command(TEXT("StopBlocking"));
			{ FCall Possess(Test, Unit, TEXT("SetPossessedByPlayer")); Possess.Set<FBoolProperty>(TEXT("bPossessed"), true); Possess.Run(); }
			BeforeAttack = Bone(TEXT("forearm_r")); Command(TEXT("PerformAttack"));
			Test.TestTrue(TEXT("Actual montage has playable sections"), ConfigForPoses->AttackMontage && !ConfigForPoses->AttackMontage->CompositeSections.IsEmpty());
			Test.TestTrue(TEXT("Canonical attack starts candidate montage"), Anim->Montage_IsPlaying(ConfigForPoses->AttackMontage)); break;
		case 6:
			Step(.2f); Test.TestTrue(TEXT("Actual attack timeline advances"), Anim->Montage_GetPosition(ConfigForPoses->AttackMontage) > 0.f);
			Test.TestFalse(TEXT("AI possession flag is not a local player camera"), Anim->bLocalFirstPerson);
			Test.TestTrue(TEXT("Non-player snapshot has no first-person IK targets"), Anim->FirstPersonHandTarget.IsNearlyZero() && Anim->FirstPersonElbowTarget.IsNearlyZero());
			Test.TestFalse(TEXT("Actual new attack montage deforms sword arm"), Bone(TEXT("forearm_r")).Equals(BeforeAttack, .01f));
			Command(TEXT("CommandStop")); Step(.1f); Test.TestFalse(TEXT("Canonical cancellation stops montage"), Anim->Montage_IsPlaying(ConfigForPoses->AttackMontage)); break;
		case 7:
			{ FCall Kill(Test, Unit, TEXT("SetCurrentHealth")); Kill.Set<FIntProperty>(TEXT("NewHealth"), 0); Kill.Run(); }
			Step(.12f); Test.TestTrue(TEXT("Canonical death reaches actual graph"), Anim->bDead); Test.TestFalse(TEXT("Death disables actor collision"), Unit->GetActorEnableCollision());
			EarlyDeath = Mesh->GetBoneTransform(Mesh->GetBoneIndex(TEXT("root"))); break;
		case 8:
			Step(1.f); FinalDeath = Mesh->GetBoneTransform(Mesh->GetBoneIndex(TEXT("root")));
			Test.TestFalse(TEXT("Actual death graph deforms root"), FinalDeath.Equals(EarlyDeath, .001f)); break;
		case 9:
			Step(.5f); Test.TestTrue(TEXT("Actual nonlooping death holds final pose"), Mesh->GetBoneTransform(Mesh->GetBoneIndex(TEXT("root"))).Equals(FinalDeath, .005f));
			Unit->Destroy(); if (Released->Units[1].IsValid()) Released->Units[1]->Destroy(); return true;
		}
		++PosePhase; return false;
	}
public:
	FExercise(FAutomationTestBase* T, TSharedRef<FReleased> R) : Test(*T), Released(R) {}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 30) { Test.AddError(TEXT("Material policy PIE timeout")); return true; }
		if (PosePhase > 0) return PoseStep();
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!State || !State->IsGameplayInitialized()) return false;
		TStrongObjectPtr<URomanCharacterPresentation> Original(LoadObject<URomanCharacterPresentation>(nullptr, TEXT("/Game/Characters/RomanSoldier/DA_RomanCharacterPresentation")));
		if (!Test.TestNotNull(TEXT("Actual legacy default bundle"), Original.Get())) return true;
		Test.TestFalse(TEXT("Existing schema1 remains default, no silent art migration"), Original->bUseMeshMaterialParents);
		FString Candidate;
		const bool Semireal = FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealPresentation="), Candidate);
		if (Semireal && !Test.TestTrue(TEXT("Candidate only in exact private project/namespace"),
			FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()).Contains(TEXT("/Artifacts/VQ01_Semireal1/GamePreview/"))
			&& Candidate.StartsWith(TEXT("/Game/ArtCandidates/Attempt10_1/Runtime")) && !Candidate.Contains(TEXT("..")))) return true;
		URomanCharacterPresentation* Source = Semireal ? LoadObject<URomanCharacterPresentation>(nullptr, *Candidate) : Original.Get();
		if (!Test.TestNotNull(TEXT("Selected source bundle"), Source)) return true;
		TStrongObjectPtr<URomanCharacterPresentation> Config(DuplicateObject<URomanCharacterPresentation>(Source, GetTransientPackage()));
		Config->bUseMeshMaterialParents = true; // Unsaved fixture copy; never mutate shared defaults.
		UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
		if (!Test.TestNotNull(TEXT("Native canonical unit class"), Class)) return true;
		FObjectPropertyBase* Art = FindFProperty<FObjectPropertyBase>(Class, TEXT("CharacterPresentation"));
		if (!Test.TestNotNull(TEXT("Reflected immutable presentation bootstrap"), Art)) return true;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			FActorSpawnParameters Spawn; Spawn.bDeferConstruction = true; Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Unit = World->SpawnActor<ACharacter>(Class, FVector(440000, 440000 + Index * 250, 96), FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Actual isolated actor"), Unit)) return true;
			Released->Units[Index] = Unit; Art->SetObjectPropertyValue_InContainer(Unit, Config.Get());
			FCall Init(Test, Unit, TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(EFactionID::Rome); Init.Run();
			Unit->FinishSpawning(Unit->GetActorTransform()); Unit->SetActorTickEnabled(false);
			FCall Hold(Test, Unit, TEXT("CommandHold")); Hold.Run(); Unit->GetCharacterMovement()->DisableMovement();
			Test.TestEqual(TEXT("Capsule remains collision authority"), Unit->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 42.f);
			Test.TestEqual(TEXT("Visual has no collision"), Unit->GetMesh()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			Test.TestFalse(TEXT("Visual does not change navigation"), Unit->GetMesh()->CanEverAffectNavigation());
		}
		ACharacter* Unit = Released->Units[0].Get(); ACharacter* Other = Released->Units[1].Get();
		USkeletalMeshComponent* Mesh = Unit->GetMesh();
		const auto Verify = [&](EFactionID Owner, bool Damaged)
		{
			const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(World);
			const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(Owner) : nullptr;
			if (!Test.TestNotNull(TEXT("Canonical faction definition"), Faction)) return;
			const TArray<FSkeletalMaterial>& Slots = Mesh->GetSkeletalMeshAsset()->GetMaterials();
			for (int32 Index = 0; Index < Slots.Num(); ++Index)
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Index));
				if (!Test.TestNotNull(TEXT("Every authored slot has a real actor MID"), MID)) continue;
				Test.TestTrue(TEXT("Exact authored parent retained, no flat override"), MID->Parent == Slots[Index].MaterialInterface);
				Test.TestTrue(TEXT("MID belongs to this actor, never shared"), MID->GetOuter() == Unit && Other->GetMesh()->GetMaterial(Index) != MID);
				const bool Tint = Config->FactionTintSlots.Contains(Slots[Index].MaterialSlotName);
				FLinearColor Color = Tint ? Faction->Definition.FactionColor : FLinearColor::White;
				if (Tint && Slots[Index].MaterialSlotName == TEXT("Shield")) Color *= .6f; Color.A = 1.f;
				Test.TestTrue(TEXT("Semantic tint preserves non-faction skin/metal/eyes"), MID->K2_GetVectorParameterValue(TEXT("Color")).Equals(Color, .001f));
				Test.TestTrue(TEXT("Committed canonical health drives every slot"), Damaged ? MID->K2_GetScalarParameterValue(TEXT("Damage")) > 0.f : FMath::IsNearlyZero(MID->K2_GetScalarParameterValue(TEXT("Damage"))));
				if (Semireal) Test.TestEqual(TEXT("Real shader semantic tint switch"), MID->K2_GetScalarParameterValue(TEXT("FactionTintStrength")), Tint ? 1.f : 0.f);
			}
		};
		Verify(EFactionID::Rome, false); Released->MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		FCall Select(Test, Unit, TEXT("SetSelected")); Select.Set<FBoolProperty>(TEXT("bNewSelected"), true); Select.Run();
		Test.TestTrue(TEXT("Selection does not replace authored body MID"), Mesh->GetMaterial(0) == Released->MID.Get());
		FCall Owner(Test, Unit, TEXT("SetOwnerFaction")); Owner.Faction(EFactionID::Carthage); Owner.Run(); Verify(EFactionID::Carthage, false);
		FCall Health(Test, Unit, TEXT("SetCurrentHealth")); Health.Set<FIntProperty>(TEXT("NewHealth"), 50); Health.Run(); Verify(EFactionID::Carthage, true);
		UMaterialInstanceDynamic* OtherMID = Cast<UMaterialInstanceDynamic>(Other->GetMesh()->GetMaterial(0));
		if (Test.TestNotNull(TEXT("Other actor retains its own MID"), OtherMID))
			Test.TestTrue(TEXT("Damage is actor local"), FMath::IsNearlyZero(OtherMID->K2_GetScalarParameterValue(TEXT("Damage"))));
		// Separate world frames are essential: montage slot update counters are
		// frame-synchronized. Repeated manual ticks in one frame are not gameplay.
		ConfigForPoses = Config.Get(); PosePhase = 1; return false;
	}
};
class FTeardown : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FReleased> Released;
	double Started = FPlatformTime::Seconds();
public:
	FTeardown(FAutomationTestBase* T, TSharedRef<FReleased> R) : Test(*T), Released(R) {}
	virtual bool Update() override
	{
		if (GEditor->PlayWorld && FPlatformTime::Seconds() - Started < 20) return false;
		Test.TestNull(TEXT("Private PIE ended"), GEditor->PlayWorld); CollectGarbage(RF_NoFlags);
		for (const auto& Unit : Released->Units) Test.TestFalse(TEXT("Actual unit collected"), Unit.IsValid());
		Test.TestFalse(TEXT("No retained actor material"), Released->MID.IsValid()); return true;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanCharacterMaterialPolicy, "RomanEmpireGame.Characters.MaterialPolicyPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanCharacterMaterialPolicy::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor"), GEditor) || !TestNull(TEXT("Fresh owned PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	TSharedRef<RomanMaterialPolicy::FReleased> Released = MakeShared<RomanMaterialPolicy::FReleased>();
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanMaterialPolicy::FExercise(this, Released));
	AddCommand(new FEndPlayMapCommand()); AddCommand(new RomanMaterialPolicy::FTeardown(this, Released)); return true;
}
#endif
