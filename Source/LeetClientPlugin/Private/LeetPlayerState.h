// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerState.h"
#include "Engine.h"
#include "LeetPlayerState.generated.h"

/**
 *
 */

USTRUCT()
struct FLeetInventoryItemStat {

	GENERATED_USTRUCT_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString itemStatKey;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		int32 itemStatValue;

};

USTRUCT()
struct FLeetInventoryItem {

	GENERATED_USTRUCT_BODY()
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		int32 quantity;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString itemId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		FString itemTitle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		TArray<FLeetInventoryItemStat> InventoryItemStats;

};

USTRUCT()
struct FLeetInventory {

	GENERATED_USTRUCT_BODY()
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LEET")
		TArray<FLeetInventoryItem> InventoryItems;
};

//THis delegate is working.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTextDelegate, FText, chatSender, FText, chatMessage);


UCLASS()
class LEETCLIENTPLUGIN_API ALeetPlayerState : public APlayerState
{
	GENERATED_BODY()

		FLeetInventory Inventory;

public:
	// This is the function that gets called in the widget Blueprint
	UFUNCTION(BlueprintCallable, Category = "LEET", Server, Reliable, WithValidation)
		void BroadcastChatMessage(const FText& ChatMessageIn);

	// This function is called remotely by the server.
	UFUNCTION(NetMulticast, Category = "LEET", Unreliable)
		void ReceiveChatMessage(const FText& ChatSender, const FText& ChatMessage);


	UPROPERTY(BlueprintAssignable)
	FTextDelegate OnTextDelegate;

};
