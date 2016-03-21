// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemLeetPackage.h"

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionLeet, ESPMode::ThreadSafe> FOnlineSessionLeetPtr;
typedef TSharedPtr<class FOnlineProfileLeet, ESPMode::ThreadSafe> FOnlineProfileLeetPtr;
typedef TSharedPtr<class FOnlineFriendsLeet, ESPMode::ThreadSafe> FOnlineFriendsLeetPtr;
typedef TSharedPtr<class FOnlineUserCloudLeet, ESPMode::ThreadSafe> FOnlineUserCloudLeetPtr;
typedef TSharedPtr<class FOnlineLeaderboardsLeet, ESPMode::ThreadSafe> FOnlineLeaderboardsLeetPtr;
typedef TSharedPtr<class FOnlineVoiceImpl, ESPMode::ThreadSafe> FOnlineVoiceImplPtr;
typedef TSharedPtr<class FOnlineExternalUILeet, ESPMode::ThreadSafe> FOnlineExternalUILeetPtr;
typedef TSharedPtr<class FOnlineIdentityLeet, ESPMode::ThreadSafe> FOnlineIdentityLeetPtr;
typedef TSharedPtr<class FOnlineAchievementsLeet, ESPMode::ThreadSafe> FOnlineAchievementsLeetPtr;

class ULeetClient;

/**
 *	OnlineSubsystemLeet - Implementation of the online subsystem for Leet services
 */
class ONLINESUBSYSTEMLEET_API FOnlineSubsystemLeet :
	public FOnlineSubsystemImpl
{

public:

	virtual ~FOnlineSubsystemLeet()
	{
	}

	ULeetClient* GetClient();

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStorePtr GetStoreInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
    virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	// FTickerObjectBase

	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemLeet

	/**
	 * Is the Leet API available for use
	 * @return true if Leet functionality is available, false otherwise
	 */
	bool IsEnabled();

	//LeetClient* GetClient();

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemLeet(FName InInstanceName) :
		FOnlineSubsystemImpl(InInstanceName),
		SessionInterface(NULL),
		VoiceInterface(NULL),
		LeaderboardsInterface(NULL),
		IdentityInterface(NULL),
		AchievementsInterface(NULL),
		OnlineAsyncTaskThreadRunnable(NULL),
		OnlineAsyncTaskThread(NULL)
	{}

	FOnlineSubsystemLeet() :
		SessionInterface(NULL),
		VoiceInterface(NULL),
		LeaderboardsInterface(NULL),
		IdentityInterface(NULL),
		AchievementsInterface(NULL),
		OnlineAsyncTaskThreadRunnable(NULL),
		OnlineAsyncTaskThread(NULL)
	{}

private:

	/** Interface to the session services */
	FOnlineSessionLeetPtr SessionInterface;

	/** Interface for voice communication */
	FOnlineVoiceImplPtr VoiceInterface;

	/** Interface to the leaderboard services */
	FOnlineLeaderboardsLeetPtr LeaderboardsInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityLeetPtr IdentityInterface;

	/** Interface for achievements */
	FOnlineAchievementsLeetPtr AchievementsInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerLeet* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	//LeetClient* _clientPtr = nullptr;
	FString _configPath = "";

	ULeetClient* _clientPtr = nullptr;
};

typedef TSharedPtr<FOnlineSubsystemLeet, ESPMode::ThreadSafe> FOnlineSubsystemLeetPtr;
