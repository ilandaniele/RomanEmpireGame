// Copyright Roman Empire Game. All Rights Reserved.

#include "AudioManager.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AAudioManager::AAudioManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create music component
	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicComponent"));
	MusicComponent->bAutoActivate = false;
	MusicComponent->bIsUISound = true;
	SetRootComponent(MusicComponent);

	// Create ambient component
	AmbientComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientComponent"));
	AmbientComponent->SetupAttachment(RootComponent);
	AmbientComponent->bAutoActivate = false;
	AmbientComponent->bIsUISound = false;

	// Default volumes
	MasterVolume = 1.0f;
	SFXVolume = 1.0f;
	MusicVolume = 0.5f;
	AmbientVolume = 0.7f;
}

void AAudioManager::BeginPlay()
{
	Super::BeginPlay();
	InitializeSoundLibrary();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Audio Manager initialized"));
}

void AAudioManager::InitializeSoundLibrary()
{
	// Combat sounds
	FSoundEffectData SwordSwingData;
	SwordSwingData.SoundID = ESoundEffect::SwordSwing;
	SwordSwingData.Category = ESoundCategory::Combat;
	SwordSwingData.VolumeMultiplier = 1.0f;
	SwordSwingData.bIs3D = true;
	SoundEffectLibrary.Add(ESoundEffect::SwordSwing, SwordSwingData);

	FSoundEffectData SwordHitData;
	SwordHitData.SoundID = ESoundEffect::SwordHit;
	SwordHitData.Category = ESoundCategory::Combat;
	SwordHitData.VolumeMultiplier = 1.2f;
	SwordHitData.bIs3D = true;
	SoundEffectLibrary.Add(ESoundEffect::SwordHit, SwordHitData);

	FSoundEffectData ShieldBlockData;
	ShieldBlockData.SoundID = ESoundEffect::ShieldBlock;
	ShieldBlockData.Category = ESoundCategory::Combat;
	ShieldBlockData.VolumeMultiplier = 1.3f;
	ShieldBlockData.bIs3D = true;
	SoundEffectLibrary.Add(ESoundEffect::ShieldBlock, ShieldBlockData);

	// UI sounds
	FSoundEffectData ButtonClickData;
	ButtonClickData.SoundID = ESoundEffect::ButtonClick;
	ButtonClickData.Category = ESoundCategory::UI;
	ButtonClickData.VolumeMultiplier = 0.8f;
	ButtonClickData.bIs3D = false;
	SoundEffectLibrary.Add(ESoundEffect::ButtonClick, ButtonClickData);

	FSoundEffectData BuildCompleteData;
	BuildCompleteData.SoundID = ESoundEffect::BuildComplete;
	BuildCompleteData.Category = ESoundCategory::UI;
	BuildCompleteData.VolumeMultiplier = 1.0f;
	BuildCompleteData.bIs3D = false;
	SoundEffectLibrary.Add(ESoundEffect::BuildComplete, BuildCompleteData);

	// Music tracks
	FMusicTrackData MainMenuMusic;
	MainMenuMusic.TrackID = TEXT("MainMenu");
	MainMenuMusic.bLoop = true;
	MainMenuMusic.FadeInTime = 2.0f;
	MusicLibrary.Add(TEXT("MainMenu"), MainMenuMusic);

	FMusicTrackData BattleMusic;
	BattleMusic.TrackID = TEXT("Battle");
	BattleMusic.bLoop = true;
	BattleMusic.FadeInTime = 1.0f;
	MusicLibrary.Add(TEXT("Battle"), BattleMusic);

	FMusicTrackData CampaignMusic;
	CampaignMusic.TrackID = TEXT("Campaign");
	CampaignMusic.bLoop = true;
	CampaignMusic.FadeInTime = 3.0f;
	MusicLibrary.Add(TEXT("Campaign"), CampaignMusic);
}

void AAudioManager::PlaySoundEffect(ESoundEffect SoundEffect, FVector Location)
{
	FSoundEffectData* Data = GetSoundData(SoundEffect);
	if (!Data || !Data->Sound)
	{
		return;
	}

	float FinalVolume = MasterVolume * SFXVolume * Data->VolumeMultiplier;

	if (Data->bIs3D)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Data->Sound,
			Location,
			FinalVolume,
			Data->PitchMultiplier
		);
	}
	else
	{
		PlaySound2D(SoundEffect);
	}
}

