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

#include "OpenAPIErrorsErrorResponseBody.h"

#include "ModerationModule.h"
#include "OpenAPIHelpers.h"

#include "Templates/SharedPointer.h"

namespace Moderation
{

void OpenAPIErrorsErrorResponseBody::WriteJson(JsonWriter& Writer) const
{
	Writer->WriteObjectStart();
	if (Code.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("code")); WriteJsonValue(Writer, Code.GetValue());
	}
	if (Detail.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("detail")); WriteJsonValue(Writer, Detail.GetValue());
	}
	if (Instance.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("instance")); WriteJsonValue(Writer, Instance.GetValue());
	}
	if (Status.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("status")); WriteJsonValue(Writer, Status.GetValue());
	}
	if (Title.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("title")); WriteJsonValue(Writer, Title.GetValue());
	}
	if (Type.IsSet())
	{
		Writer->WriteIdentifierPrefix(TEXT("type")); WriteJsonValue(Writer, Type.GetValue());
	}
	Writer->WriteObjectEnd();
}

bool OpenAPIErrorsErrorResponseBody::FromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	const TSharedPtr<FJsonObject>* Object;
	if (!JsonValue->TryGetObject(Object))
		return false;

	bool ParseSuccess = true;

	ParseSuccess &= TryGetJsonValue(*Object, TEXT("code"), Code);
	ParseSuccess &= TryGetJsonValue(*Object, TEXT("detail"), Detail);
	ParseSuccess &= TryGetJsonValue(*Object, TEXT("instance"), Instance);
	ParseSuccess &= TryGetJsonValue(*Object, TEXT("status"), Status);
	ParseSuccess &= TryGetJsonValue(*Object, TEXT("title"), Title);
	ParseSuccess &= TryGetJsonValue(*Object, TEXT("type"), Type);

	return ParseSuccess;
}

}
