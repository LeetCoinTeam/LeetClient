// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemLeetPrivatePCH.h"
#include "OnlineSessionInterfaceLeet.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSubsystemLeet.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineAsyncTaskManagerLeet.h"
#include "SocketSubsystem.h"
#include "LANBeacon.h"
#include "NboSerializerLeet.h"
//#include "LeetClient.h"

#include "VoiceInterface.h"


FOnlineSessionInfoLeet::FOnlineSessionInfoLeet() :
	HostAddr(NULL),
	SessionId(TEXT("INVALID"))
{
}

void FOnlineSessionInfoLeet::Init(const FOnlineSubsystemLeet& Subsystem)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online SessionInfo INIT"));
	// Read the IP from the system
	bool bCanBindAll;
	HostAddr = ISocketSubsystem::Get()->GetLocalHostAddr(*GLog, bCanBindAll);

	// The below is a workaround for systems that set hostname to a distinct address from 127.0.0.1 on a loopback interface.
	// See e.g. https://www.debian.org/doc/manuals/debian-reference/ch05.en.html#_the_hostname_resolution
	// and http://serverfault.com/questions/363095/why-does-my-hostname-appear-with-the-address-127-0-1-1-rather-than-127-0-0-1-in
	// Since we bind to 0.0.0.0, we won't answer on 127.0.1.1, so we need to advertise ourselves as 127.0.0.1 for any other loopback address we may have.
	uint32 HostIp = 0;
	HostAddr->GetIp(HostIp); // will return in host order
							 // if this address is on loopback interface, advertise it as 127.0.0.1
	if ((HostIp & 0xff000000) == 0x7f000000)
	{
		HostAddr->SetIp(0x7f000001);	// 127.0.0.1
	}

	// Now set the port that was configured
	HostAddr->SetPort(GetPortFromNetDriver(Subsystem.GetInstanceName()));

	FGuid OwnerGuid;
	FPlatformMisc::CreateGuid(OwnerGuid);
	SessionId = FUniqueNetIdString(OwnerGuid.ToString());
}

/**
*	Async task for ending a Leet online session
*/
class FOnlineAsyncTaskLeetEndSession : public FOnlineAsyncTaskBasic<FOnlineSubsystemLeet>
{
private:
	/** Name of session ending */
	FName SessionName;

public:
	FOnlineAsyncTaskLeetEndSession(class FOnlineSubsystemLeet* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskBasic(InSubsystem),
		SessionName(InSessionName)
	{
	}

	~FOnlineAsyncTaskLeetEndSession()
	{
	}

	/**
	*	Get a human readable description of task
	*/
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FOnlineAsyncTaskLeetEndSession bWasSuccessful: %d SessionName: %s"), bWasSuccessful, *SessionName.ToString());
	}

	/**
	* Give the async task time to do its work
	* Can only be called on the async task manager thread
	*/
	virtual void Tick() override
	{
		bIsComplete = true;
		bWasSuccessful = true;
	}

	/**
	* Give the async task a chance to marshal its data back to the game thread
	* Can only be called on the game thread by the async task manager
	*/
	virtual void Finalize() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
		if (Session)
		{
			Session->SessionState = EOnlineSessionState::Ended;
		}
	}

	/**
	*	Async task is given a chance to trigger it's delegates
	*/
	virtual void TriggerDelegates() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			SessionInt->TriggerOnEndSessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}
};

/**
*	Async task for destroying a Leet online session
*/
class FOnlineAsyncTaskLeetDestroySession : public FOnlineAsyncTaskBasic<FOnlineSubsystemLeet>
{
private:
	/** Name of session ending */
	FName SessionName;

public:
	FOnlineAsyncTaskLeetDestroySession(class FOnlineSubsystemLeet* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskBasic(InSubsystem),
		SessionName(InSessionName)
	{
	}

	~FOnlineAsyncTaskLeetDestroySession()
	{
	}

	/**
	*	Get a human readable description of task
	*/
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FOnlineAsyncTaskLeetDestroySession bWasSuccessful: %d SessionName: %s"), bWasSuccessful, *SessionName.ToString());
	}

