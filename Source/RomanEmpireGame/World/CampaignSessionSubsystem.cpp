// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignSessionSubsystem.h"
#include "Campaign/CampaignSaveSubsystem.h"
#include "AssetRegistry/AssetData.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameMode.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Templates/UnrealTemplate.h"
#include "World/CampaignManager.h"
#include "World/WorldMapManager.h"
#include "Faction/FactionManager.h"

namespace RomanCampaignTravel
{
	FSoftObjectPath MapPath(const UWorld* World)
	{
		if (!World) return FSoftObjectPath();
		const FString Package = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
		return FSoftObjectPath(Package + TEXT(".") + FPackageName::GetShortName(Package));
	}
	bool IsWorldAsset(const FSoftObjectPath& Path)
	{
		const UAssetManager* Manager = UAssetManager::GetIfInitialized();
		FAssetData Metadata;
		return Manager && Path.IsValid() && Path.GetSubPathString().IsEmpty() &&
			Manager->GetAssetDataForPath(Path, Metadata) && Metadata.GetSoftObjectPath() == Path &&
			Metadata.AssetClassPath == UWorld::StaticClass()->GetClassPathName();
	}
}

void UCampaignSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine) TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UCampaignSessionSubsystem::HandleTravelFailure);
}

void UCampaignSessionSubsystem::Deinitialize()
{
	if (GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	bPending = false; bHasSession = false; bHasPendingSaveSettings = false; PendingSnapshot = FCampaignTransferSnapshot();
	SourceWorld.Reset(); SessionWorld.Reset(); FailedWorld.Reset();
	Super::Deinitialize();
}

bool UCampaignSessionSubsystem::HasSession() const
{
	return bHasSession && SessionWorld.IsValid() && SessionWorld.Get() == GetGameInstance()->GetWorld();
}

ECampaignSessionFailure UCampaignSessionSubsystem::EnterStrategicCampaign()
{
	if (bPending || bOperationActive) return ECampaignSessionFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	if (HasSession()) return ECampaignSessionFailure::WrongPhase;
	UWorld* World = GetGameInstance()->GetWorld();
	ARomanEmpireGameMode* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	if (!Mode || !Mode->IsGameplayInitialized() || !Campaign) return ECampaignSessionFailure::Unavailable;
	return QueueTransfer(ECampaignTransferPhase::Strategic, RomanCampaignTravel::MapPath(World), FSoftObjectPath(), Campaign->EncounterState);
}

ECampaignSessionFailure UCampaignSessionSubsystem::BeginTacticalEncounter(FGuid ArmyID, FName TargetTerritoryID)
{
	if (bPending || bOperationActive) return ECampaignSessionFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	if (!HasSession() || Phase != ECampaignTransferPhase::Strategic) return ECampaignSessionFailure::WrongPhase;
	UWorld* World = SessionWorld.Get();
	ARomanEmpireGameMode* Mode = World->GetAuthGameMode<ARomanEmpireGameMode>();
	ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanTerritoryDefinition* Territory = Registry ? Registry->FindTerritory(TargetTerritoryID) : nullptr;
	if (!Campaign) return ECampaignSessionFailure::Unavailable;
	if (!Territory || !Territory->HasValidTacticalMap()) return ECampaignSessionFailure::InvalidMap;
	FCampaignEncounterState Prepared;
	if (Campaign->PrepareEncounterForTravel(ArmyID, TargetTerritoryID, Prepared) != ECampaignEncounterFailure::None)
		return ECampaignSessionFailure::EncounterRejected;
	return QueueTransfer(ECampaignTransferPhase::TacticalDeployment, Territory->TacticalMap.ToSoftObjectPath(), StrategicMap, Prepared);
}

ECampaignSessionFailure UCampaignSessionSubsystem::BeginQueuedAITacticalEncounter()
{
	if (bPending || bOperationActive) return ECampaignSessionFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	if (!HasSession() || Phase != ECampaignTransferPhase::Strategic) return ECampaignSessionFailure::WrongPhase;
	UWorld* World = SessionWorld.Get();
	ARomanEmpireGameMode* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	if (!Campaign || !Campaign->bAwaitingAITacticalTravel || Campaign->ValidateCampaignAuthority() != ECampaignEncounterFailure::None)
		return ECampaignSessionFailure::Unavailable;
	const auto Plan = Campaign->EncounterState.GetPlan();
	const auto* Registry = URomanContentRegistrySubsystem::Get(this);
	const auto* Territory = Registry ? Registry->FindTerritory(Plan.TargetTerritoryID) : nullptr;
	if (!Territory || !Territory->HasValidTacticalMap()) return ECampaignSessionFailure::InvalidMap;
	FCampaignEncounterState Checked;
	if (!Campaign->EncounterState.HasPendingEncounter() || Plan.Attacker != EFactionID::Carthage || Plan.Defender != EFactionID::Rome ||
		Plan.PlayerFaction != Campaign->FactionManager->GetPlayerFaction() || Mode->GetCurrentGameplayMode() != EGameplayMode::Strategic ||
		!Campaign->ValidateQueuedAIEncounter(Checked) ||
		!Campaign->ValidateDeploymentContent(Plan)) return ECampaignSessionFailure::EncounterRejected;
	return QueueTransfer(ECampaignTransferPhase::TacticalDeployment, Territory->TacticalMap.ToSoftObjectPath(), StrategicMap, Checked);
}

ECampaignSessionFailure UCampaignSessionSubsystem::ReturnToStrategicCampaign()
{
	if (bPending || bOperationActive) return ECampaignSessionFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	if (!HasSession() || Phase != ECampaignTransferPhase::TacticalDeployment) return ECampaignSessionFailure::WrongPhase;
	UWorld* World = SessionWorld.Get();
	ARomanEmpireGameMode* Mode = World->GetAuthGameMode<ARomanEmpireGameMode>();
	ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	const UTacticalBattleSubsystem* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
	if (!Campaign || !Battle) return ECampaignSessionFailure::Unavailable;
	if (!Battle->IsResolved()) return ECampaignSessionFailure::Unresolved;
	if (!RomanCampaignTravel::IsWorldAsset(StrategicMap)) return ECampaignSessionFailure::InvalidMap;
	const ECampaignEncounterFailure Applied = Campaign->ApplyCurrentEncounterResult();
	if (Applied != ECampaignEncounterFailure::None && Applied != ECampaignEncounterFailure::AlreadyApplied)
		return ECampaignSessionFailure::EncounterRejected;
	return QueueTransfer(ECampaignTransferPhase::Strategic, StrategicMap, FSoftObjectPath(), Campaign->EncounterState);
}

ECampaignSessionFailure UCampaignSessionSubsystem::QueueTransfer(ECampaignTransferPhase TargetPhase, const FSoftObjectPath& Destination,
	const FSoftObjectPath& ReturnMap, const FCampaignEncounterState& Encounter)
{
	UWorld* World = GetGameInstance()->GetWorld();
	ARomanEmpireGameMode* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	if (!Campaign || !Mode->IsGameplayInitialized()) return ECampaignSessionFailure::Unavailable;
	if (!RomanCampaignTravel::IsWorldAsset(Destination) ||
		(TargetPhase == ECampaignTransferPhase::TacticalDeployment && !RomanCampaignTravel::IsWorldAsset(ReturnMap)))
		return ECampaignSessionFailure::InvalidMap;
	FCampaignTransferSnapshot Snapshot;
	LastTransferFailure = Campaign->CaptureForTravel(TargetPhase, Destination, ReturnMap, Encounter, Snapshot);
	if (LastTransferFailure != ECampaignTransferFailure::None)
	{
		LastFailure = ECampaignSessionFailure::TransferRejected;
		return LastFailure;
	}
	bHasPendingSaveSettings = false;
	PendingSnapshot = MoveTemp(Snapshot); TransferNonce = FGuid::NewGuid(); SourceWorld = World;
	bPending = true; LastFailure = ECampaignSessionFailure::None;
	const FString Options = TEXT("RomanTransfer=") + TransferNonce.ToString(EGuidFormats::Digits);
	UGameplayStatics::OpenLevel(World, FName(*Destination.GetLongPackageName()), true, Options);
	OnSessionChanged.Broadcast();
	return ECampaignSessionFailure::None;
}

ECampaignSessionFailure UCampaignSessionSubsystem::QueueSavedCampaign(const FCampaignSaveRecord& Record)
{
	if (bPending || bOperationActive) return ECampaignSessionFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	UWorld* World = GetGameInstance()->GetWorld();
	auto* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	auto* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	if (!Campaign || !Mode->IsGameplayInitialized()) return ECampaignSessionFailure::Unavailable;
	if (HasSession() && Phase != ECampaignTransferPhase::Strategic) return ECampaignSessionFailure::WrongPhase;
	if (Record.Campaign.Phase != ECampaignTransferPhase::Strategic || !RomanCampaignTravel::IsWorldAsset(Record.Campaign.DestinationMap))
		return ECampaignSessionFailure::InvalidMap;
	LastTransferFailure = Campaign->ValidateSavedCampaign(Record.Campaign);
	if (LastTransferFailure != ECampaignTransferFailure::None) return ECampaignSessionFailure::TransferRejected;
	if (Record.Campaign.Turn.bAwaitingAITacticalTravel)
	{
		const auto* Registry = URomanContentRegistrySubsystem::Get(this);
		const auto* Target = Registry ? Registry->FindTerritory(Record.Campaign.Encounter.PendingPlan.TargetTerritoryID) : nullptr;
		if (!Target || !Target->HasValidTacticalMap() || !RomanCampaignTravel::IsWorldAsset(Target->TacticalMap.ToSoftObjectPath()))
			return ECampaignSessionFailure::InvalidMap;
	}
	// Explicit load abandons the source only AFTER validating the complete destination.
	// Existing receipt preflight/refunds and freeze are reused, never used to capture a save.
	FCampaignTransferSnapshot DiscardedSource;
	LastTransferFailure = Campaign->CaptureForTravel(ECampaignTransferPhase::Strategic, Record.Campaign.DestinationMap,
		FSoftObjectPath(), Campaign->EncounterState, DiscardedSource);
	if (LastTransferFailure != ECampaignTransferFailure::None) return ECampaignSessionFailure::TransferRejected;
	PendingSnapshot = Record.Campaign; PendingSaveSettings = Record.Settings; bHasPendingSaveSettings = true;
	TransferNonce = FGuid::NewGuid(); SourceWorld = World; bPending = true; LastFailure = ECampaignSessionFailure::None;
	UGameplayStatics::OpenLevel(World, FName(*PendingSnapshot.DestinationMap.GetLongPackageName()), true,
		TEXT("RomanTransfer=") + TransferNonce.ToString(EGuidFormats::Digits));
	OnSessionChanged.Broadcast(); return ECampaignSessionFailure::None;
}

bool UCampaignSessionSubsystem::CanRetryPendingTransfer() const
{
	const UWorld* World = GetGameInstance()->GetWorld();
	return !bOperationActive && bPending && LastFailure == ECampaignSessionFailure::RestoreFailed &&
		World && World == FailedWorld.Get() && RomanCampaignTravel::IsWorldAsset(PendingSnapshot.DestinationMap);
}

ECampaignSessionFailure UCampaignSessionSubsystem::RetryPendingTransfer()
{
	if (bOperationActive) return ECampaignSessionFailure::Busy;
	TGuardValue<bool> Guard(bOperationActive, true);
	UWorld* World = GetGameInstance()->GetWorld();
	if (!bPending || LastFailure != ECampaignSessionFailure::RestoreFailed || !World || World != FailedWorld.Get())
		return ECampaignSessionFailure::WrongPhase;
	if (!RomanCampaignTravel::IsWorldAsset(PendingSnapshot.DestinationMap)) return ECampaignSessionFailure::InvalidMap;
	SourceWorld = World; TransferNonce = FGuid::NewGuid(); LastFailure = ECampaignSessionFailure::None;
	UGameplayStatics::OpenLevel(World, FName(*PendingSnapshot.DestinationMap.GetLongPackageName()), true,
		TEXT("RomanTransfer=") + TransferNonce.ToString(EGuidFormats::Digits));
	OnSessionChanged.Broadcast();
	return ECampaignSessionFailure::None;
}

bool UCampaignSessionSubsystem::ExpectsIncomingWorld(const UWorld* World) const
{
	return bPending && World && World != SourceWorld.Get() && World->GetGameInstance() == GetGameInstance();
}

bool UCampaignSessionSubsystem::MatchesIncomingWorld(const UWorld* World, const FString& Options) const
{
	return bPending && World && World != SourceWorld.Get() && World->GetGameInstance() == GetGameInstance() &&
		RomanCampaignTravel::MapPath(World) == PendingSnapshot.DestinationMap &&
		UGameplayStatics::ParseOption(Options, TEXT("RomanTransfer")) == TransferNonce.ToString(EGuidFormats::Digits);
}

bool UCampaignSessionSubsystem::RestoreIncomingWorld(UWorld* World, const FString& Options)
{
	if (!ExpectsIncomingWorld(World)) return false;
	TGuardValue<bool> Guard(bOperationActive, true);
	ARomanEmpireGameMode* Mode = World->GetAuthGameMode<ARomanEmpireGameMode>();
	ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr;
	// An unrelated OpenLevel/nonce cannot silently consume the capsule or publish a
	// fresh default campaign. Keep the exact payload and offer an explicit retry.
	FailedWorld = World;
	if (!MatchesIncomingWorld(World, Options))
	{
		if (Campaign) Campaign->FreezeForTravel();
		LastFailure = ECampaignSessionFailure::RestoreFailed;
		LastTransferFailure = ECampaignTransferFailure::InvalidMapReference;
		OnSessionChanged.Broadcast();
		return false;
	}
	if (!Campaign || !RomanCampaignTravel::IsWorldAsset(PendingSnapshot.DestinationMap) ||
		(PendingSnapshot.Phase == ECampaignTransferPhase::TacticalDeployment && !RomanCampaignTravel::IsWorldAsset(PendingSnapshot.StrategicReturnMap)))
	{
		if (Campaign) Campaign->FreezeForTravel();
		LastFailure = ECampaignSessionFailure::RestoreFailed;
		LastTransferFailure = ECampaignTransferFailure::InvalidMapReference;
		OnSessionChanged.Broadcast();
		return false;
	}
	if (PendingSnapshot.Phase == ECampaignTransferPhase::TacticalDeployment)
	{
		const auto* Registry = URomanContentRegistrySubsystem::Get(this);
		const auto* Territory = Registry ? Registry->FindTerritory(PendingSnapshot.Encounter.PendingPlan.TargetTerritoryID) : nullptr;
		if (!Territory || !Territory->HasValidTacticalMap() || Territory->TacticalMap.ToSoftObjectPath() != PendingSnapshot.DestinationMap)
		{
			Campaign->FreezeForTravel(); LastFailure = ECampaignSessionFailure::RestoreFailed;
			LastTransferFailure = ECampaignTransferFailure::InvalidMapReference;
			OnSessionChanged.Broadcast();
			return false;
		}
	}
	LastTransferFailure = Campaign->RestoreFromTravel(PendingSnapshot);
	if (LastTransferFailure != ECampaignTransferFailure::None)
	{
		Campaign->FreezeForTravel();
		LastFailure = ECampaignSessionFailure::RestoreFailed;
		OnSessionChanged.Broadcast();
		return false; // Keep capsule for diagnosis/recovery; NEVER publish default campaign as restored.
	}
	if (bHasPendingSaveSettings)
	{
		if (auto* Save = GetGameInstance()->GetSubsystem<UCampaignSaveSubsystem>()) Save->ApplyLoadedSettings(PendingSaveSettings);
		bHasPendingSaveSettings = false;
	}
	Phase = PendingSnapshot.Phase;
	StrategicMap = Phase == ECampaignTransferPhase::Strategic ? PendingSnapshot.DestinationMap : PendingSnapshot.StrategicReturnMap;
	SessionWorld = World; bHasSession = true; bPending = false;
	PendingSnapshot = FCampaignTransferSnapshot(); SourceWorld.Reset(); FailedWorld.Reset(); TransferNonce.Invalidate();
	LastFailure = ECampaignSessionFailure::None;
	return true; // GameMode publishes readiness only after configuring the camera/mode.
}

void UCampaignSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type Failure, const FString& Reason)
{
	if (!bPending || !World || World != SourceWorld.Get() || World->GetGameInstance() != GetGameInstance()) return;
	if (World == FailedWorld.Get())
	{
		LastFailure = ECampaignSessionFailure::RestoreFailed;
		OnSessionChanged.Broadcast();
		return; // Retry failure retains the only recoverable capsule and quarantined world.
	}
	ARomanEmpireGameMode* Mode = World->GetAuthGameMode<ARomanEmpireGameMode>();
	if (ACampaignManager* Campaign = Mode ? Mode->GetCampaignManager() : nullptr)
	{
		Campaign->CancelTravelFreeze();
		const auto Plan = Campaign->EncounterState.GetPlan();
		if (PendingSnapshot.Phase == ECampaignTransferPhase::TacticalDeployment && Plan.EncounterID == PendingSnapshot.Encounter.PendingPlan.EncounterID &&
			Campaign->EncounterState.HasPendingEncounter() && Plan.Attacker != Plan.PlayerFaction) Campaign->bAwaitingAITacticalTravel = true;
	}
	bPending = false; bHasPendingSaveSettings = false; PendingSnapshot = FCampaignTransferSnapshot(); SourceWorld.Reset(); TransferNonce.Invalidate();
	LastFailure = ECampaignSessionFailure::RestoreFailed;
	OnSessionChanged.Broadcast();
}
