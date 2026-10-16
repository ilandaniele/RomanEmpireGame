// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalObjective.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Physical occupation gate; the tactical subsystem remains the sole result owner. */
UCLASS()
class ROMANEMPIREGAME_API ATacticalObjective : public AActor
{
	GENERATED_BODY()
public:
	ATacticalObjective();
	UFUNCTION(BlueprintPure, Category = "Battle")
	FName GetObjectiveID() const { return ObjectiveID; }
	UFUNCTION(BlueprintCallable, Category = "Battle")
	bool TryCapture();
	UStaticMeshComponent* GetObjectiveMesh() const { return ObjectiveMesh; }
	/** Approach inside the occupation zone but outside the central visual/blocking mesh. */
	FVector GetOccupationApproachLocation() const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<UBoxComponent> OccupationZone;
	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<UStaticMeshComponent> ObjectiveMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Battle")
	FName ObjectiveID = TEXT("CarthaginianCamp");
	FTimerHandle CaptureTimer;
	bool bEvaluatingCapture = false;
	void CheckCapture();
};
