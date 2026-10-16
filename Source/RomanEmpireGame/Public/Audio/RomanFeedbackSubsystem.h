// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Battle/TacticalBattleResult.h"
#include "Subsystems/WorldSubsystem.h"
#include "RomanFeedbackSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UTacticalBattleSubsystem;
class URomanPlayerSettingsSubsystem;

UENUM(BlueprintType)
enum class ERomanFeedbackCue : uint8 { Select, Order, Complete, Impact, Victory, Defeat };

/** World-local presentation only: no gameplay state, actor scans, ticks or timers. */
UCLASS()
class ROMANEMPIREGAME_API URomanFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	URomanFeedbackSubsystem();
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	UFUNCTION(BlueprintCallable, Category="Audio")
	bool PlayCue(ERomanFeedbackCue Cue, FVector Location = FVector::ZeroVector);
	/** Validated projection of settings; persistence belongs to the settings owner. */
	UFUNCTION(BlueprintCallable, Category="Audio")
	bool SetLevels(float Master, float Effects);
	UFUNCTION(BlueprintPure, Category="Audio")
	float GetMasterLevel() const { return MasterLevel; }
	UFUNCTION(BlueprintPure, Category="Audio")
	float GetEffectsLevel() const { return EffectsLevel; }
	UFUNCTION(BlueprintPure, Category="Audio")
	int32 GetPlayingVoiceCount() const;
private:
	UPROPERTY()
	TMap<ERomanFeedbackCue,TObjectPtr<USoundBase>> Sounds;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> Voices;
	TWeakObjectPtr<UTacticalBattleSubsystem> BattleSource;
	TWeakObjectPtr<URomanPlayerSettingsSubsystem> PreferencesSource;
	FDelegateHandle PreferencesChangedHandle;
	TMap<ERomanFeedbackCue,double> LastCueTimes;
	float MasterLevel = 1.f;
	float EffectsLevel = .65f;
	int32 NextVoice = 0;
	bool bReady = false;
	bool bAnnouncedResult = false;
	FGuid AnnouncedEncounter;
	UFUNCTION()
	void HandleBattleChanged(FTacticalBattleResult Result);
	void HandlePreferencesChanged();
};
