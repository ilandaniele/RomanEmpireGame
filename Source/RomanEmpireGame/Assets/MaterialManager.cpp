// Copyright Roman Empire Game. All Rights Reserved.

#include "MaterialManager.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"

AMaterialManager::AMaterialManager()
{
	PrimaryActorTick.bCanEverTick = false;

	GlobalParameterCollection = nullptr;
	GhostValidBaseMaterial = nullptr;
	GhostInvalidBaseMaterial = nullptr;
}

void AMaterialManager::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeDefaultMaterials();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Material Manager initialized"));
}

void AMaterialManager::InitializeDefaultMaterials()
{
	// In a full implementation, these would reference actual material assets
	// For now, we define the structure that would be filled in the editor
}

UMaterialInstanceDynamic* AMaterialManager::CreateFactionMaterial(UMaterialInterface* BaseMaterial, EFactionID Faction)
{
	if (!BaseMaterial)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	
	if (DynamicMaterial)
	{
		FLinearColor FactionColor = GetFactionColor(Faction);
		DynamicMaterial->SetVectorParameterValue(TEXT("FactionColor"), FactionColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("PrimaryColor"), FactionColor);
		
		// Secondary color (slightly darker)
		FLinearColor SecondaryColor = FactionColor * 0.7f;
		SecondaryColor.A = 1.0f;
		DynamicMaterial->SetVectorParameterValue(TEXT("SecondaryColor"), SecondaryColor);
	}

	return DynamicMaterial;
}

UMaterialInterface* AMaterialManager::GetMaterial(EMaterialType MaterialType) const
{
	if (const UMaterialInterface* const* FoundMaterial = BaseMaterials.Find(MaterialType))
	{
		return const_cast<UMaterialInterface*>(*FoundMaterial);
	}
	return nullptr;
}

FLinearColor AMaterialManager::GetFactionColor(EFactionID Faction) const
{
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanFactionDefinition* Definition = Registry ? Registry->FindFaction(Faction) : nullptr;
	return Definition ? Definition->Definition.FactionColor : FLinearColor::Gray;
}

void AMaterialManager::SetGlobalFactionColor(EFactionID Faction)
{
	if (!GlobalParameterCollection)
	{
		return;
	}

	FLinearColor Color = GetFactionColor(Faction);
	
	UKismetMaterialLibrary::SetVectorParameterValue(
		this,
		GlobalParameterCollection,
		TEXT("PlayerFactionColor"),
		Color
	);
}

UMaterialInstanceDynamic* AMaterialManager::CreateGhostMaterial(bool bValid)
{
	UMaterialInterface* BaseMaterial = bValid ? GhostValidBaseMaterial : GhostInvalidBaseMaterial;
	
	if (!BaseMaterial)
	{
		UE_LOG(LogRomanEmpire, Warning, TEXT("Ghost material not set"));
		return nullptr;
	}

	UMaterialInstanceDynamic* GhostMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	
	if (GhostMaterial)
	{
		// Set color - green for valid, red for invalid
		FLinearColor Color = bValid ? 
			FLinearColor(0.2f, 0.8f, 0.2f, 0.5f) : 
			FLinearColor(0.8f, 0.2f, 0.2f, 0.5f);
		
		GhostMaterial->SetVectorParameterValue(TEXT("GhostColor"), Color);
		GhostMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.5f);
	}

	return GhostMaterial;
}

AMaterialManager* AMaterialManager::GetMaterialManager(UObject* WorldContextObject)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject->GetWorld(), AMaterialManager::StaticClass(), FoundActors);
	
	if (FoundActors.Num() > 0)
	{
		return Cast<AMaterialManager>(FoundActors[0]);
	}
	return nullptr;
}

