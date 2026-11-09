// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/RomanSoldierAnimInstance.h"
#include "Units/UnitBase.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

void URomanSoldierAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	// NativeUpdateAnimation runs on the game thread; worker graph sees only this snapshot.
	const AUnitBase* Unit = Cast<AUnitBase>(TryGetPawnOwner());
	bDead = IsValid(Unit) && Unit->GetLifeState() == EUnitLifeState::Dead;
	bBlocking = IsValid(Unit) && Unit->CanAcceptCommands() && Unit->IsBlocking();
	bMoving = IsValid(Unit) && Unit->CanParticipateInWorld() && !Unit->IsTacticalBattleResolved()
		&& Unit->GetVelocity().SizeSquared2D() > 100.f;
	bLocalFirstPerson = IsValid(Unit) && Unit->CanAcceptCommands() && Unit->IsPlayerControlled() && Unit->IsLocallyControlled();
	FirstPersonHandTarget = FirstPersonElbowTarget = FVector::ZeroVector;
	if (bLocalFirstPerson && Unit->GetMesh())
	{
		// Presentation-only reach on the SAME body. No camera/FOV/capsule changes,
		// separate viewmodel, new clock or animation-owned hit/damage callbacks.
		float Pulse = 0.f;
		if (const UAnimMontage* Montage = GetCurrentActiveMontage())
			if (Montage->GetPlayLength() > 0.f)
				Pulse = FMath::Square(FMath::Sin(PI * FMath::Clamp(Montage_GetPosition(Montage) / Montage->GetPlayLength(), 0.f, 1.f)));
		const FTransform ActorToWorld = Unit->GetActorTransform();
		const FTransform MeshToWorld = Unit->GetMesh()->GetComponentTransform();
		FirstPersonHandTarget = MeshToWorld.InverseTransformPosition(ActorToWorld.TransformPosition(FVector(45 + 10 * Pulse, 25 - 5 * Pulse, 49 + 5 * Pulse)));
		FirstPersonElbowTarget = MeshToWorld.InverseTransformPosition(ActorToWorld.TransformPosition(FVector(15, 45, 22)));
	}
}
