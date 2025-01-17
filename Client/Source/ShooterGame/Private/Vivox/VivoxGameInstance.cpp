/* Copyright (c) 2014-2018 by Mercer Road Corp
 *
 * Permission to use, copy, modify or distribute this software in binary or source form
 * for any purpose is allowed only under explicit prior consent in writing from Mercer Road Corp
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND MERCER ROAD CORP DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MERCER ROAD CORP
 * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "ShooterGame.h"
#include "ShooterGameSession.h"
#include "Online/ShooterPlayerState.h"
#include "Vivox/VivoxGameInstance.h"
#include "Vivox/VivoxToken.h"
#include "Vivox/VivoxTracer.h"
#include "Runtime/Launch/Resources/Version.h"
// EDIT BEGIN
#include "Custom/VivoxTokenProvider.h"
#include "Core/AccelByteMultiRegistry.h"
#include "Api/AccelByteUserApi.h"
// EDIT END

#if ((((ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1) || ENGINE_MAJOR_VERSION == 4) && PLATFORM_PS4) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0 && defined(PLATFORM_PS4)))
#pragma comment (lib, "SceAudioIn_stub_weak")
#endif // PLATFORM_PS4

DEFINE_LOG_CATEGORY_STATIC(LogVivoxGameInstance, Log, All);

#define VIVOX_VOICE_SERVER TEXT("https://GETFROMPORTAL.www.vivox.com/api2")
#define VIVOX_VOICE_DOMAIN TEXT("GET VALUE FROM VIVOX DEVELOPER PORTAL")
#define VIVOX_VOICE_ISSUER TEXT("GET VALUE FROM VIVOX DEVELOPER PORTAL")

/// Return the full name of a UEnum value as an FString
template<typename TEnum>
static FORCEINLINE FString GetUEnumAsString(const FString& Name, TEnum Value, bool ShortName)
{
    const UEnum* enumPtr = FindObject<UEnum>(ANY_PACKAGE, *Name, true);
    if (!enumPtr)
        return FString("InvalidUEnum");
    if (ShortName)
        return enumPtr->GetNameStringByIndex((int64)Value);
    return enumPtr->GetNameByValue((int64)Value).ToString();
}
#define UEnumFullToString(Name, Value) GetUEnumAsString<Name>(#Name, Value, false)
#define UEnumShortToString(Name, Value) GetUEnumAsString<Name>(#Name, Value, true)

bool AreVivoxVoiceChatValuesSet()
{
    FString CheckString("https://GETFROMPORTAL.www.vivox.com/api2");
    if (!CheckString.Compare(VIVOX_VOICE_SERVER)) return false;

    CheckString = FString("GET VALUE FROM VIVOX DEVELOPER PORTAL");
    if (!CheckString.Compare(VIVOX_VOICE_DOMAIN)) return false;
    if (!CheckString.Compare(VIVOX_VOICE_ISSUER)) return false;

    // NB: VIVOX_VOICE_KEY matters too, but this should be
    // sufficient to base creating the warning dialog on.

    return true;
}

UVivoxGameInstance::UVivoxGameInstance(const FObjectInitializer & ObjectInitializer) : UShooterGameInstance(ObjectInitializer)
{
    Tracer::MinorMethodPrologue();
    bInitialized = false;
    bLoggedIn = false;
    bLoggingIn = false;
    VivoxVoiceClient = &static_cast<FVivoxCoreModule *>(&FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore")))->VoiceClient();
}

UVivoxGameInstance::~UVivoxGameInstance()
{
    Tracer::MinorMethodPrologue();
    FModuleManager::Get().UnloadModule(TEXT("VivoxCore"));
}

void UVivoxGameInstance::Init()
{
    Tracer::MinorMethodPrologue();

    Super::Init();

    if (!AreVivoxVoiceChatValuesSet())
    {
        FText TitleText = FText::FromString(FString("Warning: Vivox Portal Credentials Misconfigured!"));
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString("Voice Chat will not work properly until your Developer Portal credential values are defined correctly at the top of 'Vivox/VivoxGameInstance.cpp' AND 'Vivox/VivoxToken.cpp'. See the online documentation 'Vivox Unreal: Developer First Steps' for more info.")), &TitleText);
    }

    // This balances voice chat volume against all the shooting and explosions.
    ChangeSoundClassVolume(0.3F, FSoftObjectPath(TEXT("/Game/Sounds/SoundClassesAndMixes/Master.Master")));

    VivoxCoreError Status = Initialize(1); // Logs Core Errors and Warnings only.

    // If Initialize() fails, force a break in the debugger: in a real game you can just continue without Vivox,
    // but in our sample there's no point continuing without voice and we want to make sure you know about it.
    ensure (Status == VxErrorSuccess);
}

void UVivoxGameInstance::StartGameInstance()
{
    Tracer::MinorMethodPrologue();

    Super::StartGameInstance();

    // Individual Vivox users may not join the same channel from more than one client,
    // so applications using voice should provide unique Vivox usernames for each player.
    // These usernames should have a 1:1 mapping to the player and should stay the same
    // every time that player plays the game. If your game server issues and validates
    // its own unique accounts, these should be used. For additional security and to
    // ensure Vivox username restrictions are met, you can provide an md5 hash of your
    // name instead.
    // Since ShooterGame does not use its own game server or have accounts, we use the
    // Nickname of player 0, which returns the player's name/tag from the online
    // subsystem. On most platforms in most cases this is guaranteed to be unique. For
    // Switch players on LAN it should be unique as long as each Switch is signed in with
    // account profiles of different names. This Nickname is checked against Vivox
    // username length and character restrictions and used if possible, or hashed if
    // necessary. As long as the Nicknames are unique they will hash uniquely, but
    // since not every name is hashed (for the convenience of readability in the logs)
    // there is an infinitesimally small chance that a regular Nickname could match
    // another's hash. We do not recommend choosing usernames this way in production.
    //
    // Note that using GetFirstGamePlayer() will only work correctly when there's one
    // local player. Console games using networked splitscreen with multiple local
    // players will need to login each player with their own uuids. A splitscreen voice
    // chat implementation example is not included in this version of the sample.
#if !PLATFORM_SWITCH && ((((ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1) || ENGINE_MAJOR_VERSION == 4) && !PLATFORM_PS4) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0 && !defined(PLATFORM_PS4)))
    Login(GetVivoxSafePlayerName(GetFirstGamePlayer()->GetPreferredUniqueNetId()->ToString()));
#endif
}

void UVivoxGameInstance::Shutdown()
{
    Tracer::MinorMethodPrologue();

    Super::Shutdown();

    Uninitialize();
}

// logLevel: -1 = no logging, 0 = errors only, 1 = warnings, 2 = info, 3 = debug, 4 = trace, 5 = log all
VivoxCoreError UVivoxGameInstance::Initialize(int logLevel)
{
    Tracer::MinorMethodPrologue();

    if (GIsEditor)
    {
        UE_LOG(LogVivoxGameInstance, Warning, TEXT("This sample does not support Play In-Editor: the Vivox Plugin does, but hosting or joining a match in the underlying ShooterGame sample (base app, not our sample integration) doesn't function properly in the editor, so the plugin cannot adequately be showcased."));
        return VxErrorInvalidOperation; // This is not necessary for your game and is just a choice for our sample.
    }

    if (bInitialized)
    {
        return VxErrorSuccess;
    }

    VivoxConfig Config;
    Config.SetLogLevel((vx_log_level)logLevel);
    VivoxCoreError Status = VivoxVoiceClient->Initialize(Config);
    if (Status != VxErrorSuccess)
    {
        UE_LOG(LogVivoxGameInstance, Error, TEXT("Initialize failed: %s (%d)"), ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
    }
    else
    {
        bInitialized = true;
    }

    return Status;
}

void UVivoxGameInstance::Uninitialize()
{
    Tracer::MinorMethodPrologue();

    if (!bInitialized)
    {
        return;
    }

    VivoxVoiceClient->Uninitialize();
}

bool UVivoxGameInstance::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
    if (FParse::Command(&Cmd, TEXT("VIVOXSTATE")))
    {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("Initialized: %s"), IsInitialized() ? TEXT("YES") : TEXT("NO"));
        UE_LOG(LogVivoxGameInstance, Log, TEXT("Logged in: %s"), IsLoggedIn() ? TEXT("YES") : TEXT("NO"));
        UE_LOG(LogVivoxGameInstance, Log, TEXT("Logging in: %s"), bLoggingIn ? TEXT("YES") : TEXT("NO"));

        if (VivoxVoiceClient->GetLoginSession(LoggedInAccountID).ChannelSessions().Num() > 0)
        {
            UE_LOG(LogVivoxGameInstance, Log, TEXT("Current Channel List:"));
            UE_LOG(LogVivoxGameInstance, Log, TEXT("===================="));
            for (auto &Session : VivoxVoiceClient->GetLoginSession(LoggedInAccountID).ChannelSessions())
            {
                UE_LOG(LogVivoxGameInstance, Log, TEXT("Name: %s, Type: %s, IsTransmitting: %s, AudioState: %s, TextState: %s"),
                    *Session.Value->Channel().Name(),
                    *UEnumShortToString(ChannelType, Session.Value->Channel().Type()),
                    Session.Value->IsTransmitting() ? TEXT("Yes") : TEXT("No"),
                    *UEnumShortToString(ConnectionState, Session.Value->AudioState()),
                    *UEnumShortToString(ConnectionState, Session.Value->TextState()));
            }
            UE_LOG(LogVivoxGameInstance, Log, TEXT("===================="));
        }
        else
        {
            UE_LOG(LogVivoxGameInstance, Log, TEXT("Current Channel List: Empty"));
        }

        return true;
    }

    return false;
}

/**
 * \brief Bind or unbind event handlers for ILoginSession property changes.
 * \param DoBind Whether to bind or unbind the handlers.
 * \param ILoginSession Object for which to bind or unbind handlers.
 */
