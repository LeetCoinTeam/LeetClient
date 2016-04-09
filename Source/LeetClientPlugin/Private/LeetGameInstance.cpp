// Fill out your copyright notice in the Description page of Project Settings.

#include "LeetClientPluginPrivatePCH.h"
//#include "LeetGameInstance.h"

namespace LeetGameInstanceState
{
	const FName None = FName(TEXT("None"));
	const FName PendingInvite = FName(TEXT("PendingInvite"));
	const FName WelcomeScreen = FName(TEXT("WelcomeScreen"));
	const FName MainMenu = FName(TEXT("MainMenu"));
	const FName MessageMenu = FName(TEXT("MessageMenu"));
	const FName Playing = FName(TEXT("Playing"));
}

ULeetGameInstance::ULeetGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsOnline(true) // Default to online
	, bIsLicensed(true) // Default to licensed (should have been checked by OS on boot)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE CONSTRUCTOR"));
	CurrentState = LeetGameInstanceState::None;

	ServerSessionHostAddress = NULL;
	ServerSessionID = NULL;

	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE INIT"));

	_configPath = FPaths::SourceConfigDir();
	_configPath += TEXT("LeetConfig.ini");

	UE_LOG(LogTemp, Log, TEXT("[LEET] set up path"));

	if (FPaths::FileExists(_configPath))
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET]File Exists"));
		FConfigSection* Configs = GConfig->GetSectionPrivate(TEXT("Leet.Client"), false, true, _configPath);
		if (Configs)
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] Configs Exist"));
			FString test = *Configs->Find(TEXT("APIURL"));

			APIURL = *Configs->Find(TEXT("APIURL"));
			ServerAPISecret = *Configs->Find(TEXT("ServerSecret"));
			ServerAPIKey = *Configs->Find(TEXT("ServerAPIKey"));
			GameKey = *Configs->Find(TEXT("GameKey"));

		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Could not find Leet.Client in LeetConfig.ini"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Could not find LeetConfig.ini, must Initialize manually!"));
	}

	// I don't think we want this here.
	//GetServerInfo();
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE CONSTRUCTOR - DONE"));
}

void ULeetGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE INIT"));

	//IgnorePairingChangeForControllerId = -1;
	CurrentConnectionStatus = EOnlineServerConnectionStatus::Connected;

	// game requires the ability to ID users.
	const auto OnlineSub = IOnlineSubsystem::Get();
	check(OnlineSub);
	const auto IdentityInterface = OnlineSub->GetIdentityInterface();
	check(IdentityInterface.IsValid());

	const auto SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	// bind any OSS delegates we needs to handle
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(i, FOnLoginStatusChangedDelegate::CreateUObject(this, &ULeetGameInstance::HandleUserLoginChanged));
	}

	SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &ULeetGameInstance::HandleSessionFailure));

	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateUObject(this, &ULeetGameInstance::OnEndSessionComplete);
	//OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ULeetGameInstance::OnCreateSessionComplete);

}

ALeetGameSession* ULeetGameInstance::GetGameSession() const
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::GetGameSession"));
	UWorld* const World = GetWorld();
	if (World)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] World Found"));
		AGameMode* const Game = World->GetAuthGameMode();
		if (Game)
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] Game Found"));
			return Cast<ALeetGameSession>(Game->GameSession);
		}
	}

	return nullptr;
}

// prototype http function
bool ULeetGameInstance::PerformHttpRequest(void(ULeetGameInstance::*delegateCallback)(FHttpRequestPtr, FHttpResponsePtr, bool), FString APIURI, FString ArgumentString)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] PerformHttpRequest"));

	FHttpModule* Http = &FHttpModule::Get();
	if (!Http) { return false; }
	if (!Http->IsHttpEnabled()) { return false; }

	FString TargetHost = "http://" + APIURL + APIURI;

	UE_LOG(LogTemp, Log, TEXT("TargetHost: %s"), *TargetHost);
	UE_LOG(LogTemp, Log, TEXT("ServerAPIKey: %s"), *ServerAPIKey);
	UE_LOG(LogTemp, Log, TEXT("ServerAPISecret: %s"), *ServerAPISecret);

	TSharedRef < IHttpRequest > Request = Http->CreateRequest();
	Request->SetVerb("POST");
	Request->SetURL(TargetHost);
	Request->SetHeader("User-Agent", "LEET_UE4_API_CLIENT/1.0");
	Request->SetHeader("Content-Type", "application/x-www-form-urlencoded");
	Request->SetHeader("Key", ServerAPIKey);
	Request->SetHeader("Sign", "RealSignatureComingIn411");
	Request->SetContentAsString(ArgumentString);

	Request->OnProcessRequestComplete().BindUObject(this, delegateCallback);
	if (!Request->ProcessRequest()) { return false; }

	return true;
}

