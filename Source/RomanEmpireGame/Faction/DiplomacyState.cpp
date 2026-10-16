// Copyright Roman Empire Game. All Rights Reserved.

#include "Faction/DiplomacyState.h"

bool FDiplomaticPair::IsValid() const
{
	return RomanEmpireDiplomacy::IsValidFaction(FirstFaction) &&
		RomanEmpireDiplomacy::IsValidFaction(SecondFaction) &&
		FirstFaction != SecondFaction &&
		static_cast<uint8>(FirstFaction) < static_cast<uint8>(SecondFaction);
}

bool FDiplomaticPair::operator==(const FDiplomaticPair& Other) const
{
	return FirstFaction == Other.FirstFaction && SecondFaction == Other.SecondFaction;
}

EDiplomaticStatus FDiplomacyState::GetStatus(
	EFactionID FactionA,
	EFactionID FactionB) const
{
	const FDiplomaticPair Pair = RomanEmpireDiplomacy::NormalizePair(FactionA, FactionB);
	if (!Pair.IsValid())
	{
		return EDiplomaticStatus::Invalid;
	}

	if (const FDiplomaticRelation* Relation =
		RelationsByPair.Find(RomanEmpireDiplomacy::GetPairKey(Pair)))
	{
		return Relation->Status;
	}
	return EDiplomaticStatus::Neutral;
}

FDiplomaticChangeResult FDiplomacyState::TrySetStatus(
	EFactionID FactionA,
	EFactionID FactionB,
	EDiplomaticStatus NewStatus)
{
	FDiplomaticChangeResult Result;
	Result.RequestedStatus = NewStatus;

	if (!RomanEmpireDiplomacy::IsValidFaction(FactionA) ||
		!RomanEmpireDiplomacy::IsValidFaction(FactionB))
	{
		return Result;
	}

	if (FactionA == FactionB)
	{
		Result.Failure = EDiplomaticChangeFailure::SelfRelationship;
		return Result;
	}

	Result.Pair = RomanEmpireDiplomacy::NormalizePair(FactionA, FactionB);
	Result.PreviousStatus = GetStatus(FactionA, FactionB);
	if (NewStatus != EDiplomaticStatus::War &&
		NewStatus != EDiplomaticStatus::Neutral &&
		NewStatus != EDiplomaticStatus::Allied)
	{
		Result.Failure = EDiplomaticChangeFailure::InvalidStatus;
		return Result;
	}

	if (Result.PreviousStatus == NewStatus)
	{
		Result.Failure = EDiplomaticChangeFailure::AlreadyInStatus;
		return Result;
	}

	if (!RomanEmpireDiplomacy::IsLegalTransition(Result.PreviousStatus, NewStatus))
	{
		Result.Failure = EDiplomaticChangeFailure::IllegalTransition;
		return Result;
	}

	const int32 PairKey = RomanEmpireDiplomacy::GetPairKey(Result.Pair);
	if (NewStatus == EDiplomaticStatus::Neutral)
	{
		RelationsByPair.Remove(PairKey);
	}
	else
	{
		FDiplomaticRelation Relation;
		Relation.Pair = Result.Pair;
		Relation.Status = NewStatus;
		RelationsByPair.Add(PairKey, Relation);
	}

	Result.Failure = EDiplomaticChangeFailure::None;
	return Result;
}

TArray<FDiplomaticRelation> FDiplomacyState::GetSnapshot() const
{
	TArray<FDiplomaticRelation> Result;
	RelationsByPair.GenerateValueArray(Result);
	Result.Sort([](const FDiplomaticRelation& A, const FDiplomaticRelation& B)
	{ return RomanEmpireDiplomacy::GetPairKey(A.Pair) < RomanEmpireDiplomacy::GetPairKey(B.Pair); });
	return Result;
}

bool FDiplomacyState::ReplaceFromSnapshot(const TArray<FDiplomaticRelation>& Snapshot)
{
	TMap<int32, FDiplomaticRelation> Candidate;
	for (const FDiplomaticRelation& Row : Snapshot)
	{
		if (!Row.Pair.IsValid() || (Row.Status != EDiplomaticStatus::War && Row.Status != EDiplomaticStatus::Allied)) return false;
		const int32 Key = RomanEmpireDiplomacy::GetPairKey(Row.Pair);
		if (Candidate.Contains(Key)) return false;
		Candidate.Add(Key, Row);
	}
	RelationsByPair = MoveTemp(Candidate);
	return true;
}

bool RomanEmpireDiplomacy::IsValidFaction(EFactionID FactionID)
{
	return FactionID != EFactionID::None &&
		static_cast<uint8>(FactionID) <= static_cast<uint8>(EFactionID::Britannia);
}

FDiplomaticPair RomanEmpireDiplomacy::NormalizePair(
	EFactionID FactionA,
	EFactionID FactionB)
{
	FDiplomaticPair Pair;
	if (!IsValidFaction(FactionA) || !IsValidFaction(FactionB) || FactionA == FactionB)
	{
		return Pair;
	}

	if (static_cast<uint8>(FactionA) < static_cast<uint8>(FactionB))
	{
		Pair.FirstFaction = FactionA;
		Pair.SecondFaction = FactionB;
	}
	else
	{
		Pair.FirstFaction = FactionB;
		Pair.SecondFaction = FactionA;
	}
	return Pair;
}

bool RomanEmpireDiplomacy::IsLegalTransition(
	EDiplomaticStatus CurrentStatus,
	EDiplomaticStatus NewStatus)
{
	if (CurrentStatus == EDiplomaticStatus::Neutral)
	{
		return NewStatus == EDiplomaticStatus::War || NewStatus == EDiplomaticStatus::Allied;
	}

	if (CurrentStatus == EDiplomaticStatus::War || CurrentStatus == EDiplomaticStatus::Allied)
	{
		return NewStatus == EDiplomaticStatus::Neutral;
	}
	return false;
}

int32 RomanEmpireDiplomacy::GetPairKey(const FDiplomaticPair& Pair)
{
	return (static_cast<int32>(Pair.FirstFaction) << 8) |
		static_cast<int32>(Pair.SecondFaction);
}

const TCHAR* RomanEmpireDiplomacy::GetChangeFailureName(EDiplomaticChangeFailure Failure)
{
	switch (Failure)
	{
	case EDiplomaticChangeFailure::None: return TEXT("None");
	case EDiplomaticChangeFailure::InvalidFaction: return TEXT("InvalidFaction");
	case EDiplomaticChangeFailure::SelfRelationship: return TEXT("SelfRelationship");
	case EDiplomaticChangeFailure::InvalidStatus: return TEXT("InvalidStatus");
	case EDiplomaticChangeFailure::IllegalTransition: return TEXT("IllegalTransition");
	case EDiplomaticChangeFailure::AlreadyInStatus: return TEXT("AlreadyInStatus");
	default: return TEXT("Unknown");
	}
}
