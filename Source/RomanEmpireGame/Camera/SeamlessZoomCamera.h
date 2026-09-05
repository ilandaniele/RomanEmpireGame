// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/ZoomConfiguration.h"
#include "GameFramework/Pawn.h"
#include "SeamlessZoomCamera.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class UFloatingPawnMovement;

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
	float GetTargetZoomLevel() const { return TargetZoomLevel; }

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

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnZoomLevelChanged,
		EZoomLevel, OldLevel,
		EZoomLevel, NewLevel);

	UPROPERTY(BlueprintAssignable, Category = "Camera|Events")
	FOnZoomLevelChanged OnZoomLevelChanged;

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	// Zoom settings
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
	TObjectPtr<AActor> FocusedActor;

	// Pan speed per zoom level
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Movement")
	float WorldPanSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Movement")
	float CityPanSpeed;

private:
	EZoomLevel LastZoomLevel;

	void UpdateCameraFromZoom(float DeltaSeconds);
	float GetTargetHeight() const;
	float GetTargetPitch() const;
	float GetTargetFOV() const;
	float GetPanSpeed() const;
};

