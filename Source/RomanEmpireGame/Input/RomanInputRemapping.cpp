// Copyright Roman Empire Game. All Rights Reserved.
#include "Input/RomanInputRemapping.h"
#include "Input/RomanInputConfig.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace
{
TArray<UInputMappingContext*> Contexts(const URomanInputConfig* Config)
{
	return Config?TArray<UInputMappingContext*>{Config->StrategicContext.Get(),Config->TacticalContext.Get(),Config->PossessionContext.Get(),Config->SharedContext.Get()}:TArray<UInputMappingContext*>();
}
FMapPlayerKeyArgs ArgsFor(FName Name,const UEnhancedPlayerMappableKeyProfile* Profile)
{
	FMapPlayerKeyArgs Args; Args.MappingName=Name; Args.Slot=EPlayerMappableKeySlot::First;
	Args.ProfileIdString=Profile->GetProfileIdString(); Args.bCreateMatchingSlotIfNeeded=false; Args.bDeferOnSettingsChangedBroadcast=false; return Args;
}
bool RegisteredRow(const FEnhancedActionKeyMapping& Mapping,const UEnhancedPlayerMappableKeyProfile* Profile)
{
	const FKeyMappingRow* Row=Profile?Profile->FindKeyMappingRow(Mapping.GetMappingName()):nullptr;
	if(!Row || Row->Mappings.Num()!=1) return false;
	const FPlayerKeyMapping& Key=*Row->Mappings.CreateConstIterator();
	return Key.GetSlot()==EPlayerMappableKeySlot::First && Key.GetDefaultKey()==Mapping.Key && Key.GetAssociatedInputAction()==Mapping.Action;
}
}

