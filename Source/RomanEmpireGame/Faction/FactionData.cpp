// Copyright Roman Empire Game. All Rights Reserved.

#include "Faction/FactionData.h"

namespace
{
	FFactionResourceTransactionResult MakeFailure(
		EResourceTransactionFailure Failure,
		const FFactionResourceAmounts& Balance)
	{
		FFactionResourceTransactionResult Result;
		Result.Failure = Failure;
		Result.BalanceBefore = Balance;
		Result.BalanceAfter = Balance;
		return Result;
	}
}

bool FFactionResourceAmounts::IsZero() const
{
	return Gold == 0 && Food == 0 && Iron == 0 && Wood == 0 && Stone == 0 && Population == 0;
}

bool FFactionResourceAmounts::IsNonNegative() const
{
	return Gold >= 0 && Food >= 0 && Iron >= 0 && Wood >= 0 && Stone >= 0 && Population >= 0;
}

bool FFactionResourceDelta::IsZero() const
{
	return Gold == 0 && Food == 0 && Iron == 0 && Wood == 0 && Stone == 0 && Population == 0;
}

FFactionResourceTransactionResult RomanEmpireEconomy::TryApplyResourceDelta(
	FFactionResourceAmounts& InOutBalance,
	const FFactionResourceDelta& Delta)
{
	const FFactionResourceAmounts Before = InOutBalance;
	const int64 GoldAfter = static_cast<int64>(Before.Gold) + Delta.Gold;
	const int64 FoodAfter = static_cast<int64>(Before.Food) + Delta.Food;
	const int64 IronAfter = static_cast<int64>(Before.Iron) + Delta.Iron;
	const int64 WoodAfter = static_cast<int64>(Before.Wood) + Delta.Wood;
	const int64 StoneAfter = static_cast<int64>(Before.Stone) + Delta.Stone;
	const int64 PopulationAfter = static_cast<int64>(Before.Population) + Delta.Population;

	if (GoldAfter < 0) return MakeFailure(EResourceTransactionFailure::InsufficientGold, Before);
	if (FoodAfter < 0) return MakeFailure(EResourceTransactionFailure::InsufficientFood, Before);
	if (IronAfter < 0) return MakeFailure(EResourceTransactionFailure::InsufficientIron, Before);
	if (WoodAfter < 0) return MakeFailure(EResourceTransactionFailure::InsufficientWood, Before);
	if (StoneAfter < 0) return MakeFailure(EResourceTransactionFailure::InsufficientStone, Before);
	if (PopulationAfter < 0) return MakeFailure(EResourceTransactionFailure::InsufficientPopulation, Before);

	if (GoldAfter > MAX_int32 || FoodAfter > MAX_int32 || IronAfter > MAX_int32 ||
		WoodAfter > MAX_int32 || StoneAfter > MAX_int32 || PopulationAfter > MAX_int32)
	{
		return MakeFailure(EResourceTransactionFailure::ArithmeticOverflow, Before);
	}

	InOutBalance.Gold = static_cast<int32>(GoldAfter);
	InOutBalance.Food = static_cast<int32>(FoodAfter);
	InOutBalance.Iron = static_cast<int32>(IronAfter);
	InOutBalance.Wood = static_cast<int32>(WoodAfter);
	InOutBalance.Stone = static_cast<int32>(StoneAfter);
	InOutBalance.Population = static_cast<int32>(PopulationAfter);

	FFactionResourceTransactionResult Result;
	Result.Failure = EResourceTransactionFailure::None;
	Result.BalanceBefore = Before;
	Result.BalanceAfter = InOutBalance;
	return Result;
}

FFactionResourceTransactionResult RomanEmpireEconomy::TrySpendResources(
	FFactionResourceAmounts& InOutBalance,
	const FFactionResourceAmounts& Cost)
{
	if (!Cost.IsNonNegative())
	{
		return MakeFailure(EResourceTransactionFailure::InvalidCost, InOutBalance);
	}

	FFactionResourceDelta SpendDelta;
	SpendDelta.Gold = -Cost.Gold;
	SpendDelta.Food = -Cost.Food;
	SpendDelta.Iron = -Cost.Iron;
	SpendDelta.Wood = -Cost.Wood;
	SpendDelta.Stone = -Cost.Stone;
	SpendDelta.Population = -Cost.Population;
	return TryApplyResourceDelta(InOutBalance, SpendDelta);
}

FString RomanEmpireEconomy::DescribeResourceCost(const FFactionResourceAmounts& Cost)
{
	TArray<FString> Parts;
	if (Cost.Gold > 0) Parts.Add(FString::Printf(TEXT("%d Gold"), Cost.Gold));
	if (Cost.Food > 0) Parts.Add(FString::Printf(TEXT("%d Food"), Cost.Food));
	if (Cost.Iron > 0) Parts.Add(FString::Printf(TEXT("%d Iron"), Cost.Iron));
	if (Cost.Wood > 0) Parts.Add(FString::Printf(TEXT("%d Wood"), Cost.Wood));
	if (Cost.Stone > 0) Parts.Add(FString::Printf(TEXT("%d Stone"), Cost.Stone));
	if (Cost.Population > 0) Parts.Add(FString::Printf(TEXT("%d Population"), Cost.Population));
	return Parts.IsEmpty() ? TEXT("Free") : FString::Join(Parts, TEXT(", "));
}

const TCHAR* RomanEmpireEconomy::GetTransactionFailureName(EResourceTransactionFailure Failure)
{
	switch (Failure)
	{
	case EResourceTransactionFailure::None: return TEXT("None");
	case EResourceTransactionFailure::InvalidFaction: return TEXT("InvalidFaction");
	case EResourceTransactionFailure::InvalidCost: return TEXT("InvalidCost");
	case EResourceTransactionFailure::InsufficientGold: return TEXT("InsufficientGold");
	case EResourceTransactionFailure::InsufficientFood: return TEXT("InsufficientFood");
	case EResourceTransactionFailure::InsufficientIron: return TEXT("InsufficientIron");
	case EResourceTransactionFailure::InsufficientWood: return TEXT("InsufficientWood");
	case EResourceTransactionFailure::InsufficientStone: return TEXT("InsufficientStone");
	case EResourceTransactionFailure::InsufficientPopulation: return TEXT("InsufficientPopulation");
	case EResourceTransactionFailure::ArithmeticOverflow: return TEXT("ArithmeticOverflow");
	case EResourceTransactionFailure::MutationInProgress: return TEXT("MutationInProgress");
	default: return TEXT("Unknown");
	}
}