void UVivoxGameInstance::BindLoginSessionHandlers(bool DoBind, ILoginSession &LoginSession)
{
    if (DoBind)
    {
        LoginSession.EventStateChanged.AddUObject(this, &UVivoxGameInstance::OnLoginSessionStateChanged);
    }
    else
    {
        LoginSession.EventStateChanged.RemoveAll(this);
    }
}

/**
 * \brief Bind or unbind event handlers for IChannelSession property changes.
 * \param DoBind Whether to bind or unbind the handlers.
 * \param IChannelSession Object for which to bind or unbind handlers.
 */
void UVivoxGameInstance::BindChannelSessionHandlers(bool DoBind, IChannelSession &ChannelSession)
{
    if (DoBind)
    {
        ChannelSession.EventAfterParticipantAdded.AddUObject(this, &UVivoxGameInstance::OnChannelParticipantAdded);
        ChannelSession.EventBeforeParticipantRemoved.AddUObject(this, &UVivoxGameInstance::OnChannelParticipantRemoved);
        ChannelSession.EventAfterParticipantUpdated.AddUObject(this, &UVivoxGameInstance::OnChannelParticipantUpdated);
        ChannelSession.EventAudioStateChanged.AddUObject(this, &UVivoxGameInstance::OnChannelAudioStateChanged);
        ChannelSession.EventTextStateChanged.AddUObject(this, &UVivoxGameInstance::OnChannelTextStateChanged);
        ChannelSession.EventChannelStateChanged.AddUObject(this, &UVivoxGameInstance::OnChannelStateChanged);
        ChannelSession.EventTextMessageReceived.AddUObject(this, &UVivoxGameInstance::OnChannelTextMessageReceived);
    }
    else
    {
        ChannelSession.EventAfterParticipantAdded.RemoveAll(this);
        ChannelSession.EventBeforeParticipantRemoved.RemoveAll(this);
        ChannelSession.EventAfterParticipantUpdated.RemoveAll(this);
        ChannelSession.EventAudioStateChanged.RemoveAll(this);
        ChannelSession.EventTextStateChanged.RemoveAll(this);
        ChannelSession.EventChannelStateChanged.RemoveAll(this);
        ChannelSession.EventTextMessageReceived.RemoveAll(this);
    }
}

