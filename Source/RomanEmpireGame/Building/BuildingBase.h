// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/BuildingTypes.h"
#include "Faction/FactionData.h"
#include "BuildingBase.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Base class for all placeable buildings in the game
 * Handles construction progress, health, production, and garrisoning
 */
UCLASS(Blueprintable)
class ROMANEMPIREGAME_API ABuildingBase : public AActor
{
	GENERATED_BODY()

public:
	ABuildingBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// Building info
	UFUNCTION(BlueprintPure, Category = "Building")
	FBuildingData GetBuildingData() const { return BuildingData; }
	/** World-space half extent of the authored footprint collision box. */
	FVector GetPlacementHalfExtent() const;
	/** True only after a validated, opt-in definition has been applied. */
	bool UsesAuthoredPresentation() const { return bBuildingInitialized && BuildingData.PresentationMesh != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Building")
	EBuildingType GetBuildingType() const { return BuildingData.BuildingType; }

	UFUNCTION(BlueprintPure, Category = "Building")
	EBuildingState GetBuildingState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Building")
	FName GetBuildingDefinitionID() const { return BuildingDefinitionID; }

	/** Fully applies immutable content and ownership before a deferred spawn becomes observable. */
	bool InitializeBuilding(FName DefinitionID, EFactionID NewOwner);

	// Ownership
	UFUNCTION(BlueprintPure, Category = "Building")
	EFactionID GetOwnerFaction() const { return OwnerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Building")
	void SetOwnerFaction(EFactionID NewOwner);

	// Construction
	UFUNCTION(BlueprintCallable, Category = "Building|Construction")
	void StartConstruction();
	/** Finish a pre-placed scenario building without a simulated construction tick or player charge. */
	void CompleteInitialConstruction();

	UFUNCTION(BlueprintPure, Category = "Building|Construction")
	float GetConstructionProgress() const { return ConstructionProgress; }

	UFUNCTION(BlueprintPure, Category = "Building|Construction")
	bool IsComplete() const { return CurrentState == EBuildingState::Complete; }

	// Health
	UFUNCTION(BlueprintCallable, Category = "Building|Health")
	void ApplyBuildingDamage(float Damage, AActor* DamageSource);

	UFUNCTION(BlueprintCallable, Category = "Building|Health")
	void Repair(float Amount);

	UFUNCTION(BlueprintPure, Category = "Building|Health")
	float GetHealthPercent() const { return BuildingData.MaxHealth > 0 ? (float)CurrentHealth / (float)BuildingData.MaxHealth : 0.0f; }

	// Placement validation
	UFUNCTION(BlueprintPure, Category = "Building|Placement")
	virtual bool CanPlaceAt(const FVector& Location) const;

	// Selection
	/** Live placed actor readiness; ownership eligibility belongs to the controller. */
	UFUNCTION(BlueprintPure, Category = "Building|Selection")
	bool IsSelectionReady() const;

	UFUNCTION(BlueprintCallable, Category = "Building|Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Building|Selection")
	bool IsSelected() const { return bIsSelected; }

	// Visual feedback for placement
	UFUNCTION(BlueprintCallable, Category = "Building|Placement")
	void SetPlacementMode(bool bPlacing, bool bValidPlacement);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConstructionComplete, ABuildingBase*, Building);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildingDamaged, ABuildingBase*, Building, float, Damage);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingDestroyed, ABuildingBase*, Building);

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnConstructionComplete OnConstructionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnBuildingDamaged OnBuildingDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnBuildingDestroyed OnBuildingDestroyed;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingStateChanged, ABuildingBase*, Building);

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnBuildingStateChanged OnBuildingStateChanged;

protected:
	/** Stable ID only; authored values live in URomanBuildingDefinition assets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	FName BuildingDefinitionID = NAME_None;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building|Visuals")
	TObjectPtr<UMaterialInterface> DamagedMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Building|Visuals")
	TArray<TObjectPtr<UStaticMeshComponent>> SelectionOutline;

	UPROPERTY(Transient)
	TMap<UMaterialInterface*, TObjectPtr<UMaterialInstanceDynamic>> VisualMaterials;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void UpdateConstruction(float DeltaSeconds);
	virtual void OnConstructionFinished();
	virtual void OnDestroyed();

private:
	bool bBuildingInitialized = false;
	bool bBuildingEndingPlay = false;
	bool bPlacementVisualValid = true;
	bool bAuthoredPresentationApplied = false;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PrototypeMesh;
	void ApplyDefinitionPresentation();
	void UpdateVisuals();
};

