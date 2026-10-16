// Copyright Roman Empire Game. All Rights Reserved.
#include "VisualRegression.h"
#include "ImageComparer.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "DynamicRHI.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

namespace RomanVisualRegression
{
	bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("RomanVisualRegression")); }
	bool CandidatesRequested() { return FParse::Param(FCommandLine::Get(), TEXT("RomanVisualCandidates")); }
	bool UIRequested() { return FParse::Param(FCommandLine::Get(), TEXT("RomanUIVisualRegression")); }
	bool UICandidatesRequested() { return FParse::Param(FCommandLine::Get(), TEXT("RomanUIVisualCandidates")); }
	FImageTolerance Tolerance();
	FString UICaseName(int32 Width, int32 Height, int32 DPI, int32 TextScale, const TCHAR* Screen)
	{
		const bool Dimensions = (Width == 1280 && Height == 720) || (Width == 1920 && Height == 1080) || (Width == 2560 && Height == 1440);
		const FString Mode(Screen);
		const bool Known = Mode == TEXT("Tactical") || Mode == TEXT("Construction") || Mode == TEXT("Strategic") ||
			Mode == TEXT("Possession") || Mode == TEXT("Outcome") || Mode == TEXT("Settings") || Mode == TEXT("BindingCapture");
		if (!Dimensions || (DPI != 100 && DPI != 125) || (TextScale != 100 && TextScale != 200) || !Known) return FString();
		return FString::Printf(TEXT("%dx%d_%d_Text%d_%s"), Width, Height, DPI, TextScale, Screen);
	}
	bool SaveUIAndCompare(FAutomationTestBase& Test, TArrayView<const FColor> Pixels, int32 Width, int32 Height, int32 DPI, int32 TextScale, const TCHAR* Screen)
	{
		// Separate versioned UI family: accepted character code/files/thresholds remain untouched.
		const FString Name = UICaseName(Width, Height, DPI, TextScale, Screen);
		if (!Test.TestFalse(TEXT("Known UI resolution/DPI/text/screen identity"), Name.IsEmpty())) return false;
		if (!Test.TestTrue(TEXT("UI regression uses actual D3D12 raster"), GDynamicRHI && FString(GDynamicRHI->GetName()) == TEXT("D3D12"))) return false;
		if (!Test.TestTrue(TEXT("Exactly one explicit UI visual mode"), UIRequested() != UICandidatesRequested())) return false;
		if (!Test.TestEqual(TEXT("Complete actual Slate raster, never crop/rescale"), Pixels.Num(), Width * Height)) return false;
		FImage Image; Image.Init(Width, Height, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		FMemory::Memcpy(Image.RawData.GetData(), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/UIVisualRegression"), FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		IFileManager::Get().MakeDirectory(*Folder, true);
		const FString Incoming = FPaths::Combine(Folder, Name + TEXT(".png"));
		if (!Test.TestTrue(TEXT("Actual UI raster saved to unique artifact"), FImageUtils::SaveImageByExtension(*Incoming, Image))) return false;
		Test.AddInfo(TEXT("UIVisualIncoming=") + Incoming);
		if (UICandidatesRequested()) { Test.AddInfo(TEXT("UI CANDIDATE ONLY: no comparison/approval claim")); return true; }
		const FString Approved = FPaths::Combine(FPaths::ProjectDir(), TEXT("Tests/VisualBaselines/UI_v1/Win64_D3D12"), Name + TEXT(".png"));
		const int64 Size = IFileManager::Get().FileSize(*Approved), DigestSize = IFileManager::Get().FileSize(*(Approved + TEXT(".sha1")));
		if (!Test.TestTrue(TEXT("Reviewed UI baseline and bounded integrity sidecar exist"), Size > 0 && Size <= 16 * 1024 * 1024 && DigestSize > 0 && DigestSize <= 128)) return false;
		TArray<uint8> Bytes; FString Digest;
		if (!Test.TestTrue(TEXT("Read-only approved UI bytes loaded"), FFileHelper::LoadFileToArray(Bytes, *Approved) && FFileHelper::LoadFileToString(Digest, *(Approved + TEXT(".sha1"))))) return false;
		FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
		if (!Test.TestEqual(TEXT("UI baseline integrity"), Hash.ToString().ToLower(), Digest.TrimStartAndEnd().ToLower())) return false;
		FImageComparer Comparer;
		const FImageComparisonResult Result = Comparer.Compare(Approved, Incoming, Tolerance(), FPaths::Combine(Folder, Name + TEXT("-delta.png")));
		const FString Report = FString::Printf(TEXT("{\"schema\":1,\"family\":\"UI_v1\",\"view\":\"%s\",\"width\":%d,\"height\":%d,\"localError\":%.9f,\"globalError\":%.9f,\"similar\":%s,\"candidate\":false,\"localLimit\":0.01,\"globalLimit\":0.002}"),
			*Name, Width, Height, Result.MaxLocalDifference, Result.GlobalDifference, Result.AreSimilar() ? TEXT("true") : TEXT("false"));
		Test.TestTrue(TEXT("Native UI comparison receipt saved"), FFileHelper::SaveStringToFile(Report, *FPaths::Combine(Folder, Name + TEXT("-comparison.json"))));
		Test.AddInfo(FString::Printf(TEXT("UIVisualComparison %s local=%.9f global=%.9f error=%s"), *Name, Result.MaxLocalDifference, Result.GlobalDifference, *Result.ErrorMessage.ToString()));
		return Test.TestTrue(TEXT("Actual UI matches reviewed baseline within fixed native tolerance"), Result.IsValid() && Result.AreSimilar());
	}
	class FStableView : public FWorldSceneViewExtension
	{
	public:
		FStableView(const FAutoRegister& AutoRegister, UWorld* World) : FWorldSceneViewExtension(AutoRegister, World) {}
		virtual void SetupViewFamily(FSceneViewFamily& Family) override
		{
			// Standard screenshot stabilization, scoped to this PIE world; no global CVar changes.
			Family.Time = FGameTime::CreateUndilated(0.0f, 0.0f);
			Family.EngineShowFlags.SetMotionBlur(false);
			Family.EngineShowFlags.SetEyeAdaptation(false);
			Family.EngineShowFlags.SetTemporalAA(false);
			Family.EngineShowFlags.SetAntiAliasing(false);
		}
		virtual void SetupView(FSceneViewFamily&, FSceneView&) override {}
		virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
	};
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> Stabilize(UWorld* World)
	{
		return FSceneViewExtensions::NewExtension<FStableView>(World);
	}
	FImageTolerance Tolerance()
	{
		// Keep faction colors; no color/AA ignore, masking, rescaling or autoapproval.
		return FImageTolerance(8, 8, 8, 0, 0, 255, false, false, .01f, .002f);
	}
	bool SaveAndCompare(FAutomationTestBase& Test, const FImage& Image, const TCHAR* View)
	{
		const FString Name(View);
		if (!Test.TestTrue(TEXT("Known visual scenario"), Name == TEXT("CharacterProfiles") || Name == TEXT("CharacterRTS") || Name == TEXT("CharacterPossession"))) return false;
		if (!Test.TestTrue(TEXT("Regression uses actual D3D12 raster"), GDynamicRHI && FString(GDynamicRHI->GetName()) == TEXT("D3D12"))) return false;
		if (!Test.TestTrue(TEXT("Explicit candidate OR comparison mode"), Requested() != CandidatesRequested())) return false;
		if (!Test.TestTrue(TEXT("Fixed native BGRA8/sRGB resolution"), Image.SizeX == 1280 && Image.SizeY == 720 &&
			Image.Format == ERawImageFormat::BGRA8 && Image.GammaSpace == EGammaSpace::sRGB && Image.NumSlices == 1)) return false;
		const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/VisualRegression"), FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		IFileManager::Get().MakeDirectory(*Folder, true);
		const FString Incoming = FPaths::Combine(Folder, Name + TEXT(".png"));
		if (!Test.TestTrue(TEXT("Actual raster saved to new artifact"), FImageUtils::SaveImageByExtension(*Incoming, Image))) return false;
		Test.AddInfo(TEXT("VisualIncoming=") + Incoming);
		if (CandidatesRequested())
		{
			Test.AddInfo(TEXT("CANDIDATE ONLY: no regression approval/comparison claim")); return true;
		}
		const FString Approved = FPaths::Combine(FPaths::ProjectDir(), TEXT("Tests/VisualBaselines/Win64_D3D12"), Name + TEXT(".png"));
		TArray<uint8> BaselineBytes; FString Digest;
		if (!Test.TestTrue(TEXT("Reviewed baseline AND integrity sidecar exist; never autoapprove"),
			FFileHelper::LoadFileToArray(BaselineBytes, *Approved) && FFileHelper::LoadFileToString(Digest, *(Approved + TEXT(".sha1"))))) return false;
		FSHAHash Hash; FSHA1::HashBuffer(BaselineBytes.GetData(), BaselineBytes.Num(), Hash.Hash);
		if (!Test.TestEqual(TEXT("Read-only baseline integrity"), Hash.ToString().ToLower(), Digest.TrimStartAndEnd().ToLower())) return false;
		FImageComparer Comparer;
		const FImageComparisonResult Result = Comparer.Compare(Approved, Incoming, Tolerance(), FPaths::Combine(Folder, Name + TEXT("-delta.png")));
		const FString Report = FString::Printf(TEXT("{\"schema\":1,\"view\":\"%s\",\"localError\":%.9f,\"globalError\":%.9f,\"similar\":%s,\"candidate\":false,\"localLimit\":0.01,\"globalLimit\":0.002}"),
			*Name, Result.MaxLocalDifference, Result.GlobalDifference, Result.AreSimilar() ? TEXT("true") : TEXT("false"));
		Test.TestTrue(TEXT("Native comparison receipt saved"), FFileHelper::SaveStringToFile(Report, *FPaths::Combine(Folder, Name + TEXT("-comparison.json"))));
		Test.AddInfo(FString::Printf(TEXT("VisualComparison %s local=%.9f global=%.9f error=%s"), *Name, Result.MaxLocalDifference, Result.GlobalDifference, *Result.ErrorMessage.ToString()));
		return Test.TestTrue(TEXT("Actual raster matches reviewed baseline within fixed color/local/global tolerance"), Result.IsValid() && Result.AreSimilar());
	}
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIVisualIdentities, "RomanEmpireGame.VisualRegression.UIIdentities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FUIVisualIdentities::RunTest(const FString& Parameters)
{
	TSet<FString> Names;
	for (const FIntPoint Size : {FIntPoint(1280,720), FIntPoint(1920,1080), FIntPoint(2560,1440)})
		for (int32 DPI : {100,125}) for (int32 Text : {100,200})
			for (const TCHAR* Screen : {TEXT("Tactical"), TEXT("Construction"), TEXT("Strategic"), TEXT("Possession"), TEXT("Outcome"), TEXT("Settings"), TEXT("BindingCapture")})
			{
				const FString ID = RomanVisualRegression::UICaseName(Size.X, Size.Y, DPI, Text, Screen);
				TestFalse(TEXT("Required native UI case is accepted"), ID.IsEmpty());
				TestFalse(TEXT("No colliding baseline IDs"), Names.Contains(ID)); Names.Add(ID);
			}
	TestEqual(TEXT("Full supported UI matrix, not just easiest layout"), Names.Num(), 84);
	TestTrue(TEXT("Reject path injection"), RomanVisualRegression::UICaseName(1280,720,100,200,TEXT("../Tactical")).IsEmpty());
	TestTrue(TEXT("Reject rescaling/mixed aspect"), RomanVisualRegression::UICaseName(1280,1080,100,200,TEXT("Tactical")).IsEmpty());
	TestTrue(TEXT("Reject unsupported DPI"), RomanVisualRegression::UICaseName(1280,720,150,200,TEXT("Tactical")).IsEmpty());
	TestTrue(TEXT("Reject unsupported text scale"), RomanVisualRegression::UICaseName(1280,720,100,125,TEXT("Tactical")).IsEmpty());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisualComparisonContract, "RomanEmpireGame.VisualRegression.ComparisonContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FVisualComparisonContract::RunTest(const FString& Parameters)
{
	FComparableImage Approved; Approved.Width = 32; Approved.Height = 32;
	Approved.Bytes.Init(0, 32 * 32 * 4);
	for (int32 Index = 0; Index < 32 * 32; ++Index) Approved.Bytes[Index * 4 + 3] = 255;
	FComparableImage Incoming = Approved;
	FImageComparer Comparer;
	auto Compare = [&](const FComparableImage& Current, FImageTolerance Limits)
	{
		FImageComparisonResult Result = Comparer.Compare(&Approved, &Current, Limits, FString());
		// Pointer overload has no file identity; this pure fixture explicitly supplies it.
		Result.ApprovedFilePath = TEXT("PureContractApproved.png"); return Result;
	};
	TestTrue(TEXT("Identical actual pixel buffers compare"), Compare(Incoming, FImageTolerance::DefaultIgnoreNothing).AreSimilar());
	Incoming.Bytes[0] = 255;
	TestFalse(TEXT("One changed pixel fails zero tolerance"), Compare(Incoming, FImageTolerance::DefaultIgnoreNothing).AreSimilar());
	Incoming = Approved;
	for (int32 Index = 0; Index < 32 * 32 / 2; ++Index) Incoming.Bytes[Index * 4] = 255;
	TestFalse(TEXT("Real palette/geometry corruption exceeds production tolerance"), Compare(Incoming, RomanVisualRegression::Tolerance()).AreSimilar());
	Incoming.Width = 16; Incoming.Height = 64;
	TestFalse(TEXT("Dimension mismatch is not resized or accepted"), Compare(Incoming, RomanVisualRegression::Tolerance()).AreSimilar());
	FImageComparisonResult Missing;
	TestFalse(TEXT("Unapproved new image cannot pass"), Missing.AreSimilar());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisualComparisonFiles, "RomanEmpireGame.VisualRegression.FileComparisonContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FVisualComparisonFiles::RunTest(const FString& Parameters)
{
	// Synthetic codec/failure propagation contract, NOT a rendered scene or approved baseline.
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/VisualContract"), FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	IFileManager::Get().MakeDirectory(*Folder, true);
	const FString Approved = FPaths::Combine(Folder, TEXT("synthetic-approved.png"));
	const FString Incoming = FPaths::Combine(Folder, TEXT("synthetic-incoming.png"));
	FImage Image; Image.Init(32, 32, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	for (FColor& Pixel : Image.AsBGRA8()) Pixel = FColor::Black;
	if (!TestTrue(TEXT("Synthetic approved codec input saved"), FImageUtils::SaveImageByExtension(*Approved, Image))) return false;
	if (!TestTrue(TEXT("Synthetic incoming codec input saved"), FImageUtils::SaveImageByExtension(*Incoming, Image))) return false;
	FImageComparer Comparer;
	TestTrue(TEXT("Native file decode/comparison accepts identical pixels"), Comparer.Compare(Approved, Incoming, RomanVisualRegression::Tolerance(), FString()).AreSimilar());
	for (int32 Index = 0; Index < Image.AsBGRA8().Num() / 2; ++Index) Image.AsBGRA8()[Index] = FColor::Red;
	if (!TestTrue(TEXT("Synthetic visibly different file saved"), FImageUtils::SaveImageByExtension(*Incoming, Image))) return false;
	const FImageComparisonResult Changed = Comparer.Compare(Approved, Incoming, RomanVisualRegression::Tolerance(), FPaths::Combine(Folder, TEXT("synthetic-delta.png")));
	TestFalse(TEXT("File-based palette corruption fails actual production comparator"), Changed.AreSimilar());
	TestTrue(TEXT("Negative probe actually measures excess global error"), Changed.GlobalDifference > .002);
	TestFalse(TEXT("Missing approved file cannot silently approve new image"), Comparer.Compare(FPaths::Combine(Folder, TEXT("missing.png")), Incoming, RomanVisualRegression::Tolerance(), FString()).AreSimilar());
	const FString Corrupt = FPaths::Combine(Folder, TEXT("corrupt.png"));
	TestTrue(TEXT("Synthetic corrupt fixture saved"), FFileHelper::SaveStringToFile(TEXT("not a PNG"), *Corrupt));
	TestFalse(TEXT("Corrupt incoming file cannot pass"), Comparer.Compare(Approved, Corrupt, RomanVisualRegression::Tolerance(), FString()).AreSimilar());
	if (FParse::Param(FCommandLine::Get(), TEXT("RomanUIProbe")))
	{
		// Directed actual-candidate reproducibility diagnosis, NEVER reviewed regression approval.
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/UIVisualRegression")));
		TSet<FString> Screens;
		for (int32 Index = 0; Index < 7; ++Index)
		{
			FString A, B;
			if (!TestTrue(TEXT("All seven actual candidate pairs supplied"),
				FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("RomanUIProbeA%d="), Index), A) &&
				FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("RomanUIProbeB%d="), Index), B))) return false;
			A = FPaths::ConvertRelativePathToFull(A); B = FPaths::ConvertRelativePathToFull(B);
			if (!TestTrue(TEXT("Independent owned candidate paths, not self-comparison"), FPaths::IsUnderDirectory(A, Root) && FPaths::IsUnderDirectory(B, Root) && !A.Equals(B, ESearchCase::IgnoreCase))) return false;
			const FString ID = FPaths::GetBaseFilename(A);
			if (!TestEqual(TEXT("Exact same native candidate case"), FPaths::GetCleanFilename(A), FPaths::GetCleanFilename(B)) ||
				!TestFalse(TEXT("Unique candidate pair IDs"), Screens.Contains(ID))) return false;
			Screens.Add(ID);
			const int64 SizeA = IFileManager::Get().FileSize(*A), SizeB = IFileManager::Get().FileSize(*B);
			if (!TestTrue(TEXT("Bounded actual candidate PNGs"), SizeA > 0 && SizeA <= 16 * 1024 * 1024 && SizeB > 0 && SizeB <= 16 * 1024 * 1024)) return false;
			const FImageComparisonResult Result = Comparer.Compare(A, B, RomanVisualRegression::Tolerance(), FPaths::Combine(Folder, ID + TEXT("-probe-delta.png")));
			AddInfo(FString::Printf(TEXT("UIReproducibility %s local=%.9f global=%.9f error=%s"), *ID, Result.MaxLocalDifference, Result.GlobalDifference, *Result.ErrorMessage.ToString()));
			TestTrue(TEXT("Independent actual UI candidates reproduce within UNCHANGED tolerance"), Result.IsValid() && Result.AreSimilar());
		}
		TestEqual(TEXT("Seven native candidate comparisons, no approval claim"), Screens.Num(), 7);
	}
	return true;
}
#endif
