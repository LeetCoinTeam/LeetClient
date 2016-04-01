// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemLeetPrivatePCH.h"
#include "OnlineSubsystemLeet.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE(FOnlineSubsystemLeetModule, OnlineSubsystemLeet);

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryLeet : public IOnlineFactory
{
public:

	FOnlineFactoryLeet() {}
	virtual ~FOnlineFactoryLeet() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemLeetPtr OnlineSub = MakeShareable(new FOnlineSubsystemLeet(InstanceName));
		if (OnlineSub->IsEnabled())
		{
			if(!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("Leet API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Leet API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemLeetModule::StartupModule()
{
	LeetFactory = new FOnlineFactoryLeet();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(LEET_SUBSYSTEM, LeetFactory);
}

void FOnlineSubsystemLeetModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(LEET_SUBSYSTEM);

	delete LeetFactory;
	LeetFactory = NULL;
}