bool ULeetGameInstance::GetServerInfo()
{

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] GetServerInfo"));

	FString nonceString = "10951350917635";
	FString encryption = "off";  // Allowing unencrypted on sandbox for now.  

	FString OutputString = "nonce=" + nonceString + "&encryption=" + encryption;

	UE_LOG(LogTemp, Log, TEXT("ServerSessionHostAddress: %s"), *ServerSessionHostAddress);
	UE_LOG(LogTemp, Log, TEXT("ServerSessionID: %s"), *ServerSessionID);

	if (ServerSessionHostAddress.Len() > 1) {
		OutputString = OutputString + "&session_host_address=" + ServerSessionHostAddress + "&session_id=" + ServerSessionID;
	}

	FString APIURI = "/api/v2/server/info";

	bool requestSuccess = PerformHttpRequest(&ULeetGameInstance::GetServerInfoComplete, APIURI, OutputString);

	return requestSuccess;
}

void ULeetGameInstance::GetServerInfoComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Test failed. NULL response"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Completed test [%s] Url=[%s] Response=[%d] [%s]"),
			*HttpRequest->GetVerb(),
			*HttpRequest->GetURL(),
			HttpResponse->GetResponseCode(),
			*HttpResponse->GetContentAsString());

		FString JsonRaw = *HttpResponse->GetContentAsString();
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			bool Authorization = JsonParsed->GetBoolField("authorization");
			UE_LOG(LogTemp, Log, TEXT("Authorization"));
			if (Authorization)
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization True"));
				// Set up our instance variables
				if (JsonParsed->GetIntegerField("incrementBTC")) {
					incrementBTC = JsonParsed->GetIntegerField("incrementBTC");
				}
				if (JsonParsed->GetIntegerField("minimumBTCHold")) {
					minimumBTCHold = JsonParsed->GetIntegerField("minimumBTCHold");
				}
				if (JsonParsed->GetNumberField("serverRakeBTCPercentage")) {
					serverRakeBTCPercentage = JsonParsed->GetNumberField("serverRakeBTCPercentage");
				}
				if (JsonParsed->GetNumberField("leetcoinRakePercentage")) {
					leetRakePercentage = JsonParsed->GetNumberField("leetcoinRakePercentage");
				}
				if (incrementBTC && serverRakeBTCPercentage && leetRakePercentage) {
					killRewardBTC = incrementBTC - ((incrementBTC * serverRakeBTCPercentage) + (incrementBTC * leetRakePercentage));
				}
				
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization False"));
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [GetServerInfoComplete] Done!"));
}

