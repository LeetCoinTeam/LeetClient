// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeetClient.generated.h"

// Forward declarations go here

/**
 * Example of declaring a UObject in a plugin module
 */


// hack to try to get non-UCLASS working
//typedef TSharedPtr<class IHttpRequest> FHttpRequestPtr;
//typedef TSharedPtr<class IHttpResponse, ESPMode::ThreadSafe> FHttpResponsePtr;


//class ULeetClient
UCLASS()
class LEETCLIENTPLUGIN_API ULeetClient : public UObject
{
	// FOR UCLASS only
	GENERATED_UCLASS_BODY()

	virtual void BeginDestroy() override;

private:

	FString apiUrl;
	FString serverSecret;
	FString serverKey;

	int32 incrementBTC;
	int32 minimumBTCHold;
	float serverRakeBTCPercentage;
	float leetRakePercentage;
	int32 killRewardBTC;

public:

	void initialize(const FString& apiUrl,
		const FString& serverSecret,
		const FString& serverKey);

	static ULeetClient * getInstance();

	//bool GetServerInfo(FString session_host_address, FString session_id);
	//void GetServerInfoComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);


protected:

	// For non-UCLASS only
	//ULeetClient();
	//~ULeetClient();

	static ULeetClient * _instance;

};
