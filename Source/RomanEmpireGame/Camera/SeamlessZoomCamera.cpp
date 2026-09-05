// Copyright Roman Empire Game. All Rights Reserved.

#include "SeamlessZoomCamera.h"
#include "RomanEmpireGame.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ASeamlessZoomCamera::ASeamlessZoomCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Create spring arm for camera distance
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(RootScene);
	CameraArm->TargetArmLength = 10000.0f;
	CameraArm->bDoCollisionTest = false;
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritPitch = false;
	CameraArm->bInheritYaw = false;
	CameraArm->bInheritRoll = false;
	CameraArm->SetRelativeRotation(FRotator(-75.0f, 0.0f, 0.0f)); // Start looking down

	// Create camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// Create movement component
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 5000.0f;

	// Initialize zoom settings
	ZoomInterpSpeed = 50.0f;  // Near-instant camera catch-up

	// Height settings (in Unreal units, 1 unit = 1 cm)
	WorldViewHeight = 50000.0f;      // 500 meters - see continents
	TerritoryViewHeight = 20000.0f;  // 200 meters - see regions
	CityViewHeight = 5000.0f;        // 50 meters - see buildings
	GroundViewHeight = 500.0f;       // 5 meters - third person
	FirstPersonHeight = 180.0f;      // 1.8 meters - eye level

	// Pitch settings (looking down angle)
	WorldViewPitch = -85.0f;         // Almost straight down
	TerritoryViewPitch = -75.0f;     // Steep angle
	CityViewPitch = -60.0f;          // RTS angle
	GroundViewPitch = -30.0f;        // Third person angle
	FirstPersonPitch = 0.0f;         // Horizontal - FPS

	// FOV settings
	WorldViewFOV = 60.0f;            // Normal FOV for strategic
	FirstPersonFOV = 90.0f;          // Wide FOV for FPS

	// Pan speeds
	WorldPanSpeed = 10000.0f;        // Fast pan when zoomed out
	CityPanSpeed = 2000.0f;          // Slower pan when zoomed in

	// Initialize state
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	CurrentZoomLevel = Zoom.DefaultTactical;
	TargetZoomLevel = Zoom.DefaultTactical;
	FocusedActor = nullptr;
	LastZoomLevel = Zoom.Classify(CurrentZoomLevel);
}

void ASeamlessZoomCamera::BeginPlay()
{
	Super::BeginPlay();
	
	// Set initial camera state
	SetZoomLevel(CurrentZoomLevel);
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Seamless Zoom Camera initialized at zoom level: %f"), CurrentZoomLevel);
}

void ASeamlessZoomCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Smoothly interpolate to target zoom
	if (!FMath::IsNearlyEqual(CurrentZoomLevel, TargetZoomLevel, 0.001f))
	{
		CurrentZoomLevel = FMath::FInterpTo(CurrentZoomLevel, TargetZoomLevel, DeltaSeconds, ZoomInterpSpeed);
	}

	// Update camera based on current zoom
	UpdateCameraFromZoom(DeltaSeconds);

	// Check if zoom level category changed
	EZoomLevel CurrentLevel = GetZoomLevelEnum();
	if (CurrentLevel != LastZoomLevel)
	{
		OnZoomLevelChanged.Broadcast(LastZoomLevel, CurrentLevel);
		LastZoomLevel = CurrentLevel;
		
		UE_LOG(LogRomanEmpire, Log, TEXT("Zoom level changed to: %d"), static_cast<int32>(CurrentLevel));
	}

	// Follow focused actor if any
	if (FocusedActor)
	{
		FVector TargetLocation = FocusedActor->GetActorLocation();
		FVector CurrentLocation = GetActorLocation();
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, FVector(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z), DeltaSeconds, 5.0f);
		SetActorLocation(NewLocation);
	}
}

void ASeamlessZoomCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Input is handled by PlayerController
}

void ASeamlessZoomCamera::SetZoomLevel(float NewZoomLevel)
{
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	CurrentZoomLevel = FMath::Clamp(NewZoomLevel, Zoom.Minimum, Zoom.Maximum);
	TargetZoomLevel = CurrentZoomLevel;
	
	// Immediately update camera
	UpdateCameraFromZoom(0.0f);
}

void ASeamlessZoomCamera::SetTargetZoomLevel(float NewTargetZoom)
{
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	TargetZoomLevel = FMath::Clamp(NewTargetZoom, Zoom.Minimum, Zoom.Maximum);
}

EZoomLevel ASeamlessZoomCamera::GetZoomLevelEnum() const
{
	return RomanEmpireCamera::GetDefaultZoomConfiguration().Classify(CurrentZoomLevel);
}

void ASeamlessZoomCamera::FocusOnLocation(const FVector& Location)
{
	FocusedActor = nullptr;
	SetActorLocation(FVector(Location.X, Location.Y, GetActorLocation().Z));
}

