// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "TacticalBattleResult.generated.h"

/** Tactical resolution is separate from the campaign's terminal outcome. */
UENUM(BlueprintType)
enum class ETacticalBattleOutcome : uint8
{
	InProgress UMETA(DisplayName = "In Progress"),
	PlayerVictory UMETA(DisplayName = "Player Victory"),
	PlayerDefeat UMETA(DisplayName = "Player Defeat"),
	Draw UMETA(DisplayName = "Draw")
};

/** World/session-local squad handle; M5 must map it to persistent campaign identities. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTacticalBattleSquadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	FGuid SquadID;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID Faction = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	int32 InitialUnits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	int32 Survivors = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	int32 Casualties = 0;
};

/** Immutable snapshot published by the one tactical victory evaluator. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FTacticalBattleResult
{
	GENERATED_BODY()

	/** Distinct from squad/army IDs; prevents applying a different encounter's result. */
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	FGuid EncounterID;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	ETacticalBattleOutcome Outcome = ETacticalBattleOutcome::InProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID Winner = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	FName ObjectiveID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	FName CapturedObjectiveID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TArray<FTacticalBattleSquadResult> Squads;

	bool IsResolved() const { return Outcome != ETacticalBattleOutcome::InProgress; }
};
