// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class URomanInputConfig;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputUserSettings;
class UEnhancedPlayerMappableKeyProfile;
struct FEnhancedActionKeyMapping;

/** Read-only projection; Enhanced Input's local profile remains the sole key authority. */
struct FRomanKeyBinding
{
	FName Name;
	FText Category;
	FText Label;
	FKey Current;
	FKey Default;
};

enum class ERomanRemapFailure : uint8 { None, Unavailable, UnknownMapping, ReservedKey, UnsupportedKey, Conflict, EngineRejected };

namespace RomanInputRemapping
{
	ROMANEMPIREGAME_API bool RegisterContexts(const URomanInputConfig* Config,UEnhancedInputUserSettings* Settings);
	ROMANEMPIREGAME_API TArray<FKey> EffectiveKeys(const FEnhancedActionKeyMapping& Mapping,const UEnhancedPlayerMappableKeyProfile* Profile);
	ROMANEMPIREGAME_API TArray<FRomanKeyBinding> GetBindings(const URomanInputConfig* Config,const UEnhancedInputUserSettings* Settings);
	ROMANEMPIREGAME_API FString DescribeKeys(const UInputMappingContext* Context,const UInputAction* Action,const UEnhancedInputUserSettings* Settings);
	/** Validates before mutation. Does not save, generate slots, alter assets or defer events while paused. */
	ROMANEMPIREGAME_API ERomanRemapFailure SetKey(const URomanInputConfig* Config,UEnhancedInputUserSettings* Settings,FName Name,FKey Key);
	/** Resets only this project's registered rows, never foreign contexts/profile rows. */
	ROMANEMPIREGAME_API bool ResetDefaults(const URomanInputConfig* Config,UEnhancedInputUserSettings* Settings);
	/** Actual engine save result, unlike Enhanced Input's void SaveSettings(). */
	ROMANEMPIREGAME_API bool Save(UEnhancedInputUserSettings* Settings);
}
