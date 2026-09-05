// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "BuildingTypes.generated.h"

class UTexture2D;

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

	/** Stable content identity used by runtime factories and future save records. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FName BuildingID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	EBuildingType BuildingType = EBuildingType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	EBuildingCategory Category = EBuildingCategory::Civil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TSoftObjectPtr<UTexture2D> Icon;

	// Costs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Cost")
	FFactionResourceAmounts Cost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Cost")
	float ConstructionTime = 0.0f; // In seconds

	// Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Stats")
	int32 MaxHealth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Stats")
	int32 Armor = 0;

	// Size for placement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Size")
	FVector2D FootprintSize = FVector2D::ZeroVector; // Width x Depth in grid units

	/** Temporary primitive presentation, authored in data until production art replaces it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Prototype Visuals")
	FVector PrototypeScale = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Prototype Visuals")
	FLinearColor PrototypeColor = FLinearColor::White;

	// Production (for economic buildings)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Production")
	EResourceType ProducedResource = EResourceType::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Production")
	int32 ProductionRate = 0; // Per turn

	// Units that can be trained (for military buildings)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Units")
	TArray<FName> TrainableUnits;

	// Requirements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Requirements")
	TArray<EBuildingType> RequiredBuildings;

};

