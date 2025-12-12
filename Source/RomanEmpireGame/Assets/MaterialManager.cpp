// Copyright Roman Empire Game. All Rights Reserved.

#include "MaterialManager.h"
#include "RomanEmpireGame/RomanEmpireGame.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
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
	
	InitializeDefaultColors();
	InitializeDefaultMaterials();
	
	UE_LOG(LogRomanEmpire, Log, TEXT("Material Manager initialized"));
}

void AMaterialManager::InitializeDefaultColors()
{
	// Roman Red
	FactionColors.Add(EFactionID::Rome, FLinearColor(0.8f, 0.15f, 0.15f, 1.0f));
	
	// Carthaginian Purple
	FactionColors.Add(EFactionID::Carthage, FLinearColor(0.5f, 0.1f, 0.5f, 1.0f));
	
	// Gallic Green
	FactionColors.Add(EFactionID::Gaul, FLinearColor(0.15f, 0.6f, 0.15f, 1.0f));
	
	// Greek Blue
	FactionColors.Add(EFactionID::Greece, FLinearColor(0.2f, 0.4f, 0.8f, 1.0f));
	
	// Egyptian Gold
	FactionColors.Add(EFactionID::Egypt, FLinearColor(0.9f, 0.75f, 0.2f, 1.0f));
	
	// Briton Blue
	FactionColors.Add(EFactionID::Britannia, FLinearColor(0.3f, 0.5f, 0.7f, 1.0f));
	
	// Neutral Gray
	FactionColors.Add(EFactionID::None, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
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
	if (const FLinearColor* Color = FactionColors.Find(Faction))
	{
		return *Color;
	}
	return FLinearColor::Gray;
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
