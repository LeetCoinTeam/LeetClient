// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemLeetPrivatePCH.h"
#include "OnlineIdentityLeet.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"

bool FUserOnlineAccountLeet::GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = AdditionalAuthData.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountLeet::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountLeet::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr == NULL || *FoundAttr != AttrValue)
	{
		UserAttributes.Add(AttrName, AttrValue);
		return true;
	}
	return false;
}

inline FString GenerateRandomUserId(int32 LocalUserNum)
{
	FString HostName;
	if (!ISocketSubsystem::Get()->GetHostName(HostName))
	{
		// could not get hostname, use address
		bool bCanBindAll;
		TSharedPtr<class FInternetAddr> Addr = ISocketSubsystem::Get()->GetLocalHostAddr(*GLog, bCanBindAll);
		HostName = Addr->ToString(false);
	}

	const bool bForceUniqueId = FParse::Param( FCommandLine::Get(), TEXT( "StableLeetID" ) );

	if ( ( GIsFirstInstance || bForceUniqueId ) && !GIsEditor )
	{
		// When possible, return a stable user id
		return FString::Printf( TEXT( "%s-%s" ), *HostName, *FPlatformMisc::GetMachineId().ToString( EGuidFormats::Digits ) );
	}

	// If we're not the first instance (or in the editor), return truly random id
	return FString::Printf( TEXT( "%s-%s" ), *HostName, *FGuid::NewGuid().ToString() );
}

/**
* Used to do any time based processing of tasks
*
* @param DeltaTime the amount of time that has elapsed since the last tick
*/
void FOnlineIdentityLeet::Tick(float DeltaTime)
{
	// Only tick once per frame
	TickLogin(DeltaTime);
}

/**
* Ticks the registration process handling timeouts, etc.
*
* @param DeltaTime the amount of time that has elapsed since last tick
*/
void FOnlineIdentityLeet::TickLogin(float DeltaTime)
{
	
	if (bHasLoginOutstanding)
	{
		//UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::TickLogin bHasLoginOutstanding"));
		LastCheckElapsedTime += DeltaTime;
		TotalCheckElapsedTime += DeltaTime;
		// See if enough time has elapsed in order to check for completion
		if (LastCheckElapsedTime > 1.f ||
			// Do one last check if we're getting ready to time out
			TotalCheckElapsedTime > MaxCheckElapsedTime)
		{
			LastCheckElapsedTime = 0.f;
			FString Title;
			
			// Find the browser window we spawned which should now be titled with the redirect url
			if (FPlatformMisc::GetWindowTitleMatchingText(*LoginRedirectUrl, Title))
			{
				UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::TickLogin GetWindowTitleMatchingText"));
				bHasLoginOutstanding = false;

				// Parse access token from the login redirect url
				FString AccessToken;
				if (FParse::Value(*Title, TEXT("access_token="), AccessToken) &&
					!AccessToken.IsEmpty())
				{
					UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::TickLogin Found access_token"));
					// strip off any url parameters and just keep the token itself
					FString AccessTokenOnly;
					if (AccessToken.Split(TEXT("&"), &AccessTokenOnly, NULL))
					{
						AccessToken = AccessTokenOnly;
					}
					// kick off http request to get user info with the new token
					TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
					LoginUserRequests.Add(&HttpRequest.Get(), FPendingLoginUser(LocalUserNumPendingLogin, AccessToken));

					FString MeUrl = TEXT("https://leetsandbox.appspot.com/me?access_token=`token");

					HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineIdentityLeet::MeUser_HttpRequestComplete);
					HttpRequest->SetURL(MeUrl.Replace(TEXT("`token"), *AccessToken, ESearchCase::IgnoreCase));
					HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
					HttpRequest->SetVerb(TEXT("GET"));
					HttpRequest->ProcessRequest();
				}
				else
				{
					TriggerOnLoginCompleteDelegates(LocalUserNumPendingLogin, false, FUniqueNetIdString(TEXT("")),
						FString(TEXT("RegisterUser() failed to parse the user registration results")));
				}
			}
			// Trigger the delegate if we hit the timeout limit
			else if (TotalCheckElapsedTime > MaxCheckElapsedTime)
			{
				bHasLoginOutstanding = false;
				TriggerOnLoginCompleteDelegates(LocalUserNumPendingLogin, false, FUniqueNetIdString(TEXT("")),
					FString(TEXT("RegisterUser() timed out without getting the data")));
			}
		}
		// Reset our time trackers if we are done ticking for now
		if (!bHasLoginOutstanding)
		{
			LastCheckElapsedTime = 0.f;
			TotalCheckElapsedTime = 0.f;
		}
	}
}

