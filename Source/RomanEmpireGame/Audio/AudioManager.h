// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RomanEmpireGame/Audio/SoundData.h"
#include "AudioManager.generated.h"

class UAudioComponent;

/**
 * Centralized audio manager for all game sounds
 * Handles sound effects, music, and ambient audio
 */
UCLASS()
class ROMANEMPIREGAME_API AAudioManager : public AActor
{
	GENERATED_BODY()

public:
	AAudioManager();

	virtual void BeginPlay() override;

	// Sound Effects
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlaySoundEffect(ESoundEffect SoundEffect, FVector Location = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlaySoundEffectAttached(ESoundEffect SoundEffect, USceneComponent* AttachComponent);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlaySound2D(ESoundEffect SoundEffect);

	// Music
	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayMusic(FName TrackID);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopMusic(float FadeOutTime = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void CrossfadeToMusic(FName NewTrackID, float CrossfadeTime = 3.0f);

	// Ambient
	UFUNCTION(BlueprintCallable, Category = "Audio|Ambient")
	void PlayAmbientLoop(ESoundEffect AmbientSound);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambient")
	void StopAmbientLoop();

	// Volume control
	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetAmbientVolume(float Volume);

	// Singleton access
	UFUNCTION(BlueprintPure, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static AAudioManager* GetAudioManager(UObject* WorldContextObject);

protected:
	// Sound effect library
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio|Library")
	TMap<ESoundEffect, FSoundEffectData> SoundEffectLibrary;

	// Music library
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio|Library")
	TMap<FName, FMusicTrackData> MusicLibrary;

	// Audio components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Components")
	UAudioComponent* MusicComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Components")
	UAudioComponent* AmbientComponent;

	// Volume settings
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Settings")
	float MasterVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Settings")
	float SFXVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Settings")
	float MusicVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Settings")
	float AmbientVolume;

	// Current state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|State")
	FName CurrentMusicTrack;

private:
	void InitializeSoundLibrary();
	FSoundEffectData* GetSoundData(ESoundEffect SoundEffect);
};
