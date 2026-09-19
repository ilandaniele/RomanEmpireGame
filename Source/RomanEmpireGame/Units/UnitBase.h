// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Units/UnitTypes.h"
#include "Units/UnitVitalState.h"
#include "Faction/FactionData.h"
#include "UnitBase.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Unit info
	UFUNCTION(BlueprintPure, Category = "Unit")
	FUnitData GetUnitData() const { return UnitData; }

	UFUNCTION(BlueprintPure, Category = "Unit")
	EUnitType GetUnitType() const { return UnitData.UnitType; }

	UFUNCTION(BlueprintPure, Category = "Unit")
	FName GetUnitDefinitionID() const { return UnitDefinitionID; }

	/** Fully applies immutable content and ownership before a deferred spawn becomes observable. */
	UFUNCTION(BlueprintCallable, Category = "Unit")
	bool InitializeUnit(FName DefinitionID, EFactionID NewOwner);

	UFUNCTION(BlueprintPure, Category = "Unit|State")
	bool IsInitialized() const { return IsAlive() || GetLifeState() == EUnitLifeState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Unit|State")
	EUnitLifeState GetLifeState() const { return Vitals.GetLifeState(); }

	UFUNCTION(BlueprintPure, Category = "Unit|State")
	bool CanAcceptCommands() const;

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

	UFUNCTION(BlueprintPure, Category = "Unit|Commands")
	bool HasMoveCommand() const { return bHasMoveCommand; }

	UFUNCTION(BlueprintPure, Category = "Unit|Commands")
	AActor* GetAttackTarget() const { return AttackTarget.Get(); }

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

	UFUNCTION(BlueprintPure, Category = "Unit|FPS")
	bool IsBlocking() const { return bIsBlocking; }

	UFUNCTION(BlueprintCallable, Category = "Unit|FPS")
	void PerformDodge(const FVector2D& Direction);

	// Health and damage
	UFUNCTION(BlueprintCallable, Category = "Unit|Health")
	void TakeCombatDamage(float Damage, AActor* DamageSource, bool bIsRanged);

	UFUNCTION(BlueprintPure, Category = "Unit|Health")
	float GetHealthPercent() const { return Vitals.GetHealthPercent(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Health")
	int32 GetCurrentHealth() const { return Vitals.GetHealth(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Health")
	int32 GetMaxHealth() const { return Vitals.GetMaxHealth(); }

	UFUNCTION(BlueprintPure, Category = "Unit|State")
	int32 GetCurrentMorale() const { return Vitals.GetMorale(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Health")
	bool IsAlive() const { return Vitals.IsAlive(); }

	// Accessors used by HUD
	UFUNCTION(BlueprintPure, Category = "Unit")
	FString GetUnitDisplayName() const { return UnitData.DisplayName.ToString(); }

	UFUNCTION(BlueprintCallable, Category = "Unit|Health")
	void SetCurrentHealth(int32 NewHealth);

	// Apply faction-based color to visual mesh
	void ApplyFactionColor();

	// Stamina (FPS mode)
	UFUNCTION(BlueprintPure, Category = "Unit|FPS")
	float GetStaminaPercent() const
	{
		return Vitals.GetStaminaPercent();
	}

	UFUNCTION(BlueprintPure, Category = "Unit|FPS")
	float GetCurrentStamina() const { return Vitals.GetStamina(); }

	// State
	UFUNCTION(BlueprintPure, Category = "Unit")
	bool IsPossessedByPlayer() const { return bIsPossessedByPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetPossessedByPlayer(bool bPossessed);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitDamaged, AUnitBase*, Unit, float, Damage);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, AUnitBase*, Unit);

	UPROPERTY(BlueprintAssignable, Category = "Unit|Events")
	FOnUnitDamaged OnUnitDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Unit|Events")
	FOnUnitDied OnUnitDied;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitVitalsChanged, AUnitBase*, Unit);

	UPROPERTY(BlueprintAssignable, Category = "Unit|Events")
	FOnUnitVitalsChanged OnUnitVitalsChanged;

protected:
	/** Stable ID only; authored values live in URomanUnitDefinition assets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit")
	FName UnitDefinitionID = NAME_None;

	// Unit data
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
	FUnitData UnitData;

	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	EFactionID OwnerFaction;

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
	TWeakObjectPtr<AActor> AttackTarget;

	// Visual mesh (placeholder geometry)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	// Selection indicator
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> SelectionDecal;

	// Selection ring (visible green disc under selected units)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SelectionRingMesh;

	// Head mesh (sphere on top of body)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	// Weapon mesh (sword = thin cube)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// Shield mesh (flat cylinder)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	// FPS Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> FPSCameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FPSCamera;

	// Limbs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightFootMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftFootMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Unit|Lifecycle", meta=(ClampMin="0.01"))
	float CorpseLifetimeSeconds = 3.0f;

	// Combat
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Combat")
	float AttackCooldown;

	// Walk animation
	float WalkAnimTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	float AttackCooldownRemaining;

	// Internal
	virtual void UpdateAIMovement(float DeltaSeconds);
	virtual void UpdateCombatCooldowns(float DeltaSeconds);
	virtual void UpdateStamina(float DeltaSeconds);
	virtual void OnDeath();
	virtual void StopSpecialActions() {}
	/** Spend only; caller commits its action before publishing OnUnitVitalsChanged. */
	bool TrySpendUnitStamina(float Amount);
	float CalculateDamageReduction(float RawDamage, bool bIsRanged) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State", meta=(AllowPrivateAccess="true"))
	FUnitVitalState Vitals;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SelectionRingMaterial;

	// Event delivery guard, not a second life/health state.
	bool bDeathNotified = false;
	void StopAllActions();
};