// EDIT BEGIN
// VivoxCoreError UVivoxGameInstance::Login(const FString& PlayerName)
void UVivoxGameInstance::Login(const FString& PlayerName)
// EDIT END
{
    Tracer::MajorMethodPrologue("%s", *PlayerName);

    if (bLoggedIn)
    {
        UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Already logged in"));
        // EDIT BEGIN
        // return VxErrorSuccess;
        return;
        // EDIT END
    }

    if (bLoggingIn)
    {
        UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Already logging in"));
        // EDIT BEGIN
        // return VxErrorSuccess;
        return;
        // EDIT END
    }

    if (!bInitialized)
    {
        UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Not initialized"));
        // EDIT BEGIN
        // return VxErrorNotInitialized;
        return;
        // EDIT END
    }

    // EDIT BEGIN
    // LoggedInPlayerName = PlayerName;
    // LoggedInAccountID = AccountId(VIVOX_VOICE_ISSUER, LoggedInPlayerName, VIVOX_VOICE_DOMAIN);
    // ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
    //
    // // IMPORTANT: in production, developers should NOT use the insecure client-side token generation methods.
    // // To generate secure access tokens, call GenerateClientLoginToken or a custom implementation from your game server.
    // // This is important not only to secure access to chat features, but tokens issued by the client could
    // // appear expired if the client's clock is set incorrectly, resulting in rejection.
    // FString LoginToken;
    // FVivoxToken::GenerateClientLoginToken(LoginSession, LoginToken);
    //
    // UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Logging in %s with token %s"), *LoggedInPlayerName, *LoginToken);
    //
    // ILoginSession::FOnBeginLoginCompletedDelegate OnBeginLoginCompleteCallback;
    // OnBeginLoginCompleteCallback.BindLambda([this, &LoginSession](VivoxCoreError Status)
    // {
    //     bLoggingIn = false;
    //     if (VxErrorSuccess != Status)
    //     {
    //         UE_LOG(LogVivoxGameInstance, Error, TEXT("Login failure for %s: %s (%d)"), *LoggedInPlayerName, ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
    //         BindLoginSessionHandlers(false, LoginSession); // Unbind handlers if we fail to log in
    //         LoggedInAccountID = AccountId();
    //         LoggedInPlayerName = FString();
    //         bLoggedIn = false; // should already be false, but we'll just make sure
    //     }
    //     else
    //     {
    //         UE_LOG(LogVivoxGameInstance, Log, TEXT("Login success for %s"), *LoggedInPlayerName);
    //         bLoggedIn = true;
    //     }
    // });
    // BindLoginSessionHandlers(true, LoginSession);
    // bLoggingIn = true;
    //
    // return LoginSession.BeginLogin(VIVOX_VOICE_SERVER, LoginToken, OnBeginLoginCompleteCallback);
    LoginOntoAccelByte(PlayerName);
    // EDIT END
}

// EDIT BEGIN
void UVivoxGameInstance::LoginOntoAccelByte(const FString& PlayerName)
{
    AccelByte::FMultiRegistry::GetApiClient()->User.LoginWithDeviceId(
        AccelByte::FVoidHandler::CreateLambda([this, PlayerName]()
            {
                OnAccelByteLoginFinished(PlayerName);
            }),
        AccelByte::FErrorHandler::CreateLambda([this, PlayerName](const int32 ErrorCode, const FString& ErrorMessage)
            {
                OnAccelByteLoginFinished(PlayerName); // proceed even if logging into AccelByte failed (optional)
            })
    );
}