void AAudioManager::PlaySoundEffectAttached(ESoundEffect SoundEffect, USceneComponent* AttachComponent)
{
	FSoundEffectData* Data = GetSoundData(SoundEffect);
	if (!Data || !Data->Sound || !AttachComponent)
	{
		return;
	}

	float FinalVolume = MasterVolume * SFXVolume * Data->VolumeMultiplier;

	UGameplayStatics::SpawnSoundAttached(
		Data->Sound,
		AttachComponent,
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		false,
		FinalVolume,
		Data->PitchMultiplier
	);
}

void AAudioManager::PlaySound2D(ESoundEffect SoundEffect)
{
	FSoundEffectData* Data = GetSoundData(SoundEffect);
	if (!Data || !Data->Sound)
	{
		return;
	}

	float FinalVolume = MasterVolume * SFXVolume * Data->VolumeMultiplier;

	UGameplayStatics::PlaySound2D(
		this,
		Data->Sound,
		FinalVolume,
		Data->PitchMultiplier
	);
}

void AAudioManager::PlayMusic(FName TrackID)
{
	FMusicTrackData* Data = MusicLibrary.Find(TrackID);
	if (!Data || !Data->Track)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Music track not found: %s"), *TrackID.ToString());
		return;
	}

	if (MusicComponent)
	{
		MusicComponent->SetSound(Data->Track);
		MusicComponent->SetVolumeMultiplier(MasterVolume * MusicVolume);
		MusicComponent->FadeIn(Data->FadeInTime);
		CurrentMusicTrack = TrackID;
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Playing music: %s"), *TrackID.ToString());
	}
}

void AAudioManager::StopMusic(float FadeOutTime)
{
	if (MusicComponent && MusicComponent->IsPlaying())
	{
		MusicComponent->FadeOut(FadeOutTime, 0.0f);
		CurrentMusicTrack = NAME_None;
	}
}

void AAudioManager::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (MusicComponent)
	{
		MusicComponent->SetVolumeMultiplier(MasterVolume * MusicVolume);
	}
}

void AAudioManager::CrossfadeToMusic(FName NewTrackID, float CrossfadeTime)
{
	StopMusic(CrossfadeTime * 0.5f);
	
	// Delay playing new track
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUFunction(this, FName("PlayMusic"), NewTrackID);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, CrossfadeTime * 0.5f, false);
}

void AAudioManager::PlayAmbientLoop(ESoundEffect AmbientSound)
{
	FSoundEffectData* Data = GetSoundData(AmbientSound);
	if (!Data || !Data->Sound)
	{
		return;
	}

	if (AmbientComponent)
	{
		AmbientComponent->SetSound(Data->Sound);
		AmbientComponent->SetVolumeMultiplier(MasterVolume * AmbientVolume);
		AmbientComponent->Play();
	}
}

void AAudioManager::StopAmbientLoop()
{
	if (AmbientComponent)
	{
		AmbientComponent->FadeOut(2.0f, 0.0f);
	}
}

void AAudioManager::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	
	// Update all playing sounds
	if (MusicComponent)
	{
		MusicComponent->SetVolumeMultiplier(MasterVolume * MusicVolume);
	}
	if (AmbientComponent)
	{
		AmbientComponent->SetVolumeMultiplier(MasterVolume * AmbientVolume);
	}
}

void AAudioManager::SetSFXVolume(float Volume)
{
	SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}

void AAudioManager::SetAmbientVolume(float Volume)
{
	AmbientVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (AmbientComponent)
	{
		AmbientComponent->SetVolumeMultiplier(MasterVolume * AmbientVolume);
	}
}

AAudioManager* AAudioManager::GetAudioManager(UObject* WorldContextObject)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject->GetWorld(), AAudioManager::StaticClass(), FoundActors);
	
	if (FoundActors.Num() > 0)
	{
		return Cast<AAudioManager>(FoundActors[0]);
	}
	return nullptr;
}

FSoundEffectData* AAudioManager::GetSoundData(ESoundEffect SoundEffect)
{
	return SoundEffectLibrary.Find(SoundEffect);
}