	/**
	* Give the async task time to do its work
	* Can only be called on the async task manager thread
	*/
	virtual void Tick() override
	{
		bIsComplete = true;
		bWasSuccessful = true;
	}

	/**
	* Give the async task a chance to marshal its data back to the game thread
	* Can only be called on the game thread by the async task manager
	*/
	virtual void Finalize() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
			if (Session)
			{
				SessionInt->RemoveNamedSession(SessionName);
			}
		}
	}

	/**
	*	Async task is given a chance to trigger it's delegates
	*/
	virtual void TriggerDelegates() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			SessionInt->TriggerOnDestroySessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}
};

bool FOnlineSessionLeet::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Create"));
	uint32 Result = E_FAIL;

	// Check for an existing session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session == NULL)
	{
		// Create a new session and deep copy the game settings
		Session = AddNamedSession(SessionName, NewSessionSettings);
		check(Session);
		Session->SessionState = EOnlineSessionState::Creating;
		Session->NumOpenPrivateConnections = NewSessionSettings.NumPrivateConnections;
		Session->NumOpenPublicConnections = NewSessionSettings.NumPublicConnections;	// always start with full public connections, local player will register later

		Session->HostingPlayerNum = HostingPlayerNum;

		check(LeetSubsystem);
		IOnlineIdentityPtr Identity = LeetSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->OwningUserId = Identity->GetUniquePlayerId(HostingPlayerNum);
			Session->OwningUserName = Identity->GetPlayerNickname(HostingPlayerNum);
		}

		// if did not get a valid one, use just something
		if (!Session->OwningUserId.IsValid())
		{
			Session->OwningUserId = MakeShareable(new FUniqueNetIdString(FString::Printf(TEXT("%d"), HostingPlayerNum)));
			Session->OwningUserName = FString(TEXT("LeetUser"));
		}

		// Unique identifier of this build for compatibility
		Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

		// Setup the host session info
		FOnlineSessionInfoLeet* NewSessionInfo = new FOnlineSessionInfoLeet();
		NewSessionInfo->Init(*LeetSubsystem);
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// Make Leet API call to register this server online.
		//LeetSubsystem->GetClient()->GetServerInfo(NewSessionInfo->HostAddr->ToString(true), NewSessionInfo->SessionId.ToString());

		Result = UpdateLANStatus();

		if (Result != ERROR_IO_PENDING)
		{
			// Set the game state as pending (not started)
			Session->SessionState = EOnlineSessionState::Pending;

			if (Result != ERROR_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Cannot create session '%s': session already exists."), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		TriggerOnCreateSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_IO_PENDING || Result == ERROR_SUCCESS;
}

bool FOnlineSessionLeet::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	// todo: use proper	HostingPlayerId
	return CreateSession(0, SessionName, NewSessionSettings);
}

bool FOnlineSessionLeet::NeedsToAdvertise()
{
	FScopeLock ScopeLock(&SessionLock);

	bool bResult = false;
	for (int32 SessionIdx = 0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		FNamedOnlineSession& Session = Sessions[SessionIdx];
		if (NeedsToAdvertise(Session))
		{
			bResult = true;
			break;
		}
	}

	return bResult;
}

bool FOnlineSessionLeet::NeedsToAdvertise(FNamedOnlineSession& Session)
{
	// In Leet, we have to imitate missing online service functionality, so we advertise:
	// a) LAN match with open public connections (same as usually)
	// b) Not started public LAN session (same as usually)
	// d) Joinable presence-enabled session that would be advertised with in an online service
	// (all of that only if we're server)
	return Session.SessionSettings.bShouldAdvertise && IsHost(Session) &&
		(
			(
				Session.SessionSettings.bIsLANMatch &&
				(Session.SessionState != EOnlineSessionState::InProgress || (Session.SessionSettings.bAllowJoinInProgress && Session.NumOpenPublicConnections > 0))
				)
			||
			(
				Session.SessionSettings.bAllowJoinViaPresence || Session.SessionSettings.bAllowJoinViaPresenceFriendsOnly
				)
			);
}