void UVivoxGameInstance::OnAccelByteLoginFinished(const FString& PlayerName)
{
    LoggedInPlayerName = PlayerName;
    LoggedInAccountID = AccountId(VIVOX_VOICE_ISSUER, LoggedInPlayerName, VIVOX_VOICE_DOMAIN);

    FOnTokenReceived OnTokenReceived;
    OnTokenReceived.BindUObject(this, &UVivoxGameInstance::OnLoginTokenReceived);

    FTokenRequestV1 TokenRequest;
    TokenRequest.Type = "login";
    TokenRequest.Username = LoggedInPlayerName;

    bLoggingIn = true;
    VivoxTokenProvider::GetToken(TokenRequest, OnTokenReceived);
}

void UVivoxGameInstance::OnLoginTokenReceived(FString Token)
{
    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);

    UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Logging in %s with token %s"), *LoggedInPlayerName, *Token);

    ILoginSession::FOnBeginLoginCompletedDelegate OnBeginLoginCompleteCallback;
    OnBeginLoginCompleteCallback.BindLambda([this, &LoginSession](VivoxCoreError Status)
    {
        bLoggingIn = false;
        if (VxErrorSuccess != Status)
        {
            UE_LOG(LogVivoxGameInstance, Error, TEXT("Login failure for %s: %s (%d)"), *LoggedInPlayerName, ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
            BindLoginSessionHandlers(false, LoginSession); // Unbind handlers if we fail to log in
            LoggedInAccountID = AccountId();
            LoggedInPlayerName = FString();
            bLoggedIn = false; // should already be false, but we'll just make sure
        }
        else
        {
            UE_LOG(LogVivoxGameInstance, Log, TEXT("Login success for %s"), *LoggedInPlayerName);
            bLoggedIn = true;
        }
    });

    BindLoginSessionHandlers(true, LoginSession);

    LoginSession.BeginLogin(VIVOX_VOICE_SERVER, Token, OnBeginLoginCompleteCallback);
}
// EDIT END

void UVivoxGameInstance::OnLoginSessionStateChanged(LoginState State)
{
    switch (State)
    {
    case LoginState::LoggedOut:
        UE_LOG(LogVivoxGameInstance, Error, TEXT("User Logged Out. Logout could be done intentionally or due to Network"));
        LoggedInAccountID = AccountId();
        LoggedInPlayerName = FString();
        bLoggingIn = false;
        bLoggedIn = false;
        break;
    default:
        UE_LOG(LogVivoxGameInstance, Log, TEXT("LoginSession State Change: %s"), *UEnumShortToString(LoginState, State));
        break;
    }
}

void UVivoxGameInstance::Logout()
{
    Tracer::MinorMethodPrologue();

    if (!bLoggedIn && !bLoggingIn)
    {
        UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Not logged in, skipping logout"));
        return;
    }

    ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);

    LoginSession.Logout();

    LoggedInAccountID = AccountId();
    LoggedInPlayerName = FString();
    bLoggingIn = false;
    bLoggedIn = false;
}

VivoxCoreError UVivoxGameInstance::JoinVoiceChannels(FString GameMode, FString OnlineSessionId, int32 TeamNum)
{
    Tracer::MajorMethodPrologue("%s %s %d", *GameMode, *OnlineSessionId, TeamNum);

    FString channelName = *OnlineSessionId;

    if (GameMode.Equals(TEXT("FFA"))) // Free For All
    {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("FreeForAll GameType detected"));
        Join(ChannelType::Positional, true, FString::Printf(TEXT("FP%s"), *channelName));

        return VxErrorSuccess;
    }
    else if (GameMode.Equals(TEXT("TDM"))) // Team Deathmatch
    {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("TeamDeathmatch GameType detected"));
        Join(ChannelType::Positional, false, FString::Printf(TEXT("TP%s"), *channelName), PTTKey::PTTAreaChannel);

        bool bTeamChatOnAtStart = false;
#if PLATFORM_SWITCH || ((((ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1) || ENGINE_MAJOR_VERSION == 4) && PLATFORM_PS4) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0 && defined(PLATFORM_PS4)))
        bTeamChatOnAtStart = true; // on console, team chat should be toggled on at start
#endif
        Join(ChannelType::NonPositional, bTeamChatOnAtStart, FString::Printf(TEXT("TN%d_%s"), TeamNum, *channelName), PTTKey::PTTTeamChannel);

        return VxErrorSuccess;
    }
    else
    {
        UE_LOG(LogVivoxGameInstance, Warning, TEXT("Joining no voice channels: Invalid GameType detected(%s)"), *GameMode);
        return VxErrorInvalidState;
    }
}

