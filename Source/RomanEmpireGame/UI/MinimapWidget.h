// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Battle/TacticalBattleResult.h"
#include "UI/MinimapPresentation.h"
#include "MinimapWidget.generated.h"

class ARomanEmpireGameState;
class URomanUnitRegistrySubsystem;
/** Native schematic map. Detached paint values only. Shell explicitly activates
 * the visible instance. Indexed 5Hz movement queries, event-driven lifecycle.
 * No UI Tick, actor scans, render-target cameras or authoritative copies. */
UCLASS()
class ROMANEMPIREGAME_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	void SetPresentationRoot(UUserWidget* Root) { PresentationRoot = Root; }
	UFUNCTION(BlueprintCallable, Category = "Minimap") void SetPresentationActive(bool bActive);
	UFUNCTION(BlueprintCallable, Category = "Minimap") void RefreshMinimap();
	/** Invalid bounds reject atomically; deactivation resets the display fit. */
	UFUNCTION(BlueprintCallable, Category = "Minimap") void SetWorldBounds(FVector2D Min, FVector2D Max);
	UFUNCTION(BlueprintPure, Category = "Minimap") FVector2D ConvertMinimapToWorld(FVector2D NormalizedPosition) const;
	UFUNCTION(BlueprintPure, Category = "Minimap") int32 GetMarkerCount() const { return Markers.Num(); }
	UFUNCTION(BlueprintPure, Category = "Minimap") bool IsPresentationActive() const { return bPresentationActive; }
	const TArray<FMinimapMarker>& GetMarkers() const { return Markers; }
	UFUNCTION(BlueprintPure, Category = "Minimap") TArray<FMinimapMarker> GetMarkerSnapshots() const { return Markers; }
	const FMinimapProjection& GetProjection() const { return Projection; }
protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
		const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerID,
		const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	UPROPERTY(Transient) TWeakObjectPtr<ARomanEmpireGameState> State;
	UPROPERTY(Transient) TWeakObjectPtr<UUserWidget> PresentationRoot;
	UPROPERTY(Transient) TWeakObjectPtr<URomanUnitRegistrySubsystem> Registry;
	TArray<FMinimapMarker> Markers;
	TArray<TPair<FVector2D, FVector2D>> RegionLinks;
	FMinimapProjection Projection;
	FTimerHandle MovementTimer;
	FDelegateHandle RegistryChangedHandle;
	bool bPresentationActive = false;
	bool bBoundsInitialized = false;
	bool bStrategic = false;
	bool IsCurrentPresentation() const;
	void UnbindPresentation();
	UFUNCTION() void HandleTerritoryChanged(FName ID, EFactionID Previous, EFactionID Current);
	UFUNCTION() void HandleBattleChanged(FTacticalBattleResult Result);
	UFUNCTION() void HandlePlayerChanged(EFactionID Faction);
};
