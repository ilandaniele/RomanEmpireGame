// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Campaign/CampaignAIPlanning.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FCampaignArmy Army(uint32 ID, EFactionID Owner, FName Region, int32 Count)
	{
		FCampaignArmy Row; Row.ArmyID = FGuid(ID, 0, 0, 0); Row.Faction = Owner; Row.TerritoryID = Region;
		FCampaignArmyUnitStack Stack; Stack.UnitID = TEXT("Legionary"); Stack.Count = Count; Row.Units.Add(Stack); return Row;
	}
	FCampaignAIPlanningInput Fixture()
	{
		FCampaignAIPlanningInput Input; Input.Faction = EFactionID::Carthage; Input.Enemy = EFactionID::Rome; Input.bAtWar = true;
		Input.Wallet.Gold = Input.Wallet.Food = Input.Wallet.Iron = Input.Wallet.Wood = Input.Wallet.Stone = Input.Wallet.Population = 100;
		Input.Regions = {
			{ TEXT("Africa"), EFactionID::Carthage, true, true, { TEXT("Rome"), TEXT("Sicily") } },
			{ TEXT("Rome"), EFactionID::Rome, true, true, { TEXT("Africa") } },
			{ TEXT("Sicily"), EFactionID::Carthage, false, true, { TEXT("Africa") } }
		};
		Input.Armies = { Army(1, EFactionID::Carthage, TEXT("Africa"), 10), Army(2, EFactionID::Rome, TEXT("Rome"), 15) };
		FCampaignAIRecruitOption Option; Option.UnitID = TEXT("Legionary");
		Option.PerUnitCost.Gold = 1; Option.PerUnitCost.Food = 2; Option.PerUnitCost.Iron = 3;
		Option.PerUnitCost.Wood = 4; Option.PerUnitCost.Stone = 5; Option.PerUnitCost.Population = 6;
		Input.Recruitment.Add(Option); return Input;
	}
	bool Same(const FCampaignAIProposal& A, const FCampaignAIProposal& B)
	{
		return FCampaignAIProposal::StaticStruct()->CompareScriptStruct(&A, &B, 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAIPlannerPolicyTest, "RomanEmpireGame.Campaign.AIPlanning.DefendRecruitAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignAIPlannerPolicyTest::RunTest(const FString& Parameters)
{
	auto Input = Fixture();
	auto Proposal = RomanEmpireCampaignAI::Plan(Input);
	TestEqual(TEXT("Threatened existing garrison recruits before attacking"), Proposal.Action, ECampaignAIAction::Recruit);
	TestEqual(TEXT("Recruits into actual stable garrison ID"), Proposal.ArmyID, Input.Armies[0].ArmyID);
	TestEqual(TEXT("Recruit bounded to two per turn"), Proposal.Count, 2);
	Input.Armies.Add(Army(3, EFactionID::Carthage, TEXT("Sicily"), 5));
	Proposal = RomanEmpireCampaignAI::Plan(Input);
	TestEqual(TEXT("Available adjacent relief takes precedence over recruitment"), Proposal.Action, ECampaignAIAction::Defend);
	TestEqual(TEXT("Move uses relief army identity"), Proposal.ArmyID, Input.Armies[2].ArmyID);
	TestEqual(TEXT("Relief goes to owned threatened settlement"), Proposal.TerritoryID, FName(TEXT("Africa")));
	Input.Regions[2].bHasSettlement = true;
	TestEqual(TEXT("Cannot empty another settlement's sole garrison"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::Recruit);
	Input.Armies.Add(Army(4, EFactionID::Carthage, TEXT("Sicily"), 5));
	TestEqual(TEXT("Enough retained source garrison permits relief"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::Defend);
	Input = Fixture(); Input.Armies[0].Units[0].Count = 16;
	Proposal = RomanEmpireCampaignAI::Plan(Input);
	TestEqual(TEXT("Sufficient force attacks active adjacent enemy"), Proposal.Action, ECampaignAIAction::Attack);
	TestEqual(TEXT("Attack keeps source ID, not a new identity"), Proposal.ArmyID, Input.Armies[0].ArmyID);
	TestEqual(TEXT("Attack targets hostile region"), Proposal.TerritoryID, FName(TEXT("Rome")));
	TestTrue(TEXT("Attack does not reserve or debit resources"), Proposal.Cost.IsZero());
	Input.bAtWar = false;
	TestEqual(TEXT("No attack at peace when garrison ready"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input.Armies[0].Units[0].Count = 10;
	TestEqual(TEXT("At peace rebuilds below-target garrison"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::Recruit);
	Input.Armies.RemoveAt(0);
	Proposal = RomanEmpireCampaignAI::Plan(Input);
	TestEqual(TEXT("Destroyed garrison can propose replacement recruitment"), Proposal.Action, ECampaignAIAction::Recruit);
	TestFalse(TEXT("Read-only proposal never allocates a new army GUID"), Proposal.ArmyID.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAIPlannerGuardTest, "RomanEmpireGame.Campaign.AIPlanning.DeterministicGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignAIPlannerGuardTest::RunTest(const FString& Parameters)
{
	auto Input = Fixture(); Input.Armies.Add(Army(3, EFactionID::Carthage, TEXT("Sicily"), 6));
	const auto Expected = RomanEmpireCampaignAI::Plan(Input);
	Algo::Reverse(Input.Armies); Algo::Reverse(Input.Regions); for (auto& Region : Input.Regions) Algo::Reverse(Region.Neighbors);
	TestTrue(TEXT("Input order cannot change chosen stable IDs"), Same(Expected, RomanEmpireCampaignAI::Plan(Input)));
	TestTrue(TEXT("Repeated query has no random/side effects"), Same(Expected, RomanEmpireCampaignAI::Plan(Input)));
	Input = Fixture(); Input.Armies[0].Units[0].Count = 16; Input.Regions[1].bHasTacticalMap = false;
	TestEqual(TEXT("No tactical-map metadata cannot propose attack"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input.Regions[1].bHasTacticalMap = true; Input.Regions[0].Neighbors.Remove(TEXT("Rome")); Input.Regions[1].Neighbors.Reset();
	TestEqual(TEXT("No adjacency cannot propose attack"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Regions[1].Neighbors.Reset();
	TestEqual(TEXT("Asymmetric graph fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Regions[0].Neighbors.Add(TEXT("Rome"));
	TestEqual(TEXT("Duplicate neighbor fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); const auto DuplicateArmy = Input.Armies[0]; Input.Armies.Add(DuplicateArmy);
	TestEqual(TEXT("Duplicate army identity fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Armies[0].TerritoryID = TEXT("Unknown");
	TestEqual(TEXT("Unknown army region fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Faction = Input.Enemy;
	TestEqual(TEXT("Same enemy/self fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Faction = static_cast<EFactionID>(255);
	TestEqual(TEXT("Invalid runtime faction fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Armies[0].TerritoryID = TEXT("Rome");
	TestEqual(TEXT("Army outside canonical owned territory fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Wallet.Population = -1;
	TestEqual(TEXT("Malformed canonical wallet fails closed"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Armies[0].Units[0].Count = 16; Input.Armies.RemoveAt(1);
	TestEqual(TEXT("No unsupported empty-force encounter"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAIPlannerCostsTest, "RomanEmpireGame.Campaign.AIPlanning.RecruitmentCosts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FCampaignAIPlannerCostsTest::RunTest(const FString& Parameters)
{
	auto Input = Fixture(); const auto Before = Input.Wallet;
	auto Proposal = RomanEmpireCampaignAI::Plan(Input);
	TestEqual(TEXT("Gold cost"), Proposal.Cost.Gold, 2); TestEqual(TEXT("Food cost"), Proposal.Cost.Food, 4);
	TestEqual(TEXT("Iron cost"), Proposal.Cost.Iron, 6); TestEqual(TEXT("Wood cost"), Proposal.Cost.Wood, 8);
	TestEqual(TEXT("Stone cost"), Proposal.Cost.Stone, 10); TestEqual(TEXT("Population cost"), Proposal.Cost.Population, 12);
	TestTrue(TEXT("Planning leaves supplied wallet unchanged"), FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Input.Wallet, &Before, 0));
	TestEqual(TEXT("Planning leaves supplied army count unchanged"), FCampaignArmyState::GetUnitCount(Input.Armies[0]), 10);
	Input.Wallet.Population = 6;
	TestEqual(TEXT("Affordability degrades to one, no overdraw"), RomanEmpireCampaignAI::Plan(Input).Count, 1);
	Input.Wallet.Population = 5;
	TestEqual(TEXT("Cannot pay all six fields means no recruit"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Recruitment[0].PerUnitCost = {}; Input.Wallet = {};
	TestEqual(TEXT("Free authored recruitment is still bounded"), RomanEmpireCampaignAI::Plan(Input).Count, 2);
	Input = Fixture(); Input.Recruitment[0].PerUnitCost.Gold = MAX_int32; Input.Wallet.Gold = MAX_int32;
	TestEqual(TEXT("Overflowing two costs degrades to representable one"), RomanEmpireCampaignAI::Plan(Input).Count, 1);
	Input.Recruitment[0].PerUnitCost.Food = -1;
	TestEqual(TEXT("Invalid authored cost rejects, never mints credit"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Regions[0].bHasSettlement = false;
	TestEqual(TEXT("Cannot recruit without owned settlement"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Recruitment.Reset();
	TestEqual(TEXT("No eligible factory/content produces no purchase"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	Input = Fixture(); Input.Armies[0].Units[0].Count = MAX_int32; Input.Armies[1].Units[0].Count = MAX_int32;
	Input.Armies.Add(Army(3, EFactionID::Rome, TEXT("Rome"), MAX_int32));
	TestEqual(TEXT("Wide threat sums cannot overflow into an attack or overfull recruit"), RomanEmpireCampaignAI::Plan(Input).Action, ECampaignAIAction::None);
	return true;
}
#endif