bool ULeetGameInstance::ActivatePlayer(FString PlatformID, int32 playerID)
{

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] ActivatePlayer"));
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] DEBUG TEST"));

	// check to see if this player is in the active list already
	bool platformIDFound = false;
	int32 ActivePlayerIndex = 0;
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] check to see if this player is in the active list already"));

	// this is just a test to make sure the get by playerid is working.
	//  TODO refactor the below to use this.
	FLeetActivePlayer* CurrentActivePlayer = getPlayerByPlayerId(playerID);

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] ActivePlayers.Num() > 0"));
	for (int32 b = 0; b < ActivePlayers.Num(); b++)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [AuthorizePlayer] platformID: %s"), *ActivePlayers[b].platformID);
		if (ActivePlayers[b].platformID == PlatformID) {
			UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] AuthorizePlayer - FOUND MATCHING platformID"));
			platformIDFound = true;
			ActivePlayerIndex = b;
		}
	}

	if (platformIDFound == false) {
		UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] AuthorizePlayer - No existing platformID found"));

		// add the player to the TArray as authorized=false
		FLeetActivePlayer activeplayer;
		activeplayer.playerID = playerID;
		activeplayer.platformID = PlatformID;
		activeplayer.authorized = false;
		activeplayer.roundDeaths = 0;
		activeplayer.roundKills = 0;

		ActivePlayers.Add(activeplayer);

		UE_LOG(LogTemp, Log, TEXT("PlatformID: %s"), *PlatformID);
		UE_LOG(LogTemp, Log, TEXT("Object is: %s"), *GetName());


		FString nonceString = "10951350917635";
		FString encryption = "off";  // Allowing unencrypted on sandbox for now.  

		FString OutputString = "nonce=" + nonceString + "&encryption=" + encryption;

		UE_LOG(LogTemp, Log, TEXT("ServerSessionHostAddress: %s"), *ServerSessionHostAddress);
		UE_LOG(LogTemp, Log, TEXT("ServerSessionID: %s"), *ServerSessionID);

		if (ServerSessionHostAddress.Len() > 1) {
			OutputString = OutputString + "&session_host_address=" + ServerSessionHostAddress + "&session_id=" + ServerSessionID;
		}

		FString APIURI = "/api/v2/player/" + PlatformID + "/activate";;

		bool requestSuccess = PerformHttpRequest(&ULeetGameInstance::ActivateRequestComplete, APIURI, OutputString);

		return requestSuccess;
		}
	else {
		UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] AuthorizePlayer - TODO update record"));
		return true;
	}

}

void ULeetGameInstance::ActivateRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Test failed. NULL response"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Completed test [%s] Url=[%s] Response=[%d] [%s]"),
			*HttpRequest->GetVerb(),
			*HttpRequest->GetURL(),
			HttpResponse->GetResponseCode(),
			*HttpResponse->GetContentAsString());

		APlayerController* pc = NULL;
		int32 playerstateID;

		FString JsonRaw = *HttpResponse->GetContentAsString();
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			bool Authorization = JsonParsed->GetBoolField("authorization");
			UE_LOG(LogTemp, Log, TEXT("Authorization"));
			if (Authorization)
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization True"));
				bool PlayerAuthorized = JsonParsed->GetBoolField("player_authorized");
				if (PlayerAuthorized) {
					UE_LOG(LogTemp, Log, TEXT("Player Authorized"));

					int32 activeAuthorizedPlayers = 0;
					int32 activePlayerIndex;

					UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] ActivePlayers.Num() > 0"));
					for (int32 b = 0; b < ActivePlayers.Num(); b++)
					{
						UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] platformID: %s"), *ActivePlayers[b].platformID);
						if (ActivePlayers[b].platformID == JsonParsed->GetStringField("player_platformid")) {
							UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] - FOUND MATCHING platformID"));
							activePlayerIndex = b;
							ActivePlayers[b].authorized = true;
							ActivePlayers[b].playerTitle = JsonParsed->GetStringField("player_name");
							ActivePlayers[b].playerKey = JsonParsed->GetStringField("player_key");
							ActivePlayers[b].BTCHold = JsonParsed->GetIntegerField("player_btchold");
							ActivePlayers[b].Rank = JsonParsed->GetIntegerField("player_rank");

						}
						if (ActivePlayers[b].authorized) {
							activeAuthorizedPlayers++;
						}

					}

					// ALso set this player state playerName

					for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
					{
						UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] - Looking for player to set name"));
						pc = Iterator->Get();
						/*
						
						AMyPlayerController* thisPlayerController = Cast<AMyPlayerController>(pc);
						if (thisPlayerController) {
							UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] [ActivateRequestComplete] - Cast Controller success"));

							if (matchStarted) {
								UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] [ActivateRequestComplete] - Match in progress - setting spectator"));
								thisPlayerController->PlayerState->bIsSpectator = true;
								thisPlayerController->ChangeState(NAME_Spectating);
								thisPlayerController->ClientGotoState(NAME_Spectating);
							}
							playerstateID = thisPlayerController->PlayerState->PlayerId;
							if (ActivePlayers[activePlayerIndex].playerID == playerstateID)
							{
								UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] [ActivateRequestComplete] - playerID match - setting name"));
								thisPlayerController->PlayerState->SetPlayerName(JsonParsed->GetStringField("player_name"));
							}
						}
						*/
					}

					/*
					if (activeAuthorizedPlayers >= MinimumPlayersNeededToStart)
					{
						matchStarted = true;
						// travel to the third person map
						FString UrlString = TEXT("/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap?listen");
						GetWorld()->GetAuthGameMode()->bUseSeamlessTravel = true;
						GetWorld()->ServerTravel(UrlString);
					}
					*/

					

				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("Player NOT Authorized"));

					// First grab the active player data from our struct
					int32 activePlayerIndex;
					bool platformIDFound = false;
					FString jsonPlatformID = JsonParsed->GetStringField("player_platformid");

					for (int32 b = 0; b < ActivePlayers.Num(); b++)
					{
						UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] platformID: %s"), *ActivePlayers[b].platformID);
						UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] jsonPlatformID: %s"), *jsonPlatformID);
						if (ActivePlayers[b].platformID == jsonPlatformID) {
							UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] - Found active player record"));
							platformIDFound = true;
							activePlayerIndex = b;
						}
					}




					if (platformIDFound)
					{
						UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] - PlatformID is found - moving to kick"));
						for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
						{
							UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] - Looking for player to kick"));
							pc = Iterator->Get();

							playerstateID = pc->PlayerState->PlayerId;
							if (ActivePlayers[activePlayerIndex].playerID == playerstateID)
							{
								UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] - playerID match - kicking back to connect"));
								//FString UrlString = TEXT("/Game/MyConnectLevel");
								//ETravelType seamlesstravel = TRAVEL_Absolute;
								//thisPlayerController->ClientTravel(UrlString, seamlesstravel);
								// trying a session kick instead.
								// Get the Online Subsystem to work with
								IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
								const FString kickReason = TEXT("Not Authorized");
								const FText kickReasonText = FText::FromString(kickReason);
								//ALeetGameSession::KickPlayer(pc, kickReasonText);
								this->GetGameSession()->KickPlayer(pc, kickReasonText);

							}
							
						}
					}
				}
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [ActivateRequestComplete] Done!"));
}


