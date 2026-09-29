// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Units/UnitTypes.h"
#include "Units/UnitVitalState.h"
#include "Units/UnitOrderState.h"
#include "Units/UnitCombatState.h"
#include "Units/UnitSelectionPresentation.h"
#include "Units/UnitActivity.h"
#include "Units/UnitNavigationResult.h"
#include "Faction/FactionData.h"
#include "UnitBase.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UDecalComponent;
class ARomanUnitAIController;

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
	/** Restore the parked controller after direct control, or create the initial controller. */
	void RestoreTacticalController();
	UFUNCTION(BlueprintPure, Category = "Unit|Navigation")
	EUnitNavigationResult GetNavigationResult() const { return NavigationResult; }

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

	UFUNCTION(BlueprintPure, Category = "Unit|State")
	EUnitActivity GetActivity() const;

	// Ownership
	UFUNCTION(BlueprintPure, Category = "Unit")
	EFactionID GetOwnerFaction() const { return OwnerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetOwnerFaction(EFactionID NewOwner);

	// Selection (RTS mode)
	UFUNCTION(BlueprintCallable, Category = "Unit|Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Unit|Selection")
	bool IsSelected() const { return SelectionPresentation.IsSelected(); }

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
	bool HasMoveCommand() const { return Orders.HasMoveCommand(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Commands")
	AActor* GetAttackTarget() const { return Orders.GetTarget(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Commands")
	EUnitOrderType GetOrderType() const { return Orders.GetOrderType(); }

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
	bool IsBlocking() const { return Combat.IsBlocking(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Combat")
	bool IsAttacking() const { return Combat.IsAttackInProgress(); }

	UFUNCTION(BlueprintPure, Category = "Unit|Combat")
	float GetAttackRecoveryRemaining() const { return Combat.GetRecoveryRemaining(); }

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

	/** Committed ownership invalidation; observers must re-query current ownership. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitOwnerFactionChanged, AUnitBase*, Unit);
	UPROPERTY(BlueprintAssignable, Category = "Unit|Events")
	FOnUnitOwnerFactionChanged OnOwnerFactionChanged;

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
	bool bIsPossessedByPlayer;

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

	// Walk animation
	float WalkAnimTime;

	// Internal
	virtual void UpdateStamina(float DeltaSeconds);
	virtual void OnDeath();
	virtual void StopSpecialActions() {}
	/** Spend only; caller commits its action before publishing OnUnitVitalsChanged. */
	bool TrySpendUnitStamina(float Amount);
	float CalculateDamageReduction(float RawDamage, bool bIsRanged) const;

private:
	friend class ARomanUnitAIController;
	enum class EPendingMeleeKind : uint8 { None, Ordered, Direct };
	void CancelPendingMelee();
	void ResolveMeleeImpact();
	bool TryApplyMeleeHit(AUnitBase* Target);
	void StopNavigation();
	void CompleteNavigation(uint64 Revision, bool bSuccess);
	UPROPERTY(Transient)
	TObjectPtr<ARomanUnitAIController> TacticalController;
	UPROPERTY(VisibleAnywhere, Category = "Unit|Navigation")
	EUnitNavigationResult NavigationResult = EUnitNavigationResult::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State", meta=(AllowPrivateAccess="true"))
	FUnitVitalState Vitals;

	UPROPERTY(VisibleAnywhere, Category = "Unit|State")
	FUnitOrderState Orders;

	UPROPERTY(VisibleAnywhere, Category = "Unit|State")
	FUnitCombatState Combat;
	TWeakObjectPtr<AUnitBase> PendingMeleeTarget;
	uint64 PendingMeleeOrderRevision = 0;
	EPendingMeleeKind PendingMeleeKind = EPendingMeleeKind::None;

	UPROPERTY(Transient)
	FUnitSelectionPresentation SelectionPresentation;

	// Derive from the authored attack rate once when starting an attack.
	float GetAttackRecoveryDuration() const;

	// Event delivery guard, not a second life/health state.
	bool bDeathNotified = false;
	void StopAllActions();
};

