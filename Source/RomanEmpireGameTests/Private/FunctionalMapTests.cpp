// Copyright Roman Empire Game. All Rights Reserved.
#include "RomanFunctionalScenario.h"
#include "RomanCampaignFunctional.h"

#include "AssetToolsModule.h"
#include "Editor.h"
#include "FunctionalTestBase.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "IAssetTools.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "Modules/ModuleManager.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanFunctionalMaps
{
	const TCHAR* Names[] = {TEXT("FT_Navigation"), TEXT("FT_Combat"), TEXT("FT_Production"), TEXT("FT_Possession"), TEXT("FT_Victory"), TEXT("FT_CampaignHandoff")};
	/** Engine default suppresses logs while no FunctionalTest actor is active.
	 * Our cross-world coordinator continues after that actor has succeeded/died.
	 * Capture all runtime errors/warnings for the entire owned test instead.
	 */
	class FBridgeTestBase : public FFunctionalTestBase
	{
	public:
		FBridgeTestBase(const FString& Name, bool Complex) : FFunctionalTestBase(Name, Complex)
		{
			// Engine's constructor elevates warnings before the first actor RunTest
			// (its project defaults cannot be read during static test registration).
			// Match native automation for map/PIE setup without hiding ANY logs.
			// RunTest still applies the actor's explicit/project warning policy.
			bElevateLogWarningsToErrors = false;
		}
		virtual bool SuppressLogs() override { return false; }
		void PrepareOwnedSession()
		{
			// Complex variants reuse this test instance. The preceding actor may
			// have installed a different policy; reset only BEFORE the next PIE.
			SetLogErrorAndWarningHandling(false, false, false);
		}
	};
	FString Path(int32 Scenario) { return FString(TEXT("/Game/Tests/Maps/")) + Names[Scenario]; }
	FString SHA(const FString& File)
	{
		FSHAHash Hash;
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *File)) return FString();
		FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
		return Hash.ToString();
	}
	ARomanFunctionalScenario* FindOwned(UWorld* World, int32 Scenario)
	{
		ARomanFunctionalScenario* Found = nullptr;
		if (!World) return nullptr;
		for (TActorIterator<AFunctionalTest> It(World); It; ++It)
		{
			ARomanFunctionalScenario* Candidate = Cast<ARomanFunctionalScenario>(*It);
			if (Found || !Candidate || Candidate->SchemaVersion != 1 || Candidate->Scenario != Scenario) return nullptr;
			Found = Candidate;
		}
		return Found;
	}
	class FRun : public IAutomationLatentCommand
	{
	public:
		FRun(FAutomationTestBase* InTest, int32 InScenario, bool InFailureProbe = false, bool InLogProbe = false)
			: Test(*InTest), Scenario(InScenario), Started(FPlatformTime::Seconds()), bFailureProbe(InFailureProbe), bLogProbe(InLogProbe) {}
		virtual bool Update() override
		{
			if (FPlatformTime::Seconds() - Started > 150.0)
			{
				Test.AddError(TEXT("Dedicated FunctionalTest fixture timed out."));
				return true;
			}
			UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
			if (!World) return false;
			if (Travel.IsValid())
			{
				const bool Complete = Travel->Update();
				if (Complete)
				{
					Test.TestTrue(TEXT("Real travel replaced the source world"), World != SourceWorld.Get());
					Test.TestFalse(TEXT("Source FunctionalTest actor is not retained across travel"), Actor.IsValid());
					Travel.Reset();
				}
				return Complete;
			}
			if (!Actor.IsValid())
			{
				if (bStarted) { Test.AddError(TEXT("FunctionalTest actor disappeared before reporting its outcome.")); return true; }
				Actor = FindOwned(World, Scenario);
				if (!Test.TestNotNull(TEXT("Exactly one schema1 functional actor in the persistent dedicated map"), Actor.Get())) return true;
				bStarted = true;
				if (bFailureProbe) Actor->Scenario = -1; // Transient PIE copy only; source map remains unchanged.
				if (!Test.TestTrue(TEXT("Real AFunctionalTest accepts RunTest"), Actor->RunTest())) return true;
				return false;
			}
			if (Actor->IsRunning()) return false;
			if (Actor->Result == EFunctionalTestResult::Default) return false;
			const bool Passed = Test.TestEqual(TEXT("FunctionalTest native terminal outcome, not log text"), Actor->Result,
				bFailureProbe ? EFunctionalTestResult::Failed : EFunctionalTestResult::Succeeded);
			Actor->CleanUp();
			if (bFailureProbe && Scenario == 5)
			{
				const UCampaignSessionSubsystem* Session = World->GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
				if (Test.TestNotNull(TEXT("Campaign failure probe has real coordinator authority"), Session))
				{
					Test.TestFalse(TEXT("Failed functional gate installs no transfer"), Session->HasPendingTransfer());
					Test.TestFalse(TEXT("Failed functional gate creates no campaign session"), Session->HasSession());
				}
			}
			if (!Passed || bFailureProbe || Scenario != 5) return true;
			Test.TestFalse(TEXT("Source functional phase ended before coordinator starts"), FFunctionalTestBase::IsFunctionalTestRunning());
			if (bLogProbe)
			{
				// Registered expected error must actually reach automation AFTER the
				// actor phase. A successful actor is never allowed to mask this log.
				UE_LOG(LogTemp, Error, TEXT("RomanCampaignHandoffLogProbe"));
				return true;
			}
			FAutomationTestExecutionInfo Info; Test.GetExecutionInfo(Info);
			if (Info.GetErrorTotal() != 0) return true;
			SourceWorld = World;
			Travel = CreateRomanFunctionalCampaignTravel(Test);
			if (!Test.TestTrue(TEXT("Existing cross-world coordinator created only after successful phase gate"), Travel.IsValid())) return true;
			return false;
		}
	private:
		FAutomationTestBase& Test;
		int32 Scenario;
		double Started;
		bool bStarted = false;
		bool bFailureProbe = false;
		bool bLogProbe = false;
		TWeakObjectPtr<ARomanFunctionalScenario> Actor;
		TWeakObjectPtr<UWorld> SourceWorld;
		TSharedPtr<IAutomationLatentCommand> Travel;
	};
	class FWaitEnd : public IAutomationLatentCommand
	{
	public:
		explicit FWaitEnd(FAutomationTestBase* InTest) : Test(*InTest), Started(FPlatformTime::Seconds()) {}
		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld) return true;
			if (FPlatformTime::Seconds() - Started < 30.0) return false;
			Test.AddError(TEXT("Functional fixture PIE did not terminate.")); return true;
		}
	private:
		FAutomationTestBase& Test;
		double Started;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanFunctionalMapsReadback, "RomanEmpireGame.Functional.MapsReadback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanFunctionalMapsReadback::RunTest(const FString& Parameters)
{
	using namespace RomanFunctionalMaps;
	if (!TestNotNull(TEXT("Intentional editor session required"), GEditor) ||
		!TestNull(TEXT("No PIE during fixture asset validation/authoring"), GEditor->PlayWorld)) return false;
	bool Excluded = false;
	for (const FDirectoryPath& Directory : GetDefault<UProjectPackagingSettings>()->DirectoriesToNeverCook)
		Excluded |= Directory.Path == TEXT("/Game/Tests");
	if (!TestTrue(TEXT("All test fixtures explicitly excluded from distributed cooking"), Excluded)) return false;
	const FString SourcePath = TEXT("/Game/Maps/L_Testbed");
	const FString SourceFile = FPackageName::LongPackageNameToFilename(SourcePath, FPackageName::GetMapPackageExtension());
	const FString Before = SHA(SourceFile);
	if (!TestFalse(TEXT("Existing canonical source map has readable bytes"), Before.IsEmpty())) return false;
	const bool Author = FParse::Param(FCommandLine::Get(), TEXT("RomanAuthorFunctionalMaps"));
	TArray<int32> Missing;
	TArray<TPair<FString, FString>> Existing;
	// Preflight EVERY target before creating anything; never repair or overwrite.
	for (int32 Scenario = 0; Scenario < UE_ARRAY_COUNT(Names); ++Scenario)
	{
		const FString File = FPackageName::LongPackageNameToFilename(Path(Scenario), FPackageName::GetMapPackageExtension());
		if (!IFileManager::Get().FileExists(*File)) { Missing.Add(Scenario); continue; }
		UWorld* World = LoadObject<UWorld>(nullptr, *(Path(Scenario) + TEXT(".") + Names[Scenario]));
		if (!TestNotNull(TEXT("Existing fixture ownership/schema/discriminator valid; refuse foreign content"), FindOwned(World, Scenario))) return false;
		ARomanFunctionalScenario* Owned = FindOwned(World, Scenario);
		Owned->SchemaVersion = 2;
		TestNull(TEXT("Read-only preflight rejects unsupported fixture schema"), FindOwned(World, Scenario));
		Owned->SchemaVersion = 1;
		Owned->Scenario = INDEX_NONE;
		TestNull(TEXT("Read-only preflight rejects foreign scenario identity"), FindOwned(World, Scenario));
		Owned->Scenario = Scenario;
		Existing.Emplace(File, SHA(File));
	}
	if (!Missing.IsEmpty() && !Author) { AddError(TEXT("Functional maps missing; creation requires explicit -RomanAuthorFunctionalMaps.")); return false; }
	if (!Missing.IsEmpty())
	{
		UWorld* Source = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/L_Testbed.L_Testbed"));
		if (!TestNotNull(TEXT("Load exact existing source without saving it"), Source)) return false;
		IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		for (int32 Scenario : Missing)
		{
			UWorld* World = Cast<UWorld>(Tools.DuplicateAsset(Names[Scenario], TEXT("/Game/Tests/Maps"), Source));
			if (!TestNotNull(TEXT("Create missing dedicated non-destructive fixture copy"), World)) return false;
			ARomanFunctionalScenario* Actor = World->SpawnActor<ARomanFunctionalScenario>();
			if (!TestNotNull(TEXT("Persistent native FunctionalTest actor placed"), Actor)) return false;
			Actor->Scenario = Scenario;
			Actor->SetActorLabel(Names[Scenario]);
			FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
			const FString File = FPackageName::LongPackageNameToFilename(Path(Scenario), FPackageName::GetMapPackageExtension());
			if (!TestTrue(TEXT("Save only explicitly authorized new fixture map"), UPackage::SavePackage(World->GetOutermost(), World, *File, Args))) return false;
			if (!TestNotNull(TEXT("Created fixture can be queried without further mutation"), FindOwned(World, Scenario))) return false;
		}
	}
	TestEqual(TEXT("Canonical source map byte-for-byte intact"), SHA(SourceFile), Before);
	for (const auto& Entry : Existing) TestEqual(TEXT("Read-only/create-missing run never changes an existing target"), SHA(Entry.Key), Entry.Value);
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FRomanFunctionalLogPolicy, RomanFunctionalMaps::FBridgeTestBase, "RomanEmpireGame.Functional.LogPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanFunctionalLogPolicy::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Capture logs even outside the actor phase"), SuppressLogs());
	TestFalse(TEXT("Never suppress runtime errors"), SuppressLogErrors());
	TestFalse(TEXT("Never suppress runtime warnings"), SuppressLogWarnings());
	TestFalse(TEXT("PIE setup warnings retain warning severity"), ElevateLogWarningsToErrors());
	SetLogErrorAndWarningHandling(false, false, true);
	TestTrue(TEXT("Explicit actor policy can still elevate warnings"), ElevateLogWarningsToErrors());
	TestFalse(TEXT("Explicit policy still retains errors"), SuppressLogErrors());
	TestFalse(TEXT("Explicit policy still retains warnings"), SuppressLogWarnings());
	PrepareOwnedSession();
	TestFalse(TEXT("Next owned session resets prior actor elevation"), ElevateLogWarningsToErrors());
	TestFalse(TEXT("Next owned session still captures errors"), SuppressLogErrors());
	TestFalse(TEXT("Next owned session still captures warnings"), SuppressLogWarnings());
	return true;
}

