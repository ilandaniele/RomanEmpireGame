// Copyright Roman Empire Game. All Rights Reserved.

#include "SeamlessZoomCamera.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "Camera/CameraComponent.h"
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
	MinZoom = 0.0f;
	MaxZoom = 1.0f;
	ZoomInterpSpeed = 3.0f;

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
	CurrentZoomLevel = 0.3f;         // Start at territory view
	TargetZoomLevel = 0.3f;
	FocusedActor = nullptr;
	LastZoomLevel = EZoomLevel::Territory;
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
	CurrentZoomLevel = FMath::Clamp(NewZoomLevel, MinZoom, MaxZoom);
	TargetZoomLevel = CurrentZoomLevel;
	
	// Immediately update camera
	UpdateCameraFromZoom(0.0f);
}

void ASeamlessZoomCamera::SetTargetZoomLevel(float NewTargetZoom)
{
	TargetZoomLevel = FMath::Clamp(NewTargetZoom, MinZoom, MaxZoom);
}

EZoomLevel ASeamlessZoomCamera::GetZoomLevelEnum() const
{
	using namespace RomanEmpireConstants;
	
	if (CurrentZoomLevel <= ZOOM_WORLD_MAX)
	{
		return EZoomLevel::World;
	}
	else if (CurrentZoomLevel <= ZOOM_TERRITORY_MAX)
	{
		return EZoomLevel::Territory;
	}
	else if (CurrentZoomLevel <= ZOOM_CITY_MAX)
	{
		return EZoomLevel::City;
	}
	else if (CurrentZoomLevel <= ZOOM_GROUND_MAX)
	{
		return EZoomLevel::Ground;
	}
	else
	{
		return EZoomLevel::FirstPerson;
	}
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

	// Update spring arm length (affects camera distance/height)
	if (CameraArm)
	{
		if (DeltaSeconds > 0.0f)
		{
			CameraArm->TargetArmLength = FMath::FInterpTo(CameraArm->TargetArmLength, TargetHeight, DeltaSeconds, ZoomInterpSpeed);
			
			FRotator CurrentRotation = CameraArm->GetRelativeRotation();
			FRotator TargetRotation = FRotator(TargetPitch, CurrentRotation.Yaw, 0.0f);
			CameraArm->SetRelativeRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, ZoomInterpSpeed));
		}
		else
		{
			CameraArm->TargetArmLength = TargetHeight;
			CameraArm->SetRelativeRotation(FRotator(TargetPitch, 0.0f, 0.0f));
		}
	}

	// Update FOV
	if (Camera)
	{
		if (DeltaSeconds > 0.0f)
		{
			Camera->FieldOfView = FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, ZoomInterpSpeed);
		}
		else
		{
			Camera->FieldOfView = TargetFOV;
		}
	}
}

float ASeamlessZoomCamera::GetTargetHeight() const
{
	using namespace RomanEmpireConstants;
	
	// Interpolate height based on zoom level
	if (CurrentZoomLevel <= ZOOM_WORLD_MAX)
	{
		// World view
		float Alpha = CurrentZoomLevel / ZOOM_WORLD_MAX;
		return FMath::Lerp(WorldViewHeight, TerritoryViewHeight, Alpha);
	}
	else if (CurrentZoomLevel <= ZOOM_TERRITORY_MAX)
	{
		// Territory view
		float Alpha = (CurrentZoomLevel - ZOOM_WORLD_MAX) / (ZOOM_TERRITORY_MAX - ZOOM_WORLD_MAX);
		return FMath::Lerp(TerritoryViewHeight, CityViewHeight, Alpha);
	}
	else if (CurrentZoomLevel <= ZOOM_CITY_MAX)
	{
		// City view
		float Alpha = (CurrentZoomLevel - ZOOM_TERRITORY_MAX) / (ZOOM_CITY_MAX - ZOOM_TERRITORY_MAX);
		return FMath::Lerp(CityViewHeight, GroundViewHeight, Alpha);
	}
	else if (CurrentZoomLevel <= ZOOM_GROUND_MAX)
	{
		// Ground view
		float Alpha = (CurrentZoomLevel - ZOOM_CITY_MAX) / (ZOOM_GROUND_MAX - ZOOM_CITY_MAX);
		return FMath::Lerp(GroundViewHeight, FirstPersonHeight, Alpha);
	}
	else
	{
		// First person view
		return FirstPersonHeight;
	}
}

float ASeamlessZoomCamera::GetTargetPitch() const
{
	using namespace RomanEmpireConstants;
	
	// Similar interpolation for pitch
	if (CurrentZoomLevel <= ZOOM_WORLD_MAX)
	{
		float Alpha = CurrentZoomLevel / ZOOM_WORLD_MAX;
		return FMath::Lerp(WorldViewPitch, TerritoryViewPitch, Alpha);
	}
	else if (CurrentZoomLevel <= ZOOM_TERRITORY_MAX)
	{
		float Alpha = (CurrentZoomLevel - ZOOM_WORLD_MAX) / (ZOOM_TERRITORY_MAX - ZOOM_WORLD_MAX);
		return FMath::Lerp(TerritoryViewPitch, CityViewPitch, Alpha);
	}
	else if (CurrentZoomLevel <= ZOOM_CITY_MAX)
	{
		float Alpha = (CurrentZoomLevel - ZOOM_TERRITORY_MAX) / (ZOOM_CITY_MAX - ZOOM_TERRITORY_MAX);
		return FMath::Lerp(CityViewPitch, GroundViewPitch, Alpha);
	}
	else if (CurrentZoomLevel <= ZOOM_GROUND_MAX)
	{
		float Alpha = (CurrentZoomLevel - ZOOM_CITY_MAX) / (ZOOM_GROUND_MAX - ZOOM_CITY_MAX);
		return FMath::Lerp(GroundViewPitch, FirstPersonPitch, Alpha);
	}
	else
	{
		return FirstPersonPitch;
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
