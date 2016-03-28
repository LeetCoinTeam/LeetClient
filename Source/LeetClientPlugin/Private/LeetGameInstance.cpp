// Fill out your copyright notice in the Description page of Project Settings.

#include "LeetClientPluginPrivatePCH.h"
#include "LeetGameInstance.h"

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
				incrementBTC = JsonParsed->GetIntegerField("incrementBTC");
				minimumBTCHold = JsonParsed->GetIntegerField("minimumBTCHold");
				serverRakeBTCPercentage = JsonParsed->GetNumberField("serverRakeBTCPercentage");
				leetRakePercentage = JsonParsed->GetNumberField("leetcoinRakePercentage");
				killRewardBTC = incrementBTC - ((incrementBTC * serverRakeBTCPercentage) + (incrementBTC * leetRakePercentage));
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization False"));
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ULeetGameInstance] [GetServerInfoComplete] Done!"));
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