bool RomanInputRemapping::RegisterContexts(const URomanInputConfig* Config,UEnhancedInputUserSettings* Settings)
{
	if(!Config || !Settings || !Settings->GetActiveKeyProfile()) return false;
	for(auto* Context:Contexts(Config)) if(!Context) return false;
	bool Result=true;
	for(auto* Context:Contexts(Config)) if(!Settings->IsMappingContextRegistered(Context)) Result=Settings->RegisterInputMappingContext(Context) && Result;
	return Result;
}
TArray<FKey> RomanInputRemapping::EffectiveKeys(const FEnhancedActionKeyMapping& Mapping,const UEnhancedPlayerMappableKeyProfile* Profile)
{
	TArray<FKey> Keys;
	if(Profile && Mapping.IsPlayerMappable())
	{
		if(const auto* Row=Profile->FindKeyMappingRow(Mapping.GetMappingName()))
		{
			for(const auto& Key:Row->Mappings) if(Key.GetCurrentKey().IsValid()) Keys.AddUnique(Key.GetCurrentKey());
			return Keys;
		}
	}
	Keys.Add(Mapping.Key); return Keys;
}
TArray<FRomanKeyBinding> RomanInputRemapping::GetBindings(const URomanInputConfig* Config,const UEnhancedInputUserSettings* Settings)
{
	TArray<FRomanKeyBinding> Result;
	const auto* Profile=Settings?Settings->GetActiveKeyProfile():nullptr;
	if(!Config || !Profile) return Result;
	for(const auto* Context:Contexts(Config))
		if(Context) for(const auto& Mapping:Context->GetMappingsForProfile(Profile->GetProfileIdString()))
			if(Mapping.IsPlayerMappable() && RegisteredRow(Mapping,Profile))
			{
				const auto Keys=EffectiveKeys(Mapping,Profile);
				if(Keys.Num()==1) Result.Add({Mapping.GetMappingName(),Mapping.GetDisplayCategory(),Mapping.GetDisplayName(),Keys[0],Mapping.Key});
			}
	return Result;
}
FString RomanInputRemapping::DescribeKeys(const UInputMappingContext* Context,const UInputAction* Action,const UEnhancedInputUserSettings* Settings)
{
	if(!Context || !Action) return TEXT("unavailable");
	const auto* Profile=Settings?Settings->GetActiveKeyProfile():nullptr;
	TArray<FString> Names;
	for(const auto& Mapping:Context->GetMappingsForProfile(Profile?Profile->GetProfileIdString():FString()))
		if(Mapping.Action==Action) for(const FKey& Key:EffectiveKeys(Mapping,Profile)) Names.AddUnique(Key.GetDisplayName().ToString());
	return Names.IsEmpty()?FString(TEXT("unavailable")):FString::Join(Names,TEXT(" / "));
}
ERomanRemapFailure RomanInputRemapping::SetKey(const URomanInputConfig* Config,UEnhancedInputUserSettings* Settings,FName Name,FKey Key)
{
	auto* Profile=Settings?Settings->GetActiveKeyProfile():nullptr;
	if(!Config || !Profile) return ERomanRemapFailure::Unavailable;
	const FEnhancedActionKeyMapping* Target=nullptr; const UInputMappingContext* TargetContext=nullptr;
	for(const auto* Context:Contexts(Config))
		if(Context) for(const auto& Mapping:Context->GetMappingsForProfile(Profile->GetProfileIdString()))
			if(Mapping.IsPlayerMappable() && Mapping.GetMappingName()==Name)
			{
				if(Target) return ERomanRemapFailure::UnknownMapping;
				Target=&Mapping; TargetContext=Context;
			}
	if(!Target || !RegisteredRow(*Target,Profile)) return ERomanRemapFailure::UnknownMapping;
	if(Key==EKeys::Escape || Key==EKeys::F10 || Key==EKeys::Tab || Key==EKeys::Tilde) return ERomanRemapFailure::ReservedKey;
	if(!Key.IsValid() || !Key.IsDigital() || Key.IsGamepadKey() || (!Key.IsMouseButton() && Key.GetFName().ToString().StartsWith(TEXT("Touch")))) return ERomanRemapFailure::UnsupportedKey;
	// Axis2D authored swizzle/negate expects a keyboard Boolean source, not pointer/button movement.
	if((Target->Action==Config->CameraPan || Target->Action==Config->SoldierMove) && Key.IsMouseButton()) return ERomanRemapFailure::UnsupportedKey;
	for(const auto* Context:Contexts(Config))
		if(Context && (Context==TargetContext || Context==Config->SharedContext || TargetContext==Config->SharedContext))
			for(const auto& Mapping:Context->GetMappingsForProfile(Profile->GetProfileIdString()))
				if(&Mapping!=Target && EffectiveKeys(Mapping,Profile).Contains(Key)) return ERomanRemapFailure::Conflict;
	FMapPlayerKeyArgs Args=ArgsFor(Name,Profile); Args.NewKey=Key;
	FGameplayTagContainer Failure; Settings->MapPlayerKey(Args,Failure);
	const auto* Actual=Profile->FindKeyMapping(Args);
	return Failure.IsEmpty() && Actual && Actual->GetCurrentKey()==Key?ERomanRemapFailure::None:ERomanRemapFailure::EngineRejected;
}
bool RomanInputRemapping::ResetDefaults(const URomanInputConfig* Config,UEnhancedInputUserSettings* Settings)
{
	if(!Config || !Settings || !Settings->GetActiveKeyProfile()) return false;
	const auto Bindings=GetBindings(Config,Settings);
	int32 Expected=0;
	for(const auto* Context:Contexts(Config)) if(Context)
		for(const auto& Mapping:Context->GetMappingsForProfile(Settings->GetActiveKeyProfileId())) if(Mapping.IsPlayerMappable()) ++Expected;
	if(Bindings.IsEmpty() || Bindings.Num()!=Expected) return false; // Refuse a partial/unavailable project profile before any reset.
	// Batch reset skips intermediate conflict states; engine events are immediate while the menu is paused.
	for(const auto& Binding:Bindings)
	{
		FMapPlayerKeyArgs Args=ArgsFor(Binding.Name,Settings->GetActiveKeyProfile()); Args.NewKey=Binding.Default;
		FGameplayTagContainer Failure; Settings->MapPlayerKey(Args,Failure); if(!Failure.IsEmpty()) return false;
	}
	return true;
}
bool RomanInputRemapping::Save(UEnhancedInputUserSettings* Settings)
{
	ULocalPlayer* Player=Settings?Settings->GetLocalPlayer():nullptr;
	const auto* Subsystem=Player?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player):nullptr;
	const UEnhancedPlayerInput* Input=Subsystem?Subsystem->GetPlayerInput():nullptr;
	if(!Input || Subsystem->GetUserSettings()!=Settings) return false;
	const FString Slot=Input->GetUserSettingsSaveFileName();
	if(Slot.IsEmpty() || Slot.Contains(TEXT("..")) || Slot.Contains(TEXT("/")) || Slot.Contains(TEXT("\\")) || Slot.Contains(TEXT(":"))) return false;
	return UGameplayStatics::SaveGameToSlot(Settings,Slot,Player->GetLocalPlayerIndex());
}
