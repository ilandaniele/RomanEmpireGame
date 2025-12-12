// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FactionData.generated.h"

/**
 * Faction identifier enum
 */
UENUM(BlueprintType)
enum class EFactionID : uint8
{
	None		UMETA(DisplayName = "None"),
	Rome		UMETA(DisplayName = "Roman Republic"),
	Carthage	UMETA(DisplayName = "Carthage"),
	Gaul		UMETA(DisplayName = "Gallic Tribes"),
	Greece		UMETA(DisplayName = "Greek States"),
	Egypt		UMETA(DisplayName = "Ptolemaic Egypt"),
	Britannia	UMETA(DisplayName = "Briton Tribes")
};

/**
 * Diplomatic relationship between factions
 */
UENUM(BlueprintType)
enum class EDiplomaticStatus : uint8
{
	War			UMETA(DisplayName = "At War"),
	Hostile		UMETA(DisplayName = "Hostile"),
	Neutral		UMETA(DisplayName = "Neutral"),
	Friendly	UMETA(DisplayName = "Friendly"),
	Allied		UMETA(DisplayName = "Allied")
};

/**
 * Resource types in the game
 */
UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Gold		UMETA(DisplayName = "Gold"),
	Food		UMETA(DisplayName = "Food"),
	Iron		UMETA(DisplayName = "Iron"),
	Wood		UMETA(DisplayName = "Wood"),
	Stone		UMETA(DisplayName = "Stone"),
	Population	UMETA(DisplayName = "Population")
};

/**
 * Data structure holding faction information
 */
USTRUCT(BlueprintType)
struct FFactionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EFactionID FactionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FLinearColor FactionColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	UTexture2D* FactionBanner;

	// Starting bonuses
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction|Bonuses")
	float InfantryBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction|Bonuses")
	float CavalryBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction|Bonuses")
	float EconomyBonus;

	// Unique units available to this faction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction|Units")
	TArray<FName> UniqueUnitTypes;

	// Unique buildings available to this faction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction|Buildings")
	TArray<FName> UniqueBuildingTypes;

	FFactionInfo()
		: FactionID(EFactionID::None)
		, FactionColor(FLinearColor::White)
		, FactionBanner(nullptr)
		, InfantryBonus(1.0f)
		, CavalryBonus(1.0f)
		, EconomyBonus(1.0f)
	{}
};

/**
 * Resources owned by a faction
 */
USTRUCT(BlueprintType)
struct FFactionResources
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Food;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Iron;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Wood;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Stone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Population;

	FFactionResources()
		: Gold(1000)
		, Food(500)
		, Iron(200)
		, Wood(300)
		, Stone(200)
		, Population(100)
	{}

	bool CanAfford(const FFactionResources& Cost) const
	{
		return Gold >= Cost.Gold &&
			   Food >= Cost.Food &&
			   Iron >= Cost.Iron &&
			   Wood >= Cost.Wood &&
			   Stone >= Cost.Stone &&
			   Population >= Cost.Population;
	}

	void Deduct(const FFactionResources& Cost)
	{
		Gold -= Cost.Gold;
		Food -= Cost.Food;
		Iron -= Cost.Iron;
		Wood -= Cost.Wood;
		Stone -= Cost.Stone;
		Population -= Cost.Population;
	}
};

/**
 * Diplomatic relationship data
 */
USTRUCT(BlueprintType)
struct FDiplomaticRelation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diplomacy")
	EFactionID OtherFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diplomacy")
	EDiplomaticStatus Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diplomacy")
	int32 RelationshipScore; // -100 to +100

	FDiplomaticRelation()
		: OtherFaction(EFactionID::None)
		, Status(EDiplomaticStatus::Neutral)
		, RelationshipScore(0)
	{}
};
