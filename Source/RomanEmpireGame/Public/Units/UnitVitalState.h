// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitVitalState.generated.h"

UENUM(BlueprintType)
enum class EUnitLifeState : uint8
{
	Uninitialized,
	Alive,
	Dead,
	InitializationFailed,
	Ended
};

/** Single owner of live vitals. No world, delegates or actor side effects.
 * Mutators return acceptance (a valid no-op is accepted), not whether a value changed.
 * Only Initialize can enter Alive; terminal states cannot be revived or reinitialized.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FUnitVitalState
{
	GENERATED_BODY()

	bool Initialize(int32 InMaxHealth, float InMaxStamina, int32 InitialMorale);
	EUnitLifeState GetLifeState() const { return LifeState; }
	bool IsAlive() const { return LifeState == EUnitLifeState::Alive; }
	int32 GetHealth() const { return Health; }
	int32 GetMaxHealth() const { return MaxHealth; }
	int32 GetMorale() const { return Morale; }
	float GetStamina() const { return Stamina; }
	float GetMaxStamina() const { return MaxStamina; }
	float GetHealthPercent() const { return MaxHealth > 0 ? float(Health) / float(MaxHealth) : 0.0f; }
	float GetStaminaPercent() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }
	bool SetHealth(int32 NewHealth);
	bool ApplyDamage(int32 HealthDamage, int32 MoraleDamage);
	bool TrySpendStamina(float Amount);
	bool RegenerateStamina(float Amount);
	void MarkInitializationFailed();
	void EndLife();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	EUnitLifeState LifeState = EUnitLifeState::Uninitialized;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	int32 Health = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	int32 MaxHealth = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	int32 Morale = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	float Stamina = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	float MaxStamina = 0.0f;
};
