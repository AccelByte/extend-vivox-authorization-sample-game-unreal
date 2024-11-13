 /*
 * #####################################################################################
 *  Unity Vivox Plugin for Unreal Engine Copyright © 2024 Unity Technologies
 * #####################################################################################
 */

#pragma once
#include "CoreMinimal.h"
#include "OpenAPIReportApi.h"
#include "OpenAPIReportApiOperations.h"

namespace Moderation 
{

	class IModerationApi
	{

	public:

		virtual ~IModerationApi() {};
		virtual FHttpRequestPtr ReportPost(const OpenAPIReportApi::ReportPostRequest &Request, const OpenAPIReportApi::FReportPostDelegate&Delegate) = 0;
		virtual void AddHeaderParam(const FString& Key, const FString& Value) = 0;
		virtual void ClearHeaderParams() = 0;

	};

}