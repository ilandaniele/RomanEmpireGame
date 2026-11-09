// Copyright Roman Empire Game. All Rights Reserved.
#include "Building/BuildingPresentation.h"
#include "Building/BuildingTypes.h"
#include "Engine/StaticMesh.h"

namespace RomanEmpireBuildingPresentation
{
EFailure Validate(const FBuildingData& Data)
{
	if (!Data.PresentationMesh) return EFailure::None;
	const FTransform& Transform = Data.PresentationTransform;
	const FVector Scale = Transform.GetScale3D();
	if (!Transform.IsValid() || Scale.X <= 0 || Scale.Y <= 0 || Scale.Z <= 0)
		return EFailure::InvalidTransform;
	const FBox Bounds = Data.PresentationMesh->GetBoundingBox().TransformBy(Transform);
	if (!Bounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN() ||
		Bounds.GetSize().GetMin() <= 0) return EFailure::InvalidGeometry;
	const double HalfX = Data.FootprintSize.X * 50.0;
	const double HalfY = Data.FootprintSize.Y * 50.0;
	if (!FMath::IsFinite(HalfX) || !FMath::IsFinite(HalfY) || HalfX <= 0 || HalfY <= 0 ||
		Bounds.Min.X < -HalfX - .1 || Bounds.Max.X > HalfX + .1 ||
		Bounds.Min.Y < -HalfY - .1 || Bounds.Max.Y > HalfY + .1) return EFailure::OutsideFootprint;
	if (!FMath::IsNearlyEqual(Bounds.Min.Z, -static_cast<double>(PlacementHalfHeight), 1.0))
		return EFailure::NotGrounded;
	return EFailure::None;
}
}
