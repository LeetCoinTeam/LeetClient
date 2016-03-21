// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemLeetPrivatePCH.h"
#include "OnlineAsyncTaskManagerLeet.h"
#include "OnlineSubsystemLeet.h"

void FOnlineAsyncTaskManagerLeet::OnlineTick()
{
	check(LeetSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}
