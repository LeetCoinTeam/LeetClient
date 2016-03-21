// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeetClient.generated.h"

// Forward declarations go here

/**
 * Example of declaring a UObject in a plugin module
 */
UCLASS()
class LEETCLIENTPLUGIN_API ULeetClient : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	static ULeetClient * getInstance();

	void initialize(const FString& apiUrl,
		const FString& serverSecret,
		const FString& serverKey);

	bool GetServerInfo(FString sessionHostAddress, FString sessionID);
	//void GetServerInfoComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

private:

	FString apiUrl;
	FString serverSecret;
	FString serverKey;

	int32 incrementBTC;
	int32 minimumBTCHold;
	float serverRakeBTCPercentage;
	float leetRakePercentage;
	int32 killRewardBTC;

protected:

	//ULeetClient();
	//~ULeetClient();
	static ULeetClient * _instance;

};
