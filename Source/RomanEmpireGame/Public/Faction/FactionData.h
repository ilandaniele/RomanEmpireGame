// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FactionData.generated.h"

class UTexture2D;

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
 * Typed outcome for any resource transaction.
 */
UENUM(BlueprintType)
enum class EResourceTransactionFailure : uint8
{
	None				UMETA(DisplayName = "None"),
	InvalidFaction		UMETA(DisplayName = "Invalid Faction"),
	InvalidCost			UMETA(DisplayName = "Invalid Cost"),
	InsufficientGold	UMETA(DisplayName = "Insufficient Gold"),
	InsufficientFood	UMETA(DisplayName = "Insufficient Food"),
	InsufficientIron	UMETA(DisplayName = "Insufficient Iron"),
	InsufficientWood	UMETA(DisplayName = "Insufficient Wood"),
	InsufficientStone	UMETA(DisplayName = "Insufficient Stone"),
	InsufficientPopulation UMETA(DisplayName = "Insufficient Population"),
	ArithmeticOverflow	UMETA(DisplayName = "Arithmetic Overflow")
};

/**
 * Non-negative resource amounts used for wallets, costs, and snapshots.
 * Zero defaults prevent an empty cost or missing lookup from minting resources.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FFactionResourceAmounts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Food = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Iron = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Wood = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Stone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Population = 0;

	bool IsZero() const;
	bool IsNonNegative() const;
};

/**
 * Signed change to a resource wallet. Deltas are applied atomically.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FFactionResourceDelta
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Food = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Iron = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Wood = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Stone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Population = 0;

	bool IsZero() const;
};

/**
 * Immutable audit record returned by a resource transaction.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FFactionResourceTransactionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	EResourceTransactionFailure Failure = EResourceTransactionFailure::InvalidFaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	FFactionResourceAmounts BalanceBefore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	FFactionResourceAmounts BalanceAfter;

	bool WasSuccessful() const { return Failure == EResourceTransactionFailure::None; }
};

namespace RomanEmpireEconomy
{
	ROMANEMPIREGAME_API FFactionResourceTransactionResult TryApplyResourceDelta(
		FFactionResourceAmounts& InOutBalance,
		const FFactionResourceDelta& Delta);

	ROMANEMPIREGAME_API FFactionResourceTransactionResult TrySpendResources(
		FFactionResourceAmounts& InOutBalance,
		const FFactionResourceAmounts& Cost);

	ROMANEMPIREGAME_API FFactionResourceAmounts GetPrototypeStartingResources(EFactionID FactionID);
	ROMANEMPIREGAME_API const TCHAR* GetTransactionFailureName(EResourceTransactionFailure Failure);
}

/**
 * Data structure holding faction information
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FFactionInfo
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

	// Explicit starting wallet. Gameplay state copies this value during initialization.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction|Resources")
	FFactionResourceAmounts StartingResources;

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
 * Diplomatic relationship data
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FDiplomaticRelation
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

