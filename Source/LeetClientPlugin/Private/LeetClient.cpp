#include "LeetClientPluginPrivatePCH.h"
#include "LeetClient.h"

// Define all static member variables.
ULeetClient * ULeetClient::_instance = nullptr;

// Constructor for UCLASS
ULeetClient::ULeetClient(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Client CONSTRUCT"));
	if (_instance == NULL) {
		UE_LOG(LogTemp, Log, TEXT("[LEET] Client CONSTRUCT _instance == NULL"));
		_instance = this;
	}
	
	//_instance->AddToRoot();

	
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

void ULeetClient::initialize(const FString& _apiUrl,
	const FString& _serverSecret,
	const FString& _serverKey)
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Client INIT"));
	apiUrl = _apiUrl;
	serverSecret = _serverSecret;
	serverKey = _serverKey;
}

ULeetClient * ULeetClient::getInstance()
{
	UE_LOG(LogTemp, Log, TEXT("[LEET] Client getInstance"));

	if (_instance == NULL) {
		UE_LOG(LogTemp, Log, TEXT("[LEET] Client _instance == NULL"));
	}
	return _instance;
}

void ULeetClient::BeginDestroy()
{
	_instance = NULL;
}
