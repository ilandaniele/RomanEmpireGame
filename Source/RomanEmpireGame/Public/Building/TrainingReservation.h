// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "TrainingReservation.generated.h"

/** Immutable acceptance receipt owned by the producer, not a current catalog price. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTrainingReservation
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Production")
	FGuid ReceiptID;
	/** Captured destination; never resolve a different army when the order finishes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Production")
	FGuid EncounterID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Production")
	FGuid CampaignArmyID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Production")
	FName UnitID = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Production")
	EFactionID PayingFaction = EFactionID::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Production")
	FFactionResourceAmounts PaidCost;
};
