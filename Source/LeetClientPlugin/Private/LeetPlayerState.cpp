// Fill out your copyright notice in the Description page of Project Settings.

#include "LeetClientPluginPrivatePCH.h"
// Include the engine for casting
//#include "Engine.h"
// Incude our game mode and instance and state
// This is supposed to be last for some voodoo
#include "LeetPlayerState.h"


void ALeetPlayerState::BroadcastChatMessage_Implementation( const FText& ChatMessageIn)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation "));

	// Use Game Instance because it does not get erased on level change
	// Get the game instance and cast to our game instance.
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation - get game instance "));
	ULeetGameInstance* TheGameInstance = Cast<ULeetGameInstance>(GetWorld()->GetGameInstance());

	// Get the activePlayer record
	FLeetActivePlayer* playerRecord = TheGameInstance->getPlayerByPlayerId(PlayerId);

	//Check to see if it was a / command, in which case we can process it here.
	FString ChatMessageInString = ChatMessageIn.ToString();
	if (ChatMessageInString.StartsWith("/"))
	{
		UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation - Found slash command "));
		if (ChatMessageInString.StartsWith("/balance"))
		{
			UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation -  /balance  "));
			FString commandResponseSender = "SYSTEM";
			FText ChatSenderSYS = FText::FromString(commandResponseSender);
			
			FString playerBalance = FString::FromInt(playerRecord->BTCHold);
			FString commandResponse = "Balance: " + playerBalance;

			// old way
			//FString playerBalance = FString::FromInt(TheGameInstance->getPlayerBalanceByPlayerID(PlayerId));

			FText ChatMessageOut = FText::FromString(commandResponse);
			ReceiveChatMessage(ChatSenderSYS, ChatMessageOut);
			
		}
	}
	else
	{


		//Send the message upstream to the LEET API
		//TheGameInstance->OutgoingChat(PlayerId, ChatMessageIn);

		UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation Looping over Player controllers "));
		// Trying the chatacter controller iterator instead of playerarray
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			// old way
			/*
			UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation Casting to MyPlayerController "));
			AMyPlayerController* Controller = Cast<AMyPlayerController>(*Iterator);
			UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation Getting PlayerState "));
			AMyPlayerState* pstate = Cast<AMyPlayerState>(Controller->PlayerState);
			// Go through the activelist to grab our playerName
			UMyGameInstance* TheGameInstance = Cast<UMyGameInstance>(GetWorld()->GetGameInstance());
			FString thePlayerTitle = TheGameInstance->getPlayerTitleByPlayerID(PlayerId);
			*/

			FText thePlayerName = FText::FromString(playerRecord->playerTitle);
			this->ReceiveChatMessage(thePlayerName, ChatMessageIn);
		
		}
	}


	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] BroadcastChatMessage_Implementation Done. "));
}

bool ALeetPlayerState::BroadcastChatMessage_Validate(const FText& ChatMessageIn)
{
	UE_LOG(LogTemp, Log, TEXT("Validate"));
	return true;
}

void ALeetPlayerState::ReceiveChatMessage_Implementation(const FText& ChatSender, const FText& ChatMessageD)
{
	// do stuff on client here
	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] ReceiveChatMessage_Implementation "));
	//UE_LOG(LogTemp, Log, TEXT("[LEET] [AMyPlayerState] ReceiveChatMessage_Implementation Broadcasting Delegate"));
	OnTextDelegate.Broadcast(ChatSender, ChatMessageD);


	UE_LOG(LogTemp, Log, TEXT("[LEET] [ALeetPlayerState] ReceiveChatMessage_Implementation Done."));
}
