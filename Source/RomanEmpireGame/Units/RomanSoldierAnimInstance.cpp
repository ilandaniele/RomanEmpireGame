// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/RomanSoldierAnimInstance.h"
#include "Units/UnitBase.h"

void URomanSoldierAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	// NativeUpdateAnimation runs on the game thread; worker graph sees only this snapshot.
	const AUnitBase* Unit = Cast<AUnitBase>(TryGetPawnOwner());
	bDead = IsValid(Unit) && Unit->GetLifeState() == EUnitLifeState::Dead;
	bBlocking = IsValid(Unit) && Unit->CanAcceptCommands() && Unit->IsBlocking();
	bMoving = IsValid(Unit) && Unit->CanParticipateInWorld() && !Unit->IsTacticalBattleResolved()
		&& Unit->GetVelocity().SizeSquared2D() > 100.f;
}
