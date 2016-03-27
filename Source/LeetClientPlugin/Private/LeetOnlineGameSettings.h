// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 *
 */
 /**
 * General session settings for a Shooter game
 */
class FLeetOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FLeetOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);
	virtual ~FLeetOnlineSessionSettings() {}
};

/**
* General search setting for a Shooter game
*/
class FLeetOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FLeetOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FLeetOnlineSearchSettings() {}
};

/**
* Search settings for an empty dedicated server to host a match
*/
class FLeetOnlineSearchSettingsEmptyDedicated : public FLeetOnlineSearchSettings
{
public:
	FLeetOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FLeetOnlineSearchSettingsEmptyDedicated() {}
};
