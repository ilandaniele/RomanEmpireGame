// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequenceEvaluator.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/AnimBlueprintFactory.h"
#include "HAL/FileManager.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Units/RomanCharacterPresentation.h"
#include "Units/RomanSoldierAnimInstance.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanCharacterAuthoring
{
const FString Folder(TEXT("/Game/Characters/RomanSoldier/"));
const FName Tag(TEXT("RomanCharacterAnimationSchema"));
const FString Schema(TEXT("1"));

bool SaveNew(FAutomationTestBase& Test, UObject* Asset, bool IsNew = true)
{
	UPackage* Package = Asset->GetOutermost();
	Package->GetMetaData().SetValue(Asset, Tag, *Schema);
	Asset->MarkPackageDirty();
	if (IsNew) FAssetRegistryModule::AssetCreated(Asset);
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
	return Test.TestTrue(TEXT("Actual original animation asset saved"), UPackage::SavePackage(Package, Asset,
		*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args));
}

template<typename T> T* Existing(FAutomationTestBase& Test, const TCHAR* Name)
{
	const FString Path = Folder + Name;
	if (!FPackageName::DoesPackageExist(Path)) return nullptr;
	T* Asset = LoadObject<T>(nullptr, *Path);
	if (!Test.TestNotNull(TEXT("Existing asset has expected class"), Asset)) return nullptr;
	if (!Test.TestEqual(TEXT("Refuse redirected asset outside exact target"), Asset->GetOutermost()->GetName(), Path)) return nullptr;
	if (!Test.TestEqual(TEXT("Refuse foreign/unsupported existing asset"), FString(Asset->GetOutermost()->GetMetaData().GetValue(Asset, Tag)), Schema)) return nullptr;
	return Asset;
}

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
		Test.AddError(FString::Printf(TEXT("Missing actual graph pin %s:%s -> %s:%s"), *From->GetClass()->GetName(), Out, *To->GetClass()->GetName(), In));
		for (const UEdGraphNode* N : {From, To}) for (const UEdGraphPin* P : N->Pins) Test.AddInfo(P->PinName.ToString());
		return false;
	}
	return Test.TestTrue(TEXT("Actual graph schema connection"), Graph->GetSchema()->TryCreateConnection(A, B));
}

UAnimGraphNode_SequencePlayer* Player(UEdGraph* Graph, UAnimSequence* Sequence, bool Loop, int32 Y)
{
	UAnimGraphNode_SequencePlayer* Result = Node<UAnimGraphNode_SequencePlayer>(Graph, -1400, Y);
	Result->Node.SetSequence(Sequence); Result->Node.SetLoopAnimation(Loop); Result->ReconstructNode(); return Result;
}