// EDIT BEGIN
// VivoxCoreError UVivoxGameInstance::Join(ChannelType Type, bool ShouldTransmitOnJoin, const FString& ChannelName, PTTKey AssignChanneltoPTTKey)
void UVivoxGameInstance::Join(ChannelType Type, bool ShouldTransmitOnJoin, const FString& ChannelName, PTTKey AssignChanneltoPTTKey)
// EDIT END
{
    Tracer::MajorMethodPrologue("%s %d %s %s", *UEnumFullToString(ChannelType, Type), ShouldTransmitOnJoin, *ChannelName, *UEnumFullToString(PTTKey, AssignChanneltoPTTKey));

    if (!bLoggedIn)
    {
        UE_LOG(LogVivoxGameInstance, Warning, TEXT("Not logged in; cannot join a channel"));
        // EDIT BEGIN
        // return VxErrorNotLoggedIn;
        return;
        // EDIT END
    }
    ensure(!LoggedInPlayerName.IsEmpty());
    ensure(!ChannelName.IsEmpty());

    // EDIT BEGIN
    // ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
    // // It's perfectly safe to add 3D properties to any channel type (they don't have any effect if the channel type is not Positional)
    // ChannelId Channel = ChannelId(VIVOX_VOICE_ISSUER, ChannelName, VIVOX_VOICE_DOMAIN, Type, Channel3DProperties(8100, 270, 1.0, EAudioFadeModel::InverseByDistance));
    // IChannelSession &ChannelSession = LoginSession.GetChannelSession(Channel);
    //
    // // IMPORTANT: in production, developers should NOT use the insecure client-side token generation methods.
    // // To generate secure access tokens, call GenerateClientJoinToken or a custom implementation from your game server.
    // // This is important not only to secure access to Chat features, but tokens issued by the client could
    // // appear expired if the client's clock is set incorrectly, resulting in rejection.
    // FString JoinToken;
    // FVivoxToken::GenerateClientJoinToken(ChannelSession, JoinToken);
    //
    // UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Joining %s to %s with token %s"), *LoggedInPlayerName, *ChannelName, *JoinToken);
    //
    // IChannelSession::FOnBeginConnectCompletedDelegate OnBeginConnectCompleteCallback;
    // OnBeginConnectCompleteCallback.BindLambda([this, ShouldTransmitOnJoin, AssignChanneltoPTTKey, &LoginSession, &ChannelSession](VivoxCoreError Status)
    // {
    //     if (VxErrorSuccess != Status)
    //     {
    //         UE_LOG(LogVivoxGameInstance, Error, TEXT("Join failure for %s: %s (%d)"), *ChannelSession.Channel().Name(), ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
    //         BindChannelSessionHandlers(false, ChannelSession); // Unbind handlers if we fail to join.
    //         LoginSession.DeleteChannelSession(ChannelSession.Channel()); // Disassociate this ChannelSession from the LoginSession.
    //     }
    //     else
    //     {
    //         UE_LOG(LogVivoxGameInstance, Log, TEXT("Join success for %s"), *ChannelSession.Channel().Name());
    //         if (ChannelType::Positional == ChannelSession.Channel().Type())
    //         {
    //             ConnectedPositionalChannel = ChannelSession.Channel();
    //         }
    //
    //         if (PTTKey::PTTAreaChannel == AssignChanneltoPTTKey)
    //         {
    //             PTTAreaChannel = TPairInitializer<ChannelId, bool>(ChannelSession.Channel(), false);
    //         }
    //         else if (PTTKey::PTTTeamChannel == AssignChanneltoPTTKey)
    //         {
    //             PTTTeamChannel = TPairInitializer<ChannelId, bool>(ChannelSession.Channel(), false);
    //         }
    //
    //         // NB: It is usually not necessary to adjust transmission when joining channels.
    //         // The conditional below controls desired behavior specific to this application.
    //         if (ShouldTransmitOnJoin)
    //         {
    //             if (AssignChanneltoPTTKey != PTTKey::PTTNoChannel)
    //                 MultiChanToggleChat(AssignChanneltoPTTKey);
    //             else
    //                 LoginSession.SetTransmissionMode(TransmissionMode::All);
    //         }
    //         else if (LoginSession.ChannelSessions().Num() == 1)
    //         {
    //             LoginSession.SetTransmissionMode(TransmissionMode::None);
    //         }
    //     }
    // });
    // BindChannelSessionHandlers(true, ChannelSession);
    //
    // return ChannelSession.BeginConnect(true, false, ShouldTransmitOnJoin, JoinToken, OnBeginConnectCompleteCallback);
    Channel3DProperties ChannelProperties = Channel3DProperties(8100, 270, 1.0, EAudioFadeModel::InverseByDistance);

    FOnTokenReceived OnTokenReceived;
    OnTokenReceived.BindLambda([this, Type, ShouldTransmitOnJoin, ChannelName, AssignChanneltoPTTKey, ChannelProperties](FString Token)
    {
        OnJoinTokenReceived(Token, Type, ShouldTransmitOnJoin, ChannelName, AssignChanneltoPTTKey, ChannelProperties);
    });

    FTokenRequestV1 TokenRequest;
    TokenRequest.Type = "join";
    TokenRequest.Username = LoggedInPlayerName;
    TokenRequest.ChannelId = ChannelName;

    switch (Type)
    {
        case ChannelType::NonPositional:
            TokenRequest.ChannelType = TEXT("nonpositional");
            break;
	    case ChannelType::Positional:
            TokenRequest.ChannelId += "!" + ChannelProperties.ToString();
	        TokenRequest.ChannelType = TEXT("positional");
	        break;
        case ChannelType::Echo:
            TokenRequest.ChannelType = TEXT("echo");
            break;
    }

    VivoxTokenProvider::GetToken(TokenRequest, OnTokenReceived);
    // EDIT END
}

