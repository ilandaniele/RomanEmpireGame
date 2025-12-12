// Copyright Roman Empire Game. All Rights Reserved.

#include "UnitBase.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Configure movement
	UCharacterMovementComponent* Movement = GetCharacterMovementComponent();
	if (Movement)
	{
		Movement->MaxWalkSpeed = 300.0f;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	// Don't rotate when controller rotates (for RTS mode)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Initialize state
	OwnerFaction = EFactionID::None;
	CurrentHealth = 100;
	CurrentStamina = 100.0f;
	CurrentMorale = 50;
	bIsSelected = false;
	bIsPossessedByPlayer = false;
	bIsBlocking = false;
	bIsAttacking = false;
	CurrentStance = EUnitStance::Defensive;
	bHasMoveCommand = false;
	AttackTarget = nullptr;
	AttackCooldown = 1.0f;
	AttackCooldownRemaining = 0.0f;
}

void AUnitBase::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize from unit data
	CurrentHealth = UnitData.BaseStats.MaxHealth;
	CurrentStamina = UnitData.BaseStats.Stamina;
	CurrentMorale = UnitData.BaseStats.Morale;
	
	// Set movement speed from unit data
	UCharacterMovementComponent* Movement = GetCharacterMovementComponent();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}
	
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Unit spawned: %s"), *UnitData.DisplayName.ToString());
}

void AUnitBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsPossessedByPlayer)
	{
		// RTS mode - AI movement
		UpdateAIMovement(DeltaSeconds);
	}
	
	UpdateCombatCooldowns(DeltaSeconds);
	UpdateStamina(DeltaSeconds);
}

void AUnitBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Input is handled by PlayerController in FPS mode
}

void AUnitBase::SetOwnerFaction(EFactionID NewOwner)
{
	OwnerFaction = NewOwner;
}

void AUnitBase::SetSelected(bool bNewSelected)
{
	bIsSelected = bNewSelected;
	
	// Show/hide selection decal
	if (SelectionDecal)
	{
		SelectionDecal->SetVisibility(bNewSelected);
	}
}

void AUnitBase::CommandMoveTo(const FVector& Destination)
{
	MoveDestination = Destination;
	bHasMoveCommand = true;
	AttackTarget = nullptr;
	
	// Use AI move request
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->MoveToLocation(Destination, 50.0f); // 50cm acceptance radius
	}
}

void AUnitBase::CommandAttack(AActor* Target)
{
	AttackTarget = Target;
	if (Target)
	{
		CommandMoveTo(Target->GetActorLocation());
	}
}

void AUnitBase::CommandStop()
{
	bHasMoveCommand = false;
	AttackTarget = nullptr;
	
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->StopMovement();
	}
}

void AUnitBase::CommandHold()
{
	CommandStop();
	CurrentStance = EUnitStance::StandGround;
}

void AUnitBase::MoveInput(const FVector2D& InputVector)
{
	if (!bIsPossessedByPlayer)
	{
		return;
	}

	// Get controller rotation
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// Get forward and right vectors
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Apply movement
	AddMovementInput(ForwardDirection, InputVector.Y);
	AddMovementInput(RightDirection, InputVector.X);
}

void AUnitBase::LookInput(const FVector2D& InputVector)
{
	if (!bIsPossessedByPlayer)
	{
		return;
	}

	// Add rotation from mouse input
	AddControllerYawInput(InputVector.X);
	AddControllerPitchInput(InputVector.Y);
}

void AUnitBase::PerformAttack()
{
	if (AttackCooldownRemaining > 0.0f)
	{
		return; // Still on cooldown
	}

	if (CurrentStamina < 10.0f)
	{
		return; // Not enough stamina
	}

	bIsAttacking = true;
	AttackCooldownRemaining = AttackCooldown / UnitData.BaseStats.AttackSpeed;
	CurrentStamina -= 10.0f;

	// Perform melee attack trace
	FVector Start = GetActorLocation() + FVector(0, 0, 80);
	FVector End = Start + GetActorForwardVector() * UnitData.AttackRange;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, Params))
	{
		if (AUnitBase* HitUnit = Cast<AUnitBase>(HitResult.GetActor()))
		{
			// Check if enemy
			if (HitUnit->GetOwnerFaction() != OwnerFaction)
			{
				float Damage = UnitData.BaseStats.MeleeAttack;
				HitUnit->TakeCombatDamage(Damage, this, false);
			}
		}
	}

	// TODO: Play attack animation
	
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Unit %s attacking"), *GetName());
}

