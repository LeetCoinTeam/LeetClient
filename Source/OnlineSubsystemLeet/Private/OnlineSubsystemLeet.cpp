// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemLeetPrivatePCH.h"
#include "OnlineSubsystemLeet.h"
#include "OnlineAsyncTaskManagerLeet.h"
//#include "../LeetClientPlugin/Public/LeetClient.h"
//#include "LeetClient.h"
#include "OnlineSessionInterfaceLeet.h"
#include "OnlineLeaderboardInterfaceLeet.h"
#include "OnlineIdentityLeet.h"
#include "VoiceInterfaceImpl.h"
#include "OnlineAchievementsInterfaceLeet.h"

IOnlineSessionPtr FOnlineSubsystemLeet::GetSessionInterface() const
{
	return SessionInterface;
}

IOnlineFriendsPtr FOnlineSubsystemLeet::GetFriendsInterface() const
{
	return nullptr;
}

IOnlinePartyPtr FOnlineSubsystemLeet::GetPartyInterface() const
{
	return nullptr;
}

IOnlineGroupsPtr FOnlineSubsystemLeet::GetGroupsInterface() const
{
	return nullptr;
}

IOnlineSharedCloudPtr FOnlineSubsystemLeet::GetSharedCloudInterface() const
{
	return nullptr;
}

IOnlineUserCloudPtr FOnlineSubsystemLeet::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FOnlineSubsystemLeet::GetEntitlementsInterface() const
{
	return nullptr;
};

IOnlineLeaderboardsPtr FOnlineSubsystemLeet::GetLeaderboardsInterface() const
{
	return LeaderboardsInterface;
}

IOnlineVoicePtr FOnlineSubsystemLeet::GetVoiceInterface() const
{
	return VoiceInterface;
}

IOnlineExternalUIPtr FOnlineSubsystemLeet::GetExternalUIInterface() const
{
	return nullptr;
}

IOnlineTimePtr FOnlineSubsystemLeet::GetTimeInterface() const
{
	return nullptr;
}

IOnlineIdentityPtr FOnlineSubsystemLeet::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineTitleFilePtr FOnlineSubsystemLeet::GetTitleFileInterface() const
{
	return nullptr;
}

IOnlineStorePtr FOnlineSubsystemLeet::GetStoreInterface() const
{
	return nullptr;
}

IOnlineEventsPtr FOnlineSubsystemLeet::GetEventsInterface() const
{
	return nullptr;
}

IOnlineAchievementsPtr FOnlineSubsystemLeet::GetAchievementsInterface() const
{
	return AchievementsInterface;
}

IOnlineSharingPtr FOnlineSubsystemLeet::GetSharingInterface() const
{
	return nullptr;
}

IOnlineUserPtr FOnlineSubsystemLeet::GetUserInterface() const
{
	return nullptr;
}

IOnlineMessagePtr FOnlineSubsystemLeet::GetMessageInterface() const
{
	return nullptr;
}

IOnlinePresencePtr FOnlineSubsystemLeet::GetPresenceInterface() const
{
	return nullptr;
}

IOnlineChatPtr FOnlineSubsystemLeet::GetChatInterface() const
{
	return nullptr;
}

IOnlineTurnBasedPtr FOnlineSubsystemLeet::GetTurnBasedInterface() const
{
	return nullptr;
}

bool FOnlineSubsystemLeet::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->Tick(DeltaTime);
	}

	if (VoiceInterface.IsValid())
	{
		VoiceInterface->Tick(DeltaTime);
	}

	return true;
}

bool FOnlineSubsystemLeet::Init()
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Subsystem INIT"));
	const bool bLeetInit = true;

	_configPath = FPaths::SourceConfigDir();
	_configPath += TEXT("LeetConfig.ini");

	UE_LOG(LogTemp, Log, TEXT("[LEET] Online Subsystem set up path"));

	if (FPaths::FileExists(_configPath))
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] Online Subsystem File Exists"));
		FConfigSection* Configs = GConfig->GetSectionPrivate(TEXT("Leet.Client"), false, true, _configPath);
		if (Configs)
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] Online Subsystem Configs Exist"));
			FString test = *Configs->Find(TEXT("APIURL"));

			ULeetClient* leetClient = ULeetClient::getInstance();

			UE_LOG(LogTemp, Log, TEXT("[LEET] Online Subsystem Got client"));

			if (leetClient == NULL) {
				UE_LOG(LogTemp, Log, TEXT("[LEET] Online Subsystem Got client but it is NULL"));
			}
			else {
				ULeetClient::getInstance()->initialize(
					*Configs->Find(TEXT("APIURL")),
					*Configs->Find(TEXT("ServerSecret")),
					*Configs->Find(TEXT("ServerAPIKey")));
			}
			
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

	if (bLeetInit)
	{


		// Create the online async task thread
		OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerLeet(this);
		check(OnlineAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadLeet %s"), *InstanceName.ToString()), 128 * 1024, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

		SessionInterface = MakeShareable(new FOnlineSessionLeet(this));
		LeaderboardsInterface = MakeShareable(new FOnlineLeaderboardsLeet(this));
		IdentityInterface = MakeShareable(new FOnlineIdentityLeet(this));
		AchievementsInterface = MakeShareable(new FOnlineAchievementsLeet(this));
		VoiceInterface = MakeShareable(new FOnlineVoiceImpl(this));
		if (!VoiceInterface->Init())
		{
			VoiceInterface = nullptr;
		}

		//_clientPtr = LeetClient::getInstance();
	}
	else
	{
		Shutdown();
	}

	return bLeetInit;
}

bool FOnlineSubsystemLeet::Shutdown()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemLeet::Shutdown()"));

	FOnlineSubsystemImpl::Shutdown();

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}

	if (VoiceInterface.IsValid())
	{
		VoiceInterface->Shutdown();
	}

#define DESTRUCT_INTERFACE(Interface) \
 	if (Interface.IsValid()) \
 	{ \
 		ensure(Interface.IsUnique()); \
 		Interface = nullptr; \
 	}

	// Destruct the interfaces
	DESTRUCT_INTERFACE(VoiceInterface);
	DESTRUCT_INTERFACE(AchievementsInterface);
	DESTRUCT_INTERFACE(IdentityInterface);
	DESTRUCT_INTERFACE(LeaderboardsInterface);
	DESTRUCT_INTERFACE(SessionInterface);

#undef DESTRUCT_INTERFACE

	return true;
}

FString FOnlineSubsystemLeet::GetAppId() const
{
	return TEXT("");
}

bool FOnlineSubsystemLeet::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

bool FOnlineSubsystemLeet::IsEnabled()
{
	return true;
}

ULeetClient* FOnlineSubsystemLeet::GetClient()
{
	return _clientPtr;
}