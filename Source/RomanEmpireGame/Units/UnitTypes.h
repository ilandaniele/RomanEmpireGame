// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnitTypes.generated.h"

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
	int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MeleeAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 RangedAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MeleeDefense;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 RangedDefense;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Armor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Speed;

	// Morale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Morale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Stamina;

	// FPS mode stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|FPS")
	float AttackSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|FPS")
	float BlockStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|FPS")
	float DodgeSpeed;

	FUnitStats()
		: MaxHealth(100)
		, MeleeAttack(10)
		, RangedAttack(0)
		, MeleeDefense(8)
		, RangedDefense(5)
		, Armor(3)
		, Speed(300.0f)
		, Morale(50)
		, Stamina(100.0f)
		, AttackSpeed(1.0f)
		, BlockStrength(1.0f)
		, DodgeSpeed(1.0f)
	{}
};

/**
 * Data describing a unit type
 */
USTRUCT(BlueprintType)
struct FUnitData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	EUnitType UnitType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	EUnitCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FUnitStats BaseStats;

	// Cost to train
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Cost")
	int32 GoldCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Cost")
	int32 FoodCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Cost")
	float TrainingTime;

	// Combat range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	bool bCanUseRanged;

	// Equipment for FPS mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Equipment")
	FName PrimaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Equipment")
	FName SecondaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Equipment")
	bool bHasShield;

	FUnitData()
		: UnitType(EUnitType::None)
		, Category(EUnitCategory::Infantry)
		, Icon(nullptr)
		, GoldCost(100)
		, FoodCost(50)
		, TrainingTime(10.0f)
		, AttackRange(150.0f)
		, bCanUseRanged(false)
		, bHasShield(true)
	{}
};