bool ULeetGameInstance::DeActivatePlayer(int32 playerID)
{

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] DeActivatePlayer"));
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] DEBUG TEST"));

	// check to see if this player is in the active list already
	bool playerIDFound = false;
	int32 ActivePlayerIndex = 0;
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] check to see if this player is in the active list already"));

	//UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] [AuthorizePlayer] ActivePlayers: %i"), ActivePlayers);

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] ActivePlayers.Num() > 0"));
	for (int32 b = 0; b < ActivePlayers.Num(); b++)
	{
		//UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] [DeAuthorizePlayer] playerID: %s"), ActivePlayers[b].playerID);
		if (ActivePlayers[b].playerID == playerID) {
			UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] DeActivatePlayer - FOUND MATCHING playerID"));
			playerIDFound = true;
			ActivePlayerIndex = b;
		}

	}


	if (playerIDFound == true) {
		UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] DeAuthorizePlayer - existing playerID found"));

		// update the TArray as authorized=false
		FLeetActivePlayer leavingplayer;
		leavingplayer.playerID = playerID;
		leavingplayer.authorized = false;
		leavingplayer.platformID = ActivePlayers[ActivePlayerIndex].platformID;

		FString PlatformID = ActivePlayers[ActivePlayerIndex].platformID;

		ActivePlayers[ActivePlayerIndex] = leavingplayer;

		UE_LOG(LogTemp, Log, TEXT("PlatformID: %s"), *PlatformID);
		UE_LOG(LogTemp, Log, TEXT("Object is: %s"), *GetName());

		FString nonceString = "10951350917635";
		FString encryption = "off";  // Allowing unencrypted on sandbox for now.  


		FString OutputString;
		// TSharedRef<TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
		// FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

		// Build Params as text string
		OutputString = "nonce=" + nonceString + "&encryption=" + encryption;
		// urlencode the params

		FString APIURI = "/api/v2/player/" + PlatformID + "/deactivate";;

		bool requestSuccess = PerformHttpRequest(&ULeetGameInstance::ActivateRequestComplete, APIURI, OutputString);

		return requestSuccess;

	}
	else {
		UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] DeAuthorizePlayer - Not found - Ignoring"));
	}
	return true;

}

