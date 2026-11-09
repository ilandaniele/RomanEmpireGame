// Read-only private Editor fixture. Never part of the game's runtime module.
#include "Misc/AutomationTest.h"
#include "Animation/Skeleton.h"
#include "AssetCompilingManager.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "SkeletalMeshTypes.h"
#include "Units/RomanCharacterPresentation.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanSemirealLODReadback, "RomanPreview.LOD.Readback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRomanSemirealLODReadback::RunTest(const FString& Parameters)
{
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(Project);
	if (!TestTrue(TEXT("Exact private project only"), Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")))) return false;
	FString Namespace, ReceiptName;
	if (!TestTrue(TEXT("Explicit LOD namespace"), FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealLOD="), Namespace)
		&& Namespace.StartsWith(TEXT("LOD")) && Namespace.Len() > 3 && Namespace.Mid(3).IsNumeric())) return false;
	if (!TestTrue(TEXT("Explicit receipt name"), FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealLODReceipt="), ReceiptName))) return false;
	for (TCHAR Character : ReceiptName)
		if (!TestTrue(TEXT("Safe receipt filename"), FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-') || Character == TEXT('.'))) return false;
	if (!TestTrue(TEXT("Plain .txt receipt"), ReceiptName.EndsWith(TEXT(".txt")) && !ReceiptName.Contains(TEXT("..")))) return false;
	const FString Output = FPaths::Combine(Project, ReceiptName);
	if (!TestFalse(TEXT("Refuse evidence overwrite"), IFileManager::Get().FileExists(*Output))) return false;
	const FString Base = TEXT("/Game/ArtCandidates/Attempt10_1/");
	USkeletalMesh* Original = LoadObject<USkeletalMesh>(nullptr, *(Base + TEXT("Runtime05/SK_RuntimePreview")));
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *(Base + Namespace + TEXT("/SK_LODPreview")));
	URomanCharacterPresentation* Bundle = LoadObject<URomanCharacterPresentation>(nullptr, *(Base + Namespace + TEXT("/DA_LODPreview")));
	if (!TestNotNull(TEXT("Runtime05"), Original) || !TestNotNull(TEXT("LOD candidate"), Mesh) || !TestNotNull(TEXT("LOD bundle"), Bundle)) return false;
	FAssetCompilingManager::Get().FinishAllCompilation();
	TestEqual(TEXT("Source LOD0 only"), Original->GetLODNum(), 1);
	if (!TestEqual(TEXT("Three actual imported LODs"), Mesh->GetLODNum(), 3)) return false;
	TestEqual(TEXT("Own source skeleton reused"), Mesh->GetSkeleton(), Original->GetSkeleton());
	TestNull(TEXT("No physics authority introduced"), Mesh->GetPhysicsAsset());
	const FReferenceSkeleton& Reference = Mesh->GetRefSkeleton();
	const FReferenceSkeleton& SourceReference = Original->GetRefSkeleton();
	if (!TestEqual(TEXT("Seventeen candidate bones"), Reference.GetNum(), 17)) return false;
	TestEqual(TEXT("Same source bone count"), SourceReference.GetNum(), Reference.GetNum());
	for (int32 Index = 0; Index < Reference.GetNum(); ++Index)
	{
		TestEqual(TEXT("Bone names unchanged"), Reference.GetBoneName(Index), SourceReference.GetBoneName(Index));
		TestEqual(TEXT("Bone parent unchanged"), Reference.GetParentIndex(Index), SourceReference.GetParentIndex(Index));
		TestTrue(TEXT("Bind transform unchanged"), Reference.GetRefBonePose()[Index].Equals(SourceReference.GetRefBonePose()[Index], 1.e-6f));
	}
	TestEqual(TEXT("Eight PBR materials"), Mesh->GetMaterials().Num(), 8);
	for (int32 Index = 0; Index < Mesh->GetMaterials().Num(); ++Index)
	{
		TestEqual(TEXT("Slot semantics unchanged"), Mesh->GetMaterials()[Index].MaterialSlotName, Original->GetMaterials()[Index].MaterialSlotName);
		TestEqual(TEXT("Original PBR parent reused"), Mesh->GetMaterials()[Index].MaterialInterface.Get(), Original->GetMaterials()[Index].MaterialInterface.Get());
	}
	const FSkeletalMeshModel* Model = Mesh->GetImportedModel();
	if (!TestNotNull(TEXT("Real imported model"), Model) || !TestEqual(TEXT("Three imported model entries"), Model->LODModels.Num(), 3)) return false;
	// Source SHA-pinned GLBs contain 24 coincident-corner pole faces, all brass.
	// Independent source topology analysis proves this exact per-slot difference;
	// Engine import/chunk construction rejects collapsed triangle indices even
	// when the later build-settings bRemoveDegenerates flag is false.
	const uint32 ExpectedTriangles[] = {49758, 12420, 4954};
	const uint32 ExpectedBySlot[3][8] = {
		{1408, 2258, 23844, 4100, 1920, 14020, 1152, 1056},
		{911, 708, 6946, 1122, 80, 1746, 689, 218},
		{439, 285, 2718, 502, 52, 572, 308, 78}};
	FString Receipt = FString::Printf(TEXT("mesh=%s\nskeleton=%s\nbones=%d\n"), *Mesh->GetPathName(), *Mesh->GetSkeleton()->GetPathName(), Reference.GetNum());
	FBox BaseBounds(ForceInit);
	float PreviousScreenSize = TNumericLimits<float>::Max();
	for (int32 LOD = 0; LOD < 3; ++LOD)
	{
		const FSkeletalMeshLODModel& Data = Model->LODModels[LOD];
		const FSkeletalMeshLODInfo* Info = Mesh->GetLODInfo(LOD);
		if (!TestNotNull(TEXT("LOD info exists"), Info)) return false;
		const float ScreenSize = Info->ScreenSize.Default;
		TestEqual(TEXT("Original degenerate build setting preserved"), bool(Info->BuildSettings.bRemoveDegenerates), bool(Original->GetLODInfo(0)->BuildSettings.bRemoveDegenerates));
		TestTrue(TEXT("Native point threshold matches source analysis in metres"), FMath::IsNearlyEqual(Info->BuildSettings.ThresholdPosition, .00002f, 1.e-9f));
		TestTrue(TEXT("Screen thresholds finite, positive and descending"), FMath::IsFinite(ScreenSize) && ScreenSize > 0.f && ScreenSize < PreviousScreenSize);
		PreviousScreenSize = ScreenSize;
		TestEqual(TEXT("Every LOD retains eight sections"), Data.Sections.Num(), 8);
		TestTrue(TEXT("Real UV channel exists"), Data.NumTexCoords >= 1 && Data.NumTexCoords <= MAX_TEXCOORDS);
		bool ValidSkin = true, ValidGeometry = true, ValidIndices = true;
		uint32 Triangles = 0, Vertices = 0, MaxInfluences = 0;
		FBox Bounds(ForceInit);
		TSet<int32> MaterialSlots;
		for (int32 SectionIndex = 0; SectionIndex < Data.Sections.Num(); ++SectionIndex)
		{
			const FSkelMeshSection& Section = Data.Sections[SectionIndex];
			Triangles += Section.NumTriangles;
			Vertices += Section.SoftVertices.Num();
			int32 Slot = Section.MaterialIndex;
			if (Info->LODMaterialMap.IsValidIndex(SectionIndex) && Info->LODMaterialMap[SectionIndex] != INDEX_NONE)
				Slot = Info->LODMaterialMap[SectionIndex];
			ValidGeometry &= Mesh->GetMaterials().IsValidIndex(Slot) && Section.NumTriangles > 0 && !Section.bDisabled;
			MaterialSlots.Add(Slot);
			if (Slot >= 0 && Slot < 8)
				TestEqual(TEXT("Exact nondegenerate authored triangles per semantic material"), Section.NumTriangles, ExpectedBySlot[LOD][Slot]);
			ValidIndices &= uint64(Section.BaseIndex) + uint64(Section.NumTriangles) * 3 <= uint64(Data.IndexBuffer.Num());
			for (FBoneIndexType Bone : Section.BoneMap) ValidSkin &= Bone < Reference.GetNum();
			for (const FSoftSkinVertex& Vertex : Section.SoftVertices)
			{
				ValidGeometry &= !Vertex.Position.ContainsNaN() && !Vertex.TangentX.ContainsNaN() && !Vertex.TangentZ.ContainsNaN();
				for (uint32 UV = 0; UV < Data.NumTexCoords; ++UV) ValidGeometry &= !Vertex.UVs[UV].ContainsNaN();
				Bounds += FVector(Vertex.Position);
				uint32 Sum = 0, Influences = 0;
				for (int32 Influence = 0; Influence < MAX_TOTAL_INFLUENCES; ++Influence)
				{
					const uint16 Weight = Vertex.InfluenceWeights[Influence];
					Sum += Weight;
					if (Weight > 0)
					{
						++Influences;
						ValidSkin &= Section.BoneMap.IsValidIndex(Vertex.InfluenceBones[Influence]);
					}
				}
				MaxInfluences = FMath::Max(MaxInfluences, Influences);
				ValidSkin &= Influences > 0 && FMath::Abs(int32(Sum) - 65535) <= 1;
			}
		}
		for (uint32 Index : Data.IndexBuffer) ValidIndices &= Index < Data.NumVertices;
		TestEqual(TEXT("Exact authored triangle count minus independently verified pole degenerates"), Triangles, ExpectedTriangles[LOD]);
		TestEqual(TEXT("Index count matches triangles"), uint32(Data.IndexBuffer.Num()), Triangles * 3);
		TestEqual(TEXT("All actual vertices checked"), Vertices, Data.NumVertices);
		TestEqual(TEXT("Every semantic material slot mapped once"), MaterialSlots.Num(), 8);
		TestTrue(TEXT("Finite actual geometry, normals, UVs and visible valid sections"), ValidGeometry);
		TestTrue(TEXT("All vertex skin weights normalized and bones valid"), ValidSkin);
		TestTrue(TEXT("All triangle indices and section ranges valid"), ValidIndices);
		TestTrue(TEXT("At most four authored influences per vertex"), MaxInfluences <= 4);
		if (LOD == 0) BaseBounds = Bounds;
		else TestTrue(TEXT("Decimation preserves rest bounds within five cm"), Bounds.Min.Equals(BaseBounds.Min, 5.) && Bounds.Max.Equals(BaseBounds.Max, 5.));
		if (LOD > 0)
		{
			const FString Suffix = FString::Printf(TEXT("SK_Roman_Rome_Semireal01_LOD%d.glb"), LOD);
			TestTrue(TEXT("Correct custom source filename recorded"), Info->SourceImportFilename.EndsWith(Suffix));
			TestFalse(TEXT("Not generated reduction masquerading as source LOD"), Info->bHasBeenSimplified);
		}
		Receipt += FString::Printf(TEXT("LOD%d triangles=%u vertices=%u sections=%d uv_channels=%u max_influences=%u screen_size=%.6f remove_degenerates=%d point_threshold_cm=%.8f bounds_min=%s bounds_max=%s source=%s\n"),
			LOD, Triangles, Vertices, Data.Sections.Num(), Data.NumTexCoords, MaxInfluences, ScreenSize, int32(Info->BuildSettings.bRemoveDegenerates), Info->BuildSettings.ThresholdPosition, *Bounds.Min.ToString(), *Bounds.Max.ToString(), *Info->SourceImportFilename);
	}
	FAutomationTestExecutionInfo Execution;
	GetExecutionInfo(Execution);
	Receipt += FString::Printf(TEXT("readback_errors=%d\ngpu_transitions_verified=false\nanimated_lod_deformation_verified=false\nperformance_verified=false\nowner_approved=false\n"), Execution.GetErrorTotal());
	TestTrue(TEXT("Read-only native receipt saved"), FFileHelper::SaveStringToFile(Receipt, *Output));
	return !HasAnyErrors();
}
#endif
