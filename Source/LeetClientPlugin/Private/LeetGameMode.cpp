// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LeetClientPluginPrivatePCH.h"
//#include "LEETAPIDEMO5GameMode.h"
#include "LeetPawn.h"

//#include "../../LEETAPIDEMO4/Plugins/LeetClient/Source/LeetClientPlugin/Private/LeetGameSession.h"
//#include "../../LEETAPIDEMO4/Plugins/LeetClient/Source/LeetClientPlugin/Private/LeetGameInstance.h"

ALeetGameMode::ALeetGameMode()
{
	// set default pawn class to our flying pawn
	DefaultPawnClass = ALeetPawn::StaticClass();
}

/** Returns game session class to use */
TSubclassOf<AGameSession> ALeetGameMode::GetGameSessionClass() const
{
	return ALeetGameSession::StaticClass();
	//return AMyGameSession::StaticClass();
}

void ALeetGameMode::PreLogin(const FString& Options, const FString& Address, const TSharedPtr<const FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] PreLogin"));
	/*
	Ideally we could do activate here, but we don't have the playerID
	*/

}

FString ALeetGameMode::InitNewPlayer(APlayerController* NewPlayerController, const TSharedPtr<const FUniqueNetId>& UniqueId, const FString& Options, const FString& Portal)
{

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] InitNewPlayer"));

	check(NewPlayerController);
	int32 playerId = NewPlayerController->PlayerState->PlayerId;

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] InitNewPlayer playerId: %d"), playerId);
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] InitNewPlayer Options: %s"), *Options);

	FString Name = UGameplayStatics::ParseOption(Options, TEXT("Name"));

	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] InitNewPlayer Param1: %s"), *Name);

	FString ErrorMessage;

	/* At this point we want to be able to deny the Login, but we need to do it over http, which requires a delegate.
	// So we're just going to allow login, then kick them if the http request comes up negative
	// TODO find a better way to do this - it should probably be part of onlineSubsystem.
	*/
	ULeetGameInstance* TheGameInstance = Cast<ULeetGameInstance>(GetWorld()->GetGameInstance());
	TheGameInstance->ActivatePlayer(Name, playerId);

	// Register the player with the session
	GameSession->RegisterPlayer(NewPlayerController, UniqueId, UGameplayStatics::HasOption(Options, TEXT("bIsFromInvite")));

	// Init player's name
	FString InName = UGameplayStatics::ParseOption(Options, TEXT("Name")).Left(20);
	if (InName.IsEmpty())
	{
		InName = FString::Printf(TEXT("%s%i"), *DefaultPlayerName.ToString(), NewPlayerController->PlayerState->PlayerId);
	}

	ChangeName(NewPlayerController, InName, false);

	// Find a starting spot
	AActor* const StartSpot = FindPlayerStart(NewPlayerController, Portal);
	if (StartSpot != NULL)
	{
		// Set the player controller / camera in this new location
		FRotator InitialControllerRot = StartSpot->GetActorRotation();
		InitialControllerRot.Roll = 0.f;
		NewPlayerController->SetInitialLocationAndRotation(StartSpot->GetActorLocation(), InitialControllerRot);
		NewPlayerController->StartSpot = StartSpot;
	}
	else
	{
		ErrorMessage = FString::Printf(TEXT("Failed to find PlayerStart"));
	}

	return ErrorMessage;
}

void ALeetGameMode::Logout(AController* Exiting)
{
	// Handle users that disconnect from the server.
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] [Logout] "));
	Super::Logout(Exiting);

	//AMyPlayerState* ExitingPlayerState = Cast<AMyPlayerState>(Exiting->PlayerState);
	//int ExitingPlayerId = ExitingPlayerState->PlayerId;

	//UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetGameMode] [Logout] ExitingPlayerId: %i"), ExitingPlayerId);

	ULeetGameInstance* TheGameInstance = Cast<ULeetGameInstance>(GetWorld()->GetGameInstance());

	//TheGameInstance->DeAuthorizePlayer(ExitingPlayerId);

}