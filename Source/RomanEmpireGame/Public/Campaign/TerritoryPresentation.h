// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Campaign/TerritoryData.h"
#include "TerritoryPresentation.generated.h"

/** Detached read-only presentation value, not campaign/save authority.
 * Base production is authored, NOT effective settlement production or a wallet credit.
 * Unowned territories have an empty owner label and a neutral white color. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTerritoryPresentationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FName TerritoryID = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FText DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FText Description;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	ETerrainType TerrainType = ETerrainType::Plains;
	/** Authored map placement, independent of presentation actor transforms. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FVector MapLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FFactionResourceDelta BaseResourceProduction;
	/** Current live settlement; never inferred from authored starting names. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FTerritorySettlementState Settlement;
	/** Base + live settlement bonus; dormant/unowned yield is not a wallet credit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FFactionResourceDelta EffectiveResourceProduction;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	EResourceType BonusResource = EResourceType::Gold;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	TArray<FName> AdjacentTerritoryIDs;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	EFactionID OwnerFaction = EFactionID::None;
	/** Catalog metadata also exists for dormant factions such as Gaul. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FText OwnerDisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	FLinearColor OwnerColor = FLinearColor::White;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory")
	bool bOwnerActive = false;
};
