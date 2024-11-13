 /*
 * #####################################################################################
 *  Unity Vivox Plugin for Unreal Engine Copyright © 2024 Unity Technologies
 * #####################################################################################
 */


#include "Moderation/Public/ModerationSubsystem.h"
#include "ModerationApi.h"
#include "ModerationModule.h"
#include "WrappedModerationApi.h"

// Necessary to avoid triggering C4150 error for TUniquePtr<T> because the classes/structs are forward declared.
// See documentation in TDefaultDelete<T>::operator() for an explanation.
UModerationSubsystem::UModerationSubsystem() = default;
UModerationSubsystem::~UModerationSubsystem() = default;
UModerationSubsystem::UModerationSubsystem(FVTableHelper& Helper) {}

void UModerationSubsystem::Initialize(FSubsystemCollectionBase& Collection) 
{
	UE_LOG(LogModeration, Verbose, TEXT("UModerationSubsystem::Initialize()"));
	
	Super::Initialize(Collection);

	/* Api wrapper instance pointer */
	Api = Moderation::MakePimpl<Moderation::FModerationApi>(MakeUnique<Moderation::FWrappedModerationApi>());
}

void UModerationSubsystem::Deinitialize() 
{
	UE_LOG(LogModeration, Verbose, TEXT("UModerationSubsystem::Deinitialize()"));

	Super::Deinitialize();
}

FReport UModerationSubsystem::GetReport(FString ReportedUnityPlayerId, EReportReason ReportReason)
{
	UE_LOG(LogTemp, Log, TEXT("UModerationSubsystem::GetReport()"));

	AccountId reportedAccountId = AccountId(LoginSession->LoginSessionId().Issuer(), ReportedUnityPlayerId, LoginSession->LoginSessionId().Domain(), TOptional<FString>(), TOptional<TArray<FString>>(), LoginSession->LoginSessionId().UnityEnvironmentId());

	return GetReport(reportedAccountId, ReportReason);
}

FReport UModerationSubsystem::GetReport(AccountId ReportedVivoxAccountId, EReportReason ReportReason)
{
	UE_LOG(LogModeration, Verbose, TEXT("UModerationSubsystem::GetReport()"));

	CleanVivoxCache();

	FReport reportReturn;

	reportReturn.ReportedUnityPlayerID = ReportedVivoxAccountId.Name();
	if (VivoxCaching)
	{
		for (auto cacheChannel : VivoxCache)
		{
			if (!cacheChannel.Value.Contains(ReportedVivoxAccountId))
			{
				continue;
			}
			reportReturn.VivoxChannels.Add(cacheChannel.Key);
			TArray<AccountId> channelAccounts;
			cacheChannel.Value.GetKeys(channelAccounts);
			reportReturn.VivoxChannels[cacheChannel.Key] = channelAccounts;
		}
	}
	reportReturn.ReportReason = reportReasonToString(ReportReason);
	reportReturn.ReportedVivoxURI = ReportedVivoxAccountId.ToString();
	reportReturn.ReportingVivoxURI = LoginSession->LoginSessionId().ToString();

	return reportReturn;
}

FString UModerationSubsystem::reportReasonToString(EReportReason reportReason)
{
	switch (reportReason)
	{
	case EReportReason::PlayerName:
		return "player name";
	case EReportReason::AimSnapping:
		return "aim snapping";
	case EReportReason::Boosting:
		return "boosting";
	case EReportReason::Exploiting:
		return "exploiting";
	case EReportReason::Hacking:
		return "hacking";
	case EReportReason::Smurfing:
		return "smurfing";
	case EReportReason::UnrealisticMovement:
		return "unrealistic movement";
	case EReportReason::CollusionWithOpponent:
		return "collusion with opponent";
	case EReportReason::LeftTheMatch:
		return "left the match";
	case EReportReason::Inactive:
		return "inactive";
	case EReportReason::Sabotage:
		return "sabotage";
	case EReportReason::Spamming:
		return "spamming";
	case EReportReason::HateSpeech:
		return "hate speech";
	case EReportReason::PredatoryBehaviour:
		return "predatory behaviour";
	case EReportReason::NoiseDisruption:
		return "noise disruption";
	case EReportReason::Scamming:
		return "scamming";
	case EReportReason::Ads:
		return "ads";
	case EReportReason::Threat:
		return "threat";
	case EReportReason::VerbalAbuse:
		return "verbal abuse";
	}
	return "";
}

