// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RomanEmpireGame/Units/UnitBase.h"
#include "Legionary.generated.h"

/**
 * Roman Legionary - The backbone of the Roman army
 * Heavy infantry with sword (gladius) and shield (scutum)
 */
UCLASS()
class ROMANEMPIREGAME_API ALegionary : public AUnitBase
{
	GENERATED_BODY()

public:
	ALegionary();

	// Special abilities
	UFUNCTION(BlueprintCallable, Category = "Legionary")
	void ActivateTestudo();

	UFUNCTION(BlueprintCallable, Category = "Legionary")
	void DeactivateTestudo();

	UFUNCTION(BlueprintPure, Category = "Legionary")
	bool IsInTestudo() const { return bInTestudo; }

	// Pilum throw (javelin)
	UFUNCTION(BlueprintCallable, Category = "Legionary")
	void ThrowPilum();

	UFUNCTION(BlueprintPure, Category = "Legionary")
	int32 GetPilaRemaining() const { return PilaCount; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legionary")
	bool bInTestudo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legionary")
	int32 MaxPila; // Javelins

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legionary")
	int32 PilaCount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legionary")
	TSubclassOf<AActor> PilumProjectile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legionary")
	float PilumDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legionary")
	float PilumRange;
};
