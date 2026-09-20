// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnitLifecycleProbe.generated.h"

class AUnitBase;
class AActor;
class UActorComponent;
class FAutomationTestBase;

enum class EUnitProbeDamageAction : uint8
{
	None, MoveAttacker, HoldAttacker, KillAttacker, DestroyAttacker, KillTarget, DestroyTarget
};

UCLASS()
class UUnitLifecycleProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<AActor> ObservedUnit;
	int32 DeathEvents = 0;
	int32 VitalsEvents = 0;
	bool bTerminateOnVitals = false;
	bool bDestroyDuringVitals = false;
	bool bHandledVitalsAction = false;
	bool bReattackOnVitals = false;
	bool bHandledVitalsReattack = false;
	TWeakObjectPtr<AActor> ActionUnit;
	EUnitProbeDamageAction DamageAction = EUnitProbeDamageAction::None;
	int32 DamageEvents = 0;
	float RecoveryAtDamage = 0.0f;
	bool bKillOnCameraActivation = false;
	int32 CameraActivationEvents = 0;
	UFUNCTION()
	void HandleDeath(AUnitBase* Unit);
	UFUNCTION()
	void HandleVitals(AUnitBase* Unit);
	UFUNCTION()
	void HandleDamage(AUnitBase* Unit, float Damage);
	UFUNCTION()
	void HandleCameraActivated(UActorComponent* Component, bool bReset);
};