void UModerationSubsystem::ReportPost(FReport Report, FString UASToken, Moderation::THandler<FReportResponse> ResponseHandler)
{
	UE_LOG(LogModeration, Verbose, TEXT("UModerationSubsystem::ReportPost()"));

	Api->ReportPost(Report, UASToken, ResponseHandler);
}

void UModerationSubsystem::HandleChannelJoined(const IChannelSession& channelSession)
{
	if (LoginSession->ChannelSessions()[channelSession.Channel()].Get() == nullptr)
	{
		return;
	}
	if (!VivoxCache.Contains(channelSession.Channel()))
	{
		VivoxCache.Add(channelSession.Channel());
	}
	IChannelSession* session = LoginSession->ChannelSessions()[channelSession.Channel()].Get();
	session->EventAfterParticipantAdded.AddLambda([this](const IParticipant &participant) {
		HandleParticipantEditted(participant);
		});
	session->EventBeforeParticipantRemoved.AddLambda([this](const IParticipant &participant) {
		HandleParticipantEditted(participant);
		});
	//ChannelJoined can happen after some participant events have been received for the IChannelSession - do a loop through the participants to ensure proper caching
	for (auto participant : channelSession.Participants())
	{
		HandleParticipantEditted(*participant.Value);
	}
}

void UModerationSubsystem::HandleChannelLeft(const IChannelSession &channelSession)
{
	if (LoginSession->ChannelSessions()[channelSession.Channel()].Get() == nullptr)
	{
		return;
	}
	IChannelSession* session = LoginSession->ChannelSessions()[channelSession.Channel()].Get();
	session->EventAfterParticipantAdded.RemoveAll(this);
	session->EventBeforeParticipantRemoved.RemoveAll(this);
	for (auto participant : channelSession.Participants())
	{
		HandleParticipantEditted(*participant.Value);
	}
}

void UModerationSubsystem::HandleParticipantEditted(const IParticipant &participant)
{
	//When a participant is added or removed, check if that participant is Self (and ignore it)
	if (participant.IsSelf())
	{
		return;
	}
	//Add the channel and account of the participant if the channel is not in the cache, or just add the participant if the channel is present but the participant is not
	if (!VivoxCache.Contains(participant.ParentChannelSession().Channel()))
	{
		VivoxCache.Add(participant.ParentChannelSession().Channel());
		VivoxCache[participant.ParentChannelSession().Channel()].Add(participant.Account());
	}
	else if (!VivoxCache[participant.ParentChannelSession().Channel()].Contains(participant.Account()))
	{
		VivoxCache[participant.ParentChannelSession().Channel()].Add(participant.Account());
	}
	//Set the time of this participants interation to current time
	VivoxCache[participant.ParentChannelSession().Channel()][participant.Account()] = FDateTime::Now();
}

void UModerationSubsystem::HandleLoginStateChanged(LoginState state)
{
	if (state == LoginState::LoggedIn)
	{
		LoginSession->EventChannelJoined.AddLambda([this](const IChannelSession& channelSession) {
			HandleChannelJoined(channelSession);
			});
		LoginSession->EventChannelLeft.AddLambda([this](const IChannelSession& channelSession) {
			HandleChannelLeft(channelSession);
			});
	}
	if (state == LoginState::LoggedOut)
	{
		LoginSession->EventChannelJoined.RemoveAll(this);
		LoginSession->EventChannelLeft.RemoveAll(this);
	}
}

