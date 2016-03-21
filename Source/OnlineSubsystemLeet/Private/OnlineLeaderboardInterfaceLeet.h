// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineLeaderboardInterface.h"
#include "OnlineSubsystemLeetTypes.h"
#include "OnlineSubsystemLeetPackage.h"

/**
 * Interface definition for the online services leaderboard services
 */
class FOnlineLeaderboardsLeet : public IOnlineLeaderboards
{
private:

	/** Internal representation of a leadboard */
	struct FLeaderboardLeet : public FOnlineLeaderboardRead
	{
		/**
		 *	Retrieve a single record from the leaderboard for a given user
		 *
		 * @param UserId user id to retrieve a record for
		 * @return the requested user row or NULL if not found
		 */
		FOnlineStatsRow* FindOrCreatePlayerRecord(const FUniqueNetId& UserId)
		{
			FOnlineStatsRow* Row = FindPlayerRecord(UserId);
			if (Row == NULL)
			{
				// cannot have a better nickname here
				FOnlineStatsRow NewRow(UserId.ToString(), MakeShareable(new FUniqueNetIdString(UserId)));
				NewRow.Rank = -1;
				Rows.Add(NewRow);
			}

			check(FindPlayerRecord(UserId));
			return FindPlayerRecord(UserId);
		}
	};

	/** Reference to the main Leet subsystem */
	class FOnlineSubsystemLeet* LeetSubsystem;

	/** Leaderboards maintained by the subsystem */
	TMap<FName, FLeaderboardLeet> Leaderboards;

	FOnlineLeaderboardsLeet() :
		LeetSubsystem(NULL)
	{
	}

	/**
	 * Creates a Leet leaderboard
	 *
	 * If the leaderboard already exists, the leaderboard data will still be retrieved
	 * @param LeaderboardName name of leaderboard to create
	 * @param SortMethod method the leaderboard scores will be sorted, ignored if leaderboard exists
	 * @param DisplayFormat type of data the leaderboard represents, ignored if leaderboard exists
	 */
	FLeaderboardLeet* FindOrCreateLeaderboard(const FName& LeaderboardName, ELeaderboardSort::Type SortMethod, ELeaderboardFormat::Type DisplayFormat);

PACKAGE_SCOPE:

	FOnlineLeaderboardsLeet(FOnlineSubsystemLeet* InLeetSubsystem) :
		LeetSubsystem(InLeetSubsystem)
	{
	}

public:

	virtual ~FOnlineLeaderboardsLeet() {};

	// IOnlineLeaderboards
	virtual bool ReadLeaderboards(const TArray< TSharedRef<const FUniqueNetId> >& Players, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual void FreeStats(FOnlineLeaderboardRead& ReadObject) override;
	virtual bool WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject) override;
	virtual bool FlushLeaderboards(const FName& SessionName) override;
	virtual bool WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores) override;
};

typedef TSharedPtr<FOnlineLeaderboardsLeet, ESPMode::ThreadSafe> FOnlineLeaderboardsLeetPtr;
