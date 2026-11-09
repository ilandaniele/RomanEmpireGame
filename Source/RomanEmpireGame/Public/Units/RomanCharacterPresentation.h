// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Units/RomanSoldierAnimInstance.h"
#include "RomanCharacterPresentation.generated.h"

class USkeletalMesh;
class UAnimMontage;

/** Small required bootstrap art bundle. Hard references guarantee lifetime/cooking.
 * No mutable faction, combat, save or campaign authority. */
UCLASS(BlueprintType)
class ROMANEMPIREGAME_API URomanCharacterPresentation : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<USkeletalMesh> RomeMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<USkeletalMesh> CarthageMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<USkeletalMesh> GaulMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSubclassOf<URomanSoldierAnimInstance> AnimationClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Opt-in for textured art. Legacy bundles retain their exact faction-parent policy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Materials")
	bool bUseMeshMaterialParents = false;
	/** Semantic slots receiving Color/FactionTintStrength; all slots receive Damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Materials", meta = (EditCondition = "bUseMeshMaterialParents"))
	TArray<FName> FactionTintSlots = {TEXT("Body"), TEXT("Shield"), TEXT("Crest")};
};
