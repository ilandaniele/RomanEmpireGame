// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignAIPlanning.h"
#include "Content/RomanContentDefinitions.h"

namespace
{
	bool GuidLess(const FGuid& A, const FGuid& B)
	{
		if (A.A != B.A) return A.A < B.A;
		if (A.B != B.B) return A.B < B.B;
		if (A.C != B.C) return A.C < B.C;
		return A.D < B.D;
	}
	bool Valid(const FCampaignAIPlanningInput& Input)
	{
		if (!RomanEmpireContent::IsRuntimeFaction(Input.Faction) || !RomanEmpireContent::IsRuntimeFaction(Input.Enemy) || Input.Faction == Input.Enemy || !Input.Wallet.IsNonNegative()) return false;
		TMap<FName, const FCampaignAIRegion*> Regions;
		for (const auto& Region : Input.Regions)
		{
			if (!RomanEmpireContent::IsStableID(Region.TerritoryID) || Regions.Contains(Region.TerritoryID) ||
				(Region.Owner != EFactionID::None && !RomanEmpireContent::IsRuntimeFaction(Region.Owner))) return false;
			Regions.Add(Region.TerritoryID, &Region);
		}
		for (const auto& Region : Input.Regions)
		{
			TSet<FName> Seen;
			for (FName Neighbor : Region.Neighbors)
			{
				const auto* Other = Regions.Find(Neighbor);
				if (Neighbor == Region.TerritoryID || Seen.Contains(Neighbor) || !Other || !(*Other)->Neighbors.Contains(Region.TerritoryID)) return false;
				Seen.Add(Neighbor);
			}
		}
		FCampaignArmyState Checked;
		if (!Checked.ReplaceFromSnapshot(Input.Armies).WasSuccessful()) return false;
		for (const auto& Army : Input.Armies)
		{
			const auto* Region = Regions.Find(Army.TerritoryID);
			if (!Region || (*Region)->Owner != Army.Faction) return false;
		}
		TSet<FName> UnitIDs;
		for (const auto& Option : Input.Recruitment)
		{
			if (!RomanEmpireContent::IsStableID(Option.UnitID) || UnitIDs.Contains(Option.UnitID) || !Option.PerUnitCost.IsNonNegative()) return false;
			UnitIDs.Add(Option.UnitID);
		}
		return true;
	}
	int64 Strength(const TArray<FCampaignArmy>& Armies, EFactionID Faction, FName Region)
	{
		int64 Total = 0;
		for (const auto& Army : Armies) if (Army.Faction == Faction && Army.TerritoryID == Region) Total += FCampaignArmyState::GetUnitCount(Army);
		return Total;
	}
	int64 Threat(const FCampaignAIPlanningInput& Input, const FCampaignAIRegion& Region)
	{
		int64 Total = 0;
		if (Input.bAtWar) for (FName Neighbor : Region.Neighbors) Total += Strength(Input.Armies, Input.Enemy, Neighbor);
		return Total;
	}
	bool Scale(const FFactionResourceAmounts& Cost, int32 Count, FFactionResourceAmounts& Out)
	{
		const auto Field = [Count](int32 Value, int32& Dest)
		{
			const int64 Total = int64(Value) * Count;
			if (Total > MAX_int32) return false;
			Dest = int32(Total); return true;
		};
		return Field(Cost.Gold, Out.Gold) && Field(Cost.Food, Out.Food) && Field(Cost.Iron, Out.Iron) &&
			Field(Cost.Wood, Out.Wood) && Field(Cost.Stone, Out.Stone) && Field(Cost.Population, Out.Population);
	}
	FCampaignAIProposal Recruit(const FCampaignAIPlanningInput& Input, const FCampaignAIRegion& Region, int64 Desired)
	{
		FCampaignAIProposal Proposal;
		if (!Region.bHasSettlement) return Proposal;
		const int64 Missing = Desired - Strength(Input.Armies, Input.Faction, Region.TerritoryID);
		if (Missing <= 0) return Proposal;
		TArray<FCampaignAIRecruitOption> Options = Input.Recruitment;
		Options.Sort([](const auto& A, const auto& B) { return A.UnitID.LexicalLess(B.UnitID); });
		for (const auto& Option : Options)
			for (int32 Count = int32(FMath::Min<int64>(Missing, RomanEmpireCampaignAI::MaxRecruitPerTurn)); Count > 0; --Count)
			{
				FFactionResourceAmounts Cost;
				FFactionResourceAmounts Wallet = Input.Wallet;
				if (!Scale(Option.PerUnitCost, Count, Cost) || !RomanEmpireEconomy::TrySpendResources(Wallet, Cost).WasSuccessful()) continue;
				bool bHasExisting = false;
				for (const auto& Army : Input.Armies) if (Army.Faction == Input.Faction && Army.TerritoryID == Region.TerritoryID)
				{
					bHasExisting = true;
					FCampaignArmyState Candidate;
					if (Candidate.RegisterArmy(Army).WasSuccessful() && Candidate.AddUnits(Army.ArmyID, Option.UnitID, Count).WasSuccessful())
					{ Proposal.ArmyID = Army.ArmyID; break; }
				}
				// Overflow in existing garrisons must not manufacture an extra army to evade it.
				if (bHasExisting && !Proposal.ArmyID.IsValid()) continue;
				Proposal.Action = ECampaignAIAction::Recruit; Proposal.TerritoryID = Region.TerritoryID;
				Proposal.UnitID = Option.UnitID; Proposal.Count = Count; Proposal.Cost = Cost; return Proposal;
			}
		return Proposal;
	}
}

