// Copyright Roman Empire Game. All Rights Reserved.
#include "MinimapWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Core/RomanEmpireGameState.h"
#include "Content/RomanContentRegistry.h"
#include "Content/RomanContentDefinitions.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Battle/TacticalObjective.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Units/RomanUnitRegistry.h"
#include "Units/UnitBase.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UI/RomanPlayerSettingsSubsystem.h"
#include "TimerManager.h"

void UMinimapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetBrushColor(FLinearColor(0.018f, 0.035f, 0.05f, 0.96f));
		WidgetTree->RootWidget = Background;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UMinimapWidget::NativeDestruct()
{
	SetPresentationActive(false);
	Super::NativeDestruct();
}
bool UMinimapWidget::IsCurrentPresentation() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = GetOwningPlayer();
	return bPresentationActive && IsVisible() && PresentationRoot.IsValid() &&
		PresentationRoot->IsInViewport() && PresentationRoot->IsVisible() &&
		PresentationRoot->GetOwningPlayer() == Player && World && !World->bIsTearingDown &&
		World->GetGameInstance() && World->GetGameInstance()->GetWorld() == World &&
		IsValid(Player) && Player->GetWorld() == World && Player->HasActorBegunPlay() &&
		!Player->IsActorBeingDestroyed() && State.IsValid() && State->GetWorld() == World &&
		State->IsGameplayInitialized();
}
void UMinimapWidget::UnbindPresentation()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(MovementTimer);
	if (Registry.IsValid()) Registry->OnUnitRegistryChanged.Remove(RegistryChangedHandle);
	RegistryChangedHandle.Reset();
	if (State.IsValid())
	{
		State->OnTerritoryOwnerChanged.RemoveDynamic(this, &UMinimapWidget::HandleTerritoryChanged);
		State->OnTacticalBattleChanged.RemoveDynamic(this, &UMinimapWidget::HandleBattleChanged);
		State->OnPlayerFactionChanged.RemoveDynamic(this, &UMinimapWidget::HandlePlayerChanged);
	}
	State.Reset(); Registry.Reset();
	Markers.Reset(); RegionLinks.Reset(); bBoundsInitialized = false;
}
void UMinimapWidget::SetPresentationActive(bool bActive)
{
	if (!bActive)
	{
		bPresentationActive = false; UnbindPresentation();
		SetVisibility(ESlateVisibility::Collapsed); return;
	}
	if (bPresentationActive && IsCurrentPresentation()) { RefreshMinimap(); return; }
	UnbindPresentation();
	State = GetWorld() ? GetWorld()->GetGameState<ARomanEmpireGameState>() : nullptr;
	Registry = GetWorld() ? GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	bPresentationActive = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (!IsCurrentPresentation()) { SetPresentationActive(false); return; }
	State->OnTerritoryOwnerChanged.AddUniqueDynamic(this, &UMinimapWidget::HandleTerritoryChanged);
	State->OnTacticalBattleChanged.AddUniqueDynamic(this, &UMinimapWidget::HandleBattleChanged);
	State->OnPlayerFactionChanged.AddUniqueDynamic(this, &UMinimapWidget::HandlePlayerChanged);
	if (Registry.IsValid()) RegistryChangedHandle = Registry->OnUnitRegistryChanged.AddUObject(this, &UMinimapWidget::RefreshMinimap);
	RefreshMinimap();
	if (!bStrategic) GetWorld()->GetTimerManager().SetTimer(MovementTimer, this, &UMinimapWidget::RefreshMinimap, 0.2f, true);
}
void UMinimapWidget::RefreshMinimap()
{
	if (!IsCurrentPresentation()) { SetPresentationActive(false); return; }
	const bool bNewStrategic = State->GetCurrentGameplayMode() == EGameplayMode::Strategic;
	if (bStrategic != bNewStrategic) bBoundsInitialized = false;
	bStrategic = bNewStrategic;
	Markers.Reset(); RegionLinks.Reset();
	const EFactionID PlayerFaction = State->GetPlayerFaction();
	const URomanContentRegistrySubsystem* Content = URomanContentRegistrySubsystem::Get(this);
	auto FactionColor = [Content](EFactionID Faction)
	{
		const URomanFactionDefinition* Definition = Content ? Content->FindFaction(Faction) : nullptr;
		return Definition ? Definition->Definition.FactionColor : FLinearColor::White;
	};
	if (bStrategic)
	{
		const auto Rows = State->GetTerritorySnapshots();
		for (const auto& Row : Rows)
		{
			Markers.Add({EMinimapMarkerKind::Territory, Row.TerritoryID, FVector2D(Row.MapLocation), Row.OwnerColor, Row.OwnerFaction == PlayerFaction, false});
			for (FName NeighborID : Row.AdjacentTerritoryIDs)
			{
				if (!Row.TerritoryID.LexicalLess(NeighborID)) continue;
				const auto* Neighbor = Rows.FindByPredicate([NeighborID](const auto& Candidate) { return Candidate.TerritoryID == NeighborID; });
				if (Neighbor) RegionLinks.Emplace(FVector2D(Row.MapLocation), FVector2D(Neighbor->MapLocation));
			}
		}
	}
	else
	{
		if (Registry.IsValid()) for (AUnitBase* Unit : Registry->GetRegisteredUnits())
		{
			const EFactionID Faction = Unit->GetOwnerFaction();
			Markers.Add({EMinimapMarkerKind::Unit, Unit->GetFName(), FVector2D(Unit->GetActorLocation()),
				FactionColor(Faction), Faction == PlayerFaction, Unit->IsSelected()});
		}
		const auto* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
		const ATacticalObjective* Objective = Battle ? Battle->GetObjectiveActor() : nullptr;
		if (IsValid(Objective) && Objective->GetWorld() == GetWorld() && !Objective->IsActorBeingDestroyed())
		{
			const auto Result = Battle->GetResult();
			Markers.Add({EMinimapMarkerKind::Objective, Objective->GetObjectiveID(), FVector2D(Objective->GetActorLocation()),
				Result.CapturedObjectiveID == Objective->GetObjectiveID() ? FactionColor(Result.Winner) : FLinearColor(1.f, 0.85f, 0.2f), false, false});
		}
	}
	if (!bBoundsInitialized && !Markers.IsEmpty())
	{
		FVector2D Min = Markers[0].Position, Max = Min;
		for (const auto& Marker : Markers)
		{
			Min.X = FMath::Min(Min.X, Marker.Position.X); Min.Y = FMath::Min(Min.Y, Marker.Position.Y);
			Max.X = FMath::Max(Max.X, Marker.Position.X); Max.Y = FMath::Max(Max.Y, Marker.Position.Y);
		}
		// Stable square fit; moving units cannot resize the map every sample.
		const FVector2D Center = (Min + Max) * 0.5;
		const double Extent = FMath::Max(FMath::Max(Max.X - Min.X, Max.Y - Min.Y) * 0.55, 500.0);
		Projection = {Center - FVector2D(Extent), Center + FVector2D(Extent)};
		bBoundsInitialized = Projection.IsValid();
	}
	if (!bStrategic && GetOwningPlayer()->GetPawn())
		Markers.Add({EMinimapMarkerKind::Camera, NAME_None, FVector2D(GetOwningPlayer()->GetPawn()->GetActorLocation()), FLinearColor::White, false, false});
	Invalidate(EInvalidateWidgetReason::Paint);
}
void UMinimapWidget::SetWorldBounds(FVector2D Min, FVector2D Max)
{
	FMinimapProjection Candidate{Min, Max};
	if (!Candidate.IsValid()) return;
	Projection = Candidate; bBoundsInitialized = true;
	RefreshMinimap();
}
FVector2D UMinimapWidget::ConvertMinimapToWorld(FVector2D Position) const { return Projection.Unproject(Position); }
void UMinimapWidget::HandleTerritoryChanged(FName, EFactionID, EFactionID) { RefreshMinimap(); }
void UMinimapWidget::HandleBattleChanged(FTacticalBattleResult) { RefreshMinimap(); }
void UMinimapWidget::HandlePlayerChanged(EFactionID) { RefreshMinimap(); }
int32 UMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerID,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Base = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerID, Style, bParentEnabled);
	if (!IsCurrentPresentation()) return Base;
	const FVector2D Size = Geometry.GetLocalSize();
	const URomanPlayerSettingsSubsystem* Preferences=GetGameInstance()?GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>():nullptr;
	const float TextScale=Preferences?Preferences->GetPreferences().TextScale:1.f;
	const auto Font=[TextScale](float BaseSize) { FSlateFontInfo Result=FCoreStyle::GetDefaultFontStyle("Regular",10); Result.Size=BaseSize*TextScale; return Result; };
	const double Side = FMath::Max(0.0, FMath::Min(Size.X - 24, Size.Y - 48*TextScale));
	const FVector2D Offset((Size.X - Side) * 0.5, 24*TextScale), MapSize(Side, Side);
	if (Side < 8) return Base;
	const auto Screen = [&](FVector2D World) { return Offset + Projection.Project(World) * MapSize; };
	const auto Lines = [&](TArray<FVector2D> Points, FLinearColor Color, float Thickness = 1.f)
	{
		FSlateDrawElement::MakeLines(Elements, Base + 1, Geometry.ToPaintGeometry(), Points,
			ESlateDrawEffect::None, Color * Style.GetColorAndOpacityTint(), true, Thickness);
	};
	Lines({Offset, Offset + FVector2D(Side, 0), Offset + MapSize, Offset + FVector2D(0, Side), Offset}, FLinearColor(0.45f, 0.55f, 0.6f));
	for (const auto& Link : RegionLinks) Lines({Screen(Link.Key), Screen(Link.Value)}, FLinearColor(0.25f, 0.35f, 0.4f));
	for (const auto& Marker : Markers)
	{
		const FVector2D P = Screen(Marker.Position);
		const double R = Marker.Kind == EMinimapMarkerKind::Territory ? 6 : 4;
		if (Marker.Kind == EMinimapMarkerKind::Objective)
			Lines({P + FVector2D(0,-7), P + FVector2D(7,0), P + FVector2D(0,7), P + FVector2D(-7,0), P + FVector2D(0,-7)}, Marker.Color, 2.f);
		else if (Marker.Kind == EMinimapMarkerKind::Camera)
		{
			Lines({P + FVector2D(-7,0), P + FVector2D(7,0)}, Marker.Color);
			Lines({P + FVector2D(0,-7), P + FVector2D(0,7)}, Marker.Color);
		}
		else if (Marker.Kind == EMinimapMarkerKind::Unit && !Marker.bPlayerOwned)
		{
			Lines({P + FVector2D(-R,-R), P + FVector2D(R,R)}, Marker.Color, 2.f);
			Lines({P + FVector2D(-R,R), P + FVector2D(R,-R)}, Marker.Color, 2.f);
		}
		else Lines({P + FVector2D(-R,-R), P + FVector2D(R,-R), P + FVector2D(R,R), P + FVector2D(-R,R), P + FVector2D(-R,-R)}, Marker.Color, Marker.bPlayerOwned ? 2.f : 1.f);
		if (Marker.bSelected) Lines({P + FVector2D(0,-9), P + FVector2D(9,0), P + FVector2D(0,9), P + FVector2D(-9,0), P + FVector2D(0,-9)}, FLinearColor::White);
		if (Marker.Kind == EMinimapMarkerKind::Territory)
			FSlateDrawElement::MakeText(Elements, Base + 2, Geometry.ToPaintGeometry(FVector2D(70,14)*TextScale, FSlateLayoutTransform(P + FVector2D(9,-6*TextScale))),
				Marker.ID.ToString(), Font(10), ESlateDrawEffect::None, FLinearColor::White);
	}
	FSlateDrawElement::MakeText(Elements, Base + 2, Geometry.ToPaintGeometry(FVector2D(Size.X,18*TextScale), FSlateLayoutTransform(FVector2D(10,4))),
		bStrategic ? TEXT("REGIONS") : TEXT("TACTICAL"), Font(11));
	FSlateDrawElement::MakeText(Elements, Base + 2, Geometry.ToPaintGeometry(FVector2D(Size.X,18*TextScale), FSlateLayoutTransform(FVector2D(10,Size.Y-19*TextScale))),
		bStrategic ? TEXT("Thick []: own") : TEXT("[] own | X foe | +"), Font(10));
	return Base + 2;
}
