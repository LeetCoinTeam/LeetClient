// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "LeetClient.generated.h"

// Forward declarations go here

/**
 * Example of declaring a UObject in a plugin module
 */


//UCLASS()
//class LEETCLIENTPLUGIN_API ULeetClient : public UObject
class ULeetClient 
{
	//GENERATED_UCLASS_BODY()

	//virtual void BeginDestroy() override;

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

protected:

	ULeetClient();
	~ULeetClient();
	static ULeetClient * _instance;
	//ULeetClient * _instance;

};