// EDIT BEGIN
void UVivoxGameInstance::OnJoinTokenReceived(FString Token, ChannelType Type, bool ShouldTransmitOnJoin, const FString& ChannelName, PTTKey AssignChanneltoPTTKey, Channel3DProperties ChannelProperties)
{
    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
    // It's perfectly safe to add 3D properties to any channel type (they don't have any effect if the channel type is not Positional)
    ChannelId Channel = ChannelId(VIVOX_VOICE_ISSUER, ChannelName, VIVOX_VOICE_DOMAIN, Type, ChannelProperties);
    IChannelSession& ChannelSession = LoginSession.GetChannelSession(Channel);

    UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Joining %s to %s with token %s"), *LoggedInPlayerName, *ChannelName, *Token);

    IChannelSession::FOnBeginConnectCompletedDelegate OnBeginConnectCompleteCallback;
    OnBeginConnectCompleteCallback.BindLambda([this, ShouldTransmitOnJoin, AssignChanneltoPTTKey, &LoginSession, &ChannelSession](VivoxCoreError Status)
    {
        if (VxErrorSuccess != Status)
        {
            UE_LOG(LogVivoxGameInstance, Error, TEXT("Join failure for %s: %s (%d)"), *ChannelSession.Channel().Name(), ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
            BindChannelSessionHandlers(false, ChannelSession); // Unbind handlers if we fail to join.
            LoginSession.DeleteChannelSession(ChannelSession.Channel()); // Disassociate this ChannelSession from the LoginSession.
        }
        else
        {
            UE_LOG(LogVivoxGameInstance, Log, TEXT("Join success for %s"), *ChannelSession.Channel().Name());
            if (ChannelType::Positional == ChannelSession.Channel().Type())
            {
                ConnectedPositionalChannel = ChannelSession.Channel();
            }

            if (PTTKey::PTTAreaChannel == AssignChanneltoPTTKey)
            {
                PTTAreaChannel = TPairInitializer<ChannelId, bool>(ChannelSession.Channel(), false);
            }
            else if (PTTKey::PTTTeamChannel == AssignChanneltoPTTKey)
            {
                PTTTeamChannel = TPairInitializer<ChannelId, bool>(ChannelSession.Channel(), false);
            }

            // NB: It is usually not necessary to adjust transmission when joining channels.
            // The conditional below controls desired behavior specific to this application.
            if (ShouldTransmitOnJoin)
            {
                if (AssignChanneltoPTTKey != PTTKey::PTTNoChannel)
                    MultiChanToggleChat(AssignChanneltoPTTKey);
                else
                    LoginSession.SetTransmissionMode(TransmissionMode::All);
            }
            else if (LoginSession.ChannelSessions().Num() == 1)
            {
                LoginSession.SetTransmissionMode(TransmissionMode::None);
            }
        }
    });

    BindChannelSessionHandlers(true, ChannelSession);

    ChannelSession.BeginConnect(true, false, ShouldTransmitOnJoin, Token, OnBeginConnectCompleteCallback);
}
// EDIT END

void UVivoxGameInstance::LeaveVoiceChannels()
{
    Tracer::MinorMethodPrologue();

    if (!bLoggedIn)
    {
        UE_LOG(LogVivoxGameInstance, Verbose, TEXT("Not logged in; cannot leave channel(s)"));
        return;
    }

    TArray<ChannelId> ChannelSessionKeys;
    VivoxVoiceClient->GetLoginSession(LoggedInAccountID).ChannelSessions().GenerateKeyArray(ChannelSessionKeys);
    for (ChannelId SessionKey : ChannelSessionKeys)
    {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("Disconnecting from channel %s"), *SessionKey.Name());
        BindChannelSessionHandlers(false, VivoxVoiceClient->GetLoginSession(LoggedInAccountID).GetChannelSession(SessionKey));
        VivoxVoiceClient->GetLoginSession(LoggedInAccountID).DeleteChannelSession(SessionKey);
    }

    // Always clear stashed Positional channel and PTT channels
    ConnectedPositionalChannel = ChannelId();
    PTTAreaChannel.Key = ChannelId();
    PTTTeamChannel.Key = ChannelId();
}

void UVivoxGameInstance::OnChannelParticipantAdded(const IParticipant &Participant)
{
    ChannelId Channel = Participant.ParentChannelSession().Channel();
    UE_LOG(LogVivoxGameInstance, Log, TEXT("User %s has joined channel %s (self = %s)"), *Participant.Account().Name(), *Channel.Name(), Participant.IsSelf() ? TEXT("true") : TEXT("false"));
}

void UVivoxGameInstance::OnChannelParticipantRemoved(const IParticipant &Participant)
{
    ChannelId Channel = Participant.ParentChannelSession().Channel();
    UE_LOG(LogVivoxGameInstance, Log, TEXT("User %s has left channel %s (self = %s)"), *Participant.Account().Name(), *Channel.Name(), Participant.IsSelf() ? TEXT("true") : TEXT("false"));
}

void UVivoxGameInstance::OnChannelParticipantUpdated(const IParticipant &Participant)
{
    if (Participant.IsSelf()) {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("Self Participant Updated (audio=%d, text=%d, speaking=%d)"), Participant.InAudio(), Participant.InText(), Participant.SpeechDetected());
    }
}

void UVivoxGameInstance::OnChannelAudioStateChanged(const IChannelConnectionState &State)
{
    UE_LOG(LogVivoxGameInstance, Log, TEXT("ChannelSession Audio State Change in %s: %s"), *State.ChannelSession().Channel().Name(), *UEnumShortToString(ConnectionState, State.State()));
}

void UVivoxGameInstance::OnChannelTextStateChanged(const IChannelConnectionState &State)
{
    UE_LOG(LogVivoxGameInstance, Log, TEXT("ChannelSession Text State Change in %s: %s"), *State.ChannelSession().Channel().Name(), *UEnumShortToString(ConnectionState, State.State()));
}

