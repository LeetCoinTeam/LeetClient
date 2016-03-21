// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NboSerializer.h"
#include "OnlineSubsystemLeetTypes.h"

/**
 * Serializes data in network byte order form into a buffer
 */
class FNboSerializeToBufferLeet : public FNboSerializeToBuffer
{
public:
	/** Default constructor zeros num bytes*/
	FNboSerializeToBufferLeet() :
		FNboSerializeToBuffer(512)
	{
	}

	/** Constructor specifying the size to use */
	FNboSerializeToBufferLeet(uint32 Size) :
		FNboSerializeToBuffer(Size)
	{
	}

	/**
	 * Adds Leet session info to the buffer
	 */
 	friend inline FNboSerializeToBufferLeet& operator<<(FNboSerializeToBufferLeet& Ar, const FOnlineSessionInfoLeet& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar << SessionInfo.SessionId;
		Ar << *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Adds Leet Unique Id to the buffer
	 */
	friend inline FNboSerializeToBufferLeet& operator<<(FNboSerializeToBufferLeet& Ar, const FUniqueNetIdString& UniqueId)
	{
		Ar << UniqueId.UniqueNetIdStr;
		return Ar;
	}
};

/**
 * Class used to write data into packets for sending via system link
 */
class FNboSerializeFromBufferLeet : public FNboSerializeFromBuffer
{
public:
	/**
	 * Initializes the buffer, size, and zeros the read offset
	 */
	FNboSerializeFromBufferLeet(uint8* Packet,int32 Length) :
		FNboSerializeFromBuffer(Packet,Length)
	{
	}

	/**
	 * Reads Leet session info from the buffer
	 */
 	friend inline FNboSerializeFromBufferLeet& operator>>(FNboSerializeFromBufferLeet& Ar, FOnlineSessionInfoLeet& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar >> SessionInfo.SessionId;
		Ar >> *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Reads Leet Unique Id from the buffer
	 */
	friend inline FNboSerializeFromBufferLeet& operator>>(FNboSerializeFromBufferLeet& Ar, FUniqueNetIdString& UniqueId)
	{
		Ar >> UniqueId.UniqueNetIdStr;
		return Ar;
	}
};
