// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "BuildingTypes.generated.h"

/**
 * Building category for UI organization
 */
UENUM(BlueprintType)
enum class EBuildingCategory : uint8
{
	Military	UMETA(DisplayName = "Military"),
	Economic	UMETA(DisplayName = "Economic"),
	Civil		UMETA(DisplayName = "Civil"),
	Defense		UMETA(DisplayName = "Defense")
};

/**
 * Building type identifier
 */
UENUM(BlueprintType)
enum class EBuildingType : uint8
{
	None		UMETA(DisplayName = "None"),
	
	// Military buildings
	Barracks	UMETA(DisplayName = "Barracks"),       // Train infantry
	ArcheryRange UMETA(DisplayName = "Archery Range"), // Train archers
	Stable		UMETA(DisplayName = "Stable"),         // Train cavalry
	Armory		UMETA(DisplayName = "Armory"),         // Upgrade weapons
	
	// Economic buildings
	Farm		UMETA(DisplayName = "Farm"),           // Food production
	Mine		UMETA(DisplayName = "Mine"),           // Iron/gold production
	Lumbercamp	UMETA(DisplayName = "Lumber Camp"),    // Wood production
	Market		UMETA(DisplayName = "Market"),         // Trade
	
	// Civil buildings
	Forum		UMETA(DisplayName = "Forum"),          // City center/administration
	Temple		UMETA(DisplayName = "Temple"),         // Religion/morale
	Aqueduct	UMETA(DisplayName = "Aqueduct"),       // Population growth
	Amphitheater UMETA(DisplayName = "Amphitheater"),  // Entertainment
	
	// Defense buildings
	Wall		UMETA(DisplayName = "Wall"),           // Defense wall segment
	Tower		UMETA(DisplayName = "Tower"),          // Defense/vision tower
	Gate		UMETA(DisplayName = "Gate"),           // Wall gate
	Fort		UMETA(DisplayName = "Fort")            // Military outpost
};

/**
 * Building construction state
 */
UENUM(BlueprintType)
enum class EBuildingState : uint8
{
	Placing		UMETA(DisplayName = "Placing"),        // Being placed by player
	Constructing UMETA(DisplayName = "Constructing"),  // Under construction
	Complete	UMETA(DisplayName = "Complete"),       // Fully built
	Damaged		UMETA(DisplayName = "Damaged"),        // Needs repair
	Destroyed	UMETA(DisplayName = "Destroyed")       // Destroyed
};

/**
 * Data describing a building type
 */
USTRUCT(BlueprintType)
struct FBuildingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	EBuildingType BuildingType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	EBuildingCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	UTexture2D* Icon;

	// Costs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Cost")
	FFactionResources Cost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Cost")
	float ConstructionTime; // In seconds

	// Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Stats")
	int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Stats")
	int32 Armor;

	// Size for placement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Size")
	FVector2D FootprintSize; // Width x Depth in grid units

	// Production (for economic buildings)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Production")
	EResourceType ProducedResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Production")
	int32 ProductionRate; // Per turn

	// Units that can be trained (for military buildings)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Units")
	TArray<FName> TrainableUnits;

	// Requirements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Requirements")
	TArray<EBuildingType> RequiredBuildings;

	FBuildingData()
		: BuildingType(EBuildingType::None)
		, Category(EBuildingCategory::Civil)
		, Icon(nullptr)
		, ConstructionTime(30.0f)
		, MaxHealth(1000)
		, Armor(5)
		, FootprintSize(3.0f, 3.0f)
		, ProducedResource(EResourceType::Gold)
		, ProductionRate(0)
	{}
};
