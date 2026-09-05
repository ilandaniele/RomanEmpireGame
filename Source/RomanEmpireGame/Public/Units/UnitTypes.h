// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "UnitTypes.generated.h"

class UTexture2D;

/**
 * Unit type category
 */
UENUM(BlueprintType)
enum class EUnitCategory : uint8
{
	Infantry	UMETA(DisplayName = "Infantry"),
	Cavalry		UMETA(DisplayName = "Cavalry"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Siege		UMETA(DisplayName = "Siege")
};

/**
 * Specific Roman unit types
 */
UENUM(BlueprintType)
enum class EUnitType : uint8
{
	None			UMETA(DisplayName = "None"),
	
	// Infantry
	Legionary		UMETA(DisplayName = "Legionary"),      // Heavy infantry
	Centurion		UMETA(DisplayName = "Centurion"),      // Elite commander
	Velites			UMETA(DisplayName = "Velites"),        // Light infantry
	Triarii			UMETA(DisplayName = "Triarii"),        // Veteran spearmen
	Praetorian		UMETA(DisplayName = "Praetorian"),     // Elite guards
	
	// Ranged
	Sagittarii		UMETA(DisplayName = "Sagittarii"),     // Archers
	Javelinmen		UMETA(DisplayName = "Javelinmen"),     // Javelin throwers
	Slingers		UMETA(DisplayName = "Slingers"),       // Slingers
	
	// Cavalry  
	Equites			UMETA(DisplayName = "Equites"),        // Light cavalry
	CavalryArcher	UMETA(DisplayName = "Cavalry Archer"), // Mounted archer
	
	// Siege
	Onager			UMETA(DisplayName = "Onager"),         // Catapult
	Ballista		UMETA(DisplayName = "Ballista"),       // Bolt thrower
	Ram				UMETA(DisplayName = "Battering Ram")   // Wall breaker
};

/**
 * Unit combat stance
 */
UENUM(BlueprintType)
enum class EUnitStance : uint8
{
	Aggressive	UMETA(DisplayName = "Aggressive"),   // Attack on sight
	Defensive	UMETA(DisplayName = "Defensive"),    // Hold position
	StandGround	UMETA(DisplayName = "Stand Ground"), // Don't move at all
	Skirmish	UMETA(DisplayName = "Skirmish")      // Hit and run
};

/**
 * Formation type for groups of units
 */
UENUM(BlueprintType)
enum class EFormationType : uint8
{
	None		UMETA(DisplayName = "None"),
	Line		UMETA(DisplayName = "Line"),         // Standard line
	Column		UMETA(DisplayName = "Column"),       // Marching column
	Wedge		UMETA(DisplayName = "Wedge"),        // Cavalry wedge
	Square		UMETA(DisplayName = "Square"),       // Defensive square
	Testudo		UMETA(DisplayName = "Testudo"),      // Turtle formation (shields up)
	Circle		UMETA(DisplayName = "Circle")        // Last stand
};

/**
 * Unit stats structure
 */
USTRUCT(BlueprintType)
struct FUnitStats
{
	GENERATED_BODY()

	// Base stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MaxHealth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MeleeAttack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 RangedAttack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MeleeDefense = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 RangedDefense = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Armor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Speed = 0.0f;

	// Morale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Morale = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Stamina = 0.0f;

	// FPS mode stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|FPS")
	float AttackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|FPS")
	float BlockStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|FPS")
	float DodgeSpeed = 0.0f;
};

/**
 * Data describing a unit type
 */
USTRUCT(BlueprintType)
struct FUnitData
{
	GENERATED_BODY()

	/** Stable content identity used by runtime factories and future save records. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FName UnitID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	EUnitType UnitType = EUnitType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	EUnitCategory Category = EUnitCategory::Infantry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FUnitStats BaseStats;

	// Cost reserved atomically when the unit enters a training queue
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Cost")
	FFactionResourceAmounts TrainingCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Cost")
	float TrainingTime = 0.0f;

	// Combat range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	float AttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	bool bCanUseRanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	int32 StartingRangedAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	float SpecialRangedDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	float SpecialRangedRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Formation")
	float DefensiveFormationSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Formation")
	int32 DefensiveFormationMeleeDefenseBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Formation")
	int32 DefensiveFormationRangedDefenseBonus = 0;

	// Equipment for FPS mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Equipment")
	FName PrimaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Equipment")
	FName SecondaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Equipment")
	bool bHasShield = false;
};

