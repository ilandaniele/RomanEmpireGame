// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "FactionManager.generated.h"

/**
 * Manages all factions in the game
 * Handles faction initialization, resources, diplomacy, and AI behavior
 */
UCLASS()
class ROMANEMPIREGAME_API AFactionManager : public AActor
{
	GENERATED_BODY()

public:
	AFactionManager();

	virtual void BeginPlay() override;

	// Faction access
	UFUNCTION(BlueprintPure, Category = "Faction")
	FFactionInfo GetFactionInfo(EFactionID FactionID) const;

	UFUNCTION(BlueprintPure, Category = "Faction")
	FFactionResources GetFactionResources(EFactionID FactionID) const;

	UFUNCTION(BlueprintCallable, Category = "Faction")
	void ModifyFactionResources(EFactionID FactionID, const FFactionResources& Delta);

	UFUNCTION(BlueprintPure, Category = "Faction")
	bool CanFactionAfford(EFactionID FactionID, const FFactionResources& Cost) const;

	UFUNCTION(BlueprintCallable, Category = "Faction")
	bool DeductFactionResources(EFactionID FactionID, const FFactionResources& Cost);

	// Diplomacy
	UFUNCTION(BlueprintPure, Category = "Faction|Diplomacy")
	EDiplomaticStatus GetDiplomaticStatus(EFactionID Faction1, EFactionID Faction2) const;

	UFUNCTION(BlueprintCallable, Category = "Faction|Diplomacy")
	void SetDiplomaticStatus(EFactionID Faction1, EFactionID Faction2, EDiplomaticStatus NewStatus);

	UFUNCTION(BlueprintPure, Category = "Faction|Diplomacy")
	bool AreAtWar(EFactionID Faction1, EFactionID Faction2) const;

	UFUNCTION(BlueprintPure, Category = "Faction|Diplomacy")
	bool AreAllied(EFactionID Faction1, EFactionID Faction2) const;

	// Player faction
	UFUNCTION(BlueprintPure, Category = "Faction")
	EFactionID GetPlayerFaction() const { return PlayerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Faction")
	void SetPlayerFaction(EFactionID NewFaction);

	// Territory
	UFUNCTION(BlueprintCallable, Category = "Faction|Territory")
	void AssignTerritoryToFaction(FName TerritoryID, EFactionID FactionID);

	UFUNCTION(BlueprintPure, Category = "Faction|Territory")
	int32 GetFactionTerritoryCount(EFactionID FactionID) const;

	// AI turn processing
	UFUNCTION(BlueprintCallable, Category = "Faction|AI")
	void ProcessAITurns();

protected:
	// Faction data
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction|Setup")
	TMap<EFactionID, FFactionInfo> FactionInfoMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction|State")
	TMap<EFactionID, FFactionResources> FactionResourcesMap;

	// Diplomacy matrix
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction|Diplomacy")
	TArray<FDiplomaticRelation> DiplomacyMatrix;

	// Territory ownership
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction|Territory")
	TMap<FName, EFactionID> TerritoryOwnership;

	// Player's faction
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	EFactionID PlayerFaction;

private:
	void InitializeDefaultFactions();
	void InitializeDefaultDiplomacy();
	void ProcessAIFactionTurn(EFactionID FactionID);
};
