#include "LeetClientPluginPrivatePCH.h"
#include "LeetClient.h"

// Define all static member variables.
ULeetClient * ULeetClient::_instance = nullptr;

// Constructor for UCLASS
ULeetClient::ULeetClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Client CONSTRUCT"));
	if (_instance == NULL) {
		UE_LOG(LogTemp, Log, TEXT("[LEET] Client CONSTRUCT _instance == NULL"));
		_instance = this;
		//_instance->AddToRoot();
	}

	//Reset();
}

/**
* Constructor for non-UCLASS
*/
//ULeetClient::ULeetClient()
//{
//
//}

/**
* Destructor for non-UCLASS
*/
//ULeetClient::~ULeetClient()
//{
//	_instance = nullptr;
//}

void ULeetClient::initialize(FString api_url,
	const FString& server_secret,
	const FString& server_key)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Client INIT"));
	apiUrl = api_url;
	serverSecret = server_secret;
	serverKey = server_key;
}

ULeetClient * ULeetClient::getInstance()
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Client getInstance"));
	if (_instance == NULL) {
		UE_LOG(LogTemp, Log, TEXT("[LEET] Client getInstance _instance == NULL"));
	}
	return _instance;
}

void ULeetClient::BeginDestroy()
{
	_instance = NULL;
}
