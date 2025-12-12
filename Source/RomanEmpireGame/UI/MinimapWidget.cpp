// Copyright Roman Empire Game. All Rights Reserved.

#include "MinimapWidget.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "RomanEmpireGame/World/WorldMapManager.h"
#include "RomanEmpireGame/World/TerritoryRegion.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateInterval = 0.5f; // Update twice per second
	MinimapSize = FVector2D(200.0f, 200.0f);
	WorldMin = FVector2D(-50000.0f, -50000.0f);
	WorldMax = FVector2D(50000.0f, 50000.0f);
	TimeSinceLastUpdate = 0.0f;

	// Find world map manager
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldMapManager::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		WorldMapManager = Cast<AWorldMapManager>(FoundActors[0]);
	}

	RefreshMinimap();
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceLastUpdate += InDeltaTime;
	if (TimeSinceLastUpdate >= UpdateInterval)
	{
		TimeSinceLastUpdate = 0.0f;
		UpdateUnitMarkers();
	}
}

void UMinimapWidget::RefreshMinimap()
{
	UpdateTerritoryColors();
	UpdateUnitMarkers();
}

void UMinimapWidget::SetWorldBounds(FVector2D Min, FVector2D Max)
{
	WorldMin = Min;
	WorldMax = Max;
	RefreshMinimap();
}

void UMinimapWidget::AddUnitMarker(AActor* Unit, FLinearColor Color)
{
	if (!Unit || !MarkerContainer)
	{
		return;
	}

	// Create a simple image widget as marker
	UImage* Marker = NewObject<UImage>(this);
	if (Marker)
	{
		// Set color
		Marker->SetColorAndOpacity(Color);
		
		// Add to canvas
		UCanvasPanelSlot* Slot = MarkerContainer->AddChildToCanvas(Marker);
		if (Slot)
		{
			Slot->SetSize(FVector2D(8.0f, 8.0f)); // 8x8 pixel marker
			
			// Position marker
			FVector2D MinimapPos = WorldToMinimap(Unit->GetActorLocation());
			Slot->SetPosition(MinimapPos - FVector2D(4.0f, 4.0f)); // Center the marker
		}

		UnitMarkers.Add(Unit, Marker);
	}
}

void UMinimapWidget::RemoveUnitMarker(AActor* Unit)
{
	if (UWidget** Marker = UnitMarkers.Find(Unit))
	{
		(*Marker)->RemoveFromParent();
		UnitMarkers.Remove(Unit);
	}
}

void UMinimapWidget::ClearAllMarkers()
{
	for (auto& Pair : UnitMarkers)
	{
		if (Pair.Value)
		{
			Pair.Value->RemoveFromParent();
		}
	}
	UnitMarkers.Empty();
}

void UMinimapWidget::UpdateCameraView(FVector2D CameraMin, FVector2D CameraMax)
{
	if (!CameraViewBox)
	{
		return;
	}

	// Convert world positions to minimap coordinates
	FVector2D MinimapMin = WorldToMinimap(FVector(CameraMin.X, CameraMin.Y, 0));
	FVector2D MinimapMax = WorldToMinimap(FVector(CameraMax.X, CameraMax.Y, 0));

	// Update camera view box position and size
	// This would be done via canvas slot manipulation
}

FVector2D UMinimapWidget::ConvertMinimapToWorld(FVector2D MinimapPosition) const
{
	FVector2D WorldSize = WorldMax - WorldMin;
	
	float WorldX = WorldMin.X + (MinimapPosition.X / MinimapSize.X) * WorldSize.X;
	float WorldY = WorldMin.Y + (MinimapPosition.Y / MinimapSize.Y) * WorldSize.Y;
	
	return FVector2D(WorldX, WorldY);
}

FVector2D UMinimapWidget::WorldToMinimap(FVector WorldPosition) const
{
	FVector2D WorldSize = WorldMax - WorldMin;
	
	float MinimapX = ((WorldPosition.X - WorldMin.X) / WorldSize.X) * MinimapSize.X;
	float MinimapY = ((WorldPosition.Y - WorldMin.Y) / WorldSize.Y) * MinimapSize.Y;
	
	// Clamp to minimap bounds
	MinimapX = FMath::Clamp(MinimapX, 0.0f, MinimapSize.X);
	MinimapY = FMath::Clamp(MinimapY, 0.0f, MinimapSize.Y);
	
	return FVector2D(MinimapX, MinimapY);
}

void UMinimapWidget::UpdateTerritoryColors()
{
	// In a full implementation, this would render territory colors to a render target
	// The minimap image would use this render target as its brush
	
	if (!WorldMapManager)
	{
		return;
	}

	// For each territory, we would draw a colored region
	for (ATerritoryRegion* Territory : WorldMapManager->GetAllTerritories())
	{
		if (!Territory)
		{
			continue;
		}

		// Get territory color based on owner
		// Draw to render target
	}
}

void UMinimapWidget::UpdateUnitMarkers()
{
	// Update positions of existing markers
	for (auto& Pair : UnitMarkers)
	{
		if (!IsValid(Pair.Key) || !Pair.Value)
		{
			continue;
		}

		FVector2D MinimapPos = WorldToMinimap(Pair.Key->GetActorLocation());
		
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Pair.Value->Slot))
		{
			Slot->SetPosition(MinimapPos - FVector2D(4.0f, 4.0f));
		}
	}
}
