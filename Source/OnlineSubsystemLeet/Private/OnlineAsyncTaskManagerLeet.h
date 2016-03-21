// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineAsyncTaskManager.h"

/**
 *	Leet version of the async task manager to register the various Leet callbacks with the engine
 */
class FOnlineAsyncTaskManagerLeet : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemLeet* LeetSubsystem;

public:

	FOnlineAsyncTaskManagerLeet(class FOnlineSubsystemLeet* InOnlineSubsystem)
		: LeetSubsystem(InOnlineSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerLeet()
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
};
