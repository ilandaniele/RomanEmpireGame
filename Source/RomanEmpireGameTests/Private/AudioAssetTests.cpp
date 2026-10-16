// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Misc/SecureHash.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundWave.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanAudioAssets, "RomanEmpireGame.Audio.ImportedFeedbackPCM",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanAudioAssets::RunTest(const FString& Parameters)
{
	// Expected original PCM, not metadata or the existence of a silent Sound Cue.
	struct FExpected { const TCHAR* Name; const TCHAR* Digest; int32 Bytes; };
	const FExpected Expected[] = {
		{TEXT("S_RomanSelect"), TEXT("b8dd4f1d369d7fcc7a03258e638c8399"), 10560},
		{TEXT("S_RomanOrder"), TEXT("d3b8407e069bcb02d4e223ad7ef80fc1"), 13920},
		{TEXT("S_RomanComplete"), TEXT("c658a34ca8b1447549c7893a909b0864"), 33600},
		{TEXT("S_RomanImpact"), TEXT("e6b56383aa54285ab763f91efe67dcf4"), 10080},
		{TEXT("S_RomanVictory"), TEXT("7e0936d1fb13333a091460f9f4a5f720"), 62880},
		{TEXT("S_RomanDefeat"), TEXT("f2ddff0f9ec4c1d29bbb21caa17d9b49"), 55200}
	};
	TStrongObjectPtr<USoundClass> SoundClass(LoadObject<USoundClass>(nullptr, TEXT("/Game/Audio/RomanFeedback/SC_RomanSFX")));
	if (!TestNotNull(TEXT("Persistent SFX class"), SoundClass.Get())) return false;
	TestEqual(TEXT("Authored SFX volume"), SoundClass->Properties.Volume, 1.f);
	TestEqual(TEXT("Authored SFX pitch"), SoundClass->Properties.Pitch, 1.f);
	for (const FExpected& Entry : Expected)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/RomanFeedback/%s"), Entry.Name);
		TStrongObjectPtr<USoundWave> Wave(LoadObject<USoundWave>(nullptr, *Path));
		if (!TestNotNull(Path, Wave.Get())) continue;
		TestTrue(TEXT("Actual SFX class reference"), Wave->SoundClassObject == SoundClass.Get());
		TestFalse(TEXT("One-shot cue does not loop"), Wave->bLooping);
		TestEqual(TEXT("Actual mono channels"), Wave->NumChannels, 1);
		TestEqual(TEXT("Actual imported sample rate"), Wave->GetImportedSampleRate(), 24000u);
		TestTrue(TEXT("Actual asset duration"), FMath::Abs(Wave->GetDuration() - Entry.Bytes / 48000.f) < .002f);
		TArray<uint8> PCM;
		uint32 Rate = 0; uint16 Channels = 0;
		if (!TestTrue(TEXT("Actual saved source PCM decodes"), Wave->GetImportedSoundWaveData(PCM, Rate, Channels))) continue;
		TestEqual(TEXT("Decoded PCM rate"), Rate, 24000u);
		TestEqual(TEXT("Decoded PCM mono"), Channels, static_cast<uint16>(1));
		TestEqual(TEXT("Decoded actual PCM length"), PCM.Num(), Entry.Bytes);
		TestEqual(FString::Printf(TEXT("Exact non-silent recipe %s"), Entry.Name), FMD5::HashBytes(PCM.GetData(), PCM.Num()), FString(Entry.Digest));
		if (!PCM.IsEmpty())
		{
			PCM[0] ^= 1;
			TestNotEqual(TEXT("Changed PCM is rejected by fingerprint"), FMD5::HashBytes(PCM.GetData(), PCM.Num()), FString(Entry.Digest));
		}
	}
	return true;
}
#endif
