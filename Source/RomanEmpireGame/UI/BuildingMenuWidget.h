// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Building/BuildingTypes.h"
#include "Components/Button.h"
#include "BuildingMenuWidget.generated.h"

class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingSelected, FName, BuildingID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRomanBuildingButtonClicked, FName, BuildingID);

/** Mouse-operated HUD control that leaves keyboard focus with gameplay. */
UCLASS()
class ROMANEMPIREGAME_API URomanHUDButton : public UButton
{
	GENERATED_BODY()

public:
	URomanHUDButton(const FObjectInitializer& ObjectInitializer);
};

/** UMG button that carries only a stable content ID. */
UCLASS()
class ROMANEMPIREGAME_API URomanBuildingButton : public URomanHUDButton
{
	GENERATED_BODY()

public:
	void InitializeBuildingButton(FName InBuildingID, EBuildingCategory InCategory);

	FName GetBuildingID() const { return BuildingID; }
	EBuildingCategory GetBuildingCategory() const { return BuildingCategory; }

	UPROPERTY(BlueprintAssignable, Category = "Building Menu")
	FOnRomanBuildingButtonClicked OnBuildingButtonClicked;

private:
	UPROPERTY(Transient)
	FName BuildingID = NAME_None;

	UPROPERTY(Transient)
	EBuildingCategory BuildingCategory = EBuildingCategory::Military;

	UFUNCTION()
	void HandleClicked();
};

/** Data-driven native UMG building menu. */
UCLASS()
class ROMANEMPIREGAME_API UBuildingMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Building Menu")
	void PopulateBuildingList();
	/** Query effective profile keys without rebuilding buttons or storing another key authority. */
	void RefreshShortcutLabels();

	UFUNCTION(BlueprintCallable, Category = "Building Menu")
	void FilterByCategory(EBuildingCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Building Menu")
	void ShowAllBuildings();

	UPROPERTY(BlueprintAssignable, Category = "Building Menu")
	FOnBuildingSelected OnBuildingSelected;

private:
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuildingListBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URomanBuildingButton>> BuildingButtons;

	UPROPERTY(Transient)
	EBuildingCategory CurrentCategory = EBuildingCategory::Military;

	UPROPERTY(Transient)
	bool bShowAllCategories = true;

	void BuildNativeWidgetTree();
	void CreateBuildingButton(FName BuildingID, const FBuildingData& Data, int32 QuickSlot);
	void ApplyFilter();

	UFUNCTION()
	void HandleBuildingButtonClicked(FName BuildingID);
};
