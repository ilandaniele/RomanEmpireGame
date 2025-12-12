// Copyright Roman Empire Game. All Rights Reserved.

#include "Legionary.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "GameFramework/CharacterMovementComponent.h"

ALegionary::ALegionary()
{
	// Setup Legionary stats
	UnitData.UnitType = EUnitType::Legionary;
	UnitData.Category = EUnitCategory::Infantry;
	UnitData.DisplayName = FText::FromString(TEXT("Legionary"));
	UnitData.Description = FText::FromString(TEXT("The disciplined backbone of the Roman army. Armed with gladius, pilum, and scutum."));

	// Stats
	UnitData.BaseStats.MaxHealth = 120;
	UnitData.BaseStats.MeleeAttack = 12;
	UnitData.BaseStats.RangedAttack = 8;
	UnitData.BaseStats.MeleeDefense = 10;
	UnitData.BaseStats.RangedDefense = 8;
	UnitData.BaseStats.Armor = 5;
	UnitData.BaseStats.Speed = 280.0f;
	UnitData.BaseStats.Morale = 60;
	UnitData.BaseStats.Stamina = 100.0f;
	UnitData.BaseStats.AttackSpeed = 1.0f;
	UnitData.BaseStats.BlockStrength = 1.2f;

	// Cost
	UnitData.GoldCost = 150;
	UnitData.FoodCost = 50;
	UnitData.TrainingTime = 15.0f;

	// Combat
	UnitData.AttackRange = 150.0f;
	UnitData.bCanUseRanged = true; // Can throw pilum
	UnitData.bHasShield = true;

	// Equipment
	UnitData.PrimaryWeapon = TEXT("Gladius");
	UnitData.SecondaryWeapon = TEXT("Pilum");

	// Legionary specific
	bInTestudo = false;
	MaxPila = 2;
	PilaCount = MaxPila;
	PilumProjectile = nullptr;
	PilumDamage = 25.0f;
	PilumRange = 2000.0f;
}

void ALegionary::BeginPlay()
{
	Super::BeginPlay();
	PilaCount = MaxPila;
}

void ALegionary::ActivateTestudo()
{
	if (bInTestudo)
	{
		return;
	}

	bInTestudo = true;

	// Testudo greatly increases defense but reduces speed
	UCharacterMovementComponent* Movement = GetCharacterMovementComponent();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed * 0.3f;
	}

	// Boost defense (temporary stat modification)
	UnitData.BaseStats.RangedDefense += 20;
	UnitData.BaseStats.MeleeDefense += 10;

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
	UCharacterMovementComponent* Movement = GetCharacterMovementComponent();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}

	UnitData.BaseStats.RangedDefense -= 20;
	UnitData.BaseStats.MeleeDefense -= 10;

	UE_LOG(LogRomanEmpire, Log, TEXT("%s deactivated Testudo formation"), *GetName());
}

void ALegionary::ThrowPilum()
{
	if (PilaCount <= 0)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("No pila remaining"));
		return;
	}

	if (CurrentStamina < 15.0f)
	{
		return; // Not enough stamina
	}

	PilaCount--;
	CurrentStamina -= 15.0f;

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
		FVector End = SpawnLocation + GetActorForwardVector() * PilumRange;
		
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, SpawnLocation, End, ECC_Pawn, Params))
		{
			if (AUnitBase* HitUnit = Cast<AUnitBase>(HitResult.GetActor()))
			{
				if (HitUnit->GetOwnerFaction() != OwnerFaction)
				{
					HitUnit->TakeCombatDamage(PilumDamage, this, true);
					UE_LOG(LogRomanEmpire, Log, TEXT("%s hit %s with pilum for %f damage"), 
						*GetName(), *HitUnit->GetName(), PilumDamage);
				}
			}
		}
	}

	UE_LOG(LogRomanEmpire, Log, TEXT("%s threw pilum (%d remaining)"), *GetName(), PilaCount);
}
