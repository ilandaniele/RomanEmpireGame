// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FrameNumber.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanCharacterAssets, "RomanEmpireGame.Characters.ImportedRigAndMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanCharacterAssets::RunTest(const FString& Parameters)
{
	const FString Folder(TEXT("/Game/Characters/RomanSoldier/"));
	TStrongObjectPtr<USkeleton> Skeleton(LoadObject<USkeleton>(nullptr, *(Folder + TEXT("SK_Roman_Rome_Skeleton"))));
	if (!TestNotNull(TEXT("Actual saved shared Skeleton"), Skeleton.Get())) return false;
	const FName Bones[] = {TEXT("root"), TEXT("pelvis"), TEXT("spine"), TEXT("chest"), TEXT("head"),
		TEXT("upper_arm_l"), TEXT("forearm_l"), TEXT("hand_l"), TEXT("upper_arm_r"), TEXT("forearm_r"), TEXT("hand_r"),
		TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r")};
	const int32 Parents[] = {-1, 0, 1, 2, 3, 3, 5, 6, 3, 8, 9, 1, 11, 12, 1, 14, 15};
	const FReferenceSkeleton& Rig = Skeleton->GetReferenceSkeleton();
	TestEqual(TEXT("Real complete bone count, not empty Skeleton"), Rig.GetNum(), 17);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Bones); ++Index)
	{
		const int32 Bone = Rig.FindBoneIndex(Bones[Index]);
		if (!TestTrue(TEXT("Actual named bone exists"), Bone != INDEX_NONE)) continue;
		const int32 Parent = Parents[Index] < 0 ? INDEX_NONE : Rig.FindBoneIndex(Bones[Parents[Index]]);
		TestEqual(TEXT("Actual hierarchical parent"), Rig.GetParentIndex(Bone), Parent);
		TestFalse(TEXT("Finite actual bind pose"), Rig.GetRefBonePose()[Bone].ContainsNaN());
	}
	const FName Slots[] = {TEXT("Body"), TEXT("Metal"), TEXT("Skin"), TEXT("Leather"), TEXT("Shield"), TEXT("Crest")};
	uint32 ShieldTriangles[3] = {};
	const TCHAR* Profiles[] = {TEXT("Rome"), TEXT("Carthage"), TEXT("Gaul")};
	for (int32 Profile = 0; Profile < UE_ARRAY_COUNT(Profiles); ++Profile)
	{
		const FString ProfileName(Profiles[Profile]);
		TStrongObjectPtr<USkeletalMesh> Mesh(LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SK_Roman_") + ProfileName)));
		if (!TestNotNull(TEXT("Actual saved skinned mesh"), Mesh.Get())) continue;
		TestTrue(TEXT("All factions reuse identical Skeleton object"), Mesh->GetSkeleton() == Skeleton.Get());
		TestNull(TEXT("Capsule remains sole collision authority"), Mesh->GetPhysicsAsset());
		TestEqual(TEXT("Mesh has complete actual rig"), Mesh->GetRefSkeleton().GetNum(), 17);
		const FBoxSphereBounds Bounds = Mesh->GetImportedBounds();
		TestTrue(TEXT("Actual imported cm/Z-up human height"), Bounds.BoxExtent.Z * 2 > 175 && Bounds.BoxExtent.Z * 2 < 205);
		TestTrue(TEXT("Bind geometry touches ground"), FMath::Abs(Bounds.Origin.Z - Bounds.BoxExtent.Z) < .01);
		const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
		TestEqual(TEXT("Six actual material slots"), Materials.Num(), 6);
		const FString MaterialNames[] = {TEXT("MI_Faction") + ProfileName, TEXT("MI_RomanMetal"), TEXT("MI_RomanSkin"),
			TEXT("MI_RomanLeather"), TEXT("MI_Faction") + ProfileName, TEXT("MI_Faction") + ProfileName};
		for (int32 Index = 0; Index < FMath::Min(Materials.Num(), 6); ++Index)
		{
			TestEqual(TEXT("Actual semantic slot name"), Materials[Index].MaterialSlotName, Slots[Index]);
			const FString Path = (Index == 0 || Index >= 4 ? TEXT("/Game/Materials/RomanVisual/") : Folder) + MaterialNames[Index];
			TestTrue(TEXT("Actual reference to canonical material"), Materials[Index].MaterialInterface == LoadObject<UMaterialInstanceConstant>(nullptr, *Path));
		}
		const FSkeletalMeshModel* Model = Mesh->GetImportedModel();
		if (!TestNotNull(TEXT("Actual imported topology"), Model) || !TestEqual(TEXT("Single budgeted LOD"), Model->LODModels.Num(), 1)) continue;
		const FSkeletalMeshLODModel& LOD = Model->LODModels[0];
		TestEqual(TEXT("Six real geometry sections"), LOD.Sections.Num(), 6);
		uint32 Triangles = 0; uint32 Vertices = 0; TSet<int32> WeightedBones;
		for (const FSkelMeshSection& Section : LOD.Sections)
		{
			Triangles += Section.NumTriangles; Vertices += Section.SoftVertices.Num();
			TestTrue(TEXT("Nonempty actual triangles"), Section.NumTriangles > 0);
			TestTrue(TEXT("Section index range exists"), Section.BaseIndex + Section.NumTriangles * 3 <= static_cast<uint32>(LOD.IndexBuffer.Num()));
			if (Section.MaterialIndex == 4) ShieldTriangles[Profile] += Section.NumTriangles;
			for (const FSoftSkinVertex& Vertex : Section.SoftVertices)
			{
				uint32 Weight = 0; int32 Influences = 0;
				TestFalse(TEXT("Actual vertex finite"), Vertex.Position.ContainsNaN());
				TestTrue(TEXT("Actual normal unit length"), FMath::Abs(FVector3f(Vertex.TangentZ.X, Vertex.TangentZ.Y, Vertex.TangentZ.Z).SizeSquared() - 1.f) < .002f);
				for (int32 Index = 0; Index < MAX_TOTAL_INFLUENCES; ++Index)
				{
					Weight += Vertex.InfluenceWeights[Index];
					if (Vertex.InfluenceWeights[Index] == 0) continue;
					++Influences;
					const int32 Bone = Vertex.InfluenceBones[Index];
					if (TestTrue(TEXT("Actual weight maps to section bone"), Section.BoneMap.IsValidIndex(Bone)))
					{
						TestTrue(TEXT("Actual section bone maps to rig"), Section.BoneMap[Bone] < 17);
						WeightedBones.Add(Section.BoneMap[Bone]);
					}
				}
				TestEqual(TEXT("Actual normalized uint16 skin weights"), Weight, 65535u);
				TestEqual(TEXT("Rigid segmented armor: exactly one actual influence"), Influences, 1);
			}
		}
		TestTrue(TEXT("Actual triangle budget"), Triangles > 300 && Triangles <= 5000);
		TestEqual(TEXT("All saved vertices validated"), Vertices, LOD.NumVertices);
		TestTrue(TEXT("Skinning spans body and limbs, not root-only static mesh"), WeightedBones.Num() >= 12);
	}
	TestNotEqual(TEXT("Faction shield geometry differs beyond tint"), ShieldTriangles[0], ShieldTriangles[1]);
	TestNotEqual(TEXT("Gaul shield differs from Rome beyond tint"), ShieldTriangles[2], ShieldTriangles[0]);
	TestNotEqual(TEXT("Gaul shield differs from Carthage beyond tint"), ShieldTriangles[2], ShieldTriangles[1]);
	const TCHAR* Clips[] = {TEXT("Idle"), TEXT("Walk"), TEXT("Attack"), TEXT("Block"), TEXT("Death")};
	const float Durations[] = {2.f, 1.f, .6f, .8f, 1.f};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Clips); ++Index)
	{
		TStrongObjectPtr<UAnimSequence> Sequence(LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_Roman") + Clips[Index])));
		if (!TestNotNull(TEXT("Actual saved animation"), Sequence.Get())) continue;
		TestTrue(TEXT("Actual shared animation Skeleton"), Sequence->GetSkeleton() == Skeleton.Get());
		TestTrue(TEXT("Actual animation duration"), FMath::Abs(Sequence->GetPlayLength() - Durations[Index]) < .001f);
		TestFalse(TEXT("Animation cannot move gameplay actor"), Sequence->bEnableRootMotion);
		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!TestNotNull(TEXT("Actual bone key data"), Model)) continue;
		TArray<FName> Tracks; Model->GetBoneTrackNames(Tracks);
		TestEqual(TEXT("All seventeen real bone tracks"), Tracks.Num(), 17);
		TestEqual(TEXT("Baked 30fps real keys"), Model->GetNumberOfKeys(), FMath::RoundToInt(Durations[Index] * 30) + 1);
		bool bChangingPose = false;
		for (const FName Track : Tracks)
		{
			const FTransform Start = Model->GetBoneTrackTransform(Track, FFrameNumber(0));
			for (int32 Frame = 1; Frame < Model->GetNumberOfKeys(); ++Frame)
			{
				const FTransform Pose = Model->GetBoneTrackTransform(Track, FFrameNumber(Frame));
				TestFalse(TEXT("Actual animation transform finite"), Pose.ContainsNaN());
				bChangingPose |= !Pose.GetRotation().Equals(Start.GetRotation(), .0001f);
				TestTrue(TEXT("No animation translation locomotion"), Pose.GetTranslation().Equals(Start.GetTranslation(), .001f));
			}
			if (Index != 4)
				TestTrue(TEXT("Loop/action returns to actual bind pose"), Start.Equals(Model->GetBoneTrackTransform(Track, FFrameNumber(Model->GetNumberOfKeys() - 1)), .001f));
		}
		TestTrue(TEXT("Delivered clip has real changing pose, not empty asset"), bChangingPose);
	}
	return true;
}
#endif
