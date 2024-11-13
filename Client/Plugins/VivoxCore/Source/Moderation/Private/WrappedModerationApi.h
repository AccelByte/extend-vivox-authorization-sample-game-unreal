 /*
 * #####################################################################################
 *  Unity Vivox Plugin for Unreal Engine Copyright © 2024 Unity Technologies
 * #####################################################################################
 */

#pragma once
#include "CoreMinimal.h"
#include "IModerationApi.h"
#include "OpenAPIReportApi.h"

namespace Moderation 
{
	/**
	 * @brief Wrapper class for the generated OpenAPI code.
	 */
	class FWrappedModerationApi : public IModerationApi
	{

	public:

		FWrappedModerationApi();
		virtual ~FWrappedModerationApi() override;
		virtual FHttpRequestPtr ReportPost(const OpenAPIReportApi::ReportPostRequest& Request, const OpenAPIReportApi::FReportPostDelegate& Delegate) override;
		virtual void AddHeaderParam(const FString& Key, const FString& Value) override;
		virtual void ClearHeaderParams() override;

	private:

		TUniquePtr<OpenAPIReportApi> Api;

	};

}