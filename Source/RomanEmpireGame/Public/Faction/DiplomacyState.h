// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "DiplomacyState.generated.h"

/** Canonical relationship states. Invalid also represents invalid/self queries. */
UENUM(BlueprintType)
enum class EDiplomaticStatus : uint8
{
	Invalid	UMETA(DisplayName = "Invalid"),
	War		UMETA(DisplayName = "At War"),
	Neutral	UMETA(DisplayName = "Neutral"),
	Allied	UMETA(DisplayName = "Allied")
};

/** A normalized unordered pair. FirstFaction is always less than SecondFaction. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FDiplomaticPair
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Diplomacy")
	EFactionID FirstFaction = EFactionID::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Diplomacy")
	EFactionID SecondFaction = EFactionID::None;

	bool IsValid() const;
	bool operator==(const FDiplomaticPair& Other) const;
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FDiplomaticRelation
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Diplomacy")
	FDiplomaticPair Pair;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Diplomacy")
	EDiplomaticStatus Status = EDiplomaticStatus::Neutral;
};

UENUM(BlueprintType)
enum class EDiplomaticChangeFailure : uint8
{
	None			UMETA(DisplayName = "None"),
	InvalidFaction	UMETA(DisplayName = "Invalid Faction"),
	SelfRelationship UMETA(DisplayName = "Self Relationship"),
	InvalidStatus	UMETA(DisplayName = "Invalid Status"),
	IllegalTransition UMETA(DisplayName = "Illegal Transition"),
	AlreadyInStatus	UMETA(DisplayName = "Already In Status"),
	MutationInProgress UMETA(DisplayName = "Mutation In Progress")
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FDiplomaticChangeResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diplomacy")
	EDiplomaticChangeFailure Failure = EDiplomaticChangeFailure::InvalidFaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diplomacy")
	FDiplomaticPair Pair;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diplomacy")
	EDiplomaticStatus PreviousStatus = EDiplomaticStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diplomacy")
	EDiplomaticStatus RequestedStatus = EDiplomaticStatus::Invalid;

	bool WasSuccessful() const { return Failure == EDiplomaticChangeFailure::None; }
};

/** Serializable diplomacy state keyed once for each unordered faction pair. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FDiplomacyState
{
	GENERATED_BODY()

	EDiplomaticStatus GetStatus(EFactionID FactionA, EFactionID FactionB) const;
	FDiplomaticChangeResult TrySetStatus(
		EFactionID FactionA,
		EFactionID FactionB,
		EDiplomaticStatus NewStatus);
	int32 GetStoredRelationCount() const { return RelationsByPair.Num(); }
	TArray<FDiplomaticRelation> GetSnapshot() const;
	/** Stored non-neutral normalized pairs only. Atomic replacement, not a transition command. */
	bool ReplaceFromSnapshot(const TArray<FDiplomaticRelation>& Snapshot);
	void Reset() { RelationsByPair.Reset(); }

private:
	UPROPERTY()
	TMap<int32, FDiplomaticRelation> RelationsByPair;
};

namespace RomanEmpireDiplomacy
{
	ROMANEMPIREGAME_API bool IsValidFaction(EFactionID FactionID);
	ROMANEMPIREGAME_API FDiplomaticPair NormalizePair(EFactionID FactionA, EFactionID FactionB);
	ROMANEMPIREGAME_API bool IsLegalTransition(
		EDiplomaticStatus CurrentStatus,
		EDiplomaticStatus NewStatus);
	ROMANEMPIREGAME_API int32 GetPairKey(const FDiplomaticPair& Pair);
	ROMANEMPIREGAME_API const TCHAR* GetChangeFailureName(EDiplomaticChangeFailure Failure);
}
