// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Input/RomanInputConfig.h"
#include "Input/RomanInputRemapping.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "Misc/ScopeExit.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanInputRemappingProfileTest,"RomanEmpireGame.Input.RemappingProfile",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanInputRemappingProfileTest::RunTest(const FString& Parameters)
{
	using namespace RomanInputRemapping;
	TStrongObjectPtr<URomanInputConfig> Config(LoadObject<URomanInputConfig>(nullptr,TEXT("/Game/Input/DA_RomanInputConfig")));
	TStrongObjectPtr<UEnhancedInputUserSettings> Settings(NewObject<UEnhancedInputUserSettings>());
	FPlayerMappableKeyProfileCreationArgs Args; Args.ProfileStringIdentifier=TEXT("Roman.TestDetached"); Args.ProfileType=UEnhancedPlayerMappableKeyProfile::StaticClass(); Args.bSetAsCurrentProfile=true;
	if(!TestNotNull(TEXT("Actual engine profile created without modifying personal profile"),Settings->CreateNewKeyProfile(Args)) || !TestTrue(TEXT("Register all four original contexts"),RegisterContexts(Config.Get(),Settings.Get()))) return false;
	const auto Before=GetBindings(Config.Get(),Settings.Get()); TestEqual(TEXT("Actual engine rows expose all29 bindings"),Before.Num(),29);
	TestTrue(TEXT("Registration is idempotent"),RegisterContexts(Config.Get(),Settings.Get()));
	TestEqual(TEXT("No additional slots on repeated registration"),GetBindings(Config.Get(),Settings.Get()).Num(),29);
	const FName Move(TEXT("Roman.v1.Tactical.Move_forward"));
	const auto Current=[&](FName ID)
	{
		const auto Values=GetBindings(Config.Get(),Settings.Get());
		const auto* Row=Values.FindByPredicate([&](const FRomanKeyBinding& Value){return Value.Name==ID;}); return Row?Row->Current:FKey();
	};
	TestTrue(TEXT("Digital keyboard remap accepted by canonical engine profile"),SetKey(Config.Get(),Settings.Get(),Move,EKeys::U)==ERomanRemapFailure::None);
	TestEqual(TEXT("Profile readback is actual new key"),Current(Move),EKeys::U);
	const auto& Authored=Config->TacticalContext->GetMappings();
	const auto* Mapping=Authored.FindByPredicate([&](const FEnhancedActionKeyMapping& Value){return Value.GetMappingName()==Move;});
	if(!TestNotNull(TEXT("Stable per-direction authored metadata"),Mapping)) return false;
	TestEqual(TEXT("Asset default key is never edited by remapping"),Mapping->Key,EKeys::W);
	TestTrue(TEXT("Movement latch projection resolves customized key, not authored default"),EffectiveKeys(*Mapping,Settings->GetActiveKeyProfile())==TArray<FKey>{EKeys::U});
	for(FKey Key:{EKeys::Escape,EKeys::F10,EKeys::Tab,EKeys::Tilde}) TestTrue(TEXT("Reserved menu/cancel/navigation/console controls reject before mutation"),SetKey(Config.Get(),Settings.Get(),Move,Key)==ERomanRemapFailure::ReservedKey);
	for(FKey Key:{FKey(),EKeys::Mouse2D,EKeys::MouseWheelAxis,EKeys::LeftMouseButton,EKeys::Gamepad_FaceButton_Bottom}) TestTrue(TEXT("Movement rejects invalid/analog/pointer/gamepad sources"),SetKey(Config.Get(),Settings.Get(),Move,Key)==ERomanRemapFailure::UnsupportedKey);
	for(FKey Key:{EKeys::D,EKeys::B,EKeys::RightShift}) TestTrue(TEXT("Same-mode conflicts use effective keys across all actions"),SetKey(Config.Get(),Settings.Get(),Move,Key)==ERomanRemapFailure::Conflict);
	TestTrue(TEXT("Unknown row rejected, no implicit slot creation"),SetKey(Config.Get(),Settings.Get(),FName(TEXT("Foreign.Unknown")),EKeys::U)==ERomanRemapFailure::UnknownMapping);
	TestEqual(TEXT("All rejected operations preserve current binding"),Current(Move),EKeys::U);
	TestTrue(TEXT("Mutually exclusive Strategic permits tactical B"),SetKey(Config.Get(),Settings.Get(),FName(TEXT("Roman.v1.Strategic.Move_forward")),EKeys::B)==ERomanRemapFailure::None);
	TestTrue(TEXT("Boolean selection permits a keyboard key without changing pointer semantics"),SetKey(Config.Get(),Settings.Get(),FName(TEXT("Roman.v1.Tactical.Select")),EKeys::SpaceBar)==ERomanRemapFailure::None);
	TestFalse(TEXT("Detached profile cannot claim production persistence"),Save(Settings.Get()));
	const FString Slot=TEXT("RomanRemapFixture_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
	ON_SCOPE_EXIT {UGameplayStatics::DeleteGameInSlot(Slot,0);};
	TestFalse(TEXT("GUID fixture cannot overwrite an existing file"),UGameplayStatics::DoesSaveGameExist(Slot,0));
	TestTrue(TEXT("Actual engine serializer writes owned GUID save slot"),UGameplayStatics::SaveGameToSlot(Settings.Get(),Slot,0));
	TStrongObjectPtr<UEnhancedInputUserSettings> Loaded(Cast<UEnhancedInputUserSettings>(UGameplayStatics::LoadGameFromSlot(Slot,0)));
	if(!TestNotNull(TEXT("Actual save file reloads the engine class"),Loaded.Get())) return false;
	TestTrue(TEXT("Fresh deserialized profile registers metadata/defaults without dropping overrides"),RegisterContexts(Config.Get(),Loaded.Get()));
	const auto Restored=GetBindings(Config.Get(),Loaded.Get());
	const auto* Row=Restored.FindByPredicate([&](const FRomanKeyBinding& Value){return Value.Name==Move;});
	TestTrue(TEXT("Actual file reload preserves key and authored W default"),Row && Row->Current==EKeys::U && Row->Default==EKeys::W);
	TestTrue(TEXT("Reset only project rows through engine APIs"),ResetDefaults(Config.Get(),Loaded.Get()));
	for(const auto& Value:GetBindings(Config.Get(),Loaded.Get())) TestEqual(TEXT("All project rows restore authored defaults"),Value.Current,Value.Default);
	TestTrue(TEXT("Actual save serializes reset overrides"),UGameplayStatics::SaveGameToSlot(Loaded.Get(),Slot,0));
	Loaded.Reset(Cast<UEnhancedInputUserSettings>(UGameplayStatics::LoadGameFromSlot(Slot,0))); RegisterContexts(Config.Get(),Loaded.Get());
	for(const auto& Value:GetBindings(Config.Get(),Loaded.Get())) TestEqual(TEXT("Defaults survive an actual second reload"),Value.Current,Value.Default);
	TestEqual(TEXT("Default reset preserves every row"),GetBindings(Config.Get(),Loaded.Get()).Num(),29);
	return !HasAnyErrors();
}
#endif
