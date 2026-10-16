// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignSaveSubsystem.h"
#include "Campaign/CampaignSessionSubsystem.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/StrongObjectPtr.h"
#include "World/CampaignManager.h"

namespace
{
	bool ValidSlot(ECampaignSaveSlot Slot) { return Slot == ECampaignSaveSlot::Manual || Slot == ECampaignSaveSlot::Autosave; }
	ECampaignSaveFailure ReadRecord(const FString& Slot, FCampaignSaveRecord& Out, TArray<uint8>* OutBytes = nullptr)
	{
		if (Slot.IsEmpty()) return ECampaignSaveFailure::Unavailable;
		if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return ECampaignSaveFailure::NotFound;
		// Windows platform slots: reject oversized files BEFORE the platform allocates a buffer.
		const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), Slot + TEXT(".sav"));
		const int64 Size = IFileManager::Get().FileSize(*Path);
		if (Size < 0) return ECampaignSaveFailure::IOError;
		if (Size < RomanCampaignSave::HeaderBytes || Size > RomanCampaignSave::HeaderBytes + RomanCampaignSave::MaxPayloadBytes)
			return ECampaignSaveFailure::CorruptData;
		TArray<uint8> Bytes;
		if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, 0)) return ECampaignSaveFailure::IOError;
		const auto Failure = RomanCampaignSave::Decode(Bytes, Out);
		if (Failure == ECampaignSaveFailure::None && OutBytes) *OutBytes = MoveTemp(Bytes);
		return Failure;
	}
}

FString UCampaignSaveSubsystem::GetStorageSlotName(ECampaignSaveSlot Slot, int32 Index) const
{
	if (!ValidSlot(Slot) || Index < 0 || Index > 1) return FString();
	FString Prefix = TEXT("RomanEmpire");
#if WITH_DEV_AUTOMATION_TESTS
	if (GIsAutomationTesting)
	{
		// Test runs NEVER touch the user's fixed production slots, even across PIE travel.
		static const FString AutomationPrefix = FString::Printf(TEXT("RomanEmpire_Automation_%u_%s"),
			FPlatformProcess::GetCurrentProcessId(), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		Prefix = AutomationPrefix;
	}
#endif
	return FString::Printf(TEXT("%s_%s_%d"), *Prefix, Slot == ECampaignSaveSlot::Manual ? TEXT("Manual") : TEXT("Autosave"), Index);
}

ACampaignManager* UCampaignSaveSubsystem::CurrentCampaign() const
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const auto* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	return Mode && Mode->IsGameplayInitialized() ? Mode->GetCampaignManager() : nullptr;
}

bool UCampaignSaveSubsystem::CanSaveCampaign() const
{
	FCampaignTransferSnapshot Snapshot;
	const auto* Campaign = CurrentCampaign();
	return !bOperationActive && Campaign && Campaign->CaptureForSave(Snapshot) == ECampaignTransferFailure::None;
}

bool UCampaignSaveSubsystem::CanLoadCampaign(ECampaignSaveSlot Slot) const
{
	const auto* Session = GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
	const auto* Campaign = CurrentCampaign();
	return ValidSlot(Slot) && !bOperationActive && Campaign && Session && !Session->HasPendingTransfer() &&
		(!Session->HasSession() || Session->GetPhase() == ECampaignTransferPhase::Strategic) &&
		Campaign->ValidateCampaignAuthority(true) == ECampaignEncounterFailure::None &&
		(UGameplayStatics::DoesSaveGameExist(GetStorageSlotName(Slot, 0), 0) || UGameplayStatics::DoesSaveGameExist(GetStorageSlotName(Slot, 1), 0));
}

bool UCampaignSaveSubsystem::SetAutosaveEnabled(bool bEnabled)
{
	const auto* Campaign = CurrentCampaign();
	const auto* Session = GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
	if (bOperationActive || !Campaign || !Session || Session->HasPendingTransfer() ||
		Campaign->ValidateCampaignAuthority(true) != ECampaignEncounterFailure::None) return false;
	if (Settings.bAutosaveEnabled == bEnabled) return true;
	TGuardValue<bool> Guard(bOperationActive, true);
	Settings.bAutosaveEnabled = bEnabled; OnSaveStateChanged.Broadcast(); return true;
}

ECampaignSaveFailure UCampaignSaveSubsystem::ReadLatest(ECampaignSaveSlot Slot, FCampaignSaveRecord& Out, int32& OutIndex, bool& bOutRecovered) const
{
	if (!ValidSlot(Slot)) return ECampaignSaveFailure::Unavailable;
	FCampaignSaveRecord Records[2]; TArray<uint8> Bytes[2];
	const ECampaignSaveFailure Failures[2] = {
		ReadRecord(GetStorageSlotName(Slot, 0), Records[0], &Bytes[0]),
		ReadRecord(GetStorageSlotName(Slot, 1), Records[1], &Bytes[1]) };
	// A newer schema must be explicitly rejected, NEVER hidden by an older backup.
	for (const auto Failure : Failures) if (Failure == ECampaignSaveFailure::UnsupportedVersion) return Failure;
	const bool Good0 = Failures[0] == ECampaignSaveFailure::None, Good1 = Failures[1] == ECampaignSaveFailure::None;
	if (!Good0 && !Good1)
	{
		if (Failures[0] == ECampaignSaveFailure::NotFound && Failures[1] == ECampaignSaveFailure::NotFound) return ECampaignSaveFailure::NotFound;
		if (Failures[0] == ECampaignSaveFailure::IOError || Failures[1] == ECampaignSaveFailure::IOError) return ECampaignSaveFailure::IOError;
		return ECampaignSaveFailure::CorruptData;
	}
	if (Good0 && Good1 && Records[0].Generation == Records[1].Generation && Bytes[0] != Bytes[1]) return ECampaignSaveFailure::CorruptData;
	const int32 Latest = Good0 && (!Good1 || Records[0].Generation >= Records[1].Generation) ? 0 : 1;
	bOutRecovered = Failures[1 - Latest] != ECampaignSaveFailure::None && Failures[1 - Latest] != ECampaignSaveFailure::NotFound;
	Out = MoveTemp(Records[Latest]); OutIndex = Latest; return ECampaignSaveFailure::None;
}

