#include "LeetClientPluginPrivatePCH.h"
#include "LeetClient.h"

// Define all static member variables.
ULeetClient * ULeetClient::_instance = nullptr;

// Constructor for UCLASS
ULeetClient::ULeetClient(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	//UE_LOG(LogTemp, Log, TEXT("[LEET] Client CONSTRUCT"));
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

		// Trying different ways to return the UCLASS instance
		//_instance = NewObject<ULeetClient>(nullptr, ULeetClient*);
		//ULeetClient* leetclient = NewObject<ULeetClient>(nullptr);  // Assertion fail
		//ULeetClient* leetclient = NewObject<ULeetClient>(ULeetClient::StaticClass()); // FATAL ERROR
		//ULeetClient* leetclient = NewObject<ULeetClient>(nullptr, ULeetClient::StaticClass()); // Assertion Failed
		_instance = NewObject<ULeetClient>(ULeetClient::StaticClass()); // FATAL ERROR

		//UClass *LeetClientclass = ULeetClient::StaticClass();
		//_instance = NewObject<ULeetClient>(LeetClientclass); // FATAL ERROR
		//_instance = (ULeetClient*)ConstructObject<UObject>(LeetClientclass); // Assertion fail



		// THis worked before it was a UClass.
		//_instance = new ULeetClient();
	}
	return _instance;
}

void ULeetClient::BeginDestroy()
{
	_instance = NULL;
}

