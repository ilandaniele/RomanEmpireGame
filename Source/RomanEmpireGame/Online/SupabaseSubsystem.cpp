// Copyright Roman Empire Game. All Rights Reserved.

#include "SupabaseSubsystem.h"
#include "../../RomanEmpireGame.h"
#include "Misc/ConfigCacheIni.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

void USupabaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GConfig->GetString(TEXT("Supabase"), TEXT("URL"),     SupabaseURL, GGameIni);
	GConfig->GetString(TEXT("Supabase"), TEXT("AnonKey"), AnonKey,     GGameIni);

	if (SupabaseURL.IsEmpty())
	{
		UE_LOG(LogRomanEmpireGame, Warning,
			TEXT("USupabaseSubsystem: No URL configured. Add [Supabase] URL=... AnonKey=... to DefaultGame.ini"));
	}
	else
	{
		UE_LOG(LogRomanEmpireGame, Log, TEXT("USupabaseSubsystem initialized: %s"), *SupabaseURL);
	}
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> USupabaseSubsystem::MakeRequest(
	const FString& Endpoint, const FString& Method, const FString& Body)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(SupabaseURL + TEXT("/rest/v1/") + Endpoint);
	Req->SetVerb(Method);
	Req->SetHeader(TEXT("apikey"), AnonKey);
	Req->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + AnonKey);
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetHeader(TEXT("Prefer"), TEXT("return=representation"));
	if (!Body.IsEmpty()) Req->SetContentAsString(Body);
	return Req;
}

void USupabaseSubsystem::SaveGameOnline(const FString& PlayerID, const FString& SaveDataJson, FOnSaveResult OnResult)
{
	if (!IsConfigured()) { OnResult.ExecuteIfBound(false); return; }

	FString Body = FString::Printf(TEXT("{\"player_id\":\"%s\",\"save_data\":%s}"), *PlayerID, *SaveDataJson);
	auto Req = MakeRequest(
		FString::Printf(TEXT("saves?player_id=eq.%s"), *PlayerID),
		TEXT("POST"), Body);

	Req->OnProcessRequestComplete().BindLambda(
		[OnResult](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{ OnResult.ExecuteIfBound(bOk && Res && Res->GetResponseCode() < 300); });
	Req->ProcessRequest();
}

void USupabaseSubsystem::LoadGameOnline(const FString& PlayerID, FOnSaveLoaded OnLoaded)
{
	if (!IsConfigured()) { OnLoaded.ExecuteIfBound(TEXT("")); return; }

	auto Req = MakeRequest(
		FString::Printf(TEXT("saves?player_id=eq.%s&order=updated_at.desc&limit=1"), *PlayerID),
		TEXT("GET"));
	Req->OnProcessRequestComplete().BindLambda(
		[OnLoaded](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{ if (bOk && Res) OnLoaded.ExecuteIfBound(Res->GetContentAsString()); });
	Req->ProcessRequest();
}

void USupabaseSubsystem::SubmitScore(const FString& PlayerID, const FString& PlayerName,
	int32 TurnsPlayed, int32 TerritoriesConquered, const FString& Faction, FOnSaveResult OnResult)
{
	if (!IsConfigured()) { OnResult.ExecuteIfBound(false); return; }

	FString Body = FString::Printf(
		TEXT("{\"player_id\":\"%s\",\"player_name\":\"%s\",\"turns_played\":%d,\"territories_conquered\":%d,\"faction\":\"%s\"}"),
		*PlayerID, *PlayerName, TurnsPlayed, TerritoriesConquered, *Faction);

	auto Req = MakeRequest(TEXT("leaderboard"), TEXT("POST"), Body);
	Req->OnProcessRequestComplete().BindLambda(
		[OnResult](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{ OnResult.ExecuteIfBound(bOk && Res && Res->GetResponseCode() < 300); });
	Req->ProcessRequest();
}

void USupabaseSubsystem::FetchLeaderboard(FOnLeaderboardFetched OnFetched)
{
	if (!IsConfigured()) { TArray<FLeaderboardEntry> E; OnFetched.ExecuteIfBound(E); return; }

	auto Req = MakeRequest(TEXT("leaderboard?order=territories_conquered.desc&limit=10"), TEXT("GET"));
	Req->OnProcessRequestComplete().BindLambda(
		[OnFetched](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			TArray<FLeaderboardEntry> Entries;
			if (!bOk || !Res) { OnFetched.ExecuteIfBound(Entries); return; }

			TArray<TSharedPtr<FJsonValue>> JsonArray;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());
			if (FJsonSerializer::Deserialize(Reader, JsonArray))
			{
				for (auto& Val : JsonArray)
				{
					TSharedPtr<FJsonObject> Obj = Val->AsObject();
					FLeaderboardEntry E;
					E.PlayerID             = Obj->GetStringField(TEXT("player_id"));
					E.PlayerName           = Obj->GetStringField(TEXT("player_name"));
					E.TurnsPlayed          = (int32)Obj->GetNumberField(TEXT("turns_played"));
					E.TerritoriesConquered = (int32)Obj->GetNumberField(TEXT("territories_conquered"));
					E.Faction              = Obj->GetStringField(TEXT("faction"));
					Entries.Add(E);
				}
			}
			OnFetched.ExecuteIfBound(Entries);
		});
	Req->ProcessRequest();
}
