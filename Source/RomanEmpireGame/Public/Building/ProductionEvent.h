// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProductionEvent.generated.h"

UENUM(BlueprintType)
enum class EProductionEventType : uint8
{
	Started UMETA(DisplayName = "Started"),
	Queued UMETA(DisplayName = "Queued"),
	Completed UMETA(DisplayName = "Completed"),
	Cancelled UMETA(DisplayName = "Cancelled"),
	Failed UMETA(DisplayName = "Failed")
};

/** Notification emitted after a queue transition; pending count describes the committed queue. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FProductionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Production")
	EProductionEventType Type = EProductionEventType::Started;

	UPROPERTY(BlueprintReadOnly, Category = "Production")
	FName UnitID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Production")
	int32 PendingOrders = 0;
};
