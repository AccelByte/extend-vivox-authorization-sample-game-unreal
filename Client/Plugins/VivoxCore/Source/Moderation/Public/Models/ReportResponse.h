 /*
 * #####################################################################################
 *  Unity Vivox Plugin for Unreal Engine Copyright © 2024 Unity Technologies
 * #####################################################################################
 */

#pragma once
#include "CoreMinimal.h"
#include "Interfaces/IHttpResponse.h"
#include "ReportResponse.generated.h"

/**
 * @brief Struct representing a moderation report response.
 */
USTRUCT(BlueprintType)
struct MODERATION_API FReportResponse
{
	GENERATED_BODY()

	/*!< Whether the response was a success. */
	UPROPERTY(BlueprintReadOnly, Category = "Unity Gaming Services | Moderation")
	bool bWasSuccessful = false;
};