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

	void initialize(FString api_url,
		const FString& server_secret,
		const FString& server_key);

	static ULeetClient * getInstance();
	//ULeetClient * getInstance();

	//bool GetServerInfo(FString session_host_address, FString session_id);
	//void GetServerInfoComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);


protected:

	// For non-UCLASS only
	//ULeetClient();
	//~ULeetClient();

	//UPROPERTY()
	//ULeetClient * _instance;

	static ULeetClient * _instance;

	//UClass *_instance = ULeetClient::StaticClass();

};
