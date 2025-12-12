// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RomanEmpireGame/Building/BuildingTypes.h"
#include "BuildingMenuWidget.generated.h"

class UVerticalBox;
class UButton;
class UImage;
class UTextBlock;
class ABuildingBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingSelected, EBuildingType, BuildingType);

/**
 * Building selection menu (Age of Empires style)
 * Shows available buildings organized by category
 */
UCLASS()
class ROMANEMPIREGAME_API UBuildingMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Populate menu with available buildings
	UFUNCTION(BlueprintCallable, Category = "Building Menu")
	void PopulateBuildingList();

	// Filter by category
	UFUNCTION(BlueprintCallable, Category = "Building Menu")
	void FilterByCategory(EBuildingCategory Category);

	// Show all buildings
	UFUNCTION(BlueprintCallable, Category = "Building Menu")
	void ShowAllBuildings();

	// Event when building is selected
	UPROPERTY(BlueprintAssignable, Category = "Building Menu")
	FOnBuildingSelected OnBuildingSelected;

protected:
	// UI containers
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UVerticalBox* MilitaryBuildingsBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UVerticalBox* EconomicBuildingsBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UVerticalBox* CivilBuildingsBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UVerticalBox* DefenseBuildingsBox;

	// Category buttons
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UButton* MilitaryTabButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UButton* EconomicTabButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UButton* CivilTabButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UButton* DefenseTabButton;

	// Building button template
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Menu")
	TSubclassOf<UUserWidget> BuildingButtonTemplate;

	// Building class references
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Menu")
	TMap<EBuildingType, TSubclassOf<ABuildingBase>> BuildingClasses;

	// Current filter
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building Menu")
	EBuildingCategory CurrentCategory;

	UFUNCTION()
	void OnMilitaryTabClicked();

	UFUNCTION()
	void OnEconomicTabClicked();

	UFUNCTION()
	void OnCivilTabClicked();

	UFUNCTION()
	void OnDefenseTabClicked();

private:
	void CreateBuildingButton(EBuildingType BuildingType, const FBuildingData& Data, UVerticalBox* Container);
	void SetCategoryVisibility(EBuildingCategory Category);
};