void UModerationSubsystem::BeginVivoxCache(AccountId accountId)
{
	if (VivoxCaching)
	{
		UE_LOG(LogModeration, Verbose, TEXT("The Vivox Cache has already been started - use EndVivoxCache to stop it before restarting"));
		return;
	}
	//Attempt to grab, then check that grabbing the Vivox Module was successful
	vModule = static_cast<FVivoxCoreModule*>(&FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore")));

	if (vModule == nullptr)
	{
		UE_LOG(LogModeration, Verbose, TEXT("The Vivox Module is not loaded, Moderation will not compile and post Vivox Channel Participants"));
		return;
	}

	//Grab the VoiceClient, then check if a current LoginSession belongs to the AccountId provided
	VivoxVoiceClient = &vModule->VoiceClient();
	if (!VivoxVoiceClient->LoginSessions().Contains(accountId) && !accountId.IsEmpty())
	{
		UE_LOG(LogModeration, Verbose, TEXT("Vivox does not have a LoginSession that matches the accountId provided - make sure to call IClient::GetLoginSession() with the accountId before beginning the Vivox Cache"));
	}
	else if(accountId.IsEmpty())
	{
		//If AccountId was not provided, check if any LoginSessions exist, and connect to either the one most recently connected, or the one currently logged in (if any)
		TMap<AccountId, TSharedPtr<ILoginSession>> loginSessions = VivoxVoiceClient->LoginSessions();
		if (loginSessions.Num() == 0)
		{
			UE_LOG(LogModeration, Verbose, TEXT("Vivox does not have any LoginSessions, get a LoginSession using IClient::GetLoginSession() prior to calling ModerationSubsystem::BeginVivoxCache"));
		}
		for (auto _loginSession : loginSessions)
		{
			LoginSession = _loginSession.Value.Get();
			if (LoginSession->State() == LoginState::LoggedIn)
			{
				break;
			}
		}
	}
	else
	{
		LoginSession = VivoxVoiceClient->LoginSessions()[accountId].Get();
	}
	VivoxCaching = true;
	LoginSession->EventStateChanged.AddLambda([this](LoginState state) {
		HandleLoginStateChanged(state);
		});
	if (LoginSession->State() == LoginState::LoggedIn)
	{
		LoginSession->EventChannelJoined.AddLambda([this](const IChannelSession& session) {
			HandleChannelJoined(session);
			});
		LoginSession->EventChannelLeft.AddLambda([this](const IChannelSession& session) {
			HandleChannelLeft(session);
			});
		for (auto _channelSession : LoginSession->ChannelSessions())
		{
			_channelSession.Value.Get()->EventAfterParticipantAdded.AddLambda([this](const IParticipant& participant) {
				HandleParticipantEditted(participant);
				});
			_channelSession.Value.Get()->EventBeforeParticipantRemoved.AddLambda([this](const IParticipant& participant) {
				HandleParticipantEditted(participant);
				});
		}
	}
}

void UModerationSubsystem::EndVivoxCache()
{
	for (auto channelSession : LoginSession->ChannelSessions())
	{
		channelSession.Value.Get()->EventAfterParticipantAdded.RemoveAll(this);
		channelSession.Value.Get()->EventBeforeParticipantRemoved.RemoveAll(this);
	}
	LoginSession->EventChannelJoined.RemoveAll(this);
	LoginSession->EventChannelLeft.RemoveAll(this);
	for (auto cacheEntry : VivoxCache)
	{
		cacheEntry.Value.Empty();
	}
	VivoxCache.Empty();
	VivoxCaching = false;
}

void UModerationSubsystem::CleanVivoxCache()
{
	TArray<ChannelId> staleChannels;
	TArray<AccountId> staleAccounts;
	for (auto cacheEntry : VivoxCache)
	{
		if (LoginSession->ChannelSessions().Contains(cacheEntry.Key))
		{
			for (auto cachedPlayer : cacheEntry.Value)
			{
				if (LoginSession->ChannelSessions()[cacheEntry.Key]->Participants().Contains(cachedPlayer.Key.Name()))
				{
					cachedPlayer.Value = FDateTime::Now();
				}
				if (cachedPlayer.Value + CacheTimeout < FDateTime::Now())
				{
					staleAccounts.Add(cachedPlayer.Key);
				}
			}
		}
		else 
		{
			for (auto cachedPlayer : cacheEntry.Value)
			{
				if (cachedPlayer.Value + CacheTimeout < FDateTime::Now())
				{
					staleAccounts.Add(cachedPlayer.Key);
				}
			}
		}
		for (auto staleAccount : staleAccounts)
		{
			cacheEntry.Value.Remove(staleAccount);
		}
		if (cacheEntry.Value.Num() == 0)
		{
			staleChannels.Add(cacheEntry.Key);
		}
		staleAccounts.Empty();
	}
	for (auto staleChannel : staleChannels)
	{
		VivoxCache.Remove(staleChannel);
	}
	staleChannels.Empty();
}

bool UModerationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}