void UVivoxGameInstance::OnChannelStateChanged(const IChannelConnectionState &State)
{
    UE_LOG(LogVivoxGameInstance, Log, TEXT("ChannelSession Connection State Change in %s: %s"), *State.ChannelSession().Channel().Name(), *UEnumShortToString(ConnectionState, State.State()));
}

void UVivoxGameInstance::OnChannelTextMessageReceived(const IChannelTextMessage &Message)
{
    UE_LOG(LogVivoxGameInstance, Log, TEXT("Message Received from %s: %s"), *Message.Sender().Name(), *Message.Message());
}

bool UVivoxGameInstance::Get3DValuesAreDirty() const
{
    return (CachedPosition.IsDirty() ||
            CachedForwardVector.IsDirty() ||
            CachedUpVector.IsDirty());
}

void UVivoxGameInstance::Clear3DValuesAreDirty()
{
    CachedPosition.SetDirty(false);
    CachedForwardVector.SetDirty(false);
    CachedUpVector.SetDirty(false);
}

void UVivoxGameInstance::Update3DPosition(APawn* Pawn)
{
    /// Return if argument is invalid.
    if (NULL == Pawn)
        return;

    /// Return if we're not in a positional channel.
    if (ConnectedPositionalChannel.IsEmpty())
        return;

    /// Update cached 3D position and orientation.
    CachedPosition.SetValue(Pawn->GetActorLocation());
    CachedForwardVector.SetValue(Pawn->GetActorForwardVector());
    CachedUpVector.SetValue(Pawn->GetActorUpVector());

    /// Return If there's no change from cached values.
    if (!Get3DValuesAreDirty())
        return;

    /// Set new position and orientation in connected positional channel.
    Tracer::MajorMethodPrologue("%s %s %s %s %s", *ConnectedPositionalChannel.Name(), *CachedPosition.GetValue().ToCompactString(), *CachedPosition.GetValue().ToCompactString(), *CachedForwardVector.GetValue().ToCompactString(), *CachedUpVector.GetValue().ToCompactString());
    ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
    LoginSession.GetChannelSession(ConnectedPositionalChannel).Set3DPosition(CachedPosition.GetValue(), CachedPosition.GetValue(), CachedForwardVector.GetValue(), CachedUpVector.GetValue());

    Clear3DValuesAreDirty();
}

VivoxCoreError UVivoxGameInstance::MultiChanPushToTalk(PTTKey Key, bool PTTKeyPressed)
{
    FString Channel;
    if (PTTKey::PTTAreaChannel == Key && !PTTAreaChannel.Key.IsEmpty())
    {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("MultiChanPushToTalk: %s talking in %s"), PTTKeyPressed ? TEXT("Started") : TEXT("Stopped"), *PTTAreaChannel.Key.Name());
        PTTAreaChannel.Value = PTTKeyPressed;
    }
    else if (PTTKey::PTTTeamChannel == Key && !PTTTeamChannel.Key.IsEmpty())
    {
        UE_LOG(LogVivoxGameInstance, Log, TEXT("MultiChanPushToTalk: %s talking in %s"), PTTKeyPressed ? TEXT("Started") : TEXT("Stopped"), *PTTTeamChannel.Key.Name());
        PTTTeamChannel.Value = PTTKeyPressed;
    }
    else
    {
        UE_LOG(LogVivoxGameInstance, Warning, TEXT("MultiChanPushToTalk: No ChannelId assigned to %s"), *UEnumFullToString(PTTKey, Key));
        return VxErrorInvalidState;
    }

    if (PTTAreaChannel.Value && PTTTeamChannel.Value) // Both
    {
        LastKnownTransmittingChannel = LastKnownTransmittingChannel == PTTAreaChannel.Key ? PTTTeamChannel.Key : PTTAreaChannel.Key; // flip
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::All);
    }
    else if (PTTAreaChannel.Value) // Area Only
    {
        LastKnownTransmittingChannel = PTTAreaChannel.Key;
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTAreaChannel.Key);
    }
    else if (PTTTeamChannel.Value) // Team Only
    {
        LastKnownTransmittingChannel = PTTTeamChannel.Key;
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTTeamChannel.Key);
    }
    else // None
    {
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::None);
    }
}

/*
 * Console only. When a push-to-toggle key is pressed, transmission to that channel is toggled,
 * and we ensure only one channel transmits at a time. This is not a limitation on console,
 * simply a design choice for this particular application. Desktop uses MultiChanPushToTalk.
 */
