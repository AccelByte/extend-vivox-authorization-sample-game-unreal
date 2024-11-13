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

#include "OpenAPIReportApiOperations.h"

#include "ModerationModule.h"
#include "OpenAPIHelpers.h"

#include "Dom/JsonObject.h"
#include "Templates/SharedPointer.h"
#include "HttpModule.h"
#include "PlatformHttp.h"

namespace Moderation
{

FString OpenAPIReportApi::ReportPostRequest::ComputePath() const
{
	FString Path(TEXT("/report"));
	return Path;
}

void OpenAPIReportApi::ReportPostRequest::SetupHttpRequest(const FHttpRequestRef& HttpRequest) const
{
	static const TArray<FString> Consumes = { TEXT("application/json") };
	//static const TArray<FString> Produces = { TEXT("application/json") };

	HttpRequest->SetVerb(TEXT("POST"));

	// Default to Json Body request
	if (Consumes.Num() == 0 || Consumes.Contains(TEXT("application/json")))
	{
		// Body parameters
		FString JsonBody;
		JsonWriter Writer = TJsonWriterFactory<>::Create(&JsonBody);

		WriteJsonValue(Writer, Payload);
		Writer->Close();

		HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
		HttpRequest->SetContentAsString(JsonBody);
	}
	else if (Consumes.Contains(TEXT("multipart/form-data")))
	{
		UE_LOG(LogModeration, Error, TEXT("Body parameter (payload) was ignored, not supported in multipart form"));
	}
	else if (Consumes.Contains(TEXT("application/x-www-form-urlencoded")))
	{
		UE_LOG(LogModeration, Error, TEXT("Body parameter (payload) was ignored, not supported in urlencoded requests"));
	}
	else
	{
		UE_LOG(LogModeration, Error, TEXT("Request ContentType not supported (%s)"), *FString::Join(Consumes, TEXT(",")));
	}
}

void OpenAPIReportApi::ReportPostResponse::SetHttpResponseCode(EHttpResponseCodes::Type InHttpResponseCode)
{
	Response::SetHttpResponseCode(InHttpResponseCode);
	switch ((int)InHttpResponseCode)
	{
	case 200:
		SetResponseString(TEXT("Report has been successfully created"));
		break;
	case 400:
		SetResponseString(TEXT("Bad Request"));
		break;
	case 500:
		SetResponseString(TEXT("Internal Server Error"));
		break;
	}
}

bool OpenAPIReportApi::ReportPostResponse::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	return true;
}

}