void AUnitBase::StartBlocking()
{
	if (!UnitData.bHasShield)
	{
		return;
	}

	bIsBlocking = true;
	
	// Reduce movement speed while blocking
	UCharacterMovementComponent* Movement = GetCharacterMovementComponent();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed * 0.5f;
	}
}

void AUnitBase::StopBlocking()
{
	bIsBlocking = false;
	
	// Restore movement speed
	UCharacterMovementComponent* Movement = GetCharacterMovementComponent();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}
}

void AUnitBase::PerformDodge(const FVector2D& Direction)
{
	if (CurrentStamina < 20.0f)
	{
		return; // Not enough stamina
	}

	CurrentStamina -= 20.0f;

	// Launch in direction
	FVector DodgeVector = GetActorRightVector() * Direction.X + GetActorForwardVector() * Direction.Y;
	DodgeVector.Normalize();
	DodgeVector *= 500.0f * UnitData.BaseStats.DodgeSpeed;
	DodgeVector.Z = 100.0f; // Slight upward push

	LaunchCharacter(DodgeVector, true, true);
}

void AUnitBase::TakeCombatDamage(float Damage, AActor* DamageSource, bool bIsRanged)
{
	if (!IsAlive())
	{
		return;
	}

	// Calculate damage reduction from armor and blocking
	float ActualDamage = CalculateDamageReduction(Damage, bIsRanged);

	CurrentHealth = FMath::Max(0, CurrentHealth - FMath::RoundToInt(ActualDamage));
	
	// Reduce morale when taking damage
	CurrentMorale = FMath::Max(0, CurrentMorale - FMath::RoundToInt(ActualDamage * 0.1f));

	OnUnitDamaged.Broadcast(this, ActualDamage);

	if (CurrentHealth <= 0)
	{
		OnDeath();
	}
}

float AUnitBase::CalculateDamageReduction(float RawDamage, bool bIsRanged) const
{
	float Defense = bIsRanged ? UnitData.BaseStats.RangedDefense : UnitData.BaseStats.MeleeDefense;
	float ArmorReduction = UnitData.BaseStats.Armor;

	// Blocking reduces damage significantly
	if (bIsBlocking && !bIsRanged)
	{
		Defense += 20 * UnitData.BaseStats.BlockStrength;
	}

	// Apply defense and armor
	float DamageMultiplier = 100.0f / (100.0f + Defense);
	float FinalDamage = (RawDamage * DamageMultiplier) - ArmorReduction;

	return FMath::Max(1.0f, FinalDamage); // Minimum 1 damage
}

void AUnitBase::SetPossessedByPlayer(bool bPossessed)
{
	bIsPossessedByPlayer = bPossessed;

	// Enable/disable FPS mode settings
	bUseControllerRotationYaw = bPossessed;
	bUseControllerRotationPitch = bPossessed;
	
	if (bPossessed)
	{
		// Stop AI movement when possessed
		CommandStop();
	}
}

void AUnitBase::UpdateAIMovement(float DeltaSeconds)
{
	// Check if we have an attack target and are in range
	if (AttackTarget && IsValid(AttackTarget))
	{
		float Distance = FVector::Distance(GetActorLocation(), AttackTarget->GetActorLocation());
		
		if (Distance <= UnitData.AttackRange)
		{
			// In attack range - stop and attack
			CommandStop();
			PerformAttack();
		}
	}
}

void AUnitBase::UpdateCombatCooldowns(float DeltaSeconds)
{
	if (AttackCooldownRemaining > 0.0f)
	{
		AttackCooldownRemaining -= DeltaSeconds;
		
		if (AttackCooldownRemaining <= 0.0f)
		{
			bIsAttacking = false;
		}
	}
}

void AUnitBase::UpdateStamina(float DeltaSeconds)
{
	// Regenerate stamina when not blocking or attacking
	if (!bIsBlocking && !bIsAttacking)
	{
		float RegenRate = 10.0f; // Stamina per second
		CurrentStamina = FMath::Min(UnitData.BaseStats.Stamina, CurrentStamina + RegenRate * DeltaSeconds);
	}
}

void AUnitBase::OnDeath()
{
	UE_LOG(LogRomanEmpire, Log, TEXT("Unit died: %s"), *GetName());
	
	OnUnitDied.Broadcast(this);
	
	// TODO: Play death animation, spawn ragdoll
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	
	// Destroy after delay
	SetLifeSpan(5.0f);
}
