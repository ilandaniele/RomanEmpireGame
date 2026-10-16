// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "MinimapPresentation.generated.h"

/** Detached display geometry, not gameplay authority. +X right, +Y down. */
struct ROMANEMPIREGAME_API FMinimapProjection
{
	FVector2D WorldMin = FVector2D(-1000, -1000);
	FVector2D WorldMax = FVector2D(1000, 1000);
	bool IsValid() const;
	FVector2D Project(FVector2D WorldPosition) const;
	FVector2D Unproject(FVector2D NormalizedPosition) const;
};
UENUM(BlueprintType)
enum class EMinimapMarkerKind : uint8 { Unit, Territory, Objective, Camera };
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FMinimapMarker
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EMinimapMarkerKind Kind = EMinimapMarkerKind::Unit;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName ID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D Position = FVector2D::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FLinearColor Color = FLinearColor::White;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPlayerOwned = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSelected = false;
	FMinimapMarker() = default;
	FMinimapMarker(EMinimapMarkerKind InKind, FName InID, FVector2D InPosition, FLinearColor InColor, bool bInPlayer, bool bInSelected)
		: Kind(InKind), ID(InID), Position(InPosition), Color(InColor), bPlayerOwned(bInPlayer), bSelected(bInSelected) {}
};
