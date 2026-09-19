// Copyright Roman Empire Game. All Rights Reserved.

#include "Legionary.h"
#include "RomanEmpireGame.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

ALegionary::ALegionary()
{
	UnitDefinitionID = TEXT("Legionary");

	// Legionary specific
	bInTestudo = false;
	PilaCount = 0;
	PilumProjectile = nullptr;
}

void ALegionary::BeginPlay()
{
	Super::BeginPlay();
	if (!IsAlive()) return;
	PilaCount = UnitData.StartingRangedAmmo;
}

void ALegionary::ActivateTestudo()
{
	if (!CanAcceptCommands() || bInTestudo)
	{
		return;
	}

	bInTestudo = true;

	// Testudo greatly increases defense but reduces speed
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed * UnitData.DefensiveFormationSpeedMultiplier;
	}

	// Boost defense (temporary stat modification)
	UnitData.BaseStats.RangedDefense += UnitData.DefensiveFormationRangedDefenseBonus;
	UnitData.BaseStats.MeleeDefense += UnitData.DefensiveFormationMeleeDefenseBonus;

	UE_LOG(LogRomanEmpire, Log, TEXT("%s activated Testudo formation"), *GetName());
}

void ALegionary::DeactivateTestudo()
{
	if (!bInTestudo)
	{
		return;
	}

	bInTestudo = false;

	// Restore normal stats
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}

	UnitData.BaseStats.RangedDefense -= UnitData.DefensiveFormationRangedDefenseBonus;
	UnitData.BaseStats.MeleeDefense -= UnitData.DefensiveFormationMeleeDefenseBonus;

	UE_LOG(LogRomanEmpire, Log, TEXT("%s deactivated Testudo formation"), *GetName());
}

void ALegionary::ThrowPilum()
{
	if (!CanAcceptCommands()) return;
	if (PilaCount <= 0)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("No pila remaining"));
		return;
	}

	if (!TrySpendUnitStamina(15.0f))
	{
		return; // Not enough stamina
	}

	PilaCount--;
	OnUnitVitalsChanged.Broadcast(this);
	if (!CanAcceptCommands()) return;

	// Projectile spawn
	FVector SpawnLocation = GetActorLocation() + FVector(0, 0, 150); // Throw from shoulder height
	FRotator SpawnRotation = GetActorRotation();

	if (PilumProjectile)
	{
		// TODO: Spawn projectile actor
		// GetWorld()->SpawnActor<AActor>(PilumProjectile, SpawnLocation, SpawnRotation);
	}
	else
	{
		// Fallback: Instant hit trace
		FVector End = SpawnLocation + GetActorForwardVector() * UnitData.SpecialRangedRange;
		
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, SpawnLocation, End, ECC_Pawn, Params))
		{
			if (AUnitBase* HitUnit = Cast<AUnitBase>(HitResult.GetActor()))
			{
				if (HitUnit->GetOwnerFaction() != OwnerFaction)
				{
					HitUnit->TakeCombatDamage(UnitData.SpecialRangedDamage, this, true);
					UE_LOG(LogRomanEmpire, Log, TEXT("%s hit %s with pilum for %f damage"), 
						*GetName(), *HitUnit->GetName(), UnitData.SpecialRangedDamage);
				}
			}
		}
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("%s threw pilum (%d remaining)"), *GetName(), PilaCount);
}

void ALegionary::StopSpecialActions()
{
	DeactivateTestudo();
}

