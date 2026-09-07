// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/GameplayMode.h"
#include "Engine/DataAsset.h"
#include "RomanInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/** Authored input identities and contexts. These small assets remain loaded with the controller. */
UCLASS(BlueprintType)
class ROMANEMPIREGAME_API URomanInputConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	static constexpr int32 BuildingSlotCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> StrategicContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> TacticalContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> PossessionContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> SharedContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Camera")
	TObjectPtr<UInputAction> CameraPan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Camera")
	TObjectPtr<UInputAction> Zoom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> Select;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> Command;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> EnterPossession;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Possession")
	TObjectPtr<UInputAction> SoldierMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Possession")
	TObjectPtr<UInputAction> Look;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Possession")
	TObjectPtr<UInputAction> Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Possession")
	TObjectPtr<UInputAction> Block;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Possession")
	TObjectPtr<UInputAction> ExitPossession;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> BuildMenu;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Campaign")
	TObjectPtr<UInputAction> EndTurn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Shared")
	TObjectPtr<UInputAction> Cancel;

	/** Six distinct Boolean actions, ordered by the 1-based authored building quick slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tactical")
	TArray<TObjectPtr<UInputAction>> BuildingSlots;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Returns only the mode context; the controller also activates SharedContext. */
	UFUNCTION(BlueprintPure, Category = "Input")
	UInputMappingContext* GetContextForMode(EGameplayMode Mode) const;

	/** Appends configuration errors without changing assets or assuming physical key defaults. */
	UFUNCTION(BlueprintCallable, Category = "Input|Validation")
	bool ValidateConfiguration(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
