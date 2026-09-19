// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/UnitVitalState.h"

bool FUnitVitalState::Initialize(int32 InMaxHealth, float InMaxStamina, int32 InitialMorale)
{
	if (LifeState != EUnitLifeState::Uninitialized || InMaxHealth <= 0 ||
		!FMath::IsFinite(InMaxStamina) || InMaxStamina <= 0.0f || InitialMorale < 0)
	{
		return false;
	}
	Health = MaxHealth = InMaxHealth;
	Stamina = MaxStamina = InMaxStamina;
	Morale = InitialMorale;
	LifeState = EUnitLifeState::Alive;
	return true;
}

bool FUnitVitalState::SetHealth(int32 NewHealth)
{
	if (!IsAlive() || NewHealth < 0) return false;
	Health = FMath::Min(NewHealth, MaxHealth);
	if (Health == 0) LifeState = EUnitLifeState::Dead;
	return true;
}

bool FUnitVitalState::ApplyDamage(int32 HealthDamage, int32 MoraleDamage)
{
	if (!IsAlive() || HealthDamage < 0 || MoraleDamage < 0) return false;
	Health -= FMath::Min(HealthDamage, Health);
	Morale -= FMath::Min(MoraleDamage, Morale);
	if (Health == 0) LifeState = EUnitLifeState::Dead;
	return true;
}

bool FUnitVitalState::TrySpendStamina(float Amount)
{
	if (!IsAlive() || !FMath::IsFinite(Amount) || Amount < 0.0f || Amount > Stamina) return false;
	Stamina -= Amount;
	return true;
}

bool FUnitVitalState::RegenerateStamina(float Amount)
{
	if (!IsAlive() || !FMath::IsFinite(Amount) || Amount < 0.0f) return false;
	// Compute and clamp in double: float subtraction followed by addition can round
	// twice and overflow even when an increment was clamped to the apparent margin.
	Stamina = static_cast<float>(FMath::Min(double(MaxStamina), double(Stamina) + double(Amount)));
	return true;
}

void FUnitVitalState::MarkInitializationFailed()
{
	if (LifeState == EUnitLifeState::Uninitialized) LifeState = EUnitLifeState::InitializationFailed;
}

void FUnitVitalState::EndLife()
{
	// Preserve the last vitals for diagnostics, but disallow all further mutations.
	LifeState = EUnitLifeState::Ended;
}