uint32 FOnlineSessionLeet::UpdateLANStatus()
{
	uint32 Result = ERROR_SUCCESS;

	if (NeedsToAdvertise())
	{
		// set up LAN session
		if (LANSessionManager.GetBeaconState() == ELanBeaconState::NotUsingLanBeacon)
		{
			FOnValidQueryPacketDelegate QueryPacketDelegate = FOnValidQueryPacketDelegate::CreateRaw(this, &FOnlineSessionLeet::OnValidQueryPacketReceived);
			if (!LANSessionManager.Host(QueryPacketDelegate))
			{
				Result = E_FAIL;

				LANSessionManager.StopLANSession();
			}
		}
	}
	else
	{
		if (LANSessionManager.GetBeaconState() != ELanBeaconState::Searching)
		{
			// Tear down the LAN beacon
			LANSessionManager.StopLANSession();
		}
	}

	return Result;
}

bool FOnlineSessionLeet::StartSession(FName SessionName)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Start"));
	uint32 Result = E_FAIL;
	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't start a match multiple times
		if (Session->SessionState == EOnlineSessionState::Pending ||
			Session->SessionState == EOnlineSessionState::Ended)
		{
			// If this lan match has join in progress disabled, shut down the beacon
			Result = UpdateLANStatus();
			Session->SessionState = EOnlineSessionState::InProgress;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Can't start an online session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Can't start an online game for session (%s) that hasn't been created"), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		// Just trigger the delegate
		TriggerOnStartSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionLeet::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Update"));
	bool bWasSuccessful = true;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// @TODO ONLINE update LAN settings
		Session->SessionSettings = UpdatedSessionSettings;
		TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);
	}

	return bWasSuccessful;
}

bool FOnlineSessionLeet::EndSession(FName SessionName)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session End"));
	uint32 Result = E_FAIL;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't end a match that isn't in progress
		if (Session->SessionState == EOnlineSessionState::InProgress)
		{
			Session->SessionState = EOnlineSessionState::Ended;

			// If the session should be advertised and the lan beacon was destroyed, recreate
			Result = UpdateLANStatus();
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Can't end session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Can't end an online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		if (Session)
		{
			Session->SessionState = EOnlineSessionState::Ended;
		}

		TriggerOnEndSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionLeet::DestroySession(FName SessionName)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Destroy"));
	uint32 Result = E_FAIL;
	// Find the session in question
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// The session info is no longer needed
		RemoveNamedSession(Session->SessionName);

		Result = UpdateLANStatus();
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Can't destroy a null online session (%s)"), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		TriggerOnDestroySessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionLeet::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionLeet::StartMatchmaking(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG(LogOnline, Warning, TEXT("StartMatchmaking is not supported on this platform. Use FindSessions or FindSessionById."));
	TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionLeet::CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName)
{
	UE_LOG(LogOnline, Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionLeet::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	UE_LOG(LogOnline, Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionLeet::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Find"));
	uint32 Return = E_FAIL;

	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		// Free up previous results
		SearchSettings->SearchResults.Empty();

		// Copy the search pointer so we can keep it around
		CurrentSessionSearch = SearchSettings;

		// remember the time at which we started search, as this will be used for a "good enough" ping estimation
		SessionSearchStartInSeconds = FPlatformTime::Seconds();

		// Check if its a LAN query
		Return = FindLANSession();

		if (Return == ERROR_IO_PENDING)
		{
			SearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Ignoring game search request while one is pending"));
		Return = ERROR_IO_PENDING;
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionLeet::FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Sessions Find"));
	// This function doesn't use the SearchingPlayerNum parameter, so passing in anything is fine.
	return FindSessions(0, SearchSettings);
}

bool FOnlineSessionLeet::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegates)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Find by ID"));
	FOnlineSessionSearchResult EmptyResult;
	CompletionDelegates.ExecuteIfBound(0, false, EmptyResult);
	return true;
}

uint32 FOnlineSessionLeet::FindLANSession()
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Find LAN"));
	uint32 Return = ERROR_IO_PENDING;

	// Recreate the unique identifier for this client
	GenerateNonce((uint8*)&LANSessionManager.LanNonce, 8);

	FOnValidResponsePacketDelegate ResponseDelegate = FOnValidResponsePacketDelegate::CreateRaw(this, &FOnlineSessionLeet::OnValidResponsePacketReceived);
	FOnSearchingTimeoutDelegate TimeoutDelegate = FOnSearchingTimeoutDelegate::CreateRaw(this, &FOnlineSessionLeet::OnLANSearchTimeout);

	FNboSerializeToBufferLeet Packet(LAN_BEACON_MAX_PACKET_SIZE);
	LANSessionManager.CreateClientQueryPacket(Packet, LANSessionManager.LanNonce);
	if (LANSessionManager.Search(Packet, ResponseDelegate, TimeoutDelegate) == false)
	{
		Return = E_FAIL;

		FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;

		// Just trigger the delegate as having failed
		TriggerOnFindSessionsCompleteDelegates(false);
	}
	return Return;
}

