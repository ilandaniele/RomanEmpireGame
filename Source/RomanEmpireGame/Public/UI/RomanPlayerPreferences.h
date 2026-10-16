// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "RomanPlayerPreferences.generated.h"

/** Detached client presentation preferences, never campaign/save gameplay state. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FRomanPlayerPreferences
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preferences") float MasterVolume = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preferences") float EffectsVolume = .65f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preferences") float LookSensitivity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preferences") float PanSensitivity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preferences") float TextScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preferences") bool bInvertLookY = false;
	bool IsValid() const;
	bool operator==(const FRomanPlayerPreferences& Other) const;
};

/** Schema1 bounded strict codec. Decode commits output only after complete validation. */
struct ROMANEMPIREGAME_API FRomanPreferencesCodec
{
	static constexpr int32 MaxBytes = 16 * 1024;
	static bool Encode(const FRomanPlayerPreferences& Value, FString& Out);
	static bool Decode(const FString& JSON, FRomanPlayerPreferences& Out);
};

/** Bounded user preferences files, restricted to this project's Saved directory. */
struct ROMANEMPIREGAME_API FRomanPreferencesFile
{
	static bool Read(const FString& Path, FRomanPlayerPreferences& Out);
	static bool Write(const FString& Path, const FRomanPlayerPreferences& Value);
};
