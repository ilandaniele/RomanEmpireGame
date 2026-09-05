// Copyright Roman Empire Game. All Rights Reserved.

#include "Camera/ZoomConfiguration.h"

bool FZoomConfiguration::IsValid() const
{
	return Minimum >= 0.0f &&
		Minimum < Maximum &&
		Maximum <= 1.0f &&
		Minimum <= WorldMaximum &&
		WorldMaximum <= StrategicMaximum &&
		StrategicMaximum <= TacticalMaximum &&
		TacticalMaximum <= GroundMaximum &&
		GroundMaximum <= Maximum &&
		DefaultTactical > StrategicMaximum &&
		DefaultTactical <= TacticalMaximum &&
		UserControlledMaximum >= DefaultTactical &&
		UserControlledMaximum <= TacticalMaximum &&
		InputStep > 0.0f;
}

EZoomLevel FZoomConfiguration::Classify(float NormalizedZoom) const
{
	const float ClampedZoom = FMath::Clamp(NormalizedZoom, Minimum, Maximum);
	if (ClampedZoom <= WorldMaximum) return EZoomLevel::World;
	if (ClampedZoom <= StrategicMaximum) return EZoomLevel::Territory;
	if (ClampedZoom <= TacticalMaximum) return EZoomLevel::City;
	if (ClampedZoom <= GroundMaximum) return EZoomLevel::Ground;
	return EZoomLevel::FirstPerson;
}

const FZoomConfiguration& RomanEmpireCamera::GetDefaultZoomConfiguration()
{
	static const FZoomConfiguration Configuration;
	return Configuration;
}