bool FOnlineSessionLeet::CancelFindSessions()
{
	uint32 Return = E_FAIL;
	if (CurrentSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		// Make sure it's the right type
		Return = ERROR_SUCCESS;

		FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
		CurrentSessionSearch = NULL;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Can't cancel a search that isn't in progress"));
	}

	if (Return != ERROR_IO_PENDING)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(true);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionLeet::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Join"));
	uint32 Return = E_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	// Don't join a session if already in one or hosting one
	if (Session == NULL)
	{
		// Create a named session from the search result data
		Session = AddNamedSession(SessionName, DesiredSession.Session);
		Session->HostingPlayerNum = PlayerNum;

		// Create Internet or LAN match
		FOnlineSessionInfoLeet* NewSessionInfo = new FOnlineSessionInfoLeet();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		Return = JoinLANSession(PlayerNum, Session, &DesiredSession.Session);

		// turn off advertising on Join, to avoid clients advertising it over LAN
		Session->SessionSettings.bShouldAdvertise = false;

		if (Return != ERROR_IO_PENDING)
		{
			if (Return != ERROR_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Session (%s) already exists, can't join twice"), *SessionName.ToString());
	}

	if (Return != ERROR_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ERROR_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionLeet::JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Join 2"));
	// Assuming player 0 should be OK here
	return JoinSession(0, SessionName, DesiredSession);
}

bool FOnlineSessionLeet::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Leet subsystem
	FOnlineSessionSearchResult EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, EmptySearchResult);
	return false;
};

bool FOnlineSessionLeet::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Leet subsystem
	FOnlineSessionSearchResult EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	return false;
}

bool FOnlineSessionLeet::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Leet subsystem
	return false;
};

bool FOnlineSessionLeet::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Leet subsystem
	return false;
}

bool FOnlineSessionLeet::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Leet subsystem
	return false;
};

bool FOnlineSessionLeet::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Leet subsystem
	return false;
}

uint32 FOnlineSessionLeet::JoinLANSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Join LAN"));
	check(Session != nullptr);

	uint32 Result = E_FAIL;
	Session->SessionState = EOnlineSessionState::Pending;

	if (Session->SessionInfo.IsValid() && SearchSession != nullptr && SearchSession->SessionInfo.IsValid())
	{
		// Copy the session info over
		const FOnlineSessionInfoLeet* SearchSessionInfo = (const FOnlineSessionInfoLeet*)SearchSession->SessionInfo.Get();
		FOnlineSessionInfoLeet* SessionInfo = (FOnlineSessionInfoLeet*)Session->SessionInfo.Get();
		SessionInfo->SessionId = SearchSessionInfo->SessionId;

		uint32 IpAddr;
		SearchSessionInfo->HostAddr->GetIp(IpAddr);
		SessionInfo->HostAddr = ISocketSubsystem::Get()->CreateInternetAddr(IpAddr, SearchSessionInfo->HostAddr->GetPort());
		Result = ERROR_SUCCESS;
	}

	return Result;
}