ECampaignSaveFailure UCampaignSaveSubsystem::SaveCampaign(ECampaignSaveSlot Slot) { return SaveInternal(Slot, false); }

ECampaignSaveFailure UCampaignSaveSubsystem::AutosaveFromCampaign(ACampaignManager* Campaign)
{
	const auto* Session = GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
	if (!Settings.bAutosaveEnabled || !Session || !Session->HasSession()) return ECampaignSaveFailure::None;
	if (!Campaign || Campaign != CurrentCampaign()) return ECampaignSaveFailure::Unavailable;
	return SaveInternal(ECampaignSaveSlot::Autosave, true);
}

ECampaignSaveFailure UCampaignSaveSubsystem::SaveInternal(ECampaignSaveSlot Slot, bool bAutosaveBoundary)
{
	if (bOperationActive) return ECampaignSaveFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	bRecoveredBackup = false;
	const auto Finish = [this](ECampaignSaveFailure Failure) { LastFailure = Failure; OnSaveStateChanged.Broadcast(); return Failure; };
	if (!ValidSlot(Slot)) return Finish(ECampaignSaveFailure::Unavailable);
	ACampaignManager* Campaign = CurrentCampaign();
	if (!Campaign) return Finish(ECampaignSaveFailure::Unavailable);
	TStrongObjectPtr<URomanCampaignSaveGame> Save(NewObject<URomanCampaignSaveGame>(this));
	const auto Capture = Campaign->CaptureForSave(Save->Record.Campaign, bAutosaveBoundary);
	if (Capture != ECampaignTransferFailure::None) return Finish(ECampaignSaveFailure::UnsafeBoundary);
	Save->Record.Settings = Settings;
	FCampaignSaveRecord Previous; int32 PreviousIndex = 1; bool Recovered = false;
	const auto Existing = ReadLatest(Slot, Previous, PreviousIndex, Recovered);
	if (Existing != ECampaignSaveFailure::None && Existing != ECampaignSaveFailure::NotFound) return Finish(Existing);
	if (Existing == ECampaignSaveFailure::None)
	{
		if (Previous.Generation >= MAX_int64 - 1) return Finish(ECampaignSaveFailure::InvalidCampaign);
		Save->Record.Generation = Previous.Generation + 1;
	}
	TArray<uint8> Bytes; const auto Encoded = RomanCampaignSave::Encode(Save->Record, Bytes);
	if (Encoded != ECampaignSaveFailure::None) return Finish(Encoded);
	const FString Destination = GetStorageSlotName(Slot, 1 - PreviousIndex);
	if (!UGameplayStatics::SaveDataToSlot(Bytes, Destination, 0)) return Finish(ECampaignSaveFailure::IOError);
	FCampaignSaveRecord Verified; TArray<uint8> Readback;
	if (ReadRecord(Destination, Verified, &Readback) != ECampaignSaveFailure::None || Readback != Bytes) return Finish(ECampaignSaveFailure::IOError);
	// Publish only after platform write + readback; the previous good generation was never touched.
	LastSavedTurn = Save->Record.Campaign.Turn.CurrentTurn; bRecoveredBackup = Recovered;
	return Finish(ECampaignSaveFailure::None);
}

ECampaignSaveFailure UCampaignSaveSubsystem::LoadCampaign(ECampaignSaveSlot Slot)
{
	if (bOperationActive) return ECampaignSaveFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	bRecoveredBackup = false;
	const auto Finish = [this](ECampaignSaveFailure Failure) { LastFailure = Failure; OnSaveStateChanged.Broadcast(); return Failure; };
	const auto* Campaign = CurrentCampaign();
	auto* Session = GetGameInstance()->GetSubsystem<UCampaignSessionSubsystem>();
	if (!Campaign || !Session) return Finish(ECampaignSaveFailure::Unavailable);
	if (Session->HasPendingTransfer()) return Finish(ECampaignSaveFailure::Busy);
	if (Campaign->ValidateCampaignAuthority(true) != ECampaignEncounterFailure::None ||
		(Session->HasSession() && Session->GetPhase() != ECampaignTransferPhase::Strategic)) return Finish(ECampaignSaveFailure::UnsafeBoundary);
	TStrongObjectPtr<URomanCampaignSaveGame> Save(NewObject<URomanCampaignSaveGame>(this)); int32 Index = -1; bool Recovered = false;
	const auto Read = ReadLatest(Slot, Save->Record, Index, Recovered);
	if (Read != ECampaignSaveFailure::None) return Finish(Read);
	const auto Valid = Campaign->ValidateSavedCampaign(Save->Record.Campaign);
	if (Valid != ECampaignTransferFailure::None) return Finish(Valid == ECampaignTransferFailure::InvalidContent ? ECampaignSaveFailure::InvalidContent : ECampaignSaveFailure::InvalidCampaign);
	const auto Queued = Session->QueueSavedCampaign(Save->Record);
	if (Queued != ECampaignSessionFailure::None) return Finish(ECampaignSaveFailure::LoadRejected);
	bRecoveredBackup = Recovered; LastSavedTurn = Save->Record.Campaign.Turn.CurrentTurn;
	return Finish(ECampaignSaveFailure::None); // Travel queued, not a claim that incoming restoration has finished.
}
