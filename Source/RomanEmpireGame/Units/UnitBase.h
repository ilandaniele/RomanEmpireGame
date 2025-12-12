// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RomanEmpireGame/Units/UnitTypes.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "UnitBase.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;

/**
 * Base class for all military units in the game
 * Works as both RTS-selectable unit and FPS-possessible character
 */
UCLASS(Abstract, Blueprintable)
class ROMANEMPIREGAME_API AUnitBase : public ACharacter
{
	GENERATED_BODY()

public:
	AUnitBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Unit info
	UFUNCTION(BlueprintPure, Category = "Unit")
	FUnitData GetUnitData() const { return UnitData; }

	UFUNCTION(BlueprintPure, Category = "Unit")
	EUnitType GetUnitType() const { return UnitData.UnitType; }

	// Ownership
	UFUNCTION(BlueprintPure, Category = "Unit")
	EFactionID GetOwnerFaction() const { return OwnerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetOwnerFaction(EFactionID NewOwner);

	// Selection (RTS mode)
	UFUNCTION(BlueprintCallable, Category = "Unit|Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Unit|Selection")
	bool IsSelected() const { return bIsSelected; }

	// Commands (RTS mode)
	UFUNCTION(BlueprintCallable, Category = "Unit|Commands")
	void CommandMoveTo(const FVector& Destination);

	UFUNCTION(BlueprintCallable, Category = "Unit|Commands")
	void CommandAttack(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Unit|Commands")
	void CommandStop();

	UFUNCTION(BlueprintCallable, Category = "Unit|Commands")
	void CommandHold();

	// FPS Mode input
	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void MoveInput(const FVector2D& InputVector);

	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void LookInput(const FVector2D& InputVector);

	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void PerformAttack();

	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void StartBlocking();

	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void StopBlocking();

	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void PerformDodge(const FVector2D& Direction);

	// Health and damage
	UFUNCTION(BlueprintCallable, Category = "Unit|Health")
	void TakeCombatDamage(float Damage, AActor* DamageSource, bool bIsRanged);

	UFUNCTION(BlueprintPure, Category = "Unit|Health")
	float GetHealthPercent() const { return (float)CurrentHealth / (float)UnitData.BaseStats.MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Unit|Health")
	bool IsAlive() const { return CurrentHealth > 0; }

	// Stamina (FPS mode)
	UFUNCTION(BlueprintPure, Category = "Unit|FPS")
	float GetStaminaPercent() const { return CurrentStamina / UnitData.BaseStats.Stamina; }

	// State
	UFUNCTION(BlueprintPure, Category = "Unit")
	bool IsPossessedByPlayer() const { return bIsPossessedByPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetPossessedByPlayer(bool bPossessed);

protected:
	// Unit data
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit")
	FUnitData UnitData;

	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	EFactionID OwnerFaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	int32 CurrentHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	float CurrentStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	int32 CurrentMorale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsSelected;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsPossessedByPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsBlocking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsAttacking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	EUnitStance CurrentStance;

	// AI movement target
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|AI")
	FVector MoveDestination;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|AI")
	bool bHasMoveCommand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|AI")
	AActor* AttackTarget;

	// Selection indicator
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDecalComponent* SelectionDecal;

	// Combat
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Combat")
	float AttackCooldown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	float AttackCooldownRemaining;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitDamaged, AUnitBase*, Unit, float, Damage);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, AUnitBase*, Unit);

	UPROPERTY(BlueprintAssignable, Category = "Unit|Events")
	FOnUnitDamaged OnUnitDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Unit|Events")
	FOnUnitDied OnUnitDied;

	// Internal
	virtual void UpdateAIMovement(float DeltaSeconds);
	virtual void UpdateCombatCooldowns(float DeltaSeconds);
	virtual void UpdateStamina(float DeltaSeconds);
	virtual void OnDeath();
	float CalculateDamageReduction(float RawDamage, bool bIsRanged) const;
};