bool FOnlineIdentityLeet::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::Login"));
	FString ErrorStr;

	if (bHasLoginOutstanding)
	{
		ErrorStr = FString::Printf(TEXT("Registration already pending for user %d"),
			LocalUserNumPendingLogin);
	}
	else if (!(LoginUrl.Len() && LoginRedirectUrl.Len() && ClientId.Len()))
	{
		ErrorStr = FString::Printf(TEXT("OnlineSubsystemLeet is improperly configured in DefaultEngine.ini LeetEndpoint=%s RedirectUrl=%s ClientId=%s"),
			*LoginUrl, *LoginRedirectUrl, *ClientId);
	}
	else
	{
		// random number to represent client generated state for verification on login
		State = FString::FromInt(FMath::Rand() % 100000);
		// auth url to spawn in browser
		const FString& Command = FString::Printf(TEXT("%s?redirect_uri=%s&client_id=%s&state=%s&response_type=token"),
			*LoginUrl, *LoginRedirectUrl, *ClientId, *State);
		UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::Login - %s"), *LoginUrl);
		// This should open the browser with the command as the URL
		if (FPlatformMisc::OsExecute(TEXT("open"), *Command))
		{
			// keep track of local user requesting registration
			LocalUserNumPendingLogin = LocalUserNum;
			bHasLoginOutstanding = true;
		}
		else
		{
			ErrorStr = FString::Printf(TEXT("Failed to execute command %s"),
				*Command);
		}
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG(LogOnline, Error, TEXT("RegisterUser() failed: %s"),
			*ErrorStr);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdString(TEXT("")), ErrorStr);
		return false;
	}
	return true;
}

bool FOnlineIdentityLeet::Logout(int32 LocalUserNum)
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		// remove cached user account
		UserAccounts.Remove(UserId->ToString());
		// remove cached user id
		UserIds.Remove(LocalUserNum);
		// not async but should call completion delegate anyway
		TriggerOnLogoutCompleteDelegates(LocalUserNum, true);

		return true;
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No logged in user found for LocalUserNum=%d."),
			LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
	}
	return false;
}

bool FOnlineIdentityLeet::AutoLogin(int32 LocalUserNum)
{
	return false;
}

/**
* Parses the results into a user account entry
*
* @param Results the string returned by the login process
*/
bool FOnlineIdentityLeet::ParseLoginResults(const FString& Results, FUserOnlineAccountLeet& Account)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::ParseLoginResults 111"));
	UE_LOG_ONLINE(Display, TEXT("Results: %s"), *Results);
	// reset it
	Account = FUserOnlineAccountLeet();
	// get the access token
	if (FParse::Value(*Results, TEXT("access_token="), Account.AuthTicket))
	{
		UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::ParseLoginResults got access_token"));
		return !Account.AuthTicket.IsEmpty();
	}
	return false;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityLeet::GetUserAccount(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> Result;

	const TSharedRef<FUserOnlineAccountLeet>* FoundUserAccount = UserAccounts.Find(UserId.ToString());
	if (FoundUserAccount != NULL)
	{
		Result = *FoundUserAccount;
	}

	return Result;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityLeet::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > Result;

	for (FUserOnlineAccountLeetMap::TConstIterator It(UserAccounts); It; ++It)
	{
		Result.Add(It.Value());
	}

	return Result;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityLeet::GetUniquePlayerId(int32 LocalUserNum) const
{
	const TSharedPtr<const FUniqueNetId>* FoundId = UserIds.Find(LocalUserNum);
	if (FoundId != NULL)
	{
		return *FoundId;
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityLeet::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	if (Bytes != NULL && Size > 0)
	{
		FString StrId(Size, (TCHAR*)Bytes);
		return MakeShareable(new FUniqueNetIdString(StrId));
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityLeet::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdString(Str));
}

ELoginStatus::Type FOnlineIdentityLeet::GetLoginStatus(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityLeet::GetLoginStatus(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid() &&
		UserAccount->GetUserId()->IsValid())
	{
		return ELoginStatus::LoggedIn;
	}
	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityLeet::GetPlayerNickname(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniquePlayerId(LocalUserNum);
	if (UniqueId.IsValid())
	{
		return UniqueId->ToString();
	}

	return TEXT("LeetUser");
}

FString FOnlineIdentityLeet::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	return UserId.ToString();
}

FString FOnlineIdentityLeet::GetAuthToken(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}


/**
* Sets the needed configuration properties
*/
FOnlineIdentityLeet::FOnlineIdentityLeet()
	: LastCheckElapsedTime(0.f)
	, TotalCheckElapsedTime(0.f)
	, MaxCheckElapsedTime(0.f)
	, bHasLoginOutstanding(false)
	, LocalUserNumPendingLogin(0)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::FOnlineIdentityLeet"));
	if (!GConfig->GetString(TEXT("OnlineSubsystemLeet.OnlineIdentityLeet"), TEXT("LoginUrl"), LoginUrl, GEngineIni))
	{
		UE_LOG(LogOnline, Warning, TEXT("Missing LoginUrl= in [OnlineSubsystemLeet.OnlineIdentityLeet] of DefaultEngine.ini"));
	}
	if (!GConfig->GetString(TEXT("OnlineSubsystemLeet.OnlineIdentityLeet"), TEXT("LoginRedirectUrl"), LoginRedirectUrl, GEngineIni))
	{
		UE_LOG(LogOnline, Warning, TEXT("Missing LoginRedirectUrl= in [OnlineSubsystemLeet.OnlineIdentityLeet] of DefaultEngine.ini"));
	}
	if (!GConfig->GetString(TEXT("OnlineSubsystemLeet.OnlineIdentityLeet"), TEXT("ClientId"), ClientId, GEngineIni))
	{
		UE_LOG(LogOnline, Warning, TEXT("Missing ClientId= in [OnlineSubsystemLeet.OnlineIdentityLeet] of DefaultEngine.ini"));
	}
	if (!GConfig->GetFloat(TEXT("OnlineSubsystemLeet.OnlineIdentityLeet"), TEXT("LoginTimeout"), MaxCheckElapsedTime, GEngineIni))
	{
		UE_LOG(LogOnline, Warning, TEXT("Missing LoginTimeout= in [OnlineSubsystemLeet.OnlineIdentityLeet] of DefaultEngine.ini"));
		// Default to 30 seconds
		MaxCheckElapsedTime = 30.f;
	}
}

/*
FOnlineIdentityLeet::FOnlineIdentityLeet(class FOnlineSubsystemLeet* InSubsystem)
{
// autologin the 0-th player
Login(0, FOnlineAccountCredentials(TEXT("DummyType"), TEXT("DummyUser"), TEXT("DummyId")) );
}
*/

/*
FOnlineIdentityLeet::FOnlineIdentityLeet()
{
}
*/

FOnlineIdentityLeet::~FOnlineIdentityLeet()
{
}

void FOnlineIdentityLeet::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FPlatformUserId FOnlineIdentityLeet::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		auto CurrentUniqueId = GetUniquePlayerId(i);
		if (CurrentUniqueId.IsValid() && (*CurrentUniqueId == UniqueNetId))
		{
			return i;
		}
	}

	return PLATFORMUSERID_NONE;
}

