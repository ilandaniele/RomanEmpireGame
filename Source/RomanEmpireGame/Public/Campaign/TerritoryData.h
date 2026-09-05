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
	InvalidOwner	UMETA(DisplayName = "Invalid Owner")
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

/** Canonical serializable campaign territory state. Presentation actors never own ownership. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryCampaignState
{
	GENERATED_BODY()

	FTerritoryRegistrationResult RegisterTerritory(const FTerritoryDefinition& Definition);
	FTerritoryOwnershipChangeResult ChangeOwner(FName TerritoryID, EFactionID NewOwner);
	bool TryGetDefinition(FName TerritoryID, FTerritoryDefinition& OutDefinition) const;
	bool TryGetOwner(FName TerritoryID, EFactionID& OutOwner) const;
	int32 GetFactionTerritoryCount(EFactionID FactionID) const;
	int32 GetTerritoryCount() const { return TerritoryDefinitions.Num(); }
	void Reset();

private:
	UPROPERTY()
	TMap<FName, FTerritoryDefinition> TerritoryDefinitions;

	UPROPERTY()
	TMap<FName, EFactionID> TerritoryOwnership;
};

namespace RomanEmpireTerritories
{
	ROMANEMPIREGAME_API bool IsValidFactionOrUnowned(EFactionID FactionID);
	ROMANEMPIREGAME_API const TCHAR* GetRegistrationFailureName(ETerritoryRegistrationFailure Failure);
	ROMANEMPIREGAME_API const TCHAR* GetOwnershipFailureName(ETerritoryOwnershipFailure Failure);
}