void ULeetGameInstance::DeActivateRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Test failed. NULL response"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Completed test [%s] Url=[%s] Response=[%d] [%s]"),
			*HttpRequest->GetVerb(),
			*HttpRequest->GetURL(),
			HttpResponse->GetResponseCode(),
			*HttpResponse->GetContentAsString());

		FString JsonRaw = *HttpResponse->GetContentAsString();
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			bool Authorization = JsonParsed->GetBoolField("authorization");
			//  We don't care too much about the results from this call.  
			UE_LOG(LogTemp, Log, TEXT("Authorization"));
			if (Authorization)
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization True"));
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization False"));
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [DeActivateRequestComplete] Done!"));
}

bool ULeetGameInstance::OutgoingChat(int32 playerID, FText message)
{

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] OutgoingChat"));
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] DEBUG TEST"));

	// check to see if this player is in the active list already
	bool playerIDFound = false;
	int32 ActivePlayerIndex = 0;
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] check to see if this player is in the active list already"));

	// Get our player record
	FLeetActivePlayer* CurrentActivePlayer = getPlayerByPlayerId(playerID);

	if (CurrentActivePlayer == nullptr) {
		return false;
	}

		UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] OutgoingChat - existing playerID found"));

		FString PlatformID = CurrentActivePlayer->platformID;

		UE_LOG(LogTemp, Log, TEXT("PlatformID: %s"), *PlatformID);
		UE_LOG(LogTemp, Log, TEXT("Object is: %s"), *GetName());
		UE_LOG(LogTemp, Log, TEXT("message is: %s"), *message.ToString());

		FString nonceString = "10951350917635";
		FString encryption = "off";  // Allowing unencrypted on sandbox for now.  

		FString OutputString;

		// Build Params as text string
		OutputString = "nonce=" + nonceString + "&encryption=" + encryption + "&message=" + message.ToString();
		// urlencode the params

		FString APIURI = "/api/v2/player/" + PlatformID + "/chat";

		bool requestSuccess = PerformHttpRequest(&ULeetGameInstance::OutgoingChatComplete, APIURI, OutputString);

		return requestSuccess;

}

void ULeetGameInstance::OutgoingChatComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Test failed. NULL response"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Completed test [%s] Url=[%s] Response=[%d] [%s]"),
			*HttpRequest->GetVerb(),
			*HttpRequest->GetURL(),
			HttpResponse->GetResponseCode(),
			*HttpResponse->GetContentAsString());

		FString JsonRaw = *HttpResponse->GetContentAsString();
		//  We don't care too much about the results from this call.  
	}
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [OutgoingChatComplete] Done!"));
}


/**
* Delegate fired when a session create request has completed
*
* @param SessionName the name of the session this callback is for
* @param bWasSuccessful true if the async action completed without error, false if there was an error
*/
void ULeetGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	//UE_LOG(LogOnlineGame, Verbose, TEXT("OnCreateSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	UE_LOG(LogTemp, Log, TEXT("[LEET] ULeetGameInstance::OnCreateSessionComplete"));
}

/** Initiates the session searching */
bool ULeetGameInstance::FindSessions(ULocalPlayer* PlayerOwner, bool bFindLAN)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::FindSessions"));
	bool bResult = false;

	/*
	// THIS stuff came from shooter game
	// It requires a function inside gameMode for the cast to succeed:  GetGameSessionClass
	*/
	check(PlayerOwner != nullptr);
	if (PlayerOwner)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE Player Owner found"));
		ALeetGameSession* const GameSession = GetGameSession();
		UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE got game session"));
		if (GameSession)
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE Game session found"));
			GameSession->OnFindSessionsComplete().RemoveAll(this);
			OnSearchSessionsCompleteDelegateHandle = GameSession->OnFindSessionsComplete().AddUObject(this, &ULeetGameInstance::OnSearchSessionsComplete);

			GameSession->FindSessions(PlayerOwner->GetPreferredUniqueNetId(), GameSessionName, bFindLAN, true);

			bResult = true;
		}
	}
	return bResult;
}