FString FOnlineIdentityLeet::GetAuthType() const
{
	return TEXT("");
}

void FOnlineIdentityLeet::MeUser_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityLeet::MeUser_HttpRequestComplete"));
	bool bResult = false;
	FString ResponseStr, ErrorStr;
	FUserOnlineAccountLeet User;

	FPendingLoginUser PendingRegisterUser = LoginUserRequests.FindRef(HttpRequest.Get());
	// Remove the request from list of pending entries
	LoginUserRequests.Remove(HttpRequest.Get());

	if (bSucceeded &&
		HttpResponse.IsValid())
	{
		ResponseStr = HttpResponse->GetContentAsString();
		if (EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
		{
			UE_LOG(LogOnline, Verbose, TEXT("RegisterUser request complete. url=%s code=%d response=%s"),
				*HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *ResponseStr);

			if (User.FromJson(ResponseStr))
			{
				if (!User.UserId.IsEmpty())
				{
					// copy and construct the unique id
					TSharedRef<FUserOnlineAccountLeet> UserRef(new FUserOnlineAccountLeet(User));
					UserRef->UserIdPtr = MakeShareable(new FUniqueNetIdString(User.UserId));
					// update/add cached entry for user
					UserAccounts.Add(User.UserId, UserRef);
					// update the access token
					UserRef->AuthTicket = PendingRegisterUser.AccessToken;
					// keep track of user ids for local users
					UserIds.Add(PendingRegisterUser.LocalUserNum, UserRef->GetUserId());

					bResult = true;
				}
				else
				{
					ErrorStr = FString::Printf(TEXT("Missing user id. payload=%s"),
						*ResponseStr);
				}
			}
			else
			{
				ErrorStr = FString::Printf(TEXT("Invalid response payload=%s"),
					*ResponseStr);
			}
		}
		else
		{
			ErrorStr = FString::Printf(TEXT("Invalid response. code=%d error=%s"),
				HttpResponse->GetResponseCode(), *ResponseStr);
		}
	}
	else
	{
		ErrorStr = TEXT("No response");
	}
	if (!ErrorStr.IsEmpty())
	{
		UE_LOG(LogOnline, Warning, TEXT("RegisterUser request failed. %s"), *ErrorStr);
	}

	TriggerOnLoginCompleteDelegates(PendingRegisterUser.LocalUserNum, bResult, FUniqueNetIdString(User.UserId), ErrorStr);
}