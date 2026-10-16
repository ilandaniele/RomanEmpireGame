// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignSave.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Class.h"

namespace
{
	ECampaignSaveFailure ValidateRecord(const FCampaignSaveRecord& Record)
	{
		if (Record.Campaign.SchemaVersion != FCampaignTransferSnapshot::CurrentSchemaVersion) return ECampaignSaveFailure::UnsupportedVersion;
		if (Record.Campaign.Phase != ECampaignTransferPhase::Strategic) return ECampaignSaveFailure::UnsafeBoundary;
		if (Record.Generation < 1 || Record.Generation == MAX_int64 || !Record.Campaign.Turn.IsValid() ||
			Record.Campaign.DestinationMap.IsNull() || !Record.Campaign.StrategicReturnMap.IsNull()) return ECampaignSaveFailure::InvalidCampaign;
		return ECampaignSaveFailure::None;
	}
	class FSaveValueReader final : public FObjectAndNameAsStringProxyArchive
	{
	public:
		explicit FSaveValueReader(FArchive& Inner, int32 Size) : FObjectAndNameAsStringProxyArchive(Inner, false)
		{ ArIsSaveGame = true; ArMaxSerializeSize = Size; }
	};
}

ECampaignSaveFailure RomanCampaignSave::Encode(const FCampaignSaveRecord& Record, TArray<uint8>& OutBytes)
{
	const auto Failure = ValidateRecord(Record);
	if (Failure != ECampaignSaveFailure::None) return Failure;
	TArray<uint8> Payload;
	{
		FCampaignSaveRecord Detached = Record;
		FMemoryWriter Writer(Payload, true); FObjectAndNameAsStringProxyArchive Archive(Writer, false); Archive.ArIsSaveGame = true;
		FCampaignSaveRecord::StaticStruct()->SerializeItem(Archive, &Detached, nullptr);
		if (Archive.IsError() || Payload.IsEmpty() || Payload.Num() > MaxPayloadBytes) return ECampaignSaveFailure::CorruptData;
	}
	TArray<uint8> Candidate;
	FMemoryWriter Writer(Candidate, true);
	uint32 FileMagic = Magic, Version = CurrentVersion, CRC = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	int32 Size = Payload.Num(); Writer << FileMagic << Version << Size << CRC;
	Writer.Serialize(Payload.GetData(), Size);
	if (Writer.IsError()) return ECampaignSaveFailure::CorruptData;
	OutBytes = MoveTemp(Candidate); return ECampaignSaveFailure::None;
}

ECampaignSaveFailure RomanCampaignSave::Decode(const TArray<uint8>& Bytes, FCampaignSaveRecord& OutRecord)
{
	if (Bytes.Num() < HeaderBytes || Bytes.Num() > HeaderBytes + MaxPayloadBytes) return ECampaignSaveFailure::CorruptData;
	FMemoryReader Reader(Bytes, true);
	uint32 FileMagic = 0, Version = 0, CRC = 0; int32 Size = 0;
	Reader << FileMagic << Version << Size << CRC;
	if (Reader.IsError() || FileMagic != Magic) return ECampaignSaveFailure::CorruptData;
	if (Version != CurrentVersion) return ECampaignSaveFailure::UnsupportedVersion; // Explicit invalidation, no silent migration.
	if (Size < 1 || Size > MaxPayloadBytes || Size != Bytes.Num() - HeaderBytes ||
		CRC != FCrc::MemCrc32(Bytes.GetData() + HeaderBytes, Size)) return ECampaignSaveFailure::CorruptData;
	TArray<uint8> Payload; Payload.Append(Bytes.GetData() + HeaderBytes, Size);
	FMemoryReader ValueReader(Payload, true); FSaveValueReader Archive(ValueReader, Size);
	FCampaignSaveRecord Candidate;
	FCampaignSaveRecord::StaticStruct()->SerializeItem(Archive, &Candidate, nullptr);
	if (Archive.IsError() || !ValueReader.AtEnd()) return ECampaignSaveFailure::CorruptData;
	const auto Failure = ValidateRecord(Candidate);
	if (Failure != ECampaignSaveFailure::None) return Failure;
	OutRecord = MoveTemp(Candidate); return ECampaignSaveFailure::None;
}
