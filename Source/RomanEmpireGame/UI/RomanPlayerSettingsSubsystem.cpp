// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanPlayerSettingsSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void URomanPlayerSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StoragePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Settings/RomanPlayerPreferences.json")));
	const int64 Size = IFileManager::Get().FileSize(*StoragePath);
	if (Size < 0)
	{
		// An interrupted promotion may leave only the previous file. Never auto-write.
		if (IFileManager::Get().FileExists(*(StoragePath + TEXT(".previous"))))
			LoadFailure = FRomanPreferencesFile::Read(StoragePath + TEXT(".previous"),Preferences) ? ERomanPreferencesFailure::None : ERomanPreferencesFailure::InvalidFile;
		return;
	}
	FString JSON;
	if (Size > FRomanPreferencesCodec::MaxBytes) LoadFailure = ERomanPreferencesFailure::InvalidFile;
	else if (!FFileHelper::LoadFileToString(JSON,*StoragePath)) LoadFailure = ERomanPreferencesFailure::IOError;
	else if (!FRomanPreferencesCodec::Decode(JSON,Preferences)) LoadFailure = ERomanPreferencesFailure::InvalidFile;
	if (LoadFailure != ERomanPreferencesFailure::None)
		FRomanPreferencesFile::Read(StoragePath + TEXT(".previous"),Preferences); // Failure preserves defaults or recovered values.
	// Corrupt/unknown file is preserved; only explicit validated user apply may replace it.
}

void URomanPlayerSettingsSubsystem::Deinitialize()
{
	Changed.Clear(); StoragePath.Reset(); Super::Deinitialize();
}

bool URomanPlayerSettingsSubsystem::Persist(const FRomanPlayerPreferences& Value) const
{
	return FRomanPreferencesFile::Write(StoragePath,Value);
}

ERomanPreferencesFailure URomanPlayerSettingsSubsystem::TryApply(FRomanPlayerPreferences Value, bool bPersist)
{
	if (bApplying) return ERomanPreferencesFailure::Busy;
	if (!Value.IsValid()) return ERomanPreferencesFailure::InvalidValue;
	TGuardValue<bool> Guard(bApplying,true);
	if (bPersist && !Persist(Value)) return ERomanPreferencesFailure::IOError;
	const bool bChanged = !(Value == Preferences);
	Preferences = Value;
	if (bPersist) LoadFailure = ERomanPreferencesFailure::None;
	if (bChanged) Changed.Broadcast();
	return ERomanPreferencesFailure::None;
}
