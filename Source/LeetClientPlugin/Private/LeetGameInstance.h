// This is a visual studio hack
// https://answers.unrealengine.com/questions/229449/stdtype-info-fails-to-build-in-development-configu.html
#ifdef WIN32

#pragma warning( push ) 
#pragma warning( disable: 4530 )
namespace std { typedef type_info type_info; }
#include "CryptoPP/5.6.2/include/sha3.h"
#include "CryptoPP/5.6.2/include/hmac.h"

#elif WIN64

#pragma warning( push ) 
#pragma warning( disable: 4530 )
namespace std { typedef type_info type_info; }
#include "CryptoPP/5.6.2/include/sha3.h"
#include "CryptoPP/5.6.2/include/hmac.h"

#endif
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "Engine/GameInstance.h"
#include "Http.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Base64.h"
#include <string>

#include "LeetGameInstance.generated.h"


USTRUCT(BlueprintType)
struct FLeetSessionSearchResult {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite)
		FString OwningUserName;

	UPROPERTY(BlueprintReadWrite)
		FString ServerTitle;
	// TODO - add more leet data that we care about

	UPROPERTY(BlueprintReadWrite)
		int32 SearchIdx;
};

USTRUCT()
struct FLeetActivePlayer {

	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LEET")
	int32 playerID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	FString platformID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	FString playerKey;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	FString playerTitle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	bool authorized;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	int32 roundKills;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	int32 roundDeaths;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	TArray<FString> killed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	int32 Rank;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	int32 BTCHold;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
	FString gamePlayerKey;

};

USTRUCT()
struct FLeetActivePlayers {

	GENERATED_USTRUCT_BODY()
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		TArray<FLeetActivePlayer> ActivePlayers;
};

USTRUCT()
struct FLeetServerLink {

	GENERATED_USTRUCT_BODY()
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString targetServerTitle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString targetServerKey;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool targetStatusIsDynamic;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool targetStatusProvisioned;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool targetStatusOnline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool targetStatusFull;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool targetStatusDead;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool permissionCanMount;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool permissionCanPlayerTravel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		bool permissionCanDismount;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString resourcesUsedToTravel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString resourceAmountsUsedToTravel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		int32 btcCostToTravel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		float coordLocationX;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		float coordLocationY;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		float coordLocationZ;
};

USTRUCT()
struct FLeetServerLinks {

	GENERATED_USTRUCT_BODY()
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		TArray<FLeetServerLink> links;
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


	// Moving this to a struct for easy JSON encode/decode
	//TArray<FLeetActivePlayer> ActivePlayers;
	FLeetActivePlayers PlayerRecord;

	FLeetServerLinks ServerLinks;

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

	/** Start the authentication flow */
	UFUNCTION(BlueprintCallable, Category = "LEET")
	void BeginLogin(FString InType, FString InId, FString InToken);

	/** Shuts down the session, and frees any net driver */
	void CleanupSessionOnReturnToMenu();

	void RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer);
	void RemoveSplitScreenPlayers();

	bool GetServerInfo();
	void GetServerInfoComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	bool GetServerLinks();
	void GetServerLinksComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

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

	/** Returns true if the passed in local player is signed in and online */
	bool IsLocalPlayerOnline(ULocalPlayer* LocalPlayer);

	/** Returns true if owning player is online. Displays proper messaging if the user can't play */
	bool ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer);


	// Activate a player against the leet api
	UFUNCTION(BlueprintCallable, Category = "LEET")
	bool ActivatePlayer(FString PlatformID, int32 playerID);
	void ActivateRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	bool DeActivatePlayer(int32 playerID);
	void DeActivateRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	bool OutgoingChat(int32 playerID, FText message);
	void OutgoingChatComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	bool SubmitMatchResults();
	void SubmitMatchResultsComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	bool GetGamePlayer(FString PlayerKey, bool bAttemptLock);
	void GetGamePlayerRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	// Get a player out of our custom array struct
	FLeetActivePlayer* getPlayerByPlayerId(int32 playerID);
	FLeetActivePlayer* getPlayerByPlayerKey(FString playerKey);

	// A Kill occurred.
	// Record it.
	UFUNCTION(BlueprintCallable, Category = "LEET")
	bool RecordKill(int32 killerPlayerID, int32 victimPlayerID);

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

	/** Show messaging and punt to welcome screen */
	void HandleSignInChangeMessaging();

	// OSS delegates to handle
	void HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId);


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

#ifdef WIN32

#pragma warning( pop )

#endif