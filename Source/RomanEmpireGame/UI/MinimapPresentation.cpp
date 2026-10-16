// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/MinimapPresentation.h"
bool FMinimapProjection::IsValid() const
{
	const FVector2D Size = WorldMax - WorldMin;
	return !WorldMin.ContainsNaN() && !WorldMax.ContainsNaN() && !Size.ContainsNaN() &&
		Size.X > UE_SMALL_NUMBER && Size.Y > UE_SMALL_NUMBER;
}
FVector2D FMinimapProjection::Project(FVector2D Position) const
{
	if (!IsValid() || Position.ContainsNaN()) return FVector2D::ZeroVector;
	const FVector2D Size = WorldMax - WorldMin;
	return FVector2D(FMath::Clamp((Position.X - WorldMin.X) / Size.X, 0.0, 1.0),
		FMath::Clamp((Position.Y - WorldMin.Y) / Size.Y, 0.0, 1.0));
}
FVector2D FMinimapProjection::Unproject(FVector2D Position) const
{
	if (!IsValid() || Position.ContainsNaN()) return FVector2D::ZeroVector;
	return WorldMin + FVector2D(FMath::Clamp(Position.X, 0.0, 1.0),
		FMath::Clamp(Position.Y, 0.0, 1.0)) * (WorldMax - WorldMin);
}
