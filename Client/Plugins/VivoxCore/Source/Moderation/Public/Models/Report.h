 /*
 * #####################################################################################
 *  Unity Vivox Plugin for Unreal Engine Copyright © 2024 Unity Technologies
 * #####################################################################################
 */

#pragma once
#include "CoreMinimal.h"
#include "ReportReason.h"
#include "VivoxCore.h"
#include "Report.generated.h"

/**
 * @brief Struct representing options for authentication sign-in calls.
 */
USTRUCT(BlueprintType)
struct MODERATION_API FReport
{
	GENERATED_BODY()

public:
	/*!< The reason that the report is being made. */
	UPROPERTY(BlueprintReadWrite, Category = "Unity Gaming Services | Moderation | Models")
	FString ReportReason;

	/*!< The Unity Authentication Player Id of the Reported Player. */
	UPROPERTY(BlueprintReadWrite, Category = "Unity Gaming Services | Moderation | Models")
	FString ReportedUnityPlayerID;

	/*!<The Vivox Sip URI of the Reported Player. */
	UPROPERTY(BlueprintReadWrite, Category = "Unity Gaming Services | Moderation | Models")
	FString ReportedVivoxURI;

	/*!<The Vivox Sip URI of the Reporting Player. */
	UPROPERTY(BlueprintReadWrite, Category = "Unity Gaming Services | Moderation | Models")
	FString ReportingVivoxURI;

	TMap<ChannelId, TArray<AccountId>> VivoxChannels = TMap<ChannelId, TArray<AccountId>>();
};

