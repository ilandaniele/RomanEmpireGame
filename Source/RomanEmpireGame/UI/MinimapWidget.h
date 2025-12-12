// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

class UImage;
class UCanvasPanel;
class UBorder;
class AWorldMapManager;
class ATerritoryRegion;

/**
 * Minimap widget showing territory ownership and unit positions
 */
UCLASS()
class ROMANEMPIREGAME_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Update minimap display
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RefreshMinimap();

	// Set the world bounds for the minimap
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetWorldBounds(FVector2D Min, FVector2D Max);

	// Add/remove markers
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void AddUnitMarker(AActor* Unit, FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RemoveUnitMarker(AActor* Unit);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ClearAllMarkers();

	// Camera view rectangle
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void UpdateCameraView(FVector2D CameraMin, FVector2D CameraMax);

	// Click handling
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	FVector2D ConvertMinimapToWorld(FVector2D MinimapPosition) const;

protected:
	// Main minimap image (render target)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UImage* MinimapImage;

	// Container for unit markers
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UCanvasPanel* MarkerContainer;

	// Camera view box
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	UBorder* CameraViewBox;

	// Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minimap")
	float UpdateInterval;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minimap")
	FVector2D MinimapSize;

	// World bounds
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	FVector2D WorldMin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	FVector2D WorldMax;

	// Marker tracking
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TMap<AActor*, UWidget*> UnitMarkers;

	// Reference
	UPROPERTY()
	AWorldMapManager* WorldMapManager;

private:
	float TimeSinceLastUpdate;

	FVector2D WorldToMinimap(FVector WorldPosition) const;
	void UpdateTerritoryColors();
	void UpdateUnitMarkers();
};
