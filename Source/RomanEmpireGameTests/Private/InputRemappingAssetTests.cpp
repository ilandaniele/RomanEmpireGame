// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Input/RomanInputConfig.h"
#include "Editor.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "PlayerMappableKeySettings.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanRemapAuthoring
{
const FName SchemaTag(TEXT("RomanRemappableKeysSchema"));
struct FExpected { UInputMappingContext* Context; FString Category; const UInputAction* Action; FKey Key; FName Name; FText Label; };
TArray<FExpected> Recipe(URomanInputConfig* Config)
{
	TArray<FExpected> Entries;
	const auto Add=[&](UInputMappingContext* Context,const TCHAR* Category,const UInputAction* Action,FKey Key,const FString& Role)
	{
		const FString ID=FString::Printf(TEXT("Roman.v1.%s.%s"),Category,*Role);
		Entries.Add({Context,Category,Action,Key,FName(*ID),FText::FromString(Role.Replace(TEXT("_"),TEXT(" ")))});
	};
	for(int32 Mode=0;Mode<3;++Mode)
	{
		UInputMappingContext* Context=Mode==0?Config->StrategicContext.Get():Mode==1?Config->TacticalContext.Get():Config->PossessionContext.Get();
		const TCHAR* Category=Mode==0?TEXT("Strategic"):Mode==1?TEXT("Tactical"):TEXT("Possession");
		const UInputAction* Movement=Mode==2?Config->SoldierMove.Get():Config->CameraPan.Get();
		Add(Context,Category,Movement,EKeys::W,TEXT("Move_forward")); Add(Context,Category,Movement,EKeys::A,TEXT("Move_left"));
		Add(Context,Category,Movement,EKeys::S,TEXT("Move_back")); Add(Context,Category,Movement,EKeys::D,TEXT("Move_right"));
		if(Mode<2) Add(Context,Category,Config->EndTurn,EKeys::T,TEXT("End_turn"));
	}
	Add(Config->TacticalContext,TEXT("Tactical"),Config->Select,EKeys::LeftMouseButton,TEXT("Select"));
	Add(Config->TacticalContext,TEXT("Tactical"),Config->SelectionModifier,EKeys::LeftShift,TEXT("Add_selection_left"));
	Add(Config->TacticalContext,TEXT("Tactical"),Config->SelectionModifier,EKeys::RightShift,TEXT("Add_selection_right"));
	Add(Config->TacticalContext,TEXT("Tactical"),Config->Command,EKeys::RightMouseButton,TEXT("Command"));
	Add(Config->TacticalContext,TEXT("Tactical"),Config->EnterPossession,EKeys::F,TEXT("Enter_possession"));
	Add(Config->TacticalContext,TEXT("Tactical"),Config->BuildMenu,EKeys::B,TEXT("Build_menu"));
	const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six};
	for(int32 Index=0;Index<6;++Index) Add(Config->TacticalContext,TEXT("Tactical"),Config->BuildingSlots[Index],Keys[Index],FString::Printf(TEXT("Building_slot_%d"),Index+1));
	Add(Config->PossessionContext,TEXT("Possession"),Config->Attack,EKeys::LeftMouseButton,TEXT("Attack"));
	Add(Config->PossessionContext,TEXT("Possession"),Config->Block,EKeys::RightMouseButton,TEXT("Block"));
	Add(Config->PossessionContext,TEXT("Possession"),Config->ExitPossession,EKeys::F,TEXT("Exit_possession"));
	return Entries;
}
FEnhancedActionKeyMapping* MappingFor(const FExpected& Expected)
{
	for(int32 Index=0;Index<Expected.Context->GetMappings().Num();++Index)
	{
		auto& Mapping=Expected.Context->GetMapping(Index);
		if(Mapping.Action==Expected.Action && Mapping.Key==Expected.Key) return &Mapping;
	}
	return nullptr;
}
bool Matches(const FExpected& Expected,const UPlayerMappableKeySettings* Settings)
{
	return Settings && Settings->GetClass()==UPlayerMappableKeySettings::StaticClass() && Settings->GetOuter()==Expected.Context && Settings->Name==Expected.Name && Settings->DisplayName.EqualTo(Expected.Label) &&
		Settings->DisplayCategory.ToString()==Expected.Category && !Settings->Metadata && Settings->SupportedKeyProfileIds.IsEmpty();
}
FString FileSHA(const FString& Filename)
{
	TArray<uint8> Bytes; if(!FFileHelper::LoadFileToArray(Bytes,*Filename)) return FString();
	FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash); return Hash.ToString().ToUpper();
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanInputRemappingMetadataTest,"RomanEmpireGame.Input.RemappingMetadata",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanInputRemappingMetadataTest::RunTest(const FString& Parameters)
{
	using namespace RomanRemapAuthoring;
	const bool Apply=FParse::Param(FCommandLine::Get(),TEXT("RomanAuthorRemappingMetadata"));
	if(!TestNull(TEXT("No asset editing during PIE"),GEditor?GEditor->PlayWorld:nullptr)) return false;
	TStrongObjectPtr<URomanInputConfig> Config(LoadObject<URomanInputConfig>(nullptr,TEXT("/Game/Input/DA_RomanInputConfig")));
	if(!TestNotNull(TEXT("Exact existing input configuration"),Config.Get())) return false;
	TArray<FText> Errors; if(!TestTrue(TEXT("Keep existing logical action contract"),Config->ValidateConfiguration(Errors))) return false;
	const auto Expected=Recipe(Config.Get()); TestEqual(TEXT("29 discrete bindings; Mouse2D is one locked mapping"),Expected.Num(),29);
	TSet<FName> Names;
	const TArray<UInputMappingContext*> Contexts={Config->StrategicContext.Get(),Config->TacticalContext.Get(),Config->PossessionContext.Get(),Config->SharedContext.Get()};
	const TCHAR* Paths[]={TEXT("/Game/Input/Contexts/IMC_Strategic"),TEXT("/Game/Input/Contexts/IMC_Tactical"),TEXT("/Game/Input/Contexts/IMC_Possession"),TEXT("/Game/Input/Contexts/IMC_Shared")};
	const TCHAR* Baseline[]={TEXT("2D2963B2277B96FD9EB54B31174A3157505BD9AA"),TEXT("710AF25DDAEF43AA6DFFA21C4FCCC9425F816D94"),TEXT("F0142A53B232F1BF866691C3643F24886B9AF4FD"),TEXT("194B32BDBD1A5DDEFCB03961A2BF35C4CC8CE7D5")};
	TArray<UInputMappingContext*> Missing;
	// Entire batch preflight first; original bytes are accepted only by their exact recorded hash.
	for(int32 Index=0;Index<Contexts.Num();++Index)
	{
		auto* Context=Contexts[Index]; UPackage* Package=Context->GetOutermost();
		if(!TestEqual(TEXT("Refuse redirected/foreign context path"),Package->GetName(),FString(Paths[Index])) || !TestFalse(TEXT("Refuse unrelated unsaved context changes"),Package->IsDirty()) ||
			!TestTrue(TEXT("No unreviewed authored profile override migration"),Context->GetProfilesWithOverridenMappings().IsEmpty())) return false;
		const FString Tag=Package->GetMetaData().GetValue(Context,SchemaTag);
		if(Tag.IsEmpty())
		{
			const FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
			if(!TestEqual(TEXT("Original pre-migration bytes must match audited input asset"),FileSHA(Filename),FString(Baseline[Index]))) return false;
			if(Index<3) Missing.Add(Context);
		}
		else if(!TestEqual(TEXT("Refuse foreign/unsupported metadata schema"),Tag,FString(TEXT("1")))) return false;
		for(const auto& Mapping:Context->GetMappings())
		{
			const auto* Entry=Expected.FindByPredicate([&](const FExpected& E){return E.Context==Context && E.Action==Mapping.Action && E.Key==Mapping.Key;});
			const auto* Settings=Mapping.GetPlayerMappableKeySettings();
			if(Settings && !TestTrue(TEXT("Never adopt/overwrite foreign per-key metadata"),Entry && Matches(*Entry,Settings))) return false;
			if(!Entry && !TestFalse(TEXT("Locked analog/cancel mappings remain unmappable"),Mapping.IsPlayerMappable())) return false;
		}
	}
	for(const auto& Entry:Expected)
	{
		if(!TestFalse(TEXT("Globally unique stable per-direction mapping IDs"),Names.Contains(Entry.Name))) return false;
		Names.Add(Entry.Name);
		if(!TestNotNull(TEXT("Existing authored default mapping found without rebuilding it"),MappingFor(Entry))) return false;
	}
	if(Apply && !Missing.IsEmpty())
	{
		auto* Behavior=FindFProperty<FEnumProperty>(FEnhancedActionKeyMapping::StaticStruct(),TEXT("SettingBehavior"));
		auto* KeySettings=FindFProperty<FObjectProperty>(FEnhancedActionKeyMapping::StaticStruct(),TEXT("PlayerMappableKeySettings"));
		if(!TestTrue(TEXT("Exact engine metadata reflection API"),Behavior && KeySettings && KeySettings->PropertyClass==UPlayerMappableKeySettings::StaticClass())) return false;
		// Byte-for-byte recovery artifacts, never overwrite an existing backup with migrated bytes.
		const FString Backup=FPaths::ProjectDir()/TEXT("Artifacts/C017H_InputMetadataBackup");
		if(!TestTrue(TEXT("Create scoped original input recovery directory"),IFileManager::Get().MakeDirectory(*Backup,true))) return false;
		for(int32 Index=0;Index<Contexts.Num();++Index) if(Missing.Contains(Contexts[Index]))
		{
			const FString Source=FPackageName::LongPackageNameToFilename(Contexts[Index]->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
			const FString Target=Backup/FPaths::GetCleanFilename(Source);
			if(!IFileManager::Get().FileExists(*Target) && !TestEqual(TEXT("Preserve exact original bytes before migration"),IFileManager::Get().Copy(*Target,*Source,false),COPY_OK)) return false;
			if(!TestEqual(TEXT("Recovery copy matches original baseline"),FileSHA(Target),FString(Baseline[Index]))) return false;
		}
		for(const auto& Entry:Expected) if(Missing.Contains(Entry.Context))
		{
			auto* Mapping=MappingFor(Entry);
			auto* Settings=NewObject<UPlayerMappableKeySettings>(Entry.Context,Entry.Name,RF_Transactional);
			Settings->Name=Entry.Name; Settings->DisplayName=Entry.Label; Settings->DisplayCategory=FText::FromString(Entry.Category);
			Behavior->GetUnderlyingProperty()->SetIntPropertyValue(Behavior->ContainerPtrToValuePtr<void>(Mapping),static_cast<uint64>(EPlayerMappableKeySettingBehaviors::OverrideSettings));
			KeySettings->SetObjectPropertyValue_InContainer(Mapping,Settings);
		}
		for(auto* Context:Missing)
		{
			UPackage* Package=Context->GetOutermost(); Package->GetMetaData().SetValue(Context,SchemaTag,TEXT("1")); Context->MarkPackageDirty();
			FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
			if(!TestTrue(TEXT("Explicit scoped context metadata saved"),UPackage::SavePackage(Package,Context,*FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension()),Args))) return false;
		}
	}
	int32 Mappable=0,Total=0;
	for(const auto& Entry:Expected)
	{
		const auto* Mapping=MappingFor(Entry);
		TestTrue(TEXT("Saved metadata carries stable ID/category/label and no external authority"),Matches(Entry,Mapping->GetPlayerMappableKeySettings()));
		if(auto* Settings=Mapping->GetPlayerMappableKeySettings())
		{
			const FName BeforeName=Settings->Name; Settings->Name=TEXT("Foreign.Row");
			TestFalse(TEXT("Read-only ownership check rejects foreign mapping ID"),Matches(Entry,Settings)); Settings->Name=BeforeName;
			const auto BeforeProfiles=Settings->SupportedKeyProfileIds; const auto BeforeMetadata=Settings->Metadata;
			Settings->SupportedKeyProfileIds.Add(TEXT("Foreign.Profile"));
			TestFalse(TEXT("Read-only ownership check rejects unreviewed authored profile filter"),Matches(Entry,Settings)); Settings->SupportedKeyProfileIds=BeforeProfiles;
			Settings->Metadata=Config.Get(); TestFalse(TEXT("Read-only ownership check rejects foreign key authority reference"),Matches(Entry,Settings)); Settings->Metadata=BeforeMetadata;
			TestTrue(TEXT("All rejection probes restore original metadata without save"),Matches(Entry,Settings));
		}
	}
	for(const auto* Context:Contexts) for(const auto& Mapping:Context->GetMappings()) {++Total; if(Mapping.IsPlayerMappable()) ++Mappable;}
	TestEqual(TEXT("All original 33 mappings remain"),Total,33); TestEqual(TEXT("29 editable digital rows"),Mappable,29);
	AddInfo(FString::Printf(TEXT("Remapping metadata: mappings=%d editable=%d saved=%d apply=%d"),Total,Mappable,Apply?Missing.Num():0,Apply));
	return !HasAnyErrors();
}
#endif
