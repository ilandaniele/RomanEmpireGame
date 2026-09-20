// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitSelectionPresentation.generated.h"

class UDecalComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Local presentation only. The controller still owns the selected-unit set.
 * Actor eligibility is checked by the coordinator; deselection always permits cleanup.
 * Components remain actor-owned default subobjects, not duplicated here.
 */
USTRUCT()
struct ROMANEMPIREGAME_API FUnitSelectionPresentation
{
	GENERATED_BODY()

	void SetSelected(bool bNewSelected, UDecalComponent* Decal, UStaticMeshComponent* Ring, UObject* MaterialOuter);
	bool IsSelected() const { return bSelected; }

private:
	UPROPERTY(VisibleAnywhere)
	bool bSelected = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMaterial;
};
