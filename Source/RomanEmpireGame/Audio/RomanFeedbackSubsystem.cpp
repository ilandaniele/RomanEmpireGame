// Copyright Roman Empire Game. All Rights Reserved.
#include "Audio/RomanFeedbackSubsystem.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/RomanPlayerSettingsSubsystem.h"

URomanFeedbackSubsystem::URomanFeedbackSubsystem()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Select(TEXT("/Game/Audio/RomanFeedback/S_RomanSelect"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Order(TEXT("/Game/Audio/RomanFeedback/S_RomanOrder"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Complete(TEXT("/Game/Audio/RomanFeedback/S_RomanComplete"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Impact(TEXT("/Game/Audio/RomanFeedback/S_RomanImpact"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Victory(TEXT("/Game/Audio/RomanFeedback/S_RomanVictory"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Defeat(TEXT("/Game/Audio/RomanFeedback/S_RomanDefeat"));
	Sounds.Add(ERomanFeedbackCue::Select, Select.Object);
	Sounds.Add(ERomanFeedbackCue::Order, Order.Object);
	Sounds.Add(ERomanFeedbackCue::Complete, Complete.Object);
	Sounds.Add(ERomanFeedbackCue::Impact, Impact.Object);
	Sounds.Add(ERomanFeedbackCue::Victory, Victory.Object);
	Sounds.Add(ERomanFeedbackCue::Defeat, Defeat.Object);
}

bool URomanFeedbackSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void URomanFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bReady || InWorld.GetNetMode() == NM_DedicatedServer) return;
	for (const auto& Pair : Sounds) if (!IsValid(Pair.Value)) return;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		UAudioComponent* Voice = NewObject<UAudioComponent>(this, NAME_None, RF_Transient);
		Voice->bAutoActivate = false; Voice->bAutoDestroy = false;
		Voice->bIsUISound = true;
		Voice->SetVolumeMultiplier(MasterLevel * EffectsLevel);
		Voice->RegisterComponentWithWorld(&InWorld);
		Voices.Add(Voice);
	}
	BattleSource = InWorld.GetSubsystem<UTacticalBattleSubsystem>();
	if (BattleSource.IsValid()) BattleSource->OnTacticalBattleChanged.AddUniqueDynamic(this, &URomanFeedbackSubsystem::HandleBattleChanged);
	PreferencesSource = InWorld.GetGameInstance() ? InWorld.GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>() : nullptr;
	if (PreferencesSource.IsValid())
	{
		PreferencesChangedHandle = PreferencesSource->OnPreferencesChanged().AddUObject(this,&URomanFeedbackSubsystem::HandlePreferencesChanged);
		HandlePreferencesChanged();
	}
	bReady = true;
}

void URomanFeedbackSubsystem::HandlePreferencesChanged()
{
	if (!PreferencesSource.IsValid()) return;
	const FRomanPlayerPreferences Current = PreferencesSource->GetPreferences();
	SetLevels(Current.MasterVolume,Current.EffectsVolume);
}

bool URomanFeedbackSubsystem::PlayCue(ERomanFeedbackCue Cue, FVector Location)
{
	UWorld* World = GetWorld();
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	USoundBase* Sound = Sounds.FindRef(Cue);
	if (!bReady || !World || !World->HasBegunPlay() || World->bIsTearingDown ||
		!Player || !Player->IsLocalController() || Player->IsActorBeingDestroyed() ||
		!IsValid(Sound) || Location.ContainsNaN() || Voices.Num() != 8) return false;
	// Coalesce mass selection/impacts without changing a command or simulation clock.
	const double Now = FPlatformTime::Seconds();
	if (const double* Previous = LastCueTimes.Find(Cue); Previous && Now - *Previous < .1) return false;
	UAudioComponent* Voice = nullptr;
	for (const auto& Candidate : Voices) if (IsValid(Candidate) && !Candidate->IsPlaying()) { Voice = Candidate; break; }
	if (!Voice) { Voice = Voices[NextVoice]; NextVoice = (NextVoice + 1) % Voices.Num(); }
	if (!IsValid(Voice) || !Voice->IsRegistered()) return false;
	Voice->Stop();
	const bool bSpatial = Cue == ERomanFeedbackCue::Impact;
	Voice->bIsUISound = !bSpatial;
	Voice->bAllowSpatialization = bSpatial;
	Voice->bOverrideAttenuation = bSpatial;
	Voice->AttenuationOverrides.bAttenuate = bSpatial;
	Voice->AttenuationOverrides.bSpatialize = bSpatial;
	Voice->AttenuationOverrides.AttenuationShapeExtents = FVector(200.f);
	Voice->AttenuationOverrides.FalloffDistance = 1600.f;
	Voice->SetWorldLocation(Location);
	Voice->SetSound(Sound);
	Voice->SetVolumeMultiplier(MasterLevel * EffectsLevel);
	Voice->Play();
	if (!Voice->IsPlaying()) return false;
	LastCueTimes.Add(Cue, Now);
	return true;
}

bool URomanFeedbackSubsystem::SetLevels(float Master, float Effects)
{
	if (!FMath::IsFinite(Master) || !FMath::IsFinite(Effects) || Master < 0 || Master > 1 || Effects < 0 || Effects > 1) return false;
	MasterLevel = Master; EffectsLevel = Effects;
	for (const auto& Voice : Voices) if (IsValid(Voice)) Voice->SetVolumeMultiplier(MasterLevel * EffectsLevel);
	return true;
}

int32 URomanFeedbackSubsystem::GetPlayingVoiceCount() const
{
	int32 Count = 0;
	for (const auto& Voice : Voices) if (IsValid(Voice) && Voice->IsPlaying()) ++Count;
	return Count;
}

void URomanFeedbackSubsystem::HandleBattleChanged(FTacticalBattleResult Result)
{
	if (!bReady || !BattleSource.IsValid()) return;
	const FTacticalBattleResult Current = BattleSource->GetResult();
	if (Current.EncounterID != Result.EncounterID || Current.Outcome != Result.Outcome) return;
	if (!Result.IsResolved()) { bAnnouncedResult = false; AnnouncedEncounter.Invalidate(); return; }
	if (bAnnouncedResult && AnnouncedEncounter == Result.EncounterID) return;
	bAnnouncedResult = true; AnnouncedEncounter = Result.EncounterID;
	PlayCue(Result.Outcome == ETacticalBattleOutcome::PlayerVictory ? ERomanFeedbackCue::Victory :
		Result.Outcome == ETacticalBattleOutcome::PlayerDefeat ? ERomanFeedbackCue::Defeat : ERomanFeedbackCue::Order);
}

void URomanFeedbackSubsystem::Deinitialize()
{
	bReady = false;
	if (BattleSource.IsValid()) BattleSource->OnTacticalBattleChanged.RemoveDynamic(this, &URomanFeedbackSubsystem::HandleBattleChanged);
	BattleSource.Reset();
	if (PreferencesSource.IsValid()) PreferencesSource->OnPreferencesChanged().Remove(PreferencesChangedHandle);
	PreferencesChangedHandle.Reset(); PreferencesSource.Reset();
	for (const auto& Voice : Voices) if (IsValid(Voice)) { Voice->Stop(); Voice->DestroyComponent(); }
	Voices.Empty(); LastCueTimes.Empty(); NextVoice = 0; bAnnouncedResult = false;
	Super::Deinitialize();
}