UAnimGraphNode_BlendListByBool* Blend(FAutomationTestBase& Test, UEdGraph* Graph, const TCHAR* Variable, int32 X, int32 Y)
{
	UAnimGraphNode_BlendListByBool* Result = Node<UAnimGraphNode_BlendListByBool>(Graph, X, Y);
	// UE5.8 field is intentionally spelled ChildUpateMode. Use current reflection,
	// not the removed/deprecated bResetChildOnActivation property.
	FEnumProperty* Update = FindFProperty<FEnumProperty>(FAnimNode_BlendListByBool::StaticStruct(), TEXT("ChildUpateMode"));
	if (Test.TestNotNull(TEXT("Current native child update property"), Update))
		Update->GetUnderlyingProperty()->SetIntPropertyValue(Update->ContainerPtrToValuePtr<void>(&Result->Node), static_cast<uint64>(EBlendListChildUpdateMode::ResetChildOnActivate));
	UK2Node_VariableGet* Getter = Node<UK2Node_VariableGet>(Graph, X - 250, Y - 130);
	Getter->VariableReference.SetSelfMember(FName(Variable)); Getter->ReconstructNode();
	Link(Test, Graph, Getter, Variable, Result, TEXT("bActiveValue"));
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanCharacterAnimationAuthoring, "RomanEmpireGame.Characters.AnimationGraphContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanCharacterAnimationAuthoring::RunTest(const FString& Parameters)
{
	using namespace RomanCharacterAuthoring;
	const bool Apply = FParse::Param(FCommandLine::Get(), TEXT("RomanAuthorCharacterAnimations"));
	// Check all existing bundle targets BEFORE any creation. Test without flag is readonly.
	TStrongObjectPtr<UAnimBlueprint> BP(Existing<UAnimBlueprint>(*this, TEXT("ABP_RomanSoldier")));
	TStrongObjectPtr<UAnimMontage> Montage(Existing<UAnimMontage>(*this, TEXT("AM_RomanAttack")));
	TStrongObjectPtr<URomanCharacterPresentation> Config(Existing<URomanCharacterPresentation>(*this, TEXT("DA_RomanCharacterPresentation")));
	if (HasAnyErrors()) return false;
	TStrongObjectPtr<USkeleton> Skeleton(LoadObject<USkeleton>(nullptr, *(Folder + TEXT("SK_Roman_Rome_Skeleton"))));
	if (!TestNotNull(TEXT("Actual shared Skeleton"), Skeleton.Get())) return false;
	TStrongObjectPtr<USkeletalMesh> Gaul(LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Gaul"))));
	if (!TestNotNull(TEXT("Actual original Gaul profile"), Gaul.Get()) || !TestTrue(TEXT("Gaul shares original rig"), Gaul->GetSkeleton() == Skeleton.Get())) return false;
	TStrongObjectPtr<UAnimSequence> Idle(LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_RomanIdle"))));
	TStrongObjectPtr<UAnimSequence> Walk(LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_RomanWalk"))));
	TStrongObjectPtr<UAnimSequence> Block(LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_RomanBlock"))));
	TStrongObjectPtr<UAnimSequence> Death(LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_RomanDeath"))));
	TStrongObjectPtr<UAnimSequence> Attack(LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_RomanAttack"))));
	for (UAnimSequence* Clip : {Idle.Get(), Walk.Get(), Block.Get(), Death.Get(), Attack.Get()})
		if (!TestNotNull(TEXT("Actual required animation"), Clip) || !TestTrue(TEXT("Same rig"), Clip->GetSkeleton() == Skeleton.Get())) return false;
	if (!BP.IsValid())
	{
		if (!TestTrue(TEXT("Missing AnimBP needs explicit authoring flag"), Apply)) return false;
		TStrongObjectPtr<UAnimBlueprintFactory> Factory(NewObject<UAnimBlueprintFactory>());
		Factory->ParentClass = URomanSoldierAnimInstance::StaticClass(); Factory->TargetSkeleton = Skeleton.Get();
		BP.Reset(Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), CreatePackage(*(Folder + TEXT("ABP_RomanSoldier"))),
			TEXT("ABP_RomanSoldier"), RF_Public | RF_Standalone, nullptr, GWarn)));
		if (!TestNotNull(TEXT("AnimBP factory result"), BP.Get())) return false;
		UEdGraph* Graph = nullptr;
		for (UEdGraph* Candidate : BP->FunctionGraphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
		if (!TestNotNull(TEXT("Real AnimGraph"), Graph)) return false;
		UAnimGraphNode_Root* Root = nullptr;
		for (UEdGraphNode* Candidate : Graph->Nodes) if (UAnimGraphNode_Root* R = Cast<UAnimGraphNode_Root>(Candidate)) Root = R;
		if (!TestNotNull(TEXT("Actual pose root"), Root)) return false;
		UAnimGraphNode_SequencePlayer* IdlePlayer = Player(Graph, Idle.Get(), true, -300);
		UAnimGraphNode_SequencePlayer* WalkPlayer = Player(Graph, Walk.Get(), true, -550);
		UAnimGraphNode_BlendListByBool* Moving = Blend(*this, Graph, TEXT("bMoving"), -1000, -350);
		UAnimGraphNode_SequenceEvaluator* Guard = Node<UAnimGraphNode_SequenceEvaluator>(Graph, -1400, 0);
		Guard->Node.SetSequence(Block.Get()); Guard->Node.SetExplicitTime(.4f); Guard->ReconstructNode();
		// Visible pins override folded struct values. Reconstruct preserves the old
		// pin default0; explicitly author the real compiled evaluator input.
		if (UEdGraphPin* Time = Guard->FindPin(TEXT("ExplicitTime"), EGPD_Input)) Graph->GetSchema()->TrySetDefaultValue(*Time, TEXT("0.4"));
		else { AddError(TEXT("Guard evaluator time pin missing")); return false; }
		UAnimGraphNode_BlendListByBool* Blocking = Blend(*this, Graph, TEXT("bBlocking"), -650, -100);
		UAnimGraphNode_Slot* Slot = Node<UAnimGraphNode_Slot>(Graph, -300, -100); Slot->Node.SlotName = TEXT("DefaultSlot");
		UAnimGraphNode_SequencePlayer* DeathPlayer = Player(Graph, Death.Get(), false, 350);
		UAnimGraphNode_BlendListByBool* Dead = Blend(*this, Graph, TEXT("bDead"), 0, 150);
		Link(*this, Graph, WalkPlayer, TEXT("Pose"), Moving, TEXT("BlendPose_0"));
		Link(*this, Graph, IdlePlayer, TEXT("Pose"), Moving, TEXT("BlendPose_1"));
		Link(*this, Graph, Guard, TEXT("Pose"), Blocking, TEXT("BlendPose_0"));
		Link(*this, Graph, Moving, TEXT("Pose"), Blocking, TEXT("BlendPose_1"));
		Link(*this, Graph, Blocking, TEXT("Pose"), Slot, TEXT("Source"));
		Link(*this, Graph, DeathPlayer, TEXT("Pose"), Dead, TEXT("BlendPose_0"));
		Link(*this, Graph, Slot, TEXT("Pose"), Dead, TEXT("BlendPose_1"));
		Link(*this, Graph, Dead, TEXT("Pose"), Root, TEXT("Result"));
		if (HasAnyErrors()) return false;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP.Get());
		FCompilerResultsLog Compile; FKismetEditorUtilities::CompileBlueprint(BP.Get(), EBlueprintCompileOptions::None, &Compile);
		if (!TestEqual(TEXT("Actual AnimBP compiler errors"), Compile.NumErrors, 0) || !TestEqual(TEXT("Actual AnimBP compiler warnings"), Compile.NumWarnings, 0)) return false;
		if (!SaveNew(*this, BP.Get())) return false;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("RomanRepairCharacterGuard")))
	{
		UEdGraph* Graph = nullptr;
		for (UEdGraph* Candidate : BP->FunctionGraphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
		if (!TestTrue(TEXT("Known original owned guard graph only"), Graph && Graph->Nodes.Num() == 12 && BP->TargetSkeleton == Skeleton.Get()
			&& BP->ParentClass == URomanSoldierAnimInstance::StaticClass())) return false;
		UAnimGraphNode_SequenceEvaluator* Guard = nullptr; int32 Evaluators = 0;
		for (UEdGraphNode* N : Graph->Nodes) if (UAnimGraphNode_SequenceEvaluator* E = Cast<UAnimGraphNode_SequenceEvaluator>(N)) { Guard = E; ++Evaluators; }
		if (!TestTrue(TEXT("Exact original block evaluator"), Evaluators == 1 && Guard->Node.GetSequence() == Block.Get())) return false;
		UEdGraphPin* Time = Guard->FindPin(TEXT("ExplicitTime"), EGPD_Input);
		if (!TestTrue(TEXT("Known unconnected zero-time default bug only"), Time && Time->LinkedTo.IsEmpty() && FMath::IsNearlyZero(FCString::Atof(*Time->DefaultValue)))) return false;
		Graph->GetSchema()->TrySetDefaultValue(*Time, TEXT("0.4"));
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP.Get());
		FCompilerResultsLog Compile; FKismetEditorUtilities::CompileBlueprint(BP.Get(), EBlueprintCompileOptions::None, &Compile);
		if (!TestEqual(TEXT("Guard repair compiler errors"), Compile.NumErrors, 0) || !TestEqual(TEXT("Guard repair compiler warnings"), Compile.NumWarnings, 0)) return false;
		if (!SaveNew(*this, BP.Get(), false)) return false;
		AddInfo(TEXT("Repaired owned guard evaluator visible pin default only"));
	}
	if (!Montage.IsValid())
	{
		if (!TestTrue(TEXT("Missing montage needs explicit authoring flag"), Apply)) return false;
		Montage.Reset(NewObject<UAnimMontage>(CreatePackage(*(Folder + TEXT("AM_RomanAttack"))), TEXT("AM_RomanAttack"), RF_Public | RF_Standalone));
		Montage->SetSkeleton(Skeleton.Get());
		FSlotAnimationTrack Track; Track.SlotName = TEXT("DefaultSlot");
		FAnimSegment Segment; Segment.SetAnimReference(Attack.Get(), true); Segment.AnimEndTime = Attack->GetPlayLength();
		// UAnimMontage's native constructor already creates an empty DefaultSlot.
		Montage->SlotAnimTracks.Reset();
		Track.AnimTrack.AnimSegments.Add(Segment); Montage->SlotAnimTracks.Add(Track);
		Montage->SetCompositeLength(Attack->GetPlayLength()); Montage->AddAnimCompositeSection(TEXT("Attack"), 0);
		Montage->BlendIn.SetBlendTime(.035f); Montage->BlendOut.SetBlendTime(.08f);
		if (!SaveNew(*this, Montage.Get())) return false;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("RomanRepairCharacterMontage")))
	{
		// Narrow one-time repair of OUR initial authoring output only. No generic
		// reimport/overwrite: ownership was checked above, shape must match exactly.
		if (!TestEqual(TEXT("Known duplicate constructor slot only"), Montage->SlotAnimTracks.Num(), 2)) return false;
		const FSlotAnimationTrack& Empty = Montage->SlotAnimTracks[0];
		const FSlotAnimationTrack& Owned = Montage->SlotAnimTracks[1];
		if (!TestTrue(TEXT("Exact owned default-slot bug shape"), Empty.SlotName == TEXT("DefaultSlot") && Empty.AnimTrack.AnimSegments.IsEmpty()
			&& Owned.SlotName == TEXT("DefaultSlot") && Owned.AnimTrack.AnimSegments.Num() == 1
			&& Owned.AnimTrack.AnimSegments[0].GetAnimReference() == Attack.Get() && Montage->Notifies.IsEmpty()
			&& Montage->GetSkeleton() == Skeleton.Get() && FMath::IsNearlyEqual(Montage->GetPlayLength(), .6f))) return false;
		Montage->SlotAnimTracks.RemoveAt(0);
		if (!SaveNew(*this, Montage.Get(), false)) return false;
		AddInfo(TEXT("Repaired owned schema1 constructor DefaultSlot; no other package saved by repair"));
	}
	if (!Config.IsValid())
	{
		if (!TestTrue(TEXT("Missing presentation bundle needs explicit authoring flag"), Apply)) return false;
		Config.Reset(NewObject<URomanCharacterPresentation>(CreatePackage(*(Folder + TEXT("DA_RomanCharacterPresentation"))), TEXT("DA_RomanCharacterPresentation"), RF_Public | RF_Standalone));
		Config->RomeMesh = LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Rome")));
		Config->CarthageMesh = LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Carthage")));
		Config->GaulMesh = Gaul.Get();
		Config->AnimationClass = BP->GeneratedClass; Config->AttackMontage = Montage.Get();
		if (!SaveNew(*this, Config.Get())) return false;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("RomanAddGaulCharacterProfile")))
	{
		// Additive migration of this exact owned bootstrap bundle, never mesh/BP reimport.
		if (!TestTrue(TEXT("Only known owned two-profile bundle can migrate"), !Config->GaulMesh
			&& Config->RomeMesh == LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Rome")))
			&& Config->CarthageMesh == LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Carthage")))
			&& Config->AnimationClass == BP->GeneratedClass && Config->AttackMontage == Montage.Get())) return false;
		Config->GaulMesh = Gaul.Get();
		if (!SaveNew(*this, Config.Get(), false)) return false;
		AddInfo(TEXT("Added Gaul reference to exact owned schema1 art bundle; only bundle saved"));
	}
	TestTrue(TEXT("Actual native presentation-only AnimInstance parent"), BP->ParentClass == URomanSoldierAnimInstance::StaticClass());
	TestTrue(TEXT("Actual blueprint targets shared rig"), BP->TargetSkeleton == Skeleton.Get());
	TestTrue(TEXT("Actual compiled animation class is valid"), BP->GeneratedClass && BP->GeneratedClass->IsChildOf(URomanSoldierAnimInstance::StaticClass()));
	TestTrue(TEXT("Actual bundle animation class"), Config->AnimationClass == BP->GeneratedClass);
	TestTrue(TEXT("Actual original Rome bundle profile"), Config->RomeMesh == LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Rome"))));
	TestTrue(TEXT("Actual original Carthage bundle profile"), Config->CarthageMesh == LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_Carthage"))));
	TestTrue(TEXT("Actual original Gaul bundle profile"), Config->GaulMesh == Gaul.Get());
	TestTrue(TEXT("Actual bundle montage"), Config->AttackMontage == Montage.Get());
	TestTrue(TEXT("Actual montage shared rig"), Montage->GetSkeleton() == Skeleton.Get());
	TestEqual(TEXT("Actual montage length"), Montage->GetPlayLength(), .6f);
	TestEqual(TEXT("No gameplay notify authority"), Montage->Notifies.Num(), 0);
	if (TestEqual(TEXT("Actual montage slot"), Montage->SlotAnimTracks.Num(), 1))
	{
		TestEqual(TEXT("Montage slot matches graph"), Montage->SlotAnimTracks[0].SlotName, FName(TEXT("DefaultSlot")));
		if (TestEqual(TEXT("Actual montage segment"), Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num(), 1))
			TestTrue(TEXT("Actual montage plays original attack clip"), Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference() == Attack.Get());
	}
	for (USkeletalMesh* Mesh : {Config->RomeMesh.Get(), Config->CarthageMesh.Get(), Config->GaulMesh.Get()})
		if (TestNotNull(TEXT("Actual bundle mesh"), Mesh)) TestTrue(TEXT("Actual bundle mesh shared rig"), Mesh->GetSkeleton() == Skeleton.Get());
	UEdGraph* Graph = nullptr;
	for (UEdGraph* Candidate : BP->FunctionGraphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
	if (!TestNotNull(TEXT("Saved actual AnimGraph"), Graph)) return false;
	TestEqual(TEXT("Functional graph not empty shell"), Graph->Nodes.Num(), 12);
	for (UEdGraphNode* N : Graph->Nodes)
	{
		if (UAnimGraphNode_SequenceEvaluator* Guard = Cast<UAnimGraphNode_SequenceEvaluator>(N))
		{
			UEdGraphPin* Time = Guard->FindPin(TEXT("ExplicitTime"), EGPD_Input);
			TestTrue(TEXT("Actual compiled hold-time pin, not overridden struct-only value"), Time && Time->LinkedTo.IsEmpty() && FMath::IsNearlyEqual(FCString::Atof(*Time->DefaultValue), .4f));
		}
		for (const UEdGraphPin* P : N->Pins)
			if (P->Direction == EGPD_Output && P->PinName == TEXT("Pose")) TestFalse(TEXT("Every real pose feeds graph"), P->LinkedTo.IsEmpty());
	}
	return !HasAnyErrors();
}
#endif