bool FOnlineSessionLeet::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	return false;
}

/** Get a resolved connection string from a session info */
static bool GetConnectStringFromSessionInfo(TSharedPtr<FOnlineSessionInfoLeet>& SessionInfo, FString& ConnectInfo, int32 PortOverride = 0)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Get Connect String"));
	bool bSuccess = false;
	if (SessionInfo.IsValid())
	{
		if (SessionInfo->HostAddr.IsValid() && SessionInfo->HostAddr->IsValid())
		{
			if (PortOverride != 0)
			{
				ConnectInfo = FString::Printf(TEXT("%s:%d"), *SessionInfo->HostAddr->ToString(false), PortOverride);
			}
			else
			{
				ConnectInfo = FString::Printf(TEXT("%s"), *SessionInfo->HostAddr->ToString(true));
			}

			bSuccess = true;
		}
	}

	return bSuccess;
}

bool FOnlineSessionLeet::GetResolvedConnectString(FName SessionName, FString& ConnectInfo)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Get Resolved Connect String"));
	bool bSuccess = false;
	// Find the session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session != NULL)
	{
		TSharedPtr<FOnlineSessionInfoLeet> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoLeet>(Session->SessionInfo);
		bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		if (!bSuccess)
		{
			UE_LOG_ONLINE(Warning, TEXT("Invalid session info for session %s in GetResolvedConnectString()"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning,
			TEXT("Unknown session name (%s) specified to GetResolvedConnectString()"),
			*SessionName.ToString());
	}

	return bSuccess;
}

bool FOnlineSessionLeet::GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Get Resolved Connect String 2"));
	bool bSuccess = false;
	if (SearchResult.Session.SessionInfo.IsValid())
	{
		TSharedPtr<FOnlineSessionInfoLeet> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoLeet>(SearchResult.Session.SessionInfo);

		if (PortType == BeaconPort)
		{
			int32 BeaconListenPort = 15000;
			if (SearchResult.Session.SessionSettings.Get(SETTING_BEACONPORT, BeaconListenPort) && BeaconListenPort > 0)
			{
				bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);
			}
		}
		else if (PortType == GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}
	}

	if (!bSuccess || ConnectInfo.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session info in search result to GetResolvedConnectString()"));
	}

	return bSuccess;
}

FOnlineSessionSettings* FOnlineSessionLeet::GetSessionSettings(FName SessionName)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Get Session Settings"));
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		return &Session->SessionSettings;
	}
	return NULL;
}

void FOnlineSessionLeet::RegisterLocalPlayers(FNamedOnlineSession* Session)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Register Local Players"));
	if (!LeetSubsystem->IsDedicated())
	{
		IOnlineVoicePtr VoiceInt = LeetSubsystem->GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			for (int32 Index = 0; Index < MAX_LOCAL_PLAYERS; Index++)
			{
				// Register the local player as a local talker
				VoiceInt->RegisterLocalTalker(Index);
			}
		}
	}
}

void FOnlineSessionLeet::RegisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineVoicePtr VoiceInt = LeetSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!LeetSubsystem->IsLocalPlayer(PlayerId))
		{
			VoiceInt->RegisterRemoteTalker(PlayerId);
		}
		else
		{
			// This is a local player. In case their PlayerState came last during replication, reprocess muting
			VoiceInt->ProcessMuteChangeNotification();
		}
	}
}

void FOnlineSessionLeet::UnregisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineVoicePtr VoiceInt = LeetSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!LeetSubsystem->IsLocalPlayer(PlayerId))
		{
			if (VoiceInt.IsValid())
			{
				VoiceInt->UnregisterRemoteTalker(PlayerId);
			}
		}
	}
}

bool FOnlineSessionLeet::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Register Player"));
	TArray< TSharedRef<const FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FUniqueNetIdString(PlayerId)));
	return RegisterPlayers(SessionName, Players, bWasInvited);
}

