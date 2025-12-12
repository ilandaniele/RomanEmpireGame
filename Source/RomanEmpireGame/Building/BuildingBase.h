// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RomanEmpireGame/Building/BuildingTypes.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "BuildingBase.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Base class for all placeable buildings in the game
 * Handles construction progress, health, production, and garrisoning
 */
UCLASS(Abstract, Blueprintable)
class ROMANEMPIREGAME_API ABuildingBase : public AActor
{
	GENERATED_BODY()

public:
	ABuildingBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Building info
	UFUNCTION(BlueprintPure, Category = "Building")
	FBuildingData GetBuildingData() const { return BuildingData; }

	UFUNCTION(BlueprintPure, Category = "Building")
	EBuildingType GetBuildingType() const { return BuildingData.BuildingType; }

	UFUNCTION(BlueprintPure, Category = "Building")
	EBuildingState GetBuildingState() const { return CurrentState; }

	// Ownership
	UFUNCTION(BlueprintPure, Category = "Building")
	EFactionID GetOwnerFaction() const { return OwnerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Building")
	void SetOwnerFaction(EFactionID NewOwner);

	// Construction
	UFUNCTION(BlueprintCallable, Category = "Building|Construction")
	void StartConstruction();

	UFUNCTION(BlueprintPure, Category = "Building|Construction")
	float GetConstructionProgress() const { return ConstructionProgress; }

	UFUNCTION(BlueprintPure, Category = "Building|Construction")
	bool IsComplete() const { return CurrentState == EBuildingState::Complete; }

	// Health
	UFUNCTION(BlueprintCallable, Category = "Building|Health")
	void TakeDamage(float Damage, AActor* DamageSource);

	UFUNCTION(BlueprintCallable, Category = "Building|Health")
	void Repair(float Amount);

	UFUNCTION(BlueprintPure, Category = "Building|Health")
	float GetHealthPercent() const { return (float)CurrentHealth / (float)BuildingData.MaxHealth; }

	// Placement validation
	UFUNCTION(BlueprintPure, Category = "Building|Placement")
	virtual bool CanPlaceAt(const FVector& Location) const;

	// Selection
	UFUNCTION(BlueprintCallable, Category = "Building|Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Building|Selection")
	bool IsSelected() const { return bIsSelected; }

	// Visual feedback for placement
	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void SetPlacementMode(bool bPlacing, bool bValidPlacement);

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BuildingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ConstructionScaffolding;

	// Building data
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	FBuildingData BuildingData;

	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|State")
	EBuildingState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|State")
	EFactionID OwnerFaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|State")
	float ConstructionProgress; // 0.0 to 1.0

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|State")
	int32 CurrentHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|State")
	bool bIsSelected;

	// Materials for visual feedback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Visuals")
	UMaterialInterface* ValidPlacementMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Visuals")
	UMaterialInterface* InvalidPlacementMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Visuals")
	UMaterialInterface* ConstructionMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Visuals")
	UMaterialInterface* CompleteMaterial;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConstructionComplete, ABuildingBase*, Building);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildingDamaged, ABuildingBase*, Building, float, Damage);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingDestroyed, ABuildingBase*, Building);

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnConstructionComplete OnConstructionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnBuildingDamaged OnBuildingDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnBuildingDestroyed OnBuildingDestroyed;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void UpdateConstruction(float DeltaSeconds);
	virtual void OnConstructionFinished();
	virtual void OnDestroyed();

private:
	void UpdateVisuals();
};
