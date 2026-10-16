// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/RomanPlayerPreferences.h"
#include "RomanPlayerSettingsSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERomanPreferencesFailure : uint8 { None, InvalidValue, InvalidFile, IOError, Busy };

/** One client preferences owner. Consumers project queries/events; no ticks/actor scans. */
UCLASS()
class ROMANEMPIREGAME_API URomanPlayerSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	UFUNCTION(BlueprintPure, Category="Preferences") FRomanPlayerPreferences GetPreferences() const { return Preferences; }
	UFUNCTION(BlueprintPure, Category="Preferences") ERomanPreferencesFailure GetLoadFailure() const { return LoadFailure; }
	UFUNCTION(BlueprintCallable, Category="Preferences") ERomanPreferencesFailure TryApply(FRomanPlayerPreferences Value, bool bPersist = true);
	DECLARE_EVENT(URomanPlayerSettingsSubsystem,FOnPreferencesChanged);
	FOnPreferencesChanged& OnPreferencesChanged() { return Changed; }
private:
	friend struct FRomanPreferencesStorageProbe; // Editor-only fixture redirects IO; no runtime setter/test branch.
	FRomanPlayerPreferences Preferences;
	ERomanPreferencesFailure LoadFailure = ERomanPreferencesFailure::None;
	FString StoragePath;
	bool bApplying = false;
	FOnPreferencesChanged Changed;
	bool Persist(const FRomanPlayerPreferences& Value) const;
};
