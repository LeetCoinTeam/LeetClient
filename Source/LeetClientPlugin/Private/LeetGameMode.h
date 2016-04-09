// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/GameMode.h"
//#include "LeetGameSession.h"
//#include "../../LEETAPIDEMO4/Plugins/LeetClient/Source/LeetClientPlugin/Private/LeetGameSession.h"
//#include "../../LEETAPIDEMO4/Plugins/LeetClient/Source/LeetClientPlugin/Private/LeetGameInstance.h"
#include "LeetGameMode.generated.h"

UCLASS()
class LEETCLIENTPLUGIN_API ALeetGameMode : public AGameMode
{
	GENERATED_BODY()


protected:
	/** Returns game session class to use */
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;

	/**
	* Customize incoming player based on URL options
	*
	* @param NewPlayerController player logging in
	* @param UniqueId unique id for this player
	* @param Options URL options that came at login
	*
	*/
	virtual FString InitNewPlayer(class APlayerController* NewPlayerController, const TSharedPtr<const FUniqueNetId>& UniqueId, const FString& Options, const FString& Portal = TEXT(""));


public:
	ALeetGameMode();
	virtual void PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage) override;

	void Logout(AController* Exiting);
};
