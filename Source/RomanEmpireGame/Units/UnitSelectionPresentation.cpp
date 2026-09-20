// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/UnitSelectionPresentation.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

void FUnitSelectionPresentation::SetSelected(bool bNewSelected, UDecalComponent* Decal,
	UStaticMeshComponent* Ring, UObject* MaterialOuter)
{
	bSelected = bNewSelected;
	if (Decal)
	{
		Decal->SetVisibility(bSelected);
		Decal->SetHiddenInGame(!bSelected);
	}
	if (!Ring) return;
	Ring->SetVisibility(bSelected);
	Ring->SetHiddenInGame(!bSelected);
	if (bSelected && !RingMaterial && MaterialOuter)
	{
		if (UMaterial* Base = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial")))
		{
			RingMaterial = UMaterialInstanceDynamic::Create(Base, MaterialOuter);
			if (RingMaterial)
			{
				RingMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 1.0f, 0.0f, 0.7f));
				Ring->SetMaterial(0, RingMaterial);
			}
		}
	}
}
