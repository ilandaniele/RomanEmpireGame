// Copyright Roman Empire Game. All Rights Reserved.
#include "World/CampaignManager.h"
#include "Content/RomanContentRegistry.h"
#include "Faction/FactionManager.h"
#include "Units/UnitBase.h"
#include "World/WorldMapManager.h"

FCampaignAIProposal ACampaignManager::GetCampaignAIProposal() const
{
	if (ValidateStrategicArmyAction(true) != ECampaignArmyFailure::None ||
		!FactionManager->IsFactionActive(EFactionID::Carthage) || FactionManager->GetPlayerFaction() != EFactionID::Rome) return {};
	const auto* Registry = URomanContentRegistrySubsystem::Get(this);
	const auto* Faction = Registry && Registry->IsReady() ? Registry->FindFaction(EFactionID::Carthage) : nullptr;
	if (!Faction) return {};
	FCampaignAIPlanningInput Input;
	Input.Faction = EFactionID::Carthage; Input.Enemy = EFactionID::Rome;
	Input.bAtWar = FactionManager->AreAtWar(Input.Faction, Input.Enemy);
	Input.Wallet = FactionManager->GetFactionResources(Input.Faction);
	Input.Armies = ArmyState.GetArmies();
	for (FName ID : WorldMapManager->GetTerritoryIDs())
	{
		const auto* Definition = Registry->FindTerritory(ID);
		FTerritorySettlementState Settlement;
		if (!Definition || !WorldMapManager->TryGetTerritorySettlement(ID, Settlement)) return {};
		FCampaignAIRegion Region;
		Region.TerritoryID = ID; Region.Owner = WorldMapManager->GetTerritoryOwner(ID);
		Region.bHasSettlement = Settlement.bHasSettlement;
		Region.bHasTacticalMap = Definition->HasValidTacticalMap();
		Region.Neighbors = WorldMapManager->GetAdjacentTerritoryIDs(ID);
		Input.Regions.Add(MoveTemp(Region));
	}
	for (const auto* Unit : Registry->GetAllUnits())
		if (Unit && Unit->UnitClass.Get() && Unit->UnitClass.Get()->IsChildOf(AUnitBase::StaticClass()) && Unit->AvailableToFactions.Contains(Faction->Definition.FactionID))
		{
			FCampaignAIRecruitOption Option; Option.UnitID = Unit->Definition.UnitID;
			Option.PerUnitCost = Unit->Definition.TrainingCost; Input.Recruitment.Add(Option);
		}
	return RomanEmpireCampaignAI::Plan(Input);
}
