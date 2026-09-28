// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnitLifecycleProbe.generated.h"

class AUnitBase;
class AActor;
class UActorComponent;
class ABuildingBase;
class FAutomationTestBase;

enum class EUnitProbeDamageAction : uint8
{
	None, MoveAttacker, HoldAttacker, KillAttacker, DestroyAttacker, KillTarget, DestroyTarget
};

enum class EUnitControllerProbeAction : uint8 { ChangeOwner, Kill, Destroy, Exit, SelectReplacement };

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
	TWeakObjectPtr<UObject> ObservedController;
	TWeakObjectPtr<AActor> ReplacementUnit;
	EUnitControllerProbeAction ControllerAction = EUnitControllerProbeAction::ChangeOwner;
	bool bHandledControllerAction = false;
	UFUNCTION()
	void HandleControllerCameraActivated(UActorComponent* Component, bool bReset);
	UFUNCTION()
	void HandleControllerCameraDeactivated(UActorComponent* Component);
	UFUNCTION()
	void HandleControllerSelection(const TArray<AUnitBase*>& Units, ABuildingBase* Building);
	int32 RegistryExpectedLiveCount = 0;
	int32 RegistryOwnerEvents = 0;
	int32 RegistryDeathEvents = 0;
	bool bRegistryReentered = false;
	EUnitControllerProbeAction RegistryAction = EUnitControllerProbeAction::ChangeOwner;
	UFUNCTION()
	void HandleRegistryOwner(AUnitBase* Unit);
	UFUNCTION()
	void HandleRegistryDeath(AUnitBase* Unit);
	FGuid SquadObserved;
	int32 SquadOwnerEvents = 0;
	int32 SquadDeathEvents = 0;
	UFUNCTION()
	void HandleSquadOwner(AUnitBase* Unit);
	UFUNCTION()
	void HandleSquadDeath(AUnitBase* Unit);
};
