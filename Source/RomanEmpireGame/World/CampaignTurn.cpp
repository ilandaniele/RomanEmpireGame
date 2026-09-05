// Copyright Roman Empire Game. All Rights Reserved.

#include "Campaign/CampaignTurn.h"

const TArray<ECampaignTurnStep>& RomanEmpireCampaign::GetCanonicalTurnPipeline()
{
	static const TArray<ECampaignTurnStep> Pipeline = {
		ECampaignTurnStep::ValidateRequest,
		ECampaignTurnStep::Production,
		ECampaignTurnStep::Upkeep,
		ECampaignTurnStep::AIActions,
		ECampaignTurnStep::ConflictResolution,
		ECampaignTurnStep::VictoryEvaluation,
		ECampaignTurnStep::IncrementTurn,
		ECampaignTurnStep::BroadcastTurnCompleted,
		ECampaignTurnStep::Autosave
	};
	return Pipeline;
}

const TCHAR* RomanEmpireCampaign::GetTurnStepName(ECampaignTurnStep Step)
{
	switch (Step)
	{
	case ECampaignTurnStep::ValidateRequest: return TEXT("ValidateRequest");
	case ECampaignTurnStep::Production: return TEXT("Production");
	case ECampaignTurnStep::Upkeep: return TEXT("Upkeep");
	case ECampaignTurnStep::AIActions: return TEXT("AIActions");
	case ECampaignTurnStep::ConflictResolution: return TEXT("ConflictResolution");
	case ECampaignTurnStep::VictoryEvaluation: return TEXT("VictoryEvaluation");
	case ECampaignTurnStep::IncrementTurn: return TEXT("IncrementTurn");
	case ECampaignTurnStep::BroadcastTurnCompleted: return TEXT("BroadcastTurnCompleted");
	case ECampaignTurnStep::Autosave: return TEXT("Autosave");
	default: return TEXT("Unknown");
	}
}

const TCHAR* RomanEmpireCampaign::GetTurnFailureName(ECampaignTurnFailure Failure)
{
	switch (Failure)
	{
	case ECampaignTurnFailure::None: return TEXT("None");
	case ECampaignTurnFailure::CampaignInactive: return TEXT("CampaignInactive");
	case ECampaignTurnFailure::MissingCampaignManager: return TEXT("MissingCampaignManager");
	case ECampaignTurnFailure::MissingFactionManager: return TEXT("MissingFactionManager");
	case ECampaignTurnFailure::MissingWorldMapManager: return TEXT("MissingWorldMapManager");
	case ECampaignTurnFailure::ResourceTransactionFailed: return TEXT("ResourceTransactionFailed");
	case ECampaignTurnFailure::AutosaveFailed: return TEXT("AutosaveFailed");
	default: return TEXT("Unknown");
	}
}
