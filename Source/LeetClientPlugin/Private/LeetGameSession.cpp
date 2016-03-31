// Fill out your copyright notice in the Description page of Project Settings.

#include "LeetClientPluginPrivatePCH.h"
#include "LeetGameSession.h"

namespace
{
	const FString CustomMatchKeyword("Custom");
}

ALeetGameSession::ALeetGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::ALeetGameSession"));
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::ALeetGameSession !HasAnyFlags"));
		OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ALeetGameSession::OnCreateSessionComplete);
		OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ALeetGameSession::OnDestroySessionComplete);
		OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ALeetGameSession::OnFindSessionsComplete);
		OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ALeetGameSession::OnJoinSessionComplete);
		if (IsRunningDedicatedServer()) {
			//RegisterServer();
		}
	}
}

EOnlineAsyncTaskState::Type ALeetGameSession::GetSearchResultStatus(int32& SearchResultIdx, int32& NumSearchResults)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION ::GetSearchResultStatus"));
	SearchResultIdx = 0;
	NumSearchResults = 0;

	if (SearchSettings.IsValid())
	{
		if (SearchSettings->SearchState == EOnlineAsyncTaskState::Done)
		{
			SearchResultIdx = CurrentSessionParams.BestSessionIdx;
			NumSearchResults = SearchSettings->SearchResults.Num();
		}
		return SearchSettings->SearchState;
	}

	return EOnlineAsyncTaskState::NotStarted;
}

/**
* Get the search results.
*
* @return Search results
*/
const TArray<FOnlineSessionSearchResult> & ALeetGameSession::GetSearchResults() const
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION ::GetSearchResults"));
	return SearchSettings->SearchResults;
};

/**
* Delegate fired when a session create request has completed
*
* @param SessionName the name of the session this callback is for
* @param bWasSuccessful true if the async action completed without error, false if there was an error
*/
void ALeetGameSession::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	//UE_LOG(LogOnlineGame, Verbose, TEXT("OnCreateSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::OnCreateSessionComplete"));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::OnCreateSessionComplete OnlineSub"));
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		FNamedOnlineSession *sess = Sessions->GetNamedSession(SessionName);
		// TODO - move this off instance, and just fire off the http request from inside here.
		ULeetGameInstance *gameInstance = Cast<ULeetGameInstance>(this->GetGameInstance());
		//if (GEngine->GetWorld() != nullptr && GEngine->GetWorld()->GetGameInstance() != nullptr)
		//{
			UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::OnCreateSessionComplete Got Instance"));
			//ULeetGameInstance *gameInstance = Cast<ULeetGameInstance>(GEngine->GetWorld()->GetGameInstance());
			UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::OnCreateSessionComplete 2"));
			bool result = gameInstance->RegisterNewSession(sess->SessionInfo->ToString(), sess->SessionInfo->GetSessionId().ToString());
			UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::OnCreateSessionComplete 3"));
			gameInstance->GetServerInfo();
		//}
	}
	//OnCreateSessionComplete().Broadcast(SessionName, bWasSuccessful);
	OnCreatePresenceSessionComplete().Broadcast(SessionName, bWasSuccessful);
}

void ALeetGameSession::OnFindSessionsComplete(bool bWasSuccessful)
{
	//UE_LOG(LogOnlineGame, Verbose, TEXT("OnFindSessionsComplete bSuccess: %d"), bWasSuccessful);
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION ::OnFindSessionsComplete"));

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

			UE_LOG(LogTemp, Verbose, TEXT("Num Search Results: %d"), SearchSettings->SearchResults.Num());
			for (int32 SearchIdx = 0; SearchIdx < SearchSettings->SearchResults.Num(); SearchIdx++)
			{
				const FOnlineSessionSearchResult& SearchResult = SearchSettings->SearchResults[SearchIdx];
				DumpSession(&SearchResult.Session);
			}

			OnFindSessionsComplete().Broadcast(bWasSuccessful);
			
		}
	}
}

void ALeetGameSession::FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions"));
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  OnlineSub"));
		
		CurrentSessionParams.SessionName = SessionName;
		CurrentSessionParams.bIsLAN = bIsLAN;
		CurrentSessionParams.bIsPresence = bIsPresence;
		CurrentSessionParams.UserId = UserId;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  Session Valid"));
			SearchSettings = MakeShareable(new FLeetOnlineSearchSettings(bIsLAN, bIsPresence));
			//UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  1"));
			SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, CustomMatchKeyword, EOnlineComparisonOp::Equals);
			//UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  2"));

			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();
			//UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  3"));

			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			//UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  4"));
			Sessions->FindSessions(*CurrentSessionParams.UserId, SearchSettingsRef);
			UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION FindSessions:  Done"));
		}
		
	}
	else
	{
		OnFindSessionsComplete(false);
	}
}

/**
* Delegate fired when a destroying an online session has completed
*
* @param SessionName the name of the session this callback is for
* @param bWasSuccessful true if the async action completed without error, false if there was an error
*/
void ALeetGameSession::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnDestroySessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		HostSettings = NULL;
	}
}

/**
* Delegate fired when the joining process for an online session has completed
*
* @param SessionName the name of the session this callback is for
* @param bWasSuccessful true if the async action completed without error, false if there was an error
*/
void ALeetGameSession::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bWillTravel = false;

	UE_LOG(LogOnlineGame, Verbose, TEXT("OnJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), static_cast<int32>(Result));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = NULL;
	if (OnlineSub)
	{
		Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}

	OnJoinSessionComplete().Broadcast(Result);
}

bool ALeetGameSession::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, int32 SessionIndexInSearchResults)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION JoinSession 1"));
	bool bResult = false;

	if (SessionIndexInSearchResults >= 0 && SessionIndexInSearchResults < SearchSettings->SearchResults.Num())
	{
		bResult = JoinSession(UserId, SessionName, SearchSettings->SearchResults[SessionIndexInSearchResults]);
	}

	return bResult;
}

bool ALeetGameSession::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION JoinSession 2"));
	bool bResult = false;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION JoinSession : OnelineSub"));
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && UserId.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] GAME SESSION JoinSession : Session valid"));
			OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
			bResult = Sessions->JoinSession(*UserId, SessionName, SearchResult);
		}
	}

	return bResult;
}

void ALeetGameSession::RegisterServer() {
	UE_LOG(LogTemp, Log, TEXT("[LEET] ALeetGameSession::RegisterServer"));
	UWorld* World = GetWorld();
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = 3;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bIsLANMatch = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bIsDedicated = true;

	OnCreateSessionCompleteDelegateHandle = SessionInt->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
	SessionInt->CreateSession(0, GameSessionName, Settings);
	
	//OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	return;
}