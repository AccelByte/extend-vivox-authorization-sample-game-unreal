/**
 * Unity Moderation Events API
 * No description provided (generated by Openapi Generator https://github.com/openapitools/openapi-generator)
 *
 * OpenAPI spec version: alpha-1.0
 * Contact: mp-suite-player-safety@unity3d.com
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * https://github.com/OpenAPITools/openapi-generator
 * Do not edit the class manually.
 */

#pragma once

#include "OpenAPIBaseModel.h"
#include "OpenAPIReportApi.h"

#include "OpenAPIErrorsErrorResponseBody.h"
#include "OpenAPIModelPostReportRequest.h"

namespace Moderation
{

/* Submit a new report for a user. This resource will trigger the creation of a new Moderation incident. This API resource will always create an incident that has a reporting player, as it can only be called with player authentication.

*/
class MODERATION_API OpenAPIReportApi::ReportPostRequest : public Request
{
public:
    virtual ~ReportPostRequest() {}
	void SetupHttpRequest(const FHttpRequestRef& HttpRequest) const final;
	FString ComputePath() const final;

	/* report payload */
	OpenAPIModelPostReportRequest Payload;
};

class MODERATION_API OpenAPIReportApi::ReportPostResponse : public Response
{
public:
    virtual ~ReportPostResponse() {}
	void SetHttpResponseCode(EHttpResponseCodes::Type InHttpResponseCode) final;
	bool FromJson(const TSharedPtr<FJsonValue>& JsonValue) final;

    
};

}