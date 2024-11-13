 /*
 * #####################################################################################
 *  Unity Vivox Plugin for Unreal Engine Copyright © 2024 Unity Technologies
 * #####################################################################################
 */

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "IModerationApi.h"
#include "Utils/ModerationUtils.h"
#include "Models/Report.h"
#include "Models/ReportResponse.h"

namespace Moderation
{

#if ENGINE_MAJOR_VERSION == 5
	class FModerationApi : public FTSTickerObjectBase
#else
	class FModerationApi : public FTickerObjectBase
#endif
	{

	public:

		/**
		 * @brief Instantiates a new API wrapper without a project Id or environment Id.
		 */
		FModerationApi();

		/**
		 * @brief                Instantiates a new API wrapper with a unique API wrapper instance pointer.
		 * @param Api            The API wrapper instance pointer.
		 */
		FModerationApi(TUniquePtr<IModerationApi> Api);

		/**
		 * @brief           Function overload from FTickerObjectBase/FTSTickerObjectBase.
		 * @param DeltaTime Time (in seconds) between each tick.
		 */
		bool Tick(float DeltaTime) final;

		/**
		 * @brief                 Signs in the current user anonymously. No credentials are required and the session is confined to the current device.
		 * @param Report          A constructed Report object with the Reported User's Unity Authentication PlayerId and the Report Reason
		 * @param ResponseHandler Handler that executes when the API has returned a response.
		 */
		void ReportPost(FReport Report, FString UASToken, Moderation::THandler<FReportResponse> ResponseHandler = Moderation::THandler<FReportResponse>());

		/**
		 * @brief Resets variables for moderation operations.
		 */
		void Flush();

	private:
		
		/*!< A pointer to the report post request object. */
		FHttpRequestPtr ReportPostRequestPtr;
		/*!< Handler that executes when a report response has been received. */
		Moderation::THandler<FReportResponse> ReportDelegate;

		TUniquePtr<Moderation::IModerationApi> Api;

		void HandleErrorResponse(TSharedPtr<FJsonValue> JsonParsed);

		void OnReportPost(const OpenAPIReportApi::ReportPostResponse& ReportResponse);
	};

}