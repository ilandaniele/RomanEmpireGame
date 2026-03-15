// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "SupabaseSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FLeaderboardEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard") FString PlayerID;
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard") FString PlayerName;
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard") int32 TurnsPlayed = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard") int32 TerritoriesConquered = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard") FString Faction;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLeaderboardFetched, const TArray<FLeaderboardEntry>&, Entries);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSaveLoaded, const FString&, SaveDataJson);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSaveResult, bool, bSuccess);

/**
 * USupabaseSubsystem — HTTP wrapper over Supabase REST API.
 *
 * Config in Config/DefaultGame.ini:
 *   [Supabase]
 *   URL=https://YOUR_PROJECT.supabase.co
 *   AnonKey=YOUR_ANON_KEY
 *
 * SQL tables to create in Supabase dashboard:
 *   create table saves (
 *     id uuid primary key default gen_random_uuid(),
 *     player_id text not null,
 *     save_data jsonb not null,
 *     updated_at timestamptz default now()
 *   );
 *   create table leaderboard (
 *     id uuid primary key default gen_random_uuid(),
 *     player_id text not null,
 *     player_name text,
 *     turns_played int,
 *     territories_conquered int,
 *     faction text,
 *     created_at timestamptz default now()
 *   );
 */
UCLASS()
class ROMANEMPIREGAME_API USupabaseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Online|Save")
	void SaveGameOnline(const FString& PlayerID, const FString& SaveDataJson, FOnSaveResult OnResult);

	UFUNCTION(BlueprintCallable, Category = "Online|Save")
	void LoadGameOnline(const FString& PlayerID, FOnSaveLoaded OnLoaded);

	UFUNCTION(BlueprintCallable, Category = "Online|Leaderboard")
	void SubmitScore(const FString& PlayerID, const FString& PlayerName,
		int32 TurnsPlayed, int32 TerritoriesConquered, const FString& Faction, FOnSaveResult OnResult);

	UFUNCTION(BlueprintCallable, Category = "Online|Leaderboard")
	void FetchLeaderboard(FOnLeaderboardFetched OnFetched);

	UFUNCTION(BlueprintPure, Category = "Online")
	bool IsConfigured() const { return !SupabaseURL.IsEmpty() && !AnonKey.IsEmpty(); }

private:
	FString SupabaseURL;
	FString AnonKey;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MakeRequest(
		const FString& Endpoint, const FString& Method, const FString& Body = TEXT(""));
};
