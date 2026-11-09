// Private Editor fixture: real actor/graph + engine-skinned render-LOD vertices.
// CPU readback only; not a GPU transition, physical-input or player QA claim.
#include "Misc/AutomationTest.h"
#include "Animation/AnimMontage.h"
#include "AssetCompilingManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Faction/FactionData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Tests/AutomationEditorCommon.h"
#include "Units/RomanCharacterPresentation.h"
#include "Units/RomanSoldierAnimInstance.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanLODAnimation
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
		if (Test.TestNotNull(TEXT("Faction argument"), Property))
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
	}
	void Run() { if (Function && IsValid(Target)) Target->ProcessEvent(Function, Params.GetStructMemory()); }
};
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test;
	FString Output, Receipt;
	double Started = FPlatformTime::Seconds();
	int32 Phase = 0;
	TWeakObjectPtr<ACharacter> Units[3];
	TWeakObjectPtr<UAnimMontage> Montage;
	TArray<FVector3f> Previous[3];
	const bool SuppressAttack = FParse::Param(FCommandLine::Get(), TEXT("RomanLODAnimationSuppressAttack"));

	void Command(ACharacter* Unit, FName Name) { FCall Call(Test, Unit, Name); Call.Run(); }
	bool Setup(UWorld* World)
	{
		TStrongObjectPtr<URomanCharacterPresentation> Config(LoadObject<URomanCharacterPresentation>(nullptr,
			TEXT("/Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview")));
		if (!Test.TestTrue(TEXT("Actual private LOD01 bundle"), Config.IsValid() && Config->RomeMesh && Config->RomeMesh->GetLODNum() == 3)) return false;
		Montage = Config->AttackMontage;
		UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
		FObjectPropertyBase* Art = Class ? FindFProperty<FObjectPropertyBase>(Class, TEXT("CharacterPresentation")) : nullptr;
		if (!Test.TestNotNull(TEXT("Canonical native presentation bootstrap"), Art) || !Test.TestNotNull(TEXT("Canonical attack montage"), Montage.Get())) return false;
		for (int32 LOD = 0; LOD < 3; ++LOD)
		{
			FActorSpawnParameters Spawn; Spawn.bDeferConstruction = true; Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Unit = World->SpawnActor<ACharacter>(Class, FVector(440000, 440000 + LOD * 250, 96), FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Real actor for each render LOD"), Unit)) return false;
			Units[LOD] = Unit; Art->SetObjectPropertyValue_InContainer(Unit, Config.Get());
			FCall Init(Test, Unit, TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(EFactionID::Rome); Init.Run();
			Unit->FinishSpawning(Unit->GetActorTransform()); Unit->SetActorTickEnabled(false);
			Command(Unit, TEXT("CommandHold")); Unit->GetCharacterMovement()->DisableMovement();
			USkeletalMeshComponent* Mesh = Unit->GetMesh();
			if (!Test.TestTrue(TEXT("Actual LOD candidate on canonical actor"), Mesh->GetSkeletalMeshAsset() == Config->RomeMesh && Mesh->GetAnimInstance())) return false;
			Test.TestTrue(TEXT("Reuses IntegrationV3 graph"), Mesh->GetAnimInstance()->GetClass()->GetPathName().Contains(TEXT("/IntegrationV3/")));
			Test.TestEqual(TEXT("Capsule remains authority"), Unit->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 42.f);
			Test.TestEqual(TEXT("Skeletal mesh has no collision"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			Test.TestFalse(TEXT("No visual navigation authority"), Mesh->CanEverAffectNavigation());
			Mesh->SetComponentTickEnabled(false); Mesh->SetForcedLOD(LOD + 1); Mesh->UpdateLODStatus();
		}
		FAssetCompilingManager::Get().FinishAllCompilation();
		Receipt = TEXT("candidate=/Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview\nmethod=USkinnedMeshComponent::ComputeSkinnedPositions\n");
		Receipt += FString::Printf(TEXT("mutation_suppress_attack=%d\n"), int32(SuppressAttack));
		return true;
	}
	bool Sample(int32 LOD, FName Region, TArray<FVector3f>& Positions, TArray<uint32>& Witness)
	{
		USkeletalMeshComponent* Mesh = Units[LOD].IsValid() ? Units[LOD]->GetMesh() : nullptr;
		if (!Test.TestNotNull(TEXT("Live mesh for actual skinning"), Mesh)) return false;
		Mesh->UpdateLODStatus();
		if (!Test.TestEqual(TEXT("Requested actual animation LOD"), Mesh->GetPredictedLODLevel(), LOD)) return false;
		const FSkeletalMeshRenderData* Render = Mesh->GetSkeletalMeshRenderData();
		if (!Test.TestTrue(TEXT("Real render LOD data present"), Render && Render->LODRenderData.IsValidIndex(LOD))) return false;
		const FSkeletalMeshLODRenderData& Data = Render->LODRenderData[LOD];
		FSkinWeightVertexBuffer* Weights = Mesh->GetSkinWeightBuffer(LOD);
		if (!Test.TestNotNull(TEXT("Real render skin weights"), Weights)) return false;
		TArray<FMatrix44f> Matrices;
		Mesh->GetCurrentRefToLocalMatrices(Matrices, LOD);
		USkinnedMeshComponent::ComputeSkinnedPositions(Mesh, Positions, Matrices, Data, *Weights);
		if (!Test.TestEqual(TEXT("Actual skinned vertex count"), uint32(Positions.Num()), Data.GetNumVertices())) return false;
		bool Finite = true;
		for (const FVector3f& Position : Positions) Finite &= !Position.ContainsNaN() && Position.SizeSquared() < 1000000.f;
		if (!Test.TestTrue(TEXT("All deformed vertices finite and within ten metres in mesh space"), Finite)) return false;
		const FReferenceSkeleton& Skeleton = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
		const int32 Target = Skeleton.FindBoneIndex(Region);
		if (!Test.TestTrue(TEXT("Actual regional witness bone exists"), Target != INDEX_NONE)) return false;
		for (const FSkelMeshRenderSection& Section : Data.RenderSections)
			for (uint32 Local = 0; Local < Section.NumVertices; ++Local)
			{
				const uint32 Vertex = Section.BaseVertexIndex + Local;
				uint32 RegionalWeight = 0;
				for (uint32 Influence = 0; Influence < Weights->GetMaxBoneInfluences(); ++Influence)
				{
					const uint16 Weight = Weights->GetBoneWeight(Vertex, Influence);
					if (!Weight) continue;
					const int32 LocalBone = Weights->GetBoneIndex(Vertex, Influence);
					if (!Test.TestTrue(TEXT("Render bone palette index valid"), Section.BoneMap.IsValidIndex(LocalBone))) return false;
					int32 Bone = Section.BoneMap[LocalBone];
					while (Bone != INDEX_NONE && Bone != Target) Bone = Skeleton.GetParentIndex(Bone);
					if (Bone == Target) RegionalWeight += Weight;
				}
				if (RegionalWeight >= 16384) Witness.Add(Vertex);
			}
		return Test.TestTrue(TEXT("At least25 genuinely weighted regional vertices at every LOD"), Witness.Num() >= 25);
	}
	bool StepAndCompare(float Delta, FName Region, const TCHAR* Label, float Minimum, bool Hold = false)
	{
		for (int32 LOD = 0; LOD < 3; ++LOD)
		{
			ACharacter* Unit = Units[LOD].Get();
			USkeletalMeshComponent* Mesh = Unit ? Unit->GetMesh() : nullptr;
			if (!Test.TestNotNull(TEXT("Actual actor across frames"), Mesh)) return false;
			Mesh->TickAnimation(Delta, false); Mesh->RefreshBoneTransforms();
			TArray<FVector3f> Positions; TArray<uint32> Witness;
			if (!Sample(LOD, Region, Positions, Witness)) return false;
			if (!Previous[LOD].IsEmpty())
			{
				if (!Test.TestEqual(TEXT("Same vertex identity within each LOD across poses"), Positions.Num(), Previous[LOD].Num())) return false;
				float Maximum = 0; double SumSquares = 0; int32 Changed = 0;
				for (uint32 Vertex : Witness)
				{
					const float Distance = (Positions[Vertex] - Previous[LOD][Vertex]).Size();
					Maximum = FMath::Max(Maximum, Distance); SumSquares += Distance * Distance;
					if (Distance > Minimum) ++Changed;
				}
				const FString Check = FString::Printf(TEXT("LOD%d %s actual %s-weighted vertices deform"), LOD, Label, *Region.ToString());
				if (Hold) Test.TestTrue(Check + TEXT(" holds final pose"), Maximum < .1f);
				else Test.TestTrue(Check, Maximum > Minimum && Changed >= 25);
				Receipt += FString::Printf(TEXT("LOD%d state=%s region=%s vertices=%d witness=%d changed=%d max_delta_cm=%.6f rms_delta_cm=%.6f\n"),
					LOD, Label, *Region.ToString(), Positions.Num(), Witness.Num(), Changed, Maximum, FMath::Sqrt(SumSquares / Witness.Num()));
			}
			Previous[LOD] = MoveTemp(Positions);
		}
		return true;
	}
public:
	FExercise(FAutomationTestBase& T, FString O) : Test(T), Output(MoveTemp(O)) {}
	virtual ~FExercise() override { for (auto& Unit : Units) if (Unit.IsValid()) Unit->Destroy(); }
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 90) { Test.AddError(TEXT("Private LOD animation timeout")); return true; }
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!State || !State->IsGameplayInitialized()) return false;
		bool OK = true;
		switch (Phase)
		{
		case 0: OK = Setup(World); break;
		case 1: OK = StepAndCompare(.01f, TEXT("spine"), TEXT("IdleStart"), .01f); break;
		case 2:
			OK = StepAndCompare(.4f, TEXT("spine"), TEXT("Idle"), .1f);
			for (auto& Unit : Units) if (Unit.IsValid()) Unit->GetCharacterMovement()->Velocity = FVector(150, 0, 0); break;
		case 3: OK = StepAndCompare(.25f, TEXT("thigh_l"), TEXT("Walk"), 10.f); break;
		case 4:
			for (auto& Unit : Units) if (Unit.IsValid()) { Unit->GetCharacterMovement()->Velocity = FVector::ZeroVector; Command(Unit.Get(), TEXT("CommandHold")); }
			// Settle idle before the guard comparison, not a walk-to-idle witness.
			for (auto& Positions : Previous) Positions.Reset();
			OK = StepAndCompare(.2f, TEXT("forearm_l"), TEXT("BeforeBlock"), .5f);
			for (auto& Unit : Units) if (Unit.IsValid()) Command(Unit.Get(), TEXT("StartBlocking")); break;
		case 5: OK = StepAndCompare(.2f, TEXT("forearm_l"), TEXT("Block"), 10.f); break;
		case 6: OK = StepAndCompare(.6f, TEXT("forearm_l"), TEXT("BlockHold"), .5f, true); break;
		case 7:
			for (auto& Unit : Units) if (Unit.IsValid()) Command(Unit.Get(), TEXT("StopBlocking"));
			for (auto& Positions : Previous) Positions.Reset();
			OK = StepAndCompare(.2f, TEXT("forearm_r"), TEXT("BeforeAttack"), .5f);
			for (auto& Unit : Units) if (Unit.IsValid())
			{
				FCall Flag(Test, Unit.Get(), TEXT("SetPossessedByPlayer")); Flag.Set<FBoolProperty>(TEXT("bPossessed"), true); Flag.Run();
				Command(Unit.Get(), TEXT("PerformAttack"));
				Test.TestTrue(TEXT("Canonical attack starts actual montage"), Unit->GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage.Get()));
				// Explicit negative probe: transient actor only, never edits a montage/asset.
				if (SuppressAttack) Unit->GetMesh()->GetAnimInstance()->Montage_Stop(0.f, Montage.Get());
			}
			break;
		case 8:
			OK = StepAndCompare(.2f, TEXT("forearm_r"), TEXT("Attack"), 10.f);
			for (auto& Unit : Units) if (Unit.IsValid())
			{
				URomanSoldierAnimInstance* Anim = Cast<URomanSoldierAnimInstance>(Unit->GetMesh()->GetAnimInstance());
				Test.TestTrue(TEXT("Actual native attack timeline advances; AI does not enable FPS reach"), Anim && (SuppressAttack || Anim->Montage_GetPosition(Montage.Get()) > 0) && !Anim->bLocalFirstPerson);
				Command(Unit.Get(), TEXT("CommandStop"));
				FCall Kill(Test, Unit.Get(), TEXT("SetCurrentHealth")); Kill.Set<FIntProperty>(TEXT("NewHealth"), 0); Kill.Run();
			}
			break;
		case 9:
			for (auto& Positions : Previous) Positions.Reset();
			OK = StepAndCompare(.12f, TEXT("root"), TEXT("EarlyDeath"), 10.f); break;
		case 10: OK = StepAndCompare(1.f, TEXT("root"), TEXT("Death"), 10.f); break;
		case 11:
			OK = StepAndCompare(.5f, TEXT("root"), TEXT("DeathHold"), .1f, true);
			Receipt += TEXT("gpu_transitions_verified=false\nphysical_input_verified=false\nperformance_verified=false\nowner_approved=false\n");
			FAutomationTestExecutionInfo Execution; Test.GetExecutionInfo(Execution);
			Receipt += FString::Printf(TEXT("test_errors=%d\n"), Execution.GetErrorTotal());
			Test.TestTrue(TEXT("Create-only animation receipt saved"), FFileHelper::SaveStringToFile(Receipt, *Output));
			return true;
		}
		++Phase; return !OK;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanSemirealLODAnimation, "RomanPreview.LOD.Animation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanSemirealLODAnimation::RunTest(const FString& Parameters)
{
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); FPaths::NormalizeDirectoryName(Project);
	if (!TestTrue(TEXT("Exact private project only"), Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")))) return false;
	FString Name;
	if (!TestTrue(TEXT("Explicit fresh evidence filename"), FParse::Value(FCommandLine::Get(), TEXT("RomanLODAnimationReceipt="), Name) && Name.EndsWith(TEXT(".txt")) && !Name.Contains(TEXT("..")))) return false;
	for (TCHAR Character : Name) if (!TestTrue(TEXT("Safe receipt name"), FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-') || Character == TEXT('.'))) return false;
	const FString Output = FPaths::Combine(Project, Name);
	if (!TestFalse(TEXT("Do not overwrite evidence"), IFileManager::Get().FileExists(*Output))) return false;
	if (!TestNotNull(TEXT("Editor"), GEditor) || !TestNull(TEXT("Own fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanLODAnimation::FExercise(*this, Output)); AddCommand(new FEndPlayMapCommand());
	return true;
}
#endif