void ASeamlessZoomCamera::FocusOnActor(AActor* Actor)
{
	FocusedActor = Actor;
}

void ASeamlessZoomCamera::PanCamera(const FVector2D& Direction)
{
	if (GetZoomLevelEnum() == EZoomLevel::FirstPerson)
	{
		return; // Don't pan in FPS mode
	}

	float Speed = GetPanSpeed();
	FVector Movement = FVector(Direction.Y, Direction.X, 0.0f) * Speed * GetWorld()->GetDeltaSeconds();
	AddActorWorldOffset(Movement);
	
	// Clear focus when manually panning
	FocusedActor = nullptr;
}

void ASeamlessZoomCamera::UpdateCameraFromZoom(float DeltaSeconds)
{
	// Calculate target values based on zoom level
	float TargetHeight = GetTargetHeight();
	float TargetPitch = GetTargetPitch();
	float TargetFOV = GetTargetFOV();

	// Fast but smooth interpolation (speed 15 = responsive, not jerky)
	float Speed = 15.0f;

	if (CameraArm)
	{
		if (DeltaSeconds > 0.0f)
		{
			CameraArm->TargetArmLength = FMath::FInterpTo(CameraArm->TargetArmLength, TargetHeight, DeltaSeconds, Speed);
			FRotator CurrentRotation = CameraArm->GetRelativeRotation();
			FRotator TargetRotation = FRotator(TargetPitch, CurrentRotation.Yaw, 0.0f);
			CameraArm->SetRelativeRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, Speed));
		}
		else
		{
			CameraArm->TargetArmLength = TargetHeight;
			CameraArm->SetRelativeRotation(FRotator(TargetPitch, 0.0f, 0.0f));
		}
	}

	if (Camera)
	{
		if (DeltaSeconds > 0.0f)
		{
			Camera->FieldOfView = FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, Speed);
		}
		else
		{
			Camera->FieldOfView = TargetFOV;
		}
	}
}

float ASeamlessZoomCamera::GetTargetHeight() const
{
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	// Single smooth exponential curve: zoom 0.0 = 30km, zoom 0.70 = 500, no hard breaks
	// Use log interpolation for perceptually smooth zoom
	const float HeightAtLow = 3000000.0f;  // 30km at full zoom out
	const float HeightAtHigh = 500.0f;     // 5m at full zoom in

	// Clamp zoom to valid range
	const float Z = FMath::Clamp(CurrentZoomLevel, Zoom.Minimum, Zoom.TacticalMaximum);
	const float T = (Z - Zoom.Minimum) / (Zoom.TacticalMaximum - Zoom.Minimum); // 0.0..1.0

	// Exponential: log-lerp between min and max heights
	float LogLow = FMath::Loge(HeightAtLow);
	float LogHigh = FMath::Loge(HeightAtHigh);
	float LogH = FMath::Lerp(LogLow, LogHigh, T);
	return FMath::Exp(LogH);
}

float ASeamlessZoomCamera::GetTargetPitch() const
{
	const FZoomConfiguration& Zoom = RomanEmpireCamera::GetDefaultZoomConfiguration();
	// Strategic: -85 → -70
	if (CurrentZoomLevel <= Zoom.StrategicMaximum)
	{
		const float Alpha = (CurrentZoomLevel - Zoom.Minimum) /
			(Zoom.StrategicMaximum - Zoom.Minimum);
		return FMath::Lerp(-85.0f, -70.0f, Alpha);
	}
	// Tactical: -70 → -45
	else if (CurrentZoomLevel <= Zoom.TacticalMaximum)
	{
		const float Alpha = (CurrentZoomLevel - Zoom.StrategicMaximum) /
			(Zoom.TacticalMaximum - Zoom.StrategicMaximum);
		return FMath::Lerp(-70.0f, -45.0f, Alpha);
	}
	// Ground: -45 → 0
	else if (CurrentZoomLevel <= Zoom.GroundMaximum)
	{
		const float Alpha = (CurrentZoomLevel - Zoom.TacticalMaximum) /
			(Zoom.GroundMaximum - Zoom.TacticalMaximum);
		return FMath::Lerp(-45.0f, 0.0f, Alpha);
	}
	else
	{
		return 0.0f;
	}
}

float ASeamlessZoomCamera::GetTargetFOV() const
{
	// Linear interpolation of FOV across entire zoom range
	return FMath::Lerp(WorldViewFOV, FirstPersonFOV, CurrentZoomLevel);
}

float ASeamlessZoomCamera::GetPanSpeed() const
{
	// Faster pan when zoomed out, slower when zoomed in
	return FMath::Lerp(WorldPanSpeed, CityPanSpeed, CurrentZoomLevel);
}