bool FOnlineSessionLeet::RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Register Players"));
	bool bSuccess = false;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		bSuccess = true;

		for (int32 PlayerIdx = 0; PlayerIdx<Players.Num(); PlayerIdx++)
		{
			const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

			FUniqueNetIdMatcher PlayerMatch(*PlayerId);
			if (Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) == INDEX_NONE)
			{
				Session->RegisteredPlayers.Add(PlayerId);
				RegisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections > 0)
				{
					Session->NumOpenPublicConnections--;
				}
				else if (Session->NumOpenPrivateConnections > 0)
				{
					Session->NumOpenPrivateConnections--;
				}
			}
			else
			{
				RegisterVoice(*PlayerId);
				UE_LOG_ONLINE(Log, TEXT("Player %s already registered in session %s"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

bool FOnlineSessionLeet::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session UNRegister Player"));
	TArray< TSharedRef<const FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FUniqueNetIdString(PlayerId)));
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionLeet::UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session UNRegister Players"));
	bool bSuccess = true;

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

			FUniqueNetIdMatcher PlayerMatch(*PlayerId);
			int32 RegistrantIndex = Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch);
			if (RegistrantIndex != INDEX_NONE)
			{
				Session->RegisteredPlayers.RemoveAtSwap(RegistrantIndex);
				UnregisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections < Session->SessionSettings.NumPublicConnections)
				{
					Session->NumOpenPublicConnections++;
				}
				else if (Session->NumOpenPrivateConnections < Session->SessionSettings.NumPrivateConnections)
				{
					Session->NumOpenPrivateConnections++;
				}
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("No game present to leave for session (%s)"), *SessionName.ToString());
		bSuccess = false;
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionLeet::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_Session_Interface);
	TickLanTasks(DeltaTime);
}

void FOnlineSessionLeet::TickLanTasks(float DeltaTime)
{
	LANSessionManager.Tick(DeltaTime);
}

void FOnlineSessionLeet::AppendSessionToPacket(FNboSerializeToBufferLeet& Packet, FOnlineSession* Session)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Session Append to Packet"));
	/** Owner of the session */
	Packet << *StaticCastSharedPtr<const FUniqueNetIdString>(Session->OwningUserId)
		<< Session->OwningUserName
		<< Session->NumOpenPrivateConnections
		<< Session->NumOpenPublicConnections;

	// Try to get the actual port the netdriver is using
	SetPortFromNetDriver(*LeetSubsystem, Session->SessionInfo);

	// Write host info (host addr, session id, and key)
	Packet << *StaticCastSharedPtr<FOnlineSessionInfoLeet>(Session->SessionInfo);

	// Now append per game settings
	AppendSessionSettingsToPacket(Packet, &Session->SessionSettings);
}

void FOnlineSessionLeet::AppendSessionSettingsToPacket(FNboSerializeToBufferLeet& Packet, FOnlineSessionSettings* SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE(Verbose, TEXT("Sending session settings to client"));
#endif

	// Members of the session settings class
	Packet << SessionSettings->NumPublicConnections
		<< SessionSettings->NumPrivateConnections
		<< (uint8)SessionSettings->bShouldAdvertise
		<< (uint8)SessionSettings->bIsLANMatch
		<< (uint8)SessionSettings->bIsDedicated
		<< (uint8)SessionSettings->bUsesStats
		<< (uint8)SessionSettings->bAllowJoinInProgress
		<< (uint8)SessionSettings->bAllowInvites
		<< (uint8)SessionSettings->bUsesPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresenceFriendsOnly
		<< (uint8)SessionSettings->bAntiCheatProtected
		<< SessionSettings->BuildUniqueId;

	// First count number of advertised keys
	int32 NumAdvertisedProperties = 0;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			NumAdvertisedProperties++;
		}
	}

	// Add count of advertised keys and the data
	Packet << (int32)NumAdvertisedProperties;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			Packet << It.Key();
			Packet << Setting;
