#include "LeetClientPluginPrivatePCH.h"
#include "LeetClient.h"

ULeetClient::ULeetClient(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	//Reset();
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

ULeetClient * ULeetClient::_instance = nullptr;


ULeetClient * ULeetClient::getInstance()
{
	if (_instance == NULL) {
		_instance = new ULeetClient();
	}
	return _instance;
}