// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignTransfer.h"
#include "GameFramework/SaveGame.h"
#include "CampaignSave.generated.h"

UENUM(BlueprintType)
enum class ECampaignSaveSlot : uint8 { Manual, Autosave };

UENUM(BlueprintType)
enum class ECampaignSaveFailure : uint8
{
	None, Unavailable, Busy, UnsafeBoundary, NotFound, IOError, UnsupportedVersion,
	CorruptData, InvalidCampaign, InvalidContent, LoadRejected
};

/** Persistence preferences only; input/audio/display profiles remain their own domains. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignSaveSettings
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Save") bool bAutosaveEnabled = true;
};

/** Detached saved value, never another live campaign authority or an actor graph. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignSaveRecord
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Save") int64 Generation = 1;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Save") FCampaignSaveSettings Settings;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Save") FCampaignTransferSnapshot Campaign;
};

UCLASS()
class ROMANEMPIREGAME_API URomanCampaignSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Save") FCampaignSaveRecord Record;
};

namespace RomanCampaignSave
{
	inline constexpr uint32 Magic = 0x52454753;
	inline constexpr uint32 CurrentVersion = 1;
	inline constexpr int32 HeaderBytes = 16;
	inline constexpr int32 MaxPayloadBytes = 4 * 1024 * 1024;
	/** Fixed magic/version/length/CRC envelope around the existing reflected value archive.
	 * CRC detects accidental corruption; it is NOT authentication. No class loading.
	 * Decode is atomic. Domain/catalog/content validation is still required before load. */
	ROMANEMPIREGAME_API ECampaignSaveFailure Encode(const FCampaignSaveRecord& Record, TArray<uint8>& OutBytes);
	ROMANEMPIREGAME_API ECampaignSaveFailure Decode(const TArray<uint8>& Bytes, FCampaignSaveRecord& OutRecord);
}
