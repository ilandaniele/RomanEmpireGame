// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "TerritoryData.generated.h"

/** Terrain classification used by immutable territory definitions. */
UENUM(BlueprintType)
enum class ETerrainType : uint8
{
	Plains		UMETA(DisplayName = "Plains"),
	Forest		UMETA(DisplayName = "Forest"),
	Mountain	UMETA(DisplayName = "Mountain"),
	Desert		UMETA(DisplayName = "Desert"),
	Coast		UMETA(DisplayName = "Coast")
};

/** Stable identity and authored presentation/gameplay data for one territory. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
	FName TerritoryID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
	ETerrainType TerrainType = ETerrainType::Plains;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Resources")
	FFactionResourceDelta ResourceProduction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Resources")
	EResourceType BonusResource = EResourceType::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Campaign")
	EFactionID StartingOwner = EFactionID::None;

	/** Authored undirected campaign connections; actor placement is presentation only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Campaign")
	TArray<FName> AdjacentTerritoryIDs;

	bool IsValid() const;
};

UENUM(BlueprintType)
enum class ETerritoryRegistrationFailure : uint8
{
	None				UMETA(DisplayName = "None"),
	InvalidDefinition	UMETA(DisplayName = "Invalid Definition"),
	DuplicateID			UMETA(DisplayName = "Duplicate ID")
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryRegistrationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	ETerritoryRegistrationFailure Failure = ETerritoryRegistrationFailure::InvalidDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FName TerritoryID = NAME_None;

	bool WasSuccessful() const { return Failure == ETerritoryRegistrationFailure::None; }
};

UENUM(BlueprintType)
enum class ETerritoryOwnershipFailure : uint8
{
	None			UMETA(DisplayName = "None"),
	UnknownTerritory UMETA(DisplayName = "Unknown Territory"),
	InvalidOwner	UMETA(DisplayName = "Invalid Owner"),
	TransactionInProgress UMETA(DisplayName = "Transaction In Progress")
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryOwnershipChangeResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	ETerritoryOwnershipFailure Failure = ETerritoryOwnershipFailure::UnknownTerritory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FName TerritoryID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	EFactionID PreviousOwner = EFactionID::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	EFactionID NewOwner = EFactionID::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	bool bOwnershipChanged = false;

	bool WasSuccessful() const { return Failure == ETerritoryOwnershipFailure::None; }
};

/** Live campaign value, independent of settlement/building presentation actors. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritorySettlementState
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settlement")
	bool bHasSettlement = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settlement")
	FText SettlementName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settlement")
	int32 Population = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settlement")
	FFactionResourceDelta ProductionBonus;
};

/** Player-entered city name is durable text, not an Editor-generated localization key. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritorySettlementSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Settlement") bool bHasSettlement = false;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Settlement") FString SettlementName;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Settlement") int32 Population = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Settlement") FFactionResourceDelta ProductionBonus;
};

/** Mutable campaign values only; definitions/adjacency are rebuilt from the current catalog. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryRuntimeSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Territory") FName TerritoryID = NAME_None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Territory") EFactionID Owner = EFactionID::None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Territory") FTerritorySettlementSnapshot Settlement;
};

UENUM(BlueprintType)
enum class ESettlementFoundingFailure : uint8
{
	None, UnknownTerritory, InvalidName, AlreadyFounded, ProductionOverflow, TransactionInProgress
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FSettlementFoundingResult
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settlement")
	ESettlementFoundingFailure Failure = ESettlementFoundingFailure::UnknownTerritory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settlement")
	FName TerritoryID = NAME_None;
	bool WasSuccessful() const { return Failure == ESettlementFoundingFailure::None; }
};

/** Canonical serializable campaign territory state. Actors never own live campaign values. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryCampaignState
{
	GENERATED_BODY()

	FTerritoryRegistrationResult RegisterTerritory(const FTerritoryDefinition& Definition);
	FTerritoryOwnershipChangeResult ChangeOwner(FName TerritoryID, EFactionID NewOwner);
	FSettlementFoundingResult FoundSettlement(FName TerritoryID, const FText& Name);
	bool TryGetSettlement(FName TerritoryID, FTerritorySettlementState& OutSettlement) const;
	bool TryGetResourceProduction(FName TerritoryID, FFactionResourceDelta& OutProduction) const;
	bool TryGetDefinition(FName TerritoryID, FTerritoryDefinition& OutDefinition) const;
	bool TryGetOwner(FName TerritoryID, EFactionID& OutOwner) const;
	int32 GetFactionTerritoryCount(EFactionID FactionID) const;
	int32 GetTerritoryCount() const { return TerritoryDefinitions.Num(); }
	TArray<FName> GetTerritoryIDs() const;
	bool HasValidAdjacency() const;
	bool AreAdjacent(FName FirstID, FName SecondID) const;
	TArray<FName> GetAdjacentIDs(FName TerritoryID) const;
	TArray<FTerritoryRuntimeSnapshot> GetRuntimeSnapshot() const;
	/** Exact coverage of registered IDs, valid settlements/yields; failure changes nothing. */
	bool ReplaceRuntimeSnapshot(const TArray<FTerritoryRuntimeSnapshot>& Snapshot);
	void Reset();

private:
	UPROPERTY()
	TMap<FName, FTerritoryDefinition> TerritoryDefinitions;

	UPROPERTY()
	TMap<FName, EFactionID> TerritoryOwnership;

	UPROPERTY()
	TMap<FName, FTerritorySettlementState> TerritorySettlements;
};

namespace RomanEmpireTerritories
{
	ROMANEMPIREGAME_API bool IsValidFactionOrUnowned(EFactionID FactionID);
	ROMANEMPIREGAME_API const TCHAR* GetRegistrationFailureName(ETerritoryRegistrationFailure Failure);
	ROMANEMPIREGAME_API const TCHAR* GetOwnershipFailureName(ETerritoryOwnershipFailure Failure);
}