VivoxCoreError UVivoxGameInstance::MultiChanToggleChat(PTTKey Key)
{
    Tracer::MajorMethodPrologue("%s", *UEnumFullToString(PTTKey, Key));

    if (PTTKey::PTTAreaChannel == Key && !PTTAreaChannel.Key.IsEmpty())
    {
        PTTAreaChannel.Value = !PTTAreaChannel.Value;
        UE_LOG(LogVivoxGameInstance, Log, TEXT("MultiChanToggleChat: Toggling transmission %s for %s"), PTTAreaChannel.Value ? TEXT("on") : TEXT("off"), *PTTAreaChannel.Key.Name());
        if (PTTAreaChannel.Value && PTTTeamChannel.Value)
        {
            UE_LOG(LogVivoxGameInstance, Log, TEXT("MultiChanToggleChat: Toggling transmission off for %s"), *PTTTeamChannel.Key.Name());
            PTTTeamChannel.Value = false;
        }
    }
    else if (PTTKey::PTTTeamChannel == Key && !PTTTeamChannel.Key.IsEmpty())
    {
        PTTTeamChannel.Value = !PTTTeamChannel.Value;
        UE_LOG(LogVivoxGameInstance, Log, TEXT("MultiChanToggleChat: Toggling transmission %s for %s"), PTTTeamChannel.Value ? TEXT("on") : TEXT("off"), *PTTTeamChannel.Key.Name());
        if (PTTTeamChannel.Value && PTTAreaChannel.Value)
        {
            UE_LOG(LogVivoxGameInstance, Log, TEXT("MultiChanToggleChat: Toggling transmission off for %s"), *PTTAreaChannel.Key.Name());
            PTTAreaChannel.Value = false;
        }
    }
    else
    {
        UE_LOG(LogVivoxGameInstance, Warning, TEXT("MultiChanToggleChat: No ChannelId assigned to %s"), *UEnumFullToString(PTTKey, Key));
        return VxErrorInvalidState;
    }

    if (PTTAreaChannel.Value && PTTTeamChannel.Value) // Both
    {
        ensureMsgf(false, TEXT("MultiChanToggleChat: Transmitting to all channels on console. This is safe, but was designed not to happen in this sample."));
        LastKnownTransmittingChannel = LastKnownTransmittingChannel == PTTAreaChannel.Key ? PTTTeamChannel.Key : PTTAreaChannel.Key; // flip
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::All);
    }
    else if (PTTAreaChannel.Value) // Area Only
    {
        LastKnownTransmittingChannel = PTTAreaChannel.Key;
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTAreaChannel.Key);
    }
    else if (PTTTeamChannel.Value) // Team Only
    {
        LastKnownTransmittingChannel = PTTTeamChannel.Key;
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTTeamChannel.Key);
    }
    else // None
    {
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::None);
    }
}

bool UVivoxGameInstance::IsInitialized()
{
    return bInitialized;
}

bool UVivoxGameInstance::IsLoggedIn()
{
    return bLoggedIn;
}

ILoginSession *UVivoxGameInstance::GetLoginSessionForRoster()
{
    if (!LoggedInAccountID.IsEmpty())
    {
        return &VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
    }
   return NULL;
}

TSharedPtr<IChannelSession> UVivoxGameInstance::GetChannelSessionForRoster()
{
    for (auto& Session : VivoxVoiceClient->GetLoginSession(LoggedInAccountID).ChannelSessions())
    {
        if (Session.Value->Channel().Name().StartsWith("TN", ESearchCase::CaseSensitive))
        {
            return Session.Value;
        }
    }
    return NULL;
}

bool UVivoxGameInstance::ChangeSoundClassVolume(float Volume, const FSoftObjectPath& SoundClassPath)
{
    Tracer::MajorMethodPrologue("%f %s", Volume, *SoundClassPath.ToString());

    /// Validate FSoftObjectPath.
    if (!SoundClassPath.IsValid())
        return false;

    /// Load object if not already loaded.
    USoundClass* SoundClass = LoadObject<USoundClass>(nullptr, *SoundClassPath.ToString());

    /// Validate object instantiation.
    if (SoundClass == nullptr)
        return false;

    /// Change Volume.
    SoundClass->Properties.Volume = Volume;
    return true;
}

// returns the md5 hash of BaseName if it does not meet required length and character restrictions
FString UVivoxGameInstance::GetVivoxSafePlayerName(FString BaseName)
{
    bool bDoHash = false;

    // check length is <= 60 minus length of VivoxIssuer; default assumes max issuer length
    int32 VivoxSafePlayerLength = 35;
    FString VivoxIssuer = FString(VIVOX_VOICE_ISSUER);
    if (!VivoxIssuer.IsEmpty())
        VivoxSafePlayerLength = 60 - VivoxIssuer.Len();

    // a known issue limits this by one further character so this is >= instead of > for now.
    if (BaseName.Len() >= VivoxSafePlayerLength)
    {
        bDoHash = true;
    }
    else // also check character restrictions
    {
        FString ValidCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890=+-_.!~()";
        int32 Loc;
        auto ConstItr = BaseName.CreateConstIterator();
        while (BaseName.IsValidIndex(ConstItr.GetIndex()))
        {
            if (!ValidCharacters.FindChar(BaseName[ConstItr++.GetIndex()], Loc))
            {
                bDoHash = true;
                break;
            }
        }
    }

    if (bDoHash)
        return FMD5::HashAnsiString(*BaseName);
    else
        return BaseName;
}

void UVivoxGameInstance::GotoState(FName NewState)
{
    Tracer::MajorMethodPrologue("%s", *NewState.ToString());

    Super::GotoState(NewState);

#if PLATFORM_SWITCH || ((((ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1) || ENGINE_MAJOR_VERSION == 4) && PLATFORM_PS4) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0 && defined(PLATFORM_PS4)))
    if (NewState == ShooterGameInstanceState::MainMenu)
    {
        // See UVivoxGameInstance::Connect for description of strategy for player name choice
        Login(GetVivoxSafePlayerName(GetFirstGamePlayer()->GetPreferredUniqueNetId()->ToString()));
    }
    else if (NewState == ShooterGameInstanceState::WelcomeScreen)
    {
        if (IsLoggedIn())
        {
            LeaveVoiceChannels();
            Logout();
        }
    }
#endif
}
