// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignSave.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignSaveSubsystem.generated.h"
class ACampaignManager;

/** Persistence/preferences only. Current-world managers remain campaign authorities. */
UCLASS()
class ROMANEMPIREGAME_API UCampaignSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Campaign|Save") ECampaignSaveFailure SaveCampaign(ECampaignSaveSlot Slot = ECampaignSaveSlot::Manual);
	UFUNCTION(BlueprintCallable, Category = "Campaign|Save") ECampaignSaveFailure LoadCampaign(ECampaignSaveSlot Slot = ECampaignSaveSlot::Manual);
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") bool CanSaveCampaign() const;
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") bool CanLoadCampaign(ECampaignSaveSlot Slot = ECampaignSaveSlot::Manual) const;
	UFUNCTION(BlueprintCallable, Category = "Campaign|Save") bool SetAutosaveEnabled(bool bEnabled);
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") FCampaignSaveSettings GetSettings() const { return Settings; }
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") ECampaignSaveFailure GetLastFailure() const { return LastFailure; }
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") int32 GetLastSavedTurn() const { return LastSavedTurn; }
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") bool WasBackupRecovered() const { return bRecoveredBackup; }
	/** Fixed names, useful for diagnostics. Never accepts a caller-provided filesystem path. */
	UFUNCTION(BlueprintPure, Category = "Campaign|Save") FString GetStorageSlotName(ECampaignSaveSlot Slot, int32 Index) const;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveStateChanged);
	UPROPERTY(BlueprintAssignable, Category = "Campaign|Save") FOnSaveStateChanged OnSaveStateChanged;
private:
	friend class ACampaignManager;
	friend class UCampaignSessionSubsystem;
	ECampaignSaveFailure AutosaveFromCampaign(ACampaignManager* Campaign);
	ECampaignSaveFailure SaveInternal(ECampaignSaveSlot Slot, bool bAutosaveBoundary);
	ECampaignSaveFailure ReadLatest(ECampaignSaveSlot Slot, FCampaignSaveRecord& Out, int32& OutIndex, bool& bOutRecovered) const;
	ACampaignManager* CurrentCampaign() const;
	void ApplyLoadedSettings(const FCampaignSaveSettings& Loaded) { Settings = Loaded; }
	UPROPERTY() FCampaignSaveSettings Settings;
	ECampaignSaveFailure LastFailure = ECampaignSaveFailure::None;
	int32 LastSavedTurn = 0;
	bool bRecoveredBackup = false;
	bool bOperationActive = false;
};
