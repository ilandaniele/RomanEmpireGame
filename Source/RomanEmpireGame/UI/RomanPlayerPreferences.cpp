// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanPlayerPreferences.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

bool FRomanPlayerPreferences::IsValid() const
{
	const auto Range = [](float V, float Min, float Max) { return FMath::IsFinite(V) && V >= Min && V <= Max; };
	return Range(MasterVolume,0,1) && Range(EffectsVolume,0,1) && Range(LookSensitivity,.1f,5)
		&& Range(PanSensitivity,.1f,5) && Range(TextScale,1,2);
}

bool FRomanPlayerPreferences::operator==(const FRomanPlayerPreferences& Other) const
{
	return MasterVolume == Other.MasterVolume && EffectsVolume == Other.EffectsVolume
		&& LookSensitivity == Other.LookSensitivity && PanSensitivity == Other.PanSensitivity
		&& TextScale == Other.TextScale && bInvertLookY == Other.bInvertLookY;
}

bool FRomanPreferencesCodec::Encode(const FRomanPlayerPreferences& Value, FString& Out)
{
	if (!Value.IsValid()) return false;
	TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetNumberField(TEXT("Schema"),1);
	Object->SetNumberField(TEXT("MasterVolume"),Value.MasterVolume);
	Object->SetNumberField(TEXT("EffectsVolume"),Value.EffectsVolume);
	Object->SetNumberField(TEXT("LookSensitivity"),Value.LookSensitivity);
	Object->SetNumberField(TEXT("PanSensitivity"),Value.PanSensitivity);
	Object->SetNumberField(TEXT("TextScale"),Value.TextScale);
	Object->SetBoolField(TEXT("InvertLookY"),Value.bInvertLookY);
	FString Encoded;
	if (!FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Encoded))) return false;
	Out = MoveTemp(Encoded); return true;
}

bool FRomanPreferencesCodec::Decode(const FString& JSON, FRomanPlayerPreferences& Out)
{
	if (JSON.Len() > MaxBytes || FTCHARToUTF8(*JSON).Length() > MaxBytes) return false;
	TSharedPtr<FJsonObject> Object;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JSON),Object) || !Object.IsValid() || Object->Values.Num() != 7) return false;
	// TryGet permits conversions for some JSON values; this persisted contract does not.
	for (const TCHAR* Name : {TEXT("Schema"),TEXT("MasterVolume"),TEXT("EffectsVolume"),TEXT("LookSensitivity"),TEXT("PanSensitivity"),TEXT("TextScale")})
	{
		const TSharedPtr<FJsonValue>* Field = Object->Values.Find(Name);
		if (!Field || !Field->IsValid() || (*Field)->Type != EJson::Number) return false;
	}
	const TSharedPtr<FJsonValue>* Invert = Object->Values.Find(TEXT("InvertLookY"));
	if (!Invert || !Invert->IsValid() || (*Invert)->Type != EJson::Boolean) return false;
	double Schema = 0;
	FRomanPlayerPreferences Candidate;
	if (!Object->TryGetNumberField(TEXT("Schema"),Schema) || Schema != 1
		|| !Object->TryGetNumberField(TEXT("MasterVolume"),Candidate.MasterVolume)
		|| !Object->TryGetNumberField(TEXT("EffectsVolume"),Candidate.EffectsVolume)
		|| !Object->TryGetNumberField(TEXT("LookSensitivity"),Candidate.LookSensitivity)
		|| !Object->TryGetNumberField(TEXT("PanSensitivity"),Candidate.PanSensitivity)
		|| !Object->TryGetNumberField(TEXT("TextScale"),Candidate.TextScale)
		|| !Object->TryGetBoolField(TEXT("InvertLookY"),Candidate.bInvertLookY) || !Candidate.IsValid()) return false;
	Out = Candidate; return true;
}

namespace
{
bool PreferencePath(const FString& Path, FString& Resolved)
{
	if (Path.IsEmpty()) return false;
	Resolved = FPaths::ConvertRelativePathToFull(Path); FPaths::NormalizeFilename(Resolved);
	if (!FPaths::CollapseRelativeDirectories(Resolved)) return false;
	return FPaths::IsUnderDirectory(Resolved,FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));
}
}

bool FRomanPreferencesFile::Read(const FString& Path, FRomanPlayerPreferences& Out)
{
	FString Resolved;
	if (!PreferencePath(Path,Resolved)) return false;
	const int64 Size = IFileManager::Get().FileSize(*Resolved);
	if (Size < 0 || Size > FRomanPreferencesCodec::MaxBytes) return false;
	FString JSON;
	return FFileHelper::LoadFileToString(JSON,*Resolved) && FRomanPreferencesCodec::Decode(JSON,Out);
}

bool FRomanPreferencesFile::Write(const FString& Path, const FRomanPlayerPreferences& Value)
{
	FString Resolved, JSON;
	if (!PreferencePath(Path,Resolved) || !FRomanPreferencesCodec::Encode(Value,JSON)) return false;
	IFileManager& Files = IFileManager::Get();
	if (!Files.MakeDirectory(*FPaths::GetPath(Resolved),true)) return false;
	const FString Temporary = Resolved + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
	ON_SCOPE_EXIT { Files.Delete(*Temporary,false,true); }; // Exact owned temp, never recursive.
	if (!FFileHelper::SaveStringToFile(JSON,*Temporary,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
	FRomanPlayerPreferences Verified;
	if (!Read(Temporary,Verified) || !(Verified == Value)) return false;
	// Generic IFileManager::Move deletes the destination before attempting a move.
	// Keep its previous bytes recoverable instead of risking loss on failed promotion.
	IPlatformFile& Platform = FPlatformFileManager::Get().GetPlatformFile();
	FRomanPlayerPreferences Current;
	// A corrupt current file must not overwrite a previously validated recovery file.
	const FString Previous = Resolved + (Read(Resolved,Current) ? TEXT(".previous") : TEXT(".rejected"));
	const bool bHadPrevious = Files.FileExists(*Resolved);
	if (bHadPrevious)
	{
		if (Files.FileExists(*Previous) && !Files.Delete(*Previous,false,false,true)) return false;
		if (!Platform.MoveFile(*Previous,*Resolved)) return false;
	}
	if (Platform.MoveFile(*Resolved,*Temporary)) return true;
	if (bHadPrevious) Platform.MoveFile(*Resolved,*Previous); // Best effort; failed rollback retains .previous.
	return false;
}