/** Callback which is intended to be called upon finding sessions */
void ULeetGameInstance::OnSearchSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::OnSearchSessionsComplete"));
	ALeetGameSession* const Session = GetGameSession();
	if (Session)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::OnSearchSessionsComplete: Session found"));
		Session->OnFindSessionsComplete().Remove(OnSearchSessionsCompleteDelegateHandle);

		//Session->GetSearchResults();

		const TArray<FOnlineSessionSearchResult> & SearchResults = Session->GetSearchResults();

		for (int32 IdxResult = 0; IdxResult < SearchResults.Num(); ++IdxResult)
		{
			//TSharedPtr<FServerEntry> NewServerEntry = MakeShareable(new FServerEntry());

			const FOnlineSessionSearchResult& Result = SearchResults[IdxResult];

			// setup a ustruct for bp
			// add the results to the TArray 
			FLeetSessionSearchResult searchresult;
			searchresult.OwningUserName = Result.Session.OwningUserName;
			searchresult.SearchIdx = IdxResult;
			FName key = "serverTitle";
			FString serverTitle;
			bool settingsSuccess = Result.Session.SessionSettings.Get(key, serverTitle);
			searchresult.ServerTitle = serverTitle;

			LeetSessionSearchResults.Add(searchresult);
		}
	}
}


void ULeetGameInstance::BeginWelcomeScreenState()
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::BeginWelcomeScreenState"));
	//this must come before split screen player removal so that the OSS sets all players to not using online features.
	SetIsOnline(false);

	// Remove any possible splitscren players
	RemoveSplitScreenPlayers();

	//LoadFrontEndMap(WelcomeScreenMap);

	ULocalPlayer* const LocalPlayer = GetFirstGamePlayer();
	//LocalPlayer->SetCachedUniqueNetId(nullptr);
	//check(!WelcomeMenuUI.IsValid());
	//WelcomeMenuUI = MakeShareable(new FShooterWelcomeMenu);
	//WelcomeMenuUI->Construct(this);
	//WelcomeMenuUI->AddToGameViewport();

	// Disallow splitscreen (we will allow while in the playing state)
	GetGameViewportClient()->SetDisableSplitscreenOverride(true);
}

void ULeetGameInstance::SetIsOnline(bool bInIsOnline)
{
	bIsOnline = bInIsOnline;
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		for (int32 i = 0; i < LocalPlayers.Num(); ++i)
		{
			ULocalPlayer* LocalPlayer = LocalPlayers[i];

			TSharedPtr<const FUniqueNetId> PlayerId = LocalPlayer->GetPreferredUniqueNetId();
			if (PlayerId.IsValid())
			{
				OnlineSub->SetUsingMultiplayerFeatures(*PlayerId, bIsOnline);
			}
		}
	}
}

void ULeetGameInstance::RemoveSplitScreenPlayers()
{
	// if we had been split screen, toss the extra players now
	// remove every player, back to front, except the first one
	while (LocalPlayers.Num() > 1)
	{
		ULocalPlayer* const PlayerToRemove = LocalPlayers.Last();
		RemoveExistingLocalPlayer(PlayerToRemove);
	}
}

void ULeetGameInstance::RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	check(ExistingPlayer);
	if (ExistingPlayer->PlayerController != NULL)
		//{
		//	// Kill the player
		//	AShooterCharacter* MyPawn = Cast<AShooterCharacter>(ExistingPlayer->PlayerController->GetPawn());
		//	if (MyPawn)
		//	{
		//		MyPawn->KilledBy(NULL);
		//	}
		//}

		// Remove local split-screen players from the list
		RemoveLocalPlayer(ExistingPlayer);
}



bool ULeetGameInstance::JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	// needs to tear anything down based on current state?

	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::JoinSession 1"));


	ALeetGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &ULeetGameInstance::OnJoinSessionComplete); //.AddUObject(this, &UMyGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId(), GameSessionName, SessionIndexInSearchResults))
		{
			// If any error occured in the above, pending state would be set
			//if ((PendingState == CurrentState) || (PendingState == LeetGameInstanceState::None))
			//{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				//ShowLoadingScreen();
			//	GotoState(LeetGameInstanceState::Playing);
			//	return true;
			//}
		}
	}


	return false;
}

bool ULeetGameInstance::JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::JoinSession 2"));
	// needs to tear anything down based on current state?
	ALeetGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &ULeetGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId(), GameSessionName, SearchResult))
		{
			// If any error occured in the above, pending state would be set
			if ((PendingState == CurrentState) || (PendingState == LeetGameInstanceState::None))
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				//ShowLoadingScreen();
				GotoState(LeetGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}


/**
* Delegate fired when the joining process for an online session has completed
*
* @param SessionName the name of the session this callback is for
* @param bWasSuccessful true if the async action completed without error, false if there was an error
*/
void ULeetGameInstance::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::OnJoinSessionComplete"));
	// unhook the delegate
	ALeetGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnJoinSessionComplete().Remove(OnJoinSessionCompleteDelegateHandle);
	}
	FinishJoinSession(Result);

}

