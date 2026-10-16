// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignTransfer.h"
#include "Campaign/CampaignSave.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignSessionSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECampaignSessionFailure : uint8
{
	None, Unavailable, Busy, WrongPhase, InvalidMap, TransferRejected, EncounterRejected, Unresolved, RestoreFailed
};

/** Travel coordinator only. Managers in the current world remain the sole live
 * authorities; the detached capsule exists only between validated world boundaries. */
UCLASS()
class ROMANEMPIREGAME_API UCampaignSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** Leave the sandbox for a strategic session, retaining its canonical campaign. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Session")
	ECampaignSessionFailure EnterStrategicCampaign();
	UFUNCTION(BlueprintCallable, Category = "Campaign|Session")
	ECampaignSessionFailure BeginTacticalEncounter(FGuid ArmyID, FName TargetTerritoryID);
	/** Apply only the actual mapped terminal result once, then reload the strategic world. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Session")
	ECampaignSessionFailure ReturnToStrategicCampaign();
	/** Retry a quarantined incoming world with the SAME validated capsule, not defaults. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Session")
	ECampaignSessionFailure RetryPendingTransfer();
	UFUNCTION(BlueprintPure, Category = "Campaign|Session") bool CanRetryPendingTransfer() const;
	UFUNCTION(BlueprintPure, Category = "Campaign|Session") bool HasSession() const;
	UFUNCTION(BlueprintPure, Category = "Campaign|Session") bool HasPendingTransfer() const { return bPending; }
	UFUNCTION(BlueprintPure, Category = "Campaign|Session") ECampaignTransferPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category = "Campaign|Session") ECampaignSessionFailure GetLastFailure() const { return LastFailure; }
	UFUNCTION(BlueprintPure, Category = "Campaign|Session") ECampaignTransferFailure GetLastTransferFailure() const { return LastTransferFailure; }
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionChanged);
	UPROPERTY(BlueprintAssignable, Category = "Campaign|Session") FOnSessionChanged OnSessionChanged;
private:
	friend class ARomanEmpireGameMode;
	friend class ACampaignManager;
	friend class UCampaignSaveSubsystem;
	ECampaignSessionFailure QueueSavedCampaign(const FCampaignSaveRecord& Record);
	ECampaignSessionFailure BeginQueuedAITacticalEncounter();
	bool ExpectsIncomingWorld(const UWorld* World) const;
	bool MatchesIncomingWorld(const UWorld* World, const FString& Options) const;
	bool RestoreIncomingWorld(UWorld* World, const FString& Options);
	ECampaignSessionFailure QueueTransfer(ECampaignTransferPhase TargetPhase, const FSoftObjectPath& Destination,
		const FSoftObjectPath& ReturnMap, const FCampaignEncounterState& Encounter);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type Failure, const FString& Reason);
	UPROPERTY() FCampaignTransferSnapshot PendingSnapshot;
	UPROPERTY() FCampaignSaveSettings PendingSaveSettings;
	bool bHasPendingSaveSettings = false;
	TWeakObjectPtr<UWorld> SourceWorld;
	TWeakObjectPtr<UWorld> SessionWorld;
	TWeakObjectPtr<UWorld> FailedWorld;
	FSoftObjectPath StrategicMap;
	FGuid TransferNonce;
	FDelegateHandle TravelFailureHandle;
	bool bPending = false;
	bool bOperationActive = false;
	bool bHasSession = false;
	ECampaignTransferPhase Phase = ECampaignTransferPhase::Strategic;
	ECampaignSessionFailure LastFailure = ECampaignSessionFailure::None;
	ECampaignTransferFailure LastTransferFailure = ECampaignTransferFailure::None;
};
