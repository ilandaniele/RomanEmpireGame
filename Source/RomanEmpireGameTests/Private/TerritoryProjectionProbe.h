// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "TerritoryProjectionProbe.generated.h"

class ARomanEmpireGameState;
class FAutomationTestBase;

/** Editor-only reentrant observer; never a runtime gameplay authority. */
UCLASS()
class UTerritoryProjectionProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<UObject> WorldMap;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	int32 SourceNotifications = 0;
	TArray<EFactionID> PublicOwners;
	bool bNestOnce = false;
	bool bNestPublicOnce = false;
	int32 SettlementNotifications = 0;
	bool bNestSettlementOnce = false;

	UFUNCTION()
	void OnSourceOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner);
	UFUNCTION()
	void OnPublicOwnerChanged(FName TerritoryID, EFactionID PreviousOwner, EFactionID NewOwner);
	UFUNCTION()
	void OnPublicSettlementChanged(FName TerritoryID);
};
