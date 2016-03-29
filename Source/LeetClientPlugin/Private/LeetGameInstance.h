// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameInstance.h"
#include "Http.h"
#include "Json.h"
#include "Base64.h"
#include <string>
#include "LeetGameInstance.generated.h"


USTRUCT(BlueprintType)
struct FLeetSessionSearchResult {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite)
		FString OwningUserName;

	UPROPERTY(BlueprintReadWrite)
		int32 SearchIdx;
};

class ALeetGameSession;

namespace LeetGameInstanceState
{
	extern const FName None;
	extern const FName PendingInvite;
	extern const FName WelcomeScreen;
	extern const FName MainMenu;
	extern const FName MessageMenu;
	extern const FName Playing;
}

/**
 *
 */
UCLASS()
class LEETCLIENTPLUGIN_API ULeetGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()

	// Populated through config file
	FString APIURL;
	FString ServerAPIKey;
	FString ServerAPISecret;
	FString GameKey;
	// Populated through the get server info API call
	int32 incrementBTC;
	int32 killRewardBTC; //convenience so we don't have to do the math every time.
	int32 minimumBTCHold;
	float serverRakeBTCPercentage;
	float leetRakePercentage;
	// Populated through the online subsystem
	FString ServerSessionHostAddress;
	FString ServerSessionID;

	// Constructor declaration
	//ULeetGameInstance(const FObjectInitializer& ObjectInitializer);

	//ULeetGameInstance();

	bool PerformHttpRequest(void(ULeetGameInstance::*delegateCallback)(FHttpRequestPtr, FHttpResponsePtr, bool), FString APIURI, FString ArgumentString);

public:
	
	ALeetGameSession* GetGameSession() const;
	virtual void Init() override;

	/**
	*	Find an online session
	*
	*	@param UserId user that initiated the request
	*	@param SessionName name of session this search will generate
	*	@param bIsLAN are we searching LAN matches
	*	@param bIsPresence are we searching presence sessions
	*/
	UFUNCTION(BlueprintCallable, Category = "LEET")
	bool FindSessions(ULocalPlayer* PlayerOwner, bool bLANMatch);

	/** Sends the game to the specified state. */
	void GotoState(FName NewState);

	/** Obtains the initial welcome state, which can be different based on platform */
	FName GetInitialState();

	/** Sends the game to the initial startup/frontend state  */
	void GotoInitialState();

	UFUNCTION(BlueprintCallable, Category = "LEET")
	bool JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults);

	bool JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult);

	/** Returns true if the game is in online mode */
	bool GetIsOnline() const { return bIsOnline; }

	/** Sets the online mode of the game */
	UFUNCTION(BlueprintCallable, Category = "LEET")
	void SetIsOnline(bool bInIsOnline);

	/** Shuts down the session, and frees any net driver */
	void CleanupSessionOnReturnToMenu();

	void RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer);
	void RemoveSplitScreenPlayers();

	bool GetServerInfo();
	void GetServerInfoComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	UPROPERTY(BlueprintReadOnly)
	TArray<FLeetSessionSearchResult> LeetSessionSearchResults;

	// Holds session search results
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	/**
	* Travel to a session URL (as client) for a given session
	*
	* @param ControllerId controller initiating the session travel
	* @param SessionName name of session to travel to
	*
	* @return true if successful, false otherwise
	*/
	bool TravelToSession(int32 ControllerId, FName SessionName);

	bool RegisterNewSession(FString IncServerSessionHostAddress, FString IncServerSessionID);

private:
	UPROPERTY(config)
		FString WelcomeScreenMap;

	UPROPERTY(config)
		FString MainMenuMap;

	FName CurrentState;
	FName PendingState;

	/** URL to travel to after pending network operations */
	FString TravelURL;

	FString _configPath = "";

	/** Whether the match is online or not */
	bool bIsOnline;

	/** If true, enable splitscreen when map starts loading */
	bool bPendingEnableSplitscreen;

	/** Whether the user has an active license to play the game */
	bool bIsLicensed;

	/** Last connection status that was passed into the HandleNetworkConnectionStatusChanged hander */
	EOnlineServerConnectionStatus::Type	CurrentConnectionStatus;

	/** Handle to various registered delegates */
	FDelegateHandle OnSearchSessionsCompleteDelegateHandle;
	//FDelegateHandle OnSearchResultsAvailableHandle;
	FDelegateHandle OnCreatePresenceSessionCompleteDelegateHandle;
	FDelegateHandle TravelLocalSessionFailureDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FDelegateHandle OnEndSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;

	/** Callback which is intended to be called upon finding sessions */
	void OnSearchSessionsComplete(bool bWasSuccessful);
	/** Callback which is intended to be called upon joining session */
	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);
	/** Called after all the local players are registered in a session we're joining */
	void FinishJoinSession(EOnJoinSessionCompleteResult::Type Result);
	/** Travel directly to the named session */
	void InternalTravelToSession(const FName& SessionName);

	

	/** Delegate for ending a session */
	FOnEndSessionCompleteDelegate OnEndSessionCompleteDelegate;

	void HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType);

	void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);

	void BeginWelcomeScreenState();
	void AddNetworkFailureHandlers();
	void RemoveNetworkFailureHandlers();

	/** Called when there is an error trying to travel to a local session */
	void TravelLocalSessionFailure(UWorld *World, ETravelFailure::Type FailureType, const FString& ErrorString);

protected:
	/** Delegate for creating a new session */
	//FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	/**
	* Delegate fired when a session create request has completed
	*
	* @param SessionName the name of the session this callback is for
	* @param bWasSuccessful true if the async action completed without error, false if there was an error
	*/
	virtual void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
};
