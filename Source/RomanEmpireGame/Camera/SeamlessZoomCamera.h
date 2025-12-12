// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SeamlessZoomCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFloatingPawnMovement;

/**
 * Zoom level enum representing the current camera state
 */
UENUM(BlueprintType)
enum class EZoomLevel : uint8
{
	World		UMETA(DisplayName = "World View"),       // 0-20% - Continental/world view
	Territory	UMETA(DisplayName = "Territory View"),   // 20-40% - Regional view
	City		UMETA(DisplayName = "City View"),        // 40-60% - Settlement/RTS view
	Ground		UMETA(DisplayName = "Ground View"),      // 60-80% - Third-person view
	FirstPerson	UMETA(DisplayName = "First Person View") // 80-100% - FPS view
};

/**
 * Camera pawn that handles seamless zooming from world map to first-person view
 * This is the core innovation of the game - smooth transitions between all gameplay layers
 */
UCLASS()
class ROMANEMPIREGAME_API ASeamlessZoomCamera : public APawn
{
	GENERATED_BODY()

public:
	ASeamlessZoomCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Zoom control
	UFUNCTION(BlueprintCallable, Category = "Camera|Zoom")
	void SetZoomLevel(float NewZoomLevel);

	UFUNCTION(BlueprintCallable, Category = "Camera|Zoom")
	void SetTargetZoomLevel(float NewTargetZoom);

	UFUNCTION(BlueprintPure, Category = "Camera|Zoom")
	float GetZoomLevel() const { return CurrentZoomLevel; }

	UFUNCTION(BlueprintPure, Category = "Camera|Zoom")
	EZoomLevel GetZoomLevelEnum() const;

	// Camera focus
	UFUNCTION(BlueprintCallable, Category = "Camera|Focus")
	void FocusOnLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Camera|Focus")
	void FocusOnActor(AActor* Actor);

	// Movement
	UFUNCTION(BlueprintCallable, Category = "Camera|Movement")
	void PanCamera(const FVector2D& Direction);

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFloatingPawnMovement* MovementComponent;

	// Zoom settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
	float MinZoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
	float MaxZoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Zoom")
	float ZoomInterpSpeed;

	// Height settings per zoom level
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Height")
	float WorldViewHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Height")
	float TerritoryViewHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Height")
	float CityViewHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Height")
	float GroundViewHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Height")
	float FirstPersonHeight;

	// Pitch settings per zoom level
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Pitch")
	float WorldViewPitch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Pitch")
	float TerritoryViewPitch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Pitch")
	float CityViewPitch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Pitch")
	float GroundViewPitch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Pitch")
	float FirstPersonPitch;

	// FOV settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|FOV")
	float WorldViewFOV;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|FOV")
	float FirstPersonFOV;

	// Current state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|State")
	float CurrentZoomLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|State")
	float TargetZoomLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|State")
	AActor* FocusedActor;

	// Pan speed per zoom level
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Movement")
	float WorldPanSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Movement")
	float CityPanSpeed;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoomLevelChanged, EZoomLevel, OldLevel, EZoomLevel, NewLevel);
	
	UPROPERTY(BlueprintAssignable, Category = "Camera|Events")
	FOnZoomLevelChanged OnZoomLevelChanged;

private:
	EZoomLevel LastZoomLevel;

	void UpdateCameraFromZoom(float DeltaSeconds);
	float GetTargetHeight() const;
	float GetTargetPitch() const;
	float GetTargetFOV() const;
	float GetPanSpeed() const;
};