void ULeetGameInstance::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		FText ReturnReason;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Game is full.");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Game no longer exists.");
			break;
		default:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join failed.");
			break;
		}

		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		//ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	InternalTravelToSession(GameSessionName);
}

void ULeetGameInstance::InternalTravelToSession(const FName& SessionName)
{
	APlayerController * const PlayerController = GetFirstLocalPlayerController();

	if (PlayerController == nullptr)
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "InvalidPlayerController", "Invalid Player Controller");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		//ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	// travel to session
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub == nullptr)
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "OSSMissing", "OSS missing");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		//ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	FString URL;
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, URL))
	{
		FText FailReason = NSLOCTEXT("NetworkErrors", "TravelSessionFailed", "Travel to Session failed.");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		//ShowMessageThenGoMain(FailReason, OKButton, FText::GetEmpty());
		UE_LOG(LogOnlineGame, Warning, TEXT("Failed to travel to session upon joining it"));
		return;
	}

	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}

bool ULeetGameInstance::TravelToSession(int32 ControllerId, FName SessionName)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		FString URL;
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && Sessions->GetResolvedConnectString(SessionName, URL))
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
			if (PC)
			{
				PC->ClientTravel(URL, TRAVEL_Absolute);
				return true;
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("Failed to join session %s"), *SessionName.ToString());
		}
	}
#if !UE_BUILD_SHIPPING
	else
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
		if (PC)
		{
			FString LocalURL(TEXT("127.0.0.1"));
			PC->ClientTravel(LocalURL, TRAVEL_Absolute);
			return true;
		}
	}
#endif //!UE_BUILD_SHIPPING

	return false;
}

void ULeetGameInstance::RemoveNetworkFailureHandlers()
{
	// Remove the local session/travel failure bindings if they exist
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == true)
	{
		GEngine->OnTravelFailure().Remove(TravelLocalSessionFailureDelegateHandle);
	}
}

void ULeetGameInstance::AddNetworkFailureHandlers()
{
	// Add network/travel error handlers (if they are not already there)
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == false)
	{
		TravelLocalSessionFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(this, &ULeetGameInstance::TravelLocalSessionFailure);
	}
}

void ULeetGameInstance::TravelLocalSessionFailure(UWorld *World, ETravelFailure::Type FailureType, const FString& ReasonString)
{
	/*
	AShooterPlayerController_Menu* const FirstPC = Cast<AShooterPlayerController_Menu>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (FirstPC != nullptr)
	{
	FText ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join Session failed.");
	if (ReasonString.IsEmpty() == false)
	{
	ReturnReason = FText::Format(NSLOCTEXT("NetworkErrors", "JoinSessionFailedReasonFmt", "Join Session failed. {0}"), FText::FromString(ReasonString));
	}

	FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
	ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
	}
	*/
}


FName ULeetGameInstance::GetInitialState()
{
	// On PC, go directly to the main menu
	return LeetGameInstanceState::MainMenu;
}

void ULeetGameInstance::GotoInitialState()
{
	GotoState(GetInitialState());
}

void ULeetGameInstance::GotoState(FName NewState)
{
	UE_LOG(LogOnline, Log, TEXT("GotoState: NewState: %s"), *NewState.ToString());

	PendingState = NewState;
}

void ULeetGameInstance::HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	UE_LOG(LogOnlineGame, Warning, TEXT("ULeetGameInstance::HandleSessionFailure: %u"), (uint32)FailureType);

}

void ULeetGameInstance::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Log, TEXT("ULeetGameInstance::OnEndSessionComplete: Session=%s bWasSuccessful=%s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
			Sessions->ClearOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegateHandle);
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		}
	}

	// continue
	CleanupSessionOnReturnToMenu();
}