#if DEBUG_LAN_BEACON
			UE_LOG_ONLINE(Verbose, TEXT("%s"), *Setting.ToString());
#endif
		}
	}
}

void FOnlineSessionLeet::OnValidQueryPacketReceived(uint8* PacketData, int32 PacketLength, uint64 ClientNonce)
{
	// Iterate through all registered sessions and respond for each one that can be joinable
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SessionIndex = 0; SessionIndex < Sessions.Num(); SessionIndex++)
	{
		FNamedOnlineSession* Session = &Sessions[SessionIndex];

		if (Session)
		{
			bool bAdvertiseSession = ((Session->SessionSettings.bIsLANMatch || Session->SessionSettings.bAllowJoinInProgress) && Session->NumOpenPublicConnections > 0) ||
				Session->SessionSettings.bAllowJoinViaPresence ||
				Session->SessionSettings.bAllowJoinViaPresenceFriendsOnly;

			if (bAdvertiseSession)
			{
				FNboSerializeToBufferLeet Packet(LAN_BEACON_MAX_PACKET_SIZE);
				// Create the basic header before appending additional information
				LANSessionManager.CreateHostResponsePacket(Packet, ClientNonce);

				// Add all the session details
				AppendSessionToPacket(Packet, Session);

				// Broadcast this response so the client can see us
				if (!Packet.HasOverflow())
				{
					LANSessionManager.BroadcastPacket(Packet, Packet.GetByteCount());
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("LAN broadcast packet overflow, cannot broadcast on LAN"));
				}
			}
		}
	}
}

void FOnlineSessionLeet::ReadSessionFromPacket(FNboSerializeFromBufferLeet& Packet, FOnlineSession* Session)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE(Verbose, TEXT("Reading session information from server"));
#endif

	/** Owner of the session */
	FUniqueNetIdString* UniqueId = new FUniqueNetIdString;
	Packet >> *UniqueId
		>> Session->OwningUserName
		>> Session->NumOpenPrivateConnections
		>> Session->NumOpenPublicConnections;

	Session->OwningUserId = MakeShareable(UniqueId);

	// Allocate and read the connection data
	FOnlineSessionInfoLeet* LeetSessionInfo = new FOnlineSessionInfoLeet();
	LeetSessionInfo->HostAddr = ISocketSubsystem::Get()->CreateInternetAddr();
	Packet >> *LeetSessionInfo;
	Session->SessionInfo = MakeShareable(LeetSessionInfo);

	// Read any per object data using the server object
	ReadSettingsFromPacket(Packet, Session->SessionSettings);
}

void FOnlineSessionLeet::ReadSettingsFromPacket(FNboSerializeFromBufferLeet& Packet, FOnlineSessionSettings& SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE(Verbose, TEXT("Reading game settings from server"));
#endif

	// Clear out any old settings
	SessionSettings.Settings.Empty();

	// Members of the session settings class
	Packet >> SessionSettings.NumPublicConnections
		>> SessionSettings.NumPrivateConnections;
	uint8 Read = 0;
	// Read all the bools as bytes
	Packet >> Read;
	SessionSettings.bShouldAdvertise = !!Read;
	Packet >> Read;
	SessionSettings.bIsLANMatch = !!Read;
	Packet >> Read;
	SessionSettings.bIsDedicated = !!Read;
	Packet >> Read;
	SessionSettings.bUsesStats = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinInProgress = !!Read;
	Packet >> Read;
	SessionSettings.bAllowInvites = !!Read;
	Packet >> Read;
	SessionSettings.bUsesPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = !!Read;
	Packet >> Read;
	SessionSettings.bAntiCheatProtected = !!Read;

	// BuildId
	Packet >> SessionSettings.BuildUniqueId;

	// Now read the contexts and properties from the settings class
	int32 NumAdvertisedProperties = 0;
	// First, read the number of advertised properties involved, so we can presize the array
	Packet >> NumAdvertisedProperties;
	if (Packet.HasOverflow() == false)
	{
		FName Key;
		// Now read each context individually
		for (int32 Index = 0;
		Index < NumAdvertisedProperties && Packet.HasOverflow() == false;
			Index++)
		{
			FOnlineSessionSetting Setting;
			Packet >> Key;
			Packet >> Setting;
			SessionSettings.Set(Key, Setting);

#if DEBUG_LAN_BEACON
			UE_LOG_ONLINE(Verbose, TEXT("%s"), *Setting->ToString());
#endif
		}
	}

	// If there was an overflow, treat the string settings/properties as broken
	if (Packet.HasOverflow())
	{
		SessionSettings.Settings.Empty();
		UE_LOG_ONLINE(Verbose, TEXT("Packet overflow detected in ReadGameSettingsFromPacket()"));
	}
}

