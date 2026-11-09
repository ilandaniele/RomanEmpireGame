// Private preview fixture only: copied into the disposable Editor test module.
// Never part of the project's runtime, Shipping, or normal automation suite.
#include "Misc/AutomationTest.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_SequenceEvaluator.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "K2Node_VariableGet.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "Units/RomanCharacterPresentation.h"
#include "Units/RomanSoldierAnimInstance.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanSemirealPreviewAuthoring
{
const FString Candidate(TEXT("/Game/ArtCandidates/Attempt10_1/"));
const FString Output = Candidate + TEXT("IntegrationV3/");
const FName Tag(TEXT("RomanSemirealPreviewSchema"));
template<typename T> T* Node(UEdGraph* Graph, int32 X, int32 Y)
{
	FGraphNodeCreator<T> Creator(*Graph); T* Result = Creator.CreateNode();
	Result->NodePosX = X; Result->NodePosY = Y; Creator.Finalize(); return Result;
}
bool Link(FAutomationTestBase& Test, UEdGraph* Graph, UEdGraphNode* From, const TCHAR* Out, UEdGraphNode* To, const TCHAR* In)
{
	UEdGraphPin* A = From->FindPin(Out, EGPD_Output); UEdGraphPin* B = To->FindPin(In, EGPD_Input);
	if (!A || !B)
	{
		for (const UEdGraphNode* N : {From, To}) for (const UEdGraphPin* P : N->Pins) Test.AddInfo(P->PinName.ToString());
		Test.AddError(FString::Printf(TEXT("Missing actual IK graph pin %s -> %s"), Out, In)); return false;
	}
	return Test.TestTrue(TEXT("Actual IK graph schema link"), Graph->GetSchema()->TryCreateConnection(A, B));
}
bool AddFirstPersonReach(FAutomationTestBase& Test, UAnimBlueprint* BP)
{
	UEdGraph* Graph = nullptr;
	for (UEdGraph* G : BP->FunctionGraphs) if (G->GetFName() == TEXT("AnimGraph")) Graph = G;
	if (!Test.TestNotNull(TEXT("Candidate graph"), Graph)) return false;
	UAnimGraphNode_Slot* Slot = nullptr;
	for (UEdGraphNode* N : Graph->Nodes) if (UAnimGraphNode_Slot* S = Cast<UAnimGraphNode_Slot>(N)) Slot = S;
	UEdGraphPin* SlotPose = Slot ? Slot->FindPin(TEXT("Pose"), EGPD_Output) : nullptr;
	if (!Test.TestTrue(TEXT("Exact pre-death montage output"), SlotPose && SlotPose->LinkedTo.Num() == 1)) return false;
	UEdGraphPin* Destination = SlotPose->LinkedTo[0]; SlotPose->BreakLinkTo(Destination);
	auto* ToComponent = Node<UAnimGraphNode_LocalToComponentSpace>(Graph, -240, -180);
	auto* IK = Node<UAnimGraphNode_TwoBoneIK>(Graph, 20, -180);
	auto* ToLocal = Node<UAnimGraphNode_ComponentToLocalSpace>(Graph, 300, -180);
	IK->Node.IKBone.BoneName = TEXT("hand_r");
	IK->Node.EffectorLocationSpace = BCS_ComponentSpace; IK->Node.JointTargetLocationSpace = BCS_ComponentSpace;
	IK->Node.bAllowStretching = false; IK->Node.bMaintainEffectorRelRot = true;
	IK->Node.AlphaInputType = EAnimAlphaInputType::Bool; IK->ReconstructNode();
	const TCHAR* Variables[] = {TEXT("bLocalFirstPerson"), TEXT("FirstPersonHandTarget"), TEXT("FirstPersonElbowTarget")};
	const TCHAR* Inputs[] = {TEXT("bAlphaBoolEnabled"), TEXT("EffectorLocation"), TEXT("JointTargetLocation")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Getter = Node<UK2Node_VariableGet>(Graph, -300, -450 - 120 * Index);
		Getter->VariableReference.SetSelfMember(Variables[Index]); Getter->ReconstructNode();
		if (!Link(Test, Graph, Getter, Variables[Index], IK, Inputs[Index])) return false;
	}
	return Link(Test, Graph, Slot, TEXT("Pose"), ToComponent, TEXT("LocalPose"))
		&& Link(Test, Graph, ToComponent, TEXT("ComponentPose"), IK, TEXT("ComponentPose"))
		&& Link(Test, Graph, IK, TEXT("Pose"), ToLocal, TEXT("ComponentPose"))
		&& Link(Test, Graph, ToLocal, TEXT("Pose"), Destination->GetOwningNode(), *Destination->PinName.ToString());
}
bool Save(FAutomationTestBase& Test, UObject* Asset)
{
	UPackage* Package = Asset->GetOutermost();
	Package->GetMetaData().SetValue(Asset, Tag, TEXT("1"));
	Asset->MarkPackageDirty(); FAssetRegistryModule::AssetCreated(Asset);
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
	return Test.TestTrue(TEXT("New private asset saved"), UPackage::SavePackage(Package, Asset,
		*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanSemirealPreviewAuthoring, "RomanEmpireGame.ArtCandidates.SemirealV2Authoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanSemirealPreviewAuthoring::RunTest(const FString& Parameters)
{
	using namespace RomanSemirealPreviewAuthoring;
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(Project);
	if (!TestTrue(TEXT("Exact disposable project only"), Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")))
		|| !TestTrue(TEXT("Verified snapshot receipt exists"), IFileManager::Get().FileExists(*(Project + TEXT("/game-context-readback01.json"))))) return false;
	TStrongObjectPtr<USkeletalMesh> Mesh(LoadObject<USkeletalMesh>(nullptr, *(Candidate + TEXT("SK_Roman_Rome_Semireal01"))));
	if (!TestNotNull(TEXT("Real selected imported mesh"), Mesh.Get())) return false;
	USkeleton* Skeleton = Mesh->GetSkeleton();
	if (!TestNotNull(TEXT("Candidate's own bind pose"), Skeleton)) return false;
	TStrongObjectPtr<UAnimBlueprint> Original(LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Characters/RomanSoldier/ABP_RomanSoldier")));
	if (!TestNotNull(TEXT("Existing verified canonical graph"), Original.Get())) return false;
	if (!TestTrue(TEXT("No old bind pose reuse"), Original->TargetSkeleton != Skeleton)) return false;
	TStrongObjectPtr<UAnimSequence> Attack(LoadObject<UAnimSequence>(nullptr, *(Candidate + TEXT("Attack"))));
	if (!TestNotNull(TEXT("Candidate attack clip"), Attack.Get()) || !TestTrue(TEXT("Candidate attack rig"), Attack->GetSkeleton() == Skeleton)) return false;
	const TCHAR* Names[] = {TEXT("ABP_SemirealPreview"), TEXT("AM_SemirealAttack"), TEXT("DA_SemirealPreview")};
	int32 Existing = 0;
	for (const TCHAR* Name : Names) Existing += FPackageName::DoesPackageExist(Output + Name) ? 1 : 0;
	if (!TestTrue(TEXT("Refuse partial or foreign output bundle"), Existing == 0 || Existing == 3)) return false;
	TStrongObjectPtr<UAnimBlueprint> BP;
	TStrongObjectPtr<UAnimMontage> Montage;
	TStrongObjectPtr<URomanCharacterPresentation> Config;
	if (Existing == 0)
	{
		if (!TestTrue(TEXT("Authoring requires explicit private opt-in"), FParse::Param(FCommandLine::Get(), TEXT("RomanAuthorSemirealPreview")))) return false;
		BP.Reset(DuplicateObject<UAnimBlueprint>(Original.Get(), CreatePackage(*(Output + Names[0])), Names[0]));
		if (!TestNotNull(TEXT("Independent actual graph duplicate"), BP.Get())) return false;
		BP->SetFlags(RF_Public | RF_Standalone); BP->TargetSkeleton = Skeleton; BP->SetPreviewMesh(Mesh.Get());
		int32 Replaced = 0;
		for (UEdGraph* Graph : BP->FunctionGraphs) for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_SequencePlayer* Player = Cast<UAnimGraphNode_SequencePlayer>(Node);
			UAnimGraphNode_SequenceEvaluator* Evaluator = Cast<UAnimGraphNode_SequenceEvaluator>(Node);
			UAnimSequenceBase* Previous = Player ? Player->Node.GetSequence() : Evaluator ? Evaluator->Node.GetSequence() : nullptr;
			if (!Previous) continue;
			FString ClipName = Previous->GetName();
			if (!TestTrue(TEXT("Only known original clip reference may be replaced"), ClipName.RemoveFromStart(TEXT("A_Roman")))) return false;
			UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *(Candidate + ClipName));
			if (!TestNotNull(TEXT("Actual new clip for compiled graph node"), Clip) || !TestTrue(TEXT("New clip rig"), Clip->GetSkeleton() == Skeleton)) return false;
			if (Player) Player->Node.SetSequence(Clip); else Evaluator->Node.SetSequence(Clip);
			Node->ReconstructNode(); ++Replaced;
		}
		if (!TestEqual(TEXT("All four locomotion/guard/death graph clips replaced"), Replaced, 4)) return false;
		if (!AddFirstPersonReach(*this, BP.Get())) return false;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP.Get());
		FCompilerResultsLog Compile; FKismetEditorUtilities::CompileBlueprint(BP.Get(), EBlueprintCompileOptions::None, &Compile);
		if (!TestEqual(TEXT("Actual new graph compiler errors"), Compile.NumErrors, 0) || !TestEqual(TEXT("Actual new graph compiler warnings"), Compile.NumWarnings, 0)) return false;
		Montage.Reset(NewObject<UAnimMontage>(CreatePackage(*(Output + Names[1])), Names[1], RF_Public | RF_Standalone));
		Montage->SetSkeleton(Skeleton); Montage->SetPreviewMesh(Mesh.Get());
		Montage->SlotAnimTracks.Empty(); FSlotAnimationTrack& Track = Montage->SlotAnimTracks.AddDefaulted_GetRef();
		Track.SlotName = TEXT("DefaultSlot"); FAnimSegment Segment; Segment.SetAnimReference(Attack.Get(), true);
		Segment.AnimStartTime = 0; Segment.AnimEndTime = Attack->GetPlayLength(); Segment.AnimPlayRate = 1; Segment.LoopingCount = 1;
		Track.AnimTrack.AnimSegments.Add(Segment); Montage->SetCompositeLength(Attack->GetPlayLength());
		Montage->AddAnimCompositeSection(TEXT("Attack"), 0);
		Montage->BlendIn.SetBlendTime(.035f); Montage->BlendOut.SetBlendTime(.08f);
		Config.Reset(NewObject<URomanCharacterPresentation>(CreatePackage(*(Output + Names[2])), Names[2], RF_Public | RF_Standalone));
		// One legionary technical preview, NOT three accepted faction designs.
		Config->RomeMesh = Mesh.Get(); Config->CarthageMesh = Mesh.Get(); Config->GaulMesh = Mesh.Get();
		Config->AnimationClass = BP->GeneratedClass; Config->AttackMontage = Montage.Get();
		if (!Save(*this, BP.Get()) || !Save(*this, Montage.Get()) || !Save(*this, Config.Get())) return false;
	}
	else
	{
		BP.Reset(LoadObject<UAnimBlueprint>(nullptr, *(Output + Names[0])));
		Montage.Reset(LoadObject<UAnimMontage>(nullptr, *(Output + Names[1])));
		Config.Reset(LoadObject<URomanCharacterPresentation>(nullptr, *(Output + Names[2])));
	}
	for (UObject* Asset : {static_cast<UObject*>(BP.Get()), static_cast<UObject*>(Montage.Get()), static_cast<UObject*>(Config.Get())})
		if (!TestNotNull(TEXT("Actual owned output asset"), Asset) || !TestEqual(TEXT("Owned new preview schema"), FString(Asset->GetOutermost()->GetMetaData().GetValue(Asset, Tag)), FString(TEXT("1")))) return false;
	TestTrue(TEXT("Native canonical presentation-only parent retained"), BP->ParentClass == URomanSoldierAnimInstance::StaticClass());
	TestTrue(TEXT("New blueprint uses new skeleton"), BP->TargetSkeleton == Skeleton);
	TestTrue(TEXT("New compiled class linked by bundle"), BP->GeneratedClass && Config->AnimationClass == BP->GeneratedClass);
	TestTrue(TEXT("Candidate mesh linked, original bundle not replaced"), Config->RomeMesh == Mesh.Get());
	TestTrue(TEXT("New montage linked"), Config->AttackMontage == Montage.Get() && Montage->GetSkeleton() == Skeleton);
	TestEqual(TEXT("New montage has exactly one slot"), Montage->SlotAnimTracks.Num(), 1);
	if (Montage->SlotAnimTracks.Num() == 1)
	{
		const FSlotAnimationTrack& Track = Montage->SlotAnimTracks[0];
		TestEqual(TEXT("Canonical montage slot"), Track.SlotName, FName(TEXT("DefaultSlot")));
		if (TestEqual(TEXT("New montage one segment"), Track.AnimTrack.AnimSegments.Num(), 1))
			TestTrue(TEXT("New montage plays new attack, not old clip"), Track.AnimTrack.AnimSegments[0].GetAnimReference() == Attack.Get());
	}
	TestEqual(TEXT("No gameplay damage notifies"), Montage->Notifies.Num(), 0);
	if (TestEqual(TEXT("Actual montage has a playable section, not only a segment"), Montage->CompositeSections.Num(), 1))
	{
		TestEqual(TEXT("Actual first montage section"), Montage->GetSectionName(0), FName(TEXT("Attack")));
		float Start = 0.f, End = 0.f; Montage->GetSectionStartAndEndTime(0, Start, End);
		TestEqual(TEXT("Attack section starts at zero"), Start, 0.f);
		TestEqual(TEXT("Attack section spans actual clip"), End, Attack->GetPlayLength());
	}
	TestEqual(TEXT("Canonical short attack blend-in"), Montage->BlendIn.GetBlendTime(), .035f);
	TestEqual(TEXT("Canonical short attack blend-out"), Montage->BlendOut.GetBlendTime(), .08f);
	int32 ActualClips = 0;
	int32 ReachNodes = 0;
	for (UEdGraph* Graph : BP->FunctionGraphs) for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UAnimGraphNode_TwoBoneIK* IK = Cast<UAnimGraphNode_TwoBoneIK>(Node))
		{
			++ReachNodes;
			TestEqual(TEXT("Same-body right-hand IK bone"), IK->Node.IKBone.BoneName, FName(TEXT("hand_r")));
			TestFalse(TEXT("No limb stretching for camera framing"), bool(IK->Node.bAllowStretching));
			TestTrue(TEXT("Component-space targets with explicit local-player alpha"), IK->Node.EffectorLocationSpace == BCS_ComponentSpace && IK->Node.JointTargetLocationSpace == BCS_ComponentSpace && IK->Node.AlphaInputType == EAnimAlphaInputType::Bool);
			for (const TCHAR* Pin : {TEXT("bAlphaBoolEnabled"), TEXT("EffectorLocation"), TEXT("JointTargetLocation")})
				TestTrue(TEXT("Actual reach inputs are wired, not struct-only defaults"), IK->FindPin(Pin, EGPD_Input) && IK->FindPin(Pin, EGPD_Input)->LinkedTo.Num() == 1);
		}
		UAnimGraphNode_SequencePlayer* Player = Cast<UAnimGraphNode_SequencePlayer>(Node);
		UAnimGraphNode_SequenceEvaluator* Evaluator = Cast<UAnimGraphNode_SequenceEvaluator>(Node);
		UAnimSequenceBase* Clip = Player ? Player->Node.GetSequence() : Evaluator ? Evaluator->Node.GetSequence() : nullptr;
		if (Clip) { ++ActualClips; TestTrue(TEXT("Readback: every actual graph clip in new namespace/rig"), Clip->GetPathName().StartsWith(Candidate) && Clip->GetSkeleton() == Skeleton); }
	}
	TestEqual(TEXT("Readback: all four actual graph clips"), ActualClips, 4);
	TestEqual(TEXT("Exactly one local first-person reach control"), ReachNodes, 1);
	AddInfo(TEXT("Private native graph/montage/bundle only. No live game replacement, camera, faction-art or material-override approval."));
	return !HasAnyErrors();
}
#endif
