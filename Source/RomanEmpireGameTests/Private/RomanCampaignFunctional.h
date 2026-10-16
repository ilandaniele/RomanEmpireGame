// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

class FAutomationTestBase;
class IAutomationLatentCommand;

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
/** Non-mutating source-world phase, then the existing cross-world coordinator. */
TSharedPtr<IAutomationLatentCommand> CreateRomanFunctionalCampaignGate(FAutomationTestBase& Assertions);
TSharedPtr<IAutomationLatentCommand> CreateRomanFunctionalCampaignTravel(FAutomationTestBase& Assertions);
#endif
