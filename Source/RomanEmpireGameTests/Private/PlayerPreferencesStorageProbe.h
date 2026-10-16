// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "UI/RomanPlayerSettingsSubsystem.h"

/** Editor-module-only scoped IO destination fixture. Preferences authority remains the actual GI owner. */
struct FRomanPreferencesStorageProbe
{
	URomanPlayerSettingsSubsystem& Owner;
	FString OriginalPath;
	ERomanPreferencesFailure OriginalLoadFailure;
	explicit FRomanPreferencesStorageProbe(URomanPlayerSettingsSubsystem& InOwner)
		:Owner(InOwner),OriginalPath(InOwner.StoragePath),OriginalLoadFailure(InOwner.LoadFailure){}
	~FRomanPreferencesStorageProbe() { Owner.StoragePath=OriginalPath; Owner.LoadFailure=OriginalLoadFailure; }
	void SetPath(const FString& Path) { Owner.StoragePath=Path; }
};