void FOnlineSessionLeet::OnValidResponsePacketReceived(uint8* PacketData, int32 PacketLength)
{
	// Create an object that we'll copy the data to
	FOnlineSessionSettings NewServer;
	if (CurrentSessionSearch.IsValid())
	{
		// Add space in the search results array
		FOnlineSessionSearchResult* NewResult = new (CurrentSessionSearch->SearchResults) FOnlineSessionSearchResult();
		// this is not a correct ping, but better than nothing
		NewResult->PingInMs = static_cast<int32>((FPlatformTime::Seconds() - SessionSearchStartInSeconds) * 1000);

		FOnlineSession* NewSession = &NewResult->Session;

		// Prepare to read data from the packet
		FNboSerializeFromBufferLeet Packet(PacketData, PacketLength);

		ReadSessionFromPacket(Packet, NewSession);

		// NOTE: we don't notify until the timeout happens
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to create new online game settings object"));
	}
}

uint32 FOnlineSessionLeet::FinalizeLANSearch()
{
	if (LANSessionManager.GetBeaconState() == ELanBeaconState::Searching)
	{
		LANSessionManager.StopLANSession();
	}

	return UpdateLANStatus();
}

void FOnlineSessionLeet::OnLANSearchTimeout()
{
	FinalizeLANSearch();

	if (CurrentSessionSearch.IsValid())
	{
		// Allow game code to sort the servers
		CurrentSessionSearch->SortSearchResults();
		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;

		CurrentSessionSearch = NULL;
	}

	// Trigger the delegate as complete
	TriggerOnFindSessionsCompleteDelegates(true);
}

int32 FOnlineSessionLeet::GetNumSessions()
{
	FScopeLock ScopeLock(&SessionLock);
	return Sessions.Num();
}

void FOnlineSessionLeet::DumpSessionState()
{
	FScopeLock ScopeLock(&SessionLock);

	for (int32 SessionIdx = 0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		DumpNamedSession(&Sessions[SessionIdx]);
	}
}

void FOnlineSessionLeet::RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::Success);
}

void FOnlineSessionLeet::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, true);
}

void FOnlineSessionLeet::SetPortFromNetDriver(const FOnlineSubsystemLeet& Subsystem, const TSharedPtr<FOnlineSessionInfo>& SessionInfo)
{
	auto NetDriverPort = GetPortFromNetDriver(Subsystem.GetInstanceName());
	auto SessionInfoLeet = StaticCastSharedPtr<FOnlineSessionInfoLeet>(SessionInfo);
	if (SessionInfoLeet.IsValid() && SessionInfoLeet->HostAddr.IsValid())
	{
		SessionInfoLeet->HostAddr->SetPort(NetDriverPort);
	}
}

bool FOnlineSessionLeet::IsHost(const FNamedOnlineSession& Session) const
{
	if (LeetSubsystem->IsDedicated())
	{
		return true;
	}

	IOnlineIdentityPtr IdentityInt = LeetSubsystem->GetIdentityInterface();
	if (!IdentityInt.IsValid())
	{
		return false;
	}

	TSharedPtr<const FUniqueNetId> UserId = IdentityInt->GetUniquePlayerId(Session.HostingPlayerNum);
	return (UserId.IsValid() && (*UserId == *Session.OwningUserId));
}