void ULeetGameInstance::CleanupSessionOnReturnToMenu()
{
	bool bPendingOnlineOp = false;

	// end online game and then destroy it
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = (OnlineSub != NULL) ? OnlineSub->GetSessionInterface() : NULL;

	if (Sessions.IsValid())
	{
		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(GameSessionName);
		UE_LOG(LogOnline, Log, TEXT("Session %s is '%s'"), *GameSessionName.ToString(), EOnlineSessionState::ToString(SessionState));

		if (EOnlineSessionState::InProgress == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Ending session %s on return to main menu"), *GameSessionName.ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->EndSession(GameSessionName);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ending == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to end on return to main menu"), *GameSessionName.ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ended == SessionState || EOnlineSessionState::Pending == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Destroying session %s on return to main menu"), *GameSessionName.ToString());
			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->DestroySession(GameSessionName);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Starting == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to start, and then we will end it to return to main menu"), *GameSessionName.ToString());
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
	}

	if (!bPendingOnlineOp)
	{
		//GEngine->HandleDisconnect( GetWorld(), GetWorld()->GetNetDriver() );
	}
}

bool ULeetGameInstance::RegisterNewSession(FString IncServerSessionHostAddress, FString IncServerSessionID)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::RegisterNewSession"));
	ServerSessionHostAddress = IncServerSessionHostAddress;
	ServerSessionID = IncServerSessionID;
	GetServerInfo();
	return true;
}

bool ULeetGameInstance::IsLocalPlayerOnline(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == NULL)
	{
		return false;
	}
	const auto OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			auto UniqueId = LocalPlayer->GetCachedUniqueNetId();
			if (UniqueId.IsValid())
			{
				const auto LoginStatus = IdentityInterface->GetLoginStatus(*UniqueId);
				if (LoginStatus == ELoginStatus::LoggedIn)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool ULeetGameInstance::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{
	// This is all UI stuff.
	// Removing it for now

	return true;
}

void ULeetGameInstance::HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId)
{
	/*
	const bool bDowngraded = (LoginStatus == ELoginStatus::NotLoggedIn && !GetIsOnline()) || (LoginStatus != ELoginStatus::LoggedIn && GetIsOnline());

	UE_LOG(LogOnline, Log, TEXT("HandleUserLoginChanged: bDownGraded: %i"), (int)bDowngraded);

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	bIsLicensed = GenericApplication->ApplicationLicenseValid();

	// Find the local player associated with this unique net id
	ULocalPlayer * LocalPlayer = FindLocalPlayerFromUniqueNetId(UserId);

	// If this user is signed out, but was previously signed in, punt to welcome (or remove splitscreen if that makes sense)
	if (LocalPlayer != NULL)
	{
		if (bDowngraded)
		{
			UE_LOG(LogOnline, Log, TEXT("HandleUserLoginChanged: Player logged out: %s"), *UserId.ToString());

			//LabelPlayerAsQuitter(LocalPlayer);

			// Check to see if this was the master, or if this was a split-screen player on the client
			if (LocalPlayer == GetFirstGamePlayer() || GetIsOnline())
			{
				HandleSignInChangeMessaging();
			}
			else
			{
				// Remove local split-screen players from the list
				RemoveExistingLocalPlayer(LocalPlayer);
			}
		}
	}
	*/
}

void ULeetGameInstance::HandleSignInChangeMessaging()
{
	// Master user signed out, go to initial state (if we aren't there already)
	if (CurrentState != GetInitialState())
	{	
		GotoInitialState();
	}
}

void ULeetGameInstance::BeginLogin(FString InType, FString InId, FString InToken)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME INSTANCE ULeetGameInstance::BeginLogin"));

	const auto OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			FOnlineAccountCredentials* Credentials = new FOnlineAccountCredentials(InType, InId, InToken);
			IdentityInterface->Login(0, *Credentials);
		}
	}

}

FLeetActivePlayer* ULeetGameInstance::getPlayerByPlayerId(int32 playerID)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] getPlayerByPlayerID"));
	for (int32 b = 0; b < ActivePlayers.Num(); b++)
	{
		if (ActivePlayers[b].playerID == playerID) {
			UE_LOG(LogTemp, Log, TEXT("[LEET] [UMyGameInstance] AuthorizePlayer - FOUND MATCHING playerID"));
			FLeetActivePlayer* playerPointer = &ActivePlayers[b];
			return playerPointer;
		}
	}

	return nullptr;
}