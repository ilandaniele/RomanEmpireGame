// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "Animation/AnimInstance.h"
#include "RomanSoldierAnimInstance.generated.h"

/** Presentation-only snapshot of the canonical owning soldier. No damage notifies. */
UCLASS(Blueprintable, Transient)
class ROMANEMPIREGAME_API URomanSoldierAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	UPROPERTY(BlueprintReadOnly, Category = "Soldier")
	bool bMoving = false;
	UPROPERTY(BlueprintReadOnly, Category = "Soldier")
	bool bBlocking = false;
	UPROPERTY(BlueprintReadOnly, Category = "Soldier")
	bool bDead = false;
};
