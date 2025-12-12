// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoundData.generated.h"

/**
 * Sound category for organization
 */
UENUM(BlueprintType)
enum class ESoundCategory : uint8
{
	Combat		UMETA(DisplayName = "Combat"),
	Ambient		UMETA(DisplayName = "Ambient"),
	UI			UMETA(DisplayName = "UI"),
	Music		UMETA(DisplayName = "Music"),
	Voice		UMETA(DisplayName = "Voice")
};

/**
 * Specific sound effect identifiers
 */
UENUM(BlueprintType)
enum class ESoundEffect : uint8
{
	// Combat sounds
	SwordSwing,
	SwordHit,
	ShieldBlock,
	ArrowFire,
	ArrowHit,
	PilumThrow,
	Death,
	Charge,
	
	// Building sounds
	BuildStart,
	BuildProgress,
	BuildComplete,
	BuildingDamaged,
	BuildingDestroyed,
	
	// Unit sounds
	UnitSelect,
	UnitMove,
	UnitAttack,
	UnitAcknowledge,
	
	// UI sounds
	ButtonClick,
	ButtonHover,
	MenuOpen,
	MenuClose,
	ErrorSound,
	VictoryFanfare,
	DefeatSound,
	TurnEnd,
	
	// Ambient
	CrowdCheer,
	BattleAmbience,
	CityAmbience,
	NatureAmbience,
	MarchingFootsteps
};

/**
 * Data for a single sound effect
 */
USTRUCT(BlueprintType)
struct FSoundEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	ESoundEffect SoundID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	ESoundCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float VolumeMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float PitchMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bIs3D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float AttenuationRadius;

	FSoundEffectData()
		: SoundID(ESoundEffect::SwordSwing)
		, Category(ESoundCategory::Combat)
		, Sound(nullptr)
		, VolumeMultiplier(1.0f)
		, PitchMultiplier(1.0f)
		, bIs3D(true)
		, AttenuationRadius(2000.0f)
	{}
};

/**
 * Music track data
 */
USTRUCT(BlueprintType)
struct FMusicTrackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	FName TrackID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	USoundBase* Track;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float FadeInTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float FadeOutTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bLoop;

	FMusicTrackData()
		: FadeInTime(2.0f)
		, FadeOutTime(2.0f)
		, bLoop(true)
		, Track(nullptr)
	{}
};
