// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignArmyState.h"
#include "UObject/Object.h"
#include "CampaignArmyProbe.generated.h"
class ARomanEmpireGameState;
UCLASS()
class UCampaignArmyProbe : public UObject
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<ARomanEmpireGameState> State;
	int32 Notifications = 0;
	TArray<FCampaignArmy> LastObserved;
	UFUNCTION()
	void OnArmiesChanged();
};