IMPLEMENT_CUSTOM_COMPLEX_AUTOMATION_TEST(FRomanDedicatedFunctionalMaps, RomanFunctionalMaps::FBridgeTestBase, "RomanEmpireGame.Functional.Scenarios",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FRomanDedicatedFunctionalMaps::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(RomanFunctionalMaps::Names); ++Index)
	{
		Names.Add(RomanFunctionalMaps::Names[Index]); Commands.Add(FString::FromInt(Index));
	}
	Names.Add(TEXT("FailurePropagation")); Commands.Add(TEXT("failure"));
	Names.Add(TEXT("LogCaptureAfterPhase")); Commands.Add(TEXT("logprobe"));
	Names.Add(TEXT("CampaignGateFailure")); Commands.Add(TEXT("campaignfailure"));
}
bool FRomanDedicatedFunctionalMaps::RunTest(const FString& Parameters)
{
	PrepareOwnedSession();
	int32 Scenario = INDEX_NONE;
	const bool CampaignFailure = Parameters == TEXT("campaignfailure");
	const bool FailureProbe = Parameters == TEXT("failure") || CampaignFailure;
	const bool LogProbe = Parameters == TEXT("logprobe");
	if (FailureProbe) Scenario = CampaignFailure ? 5 : 0;
	if (LogProbe) Scenario = 5;
	if (!TestTrue(TEXT("Known fixture variant only"), FailureProbe || LogProbe || (LexTryParseString(Scenario, *Parameters) && Scenario >= 0 && Scenario < UE_ARRAY_COUNT(RomanFunctionalMaps::Names)))) return false;
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Own fresh PIE"), GEditor->PlayWorld)) return false;
	const FString Path = RomanFunctionalMaps::Path(Scenario);
	FAutomationEditorCommonUtils::LoadMap(Path);
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestTrue(TEXT("Persistent dedicated map loaded"), World && World->GetName() == RomanFunctionalMaps::Names[Scenario]) ||
		!TestNotNull(TEXT("Fixture preflight"), RomanFunctionalMaps::FindOwned(World, Scenario))) return false;
	if (FailureProbe) AddExpectedError(TEXT("Deliberate functional assertion probe"), EAutomationExpectedErrorFlags::Contains, 1);
	if (LogProbe) AddExpectedError(TEXT("RomanCampaignHandoffLogProbe"), EAutomationExpectedErrorFlags::Contains, 1);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanFunctionalMaps::FRun(this, Scenario, FailureProbe, LogProbe));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(RomanFunctionalMaps::FWaitEnd(this));
	return true;
}
#endif
