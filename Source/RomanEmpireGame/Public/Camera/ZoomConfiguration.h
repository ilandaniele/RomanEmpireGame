// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZoomConfiguration.generated.h"

/** Semantic camera bands. They describe framing, not gameplay authority. */
UENUM(BlueprintType)
enum class EZoomLevel : uint8
{
	World		UMETA(DisplayName = "World View"),
	Territory	UMETA(DisplayName = "Territory View"),
	City		UMETA(DisplayName = "City View"),
	Ground		UMETA(DisplayName = "Ground View"),
	FirstPerson UMETA(DisplayName = "First Person View")
};

/** The sole definition of normalized zoom bounds, thresholds, and user steps. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FZoomConfiguration
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float Minimum = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float WorldMaximum = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float StrategicMaximum = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float TacticalMaximum = 0.70f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float GroundMaximum = 0.80f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float Maximum = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float DefaultTactical = 0.55f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float UserControlledMaximum = 0.70f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float InputStep = 0.03f;

	bool IsValid() const;
	EZoomLevel Classify(float NormalizedZoom) const;
};

namespace RomanEmpireCamera
{
	ROMANEMPIREGAME_API const FZoomConfiguration& GetDefaultZoomConfiguration();
}