FCampaignAIProposal RomanEmpireCampaignAI::Plan(const FCampaignAIPlanningInput& Input)
{
	if (!Valid(Input)) return {};
	FCampaignAIPlanningInput Ordered = Input;
	Ordered.Regions.Sort([](const auto& A, const auto& B) { return A.TerritoryID.LexicalLess(B.TerritoryID); });
	Ordered.Armies.Sort([](const auto& A, const auto& B) { return GuidLess(A.ArmyID, B.ArmyID); });
	for (const auto& Region : Ordered.Regions)
	{
		if (Region.Owner != Ordered.Faction || !Region.bHasSettlement) continue;
		const int64 Garrison = Strength(Ordered.Armies, Ordered.Faction, Region.TerritoryID);
		const int64 Danger = Threat(Ordered, Region);
		if (Garrison >= Danger) continue;
		for (const auto& Army : Ordered.Armies)
		{
			if (Army.Faction != Ordered.Faction || !Region.Neighbors.Contains(Army.TerritoryID) || Garrison + FCampaignArmyState::GetUnitCount(Army) < Danger) continue;
			const auto* Source = Ordered.Regions.FindByPredicate([&](const auto& Row) { return Row.TerritoryID == Army.TerritoryID; });
			if (!Source || Source->Owner != Ordered.Faction) continue;
			const int64 Remaining = Strength(Ordered.Armies, Ordered.Faction, Source->TerritoryID) - FCampaignArmyState::GetUnitCount(Army);
			if ((Source->bHasSettlement && Remaining < MinimumSourceGarrison) || Remaining < Threat(Ordered, *Source)) continue;
			FCampaignAIProposal Proposal; Proposal.Action = ECampaignAIAction::Defend; Proposal.ArmyID = Army.ArmyID; Proposal.TerritoryID = Region.TerritoryID; return Proposal;
		}
		const auto Proposal = Recruit(Ordered, Region, FMath::Max<int64>(Danger, TargetSettlementGarrison));
		// An unresolved threatened settlement takes precedence over a speculative attack.
		return Proposal;
	}
	if (Ordered.bAtWar) for (const auto& Army : Ordered.Armies)
	{
		if (Army.Faction != Ordered.Faction) continue;
		const auto* Source = Ordered.Regions.FindByPredicate([&](const auto& Row) { return Row.TerritoryID == Army.TerritoryID; });
		if (!Source || Source->Owner != Ordered.Faction) continue;
		for (const auto& Target : Ordered.Regions)
		{
			const int64 Defenders = Strength(Ordered.Armies, Ordered.Enemy, Target.TerritoryID);
			if (Target.Owner != Ordered.Enemy || !Target.bHasTacticalMap || !Source->Neighbors.Contains(Target.TerritoryID) || Defenders <= 0 || FCampaignArmyState::GetUnitCount(Army) < Defenders) continue;
			FCampaignAIProposal Proposal; Proposal.Action = ECampaignAIAction::Attack; Proposal.ArmyID = Army.ArmyID; Proposal.TerritoryID = Target.TerritoryID; return Proposal;
		}
	}
	for (const auto& Region : Ordered.Regions) if (Region.Owner == Ordered.Faction)
	{
		const auto Proposal = Recruit(Ordered, Region, TargetSettlementGarrison);
		if (Proposal.Action != ECampaignAIAction::None) return Proposal;
	}
	return {};
}
