// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
//#include "Core.h"
#include "Http.h"
#include "Json.h"
#include "OnlineSubsystemLeetModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystem.h"
//#include "LeetGameInstance.h"
#include "Networking.h"
#include "ModuleManager.h"

#define INVALID_INDEX -1

/** FName declaration of Leet subsystem */
#define LEET_SUBSYSTEM FName(TEXT("Leet"))
/** URL Prefix when using Leet socket connection */
#define LEET_URL_PREFIX TEXT("Leet.")

/** pre-pended to all NULL logging */
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("LEET: ")
