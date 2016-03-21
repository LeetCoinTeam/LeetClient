#include "LeetClientPluginPrivatePCH.h"
#include "LeetClient.h"

// Define all static member variables.
ULeetClient * ULeetClient::_instance = nullptr;

// Constructor for UCLASS
//ULeetClient::ULeetClient(const class FObjectInitializer& PCIP)
//	: Super(PCIP)
//{
//	//UE_LOG(LogTemp, Log, TEXT("[LEET] Client CONSTRUCT"));
//	//Reset();
//}

/**
* Constructor
*/
ULeetClient::ULeetClient()
{

}

/**
* Destructor
*/
ULeetClient::~ULeetClient()
{
	_instance = nullptr;
}

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
		//_instance = NewObject<ULeetClient>(nullptr, ULeetClient*);
		//_instance = ConstructObject<ULeetClient>(ULeetClient::StaticClass());

		_instance = new ULeetClient();
	}
	return _instance;
}

