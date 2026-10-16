// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Audio/RomanFeedbackSubsystem.h"
#include "Components/AudioComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/RomanPlayerPreferences.h"
#include "UI/RomanPlayerSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanPreferencesCodecTest,"RomanEmpireGame.Preferences.StrictCodec",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanPreferencesCodecTest::RunTest(const FString& Parameters)
{
	FRomanPlayerPreferences Value; Value.MasterVolume=.4f; Value.LookSensitivity=2.5f; Value.TextScale=1.5f; Value.bInvertLookY=true;
	FString JSON; TestTrue(TEXT("Encode valid user values"),FRomanPreferencesCodec::Encode(Value,JSON));
	FRomanPlayerPreferences Result;
	TestTrue(TEXT("Decode actual schema1 data"),FRomanPreferencesCodec::Decode(JSON,Result));
	TestTrue(TEXT("Exact float/bool roundtrip"),Result==Value);
	const FString Invalid[] = {TEXT("{}"),TEXT("null"),TEXT("broken"),JSON.Replace(TEXT("\"Schema\": 1"),TEXT("\"Schema\": 2")),JSON.Replace(TEXT("true"),TEXT("\"true\""))};
	for(const FString& Bad:Invalid) { Result=Value; TestFalse(TEXT("Reject incomplete/unknown/wrong-type data"),FRomanPreferencesCodec::Decode(Bad,Result)); TestTrue(TEXT("Rejected decode preserves output"),Result==Value); }
	Result=Value; TestFalse(TEXT("Bound malformed oversized input"),FRomanPreferencesCodec::Decode(FString::ChrN(FRomanPreferencesCodec::MaxBytes+1,TEXT('x')),Result));
	for(float Bad:{-1.f,2.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
	{ FRomanPlayerPreferences InvalidValue=Value; InvalidValue.MasterVolume=Bad; FString Retained=TEXT("unchanged"); TestFalse(TEXT("Reject invalid numbers before encoding"),FRomanPreferencesCodec::Encode(InvalidValue,Retained)); TestEqual(TEXT("Failed encode preserves output"),Retained,FString(TEXT("unchanged"))); }
	FRomanPlayerPreferences Limit; Limit.LookSensitivity=.09f; TestFalse(TEXT("Reject low sensitivity"),Limit.IsValid()); Limit=FRomanPlayerPreferences(); Limit.TextScale=2.01f; TestFalse(TEXT("Reject unusable text size"),Limit.IsValid());
	return !HasAnyErrors();
}

namespace
{
struct FPreferenceLifetime { TWeakObjectPtr<URomanPlayerSettingsSubsystem> Settings; };
class FCheckPreferencesLifecycle : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FPreferenceLifetime> Lifetime; double Started=FPlatformTime::Seconds();
public:
	FCheckPreferencesLifecycle(FAutomationTestBase* InTest,TSharedRef<FPreferenceLifetime> InLifetime):Test(*InTest),Lifetime(InLifetime){}
	virtual bool Update() override
	{
		UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
		if (!World || !World->HasBegunPlay() || !World->GetGameInstance()) { if(FPlatformTime::Seconds()-Started>30){Test.AddError(TEXT("Preferences PIE startup timed out"));return true;} return false; }
		UGameInstance* Instance=World->GetGameInstance(); Lifetime->Settings=Instance->GetSubsystem<URomanPlayerSettingsSubsystem>();
		if (!Test.TestTrue(TEXT("Automatic real game-instance subsystem"),Lifetime->Settings.IsValid() && Lifetime->Settings->GetGameInstance()==Instance)) return true;
		URomanPlayerSettingsSubsystem* Settings=Lifetime->Settings.Get();
		const FRomanPlayerPreferences Before=Settings->GetPreferences(); Test.TestTrue(TEXT("Actual startup values fully valid"),Before.IsValid());
		URomanFeedbackSubsystem* Audio=World->GetSubsystem<URomanFeedbackSubsystem>();
		if (!Test.TestNotNull(TEXT("Real world audio projection"),Audio)) return true;
		Test.TestEqual(TEXT("Audio startup master reads preferences"),Audio->GetMasterLevel(),Before.MasterVolume);
		Test.TestEqual(TEXT("Audio startup effects reads preferences"),Audio->GetEffectsLevel(),Before.EffectsVolume);
		UUserWidget* Root=nullptr;
		APlayerController* Controller=World->GetFirstPlayerController();
		UFunction* GetRoot=Controller?Controller->FindFunction(TEXT("GetMainWidget")):nullptr;
		if (GetRoot)
		{
			FStructOnScope Params(GetRoot); Controller->ProcessEvent(GetRoot,Params.GetStructMemory());
			if(FObjectPropertyBase* Return=FindFProperty<FObjectPropertyBase>(GetRoot,TEXT("ReturnValue")))
				Root=Cast<UUserWidget>(Return->GetObjectPropertyValue_InContainer(Params.GetStructMemory()));
		}
		UTextBlock* Resource=Root?Cast<UTextBlock>(Root->GetWidgetFromName(TEXT("ResourceText"))):nullptr;
		if (!Test.TestNotNull(TEXT("Actual production resource label"),Resource)) return true;
		const float InitialFont=Resource->GetFont().Size;
		FRomanPlayerPreferences Expected=Before; Expected.TextScale=Before.TextScale==1.25f?1.5f:1.25f; Expected.MasterVolume=.3f; Expected.EffectsVolume=.4f;
		int32 Events=0;
		const FDelegateHandle Listener=Settings->OnPreferencesChanged().AddLambda([&]{++Events;Test.TestTrue(TEXT("Real owner publishes postcommit query"),Settings->GetPreferences()==Expected);});
		Test.TestEqual(TEXT("Real GI preview without personal disk writes"),Settings->TryApply(Expected,false),ERomanPreferencesFailure::None);
		Test.TestEqual(TEXT("Committed master reaches live audio service"),Audio->GetMasterLevel(),Expected.MasterVolume);
		Test.TestEqual(TEXT("Committed effects reaches live audio service"),Audio->GetEffectsLevel(),Expected.EffectsVolume);
		Test.TestTrue(TEXT("Committed scale changes actual layout font size"),FMath::IsNearlyEqual(Resource->GetFont().Size,InitialFont/Before.TextScale*Expected.TextScale));
		Test.TestTrue(TEXT("Readable text uses layout font rather than clipped render scaling"),Resource->GetRenderTransform().Scale.Equals(FVector2D(1,1)));
		if (FArrayProperty* Pool=FindFProperty<FArrayProperty>(Audio->GetClass(),TEXT("Voices")))
		{
			FScriptArrayHelper Voices(Pool,Pool->ContainerPtrToValuePtr<void>(Audio));
			FObjectPropertyBase* Inner=CastField<FObjectPropertyBase>(Pool->Inner);
			Test.TestEqual(TEXT("Actual bounded world voice pool"),Voices.Num(),8);
			if (Test.TestNotNull(TEXT("Actual owned audio objects"),Inner)) for(int32 Index=0;Index<Voices.Num();++Index)
			{
				UAudioComponent* Voice=Cast<UAudioComponent>(Inner->GetObjectPropertyValue(Voices.GetRawPtr(Index)));
				if(Test.TestNotNull(TEXT("Actual live voice component"),Voice)) Test.TestTrue(TEXT("Committed preferences update every actual voice"),FMath::IsNearlyEqual(Voice->VolumeMultiplier,Expected.MasterVolume*Expected.EffectsVolume));
			}
		}
		else Test.AddError(TEXT("Audio voice property unavailable"));
		Expected=Before; Test.TestEqual(TEXT("Fixture restores original user preferences"),Settings->TryApply(Before,false),ERomanPreferencesFailure::None);
		Test.TestTrue(TEXT("Restored typography does not compound scaling"),FMath::IsNearlyEqual(Resource->GetFont().Size,InitialFont));
		Test.TestEqual(TEXT("Exactly two real changes"),Events,2); Settings->OnPreferencesChanged().Remove(Listener); return true;
	}
};
class FCheckPreferencesReleased : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FPreferenceLifetime> Lifetime; double Started=FPlatformTime::Seconds();
public:
	FCheckPreferencesReleased(FAutomationTestBase* InTest,TSharedRef<FPreferenceLifetime> InLifetime):Test(*InTest),Lifetime(InLifetime){}
	virtual bool Update() override
	{
		if(GEditor->PlayWorld){if(FPlatformTime::Seconds()-Started>30){Test.AddError(TEXT("Preferences PIE teardown timed out"));return true;}return false;}
		CollectGarbage(RF_NoFlags); Test.TestFalse(TEXT("Actual preferences owner collected on PIE teardown"),Lifetime->Settings.IsValid()); return true;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanPreferencesLifecycleTest,"RomanEmpireGame.Preferences.LifecyclePIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanPreferencesLifecycleTest::RunTest(const FString& Parameters)
{
	if(!TestNotNull(TEXT("Editor"),GEditor)||!TestNull(TEXT("Own fresh PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	const TSharedRef<FPreferenceLifetime> Lifetime=MakeShared<FPreferenceLifetime>();
	AddCommand(new FStartPIECommand(false));AddCommand(new FCheckPreferencesLifecycle(this,Lifetime));
	AddCommand(new FEndPlayMapCommand());AddCommand(new FCheckPreferencesReleased(this,Lifetime));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanPreferencesFileTest,"RomanEmpireGame.Preferences.RealFile",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanPreferencesFileTest::RunTest(const FString& Parameters)
{
	const FString Path=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Preferences"),FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json")));
	ON_SCOPE_EXIT { IFileManager::Get().Delete(*Path,false,true); IFileManager::Get().Delete(*(Path+TEXT(".previous")),false,true); IFileManager::Get().Delete(*(Path+TEXT(".rejected")),false,true); };
	FRomanPlayerPreferences Value; Value.TextScale=1.25f; Value.PanSensitivity=2; Value.bInvertLookY=true;
	TestTrue(TEXT("Actual staged write/readback/replace"),FRomanPreferencesFile::Write(Path,Value));
	FRomanPlayerPreferences Loaded;
	TestTrue(TEXT("Fresh actual persisted read"),FRomanPreferencesFile::Read(Path,Loaded)); TestTrue(TEXT("Persisted values exact"),Loaded==Value);
	FString Before; FFileHelper::LoadFileToString(Before,*Path);
	FRomanPlayerPreferences Bad=Value; Bad.MasterVolume=-1; TestFalse(TEXT("Invalid save rejected"),FRomanPreferencesFile::Write(Path,Bad));
	FString After; FFileHelper::LoadFileToString(After,*Path); TestEqual(TEXT("Invalid save preserves original file"),After,Before);
	Value.LookSensitivity=4.5f; TestTrue(TEXT("Existing owned file replacement"),FRomanPreferencesFile::Write(Path,Value)); TestTrue(TEXT("Read replaced file"),FRomanPreferencesFile::Read(Path,Loaded)); TestTrue(TEXT("Replacement values exact"),Loaded==Value);
	TestTrue(TEXT("Previous bytes remain independently recoverable"),FRomanPreferencesFile::Read(Path+TEXT(".previous"),Loaded)); TestTrue(TEXT("Previous values differ from committed replacement"),Loaded.LookSensitivity==1.f && Loaded.TextScale==Value.TextScale);
	TestTrue(TEXT("Fixture writes malformed owned file"),FFileHelper::SaveStringToFile(TEXT("broken"),*Path));
	Loaded=Value; TestFalse(TEXT("Corrupt preferences reject"),FRomanPreferencesFile::Read(Path,Loaded)); TestTrue(TEXT("Corrupt file never changes output"),Loaded==Value);
	TestTrue(TEXT("Explicit apply can replace malformed current"),FRomanPreferencesFile::Write(Path,Value));
	TestTrue(TEXT("Corrupt current does not destroy valid previous backup"),FRomanPreferencesFile::Read(Path+TEXT(".previous"),Loaded)); TestTrue(TEXT("Last good prior values preserved"),Loaded.LookSensitivity==1.f);
	FString Rejected; TestTrue(TEXT("Malformed previous current preserved separately"),FFileHelper::LoadFileToString(Rejected,*(Path+TEXT(".rejected")))); TestEqual(TEXT("Original rejected bytes recoverable"),Rejected,FString(TEXT("broken")));
	TestFalse(TEXT("Reject path outside Saved before file IO"),FRomanPreferencesFile::Write(FPaths::Combine(FPaths::ProjectDir(),TEXT("DoNotCreatePreferences.json")),Value));
	TestFalse(TEXT("Reject Saved traversal before file IO"),FRomanPreferencesFile::Write(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("../DoNotCreatePreferences.json")),Value));
	TestFalse(TEXT("No file created outside owned settings root"),IFileManager::Get().FileExists(*FPaths::Combine(FPaths::ProjectDir(),TEXT("DoNotCreatePreferences.json"))));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanPreferencesOwnerTest,"RomanEmpireGame.Preferences.AtomicOwner",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanPreferencesOwnerTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
	TStrongObjectPtr<URomanPlayerSettingsSubsystem> Settings(NewObject<URomanPlayerSettingsSubsystem>(Owner.Get()));
	int32 Events=0; ERomanPreferencesFailure Nested=ERomanPreferencesFailure::None; FRomanPlayerPreferences Observed;
	Settings->OnPreferencesChanged().AddLambda([&] { ++Events; Observed=Settings->GetPreferences(); Nested=Settings->TryApply(FRomanPlayerPreferences(),false); });
	const FRomanPlayerPreferences Before=Settings->GetPreferences(); FRomanPlayerPreferences Value=Before; Value.EffectsVolume=.2f;
	TestEqual(TEXT("Uninitialized persistent destination safely rejects"),Settings->TryApply(Value),ERomanPreferencesFailure::IOError);
	TestTrue(TEXT("IO failure preserves canonical preferences"),Settings->GetPreferences()==Before); TestEqual(TEXT("No event on failed persist"),Events,0);
	TestEqual(TEXT("Validated detached apply"),Settings->TryApply(Value,false),ERomanPreferencesFailure::None);
	TestTrue(TEXT("Postcommit observer reads live canonical data"),Observed==Value); TestEqual(TEXT("Nested write rejected"),Nested,ERomanPreferencesFailure::Busy);
	TestEqual(TEXT("One change event"),Events,1); Settings->TryApply(Value,false); TestEqual(TEXT("Idempotent apply no extra event"),Events,1);
	FRomanPlayerPreferences Invalid=Value; Invalid.PanSensitivity=0;
	TestEqual(TEXT("Invalid mutation rejects"),Settings->TryApply(Invalid,false),ERomanPreferencesFailure::InvalidValue);
	TestTrue(TEXT("Invalid mutation preserves all values"),Settings->GetPreferences()==Value); TestEqual(TEXT("No invalidation for rejected mutation"),Events,1);
	Settings->OnPreferencesChanged().Clear(); // Lambda references only this owned fixture.
	return !HasAnyErrors();
}
#endif
